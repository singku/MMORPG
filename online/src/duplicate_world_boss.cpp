#include "duplicate_world_boss.h"
#include "rank_utils.h"
#include "mail_utils.h"
#include "global_data.h"
#include "player_utils.h"
#include "player.h"
#include "prize.h"
#include "player_manager.h"
#include "duplicate_conf.h"
#include "pet_conf.h"
#include "chat_processor.h"
#include "sys_ctrl.h"

int duplicate_world_boss_mgr_t::init(uint32_t dup_id)
{
    std::map<uint32_t, world_boss_dup_info_t>::iterator iter = 
        world_dup_map_.find(dup_id);
    if (iter == world_dup_map_.end()) {
        world_boss_dup_info_t dup_info;
        dup_info.status = commonproto::WORLD_BOSS_DUP_CLOSED;
        dup_info.boss_hp = 0;
        dup_info.boss_maxhp = 0;
        dup_info.boss_lv = 0;
        dup_info.boss_exp = 0;

        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
        if (dup && dup->mon_vec.size() > 0) {
            uint32_t pet_id = dup->mon_vec[0];
            const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
            if (pet_conf) {
                dup_info.boss_maxhp = 
                    pet_conf->basic_normal_battle_values[kBattleValueNormalTypeHp];
                dup_info.boss_hp = dup_info.boss_maxhp;
            }
        }

        dup_info.start_time = 0;
        dup_info.end_time = 0;
        dup_info.reward_flag = false;
        dup_info.kill_user_key = 0;
        dup_info.kill_flag = false;
        world_dup_map_.insert(
                std::pair<uint32_t, world_boss_dup_info_t>(dup_id, dup_info));
    } else {
        iter->second.status = commonproto::WORLD_BOSS_DUP_CLOSED;
        iter->second.boss_hp = 0;
        iter->second.boss_lv = 0;
        iter->second.boss_exp = 0;
        iter->second.start_time = 0;
        iter->second.end_time = 0;

        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
        if (dup && dup->mon_vec.size() > 0) {
            uint32_t pet_id = dup->mon_vec[0];
            const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
            if (pet_conf) {
                iter->second.boss_maxhp = 
                    pet_conf->basic_normal_battle_values[kBattleValueNormalTypeHp];
                iter->second.boss_hp = iter->second.boss_maxhp;
            }
        }

        iter->second.reward_flag = false;
        iter->second.kill_user_key = 0;
        iter->second.kill_flag = false;
        iter->second.damage_map.clear();
        iter->second.top_n_users.clear();
        iter->second.reward_mask_map.clear();
    }

    return 0;
}

/** 
 * @brief 清除指定服世界boss里的个人信息
 * 
 * @param userid 
 * 
 * @return 
 */
int duplicate_world_boss_mgr_t::clear_player_record(
        uint32_t dup_id, uint32_t userid, 
        uint32_t u_create_tm, uint32_t svr_id) 
{
    if(svr_id == g_online_id) {
        clear_player_damage_record(dup_id, userid, u_create_tm);            
    } 

    std::vector<commonproto::attr_data_t> attr_vec;
    commonproto::attr_data_t attr_data;
    attr_data.set_type(kAttrWorldBossDamage);
    attr_data.set_value(0);
    attr_vec.push_back(attr_data);
    attr_data.set_type(kAttrWorldBossRewardRecord);
    attr_data.set_value(0);
    attr_vec.push_back(attr_data);
    attr_data.set_type(kAttrWorldBossGoldReward);
    attr_data.set_value(0);
    attr_vec.push_back(attr_data);
    attr_data.set_type(kAttrWorldBossLastSvrId);
    attr_data.set_value(0);
    attr_vec.push_back(attr_data);
    AttrUtils::update_other_attr_value(userid, u_create_tm, attr_vec);

    // 清除世界boss排名
    std::ostringstream key;
    key << commonproto::RANKING_TYPE_WORLD_BOSS_DAMAGE_1 << ":" << svr_id;
    RankUtils::rank_del_user(userid, u_create_tm, key.str());

    return 0;
}

/** 
 * @brief 清除世界boss伤害集合里的个人信息
 * 
 * @param dup_id 
 * @param userid 
 * 
 * @return 
 */
