#include "player.h"
#include "global_data.h"
#include "xmlutils.h"
#include "map_conf.h"
#include "map_user_manager.h"
#include "map_utils.h"
#include "service.h"
#include "attr_utils.h"
#include "utils.h"
#include "player_manager.h"
#include "data_proto_utils.h"
#include "duplicate_utils.h"
#include "home_data.h"
#include "family_utils.h"
#include "pet_utils.h"
#include "utils.h"

bool MapUtils::is_valid_map_id(uint32_t map_id)
{
    if (get_map_conf(map_id)) {
        return true;
    }
    return false; 
}

bool MapUtils::is_mon_crisis_map_id(uint32_t map_id)
{
    return (map_id >= 10800 && map_id <= 10803);
}

const map_conf_t *MapUtils::get_map_conf(uint32_t map_id)
{
    return g_map_conf_mgr.find_map_conf(map_id);     
}

bool MapUtils::is_player_in_map(player_t *player)
{
    assert(player);
    std::set<const player_t *> ret_set;
    g_map_user_mgr.get_players_in_the_same_map_line(player, ret_set);
    if (ret_set.empty()) return false;
    return true;
}

int MapUtils::enter_map(player_t* player, uint32_t map_id, uint32_t x, uint32_t y, uint32_t heading)
{
    if(0 != player->cur_map_id) {
        MapUtils::leave_map(player);
    }

    const map_conf_t *map = get_map_conf(map_id);
    if (map == NULL) {
        return cli_err_map_not_exist; 
    }

    if (DupUtils::is_player_in_duplicate(player) && !map->is_dup_scene) {
        //在副本中切换到非副本场景 则强制退出副本
        DupUtils::tell_btl_exit(player, NO_WAIT_SVR);
    }
    //从怪物危机地图回到普通地图时 恢复
    if (is_mon_crisis_map_id(player->last_map_id) && !is_mon_crisis_map_id(map_id)) {
        PetUtils::retrieve_fight_pet_pos(player);
    }

    if (x == 0) {
        // map.xml没配置init_pos这里会core
        x = map->init_pos[0].x;
        y = map->init_pos[0].y;
    }

	if (map_id == HOME_MAP_ID) {
		player->map_x = x;
		player->map_y = y;
		if (heading > 360) {
			heading = 0;
		}
		player->heading = heading;
		player->cur_map_id = map_id;
		g_stat_logger->log("地图", std::string(map->name) + "进入", Utils::to_string(player->userid), "");
		return 0;
	}
    //进入地图统计
    Utils::write_msglog_new(player->userid, "基本", "地图进入", string(map->name));


    bool ret = g_map_user_mgr.add_player(player, map_id);
    if (!ret) {
        return cli_err_sys_err;
    }

    player->map_x = x;
    player->map_y = y;
    if (heading > 360) {
        heading = 0;
    }
    player->heading = heading;

    // 向地图上的玩家发送消息
    onlineproto::sc_0x0103_notify_enter_map noti_msg;
	DataProtoUtils::pack_map_player_info(player, noti_msg.mutable_player());
	noti_msg.set_x_pos(player->map_x);
	noti_msg.set_y_pos(player->map_y);
	noti_msg.set_heading(player->heading);
    send_msg_to_map_users(player, 
                cli_cmd_cs_0x0103_notify_enter_map, noti_msg);

    g_stat_logger->log("地图", std::string(map->name) + "进入", Utils::to_string(player->userid), "");

    return 0;
}

