#include "item.h"
#include "global_data.h"
#include "player.h"
#include "package.h"
#include "service.h"
#include "data_proto_utils.h"
#include "time_utils.h"
#include "attr_utils.h"
#include "user_action_log_utils.h"
#include "utils.h"
#include "item_conf.h"
#include "mail.h"
#include "mail_utils.h"
#include "player_utils.h"
#include "statlogger/statlogger.h"
#include "task_utils.h"
#include "map_utils.h"
#include "achieve.h"
#include "equip_utils.h"

#define SWAP_ITEM_RETURN_ERR(errno) do { \
    if (wait) { \
        return send_err_to_player(player, player->cli_wait_cmd, errno); \
    } else { \
        return errno;  \
    } \
} while (0)

uint32_t check_swap_item_by_item_id(player_t* player,
        std::vector<reduce_item_info_t> *reduce_vec,
        std::vector<add_item_info_t> *add_vec, 
        bool addict_detec,
        std::vector<attachment_t> *attach_vec)
{
    //TODO(singku) 加物品防沉迷机制
	//单纯加物品，需要防沉迷
	if(addict_detec && reduce_vec == NULL && add_vec != NULL){
		if(check_player_addicted_threshold_none(player)){
			//none什么都不给
			FOREACH((*add_vec), it) {
				add_item_info_t &item = *it;
				item.count = 0;
			}
			// ERROR_TLOG("%u exceed online time threshold not item output", 
					// player->userid);
			// return cli_err_addicted_time_threshold_max; 
		} else if(check_player_addicted_threshold_half(player)){
			//物品不发，属性减半
			FOREACH((*add_vec), it) {
				add_item_info_t &item = *it;
                if (g_item_conf_mgr.is_addicted_attr_item(item.item_id)) {
					item.count /= 2;
				} else {
					item.count = 0;
				}
			}
		}
	}

    Package* package = player->package;

    // 要扣的物品
    if (reduce_vec) {
        FOREACH((*reduce_vec), it) {
            const reduce_item_info_t &item = *it;
            const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(item.item_id);
            if (!item_conf) {
                return cli_err_item_not_exist;
            }

            if (item_conf->attr_id == 0) {//非属性道具
                uint32_t total_num = package->get_total_usable_item_count(item.item_id);
                if (total_num < item.count) {
                    return cli_err_no_enough_item_num; 
                }
            } else { //属性道具
                uint32_t value = GET_A(((attr_type_t)item_conf->attr_id));
                if (value < item.count) {
                    if (item_conf->attr_id == kAttrGold) {
                        return cli_err_gold_not_enough;
                    } else {
                        return cli_err_no_enough_item_num;
                    }
                }
            }
        }
    }

    //要加的物品
    if (add_vec) {
        FOREACH((*add_vec), it) {
            add_item_info_t &item = *it;
            const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(item.item_id);
            if (!item_conf) {
                return cli_err_item_not_exist;
            }

            //TODO(singku) 判断要加的物品/属性是否超过每日上限

            if (item_conf->attr_id == 0) {//判断非属性物品是否超过最大拥有
                // 这里某种物品超出上限后导致无法进行主线任务
                uint32_t total_num = package->get_total_item_count(item.item_id);
                if (total_num + item.count > item_conf->own_max) {
                    uint32_t can_cnt = (item_conf->own_max > total_num) ? (item_conf->own_max - total_num) : 0;
                    uint32_t extra_cnt = item.count - can_cnt;
                    item.count = can_cnt;
                    if (attach_vec) {
                        attachment_t attach;
                        attach.type = kMailAttachItem;
                        attach.id = item.item_id;
                        attach.count = extra_cnt;
                        attach_vec->push_back(attach);
                    }
                }
            }
        }
    }
    return 0;
}