int duplicate_world_boss_mgr_t::clear_player_damage_record(
        uint32_t dup_id, uint32_t userid, uint32_t u_create_tm) 
{
    uint64_t user_key = ROLE_KEY(ROLE(userid, u_create_tm));

    std::map<uint32_t, world_boss_dup_info_t>::iterator iter = 
        world_dup_map_.find(dup_id);
    if (iter != world_dup_map_.end()) {
        iter->second.damage_map.erase(user_key);
        iter->second.reward_mask_map.erase(user_key);

        FOREACH(iter->second.top_n_users, it) {
            if ((*it).user_key == user_key) {
                iter->second.top_n_users.erase(it);
                break;
            }
        }
        if (iter->second.kill_user_key == user_key) {
            iter->second.kill_user_key = 0;
        }
    }

    return 0;
}

/** 
 * @brief 清空世界boss信息集合
 * 
 * @param dup_id 
 * 
 * @return 
 */
int duplicate_world_boss_mgr_t::clear_dup_record(uint32_t dup_id) 
{
    std::map<uint32_t, world_boss_dup_info_t>::iterator iter = 
        world_dup_map_.find(dup_id);
    if (iter != world_dup_map_.end()) {
        init(dup_id);
    }

    // 清除伤害排行榜
    std::ostringstream key;
    key << commonproto::RANKING_TYPE_WORLD_BOSS_DAMAGE_1 << ":" << g_online_id;
    RankUtils::redis_del_key_str(0, 0, 0, key.str(), rankproto::REDIS_KEY_TYPE_ZSET);
    return 0;
}