int MapUtils::leave_map(player_t* player)
{
    const map_conf_t *map = get_map_conf(player->cur_map_id);
    if (!map) {
        return -1;
    }
    player->last_map_id = player->cur_map_id;

	//小屋地图的离开，不走此逻辑(目前小屋地图只有15，以后如果增加，则改成范围判断)
	if (player->cur_map_id == HOME_MAP_ID) {
		player->cur_map_id = 0;
        player->cur_map_line_id = 0;
        player->map_x = 0;
        player->map_y = 0;
		player->temp_info.state_buf->clear();
		return 0;
	}

    // TODO toby register func
    if (player->cur_map_id == FAMILY_MAP_ID) {
        uint32_t family_id = GET_A(kAttrFamilyId);
        if (!FamilyUtils::is_valid_family_id(family_id)) {
            return send_err_to_player(
                    player, player->cli_wait_cmd, cli_err_family_id_illegal);
        }
        homeproto::cs_leave_family_hall leave_info;
        leave_info.set_family_id(family_id);
        leave_info.set_line_id(player->family_hall_line_id);
        g_dbproxy->send_msg(
            0, player->userid, player->create_tm, home_cmd_leave_family_hall, leave_info);
        player->cur_map_id = 0;
        player->family_hall_line_id = 0;
        player->temp_info.state_buf->clear();
        return 0;
    }

    // 向其他玩家发送玩家已经离开地图
    onlineproto::sc_0x0105_notify_leave_map remote_data;
    remote_data.add_userid_list(player->userid);
    send_msg_to_map_users_except_self(player,
            cli_cmd_cs_0x0105_notify_leave_map, remote_data);


    g_map_user_mgr.del_player(player);
    player->cur_map_id = 0;
    player->temp_info.state_buf->clear();

    return 0;
}

int MapUtils::sync_player_change_state(player_t* player, onlineproto::cs_0x0106_player_change_state &player_change_state)
{
    const map_conf_t* map = get_map_conf(player->cur_map_id);
    if (!map) {
        KERROR_LOG(player->userid, "player not in map %u",
                player->cur_map_id);
        return -1;
    }

	//NOTI(kevin:当前调试小屋时，小屋默认地图暂定等于15)
	if (player->cur_map_id == HOME_MAP_ID) {
		player->temp_info.state_buf->assign(player_change_state.state_bytes());
		homeproto::cs_change_state	home_msg;
		home_msg.set_type(player_change_state.type());
        home_msg.set_userid(player->userid);
        //home_msg.set_create_tm(player_change_state.type() == 1 
		//      ?GET_A(kAttrCreateTm) :player_change_state.create_tm());
		home_msg.set_create_tm(player->create_tm);
		home_msg.set_state_bytes(player_change_state.state_bytes());
		if (player_change_state.type() == 2) {
			home_msg.set_pet_create_tm(player_change_state.create_tm());
		}
		role_info_t orig_host = player->home_data->at_whos_home();
		return g_dbproxy->send_msg(
                0, orig_host.userid, orig_host.u_create_tm,
				home_cmd_change_state, home_msg, player->userid);

    //(当玩家在多人副本中时, 也需要同步状态)
	} else if (map->is_dup_scene && player->temp_info.dup_state == PLAYER_DUP_PLAY){
        duplicate_battle_type_t type = DupUtils::get_player_duplicate_battle_type(player);
        if (type == DUP_BTL_TYPE_PPVE
            || type == DUP_BTL_TYPE_WORLD_BOSS
            || type == DUP_BTL_TYPE_RPVP) {
            battleproto::cs_battle_relay btl_relay;
            btl_relay.set_uid(player->userid);
            btl_relay.set_create_tm(GET_A(kAttrCreateTm));
            btl_relay.set_cmd(cli_cmd_cs_0x0106_player_change_state);
            string str;
            player_change_state.SerializeToString(&str);
            btl_relay.set_pkg(str);
            return DupUtils::send_to_battle(player, btl_cmd_msg_relay, btl_relay, NO_WAIT_SVR);
        }
        return 0;
    }

    // 在家族大厅
    uint32_t family_id = GET_A(kAttrFamilyId);
    if (player->cur_map_id == FAMILY_MAP_ID &&
            FamilyUtils::is_valid_family_id(family_id)) {
        player->temp_info.state_buf->assign(player_change_state.state_bytes());
		homeproto::cs_family_hall_change_state	home_msg;
		home_msg.Clear();
		home_msg.set_type(player_change_state.type());
        home_msg.set_userid(player->userid);
        home_msg.set_create_tm(player_change_state.type() == 1 
                ?GET_A(kAttrCreateTm) :player_change_state.create_tm());
		home_msg.set_state_bytes(player_change_state.state_bytes());
        home_msg.set_family_id(family_id);
        home_msg.set_family_hall_line_id(player->family_hall_line_id);
		return g_dbproxy->send_msg(
                0, player->userid, player->create_tm, 
                home_cmd_family_hall_state_change, home_msg, player->userid);
    }

    //在社区地图
    onlineproto::sc_0x0107_notify_player_change_state sc_player_change_state;
    sc_player_change_state.set_userid(player->userid);
    sc_player_change_state.set_type(player_change_state.type());
    sc_player_change_state.set_create_tm( player_change_state.type() == 1 
            ?GET_A(kAttrCreateTm) :player_change_state.create_tm());
    sc_player_change_state.set_state_bytes(player_change_state.state_bytes());


    return send_msg_to_map_users_except_self(player,
            cli_cmd_cs_0x0107_notify_player_change_state, sc_player_change_state);
}

