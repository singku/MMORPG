#include "common.h"
#include "equip_utils.h"
#include "attr_utils.h"
#include "cultivate_equip_conf.h"
#include "global_data.h"
#include "item_conf.h"
#include "player.h"
#include "player_utils.h"

int EquipUtils::cultivate_equip(
        player_t *player, uint32_t type, uint32_t buy_flag, uint32_t &up_flag)
{
    if (player == NULL || player->package == NULL) {
        return cli_err_data_error;
    }

    if (!(type == 0 || type == 1)) {
        return cli_err_data_error;
    }

    uint32_t cult_attr[][4] = {
        // 坐骑
        {CULTIVATE_TYPE_MOUNT, kAttrOtherEquip2, kAttrMountLevel, kAttrMountCultivateValue},
        // 翅膀
        {CULTIVATE_TYPE_WING, kAttrOtherEquip3, kAttrWingLevel, kAttrWingCultivateValue},
    };


    // 培养坐骑翅膀
    up_flag = 0;
    uint32_t cult_lv = GET_A((attr_type_t)cult_attr[type][2]);
    cultivate_equip_conf_t *cult_conf = 
        g_cultivate_equip_mgr.find_cultivate_equip_conf(cult_attr[type][0]);
    if (cult_conf == NULL) {
        return cli_err_data_error;
    }

    if (cult_lv >= cult_conf->max_level) {
        int err = cli_err_mount_level_max;
        if (type) {
            err = cli_err_wing_level_max;
        }
        return err;
    }

    std::map<uint32_t, cultivate_equip_level_info_t>::iterator iter = 
        cult_conf->equip_info_map_.find(cult_lv + 1);
    if (iter == cult_conf->equip_info_map_.end()) {
        int err = cli_err_mount_level_illegal;
        if (type) {
            err = cli_err_wing_level_illegal;
        }
        return err;
    }
    cultivate_equip_level_info_t *level_conf = &(iter->second);

    // 检查物品
    std::vector<reduce_item_info_t> reduce_vec;
    FOREACH(level_conf->cost_item_vec_, iter) {
        uint32_t item_id = (*iter).item_id;
        uint32_t item_num  = (*iter).num;
        uint32_t usable_cnt = player->package->get_total_usable_item_count(item_id);
        if (usable_cnt < item_num) {
            //uint32_t need_num = item_num - usable_cnt;
            if (buy_flag) {
                ////物品不足 自动购买
                //std::map<uint32_t, uint32_t> item_product_map;
                //item_product_map[16201]=52201;
                //item_product_map[16202]=52202;
                //item_product_map[16203]=52203;
                //item_product_map[16204]=52204;
                //if (item_product_map.count(item_id) == 0) {
                //return cli_err_mount_cost_item_illegal;
                //}
                //int err = buy_product(player, item_product_map[item_id], need_num);
                //if (err) {
                //return send_err_to_player(player, player->cli_wait_cmd, err);
                //}
            } else {
                return cli_err_no_enough_item_num;
            }
        }

        reduce_item_info_t rd;
        rd.item_id = item_id;
        rd.count = item_num;
        reduce_vec.push_back(rd);
    }

    // 扣除道具，增加祝福值
    int err = check_swap_item_by_item_id(player, &reduce_vec, 0);
    if (err) {
        return err;
    }

    // TODO toby config item
    std::map<uint32_t, uint32_t> exchg_map;
    exchg_map[39601]=18;
    exchg_map[39602]=19;
    exchg_map[39603]=20;
    exchg_map[39604]=21;
    FOREACH(reduce_vec, iter) {
        uint32_t item_id = (*iter).item_id;
        uint32_t item_num = (*iter).count;
        if (exchg_map.count(item_id) > 0) {
            int err = transaction_proc_exchange(player, exchg_map[item_id], item_num);
            if (err) {
                return send_err_to_player(player, player->cli_wait_cmd, err);
            }
        }
    }

    // 升阶判断
    uint32_t bless_value = GET_A((attr_type_t)cult_attr[type][3]);
    uint32_t succ_prob = 0;
    if (bless_value/level_conf->need_cultivate_value >= 1) {
        succ_prob = 100;
    }

	if (type == 0) {
		AttrUtils::add_attr_in_special_time_range(player,
				TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
				kAttrActivUpMountLvCnt);	
	} else if (type == 1) {
		AttrUtils::add_attr_in_special_time_range(player, 
				TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
				kAttrActivUpWingLvCnt);	
	}

    FOREACH(level_conf->step_probs_map_, iter) {
        if (bless_value * 100.0 / level_conf->need_cultivate_value < iter->first) {
            succ_prob = iter->second;
            break;
        }
    }

    uint32_t rand_idx = rand()%100;
    if (rand_idx < succ_prob) {
        ADD_A((attr_type_t)cult_attr[type][2], 1);
        up_flag = 1;
        SET_A((attr_type_t)cult_attr[type][3], 0);

        PlayerUtils::calc_player_battle_value(player, onlineproto::ATTR_OTHER_REASON);

        // 升阶奖励
        if (level_conf->lv_reward_item) {
            if (!player->package->has_item(level_conf->lv_reward_item, false)) {
                add_single_item(player, level_conf->lv_reward_item, 1);
            }
        }
    }

    return 0;
}

