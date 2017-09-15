#include "duplicate_utils.h"
#include <boost/lexical_cast.hpp>
#include "package.h"
#include "item.h"
#include "player.h"
#include "duplicate_conf.h"
#include "global_data.h"
#include "pet_utils.h"
#include "service.h"
#include "buff_conf.h"
#include "chat_processor.h"
#include "buff.h"
#include "mail_utils.h"
#include "duplicate_processor.h"
#include "player_utils.h"
#include "bless_pet_utils.h"

bool DupUtils::can_send_to_battle(player_t *player)
{
    if (player->temp_info.dup_state != PLAYER_DUP_PLAY) {
        return false;
    }
    return true;
}

int DupUtils::send_to_battle(player_t *player, uint32_t cmd,
        const google::protobuf::Message &msg, uint32_t wait_svr)
{
#if 1
    duplicate_battle_type_t type = 
        DupUtils::get_player_duplicate_battle_type(player);

    //pvp
    if (type == DUP_BTL_TYPE_RPVP || type == DUP_BTL_TYPE_PPVE) {
        return g_battle_center->send_msg(wait_svr ?player :0, player->userid, player->create_tm, cmd, msg);
    } else {
        return g_battle->send_msg(wait_svr ?player :0, player->userid, player->create_tm, cmd, msg);
    }
#endif
    return g_battle->send_msg(wait_svr ?player :0, player->userid, player->create_tm, cmd, msg);
}

bool DupUtils::is_player_in_duplicate(player_t *player) 
{
    return g_duplicate_conf_mgr.duplicate_exist(
            player->temp_info.dup_id);
}

bool DupUtils::is_duplicate_passed(player_t *player, uint32_t dup_id)
{
    if (player == NULL) {
        return false;
    }

    uint32_t pass_time = GET_A(AttrUtils::get_duplicate_pass_time_attr(dup_id));
    if (pass_time) {
        return true;
    }

    return false;
}

duplicate_battle_type_t DupUtils::get_player_duplicate_battle_type(player_t *player)
{
    const duplicate_t *dup;
    if (player->temp_info.dup_id) {
        dup = g_duplicate_conf_mgr.find_duplicate(player->temp_info.dup_id);
    } else {
        dup = g_duplicate_conf_mgr.find_duplicate(player->temp_info.ready_enter_dup_id);
    }
    if (!dup) {
        return DUP_BTL_TYPE_ERR;
    }
    return dup->battle_type;
}

