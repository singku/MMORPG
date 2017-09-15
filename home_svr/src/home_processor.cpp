#include "home_processor.h"
#include "player.h"
#include "player_manager.h"
#include "service.h"
#include "timer_procs.h"
#include "pb0x01.pb.h"
#include "pb0x03.pb.h"
#include "family_manager.h"

int EnterHomeCmdProcessor::proc_pkg_from_client(player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	const commonproto::map_player_data_t& player_info = cli_in_.player_info();
    player->set_map_player_info(player_info);

	const commonproto::player_base_info_t& base_info = player_info.base_info();
	player->set_userid(base_info.user_id());
	player->set_name(base_info.nick());
	player->set_u_create_tm(base_info.create_tm());
	player->x_ = cli_in_.x_pos();
	player->y_ = cli_in_.y_pos();
    role_info_t host_role;
    host_role.userid = cli_in_.host_id();
    host_role.u_create_tm = cli_in_.u_create_tm();
	player_t *dest = PLAYER_MGR.get_player_by_role(host_role);
	if (!dest || !dest->home_info_loaded()) {
		if (PLAYER_MGR.home_info_is_waiting(host_role)) {
			PLAYER_MGR.add_to_home_waiting_list(player->role(), host_role);
			player->set_wait_host(host_role);
			return 0;
		}
        //先增加一个拉取的定时器
        timer_struct_t *timer = ADD_TIMER_EVENT_EX(&g_waiting_rsp_timer,
                kTimerTypeWaitingRsp,
                (void*)(ROLE_KEY(host_role)),
                NOW() + kTimerIntervalWaitingRsp);
        PLAYER_MGR.timer_map[ROLE_KEY(host_role)] = timer;

		PLAYER_MGR.add_to_home_waiting_list(player->role(), host_role);
		player->set_wait_host(host_role);
		db_in_.set_getter_id(player->uid());
		return g_dbproxy->send_msg(player, host_role.userid, host_role.u_create_tm, 
                db_cmd_get_home_info, db_in_);
	}

	homeproto::sc_enter_home msg_out;
	msg_out.set_hostid(host_role.userid);
	msg_out.set_u_create_tm(host_role.u_create_tm);
	msg_out.set_hometype(dest->home_type());
	msg_out.set_name(dest->name());
	msg_out.set_x_pos(player->x_);
	msg_out.set_x_pos(player->y_);
	FOREACH(dest->home_data_->at_home_player_map_, it) {
		player_t *other = it->second;
		if (other == player) {
			continue;
		}
        commonproto::map_player_data_t *player_info = msg_out.add_players();
        player_info->CopyFrom(other->get_map_player_info());
	}
	send_msg_to_player(player, player->wait_cmd_, msg_out);

	//通知其他玩家，进入小屋
	dest->add_visitor(player);
	player->set_at_host(dest->role());

	return 0;
}

int EnterHomeCmdProcessor::proc_pkg_from_serv(player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	uint32_t host_id = db_out_.host_id();
    uint32_t host_u_create_tm = db_out_.host_u_create_tm();
    player_t *host_p = PLAYER_MGR.get_player_by_role(ROLE(host_id, host_u_create_tm));
    if (!host_p) {
        host_p = PLAYER_MGR.create_new_player(ROLE(host_id, host_u_create_tm), 0, 0, db_out_.home_type());
        host_p->set_name(db_out_.host_nick());
    } else {
        host_p->init_home_info();
        host_p->set_name(db_out_.host_nick());
        host_p->set_home_type(db_out_.home_type());
        host_p->set_home_info_loaded();
    }
    
    //及时回包后移除定时器
    if (PLAYER_MGR.timer_map.count(host_p->role_key()) != 0) {
        REMOVE_TIMER(PLAYER_MGR.timer_map.find(host_p->role_key())->second);
        PLAYER_MGR.timer_map.erase(host_p->role_key());
    }
    //对于等待这个房主的等待列表
    homeproto::sc_enter_home msg_out;
    msg_out.Clear();
    msg_out.set_hostid(host_id);
	msg_out.set_u_create_tm(host_u_create_tm);
    msg_out.set_hometype(host_p->home_type());
	msg_out.set_name(host_p->name());
	//全部都回包
    std::set<uint64_t> w_set;
    PLAYER_MGR.delete_from_home_waiting_list(host_p->role(), w_set);
    FOREACH(w_set, it) {
        player_t *wait_p = PLAYER_MGR.get_player_by_role(KEY_ROLE(*it));
        if (!wait_p) continue;
        wait_p->set_wait_host(ROLE(0,0));
        //设置在谁家呆着
        role_info_t pre_host_role = wait_p->at_whos_home();
        player_t *pre_host = PLAYER_MGR.get_player_by_role(pre_host_role);
        if (pre_host) {
            pre_host->del_visitor(wait_p);
            wait_p->set_at_host(ROLE(0,0));
        }
        //添加访问者
        host_p->add_visitor(wait_p);
        wait_p->set_at_host(host_p->role());
        if (pre_host) PLAYER_MGR.try_release_player(pre_host);
		if (wait_p != player) {
            commonproto::map_player_data_t *player_info = msg_out.add_players();
            player_info->CopyFrom(wait_p->get_map_player_info());
		}
    }

    FOREACH(w_set, it) {
        player_t *wait_p = PLAYER_MGR.get_player_by_role(KEY_ROLE(*it));
        if (!wait_p) continue;
        send_msg_to_player(wait_p, wait_p->wait_cmd_, msg_out);
    }
	return 0;
}