uint32_t duplicate_world_boss_mgr_t::get_next_status_left_time(uint32_t dup_id)
{
    uint32_t left_time = 0xFFFFFFFF;
    world_boss_dup_info_t *dup_info = get_world_boss_dup_info(dup_id);
    if (dup_info == NULL) {
        return left_time;
    } 

    uint32_t time_config_id = 3;
    uint32_t conf_num = TimeUtils::get_sub_time_config_num(time_config_id);

    // 建立日期时间索引(周一到周日)
    if (stage_time_index.size() == 0) {
        for (uint32_t i = 1; i <= conf_num; i++) {
            const time_limit_t *time_conf = 
                TimeUtils::get_time_limit_config(time_config_id, i);
            FOREACH(time_conf->weekdays, iter) {
                std::map<uint32_t, world_boss_time_config_vec_t>::iterator iter2 = 
                    stage_time_index.find(*iter);
                if (iter2 == stage_time_index.end()) {
                    world_boss_time_config_vec_t stage_time_vec; 
                        world_boss_stage_time_t stage_time;
                    if (i % 2 == 1) {
                        stage_time.stage = commonproto::WORLD_BOSS_DUP_READY;
                    } else {
                        stage_time.stage = commonproto::WORLD_BOSS_DUP_FIGHT;
                    }

                    stage_time.time_limit = *time_conf;
                    stage_time_vec.push_back(stage_time);
                    stage_time_index.insert(
                            std::pair<uint32_t, world_boss_time_config_vec_t>(*iter, stage_time_vec));
                } else {
                    world_boss_stage_time_t stage_time;
                    if (i % 2 == 1) {
                        stage_time.stage = commonproto::WORLD_BOSS_DUP_READY;
                    } else {
                        stage_time.stage = commonproto::WORLD_BOSS_DUP_FIGHT;
                    }

                    stage_time.time_limit = *time_conf;
                    iter2->second.push_back(stage_time);
                }
            }  
        }
    }

	uint32_t status = commonproto::WORLD_BOSS_DUP_CLOSED;
    struct tm now_tm =  *get_now_tm();
    uint32_t now_wday = now_tm.tm_wday;
    if (now_wday == 0) {
        now_wday = 7;
    }

    uint32_t same_day_flag = false;
    std::map<uint32_t, world_boss_time_config_vec_t>::iterator iter = 
        stage_time_index.find(now_wday);
    if (iter != stage_time_index.end()) {
    // 计算当天的活动阶段
        FOREACH(iter->second, iter2) {
            world_boss_stage_time_t *stage_time = &(*iter2);
            time_limit_t *time_conf = &(stage_time->time_limit);

            struct tm tmp_start_tm;
            struct tm tmp_end_tm;

            tmp_start_tm.tm_year = now_tm.tm_year;
            tmp_start_tm.tm_mon = now_tm.tm_mon;
            tmp_start_tm.tm_mday = now_tm.tm_mday;
            tmp_start_tm.tm_hour = time_conf->start_hour;
            tmp_start_tm.tm_min = time_conf->start_min;
            tmp_start_tm.tm_sec = time_conf->start_second;
            time_t tmp_start_time = mktime(&tmp_start_tm);

            tmp_end_tm.tm_year = now_tm.tm_year;
            tmp_end_tm.tm_mon = now_tm.tm_mon;
            tmp_end_tm.tm_mday = now_tm.tm_mday;
            tmp_end_tm.tm_hour = time_conf->end_hour;
            tmp_end_tm.tm_min = time_conf->end_min;
            tmp_end_tm.tm_sec = time_conf->end_second;
            time_t tmp_end_time = mktime(&tmp_end_tm);

            time_t nowtime = mktime(&now_tm);

            if (stage_time->stage  == commonproto::WORLD_BOSS_DUP_READY) {
                if (nowtime < tmp_start_time) {
                    // 距离当天下次开放时间
                    left_time = tmp_start_time - nowtime;
                    status = commonproto::WORLD_BOSS_DUP_CLOSED;
                    same_day_flag = true;
                    break;
                } else if (nowtime >= tmp_start_time && nowtime < tmp_end_time) {
                    // 距离开始挑战时间
                    left_time = tmp_end_time - nowtime;
                    status = commonproto::WORLD_BOSS_DUP_READY;
                    same_day_flag = true;
                    break;
                }
            } else if (stage_time->stage == commonproto::WORLD_BOSS_DUP_FIGHT) {
                // 距离挑战结束时间
                if (nowtime >= tmp_start_time && nowtime < tmp_end_time) {
                    left_time = tmp_end_time - nowtime;
                    status = commonproto::WORLD_BOSS_DUP_FIGHT;
                    same_day_flag = true;
                    break;
                }
            }
        }
    } else {
        same_day_flag = false;
    }

    // 当天不在活动时间内,遍历找到下一次活动开始时间
    if (!same_day_flag) {
        uint32_t step_day = 1;
        bool find_flag = false;
        for (uint32_t i = now_wday + step_day; step_day < 7;) {
            std::map<uint32_t, world_boss_time_config_vec_t>::iterator iter = 
                stage_time_index.find(i);
            if (iter != stage_time_index.end()) {
                FOREACH(iter->second, iter2) {
                    world_boss_stage_time_t *stage_time = &(*iter2);
                    time_limit_t *time_conf = &(stage_time->time_limit);

                    struct tm tmp_start_tm;

                    tmp_start_tm.tm_year = now_tm.tm_year;
                    tmp_start_tm.tm_mon = now_tm.tm_mon;
                    tmp_start_tm.tm_mday = now_tm.tm_mday;
                    tmp_start_tm.tm_hour = time_conf->start_hour;
                    tmp_start_tm.tm_min = time_conf->start_min;
                    tmp_start_tm.tm_sec = time_conf->start_second;
                    time_t tmp_start_time = mktime(&tmp_start_tm) + 24 * 3600 * step_day;

                    time_t nowtime = mktime(&now_tm);
                    if (tmp_start_time > nowtime) {
                        left_time = tmp_start_time - nowtime;
                        find_flag = true;
                        status = commonproto::WORLD_BOSS_DUP_CLOSED;
                        break;
                    }
                }
            }

            if (find_flag) {
                break;
            }

            ++step_day;
            i = now_wday + step_day;
            if (i > 7) {
                i = i % 7;
            }

            // 只循环检查一次
            if (i == now_wday) {
                break;
            }
        }
    }

    return left_time;
}