int DupUtils::can_enter_duplicate(player_t *player, uint32_t dup_id)
{
    //是否存在
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
    if (!dup) {
        return cli_err_duplicate_id_not_found;
    }
	//防沉迷设置
	if( check_player_addicted_threshold_none(player)){
		ERROR_TLOG("%u exceed online time threshold time, can't enter dup:%u", player->userid, dup_id);
		return cli_err_try_to_enter_dup_with_addiction;
	}	
	//该副本指定的精灵是否处于出战位
	if (!(dup->can_fight_pets_id.empty())) {
		std::vector<uint32_t> not_in_fight_pets;
		PetUtils::check_pets_is_in_fight_pos(player,
				dup->can_fight_pets_id,
				not_in_fight_pets);
		if (!not_in_fight_pets.empty()) {
			return cli_err_dup_assign_pet_not_in_fight_pos;
		}
	}

    //是否开放
    if (dup->open_tm_sub_key 
        && !TimeUtils::is_current_time_valid(TM_CONF_KEY_DUP_OPEN, dup->open_tm_sub_key)) {
        return cli_err_duplicate_closed;
    }

    //前置副本
    if (dup->prev_id) {
        uint32_t pass_time = GET_A(AttrUtils::get_duplicate_pass_time_attr(dup->prev_id));
        //前置副本通关时间若是0，则说明没有玩过前置副本
        if (pass_time == 0) {
            return cli_err_cur_dup_id_still_lock;
        }
    }
    //等级满足
    if (GET_A(kAttrLv) < dup->level_limit) {
        return cli_err_level_too_low;
    }
    //体力满足
    if (GET_A(kAttrCurVp) < dup->consume_physique) {
        return cli_err_lack_physique;
    }
    //道具满足
    FOREACH(dup->consume_items, it) {
        uint32_t item_id = it->item_id;
        uint32_t cnt = it->cnt;
        uint32_t usable_total = 0;
        std::vector<item_t*> item_vec;
        usable_total = player->package->get_total_usable_item_count(item_id);
        if (cnt > usable_total) {
            return cli_err_no_enough_item_num;
        }
    }
    //金币满足
    if (!AttrUtils::is_player_gold_enough(player, dup->consume_gold)) {
        return cli_err_gold_not_enough;
    }
    //钻石满足
    if (player_get_diamond(player) < dup->consume_diamond) {
        return cli_err_lack_diamond;
    }
    //cd时间内
    uint32_t last_tm = GET_A(AttrUtils::get_duplicate_last_play_time_attr(dup_id));
    if (dup->cd_time && last_tm + dup->cd_time > NOW()) {
        return cli_err_in_cd_time;
    }
    //剩余次数
    attr_type_t cnt_attr;
    if (dup->limit_type == DUP_LIMIT_DAILY) {
        cnt_attr = AttrUtils::get_duplicate_daily_times_attr(dup_id);
    } else {
        cnt_attr = AttrUtils::get_duplicate_weekly_times_attr(dup_id);
    }
    
    //有次数限制的情况下次数用光
    if (dup->initial_cnt && GET_A(cnt_attr) == 0) {
        return cli_err_duplicate_cnt_run_out;
    }

    // 判断是否已接取任务
    if (g_task_conf_mgr.is_task_conf_exist(dup->task_id)) {
        if (!player->task_info->is_in_task_list(dup->task_id)) {
            return cli_err_not_finish_dup_task;
        }
    }

    // 检查多副本共用限定次数
    uint32_t left_free_cnt = 0;
    uint32_t left_buy_cnt = 0;
    if (dup->total_enter_count_limit_key > 0) {
        left_free_cnt = GET_A((attr_type_t)dup->total_enter_count_limit_key);
    }

    if (dup->buy_total_count_limit_key > 0) {
        left_buy_cnt = GET_A((attr_type_t)dup->buy_total_count_limit_key);
    }

    if ((dup->total_enter_count_limit_key > 0 || 
            dup->buy_total_count_limit_key > 0) &&
            (left_free_cnt + left_buy_cnt) <= 0) {
            return cli_err_duplicate_cnt_run_out;
    }

    return 0;
}


/** 
 * @brief 扣除副本消耗
 * 
 * @param player 
 * @param dup_id 副本id
 * 
 * @return 
 */
int DupUtils::reduce_duplicate_cost(player_t *player, uint32_t dup_id)
{
    int ret = 0;
    SUB_A(kAttrCurVp, DupUtils::duplicate_need_vp(dup_id));
    const std::vector<duplicate_consume_item_t>* reduce_items 
        = DupUtils::duplicate_need_items(dup_id);
    if (reduce_items) {
        std::vector<reduce_item_info_t> reduce_vec;
        FOREACH((*reduce_items), it) {
            reduce_item_info_t reduce;
            reduce.item_id = (*it).item_id;
            reduce.count = (*it).cnt;
            reduce_vec.push_back(reduce);
        }
        ret = swap_item_by_item_id(player, &reduce_vec, 0, false);
        if (ret) {
            ERROR_TLOG("Duplicate reduce item failed :%u", ret);
        }
    }
    //钻石金币扣减
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
    //次数扣减
    attr_type_t cnt_attr;
    if (dup->limit_type == DUP_LIMIT_DAILY) {
        cnt_attr = AttrUtils::get_duplicate_daily_times_attr(dup->duplicate_id);
    } else {
        cnt_attr = AttrUtils::get_duplicate_weekly_times_attr(dup->duplicate_id);
    }
    SUB_A(cnt_attr, 1);

    // 多副本共用次数扣减
    bool sub_buy_cnt_flag = false;
    if (dup->buy_total_count_limit_key > 0) {
        uint32_t left_buy_cnt = GET_A((attr_type_t)dup->buy_total_count_limit_key);
        if(left_buy_cnt > 0) {
            SUB_A((attr_type_t)dup->buy_total_count_limit_key, 1);
            sub_buy_cnt_flag = true;
        }
    }

    if (dup->total_enter_count_limit_key > 0 &&
            sub_buy_cnt_flag == false) {
        SUB_A((attr_type_t)dup->total_enter_count_limit_key, 1);
    }

    return ret;
}