int MapUtils::send_msg_to_map_users(player_t *player, uint32_t cmd,
        const google::protobuf::Message& message)
{
    const map_conf_t *map = get_map_conf(player->cur_map_id);
    if (!map) {
        return -1;
    }

    if(map->is_private) {
        return send_msg_to_player(player, cmd, message);  
    }

    std::set<const player_t *> ret_set;
    g_map_user_mgr.get_players_in_the_same_map_line(player, ret_set);
    FOREACH(ret_set, it) {
        const player_t *dest = (*it);
        send_msg_to_player((player_t*)dest, cmd, message);
    }
    return 0;
}

int MapUtils::send_msg_to_map_users_except_self(player_t *player, uint32_t cmd,
        const google::protobuf::Message& message)
{
    const map_conf_t *map = get_map_conf(player->cur_map_id);
    if (!map) {
        return -1;
    }

    if (map->is_private) {
        return 0;
    }

    std::set<const player_t *> ret_set;
    g_map_user_mgr.get_players_in_the_same_map_line(player, ret_set);
    FOREACH(ret_set, it) {
        const player_t *dest = (*it);
        if (dest == player) continue;
        send_msg_to_player((player_t*)dest, cmd, message);
    }
    return 0;
}

int MapUtils::send_msg_to_all_map_users(uint32_t map_id, uint32_t cmd,
        const google::protobuf::Message& message)
{
    const map_conf_t *map = get_map_conf(map_id);
    if (!map) {
        return -1;
    }

    std::vector<player_t*> player_list;
    g_player_manager->get_player_list(player_list);

    FOREACH(player_list, it) {
        const player_t *dest = (*it);
        if (dest->cur_map_id == map_id) {
            send_msg_to_player((player_t*)dest, cmd, message);
        }
    }
    return 0;
}

int MapUtils::sync_map_player_info(player_t *player,
        commonproto::map_player_change_reason_t reason)
{
    //如果在地图 则将精灵改变广播到地图
    if (player->cur_map_id == HOME_MAP_ID) {//如果是在小屋地图
        homeproto::cs_sync_map_player_info home_msg;
        DataProtoUtils::pack_map_player_info(player, home_msg.mutable_player_info());
        home_msg.set_reason(reason);
		home_msg.set_userid(player->userid);
		home_msg.set_create_tm(player->create_tm);
		return g_dbproxy->send_msg(NULL, player->userid, player->create_tm, 
                home_cmd_sync_map_player_info, home_msg, player->userid);
    }

    // 家族大厅状态同步
    uint32_t family_id = GET_A(kAttrFamilyId);
    if (player->cur_map_id == FAMILY_MAP_ID &&
            FamilyUtils::is_valid_family_id(family_id)) {
        homeproto::cs_family_hall_sync_map_player_info home_msg;
        DataProtoUtils::pack_map_player_info(player, home_msg.mutable_player_info());
        home_msg.set_reason(reason);
        home_msg.set_family_id(family_id);
        home_msg.set_family_hall_line_id(player->family_hall_line_id);
		return g_dbproxy->send_msg(NULL, player->userid, player->create_tm, 
                home_cmd_family_hall_sync_map_player_info, home_msg, player->userid);        
    }

    //直接广播到地图
    onlineproto::sc_0x0108_notify_map_player_info_change noti_msg;
    DataProtoUtils::pack_map_player_info(player, noti_msg.mutable_player_info());
    noti_msg.set_reason(reason);
    send_msg_to_map_users(player, cli_cmd_cs_0x0108_notify_map_player_info_change,
            noti_msg);
    return 0;
}