uint32_t duplicate_world_boss_mgr_t::get_reward_status(
        uint32_t dup_id, uint32_t reward_mask)
{
    uint32_t ret = commonproto::WORLD_BOSS_REWARD_NOT_OPEN;
    if ((reward_mask & 0xf) > 0 && 
            (reward_mask & 0x000f0000) == 0) {
        ret = commonproto::WORLD_BOSS_REWARD_OPEN;
    } else if ((reward_mask & 0x000f0000) > 0) {
        ret = commonproto::WORLD_BOSS_REWARD_ALREADY_GET;
    }
    return ret;
}

/** 
 * @brief 批量计算并发放世界boss挑战奖励
 * 
 * @param dup_id 世界boss副本id
 *   离线规则
     挑战期间发奖前下线，上线同一服，不清除数据，可以继续挑战
     挑战期间发奖后下线，上线同一服，不清除数据，邮件发奖，不能进入世界boss副本，提示已获得奖励
     挑战期间发奖前下线，上线不同服，不清除原服数据，进入世界boss副本前,如果原服发奖,会获得邮件通知。如果进入过世界boss副本，在线时只发本服奖励， 不会发放来自其他服的奖励通知。如果再次在本服发奖前下线，就按本服状态和下次登陆服进入规则循环。
     挑战期间发奖后下线，上线不同服，不清除原服数据，登陆时邮件发奖，奖励包来自参与过并在最后发奖的挑战服， 不能进入世界副本，提示已获得奖励
     奖励在每轮活动中每人只发放一次。
 * 
 * @return 
 */
int duplicate_world_boss_mgr_t::give_world_boss_reward(uint32_t dup_id)
{
    world_boss_dup_info_t *dup_info = 
        g_world_boss_mgr.get_world_boss_dup_info(dup_id);
    if (dup_info == NULL) {
        return cli_err_world_boss_info_not_exist;
    }

    if (dup_info->reward_flag) {
        return 0;
    }

    // <user_key, reward_mask>
    // std::map<uint64_t, uint32_t> reward_mask_map;

#define UPDATE_REWARD_MASK(user_key, bit_num) \
    do { \
        if (!(bit_num >= 1 && bit_num <= 32)) {  \
            break;  \
        }   \
            \
        uint32_t status = (1 << (bit_num - 1)); \
        std::map<uint64_t, uint32_t>::iterator r_iter = \
            dup_info->reward_mask_map.find(user_key); \
        if (r_iter == dup_info->reward_mask_map.end()) {  \
            dup_info->reward_mask_map.insert( \
                    std::pair<uint64_t, uint32_t>(user_key, status));  \
        } else {  \
            r_iter->second |= status; \
        }   \
      } while(0); 

    // 生成奖励记录
    if (dup_info->reward_mask_map.size() == 0) {
        FOREACH(dup_info->damage_map, iter) {
            role_info_t role = KEY_ROLE(iter->first);
            if (!is_valid_uid(role.userid)) {
                continue;
            }

            UPDATE_REWARD_MASK(iter->first, 4);
            // 计算top玩家
            if (dup_info->top_n_users.size() == 0) {
                user_damage_info_t ud_info;
                ud_info.user_key = iter->first;
                dup_info->top_n_users.insert(dup_info->top_n_users.begin(), iter->second);
            } else {
                bool insert_flag = false;
                FOREACH(dup_info->top_n_users, iter2) {
                    if (iter->second.damage > (*iter2).damage) {
                        user_damage_info_t ud_info;
                        ud_info.user_key = iter->first;
                        dup_info->top_n_users.insert(iter2, iter->second);
                        insert_flag = true;
                        break;
                    }
                }
                if (insert_flag == false) {
                    user_damage_info_t ud_info;
                    ud_info.user_key = iter->first;
                    dup_info->top_n_users.insert(dup_info->top_n_users.end(), iter->second);
                }

                if (dup_info->top_n_users.size() > 
                        commonproto::WORLD_BOSS_RANK_REWARD_NUM) {
                    dup_info->top_n_users.pop_back();
                }
            }
        }

        // 击杀才有排名奖
        if (dup_info->kill_user_key) {
            // top n用户奖励
            uint32_t i = 0;
            FOREACH(dup_info->top_n_users, iter) {
                ++i;
                uint64_t user_key = (*iter).user_key;
                //player_t *ol_player = g_player_manager->get_player_by_userid(userid);
                if (i == 1) {
                    UPDATE_REWARD_MASK(user_key, 2);
                } else if (i >= 2 && i<= 3) {
                    UPDATE_REWARD_MASK(user_key, 3);
                }
            }

            // 最后一击奖励
            UPDATE_REWARD_MASK(dup_info->kill_user_key, 1);
        }
    }

    // 奖励发放
    FOREACH(dup_info->reward_mask_map, iter) {
        role_info_t role = KEY_ROLE(iter->first);
        give_world_boss_single_user_reward(
                dup_id, role.userid, role.u_create_tm, iter->second);
    }

    dup_info->reward_flag = true;
    return 0;
}