void DupUtils::clear_player_dup_info(player_t *player)
{
    player->temp_info.dup_id = 0;
    player->temp_info.dup_map_id = 0;
    player->temp_info.ready_enter_dup_id = 0;
    player->temp_info.dup_state = PLAYER_DUP_NONE;
}

uint32_t DupUtils::duplicate_need_vp(uint32_t dup_id) 
{
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
    if (!dup) {
        return 0;
    }
    return dup->consume_physique;
}

const std::vector<duplicate_consume_item_t>* DupUtils::duplicate_need_items(uint32_t dup_id)
{
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
    if (!dup) {
        return 0;
    }
    return &(dup->consume_items);
}

duplicate_mode_t DupUtils::get_duplicate_mode(uint32_t dup_id) 
{
    return g_duplicate_conf_mgr.get_duplicate_mode(dup_id);
}

string DupUtils::get_duplicate_name(uint32_t dup_id) 
{
    return g_duplicate_conf_mgr.get_duplicate_name(dup_id);
}

int DupUtils::duplicate_reward_dynamic(
        player_t *player, const duplicate_mode_t mode, 
        onlineproto::sc_0x0210_duplicate_notify_result &noti_result)
{
	if(mode == DUP_MODE_TYPE_BLESS_PET){
		const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(player->temp_info.bless_pet_id);
		uint32_t item_id  = pet_conf->talent_item;
		uint32_t item_cnt = 1;
		uint32_t type     = (uint32_t)(commonproto::PRIZE_ELEM_TYPE_ITEM);
		std::string attachment;
		attachment = boost::lexical_cast<string>(type) + "," 
		+ boost::lexical_cast<string>(item_id) + ","
		+ boost::lexical_cast<string>(item_cnt) + ";"; 

		new_mail_t new_mail;
		new_mail.sender.assign("系统邮件");
		new_mail.title.assign("获得伙伴祈福奖励");
		new_mail.content.assign("感谢您协助参与伙伴祈福, 战斗胜利请领取奖励"); 
		new_mail.attachment = attachment;

		uint32_t members_cnt = 1;
		const std::vector<std::string> &info_vec = 
			*(player->temp_info.bless_team_member_info);

		FOREACH(info_vec, it){
			commonproto::battle_player_data_t *btl_info;
			BlessPetUtils::parse_battle_info_from_string(btl_info, *it);
			uint32_t userid    = btl_info->base_info().user_id();
			uint32_t create_tm = btl_info->base_info().create_tm();
			//给队友发奖励
			MailUtils::send_mail_to_player(player, userid,
				   	create_tm, new_mail, 0); 
			members_cnt ++;
		}

		uint32_t prize_id = 0;
		switch(members_cnt){
			case 1:
				prize_id = 11555;
				break;
			case 2:
				prize_id = 11556;
				break;
			case 3:
				prize_id = 11557;
				break;
			case 4:
				prize_id = 11558;
				break;
		}

		onlineproto::sc_0x0112_notify_get_prize noti_msg;
		noti_msg.Clear();
		transaction_proc_prize(player, prize_id,
				noti_msg, commonproto::PRIZE_REASON_DUPLICATE);
		noti_result.mutable_award_elems()->MergeFrom(noti_msg.award_elems());

	}	

    return 0;
}
int DupUtils::proc_duplicate_reward(
        player_t *player, const duplicate_t *dup, 
        onlineproto::sc_0x0210_duplicate_notify_result &noti_result)
{
    if (player == NULL || dup == NULL) {
        return 0;
    }

    if (dup->battle_type == DUP_BTL_TYPE_PPVE) {
        if (GET_A(kDailyPpvePrizeCnt) >= GET_A_MAX(kDailyPpvePrizeCnt)) {
            return 0;
        }
        ADD_A(kDailyPpvePrizeCnt, 1);
    }

	//根据副本类型加特殊奖励
	duplicate_reward_dynamic(player, dup->mode, noti_result);

    onlineproto::sc_0x0112_notify_get_prize noti_msg;
    FOREACH((dup->prize_vec), it) {
        uint32_t prize_id = *it;
        noti_msg.Clear();
        transaction_proc_prize(player, prize_id,
                noti_msg, commonproto::PRIZE_REASON_DUPLICATE);
        noti_result.mutable_award_elems()->MergeFrom(noti_msg.award_elems());
    }
    FOREACH((dup->vip_prize_vec), it) {
        uint32_t prize_id = *it;
        noti_msg.Clear();
        if (is_vip(player)) {
            transaction_proc_prize(player, prize_id,
                    noti_msg, commonproto::PRIZE_REASON_DUPLICATE);
            noti_result.mutable_vip_award_elems()->MergeFrom(noti_msg.award_elems());
        } else {
            std::vector<cache_prize_elem_t> result;
            result.clear();
            transaction_pack_prize(player, prize_id, result);
            FOREACH(result, it) {
                commonproto::prize_elem_t *inf = noti_result.add_vip_award_elems();
                const cache_prize_elem_t &elem = *it;
                inf->set_type((commonproto::prize_elem_type_t)(elem.type));
                inf->set_id(elem.id);
                inf->set_count(elem.count);
                inf->set_level(elem.level);
            }
        }
    }
    FOREACH((dup->svip_prize_vec), it) {
        uint32_t prize_id = *it;
        noti_msg.Clear();
        if (is_gold_vip(player)) {
            transaction_proc_prize(player, prize_id,
                    noti_msg, commonproto::PRIZE_REASON_DUPLICATE);
            noti_result.mutable_vip_award_elems()->MergeFrom(noti_msg.award_elems());
        } else {
            std::vector<cache_prize_elem_t> result;
            result.clear();
            transaction_pack_prize(player, prize_id, result);
            FOREACH(result, it) {
                commonproto::prize_elem_t *inf = noti_result.add_vip_award_elems();
                const cache_prize_elem_t &elem = *it;
                inf->set_type((commonproto::prize_elem_type_t)(elem.type));
                inf->set_id(elem.id);
                inf->set_count(elem.count);
                inf->set_level(elem.level);
            }
        }
    }

    return 0;
}