int EquipUtils::add_cult_equip_level_reward(player_t *player)
{
    if (player == NULL) {
        return 0;
    }
    // 检查是否拥有升阶坐骑
    uint32_t mount_level = GET_A(kAttrMountLevel);
    cultivate_equip_conf_t *mount_conf = 
        g_cultivate_equip_mgr.find_cultivate_equip_conf(CULTIVATE_TYPE_MOUNT);
    if (mount_conf) {
        FOREACH(mount_conf->equip_info_map_, iter) {
            cultivate_equip_level_info_t *level_conf = &(iter->second);
            if (level_conf == NULL) {
                continue;
            }

            if (mount_level >= iter->first && 
                    level_conf->lv_reward_item > 0 &&
                !player->package->has_item(level_conf->lv_reward_item, false)) {
                add_single_item(player, level_conf->lv_reward_item, 1);
            }
        }
    }

    // 检查是否拥有升阶翅膀
    uint32_t wing_level = GET_A(kAttrWingLevel);
    cultivate_equip_conf_t *wing_conf = 
        g_cultivate_equip_mgr.find_cultivate_equip_conf(CULTIVATE_TYPE_WING);
    if (wing_conf) {
        FOREACH(wing_conf->equip_info_map_, iter) {
            cultivate_equip_level_info_t *level_conf = &(iter->second);
            if (level_conf == NULL) {
                continue;
            }

            if (wing_level >= iter->first && 
                    level_conf->lv_reward_item > 0 &&
                !player->package->has_item(level_conf->lv_reward_item, false)) {
                add_single_item(player, level_conf->lv_reward_item, 1);
            }
        }
    }

    return 0;
}

int EquipUtils::cultivate_equip_addition(
        player_t *player, std::map<uint32_t, uint32_t> &attr_map)
{
    if (player == NULL || player->package == NULL) {
        return 0;
    }
    uint32_t cult_attr[][3] = {
        // 坐骑
        {CULTIVATE_TYPE_MOUNT, kAttrOtherEquip2, kAttrMountLevel},
        // 翅膀
        {CULTIVATE_TYPE_WING, kAttrOtherEquip3, kAttrWingLevel},
    };

    for (uint32_t i = 0; i < array_elem_num(cult_attr);i++) {
        cultivate_equip_conf_t *cult_conf = 
            g_cultivate_equip_mgr.find_cultivate_equip_conf(cult_attr[i][0]);
        if (cult_conf == NULL) {
            continue;
        }

        uint32_t level = GET_A((attr_type_t)cult_attr[i][2]);
        if (level == 0) {
            continue;
        }

        //uint32_t pos_id = GET_A((attr_type_t)cult_attr[i][1]);
        //if (pos_id ==0) {
            //continue;
        //}

        //const item_t *item = player->package->get_const_item_in_slot(pos_id);
        //if (item == NULL) {
            //ERROR_TLOG("uid:%u create_tm:%u slot_id no item exist, slot_id:%u", 
				//player->userid, player->create_tm, pos_id);
            //return cli_err_item_not_exist;
        //}

        //if (!g_item_conf_mgr.is_cultivate_equip(item->item_id)) {
            //ERROR_TLOG("uid:%u create_tm:%u cult_item not exist, slot_id:%u, item_id:%u", 
				//player->userid, player->create_tm, pos_id, item->item_id);
            //int err = cli_err_mount_item_not_exist;
            //if (cult_attr[i][0] == CULTIVATE_TYPE_WING) {
                //err = cli_err_wing_item_not_exist;
            //}
            //return err;
        //}

        std::map<uint32_t, cultivate_equip_level_info_t>::iterator iter = 
            cult_conf->equip_info_map_.find(level);
        if (iter == cult_conf->equip_info_map_.end()) {
            //ERROR_TLOG("uid:%u create_tm:%u cult_item level not exist, slot_id:%u, item_id:%u, level:%u", 
				//player->userid, player->create_tm, pos_id, item->item_id, level);
            ERROR_TLOG("uid:%u create_tm:%u cult_item level not exist, type:%u, level:%u", 
				player->userid, player->create_tm, cult_conf->type, level);
            int err = cli_err_mount_level_illegal;
            if (cult_attr[i][0] == CULTIVATE_TYPE_WING) {
                err = cli_err_wing_level_illegal;
            }
            return err;
        }

        // 属性数值加成
        cultivate_equip_level_info_t *level_conf = &(iter->second);
        FOREACH(level_conf->add_attrs_vec_, iter) {
            attr_map[(*iter).type] += (*iter).value;
        }
    }

    return 0;
}