/** 
 * @brief 给单个用户发放世界boss挑战奖励
 * 
 * @param dup_id 
 * @param userid        得奖用户
 * @param reward_mask   得奖掩码
                        bit1-4 是否可以得奖, bit17-20 是否已发奖,0否1是
 * 
 * @return 
 */
int duplicate_world_boss_mgr_t::give_world_boss_single_user_reward(
        uint32_t dup_id, uint32_t userid, uint32_t u_create_tm, uint32_t &reward_mask)
{
    if (userid == 0) {
        return 0;
    }

    world_boss_dup_info_t *dup_info = 
        g_world_boss_mgr.get_world_boss_dup_info(dup_id);
    if (dup_info == NULL) {
        return cli_err_world_boss_info_not_exist;
    }

    // 可以发奖
    if (get_reward_status(dup_id, reward_mask) != 
            (uint32_t)commonproto::WORLD_BOSS_REWARD_OPEN) {
        return 0;
    }


    player_t *player = g_player_manager->get_player_by_userid(userid);

    // 判断是否已发奖
    if (player) {
        if (get_reward_status(dup_id, GET_A(kAttrWorldBossRewardRecord)) == 
                (uint32_t)commonproto::WORLD_BOSS_REWARD_ALREADY_GET) {
            return 0;
        }
    }

    // 最后一击奖励
    if (taomee::test_bit_on(reward_mask, 1) && 
            !taomee::test_bit_on(reward_mask, 17)) {
        if (player) {
            uint32_t prize_id = 9954;
            std::vector<cache_prize_elem_t> prize_vec;
            prize_vec.clear();
            transaction_pack_prize(player, prize_id, prize_vec);
            if (prize_vec.size()) {
                new_mail_t new_mail;
                new_mail.sender.assign("系统邮件");
                new_mail.title.assign("获得了击杀世界boss奖励");
                new_mail.content.assign("最后一击奖");
                std::string attachment;
                MailUtils::serialize_prize_to_attach_string(prize_vec, attachment);
                new_mail.attachment = attachment;
                MailUtils::add_player_new_mail(player, new_mail);
            } 

            role_info_t role = KEY_ROLE(dup_info->kill_user_key);
            TRACE_LOG("world boss reward, type:kill, user:%u(%u), player:%p(%u) ", 
                    role.userid, role.u_create_tm, player, player->userid);

            reward_mask = taomee::set_bit_on(reward_mask, 17);
        }
    }

    // 击杀金箱奖
    if (taomee::test_bit_on(reward_mask, 2) &&
            !taomee::test_bit_on(reward_mask, 18)) {
        if (player) {
            uint32_t prize_id = 9953;
            std::vector<cache_prize_elem_t> prize_vec;
            prize_vec.clear();
            transaction_pack_prize(player, prize_id, prize_vec);
            if (prize_vec.size()) {
                std::string title_str = "获得了挑战世界boss金箱奖励";
                new_mail_t new_mail;
                new_mail.sender.assign("系统邮件");
                new_mail.title.assign(title_str.c_str());
                new_mail.content.assign("击杀金箱奖励");
                std::string attachment;
                MailUtils::serialize_prize_to_attach_string(prize_vec, attachment);
                new_mail.attachment = attachment;
                MailUtils::add_player_new_mail(player, new_mail);
            } 
            TRACE_LOG("world boss reward, type:rank 1, user:%u, player:%p(%u) ", 
                    userid, player, player->userid);
            reward_mask = taomee::set_bit_on(reward_mask, 18);
        }
    }

    // 击杀银箱奖
    if (taomee::test_bit_on(reward_mask, 3) &&
            !taomee::test_bit_on(reward_mask, 19)) {
        if (player) {
            uint32_t prize_id = 9952;
            std::vector<cache_prize_elem_t> prize_vec;
            prize_vec.clear();
            transaction_pack_prize(player, prize_id, prize_vec);
            if (prize_vec.size()) {
                std::string title_str = "获得了挑战世界boss银箱奖励";
                new_mail_t new_mail;
                new_mail.sender.assign("系统邮件");
                new_mail.title.assign(title_str.c_str());
                new_mail.content.assign("击杀银箱奖励");
                std::string attachment;
                MailUtils::serialize_prize_to_attach_string(prize_vec, attachment);
                new_mail.attachment = attachment;
                MailUtils::add_player_new_mail(player, new_mail);
            } 
            TRACE_LOG("world boss reward, type:rank 2, user:%u, player:%p(%u) ", 
                    userid, player, player->userid);
            reward_mask = taomee::set_bit_on(reward_mask, 19);
        }
    }

    // 参与伤害奖
    uint32_t add_gold_num = 0;
    if (taomee::test_bit_on(reward_mask, 4) &&
            !taomee::test_bit_on(reward_mask, 20)) {
        if (player) {
            uint64_t user_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
            // 全员参与伤害奖励
            uint32_t prize_id = 9951;
            std::vector<cache_prize_elem_t> prize_vec;
            prize_vec.clear();
            transaction_pack_prize(player, prize_id, prize_vec);

            // 伤害金币计算
            uint32_t damage = 0;
            std::map<uint64_t, user_damage_info_t>::iterator damage_iter = 
                dup_info->damage_map.find(user_key);
            if (damage_iter != dup_info->damage_map.end()) {
                damage = damage_iter->second.damage; 
            }

            cache_prize_elem_t gold_prize;
            add_gold_num = ((damage/200) >  10000) ? 10000 : (damage/200);
            if(add_gold_num) {
                gold_prize.type =  (uint32_t)commonproto::PRIZE_ELEM_TYPE_ITEM;
                gold_prize.id = 31004;
                gold_prize.count = add_gold_num;
            }
            prize_vec.push_back(gold_prize);

            if (prize_vec.size()) {
                new_mail_t new_mail;
                new_mail.sender.assign("系统邮件");
                new_mail.title.assign("获得了世界boss参与伤害奖励");
                new_mail.content.assign("参与伤害奖励");
                std::string attachment;
                MailUtils::serialize_prize_to_attach_string(prize_vec, attachment);
                new_mail.attachment = attachment;
                MailUtils::add_player_new_mail(player, new_mail);
            } 
            TRACE_LOG("world boss reward, type:all, user:%u, player:%p(%u) ", 
                    userid, player, player->userid);
            reward_mask = taomee::set_bit_on(reward_mask, 20);
        }
    }

    // 修改属性并发送通知
    std::vector<commonproto::attr_data_t> attr_vec;
    commonproto::attr_data_t attr_data;
    attr_data.set_type(kAttrWorldBossRewardRecord);
    attr_data.set_value(reward_mask);
    attr_vec.push_back(attr_data);
    attr_data.set_type(kAttrWorldBossGoldReward);
    attr_data.set_value(add_gold_num);
    attr_vec.push_back(attr_data);
    AttrUtils::update_other_attr_value(userid, u_create_tm, attr_vec);

    return 0;
}