int swap_item_by_item_id(player_t* player,
        std::vector<reduce_item_info_t> *reduce_vec,
        std::vector<add_item_info_t> *add_vec, 
        bool wait,
        onlineproto::sync_item_reason_t reason, 
        bool addict_detec)
{
    std::vector<attachment_t> attach_vec;
    int ret = check_swap_item_by_item_id(player, 
            reduce_vec, add_vec, addict_detec, &attach_vec);

    if (ret) {
        SWAP_ITEM_RETURN_ERR(ret);
    }

    std::vector<item_change_log_info_t> item_change_log_list;
    item_change_log_info_t log;

    std::vector<attr_data_info_t> attr_chg_list;
    attr_data_info_t attr_data;

    Package* package = player->package;

    dbproto::cs_change_items db_msg;
    onlineproto::sc_0x010B_sync_change_item item_msg;
    onlineproto::sc_0x0111_sync_attr attr_msg;

    db_msg.Clear();
    item_msg.Clear();
    attr_msg.Clear();

    item_msg.set_reason(reason);

    StatInfo reduce_stat; // 消耗统计
    StatInfo add_stat; // 输出统计

    // 扣除物品
    if (reduce_vec && reduce_vec->size()) {
        FOREACH((*reduce_vec), it) {
            const reduce_item_info_t &item = *it;
            const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(item.item_id);
            if (!item_conf) {
                ERROR_TLOG("no find item conf: %u", item.item_id);
                SWAP_ITEM_RETURN_ERR(cli_err_item_not_exist);
            }

            if (item_conf->attr_id == 0) {//非属性道具
                // memory
                std::vector<item_t> result_list;
                if (package->reduce_item_by_item_id(item.item_id, item.count, result_list)) {
                    ERROR_TLOG("%u not enough item[%u] to reduce", 
                            player->userid, item.item_id);
                    SWAP_ITEM_RETURN_ERR(cli_err_no_enough_item_num);
                }
                for (uint32_t j = 0; j < result_list.size(); j++) {

                    item_t result = result_list[j];
                    // db
                    commonproto::item_info_t* item_info = db_msg.add_item_info_list();
                    DataProtoUtils::pack_item_info(&result, item_info);

                    // client
                    commonproto::item_info_t* item_data = item_msg.add_items();
                    item_data->CopyFrom(*item_info);
                }
                //log
                log.item_id = item.item_id;
                log.chg_val = item.count;
                log.orig_val = package->get_total_item_count(log.item_id) + item.count;
                log.chg_reason = reason;
                log.slot_id = 0;
                item_change_log_list.push_back(log);

            } else { //属性道具
                //memory
                uint32_t attr_value = GET_A(((attr_type_t)item_conf->attr_id));
                if (attr_value < item.count) {
                    SWAP_ITEM_RETURN_ERR(cli_err_no_enough_item_num);
                }
                if (item_conf->attr_id == kAttrGold || item_conf->attr_id == kAttrPaidGold) {
                    AttrUtils::sub_player_gold(player, item.count, "swap_item消耗");
                } else {
                    attr_value -= item.count;
                    attr_data.type = item_conf->attr_id;
                    attr_data.value = attr_value;
                    attr_chg_list.push_back(attr_data);
                }

            }

            reduce_stat.add_info("item_key", item_conf->name);
            reduce_stat.add_op(StatInfo::op_item, "item_key"); 
            reduce_stat.add_info("item_value", item.count);
            reduce_stat.add_op(StatInfo::op_item_sum, "item_key", "item_value");
        }
    }

    if (add_vec && add_vec->size()) {
        // 增加物品
        FOREACH((*add_vec), it) {
            const add_item_info_t &item = *it;
            // if (item.count == 0) {
                // continue;
            // }
            const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(item.item_id);
            if (!item_conf) {
                ERROR_TLOG("no find item conf of %u", item.item_id);
                SWAP_ITEM_RETURN_ERR(cli_err_item_not_exist);
            }

            //TODO(singku)判断是否超过每日上限

            if (item_conf->attr_id == 0) {//非属性物品
                // memory
                item_t item_temp;
                item_temp.item_id = item.item_id;
                item_temp.count = item.count;
                item_temp.slot_id = 0;
                item_temp.using_count = 0;
                item_temp.opt_attr.clear();
                if (g_item_conf_mgr.is_equip(item.item_id)) {
                    init_equip_attr(&item_temp);
                }

                item_temp.expire_time = 0;
                if (item.expire_time) {
                    item_temp.expire_time = item.expire_time;
                } else if (item_conf->expire) {
                    //item_temp.expire_time = TimeUtils::second_at_day_start(item_conf->expire) - 1;
                    item_temp.expire_time = NOW() + item_conf->expire;
                } 
                std::vector<item_t> result_list;
                package->add_item(&item_temp, result_list);

                for (uint32_t j = 0; j < result_list.size(); j++) {
                    item_t result = result_list[j];
                    // db协议
                    commonproto::item_info_t* item_info = db_msg.add_item_info_list();
                    DataProtoUtils::pack_item_info(&result, item_info);

                    // 客户端协议
                    commonproto::item_info_t* item_data = item_msg.add_items();
                    item_data->CopyFrom(*item_info);
                }

                //log
                log.item_id = item.item_id;
                log.chg_val = item.count;
                log.chg_reason = reason;
                log.orig_val = package->get_total_item_count(log.item_id) - item.count;
                log.slot_id = 0;
                item_change_log_list.push_back(log);

            } else {//属性物品
                if (item_conf->attr_id == kAttrExp) {//拦截属性输出
                    uint32_t real_add;
                    PlayerUtils::add_player_exp(player, item.count, &real_add);

                } else if (item_conf->attr_id == kAttrGold || item_conf->attr_id == kAttrPaidGold) {
                    AttrUtils::add_player_gold(player, item.count, false, "swap_item获得");

                } else {
                    uint32_t attr_value = GET_A(((attr_type_t)item_conf->attr_id));
                    attr_value += item.count;
                    attr_data.type = item_conf->attr_id;
                    attr_data.value = attr_value;
                    attr_chg_list.push_back(attr_data);
                }
            }

            add_stat.add_info("item_key", item_conf->name);
            add_stat.add_op(StatInfo::op_item, "item_key");
            add_stat.add_info("item_value", item.count);
            add_stat.add_op(StatInfo::op_item_sum, "item_key", "item_value");
        }
    }

    // 发送到db
    ret = 0;
    if (wait) {
        ret = g_dbproxy->send_msg(player, player->userid,player->create_tm,
                db_cmd_change_items, db_msg);
    } else {
        ret = g_dbproxy->send_msg(NULL, player->userid,player->create_tm, 
                db_cmd_change_items, db_msg);
    }

    if (ret != 0) {
        SWAP_ITEM_RETURN_ERR(cli_err_sys_err);
    }

    // 同步物品
    if (item_msg.items_size()) {
        send_msg_to_player(player, cli_cmd_cs_0x010B_sync_change_item,
                item_msg);
    }

    // 同步属性
    if (attr_chg_list.size()) {
        AttrUtils::set_attr_value(player, attr_chg_list);
    }
    if (reduce_vec && reduce_vec->size()) {
        g_stat_logger->log("道具", "道具消耗", Utils::to_string(player->userid), "", reduce_stat); 
    }

    if (add_vec && add_vec->size()) {
        g_stat_logger->log("道具", "道具输出", Utils::to_string(player->userid), "", add_stat); 
    }

    // 悬赏任务更新
    TaskUtils::update_reward_task_item_step(player, add_vec);

    //如果attach_vec有数据 则发邮件
    if (attach_vec.size()) {
        new_mail_t new_mail;
        new_mail.sender.assign("系统邮件");
        new_mail.title.assign("您的背包已满");
        new_mail.content.assign("由于背包已满,多余的奖励会以系统邮件的方式发送给您,请及时领取附件(邮箱最多存储99封邮件)");
		std::string attachment;
		MailUtils::serialize_attach_to_attach_string(attach_vec, attachment);
		new_mail.attachment = attachment;
        MailUtils::add_player_new_mail(player, new_mail);
    }
    //log
    UserActionLogUtils::log_item_num_change(player, item_change_log_list); 

    return 0;    
}