int EquipUtils::add_init_mount(player_t *player) 
{
    if (player == NULL || player->package == NULL) {
        return 0;
    }

    cultivate_equip_conf_t *cult_conf = 
            g_cultivate_equip_mgr.find_cultivate_equip_conf(CULTIVATE_TYPE_MOUNT);
    if (cult_conf == NULL) {
        return 0;
    }

    if (cult_conf->init_level > 0 &&
            GET_A(kAttrLv) < cult_conf->init_level) {
        return 0;
    }

    if (GET_A(kAttrMountInit) == 0) {
        uint32_t item_id = cult_conf->init_item;
        if (item_id > 0 && 
                player->package->get_total_item_count(item_id) == 0) {
            add_single_item(player, item_id, 1);
        }

        SET_A(kAttrMountInit, 1);
    }

    return 0;
}


int EquipUtils::add_init_wing(player_t *player) 
{
    if (player == NULL || player->package == NULL) {
        return 0;
    }

    cultivate_equip_conf_t *cult_conf = 
            g_cultivate_equip_mgr.find_cultivate_equip_conf(CULTIVATE_TYPE_WING);
    if (cult_conf == NULL) {
        return 0;
    }

    if (cult_conf->init_level > 0 &&
            GET_A(kAttrLv) < cult_conf->init_level) {
        return 0;
    }

    if (GET_A(kAttrWingInit) == 0) {
        uint32_t item_id = cult_conf->init_item;
        if (item_id > 0 && 
                player->package->get_total_item_count(item_id) == 0) {
            add_single_item(player, item_id, 1);
        }

        SET_A(kAttrWingInit, 1);
    }

    return 0;
}

int EquipUtils::update_trial_cult_equip_expire_time(
        player_t *player, uint32_t expire_time)
{
    if (player == NULL || player->package == NULL) {
        return 0;
    }

    cultivate_equip_conf_t *cult_conf = 
        g_cultivate_equip_mgr.find_cultivate_equip_conf(CULTIVATE_TYPE_WING);
    if (cult_conf && cult_conf->svip_trial_id) {
        if (!player->package->has_item(cult_conf->svip_trial_id, false)) {
            add_single_item(player, cult_conf->svip_trial_id, 1);
        } else { 
            std::vector<item_t*> item_list;
            player->package->get_items_by_id(cult_conf->svip_trial_id, item_list);

            std::vector<item_t> change_item_vec;
            FOREACH(item_list, iter) {
                (*iter)->expire_time = 0;
                change_item_vec.push_back(*(*iter));
            }

            int ret = db_item_change(
                    player, change_item_vec, onlineproto::SYNC_REASON_NONE, true);
            if (ret) {
                return ret;
            }
        }
    }

    return 0;
}

uint32_t EquipUtils::calc_equip_btl_value(commonproto::item_optional_attr_t &equip_opt_attr)
{
    uint32_t battle_value = 0;

    //属性值
    for (int i = 0; i < equip_opt_attr.equip_attrs().attrs_size(); i++) {
        uint32_t type = equip_opt_attr.equip_attrs().attrs(i).type();
        uint32_t value = equip_opt_attr.equip_attrs().attrs(i).value();
        double x = 0;
        switch(type) {
            case EQUIP_ADD_ATTR_HP:
                x = 0.5;
                break;
            case EQUIP_ADD_ATTR_NATK:
            case EQUIP_ADD_ATTR_SATK:
                x = 1.5;
                break;
            case EQUIP_ADD_ATTR_NDEF:
            case EQUIP_ADD_ATTR_SDEF:
                x = 3.0;
                break;
            default:
                x = 1.0;
        }
        battle_value += x*value;
    }
    //转换比
    battle_value += 10 * equip_opt_attr.pet_major_attr_trans_rate();
    battle_value += 10 * equip_opt_attr.quench_2_value();
    battle_value += 40 * equip_opt_attr.pet_minor_attr_trans_rate();

    //魔法属性
    battle_value += equip_opt_attr.magic_power();

    //属性石加属性石战力
    if (item_conf_mgr_t::is_valid_elem_type(equip_opt_attr.elem_type())) {
        uint32_t rate = item_conf_mgr_t::get_equip_elem_damage_rate(equip_opt_attr.level());
        uint32_t anti = item_conf_mgr_t::get_equip_anti_value(equip_opt_attr.level());
        battle_value += rate + anti;
    }
    return battle_value;
}