int ExitHomeCmdProcessor::proc_pkg_from_client(player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	player = PLAYER_MGR.get_player_by_role(ROLE(cli_in_.userid(), cli_in_.create_tm()));
	if (player == NULL) {
		return home_err_must_in_host_hm_ope;
	}
	player_t *host = PLAYER_MGR.get_player_by_role(player->at_whos_home());
	if (host) {
		host->del_visitor(player);
		player->set_at_host(ROLE(0,0));
		PLAYER_MGR.try_release_player(host);
	}
	PLAYER_MGR.try_release_player(player);
	cli_out_.Clear();
	send_msg_to_player(player, player->wait_cmd_, cli_out_);
	return 0;
}

int HomeSyncPlayerInfoCmdProcessor::proc_pkg_from_client(player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	player = PLAYER_MGR.get_player_by_role(ROLE(cli_in_.userid(), cli_in_.create_tm()));
	if (player == NULL) {
		return home_err_must_in_host_hm_ope;
	}
    //save map_info
    const commonproto::map_player_data_t &player_info = cli_in_.player_info();
    player->set_map_player_info(player_info);

	player_t *host_p = PLAYER_MGR.get_player_by_role(player->at_whos_home());
	if (!host_p) { //不在房间(也没有去DB拉取过)
		return home_err_host_not_exsit;
	}

    onlineproto::sc_0x0108_notify_map_player_info_change noti_msg;
    noti_msg.mutable_player_info()->CopyFrom(player_info);
    noti_msg.set_reason(cli_in_.reason());

	FOREACH(host_p->home_data_->at_home_player_map_, it) {
	
		player_t *dest = it->second;
		if (!dest) continue;
		send_noti_to_player(dest, cli_cmd_cs_0x0108_notify_map_player_info_change, noti_msg);
	}
	return 0;
}

int HomePlayerStateChangeCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;

	//此时的player, 并不是状态改变的那个玩家的player
	//需要在包头中处理一下
	player = PLAYER_MGR.get_player_by_role(ROLE(cli_in_.userid(), cli_in_.create_tm()));
	if (player == NULL) {
		return home_err_must_in_host_hm_ope;
	}
    commonproto::map_player_data_t &player_info = 
        player->get_map_player_info();

    if (cli_in_.type() == 1) {//玩家状态改变
        player_info.set_state_bytes(cli_in_.state_bytes());
    } else {
        for (int i = 0; i < player_info.pet_list().pets_size(); i++) {
            commonproto::map_pet_info_t *map_pet_info = player_info.mutable_pet_list()->mutable_pets(i);
            const commonproto::pet_base_info_t &pet_base_info = map_pet_info->base_info();
            if (pet_base_info.create_tm() != cli_in_.pet_create_tm()) {
                continue;
            }
            map_pet_info->set_state_bytes(cli_in_.state_bytes());
        }
    }

	player_t *host_p = PLAYER_MGR.get_player_by_role(player->at_whos_home());
	if (!host_p) { //不在房间(也没有去DB拉取过)
		return home_err_host_not_exsit;
	}
    onlineproto::sc_0x0107_notify_player_change_state noti_msg;
    noti_msg.set_type(cli_in_.type());
    noti_msg.set_userid(cli_in_.userid());
	if (cli_in_.type() == 1) {
		noti_msg.set_create_tm(cli_in_.create_tm());
	} else {
		noti_msg.set_create_tm(cli_in_.pet_create_tm());
	}
    noti_msg.set_state_bytes(cli_in_.state_bytes());

	FOREACH(host_p->home_data_->at_home_player_map_, it) {
		player_t *dest = it->second;
		if (!dest || dest == player) continue;
		send_noti_to_player(dest, cli_cmd_cs_0x0107_notify_player_change_state, noti_msg);
	}
    return 0;
}