int db_item_change(player_t *player, const std::vector<item_t>& items, 
        onlineproto::sync_item_reason_t reason, bool noti_player)
{
    onlineproto::sc_0x010B_sync_change_item cli_msg;
    dbproto::cs_change_items db_msg;

    cli_msg.set_reason(reason);

    FOREACH(items, it) {
        const item_t &item = *it;
        commonproto::item_info_t* item_info = db_msg.add_item_info_list();
        DataProtoUtils::pack_item_info(&item, item_info);

        commonproto::item_info_t* item_data = cli_msg.add_items();
        item_data->CopyFrom(*item_info);

        assert(item.item_id);
        assert(g_item_conf_mgr.find_item_conf(item.item_id));
    }

    int ret = g_dbproxy->send_msg(NULL, player->userid,player->create_tm, 
            db_cmd_change_items, db_msg);

    if (ret != 0) {
        ERROR_TLOG("%u send add message to db failed", player->userid); 
        return -1;
    }

    if (noti_player) {
        send_msg_to_player(player, cli_cmd_cs_0x010B_sync_change_item, cli_msg);
    }
    return 0;
}

int clean_expired_items(player_t* player, bool noti_client)
{
    Package* package = player->package;

    std::vector<item_t> result_list;

    package->clean_expired_items(result_list);

    clean_expired_item_related_info(player, result_list);

    if (result_list.size() == 0) {
        return 0; 
    }

    // 前端需要先清空using_cout，再通知count=0，才能清除物品和相关资源
    std::vector<item_t> tmp_result = result_list;
    FOREACH(tmp_result, iter) {
        iter->count += iter->using_count;
        iter->using_count = 0;
    }
    int ret = db_item_change(player, tmp_result, 
            onlineproto::SYNC_REASON_CLEAN_EXPIRED_ITEM, noti_client);
    if (ret) {
        return ret;
    }

    tmp_result = result_list;
    FOREACH(tmp_result, iter) {
        iter->count = 0;
        iter->using_count = 0;
    }
    ret = db_item_change(player, tmp_result, 
            onlineproto::SYNC_REASON_CLEAN_EXPIRED_ITEM, noti_client);
    if (ret) {
        return ret;
    }

    //记载被清除道具的日志
    std::vector<item_change_log_info_t> log_list;
    FOREACH(result_list, it) {
        item_change_log_info_t log;
        log.item_id = it->item_id;
        log.chg_val = it->count;
        log.orig_val = it->count;
        log.slot_id = it->slot_id;
        log.chg_reason = (uint32_t)(onlineproto::SYNC_REASON_CLEAN_EXPIRED_ITEM);
        log_list.push_back(log);
    }
    UserActionLogUtils::log_item_num_change(player, log_list);

    return 0;
}

