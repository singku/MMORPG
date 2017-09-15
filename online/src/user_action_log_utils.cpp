#include "common.h"
#include "player.h"
#include "user_action_log_utils.h"
#include "global_data.h"
#include "service.h"

void UserActionLogUtils::write_db_log(player_t *player, 
        dbproto::action_log_type_t type,
        uint32_t target_id,
        int32_t chg_val,
        uint32_t extra1,
        uint32_t extra2,
        uint32_t extra3)

{
    dbproto::cs_insert_user_action_log log_msg;
    dbproto::user_action_log_t *info = log_msg.add_log_list();
    info->set_type(type);
    info->set_target_id(target_id);
    info->set_value(chg_val);
    info->set_extra1(extra1);
    info->set_extra2(extra2);
    info->set_extra3(extra3);
    info->set_insert_time(NOW());
    g_dbproxy->send_msg(NULL, player->userid, 
			player->create_tm,
            db_cmd_insert_user_action_log, 
            log_msg);
}

void UserActionLogUtils::log_item_num_change(player_t* player, 
        std::vector<item_change_log_info_t>& log_list) 
{
    dbproto::cs_insert_user_action_log log_msg;
    log_msg.Clear();
    FOREACH(log_list, it) {
        dbproto::user_action_log_t *info = log_msg.add_log_list();
        info->set_type(dbproto::ActionTypeItemChange);
        info->set_target_id(it->item_id);
        info->set_value(it->chg_val);
        info->set_extra1(it->orig_val);
        info->set_extra2(it->chg_reason);
        info->set_extra3(it->slot_id);
        info->set_insert_time(NOW());
    }
    g_dbproxy->send_msg(NULL, player->userid, 
			player->create_tm,
            db_cmd_insert_user_action_log, 
            log_msg);
}

void UserActionLogUtils::log_attr_change(player_t* player,
            std::vector<attr_change_log_info_t>& log_list)
{
    dbproto::cs_insert_user_action_log log_msg;
    log_msg.Clear();
    FOREACH(log_list, it) {
        dbproto::user_action_log_t *info = log_msg.add_log_list();
        info->set_type(dbproto::ActionTypeAttrChange);
        info->set_target_id(it->attr_type);
        info->set_value(it->chg_val);
        info->set_extra1(it->orig_val);
        info->set_extra2(it->chg_reason);
        info->set_extra3(0);
        info->set_insert_time(NOW());
    }
    g_dbproxy->send_msg(NULL, player->userid, 
			player->create_tm,
            db_cmd_insert_user_action_log, 
            log_msg);
}