//重置副本关卡
uint32_t DupUtils::reset_dups_record(player_t* player, uint32_t mode)
{
	if (mode == (uint32_t)DUP_MODE_TYPE_MONSTER_CRISIS) {
		std::vector<uint32_t> dup_ids;
		g_duplicate_conf_mgr.get_dup_ids_by_mode(mode, dup_ids);
		SET_A(kAttrDupLowestLock1Id, dup_ids.front());
		ADD_A(kDailyMonsterCrisisReset, 1);
		//玩家与伙伴回血
		FOREACH(*player->pets, it) {
			Pet* pet = &it->second;
			if (pet == NULL) {
				continue;
			}
			pet->set_mon_cris_hp(pet->max_hp());
			PetUtils::save_pet(player, *pet, false, true);
		}
		SET_A(kMonCrisHp, GET_A(kAttrHpMax));
	}
	return 0;
}

uint32_t DupUtils::can_reset_dups(player_t* player, uint32_t mode)
{
	if (mode == (uint32_t)DUP_MODE_TYPE_MONSTER_CRISIS) {
		uint32_t reset_cnt = GET_A(kDailyMonsterCrisisReset);
		if (is_vip(player)) {
			if (reset_cnt >= MONSTER_CRISIS_RESET_COUNT_LIMIT_VIP) {
				return cli_err_mster_cri_reset_cnt_limit;
			}
		} else {
			if (reset_cnt >= MONSTER_CRISIS_RESET_COUNT_LIMIT) {
				return cli_err_mster_cri_reset_cnt_limit;
			}
		}
	}
	return 0;
}