int clean_expired_item_related_info(
        player_t *player, std::vector<item_t> item_list)
{
    FOREACH(item_list, iter) {
        uint32_t slot_id = GET_A(kAttrOtherEquip2);
        if (iter->slot_id == slot_id) {
            // 清除坐骑相关记录
            SET_A(kAttrOtherEquip2, 0);
            SET_A(kAttrMountShowFlag, 0);
            // 修改base_info，广播到地图
            MapUtils::sync_map_player_info(player, commonproto::PLAYER_EQUIP_CHANGE);
        }

        slot_id = GET_A(kAttrOtherEquip3);
        if (iter->slot_id == slot_id) {
            // 清除翅膀相关记录
            SET_A(kAttrOtherEquip3, 0);
            SET_A(kAttrWingShowFlag, 0);
            // 修改base_info，广播到地图
            MapUtils::sync_map_player_info(player, commonproto::PLAYER_EQUIP_CHANGE);
        }
    }
    return 0;
}

int add_single_item(player_t* player, uint32_t item_id, uint32_t count, uint32_t expire_time,
        bool wait, bool addict_detect, onlineproto::sync_item_reason_t reason)
{
    add_item_info_t add_item = add_item_info_t(item_id, count, expire_time);
    std::vector<add_item_info_t> add_vec;
    add_vec.push_back(add_item);
    int ret = swap_item_by_item_id(player, 0, &add_vec, wait, reason, addict_detect);
    if (ret) {
        KERROR_LOG(player->userid, "swap item err: id %u, count %u", item_id, count);
    }
    return ret;
}

int reduce_single_item(player_t* player, uint32_t item_id, uint32_t count, bool wait,
        bool addict_detect, onlineproto::sync_item_reason_t reason)
{
    reduce_item_info_t reduce_item = {
        item_id,
        count,
    };
    std::vector<reduce_item_info_t> reduce_vec;
    reduce_vec.push_back(reduce_item);
    int ret = swap_item_by_item_id(player, &reduce_vec, 0, wait, reason, addict_detect);
    if (ret) {
        KERROR_LOG(player->userid, "swap item err: id %u, count %u", item_id, count);
    }
    return ret;
}