uint32_t duplicate_world_boss_mgr_t::get_current_start_time(uint32_t dup_id)
{
    world_boss_dup_info_t *dup_info = get_world_boss_dup_info(dup_id);
    if (dup_info == NULL) {
        return 0;
    }

    if (dup_info->status == commonproto::WORLD_BOSS_DUP_FIGHT) {
        return dup_info->start_time;
    }
    return 0;
}

/** 
 * @brief 计算传入时间所在的世界boss活动场次开始时间或结束时间
 * 
 * @param dup_id        世界boss副本id
 * @param timestamp     传入时间 
 * @param flag          0 开始时间 1结束时间
 * 
 * @return  0 不在活动时间内 >0 返回对应时间
 */
uint32_t duplicate_world_boss_mgr_t::compute_start_end_time(
        uint32_t dup_id, time_t timestamp, uint32_t flag)
{
    struct tm tm_time;
    localtime_r(&timestamp, &tm_time);

    /*
    bool fight_flag = TimeUtils::is_time_valid(timestamp, 3, 2);
    if (!fight_flag) {
        return 0;
    }
    */
    
    const time_limit_t *time_conf = TimeUtils::get_time_limit_config(3, 2);

    struct tm tmp_start_tm;
    tmp_start_tm.tm_year = tm_time.tm_year;
    tmp_start_tm.tm_mon = tm_time.tm_mon;
    tmp_start_tm.tm_mday = tm_time.tm_mday;
    tmp_start_tm.tm_hour = time_conf->start_hour;
    tmp_start_tm.tm_min = time_conf->start_min;
    tmp_start_tm.tm_sec = time_conf->start_second;
    time_t tmp_start_time = mktime(&tmp_start_tm);

    struct tm tmp_end_tm;
    tmp_end_tm.tm_year = tm_time.tm_year;
    tmp_end_tm.tm_mon = tm_time.tm_mon;
    tmp_end_tm.tm_mday = tm_time.tm_mday;
    tmp_end_tm.tm_hour = time_conf->end_hour;
    tmp_end_tm.tm_min = time_conf->end_min;
    tmp_end_tm.tm_sec = time_conf->end_second;
    time_t tmp_end_time = mktime(&tmp_end_tm);

    if (flag) {
        return tmp_end_time;
    }

    return tmp_start_time;
}