//登录之后处理：处理霸者领域（属性试练）的数据
uint32_t DupUtils::deal_with_trail_dup_after_login(player_t* player)
{
	// 霸者领域 昨天的免费挑战次数 使用处理：若未使用，则累加  
	// 计算 上次免费进入挑战的时间戳与今日相隔的天数 N (一天的累加一次,但不得超过5次) 
	if (!TimeUtils::is_same_day(GET_A(kAttrLastLogoutTm), NOW())) {
		std::vector<uint32_t> dup_ids_vec;
		g_duplicate_conf_mgr.get_dup_ids_by_mode(
				(uint32_t)DUP_MODE_TYPE_TRIAL, dup_ids_vec);
		attr_type_t cnt_attr;
		uint32_t last_play_tm_max = 0;
		uint32_t total_enter_count_limit_key = kAttrDupTrailFreeEnterCnt;
		FOREACH(dup_ids_vec, it) {
			//500 :2750;   501 :2754;   502 :2758
			attr_type_t attr= AttrUtils::get_duplicate_last_play_time_attr(*it);
			uint32_t last_play_tm = GET_A(attr);
			if (last_play_tm > last_play_tm_max) {
				last_play_tm_max = last_play_tm;
				const duplicate_t* dup = g_duplicate_conf_mgr.find_duplicate(*it);
				if (dup) {
					total_enter_count_limit_key = dup->total_enter_count_limit_key;
				}
			}
		}
		//上次玩霸者领域与今日，一共隔了天数trail_chlge_cnt
		uint32_t trail_chlge_cnt = 0;
		if (last_play_tm_max) { 
			trail_chlge_cnt = TimeUtils::get_days_between(last_play_tm_max, NOW());
		}
		cnt_attr = (attr_type_t)(total_enter_count_limit_key);
		uint32_t owned_count = GET_A(cnt_attr);
		if (trail_chlge_cnt >= 1) {
			if (trail_chlge_cnt >= 6) {
				SET_A(cnt_attr, 5);
			} else {
				if (owned_count + trail_chlge_cnt >= 5) {
					SET_A(cnt_attr, 5);
				} else {
					ADD_A(cnt_attr, trail_chlge_cnt);
				}
			}
		}
		//霸者领域初始次数置为1
		uint32_t count = GET_A((attr_type_t)(total_enter_count_limit_key));
		if (0 == count) {
			SET_A((attr_type_t)(total_enter_count_limit_key), 1);
		}
	}
	return 0;
}

