#include "equip_processor.h"
#include "player_utils.h"
#include "proto/client/pb0x01.pb.h"
#include "data_proto_utils.h"
#include "global_data.h"
#include "item_conf.h"
#include "map_utils.h"
#include "task_utils.h"
#include "equip_utils.h"
#include "rank_utils.h"
#include "cultivate_equip_conf.h"

int EquipLevelUpCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
	int ret = PlayerUtils::player_equip_level_up(player, cli_in_.slot_id());
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }
	//牙牙活动范围内属性记录
	AttrUtils::add_attr_in_special_time_range(player,
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivCompoundEquipCnt);

    ADD_A(kAttrGoldVipActivity150611Equips, 1);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int HideFashionCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    bool hide = (bool)(GET_A(kAttrHideFashion));
    if (cli_in_.hide() == hide) {
        return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    }
    if (cli_in_.hide()) {
        SET_A(kAttrHideFashion, 1);
    } else {
        SET_A(kAttrHideFashion, 0);
    }

    //通知包
    MapUtils::sync_map_player_info(player, commonproto::PLAYER_EQUIP_CHANGE);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int CultivateEquipCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    int ret = 0;
    uint32_t flag = 0; 

    if (cli_in_.auto_flag()) {
        // 自动培养 
        int i = 0;
        while (flag == 0 && i < 100) {
            ret = EquipUtils::cultivate_equip(
                    player, cli_in_.type(), cli_in_.auto_buy_flag(), flag);
            if (ret) {
                break;
            }
            i++;
        }
    } else {
        // 培养一次
        ret = EquipUtils::cultivate_equip(
                player, cli_in_.type(), cli_in_.auto_buy_flag(), flag);
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd, ret);
        }
    }

    //通知包
    MapUtils::sync_map_player_info(player, commonproto::PLAYER_EQUIP_CHANGE);

    cli_out_.set_result(false);
    if (flag) {
        cli_out_.set_result(true);
    }
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}


int EquipQuenchCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    uint32_t slot_id = cli_in_.slot_id();
    item_t *item = player->package->get_mutable_item_in_slot(slot_id);
    if (!item) {
        RET_ERR(cli_err_equip_not_found);
    }
    const item_conf_t* item_conf = g_item_conf_mgr.find_item_conf(item->item_id);
    if(!item_conf) {
        RET_ERR(cli_err_item_not_exist);
    }
    if (!g_item_conf_mgr.is_equip(item->item_id)) {
        RET_ERR(cli_err_item_is_not_equipment);
    }
    if (!g_item_conf_mgr.is_quench_equip(item->item_id)) {
        RET_ERR(cli_err_non_quench_equip);
    }

    commonproto::item_optional_attr_t opt_attr;
    opt_attr.ParseFromString(item->opt_attr);

    if (opt_attr.level() < EQUIP_QUALITY_BLUE) {//蓝色品质才开启第一重洗练
        RET_ERR(cli_err_quench_blue_required);
    }

    //判断消耗品是否足够
    uint32_t quench_item_id = 0;
    uint32_t quench_item_cnt = 1;
    uint32_t quench_buy_product;
    if (opt_attr.level() >= EQUIP_QUALITY_PURPLE) {
        quench_item_id = 37003;
        quench_buy_product = 25053;
    } else {
        quench_item_id = 37001;
        quench_buy_product = 25051;
    }

    //消耗的锁数量
    uint32_t lock_item_id = 37002;
    uint32_t lock_item_cnt = 0;
    uint32_t lock_buy_product = 25052;
    if (opt_attr.level() >= EQUIP_QUALITY_PURPLE 
        && opt_attr.level() < EQUIP_QUALITY_PURPLE3) {//第一二个锁有效
        if (cli_in_.lock_1() == true || cli_in_.lock_2() == true) {
            lock_item_cnt = 1;
        }

    } else if (opt_attr.level() >= EQUIP_QUALITY_PURPLE3) {//三个锁都有效
        if (cli_in_.lock_1() == true) {
            lock_item_cnt++;
        }
        if (cli_in_.lock_2() == true) {
            lock_item_cnt++;
        }
        if (cli_in_.lock_3() == true) {
            lock_item_cnt++;
        }
    }
    lock_item_cnt = lock_item_cnt >= 3 ?2 :lock_item_cnt;

    int ret = 0;
    //没有开自动购买
    if (!cli_in_.auto_buy_item()) {
        std::vector<reduce_item_info_t> reduce_vec;
        reduce_item_info_t reduce;
        reduce.item_id = lock_item_id;
        reduce.count = lock_item_cnt;
        reduce_vec.push_back(reduce);

        reduce.item_id = quench_item_id;
        reduce.count = quench_item_cnt;
        reduce_vec.push_back(reduce);

        ret = check_swap_item_by_item_id(player, &reduce_vec, 0);
        if (ret) {
            RET_ERR(ret);
        }

    }

    uint32_t gold_cnt = opt_attr.level() * 100;

    uint32_t buy_need_gold = 0;
    uint32_t buy_need_diamond = 0;
    uint32_t buy_lock_cnt = 0;
    //判断锁
    if (lock_item_cnt) {
        uint32_t cur_cnt = player->package->get_total_usable_item_count(lock_item_id);
        if (cur_cnt < lock_item_cnt) {
            buy_lock_cnt = lock_item_cnt - cur_cnt;
            const product_t *pd = g_product_mgr.find_product(lock_buy_product);
            if (pd) {
                if (pd->price_type == 1) {
                    buy_need_gold += pd->price * buy_lock_cnt;
                } else if (pd->price_type == 2) {
                    buy_need_diamond += pd->price * buy_lock_cnt;
                }
            }
        }
    }

    uint32_t buy_quench_cnt = 0;
    //判断道具
    if (quench_item_id) {
        uint32_t cur_cnt = player->package->get_total_usable_item_count(quench_item_id);
        if (cur_cnt < quench_item_cnt) {
            buy_quench_cnt = quench_item_cnt - cur_cnt;
            const product_t *pd = g_product_mgr.find_product(quench_buy_product);
            if (pd) {
                if (pd->price_type == 1) {
                    buy_need_gold += pd->price * buy_quench_cnt;
                } else if (pd->price_type == 2) {
                    buy_need_diamond += pd->price * buy_quench_cnt;
                }
            }
        }
    }

    if (!AttrUtils::is_player_gold_enough(player, gold_cnt + buy_need_gold)) {
        RET_ERR(cli_err_gold_not_enough);
    }

    if (player_get_diamond(player) < buy_need_diamond) {
        RET_ERR(cli_err_lack_diamond);
    }
    std::vector<buy_product_t> buy_vec;
    if (buy_lock_cnt) {
        buy_product_t inf;
        inf.pd_id = lock_buy_product;
        inf.buy_cnt = buy_lock_cnt;
        buy_vec.push_back(inf);
        /*
        ret = buy_product(player, lock_buy_product, buy_lock_cnt);
        if (ret) {
            RET_ERR(ret);
        }
        */
    }
    if (buy_quench_cnt) {
        buy_product_t inf;
        inf.pd_id = quench_buy_product;
        inf.buy_cnt = buy_quench_cnt;
        buy_vec.push_back(inf);
        /*
        ret = buy_product(player, quench_buy_product, buy_quench_cnt);
        if (ret) {
            RET_ERR(ret);
        }
        */
    }
    ret = batch_buy_product(player, buy_vec, "洗练自动购买");
    if (ret) {
        RET_ERR(ret);
    }
    std::vector<reduce_item_info_t> reduce_vec;
    reduce_item_info_t reduce;
    reduce.item_id = lock_item_id;
    reduce.count = lock_item_cnt;
    reduce_vec.push_back(reduce);

    reduce.item_id = quench_item_id;
    reduce.count = quench_item_cnt;
    reduce_vec.push_back(reduce);

    ret = swap_item_by_item_id(player, &reduce_vec, 0);
    if (ret) {
        RET_ERR(ret);
    }

    if (opt_attr.level() >= EQUIP_QUALITY_BLUE) {//可进行第一重洗练
        if (cli_in_.lock_1() == false) {//第一重未锁定 洗
            uint32_t idx = EquipUtils::random_select_quench_1_type();

            //隐藏属性
            uint32_t min, max;
            if (idx >= commonproto::EQUIP_QUENCH_TYPE_1_CRIT && idx <= commonproto::EQUIP_QUENCH_TYPE_1_BREAK_BLOCK) {
                min = 1;
                max = opt_attr.level() * 60;
            //主属性
            } else if (idx >= commonproto::EQUIP_QUENCH_TYPE_1_HP && idx <= commonproto::EQUIP_QUENCH_TYPE_1_SDEF) {
                // uint32_t x = get_equip_attr_factor((equip_add_attr_t)idx);
                uint32_t x = get_equip_attr_factor(equip_main_attr_adapter( (commonproto::equip_quench_type_t)idx));
                double p = opt_attr.level() + 0.6 * opt_attr.star();
                min = x * (p * p - p + 2) / 3;
                max = x * (p * p - p + 2);
            //抗性
            } else {
                min = 1;
                max = opt_attr.level() * 40;
            }
            uint32_t value = EquipUtils::random_quench_value(min, max);
            opt_attr.set_quench_1_type(idx);
            opt_attr.set_quench_1_value(value);
        }
    }

    if (opt_attr.level() >= EQUIP_QUALITY_PURPLE) {//可进行第二重洗练
        if (cli_in_.lock_2() == false) {//第二重未锁定 洗
            uint32_t idx = ranged_random(commonproto::EQUIP_QUENCH_TYPE_2_EQUIP_MAIN_ATTR,
                    commonproto::EQUIP_QUENCH_TYPE_2_PET_MINOR_ATTR);
            uint32_t value = EquipUtils::random_quench_value(1, 50);
            opt_attr.set_quench_2_type((commonproto::equip_quench_type_t)idx);
            opt_attr.set_quench_2_value(value);
        }
    }

    if (opt_attr.level() >= EQUIP_QUALITY_PURPLE3) {//可进行第三重洗练
        if (cli_in_.lock_3() == false) {//第三重未锁定 洗
            rand_add_equip_buff(opt_attr, item_conf->base_buff4_ID, true);
        }
    }
    item->opt_attr.clear();
    opt_attr.SerializeToString(&(item->opt_attr));

    //扣金币
    AttrUtils::sub_player_gold(player, gold_cnt, "装备洗练");
    std::vector<item_t> items;
    items.push_back(*item);
    db_item_change(player, items, onlineproto::SYNC_REASON_QUENCH, true);
    PlayerUtils::calc_player_battle_value(player);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int SetEquipShowStatusCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    if (cli_in_.type() == 0) {
        uint32_t flag = GET_A(kAttrMountShowFlag);
        cultivate_equip_conf_t *cult_conf = 
            g_cultivate_equip_mgr.find_cultivate_equip_conf(CULTIVATE_TYPE_MOUNT);
        if (cult_conf == NULL) {
           RET_ERR(cli_err_mount_level_illegal);
        }
        bool noti_flag = false;
        if (cli_in_.show_flag() && flag == 0) {
            // 更新坐骑buff_id
            uint32_t pos = GET_A(kAttrOtherEquip2);
            if (pos) {
                const item_t *item = player->package->get_const_item_in_slot(pos);
                if (item) {
                    int ret = g_item_conf_mgr.can_equip_cultivate_item(player, item->item_id);
                    if (ret) {
                        return send_err_to_player(player, player->cli_wait_cmd, ret);
                    }

                    SET_A(kAttrMountShowFlag, 1);
                    const item_conf_t* item_conf = g_item_conf_mgr.find_item_conf(item->item_id);
                    if (item_conf && item_conf->buff_id) {
                        SET_A(kAttrMountShowFlag, item_conf->buff_id);
                    } else {
                        SET_A(kAttrMountShowFlag, cult_conf->default_buff_id);
                    }
                }
                noti_flag = true;
            }
        } else if (!cli_in_.show_flag() && flag) { 
            SET_A(kAttrMountShowFlag, 0);
            noti_flag = true;
        }

        if (noti_flag) {
            //通知包
            MapUtils::sync_map_player_info(player, commonproto::PLAYER_EQUIP_CHANGE);
        }
    } else {
        uint32_t flag = GET_A(kAttrWingShowFlag);
        cultivate_equip_conf_t *cult_conf = 
            g_cultivate_equip_mgr.find_cultivate_equip_conf(CULTIVATE_TYPE_WING);
        if (cult_conf == NULL) {
            RET_ERR(cli_err_wing_level_illegal);
        }
        bool noti_flag = false;
        if (cli_in_.show_flag() && flag == 0) {
            // 更新翅膀buff_id
            uint32_t pos = GET_A(kAttrOtherEquip3);
            if (pos) {
                const item_t *item = player->package->get_const_item_in_slot(pos);
                if (item) {
                    int ret = g_item_conf_mgr.can_equip_cultivate_item(player, item->item_id);
                    if (ret) {
                        return send_err_to_player(player, player->cli_wait_cmd, ret);
                    }

                    SET_A(kAttrWingShowFlag, 1);
                    const item_conf_t* item_conf = g_item_conf_mgr.find_item_conf(item->item_id);
                    if (item_conf && item_conf->buff_id) {
                        SET_A(kAttrWingShowFlag, item_conf->buff_id);
                    } else {
                        SET_A(kAttrWingShowFlag, cult_conf->default_buff_id);
                    }
                }
                noti_flag = true;
            }
        } else if (!cli_in_.show_flag() && flag) { 
            SET_A(kAttrWingShowFlag, 0);
            noti_flag = true;
        }

        if (noti_flag) {
            //通知包
            MapUtils::sync_map_player_info(player, commonproto::PLAYER_EQUIP_CHANGE);
        }
    }

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int GetMountBattleValueCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    uint32_t battle_value = 0;
    std::map<uint32_t, uint32_t> tmp_attr_map;

    uint32_t level = cli_in_.level();
    if (cli_in_.type() == 0) {
        cultivate_equip_conf_t *cult_conf = 
            g_cultivate_equip_mgr.find_cultivate_equip_conf(CULTIVATE_TYPE_MOUNT);
        if (cult_conf) {
            if (level) {
                std::map<uint32_t, cultivate_equip_level_info_t>::iterator iter = 
                    cult_conf->equip_info_map_.find(level);
                if (iter == cult_conf->equip_info_map_.end()) {
                    ERROR_TLOG("uid:%u create_tm:%u cult_item level not exist, level:%u", 
                            player->userid, player->create_tm, level);
                    RET_ERR(cli_err_mount_level_illegal);
                }

                cultivate_equip_level_info_t *level_conf = &(iter->second);
                FOREACH(level_conf->add_attrs_vec_, iter) {
                    tmp_attr_map[(*iter).type] += (*iter).value;
                }

                battle_value = PlayerUtils::calc_attr_to_battle_value(player, tmp_attr_map);
                RankUtils::rank_user_insert_score(
                    player->userid, player->create_tm,
                    (uint32_t)commonproto::RANKING_MOUNT, 
                    0, battle_value);
            }
        }
    } else {
         cultivate_equip_conf_t *cult_conf = 
            g_cultivate_equip_mgr.find_cultivate_equip_conf(CULTIVATE_TYPE_WING);

        if (cult_conf) {
            if (level) {
                std::map<uint32_t, cultivate_equip_level_info_t>::iterator iter = 
                    cult_conf->equip_info_map_.find(level);
                if (iter == cult_conf->equip_info_map_.end()) {
                    ERROR_TLOG("uid:%u create_tm:%u cult_item level not exist, level:%u", 
                            player->userid, player->create_tm, level);
                    RET_ERR(cli_err_wing_level_illegal);
                }

                cultivate_equip_level_info_t *level_conf = &(iter->second);
                FOREACH(level_conf->add_attrs_vec_, iter) {
                    tmp_attr_map[(*iter).type] += (*iter).value;
                }

                battle_value = PlayerUtils::calc_attr_to_battle_value(player, tmp_attr_map);
                RankUtils::rank_user_insert_score(
                    player->userid, player->create_tm,
                    (uint32_t)commonproto::RANKING_WING, 
                    0, battle_value);
            }
        }
    }

    cli_out_.set_battle_value(battle_value);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