uint32_t duplicate_world_boss_mgr_t::world_boss_sys_notify(
        uint32_t type, uint32_t dup_id, std::vector<uint32_t> &paras)
{
    std::string infos[5] = {};

    infos[0] = g_module_mgr.get_module_conf_string_def(module_type_world_boss, "notify_before_start", "");
    infos[1] = g_module_mgr.get_module_conf_string_def(module_type_world_boss, "notify_time_limit", "");
    infos[2] = g_module_mgr.get_module_conf_string_def(module_type_world_boss, "notify_start", "");
    infos[3] = g_module_mgr.get_module_conf_string_def(module_type_world_boss, "notify_low_hp", "");
    infos[4] = g_module_mgr.get_module_conf_string_def(module_type_world_boss, "notify_kill", "");

    if (!(type >= 1 && type <= array_elem_num(infos))) {
        return 0;
    }

    if (type == commonproto::WORLD_BOSS_STATUS_NOTI_KILL) {
        std::string substrs[] = {"$1","$2"};
        std::map<uint32_t, world_boss_dup_info_t>::iterator iter = 
            world_dup_map_.find(dup_id);
        if (iter == world_dup_map_.end()) {
            return cli_err_world_boss_info_not_exist;
        }

        std::string &msg_str = infos[type-1];
        std::string u_name;
        std::string gap_str("、");
        FOREACH(iter->second.top_n_users, iter2) {
            std::list<user_damage_info_t>::iterator tmp_iter = iter2;
            tmp_iter++;
            if (tmp_iter != iter->second.top_n_users.end()) {
                u_name += (*iter2).nick + gap_str;
            } else {
                u_name += (*iter2).nick;
            }
        }

        std::string &sub_str = substrs[0];
        uint32_t pos = msg_str.find(sub_str);
        msg_str.replace(pos, sub_str.length(), u_name);

        u_name.clear();
        std::map<uint64_t, user_damage_info_t>::iterator iter2 = 
            iter->second.damage_map.find(iter->second.kill_user_key);
        if (iter2 != iter->second.damage_map.end()) {
            sub_str = substrs[1];
            pos = msg_str.find(sub_str);
            msg_str.replace(pos, sub_str.length(), iter2->second.nick);
        }
    }

    SystemNotify::SystemNotifyNormal(infos[type - 1]);

    return 0;
}