void DupUtils::proc_mon_drop_prize(player_t *player, std::vector<uint32_t> &drop_prize_vec,
        uint32_t pos_x, uint32_t pos_y, bool is_player_dead)
{
    //怪掉落处理
    onlineproto::sc_0x0214_duplicate_dead_pet_prize_notify noti_drop;

    noti_drop.set_x_pos(pos_x);
    noti_drop.set_y_pos(pos_y);

    uint32_t show_type = 0;

    std::vector<cache_prize_elem_t> result;
    result.clear();
    FOREACH(drop_prize_vec, it) {
        uint32_t prize_id = *it;
        std::vector<cache_prize_elem_t> tmp;
        tmp.clear();
        transaction_pack_prize(player, prize_id, tmp);
        uint32_t show = g_prize_conf_mgr.get_prize_show_type(prize_id);
        if (show && tmp.size()) {
            show_type = show;
        }
        result.insert(result.end(), tmp.begin(), tmp.end());
    }

    if (show_type && result.size()) {
        google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> award_elems;
        cache_prize_to_proto_prize(result, award_elems, true);
        SystemNotify::SystemNotifyForPrize(player, &award_elems, 
                (commonproto::system_noti_reason_t)show_type);
    }

    std::vector<cache_prize_elem_t> non_item_prizes;
    uint32_t idx = NOW();
    uint32_t prize_exp = 0; //经验奖励特殊处理
    FOREACH(result, it) {
        cache_prize_elem_t &elem = *it;
        if (elem.type != commonproto::PRIZE_ELEM_TYPE_ITEM) { 
            //非物品奖励直接给玩家
            if (is_player_dead == true 
                && elem.type == commonproto::PRIZE_ELEM_TYPE_ATTR
                && elem.id == kAttrExp) {
                //玩家死了不奖励经验
                continue;
            }
            if (elem.type == commonproto::PRIZE_ELEM_TYPE_ATTR) {
                if (elem.id == kAttrExp) {
                    //NOTI(singku)按比列改变获得的经验的buff do_buff 改变elem.count
                    std::vector<uint32_t> proc_val;
                    proc_val.push_back(elem.count);
                    g_buff_handler.do_buff(player, server_buff_effect_exp_multi, proc_val);
                    elem.count = proc_val[0];
                    //VIP杀小怪额外加成15%
                    if (is_vip(player)) {
                        elem.count = elem.count * 150 / 100;
                    }
                    //出战精灵也会奖励经验
                    prize_exp += elem.count;

                } else if (elem.id == kAttrGold) {
                    //NOTI(singku)按比例改变获得的金币的buff
                    std::vector<uint32_t> proc_val;
                    proc_val.push_back(elem.count);
                    g_buff_handler.do_buff(player, server_buff_effect_gold_multi, proc_val);
                    elem.count = proc_val[0];
                }
            }

            non_item_prizes.push_back(elem);
            onlineproto::duplicate_drop_prize_t *drop = noti_drop.add_required_prizes();
            drop->set_index(0);
            commonproto::prize_elem_t *inf = drop->mutable_display_elems();
            inf->set_type((commonproto::prize_elem_type_t)elem.type);
            inf->set_id(elem.id);
            inf->set_count(elem.count);
            inf->set_level(elem.level);
            continue;
        }
        //物品拆分
        cache_prize_elem_t tmp_elem = elem;
        //TODO(singku)双倍掉落物品 do_buff 改变elem.count
        tmp_elem.count = 1;
        for (int i = 0; i < elem.count; i++) {
            //物品奖励 掉落
            onlineproto::duplicate_drop_prize_t *drop = noti_drop.add_pickable_prizes();
            while (player->temp_info.cache_dup_drop_prize->count(idx) != 0) {
                idx ++;
            }
            drop->set_index(idx);
            commonproto::prize_elem_t *inf = drop->mutable_display_elems();
            inf->set_type((commonproto::prize_elem_type_t)elem.type);
            inf->set_id(elem.id);
            inf->set_count(1);
            inf->set_level(elem.level);
            player->temp_info.cache_dup_drop_prize->insert(make_pair(idx, tmp_elem));
        }
    }
    if (noti_drop.required_prizes_size() || noti_drop.pickable_prizes_size()) { //如果有杀死掉落
        send_msg_to_player(player, cli_cmd_cs_0x0214_duplicate_dead_pet_prize_notify, noti_drop);
    }
    if (!non_item_prizes.empty()) { //非物品奖励不空
        transaction_proc_packed_prize(player, non_item_prizes, 0, 
                commonproto::PRIZE_REASON_NO_REASON, "副本野怪掉落奖励");
        if (prize_exp) {//给出战精灵加经验
            for (int i = 0; i < MAX_FIGHT_POS; i++) {
                Pet *pet = player->fight_pet[i];
                if (!pet || pet->hp() == 0) { //出战精灵死亡不加经验
                    continue;
                }
                uint32_t real_add_exp;
                PetUtils::add_pet_exp(player, pet, prize_exp, real_add_exp, 
                        ADDICT_DETEC, onlineproto::EXP_FROM_BATTLE);
            }
        }
    }

}