int GenVisitLogCmdProcessor::proc_pkg_from_client(
	player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	switchproto::cs_sw_transmit_only out;
	player = PLAYER_MGR.get_player_by_role(ROLE(cli_in_.userid(), cli_in_.create_tm()));
	if (player == NULL) {
		return home_err_must_in_host_hm_ope;
	}
	uint32_t dest_uid = cli_in_.dest_uid();
	uint32_t dest_create_tm = cli_in_.dest_create_tm();
	switchproto::sw_player_basic_info_t* player_ptr = out.add_receivers();
	player_ptr->set_userid(dest_uid);
	player_ptr->set_create_tm(dest_create_tm);
	out.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
	out.set_cmd(cli_cmd_cs_0x0134_inform_new_visit);
	out.set_pkg(cli_in_.pkg());	
	return g_switch->send_msg(0, player->uid(), player->u_create_tm(), sw_cmd_sw_transmit_only, out);
}

int NotipetExerciseChange::proc_pkg_from_client(
	player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	player = PLAYER_MGR.get_player_by_role(ROLE(cli_in_.userid(), cli_in_.create_tm()));
	if (player == NULL) {
		return home_err_must_in_host_hm_ope;
	}
	onlineproto::sc_0x0333_hm_notify_pet_exercise_state_change noti_msg;
	std::string ol_msg_str = cli_in_.pkg(); 
	noti_msg.ParseFromString(ol_msg_str);
	if (player->home_data_) {
		FOREACH(player->home_data_->at_home_player_map_, it) {
			player_t* dest = it->second;
			if (!dest) { continue; }
			if (dest == player) { continue; }
			send_noti_to_player(dest, cli_in_.cmd(), noti_msg);
		}
	} else {
		//理论上不会执行到这里
		ERROR_TLOG("Home_data_Point NULL, uid=[%u],create_tm=[%u]",
			player->uid(), player->u_create_tm());
	}
	return 0;
}

int EnterFamilyHallCmdProcessor::proc_pkg_from_client(
	player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
    uint32_t family_id = cli_in_.mutable_player()->mutable_base_info()->family_id();
    uint32_t line_id = g_family_hall_mgr->add_player(player, family_id, cli_in_.player());
    std::set<std::string> all_user_set;
    g_family_hall_mgr->get_family_hall_all_players(player, family_id, all_user_set);

    cli_out_.Clear();
    cli_out_.set_line_id(line_id);
    FOREACH(all_user_set, iter) {
        // 通知大厅里的其他玩家
        commonproto::map_player_data_t line_user;
        line_user.ParseFromString(*iter);
        uint32_t line_user_id = line_user.mutable_base_info()->user_id();
        uint32_t line_user_create_tm = line_user.mutable_base_info()->create_tm();
        if (comp_u64(line_user_id, line_user_create_tm) != player->role_key()) {
            switchproto::cs_sw_transmit_only out;
            switchproto::sw_player_basic_info_t* player_ptr = out.add_receivers();
            player_ptr->set_userid(line_user_id);
            player_ptr->set_create_tm(line_user_create_tm);
            out.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
            out.set_cmd(cli_cmd_cs_0x0103_notify_enter_map);
            std::string pkg;

            onlineproto::sc_0x0103_notify_enter_map enter_map_;
            enter_map_.mutable_player()->CopyFrom(cli_in_.player());
            enter_map_.set_x_pos(cli_in_.x_pos());
            enter_map_.set_y_pos(cli_in_.y_pos());
            enter_map_.set_heading(cli_in_.heading());
            enter_map_.SerializeToString(&pkg);

            out.set_pkg(pkg);	
            g_switch->send_msg(0, get_server_id(), 0, sw_cmd_sw_transmit_only, out);

            // 返回玩家列表
            commonproto::map_player_data_t *line_player = cli_out_.add_players();
            line_player->CopyFrom(line_user);
        }
    }
    return send_msg_to_player(player, home_cmd_enter_family_hall, cli_out_);
}