int reduce_item_by_slot_id(player_t* player,
        uint32_t slot_id,
        uint32_t count,
        bool wait, 
        onlineproto::sync_item_reason_t reason)
{
    std::vector<item_change_log_info_t> item_change_log_list;
    item_change_log_info_t log;

    Package* package = player->package;

    dbproto::cs_change_items db_msg;
    onlineproto::sc_0x010B_sync_change_item item_msg;
    onlineproto::sc_0x0111_sync_attr attr_msg;

    db_msg.Clear();
    item_msg.Clear();
    attr_msg.Clear();

    item_msg.set_reason(reason);

    StatInfo reduce_stat; // 消耗统计

    // 扣除物品
    // memory
    item_t result;
    if (package->reduce_item_by_slot_id(slot_id, count, &result)) {
        ERROR_TLOG("%u not enough item[%u] to reduce", 
                player->userid, result.item_id);
        SWAP_ITEM_RETURN_ERR(cli_err_no_enough_item_num);
    }
    // db
    commonproto::item_info_t* item_info = db_msg.add_item_info_list();
    DataProtoUtils::pack_item_info(&result, item_info);

    // client
    commonproto::item_info_t* item_data = item_msg.add_items();
    item_data->CopyFrom(*item_info);
    //log
    log.item_id = result.item_id;
    log.chg_val = count;
    log.orig_val = result.count + count;
    log.chg_reason = reason;
    log.slot_id = slot_id;
    item_change_log_list.push_back(log);
    reduce_stat.add_info("item_key", result.item_id);
    reduce_stat.add_op(StatInfo::op_item, "item_key"); 
    reduce_stat.add_info("item_value", count);
    reduce_stat.add_op(StatInfo::op_item_sum, "item_key", "item_value");

    // 发送到db
    int ret = 0;
    if (wait) {
        ret = g_dbproxy->send_msg(player, player->userid,player->create_tm,
                db_cmd_change_items, db_msg);
    } else {
        ret = g_dbproxy->send_msg(NULL, player->userid,player->create_tm, 
                db_cmd_change_items, db_msg);
    }

    if (ret != 0) {
        SWAP_ITEM_RETURN_ERR(cli_err_sys_err);
    }

    // 同步物品
    if (item_msg.items_size()) {
        send_msg_to_player(player, cli_cmd_cs_0x010B_sync_change_item,
                item_msg);
    }

    g_stat_logger->log("道具", "道具消耗", Utils::to_string(player->userid), "", reduce_stat); 

    //log
    UserActionLogUtils::log_item_num_change(player, item_change_log_list); 

    return 0;    
}

equip_add_attr_t equip_main_attr_adapter(
		commonproto::equip_quench_type_t  attr_type)
{
    switch (attr_type) {
        case commonproto::EQUIP_QUENCH_TYPE_1_HP:
            return EQUIP_ADD_ATTR_HP;
        case commonproto::EQUIP_QUENCH_TYPE_1_NATK:
            return  EQUIP_ADD_ATTR_NATK;
        case commonproto::EQUIP_QUENCH_TYPE_1_NDEF:
            return  EQUIP_ADD_ATTR_NDEF;
        case commonproto::EQUIP_QUENCH_TYPE_1_SATK:
            return EQUIP_ADD_ATTR_SATK;
        case commonproto::EQUIP_QUENCH_TYPE_1_SDEF:
            return  EQUIP_ADD_ATTR_SDEF;
        default:
            return EQUIP_ADD_ATTR_NONE;
    }
}
int get_equip_attr_factor(equip_add_attr_t attr_type)
{
    switch (attr_type) {
        case EQUIP_ADD_ATTR_HP:
            return 42;
        case EQUIP_ADD_ATTR_NATK:
            return 12;
        case EQUIP_ADD_ATTR_NDEF:
            return 4;
        case EQUIP_ADD_ATTR_SATK:
            return 14;
        case EQUIP_ADD_ATTR_SDEF:
            return 5;
        case EQUIP_ADD_ATTR_CRIT:
        case EQUIP_ADD_ATTR_ANTI_CRIT:
        case EQUIP_ADD_ATTR_HIT:
        case EQUIP_ADD_ATTR_DODGE:
        case EQUIP_ADD_ATTR_BLOCK:
        case EQUIP_ADD_ATTR_BREAK_BLOCK:
            return 50;
        default:
            return 0;
    }
}