int DupUtils::proc_dup_area_prize(
        player_t *player,
        const duplicate_t *dup, uint32_t old_star, uint32_t new_star)
{
    if (dup == NULL) {
        return 0;
    }

    if (dup->area_id == 0) {
        return 0;
    }

    dup_area_prize_t * prize_info = 
        g_dup_area_prize_conf_mgr.get_dup_area_prize_conf(dup->area_id);
    FOREACH(*prize_info, iter) {
        if(old_star < iter->first &&
                new_star >= iter->first) {
            std::vector<cache_prize_elem_t> prize_vec;
            prize_vec.clear();
            transaction_pack_prize(player, iter->second, prize_vec);
            if (prize_vec.size()) {
                new_mail_t new_mail;
                new_mail.sender.assign("系统邮件");
                new_mail.title.assign("获得副本星级奖励");
                std::string tmp_str = "恭喜你" + Utils::to_string(dup->area_id) + "区副本累积获得" + 
                    Utils::to_string(iter->first) + "星,赤瞳特奉上以下大礼，再接再厉哦!";
                new_mail.content.assign(tmp_str);
                std::string attachment;
                MailUtils::serialize_prize_to_attach_string(prize_vec, attachment);
                new_mail.attachment = attachment;
                MailUtils::add_player_new_mail(player, new_mail);

                tmp_str.clear();
                tmp_str = "领取" + Utils::to_string(dup->area_id) + "区" + 
                    Utils::to_string(iter->first) + "星奖励";
                Utils::write_msglog_new(player->userid, "副本星级奖励", "副本", tmp_str);
            } 
            break;
        }
    } 

    return 0;
}

int DupUtils::tell_btl_exit(player_t *player, bool wait_btl)
{
	//统计
	uint32_t dup_id = player->temp_info.dup_id;	
	uint32_t last_tm = GET_A(AttrUtils::get_duplicate_last_play_time_attr(dup_id));
	if(0 == last_tm){//第一次
		string name = g_duplicate_conf_mgr.get_duplicate_name(dup_id);
		string id = Utils::to_string(dup_id);
		string stat;
		stat = "副本"+id+"_"+name;
		Utils::write_msglog_new(player->userid, "副本", "副本初次进入失败人数", stat);
	}

	//退出副本 设置CD时间
	SET_A(AttrUtils::get_duplicate_last_play_time_attr(player->temp_info.dup_id), NOW());
	//如果玩家在新手副本 则需要重置玩家的等级
	if (DupUtils::get_duplicate_mode(player->temp_info.dup_id) == DUP_MODE_TYPE_STARTER) {
		StatDuplicateCmdProcessor::after_starter(player, player->temp_info.dup_id, false);
	} else if (DupUtils::get_duplicate_mode(player->temp_info.dup_id) == DUP_MODE_TYPE_MONSTER_CRISIS) {
		StatDuplicateCmdProcessor::after_monster_crisis(player, player->temp_info.dup_id, false);
	}

	// 主动退出世界boss副本
	SET_A(kAttrWorldBossNextRevivalTime, 0);

    //清除BUFF
    player->buff_id_map->clear();
	PlayerUtils::sync_player_buff(player);

	player->temp_info.tmp_max_hp = GET_A(kAttrHpMax);
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet == NULL) {
			continue;
		}
		pet->set_tmp_max_hp(pet->max_hp());
	}
	PetUtils::retrieve_fight_pet_pos(player);

    battleproto::cs_battle_relay btl_msg;
    btl_msg.Clear();
	btl_msg.set_uid(player->userid);
	btl_msg.set_create_tm(GET_A(kAttrCreateTm));

	uint32_t cmd = cli_cmd_cs_0x0208_duplicate_exit;
    onlineproto::cs_0x0208_duplicate_exit ol_msg;
    string tmp;
    ol_msg.SerializeToString(&tmp);
	btl_msg.set_cmd(cmd);
	btl_msg.set_pkg(tmp);
	DupUtils::send_to_battle(player, btl_cmd_msg_relay, btl_msg, wait_btl);

    if (!wait_btl) {
        if (DupUtils::get_player_duplicate_battle_type(player) == DUP_BTL_TYPE_RPVP) {
            StatDuplicateCmdProcessor::after_rpvp(player, player->temp_info.dup_id, false);
        }
        DupUtils::clear_player_dup_info(player);
    }
    return 0;
}