int LeaveFamilyHallCmdProcessor::proc_pkg_from_client(
	player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
    uint32_t family_id = cli_in_.family_id();
    //uint32_t line_id = cli_in_.line_id();
    std::set<std::string> all_user_set;
    g_family_hall_mgr->get_family_hall_all_players(player, family_id, all_user_set);

    FOREACH(all_user_set, iter) {
        // 通知大厅里的其他玩家
        commonproto::map_player_data_t line_user;
        line_user.ParseFromString(*iter);
        uint32_t line_user_id = line_user.mutable_base_info()->user_id();
        uint32_t line_user_create_tm = line_user.mutable_base_info()->create_tm();
        if (comp_u64(line_user_id, line_user_create_tm) != player->role_key()) {
            switchproto::cs_sw_transmit_only out;
            switchproto::sw_player_basic_info_t* player_ptr = out.add_receivers();
            player_ptr->set_userid(line_user_id);
            player_ptr->set_create_tm(line_user_create_tm);
            out.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
            out.set_cmd(cli_cmd_cs_0x0105_notify_leave_map);
            std::string pkg;

            onlineproto::sc_0x0105_notify_leave_map leave_map_;
            leave_map_.add_userid_list(player->uid());
            leave_map_.SerializeToString(&pkg);

            out.set_pkg(pkg);	
            g_switch->send_msg(0, get_server_id(), 0, sw_cmd_sw_transmit_only, out);
        }
    }

     // 清除玩家在大厅里的记录
    g_family_hall_mgr->remove_player(player->role_key(), family_id);

    return 0;
}


int FamilyHallSyncMapPlayerInfoCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
    uint32_t family_id = cli_in_.family_id();
    uint32_t line_id = cli_in_.family_hall_line_id();

    //更新大厅玩家状态
    g_family_hall_mgr->update_player_info(
           family_id, line_id, player->role_key(), cli_in_.player_info());

    // 通知大厅同分组玩家
    std::set<std::string> line_user_set;
    g_family_hall_mgr->get_line_players(player, family_id, line_id, line_user_set);
    FOREACH(line_user_set, iter) {
        commonproto::map_player_data_t line_user;
        line_user.ParseFromString(*iter);
        uint32_t line_user_id = line_user.mutable_base_info()->user_id();
        uint32_t line_user_create_tm = line_user.mutable_base_info()->create_tm();

        switchproto::cs_sw_transmit_only out;
        switchproto::sw_player_basic_info_t* player_ptr = out.add_receivers();
        player_ptr->set_userid(line_user_id);
        player_ptr->set_create_tm(line_user_create_tm);
        out.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
        out.set_cmd(cli_cmd_cs_0x0108_notify_map_player_info_change);

        std::string pkg;
        onlineproto::sc_0x0108_notify_map_player_info_change noti_msg;
        noti_msg.mutable_player_info()->CopyFrom(cli_in_.player_info());
        noti_msg.set_reason(cli_in_.reason());
        noti_msg.SerializeToString(&pkg);
        out.set_pkg(pkg);	
        g_switch->send_msg(0, get_server_id(), 0, sw_cmd_sw_transmit_only, out);
    }

    return 0;
}

int FamilyHallStateChangeCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;

    // 通知大厅同分组玩家
    uint32_t family_id = cli_in_.family_id();
    uint32_t line_id = cli_in_.family_hall_line_id();
    std::set<std::string> line_user_set;
    g_family_hall_mgr->get_line_players(player, family_id, line_id, line_user_set);

    FOREACH(line_user_set, iter) {
        commonproto::map_player_data_t line_user;
        line_user.ParseFromString(*iter);
        uint32_t line_user_id = line_user.mutable_base_info()->user_id();
        uint32_t line_user_create_tm = line_user.mutable_base_info()->create_tm();
        switchproto::cs_sw_transmit_only out;
        switchproto::sw_player_basic_info_t* player_ptr = out.add_receivers();
        player_ptr->set_userid(line_user_id);
        player_ptr->set_create_tm(line_user_create_tm);
        out.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
        out.set_cmd(cli_cmd_cs_0x0107_notify_player_change_state);

        std::string pkg;
        onlineproto::sc_0x0107_notify_player_change_state noti_msg;
        noti_msg.set_type(cli_in_.type());
        noti_msg.set_userid(cli_in_.userid());
        noti_msg.set_create_tm(cli_in_.create_tm());
        noti_msg.set_state_bytes(cli_in_.state_bytes());
        noti_msg.SerializeToString(&pkg);
        out.set_pkg(pkg);	
        g_switch->send_msg(0, get_server_id(), 0, sw_cmd_sw_transmit_only, out);
    }

    return 0;
}