int calc_equip_add_attr(equip_add_attr_t attr_type, uint32_t level, uint32_t star)
{
    double x;
    switch (attr_type) {
        case EQUIP_ADD_ATTR_HP:
        case EQUIP_ADD_ATTR_NATK:
        case EQUIP_ADD_ATTR_NDEF:
        case EQUIP_ADD_ATTR_SATK:
        case EQUIP_ADD_ATTR_SDEF:
            x = get_equip_attr_factor(attr_type);
            break;

        case EQUIP_ADD_ATTR_CRIT:
        case EQUIP_ADD_ATTR_ANTI_CRIT:
        case EQUIP_ADD_ATTR_HIT:
        case EQUIP_ADD_ATTR_DODGE:
        case EQUIP_ADD_ATTR_BLOCK:
        case EQUIP_ADD_ATTR_BREAK_BLOCK:
            x = get_equip_attr_factor(attr_type);
            return x * (level + 0.6 * star);

        case EQUIP_ADD_ATTR_ANTI_WATER:
        case EQUIP_ADD_ATTR_ANTI_FIRE:
        case EQUIP_ADD_ATTR_ANTI_GRASS:
        case EQUIP_ADD_ATTR_ANTI_LIGHT:
        case EQUIP_ADD_ATTR_ANTI_DARK:
        case EQUIP_ADD_ATTR_ANTI_GROUND:
        case EQUIP_ADD_ATTR_ANTI_FORCE:
            return  item_conf_mgr_t::get_equip_anti_value(level);

        case EQUIP_ADD_ATTR_DAMAGE_RATE_WATER:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_FIRE:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_GRASS:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_LIGHT:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_DARK:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_GROUND:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_FORCE:
            return item_conf_mgr_t::get_equip_elem_damage_rate(level);

        default:
            return 0;
    }
    return x * ( (level + 0.6 * star) * (level + 0.6 * star) - (level + 0.6 * star) + 2) / 2;
}

int rand_add_equip_buff(commonproto::item_optional_attr_t &item_optional_attr, uint32_t buff_group_id, bool quench)
{
    const equip_buff_conf_t *ebc = g_equip_buff_rand_mgr.rand_from_equip_buff_group(buff_group_id);
    if (ebc) {
        if (ebc->type == equip_buff_elem_type_buff) {
            if (quench) {
                item_optional_attr.set_quench_3_type(commonproto::EQUIP_QUENCH_TYPE_3_BUFF);
                item_optional_attr.set_quench_3_id(ebc->target_id);
                item_optional_attr.set_quench_3_value(0);
            } else {
                item_optional_attr.add_buff_id(ebc->target_id);
            }
        } else if (ebc->type == equip_buff_elem_type_attr) {
            uint32_t add_val = 0;
            if (ebc->method == 1) {
                add_val = taomee::ranged_random(ebc->min, ebc->max);
            } else if (ebc->method == 2) {
                add_val = calc_equip_add_attr((equip_add_attr_t)ebc->target_id, item_optional_attr.level(), item_optional_attr.star());
                // add_val *= taomee::ranged_random(ebc->min, ebc->max);
                add_val = floor(add_val * (taomee::ranged_random(ebc->min, ebc->max)*0.001));
            }
            if (quench) {
                item_optional_attr.set_quench_3_type(commonproto::EQUIP_QUENCH_TYPE_3_ATTR);
                item_optional_attr.set_quench_3_id(ebc->target_id);
                item_optional_attr.set_quench_3_value(add_val);
            } else {
                commonproto::attr_data_list_t* equip_attrs = item_optional_attr.mutable_equip_attrs();
                commonproto::attr_data_t *single_attr = equip_attrs->add_attrs();
                single_attr->set_type(ebc->target_id);
                single_attr->set_value(add_val);
            }
        }
        if (quench) {
            if (item_optional_attr.buff_elem_id_size() < 4) {
                for (int i = item_optional_attr.buff_elem_id_size(); i < 4; i++) {
                    item_optional_attr.add_buff_elem_id(0);
                }
            }
            item_optional_attr.set_buff_elem_id(3, ebc->elem_id);
        } else {
            item_optional_attr.add_buff_elem_id(ebc->elem_id);
        }

        uint32_t org_magic_power = item_optional_attr.magic_power();
        item_optional_attr.set_magic_power(org_magic_power + ebc->magic_power * item_optional_attr.level());
    }

    return 0;
}