uint32_t DupUtils::set_mayin_train_gift_state(player_t* player)
{
	uint32_t total_energy = GET_A(kAttrMayinBucketTotalEnergy);
	uint32_t bitcnt = total_energy / commonproto::MAYIN_ENERGY_DIV_UNIT;
	for (uint32_t i = 0; i < bitcnt && i < 2; ++i) {
		if (GET_A(attr_type_t(kAttrMayinGift01RecvState + i)) == 0) {
			SET_A(attr_type_t(kAttrMayinGift01RecvState + i) , 1);
		}
	}
	if (total_energy >= commonproto::MAYIN_ENERGY_TOTAL_HIGH_LIMIT) {
		if (GET_A(kAttrMayinGift03RecvState) == 0) {
			SET_A(kAttrMayinGift03RecvState, 1);
		}
		if (GET_A(kAttrMayinGift04RecvState) == 0) {
			SET_A(kAttrMayinGift04RecvState, 1);
		}
	}
	return 0;
}

uint32_t DupUtils::use_diamond_buy_pass_dup(player_t* player, uint32_t dup_id)
{
	const duplicate_t* dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
	if (dup == NULL) {
		return cli_err_duplicate_id_not_found;
	}
	attr_type_t attr = AttrUtils::get_duplicate_pass_time_attr(dup_id);
	if (!GET_A(attr)) {
		SET_A(attr, NOW());
	}
	attr = AttrUtils::get_duplicate_best_time_attr(dup_id);
	SET_A(attr, 1);
	attr = AttrUtils::get_duplicate_best_star_attr(dup_id);
	SET_A(attr, DUP_PASS_STAR_THREE);
	attr = AttrUtils::get_duplicate_last_play_time_attr(dup_id);
	SET_A(attr, NOW());
	return 0;
}

uint32_t DupUtils::use_diamond_buy_pet_pass_dup(player_t* player,
		uint32_t pet_id, uint32_t dup_id)
{
	uint32_t type = 0;
	uint32_t ret = AttrUtils::get_pet_pass_dup_tm_attr(dup_id, pet_id, type);
	if (ret) {
		return ret;
	}
	attr_type_t attr_type = (attr_type_t)type;
	if (GET_A(attr_type) == 0) {
		SET_A(attr_type, NOW());
	} 
	return 0;
}

uint32_t DupUtils::clean_dup_pass_record(player_t* player, uint32_t dup_id)
{
	attr_type_t attr = AttrUtils::get_duplicate_pass_time_attr(dup_id);
	SET_A(attr, 0);
	attr = AttrUtils::get_duplicate_best_time_attr(dup_id);
	SET_A(attr, 0);
	attr = AttrUtils::get_duplicate_best_star_attr(dup_id);
	SET_A(attr, 0);
	attr = AttrUtils::get_duplicate_last_play_time_attr(dup_id);
	SET_A(attr, 0);
	return 0;	
}

uint32_t DupUtils::clean_mayin_defeat_empire_dup(player_t* player)
{
    uint32_t last_date = TimeUtils::time_to_date(GET_A(kAttrLastLoginTm));
    if (last_date == TimeUtils::get_today_date()) {
		return 0;
	}
	std::vector<uint32_t> dup_ids;
	g_duplicate_conf_mgr.get_dup_ids_by_mode(DUP_MODE_TYPE_MAYIN_DEFEAT_EMPIRE, dup_ids);

	//清理挑战修罗武士vince TODO
	std::vector<uint32_t> tmp_dup;
	g_duplicate_conf_mgr.get_dup_ids_by_mode(DUP_MODE_TYPE_CHALLENGE_DEMON, tmp_dup);
	dup_ids.insert(dup_ids.begin(), tmp_dup.begin(), tmp_dup.end());

	FOREACH(dup_ids, it) {
		DupUtils::clean_dup_pass_record(player, *it);
	}
	return 0;
}

uint32_t DupUtils::clean_daily_activity_dup(player_t* player)
{
	uint32_t last_date = TimeUtils::time_to_date(GET_A(kAttrLastLoginTm));
	if (last_date == TimeUtils::get_today_date()) {
		return 0;
	}
	std::vector<uint32_t> dup_ids;
	g_duplicate_conf_mgr.get_dup_ids_by_mode(DUP_MODE_TYPE_DAILY_ACTIVITY, dup_ids);
	FOREACH(dup_ids, it) {
		DupUtils::clean_dup_pass_record(player, *it);
	}
	return 0;
}