int init_equip_attr(item_t *item)
{
    assert(item);
    uint32_t item_id = item->item_id;
    const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(item_id);
    assert(item_conf);
    if (!g_item_conf_mgr.is_equip(item->item_id)) {
        return 0;
    }
    //需要保留之前的洗练属性
    commonproto::item_optional_attr_t old_opt_attr;
    old_opt_attr.ParseFromString(item->opt_attr);

    //星级
    uint32_t star = 0;
    if (item_conf->is_magic_equip) {
        star = item_conf->star;
    } else {
        int val = taomee::ranged_random(1, 100);
        if (val > 80) {//20%
            star = 2;
        } else if (val > 50) {//30%
            star = 1;
        } else {
            star = 0;
        }
    }
    //品级
    uint32_t level = 0;
    if (old_opt_attr.level() == 0) {
        level = (uint32_t)item_conf->base_quality;
    } else if (old_opt_attr.level() >= (uint32_t)item_conf->quality_max) {
        level = (uint32_t)item_conf->quality_max;
    } else if (old_opt_attr.level() <= (uint32_t)item_conf->base_quality) {
        level = (uint32_t)item_conf->base_quality;
    } else {
        level = old_opt_attr.level();
    }

    commonproto::item_optional_attr_t item_optional_attr;

    item_optional_attr.set_elem_type((commonproto::equip_elem_type_t)item_conf->elem_type);
    item_optional_attr.set_level(level);
    item_optional_attr.set_star(star);
    item_optional_attr.set_part((commonproto::equip_body_pos_t)item_conf->equip_body_pos);

    //固定转化比属性 TODO + 洗练
    item_optional_attr.set_pet_major_attr_trans_rate(item_conf_mgr_t::get_equip_trans_major_rate_by_quality(level));
    item_optional_attr.set_pet_minor_attr_trans_rate(item_conf_mgr_t::get_equip_trans_minor_rate_by_quality(level));

    //配表属性
    // 装备单属性加成
    if (item_conf->add_attr_type) {
        commonproto::attr_data_list_t* equip_attrs = item_optional_attr.mutable_equip_attrs();
        commonproto::attr_data_t *single_attr = equip_attrs->add_attrs();
        single_attr->set_type((uint32_t)item_conf->add_attr_type);
        if (item_conf_mgr_t::is_valid_equip_add_attr(single_attr->type())) {
            single_attr->set_value(calc_equip_add_attr((equip_add_attr_t)single_attr->type(), level, star));
        } else {
            single_attr->set_value(0);
        }
        switch(single_attr->type()) {
        case EQUIP_ADD_ATTR_DAMAGE_RATE_WATER:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_FIRE:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_GRASS:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_LIGHT:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_DARK:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_GROUND:
        case EQUIP_ADD_ATTR_DAMAGE_RATE_FORCE:
            item_optional_attr.set_total_damage_rate(item_optional_attr.total_damage_rate() + single_attr->value());
            break;
        default:
            break;
        }
    }
    // 装备多属性加成
    for (uint32_t i = 0; i < item_conf->item_attr_list.size(); i++ ){
        commonproto::attr_data_list_t* equip_attrs = item_optional_attr.mutable_equip_attrs();
        commonproto::attr_data_t *single_attr = equip_attrs->add_attrs();
        single_attr->set_type(item_conf->item_attr_list[i].type);
        if (item_conf_mgr_t::is_valid_equip_add_attr(single_attr->type())) {
            single_attr->set_value(calc_equip_add_attr((equip_add_attr_t)single_attr->type(), level, star));
        } else {
            single_attr->set_value(item_conf->item_attr_list[i].value);
        }
    }

    //一条随机属性(非属性石才有)
	if (!g_item_conf_mgr.is_valid_elem_type(item_conf->elem_type)) {
		uint32_t rand_type = taomee::ranged_random(EQUIP_ADD_ATTR_HP, EQUIP_ADD_ATTR_BREAK_BLOCK);
		commonproto::attr_data_list_t* equip_attrs = item_optional_attr.mutable_equip_attrs();
		commonproto::attr_data_t *single_attr = equip_attrs->add_attrs();
		single_attr->set_type(rand_type);
		single_attr->set_value(calc_equip_add_attr((equip_add_attr_t)single_attr->type(), level, star));
	}
    uint32_t rand_attr_rate = 0;
    if (star == 0) {
        rand_attr_rate = 0;
    } else if (star == 1) {
        rand_attr_rate = 50;
    } else {
        rand_attr_rate = 100;
    }
    bool add_rand_attr = false;
    if (taomee::ranged_random(1, 100) <= (int)rand_attr_rate) {
        add_rand_attr = true;
    }
    if (add_rand_attr) {
        //按星级的随机buff
        rand_add_equip_buff(item_optional_attr, item_conf->base_buff1_ID, false);
    } else {
        item_optional_attr.add_buff_elem_id(0);
    }
    

    //魔法装备额外的随机buff
    if (item_conf->is_magic_equip) {
        rand_add_equip_buff(item_optional_attr, item_conf->base_buff2_ID, false);
        rand_add_equip_buff(item_optional_attr, item_conf->base_buff3_ID, false);
    } else {
        item_optional_attr.add_buff_elem_id(0);
        item_optional_attr.add_buff_elem_id(0);
    }

    //保留洗练属性
    if (old_opt_attr.has_quench_1_type()) {
        item_optional_attr.set_quench_1_type(old_opt_attr.quench_1_type());
        item_optional_attr.set_quench_1_value(old_opt_attr.quench_1_value());
    }
    if (old_opt_attr.has_quench_2_type()) {
        item_optional_attr.set_quench_2_type(old_opt_attr.quench_2_type());
        item_optional_attr.set_quench_2_value(old_opt_attr.quench_2_value());
    }
    if (old_opt_attr.has_quench_3_type()) {
        item_optional_attr.set_quench_3_type(old_opt_attr.quench_3_type());
        item_optional_attr.set_quench_3_id(old_opt_attr.quench_3_id());
        item_optional_attr.set_quench_3_value(old_opt_attr.quench_3_value());
        item_optional_attr.add_buff_elem_id(old_opt_attr.buff_elem_id(old_opt_attr.buff_elem_id_size()-1));
    } else {
        item_optional_attr.add_buff_elem_id(0);
    }
    item_optional_attr.set_equip_power(EquipUtils::calc_equip_btl_value(item_optional_attr));
    item->opt_attr.clear();
    item_optional_attr.SerializeToString(&(item->opt_attr));

    return 0;
}


uint32_t smelter_pet_fragment(player_t* player,
		uint32_t slot_id, uint32_t smelter_item_cnt, uint32_t& fragment_gold)
{
	item_t* item = player->package->get_mutable_item_in_slot(slot_id);
	if (item == NULL) {
		return cli_err_item_not_exist;
	}
	if (item->count < smelter_item_cnt) {
		return cli_err_not_enough_item_num;
	}
	//判断是否是伙伴碎片
	if (!(item->item_id >= PET_FRAGMENT_ID_INDEX_START &&
			item->item_id <= PET_FRAGMENT_ID_INDEX_END)) {
		return cli_err_item_not_pet_fragment;
	}
	fragment_gold = commonproto::PET_FRAGMENT_SMELTER_CONF_VAL * smelter_item_cnt;
	uint32_t ret = reduce_item_by_slot_id(player, slot_id, smelter_item_cnt,
			false, onlineproto::SYNC_REASON_SMELTER);
	if (ret) {
		return ret;
	}
	return 0;
}

uint32_t smelter_equip(player_t* player,
		uint32_t slot_id, uint32_t smelter_item_cnt,
		uint32_t& normal_currency_cnt, uint32_t& high_currency_cnt)
{
	item_t* item = player->package->get_mutable_item_in_slot(slot_id);
	if (item == NULL) {
		return cli_err_item_not_exist;
	}
	if (item->count < smelter_item_cnt) {
		return cli_err_not_enough_item_num;
	}
	if (!g_item_conf_mgr.is_equip(item->item_id)) {
		return cli_err_equip_not_found;
	}
	const item_conf_t* item_conf = g_item_conf_mgr.find_item_conf(item->item_id);
	if (item_conf == NULL) {
		return cli_err_item_not_exist;
	}
	//if (item_conf->is_magic_equip) {
	//	return cli_err_magic_item_can_not_smelter;
//	}
	if (!(item_conf->is_magic_equip)) {
		if (item_conf->base_quality >= EQUIP_QUALITY_PURPLE &&
				item_conf->base_quality <= EQUIP_QUALITY_PURPLE3 ) {
			double tmp_exp = item_conf->base_quality - EQUIP_QUALITY_PURPLE;
			int rand_val = ceil(0.2 * pow(1.2, tmp_exp) * 100);
			TRACE_TLOG("Smelter Item Rand Val:%u", rand_val);
			if (taomee::ranged_random(0, 100) < rand_val) {
				high_currency_cnt = commonproto::HIGH_EQUIP_SMELTER_CONF;
			}
		} 
	}
	if (item_conf->is_magic_equip) {
            normal_currency_cnt = (item_conf->level_limit) * 2;
	}
	else{
           normal_currency_cnt = floor(10 * pow(2, item_conf->base_quality - 1));
	}
	uint32_t ret = reduce_item_by_slot_id(player, slot_id, smelter_item_cnt,
			false, onlineproto::SYNC_REASON_SMELTER);
	if (ret) {
		return ret;
	}

	return 0;
}
