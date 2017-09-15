extern "C" {
#include <libtaomee/tm_dirty/tm_dirty.h>
}
#include "friend_processor.h"
#include "friend.h"
#include "player.h"
#include "proto/switch/switch.pb.h"
#include "proto/switch/switch_cmd.h"
#include "service.h"
#include "global_data.h"
#include "proto/client/pb0x06.pb.h"
#include "proto/db/db_errno.h"
#include "proto/client/cli_errno.h"
#include "proto/db/dbproto.friend.pb.h"
#include "proto/switch/switch_cmd.h"
#include "proto/switch/switch.pb.h"
#include "player_manager.h"
#include "friend_utils.h"
#include "utils.h"
//TO_DO 
int RefreshFriendDataCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	//从内存中获取好友，黑名单和临时列表
	std::vector<friend_t> friendlist;
	std::vector<black_t> blacklist;
	std::vector<temp_t> templist;
	player->friend_info->get_all_friends(friendlist);
	player->friend_info->get_all_black(blacklist); 
	player->friend_info->get_all_temps(templist); 
	
	//将内存中各列表的id存入swich的协议中，准备拉取是否在线
	switchproto::cs_sw_is_online cs_sw_is_online_;
	for (uint32_t i = 0; i < friendlist.size(); i++) {
		switchproto::sw_player_online_info_t* sw_player_online_info =
			cs_sw_is_online_.add_ol_info();
		sw_player_online_info->set_userid(friendlist[i].id);
		sw_player_online_info->set_u_create_tm(friendlist[i].create_tm);
		sw_player_online_info->set_is_online(0);
		sw_player_online_info->set_team(0);
	}

	for (uint32_t i = 0; i < blacklist.size(); i++) {
		switchproto::sw_player_online_info_t* sw_player_online_info =
			cs_sw_is_online_.add_ol_info();
		sw_player_online_info->set_userid(blacklist[i].id);
		sw_player_online_info->set_u_create_tm(blacklist[i].create_tm);
		sw_player_online_info->set_is_online(0);
		sw_player_online_info->set_team(1);
	}

	for (uint32_t i = 0; i < templist.size(); i++) {
		switchproto::sw_player_online_info_t* sw_player_online_info =
			cs_sw_is_online_.add_ol_info();
		sw_player_online_info->set_userid(templist[i].friendid);
		sw_player_online_info->set_u_create_tm(templist[i].create_tm);
		sw_player_online_info->set_is_online(0);
		sw_player_online_info->set_team(3);
	}

    //没有好友直接返回
    if (cs_sw_is_online_.ol_info_size() == 0) {
        cli_out_.Clear();
        RET_MSG;
    }
	return g_switch->send_msg(player, g_online_id, player->create_tm,
            sw_cmd_sw_is_player_online, cs_sw_is_online_);
}

int RefreshFriendDataCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	switch(player->serv_cmd) {
	case sw_cmd_sw_is_player_online:
		return proc_pkg_from_serv_aft_get_online_info(player, body, bodylen);
	case cache_cmd_ol_req_users_info:
		return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
	}
	return 0;
}

int RefreshFriendDataCmdProcessor::proc_pkg_from_serv_aft_get_online_info(
		player_t* player, const char* body, int bodylen)
{
	switchproto::sc_sw_is_online online_infos;
    PARSE_SVR_MSG(online_infos);

	cacheproto::cs_batch_get_users_info cs_batch_get_users_info_;
	cli_out_.Clear();
	//get_friend_data_base_info_t* session = (
	//		get_friend_data_base_info_t*)player->session;
	//memset(session, 0, sizeof(*session) * 200);

	for (int i = 0; i < online_infos.ol_info_size(); i++) {
		//if (i >= 200) {
		//	return send_err_to_player(player, 
        //            player->cli_wait_cmd, cli_err_friend_reach_limit);
		//}
		//把好友在线信息写入内存
		if (online_infos.ol_info(i).team() == 0){
			friend_t friend_data;
			player->friend_info->get_friend_by_id(online_infos.ol_info(i).userid(), online_infos.ol_info(i).u_create_tm(), friend_data);
			friend_data.is_online = online_infos.ol_info(i).is_online();
			player->friend_info->add_friend(friend_data);
		
		}//黑名单在线信息加入内存
		else if (online_infos.ol_info(i).team() == 1) {
			black_t black_data;
			player->friend_info->get_black_by_id(online_infos.ol_info(i).userid(), online_infos.ol_info(i).u_create_tm(), black_data);
			black_data.is_online = online_infos.ol_info(i).is_online();  
			player->friend_info->add_black(black_data);
		} else if (online_infos.ol_info(i).team() == 3) {
			// NOW DO NOTHING
		} else {
			//NOW DO NOTHING
		}
		//session[i].userid = online_infos.ol_info(i).userid();
		//session[i].is_online = online_infos.ol_info(i).is_online();
		//session[i].team = online_infos.ol_info(i).team();
		//commonproto::friend_info_t* friend_info = cli_out_.add_friend_list();
		//friend_info->set_userid(online_infos.ol_info(i).userid());
		//friend_info->set_is_online(online_infos.ol_info(i).is_online());
		//friend_info->set_team(online_infos.ol_info(i).team());
		

		//准备发包拉取cache信息
		commonproto::role_info_t *inf = cs_batch_get_users_info_.add_roles();
		inf->set_userid(online_infos.ol_info(i).userid());
		inf->set_u_create_tm(online_infos.ol_info(i).u_create_tm());
   	}
	return g_dbproxy->send_msg(player, player->userid, player->create_tm,
            cache_cmd_ol_req_users_info, cs_batch_get_users_info_, 0);
}

int RefreshFriendDataCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
		player_t* player, const char* body, int bodylen)
{
	cacheproto::sc_batch_get_users_info sc_batch_get_users_info_;
    PARSE_SVR_MSG(sc_batch_get_users_info_);

	cli_out_.Clear();
	//get_friend_data_base_info_t* session = (
	//		get_friend_data_base_info_t*)player->session;

	for (int i = 0; i < sc_batch_get_users_info_.user_infos_size(); i++) {
		//if (i >= 200) {
		//	return send_err_to_player(player, 
        //            player->cli_wait_cmd, cli_err_friend_reach_limit);
		//}
		const commonproto::player_base_info_t& base_info = 
            sc_batch_get_users_info_.user_infos(i).base_info();

		if (player->friend_info->has_friend(base_info.user_id(), base_info.create_tm())) {
			commonproto::friend_info_t* friend_info = cli_out_.add_friend_list();
			friend_info->set_team(0);

			//从内存中读取好友信息
			friend_t friend_data;
			player->friend_info->get_friend_by_id(base_info.user_id(), base_info.create_tm(), friend_data);

			//将内存中信息加入到协议中
			friend_info->set_userid(friend_data.id);
			friend_info->set_create_tm(friend_data.create_tm);
			friend_info->set_is_online(friend_data.is_online);
			//friend_info->set_send_gift_count(friend_data.gift_count);

			//将缓存中最近的时间
			friend_data.last_login_tm = base_info.last_login_tm();
			player->friend_info->add_friend(friend_data);

			// friend_t tmp_test;
			// player->friend_info->get_friend_by_id(base_info.user_id(), tmp_test);

			//将cache中拉到的信息加入到协议中
			FriendUtils::pack_friend_cache_data_to_online(base_info, friend_info);
		}

		if (player->friend_info->has_black(base_info.user_id(), base_info.create_tm())) {
			commonproto::friend_info_t* friend_info = cli_out_.add_friend_list();
			friend_info->set_team(1);

			//从内存中读取黑名单信息
			black_t black_data;
			player->friend_info->get_black_by_id(base_info.user_id(), base_info.create_tm(), black_data);

			//将内存中信息加入到协议中
			friend_info->set_userid(black_data.id);
			friend_info->set_create_tm(black_data.create_tm);
			friend_info->set_is_online(black_data.is_online);

			//将cache中拉到的信息加入到协议中
			FriendUtils::pack_friend_cache_data_to_online(base_info, friend_info);
		}

		
		//如果templist中有user_id，说明在自己下线时，有玩家向自己提出了好友申请
		//这时需要通知前端，发给该玩家提示有人加自己好友的信息
		//并在内存中删除该temp信息
		if (player->friend_info->has_temp(base_info.user_id(), base_info.create_tm())) {
			onlineproto::sc_0x0606_notify_add_friend sc_0x0606_notify_add_friend_;
			sc_0x0606_notify_add_friend_.set_userid(player->userid);
			sc_0x0606_notify_add_friend_.set_u_create_tm(player->create_tm);
			sc_0x0606_notify_add_friend_.set_friendid(base_info.user_id());
			sc_0x0606_notify_add_friend_.set_f_create_tm(base_info.create_tm());
			sc_0x0606_notify_add_friend_.set_nick(base_info.nick());
			send_msg_to_player(player, 
                    cli_cmd_cs_0x0606_notify_add_friend, sc_0x0606_notify_add_friend_);
			player->friend_info->remove_temp(base_info.user_id(), base_info.create_tm());
		}
	}

	//拉recent列表
	std::vector<recent_t> recentlist;
	player->friend_info->get_all_recent(recentlist); 

	for (int i = recentlist.size() - 1; i >= 0; i--) {
		// cli_out_.add_recent_list(recentlist[i].id);
		// cli_out_.add_create_tm_list(recentlist[i].create_tm);
		commonproto::role_info_t *info = cli_out_.add_recent_list();
		info->set_userid(recentlist[i].id);
		info->set_u_create_tm(recentlist[i].create_tm);
	}
	//recent_t temp_recent;
	//temp_recent.id = base_info.user_id();
	//if (player->friend_info->has_recent(temp_recent)) {
	//	cli_out_.add_recent_list(base_info.user_id());
	//}
	return send_msg_to_player(player, cli_cmd_cs_0x0601_refresh_friend_data, cli_out_);
}

int AddFriendCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	uint32_t type = cli_in_.type();
	//好友达到上限
	if (player->friend_info->get_friends_size() >= 300) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_friend_reach_limit);
	}
	//根据id添加好友，可以直接给db发包check用户是否存在
	if (type == 0) {
		const commonproto::role_info_t role = cli_in_.role();
		if (player->userid == role.userid() && player->create_tm == role.u_create_tm()) {
			return send_err_to_player(player, player->cli_wait_cmd,
					cli_err_friend_add_self_attention);
		}

		dbproto::cs_check_user_exist cs_check_user_exist_;
		cs_check_user_exist_.set_server_id(g_server_id);
		cs_check_user_exist_.set_is_init_server(false);
		add_friend_session_t* add_friend_session = (
				add_friend_session_t*)player->session;
		memset(add_friend_session, 0, sizeof(*add_friend_session));
		add_friend_session->friendid = role.userid();
		add_friend_session->create_tm = role.u_create_tm();

		return g_dbproxy->send_msg(player, role.userid(), role.u_create_tm(), 
				db_cmd_check_user_exist, cs_check_user_exist_);

	}
	//根据nick添加好友，则需要先通过Nick获得id
	if (type == 1) {
		//add_friend_session_t* add_friend_session = (
		//		add_friend_session_t*)player->session;
		//memset(add_friend_session, 0, sizeof(*add_friend_session));
		//add_friend_session->friendid = cli_in_.userid();

		// dbproto::cs_get_userid_by_nick cs_get_userid_by_nick_;
		dbproto::cs_get_user_by_nick cs_get_userid_by_nick_;
		cs_get_userid_by_nick_.set_nick(cli_in_.nick());
		return g_dbproxy->send_msg(player, player->userid, player->create_tm,
				db_cmd_get_userid_by_nick , cs_get_userid_by_nick_);
	}
	else {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_proto_format_err);
	}
}

int AddFriendCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen) 
{
	switch(player->serv_cmd) {
	case sw_cmd_sw_is_player_online:
		return proc_pkg_from_serv_aft_get_online_info(player, body, bodylen);
	case db_cmd_check_user_exist:
		return proc_pkg_from_serv_aft_check_user_exist(player, body, bodylen);
	case db_cmd_get_userid_by_nick:
		return proc_pkg_from_serv_aft_get_userid_by_nick(player, body, bodylen);
	case cache_cmd_ol_req_users_info:
		return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
	}
	return 0;
}

int AddFriendCmdProcessor::proc_pkg_from_serv_aft_get_userid_by_nick(
		player_t* player, const char* body, int bodylen) 
{
	// dbproto::sc_get_userid_by_nick sc_get_userid_by_nick_;
	dbproto::sc_get_user_by_nick sc_get_userid_by_nick_;
    PARSE_SVR_MSG(sc_get_userid_by_nick_);

	
	uint32_t userid = sc_get_userid_by_nick_.userid();
	uint32_t create_tm = sc_get_userid_by_nick_.u_create_tm();
	//用户不存在
	if (userid == 0) {
		return send_err_to_player(player, 
                player->cli_wait_cmd, cli_err_friend_id_not_exist);
    }
	//用户是自身
	if (player->userid == userid && player->create_tm == create_tm) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_friend_add_self_attention);
	}

	add_friend_session_t* add_friend_session = (
			add_friend_session_t*)player->session;
	memset(add_friend_session, 0, sizeof(*add_friend_session));

	add_friend_session->friendid = sc_get_userid_by_nick_.userid();
	add_friend_session->create_tm = sc_get_userid_by_nick_.u_create_tm();
	add_friend_session->is_online = 0;

	//准备向switch服务器拉取玩家是否在线信息
	switchproto::cs_sw_is_online cs_sw_is_online_;

	switchproto::sw_player_online_info_t* sw_player_online_info =
		cs_sw_is_online_.add_ol_info();
	sw_player_online_info->set_userid(userid);
	sw_player_online_info->set_is_online(0);
	sw_player_online_info->set_team(0);
	sw_player_online_info->set_u_create_tm(create_tm);
	return g_switch->send_msg(player, g_online_id, player->create_tm,
            sw_cmd_sw_is_player_online, cs_sw_is_online_, 0);
}

int AddFriendCmdProcessor::proc_pkg_from_serv_aft_check_user_exist(
		        player_t* player, const char* body, int bodylen) 
{
	dbproto::sc_check_user_exist sc_check_user_exist_;
    PARSE_SVR_MSG(sc_check_user_exist_);
    add_friend_session_t* add_friend_session = (add_friend_session_t*)player->session; 

	switchproto::cs_sw_is_online cs_sw_is_online_;

	if(sc_check_user_exist_.exist()) {
		uint32_t userid = add_friend_session->friendid;
		uint32_t create_tm = add_friend_session->create_tm;
		//如果好友存在，则不用加了
		if (player->friend_info->has_friend(userid, create_tm)) {
			return send_err_to_player (player, player->cli_wait_cmd,                                                    
					cli_err_already_friend);                                                                          
		}
		//准备向switch发包拉取用户是否在线信息
		switchproto::sw_player_online_info_t* sw_player_online_info =
			cs_sw_is_online_.add_ol_info();
		sw_player_online_info->set_userid(userid);
		sw_player_online_info->set_is_online(0);
		sw_player_online_info->set_team(0);
		sw_player_online_info->set_u_create_tm(create_tm);
		return g_switch->send_msg(player, g_online_id, player->create_tm,
                sw_cmd_sw_is_player_online, cs_sw_is_online_, 0);
	}
	return send_err_to_player (player, player->cli_wait_cmd,
			cli_err_friend_id_not_exist);
}

int AddFriendCmdProcessor::proc_pkg_from_serv_aft_get_online_info(
		        player_t* player, const char* body, int bodylen) 
{
	cacheproto::cs_batch_get_users_info cs_batch_get_users_info_;
    switchproto::sc_sw_is_online online_infos;
    PARSE_SVR_MSG(online_infos);
	//将玩家是否在线的信息添加到session中
	//并将玩家的userid放入cache的协议中，准备向
	//cache服务器拉取玩家的各种面板上需要展示的信息
	add_friend_session_t* session = (add_friend_session_t *)player->session;
	for (int i = 0; i < online_infos.ol_info_size(); i++) {
		session->friendid = online_infos.ol_info(i).userid();
		session->create_tm = online_infos.ol_info(i).u_create_tm();
		session->is_online = online_infos.ol_info(i).is_online();
		session->team = online_infos.ol_info(i).team();
		commonproto::role_info_t *role_infos = cs_batch_get_users_info_.add_roles();
		role_infos->set_userid(online_infos.ol_info(i).userid()); 
		role_infos->set_u_create_tm(online_infos.ol_info(i).u_create_tm()); 
		// cs_batch_get_users_info_.add_uids(online_infos.ol_info(i).userid());
	}

	return g_dbproxy->send_msg(player, player->userid, player->create_tm,
            cache_cmd_ol_req_users_info, cs_batch_get_users_info_, 0);
}

int AddFriendCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
		        player_t* player, const char* body, int bodylen) 
{
	cacheproto::sc_batch_get_users_info sc_batch_get_users_info_;
    PARSE_SVR_MSG(sc_batch_get_users_info_);
	//和客户端同步 并将数据写入内存
	onlineproto::sc_0x0602_sync_friend_data sc_0x0602_sync_friend_data_;

	add_friend_session_t *session = (add_friend_session_t *)player->session;
	for (int i = 0; i < sc_batch_get_users_info_.user_infos_size(); i++) {
		commonproto::friend_info_t* friend_info = 
            sc_0x0602_sync_friend_data_.add_friend_list();
		const commonproto::player_base_info_t& base_info = 
            sc_batch_get_users_info_.user_infos(i).base_info();
		//将从cache服务器拉取到的信息打包到 online 对前端的回包中
		//如果前端面板今后需要展示更多cache拉取到的内容，都可以在此函数中添加
		FriendUtils::pack_friend_cache_data_to_online(base_info, friend_info);
		
		//将session中的信息存入online 对 前端的协议中
		friend_info->set_userid(session->friendid);
		friend_info->set_create_tm(session->create_tm);
		friend_info->set_is_online(session->is_online);
		friend_info->set_team(session->team);

		friend_t friend_data;
		//将cache中拉到的信息写入内存
		//如果今后内存中需要更多从cache拉取的信息，都可以在此函数中添加，
		//并在结构体中添加相应字段
		FriendUtils::pack_friend_data_to_player(base_info, &friend_data);
		//将session中信息读入内存
		friend_data.id = session->friendid;
		friend_data.create_tm = session->create_tm;
		friend_data.is_online = session->is_online;
		//内存中，给新添加的好友的送礼次数初始化为0
		//friend_data.gift_count = 0;

		player->friend_info->add_friend(friend_data);

		//如果以前该好友在黑名单中，则从黑名单中移除
		if (player->friend_info->has_black(session->friendid, session->create_tm)) {
			player->friend_info->remove_black(session->friendid, session->create_tm);
		}
	}
	//发包给客户端同步
	send_msg_to_player(player, 
            cli_cmd_cs_0x0602_sync_friend_data, sc_0x0602_sync_friend_data_);

	//和db同步
	// dbproto::cs_save_friend cs_save_friend_;
	// cs_save_friend_.set_userid(player->userid);
	// cs_save_friend_.set_friendid(session->friendid);
	// cs_save_friend_.set_is_friend(1);
	// cs_save_friend_.set_is_recent(0);
	// cs_save_friend_.set_is_black(0);
	// cs_save_friend_.set_is_temp(0);

	// g_dbproxy->send_msg(0, player->userid, player->create_tm,
			// db_cmd_save_friend , cs_save_friend_);

	dbproto::cs_save_friend cs_save_friend_;
	commonproto::friend_data_t finf;
	
	finf.set_userid(player->userid);
	finf.set_u_create_tm(player->create_tm);
	finf.set_friendid(session->friendid);
	finf.set_f_create_tm(session->create_tm);
	finf.set_is_friend(1);
	finf.set_is_recent(0);
	finf.set_is_blacklist(0);
	finf.set_is_temp(0);
   
	cs_save_friend_.mutable_finf()->CopyFrom(finf);

	g_dbproxy->send_msg(0, player->userid, player->create_tm,
			db_cmd_save_friend , cs_save_friend_);

	//如果对方不在线，且对方不是自己好友，给对方保存一个临时好友信息，上线后通知前端
	//player_t *ply = g_player_manager->get_player_by_userid(online_infos.ol_info(0).userid());
	//if (ply == 0) {
	//	return -1;
	//}
	//bool has_friend = ply->friend_info->has_friend(player->userid);
////////////////////////////////////
	
	//如果不在线，先给DB写临时信息，等上线时再处理
	if (session->is_online == 0) {
		// dbproto::cs_save_friend cs_save_friend__;
		// cs_save_friend__.set_userid(session->friendid);
		// cs_save_friend__.set_friendid(player->userid);
		// cs_save_friend__.set_is_friend(1);
		// cs_save_friend__.set_is_recent(0);
		// cs_save_friend__.set_is_black(0);
		// cs_save_friend__.set_is_temp(1);
		dbproto::cs_save_friend cs_save_friend_;
		commonproto::friend_data_t finf;

		finf.set_userid(session->friendid);
		finf.set_u_create_tm(session->create_tm);
		finf.set_friendid(player->userid);
		finf.set_f_create_tm(player->create_tm);
		finf.set_is_friend(1);
		finf.set_is_recent(0);
		finf.set_is_blacklist(0);
		finf.set_is_temp(1);

		cs_save_friend_.mutable_finf()->CopyFrom(finf);
		g_dbproxy->send_msg(0, session->friendid, 0,
				db_cmd_save_friend , cs_save_friend_);
	}
	//在线时的逻辑
	else {
		onlineproto::sc_0x0606_notify_add_friend notify_add_friend_;
		// const commonproto::role_info_t role = cli_in_.role();
		notify_add_friend_.set_userid(session->friendid);
		notify_add_friend_.set_u_create_tm(session->create_tm);
		notify_add_friend_.set_friendid(player->userid);
		notify_add_friend_.set_f_create_tm(player->create_tm);
		notify_add_friend_.set_nick(player->nick);
		//如果是同服，且对方好友中没有自己，直接发通知消息给对方
		player_t* ply = g_player_manager->get_player_by_userid(session->friendid);

		if (ply != NULL && ply->friend_info->has_friend(player->userid, player->create_tm) == false){
			send_msg_to_player(ply, cli_cmd_cs_0x0606_notify_add_friend, notify_add_friend_);
		}
		else {
			//不是同服，则转给switch,发往对方online做处理
			//对方online处理逻辑在switch_proto的
			//SwitchTransmitCmdProcessor::proc_pkg_from_serv
			//的hack下边的if中,注意对应的协议
			switchproto::cs_sw_transmit_only cs_sw_transmit_only_;
			switchproto::sw_player_basic_info_t* sw_player_basic_info = 
				cs_sw_transmit_only_.add_receivers();

			// const commonproto::role_info_t role = cli_in_.role();
			sw_player_basic_info->set_userid(session->friendid);
			sw_player_basic_info->set_create_tm(0);

			cs_sw_transmit_only_.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
			cs_sw_transmit_only_.set_cmd(cli_cmd_cs_0x0606_notify_add_friend);

			std::string pkg;
			notify_add_friend_.SerializeToString(&pkg);
			cs_sw_transmit_only_.set_pkg(pkg);

			g_switch->send_msg(NULL, g_online_id, 0, sw_cmd_sw_transmit_only,
					cs_sw_transmit_only_);
		}
	}
	
	//回包提示操作完成
	cli_out_.Clear();
	cli_out_.set_result(0);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int SendPersonalMsgCmdProcessor::proc_pkg_from_client(
		        player_t* player, const char* body, int bodylen) 
{
	PARSE_MSG;
	cli_out_.Clear();
	std::string msg = cli_in_.msg();
	if(msg.size() >= kMaxMsgLength){
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_over_personal_msg_length);
	}
	char * content_ = NULL;
	int len = msg.size();
	if(!msg.empty()){
		content_ = new char[len + 1];
		content_[len] = '\0';
        strncpy(content_, msg.c_str(), len + 1); 
		//检测脏词
		tm_dirty_replace(content_);
	} else {
		content_ = new char('\0');
	}
	msg_friend_session_t* msg_friend_session = (msg_friend_session_t*) player->session;
	memset(msg_friend_session, 0, sizeof(*msg_friend_session));
	msg_friend_session->friendid = cli_in_.userid();
	msg_friend_session->create_tm = cli_in_.create_tm();
	strncpy(msg_friend_session->content, content_, len + 1); 

	//非好友列表中的用户
	bool is_friend = player->friend_info->has_friend(cli_in_.userid(), cli_in_.create_tm());
	if (is_friend == false) {
		cli_out_.set_userid(0);
		cli_out_.set_create_tm(0);
		cli_out_.set_msg(std::string(content_));
		delete[] content_;
		return send_msg_to_player(player, cli_cmd_cs_0x0604_send_personal_msg, cli_out_);
	}

	//判断是否是同服且在线好友
	player_t* ply = g_player_manager->get_player_by_userid(cli_in_.userid());

	if (ply != NULL){
		onlineproto::sc_0x0605_notify_personal_message notify_personal_message_;
		notify_personal_message_.set_receiver(cli_in_.userid());
		notify_personal_message_.set_r_create_tm(cli_in_.create_tm());
		notify_personal_message_.set_sender(player->userid);
		notify_personal_message_.set_s_create_tm(player->create_tm);
		notify_personal_message_.set_msg(std::string(content_));
		notify_personal_message_.set_sender_nick(player->nick);
		
		FriendUtils::proc_recent_list(player, cli_in_.userid(), cli_in_.create_tm());
		//如果在对方黑名单中，就丢包,只给自己发
		if (ply->friend_info->has_black(player->userid, player->create_tm)) {
			send_msg_to_player(player, 
                    cli_cmd_cs_0x0605_notify_personal_message, notify_personal_message_);
			delete[] content_;
			return send_msg_to_player(player, cli_cmd_cs_0x0604_send_personal_msg, cli_out_);
		}
		//如果对方好友列表中没有自己，则发加好友通知
		else if (ply->friend_info->has_friend(player->userid, player->create_tm) == false) {
			onlineproto::sc_0x0606_notify_add_friend sc_0x0606_notify_add_friend_;                                              
			sc_0x0606_notify_add_friend_.set_userid(notify_personal_message_.receiver());
			sc_0x0606_notify_add_friend_.set_u_create_tm(notify_personal_message_.r_create_tm());
			sc_0x0606_notify_add_friend_.set_friendid(notify_personal_message_.sender());
			sc_0x0606_notify_add_friend_.set_f_create_tm(notify_personal_message_.s_create_tm());
			sc_0x0606_notify_add_friend_.set_nick(notify_personal_message_.sender_nick());
			send_msg_to_player(ply, cli_cmd_cs_0x0606_notify_add_friend, sc_0x0606_notify_add_friend_);

			send_msg_to_player(player, cli_cmd_cs_0x0605_notify_personal_message, notify_personal_message_);
			delete[] content_;
			return send_msg_to_player(player, cli_cmd_cs_0x0604_send_personal_msg, cli_out_);
		}
		//其他情况（不在对方黑名单中且对方好友列表中有自己）,就发聊天 消息给双方
		else {
			send_msg_to_player(ply, cli_cmd_cs_0x0605_notify_personal_message, notify_personal_message_);
			send_msg_to_player(player, cli_cmd_cs_0x0605_notify_personal_message, notify_personal_message_);
		}

		cli_out_.set_userid(cli_in_.userid());
		cli_out_.set_msg(std::string(content_));
		cli_out_.set_create_tm(cli_in_.create_tm());
		delete[] content_;
		return send_msg_to_player(player, cli_cmd_cs_0x0604_send_personal_msg, cli_out_);
	}

	//准备向switch服拉用户是否在线信息	
	switchproto::cs_sw_is_online cs_sw_is_online_;

	switchproto::sw_player_online_info_t* sw_player_online_info =
		cs_sw_is_online_.add_ol_info();
	sw_player_online_info->set_userid(cli_in_.userid());
	sw_player_online_info->set_u_create_tm(cli_in_.create_tm());
	sw_player_online_info->set_is_online(0);
	sw_player_online_info->set_team(0);

	delete[] content_;
	return g_switch->send_msg(player, g_online_id, player->create_tm, sw_cmd_sw_is_player_online,
			            cs_sw_is_online_);
}

int SendPersonalMsgCmdProcessor::proc_pkg_from_serv(
		        player_t* player, const char* body, int bodylen) 
{
	switch(player->serv_cmd) {
		case sw_cmd_sw_is_player_online:
			return proc_pkg_from_serv_aft_get_online_info(player, body, bodylen);
		default:
			return 0;
	}
	return 0;
}

int SendPersonalMsgCmdProcessor::proc_pkg_from_serv_aft_get_online_info(
		        player_t* player, const char* body, int bodylen) 
{
	switchproto::sc_sw_is_online online_infos;
    PARSE_SVR_MSG(online_infos);

	if (online_infos.ol_info_size() == 0) {
		return send_err_to_player(player, 
			player->cli_wait_cmd, cli_err_proto_format_err);
	}
	msg_friend_session_t* session = (msg_friend_session_t*)player->session;
	//屏蔽脏次
	// std::string msg = cli_in_.msg();
	// char * content_ = NULL;
	// if(!msg.empty()){
		// int len = msg.size();
		// content_ = new char[len + 1];
		// content_[len] = '\0';
        // strncpy(content_, msg.c_str(), len + 1); 
		// //检测脏词
		// tm_dirty_replace(content_);
	// } else {
		// content_ = new char('\0');
	// }
	onlineproto::sc_0x0605_notify_personal_message notify_personal_message_;

	//用户不在线,直接回包给客户端，用过userid = 0来告诉客户端
	//对方online处理逻辑在switch_proto的
	//SwitchTransmitCmdProcessor::proc_pkg_from_serv
	//的hack下边的if中,注意对应的协议
	if (online_infos.ol_info(0).is_online() == 0) {
		cli_out_.set_userid(0);
		cli_out_.set_create_tm(0);
		cli_out_.set_msg(std::string(session->content));
		
		notify_personal_message_.set_receiver(0);
		notify_personal_message_.set_r_create_tm(0);
		notify_personal_message_.set_sender(player->userid);
		notify_personal_message_.set_s_create_tm(player->create_tm);
		notify_personal_message_.set_msg(std::string(session->content));
		notify_personal_message_.set_sender_nick(player->nick);
		// delete[] content_;

		//处理recent列表
		FriendUtils::proc_recent_list(player, cli_in_.userid(),cli_in_.create_tm());
		send_msg_to_player(player, 
				cli_cmd_cs_0x0605_notify_personal_message, notify_personal_message_);
		return send_msg_to_player(player, 
				cli_cmd_cs_0x0604_send_personal_msg, cli_out_);
	}
	//用户在线则通过switch转发到对应online
	//
	
	switchproto::cs_sw_transmit_only cs_sw_transmit_only_;
	switchproto::sw_player_basic_info_t* sw_player_basic_info = 
		cs_sw_transmit_only_.add_receivers();

	sw_player_basic_info->set_userid(session->friendid);
	sw_player_basic_info->set_create_tm(0);

	cs_sw_transmit_only_.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
	cs_sw_transmit_only_.set_cmd(cli_cmd_cs_0x0605_notify_personal_message);

	notify_personal_message_.set_receiver(session->friendid);
	notify_personal_message_.set_r_create_tm(session->create_tm);
	notify_personal_message_.set_sender(player->userid);
	notify_personal_message_.set_s_create_tm(player->create_tm);
	notify_personal_message_.set_msg(std::string(session->content));
	notify_personal_message_.set_sender_nick(player->nick);

	std::string pkg;
	notify_personal_message_.SerializeToString(&pkg);
	cs_sw_transmit_only_.set_pkg(pkg);
	g_switch->send_msg(NULL, g_online_id, 0, sw_cmd_sw_transmit_only,
			cs_sw_transmit_only_);
	//处理recent列表
	FriendUtils::proc_recent_list(player, session->friendid, session->create_tm);

	send_msg_to_player(player, cli_cmd_cs_0x0605_notify_personal_message, notify_personal_message_);

	cli_out_.set_userid(session->friendid);
	cli_out_.set_create_tm(session->create_tm);
	cli_out_.set_msg(std::string(session->content));
	// delete[] content_;

	return send_msg_to_player(player, cli_cmd_cs_0x0604_send_personal_msg, cli_out_);
}
//此部分与添加好友协议类似
int AddBlacklistCmdProcessor::proc_pkg_from_client(
		        player_t* player, const char* body, int bodylen) 
{
	PARSE_MSG;
	uint32_t type = cli_in_.type();
	//黑名单上限
	if (player->friend_info->get_blacks_size() >= 200) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_black_reach_limit);
	}
	//通过id添加黑名单
	if (type == 0) {
		//不能添加自己到黑名单
		if (player->userid == cli_in_.userid() && player->create_tm == cli_in_.create_tm()) {
			return send_err_to_player(player, player->cli_wait_cmd,
					cli_err_friend_add_self_attention);
		}
		//向db发包查询好友是否存在
		dbproto::cs_check_user_exist cs_check_user_exist_;
		cs_check_user_exist_.set_server_id(g_server_id);
		cs_check_user_exist_.set_is_init_server(false);
		add_blacklist_session_t* session = (
				add_blacklist_session_t*)player->session;
		memset(session, 0, sizeof(*session));
		session->blackid = cli_in_.userid();
		session->create_tm = cli_in_.create_tm();

		return g_dbproxy->send_msg(player, cli_in_.userid(), cli_in_.create_tm(), 
				db_cmd_check_user_exist, cs_check_user_exist_);

	}
	//通过nick添加黑名单
	else if (type == 1) {
		//add_friend_session_t* add_friend_session = (
		//		add_friend_session_t*)player->session;
		//memset(add_friend_session, 0, sizeof(*add_friend_session));
		//add_friend_session->friendid = cli_in_.userid();

		//根据nick从db中拉取userid
		dbproto::cs_get_user_by_nick cs_get_user_by_nick_;
		cs_get_user_by_nick_.set_nick(cli_in_.nick());
		return g_dbproxy->send_msg(player, player->userid,player->create_tm,
				db_cmd_get_userid_by_nick , cs_get_user_by_nick_);
	}
	else {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_proto_format_err);
	}
}

int AddBlacklistCmdProcessor::proc_pkg_from_serv(
				player_t* player, const char* body, int bodylen) 
{
	switch(player->serv_cmd) {
		case db_cmd_check_user_exist:
			return proc_pkg_from_serv_aft_check_user_exist(player, body, bodylen);
		case db_cmd_get_userid_by_nick:
			return proc_pkg_from_serv_aft_get_userid_by_nick(player, body, bodylen);
		case sw_cmd_sw_is_player_online:
			return proc_pkg_from_serv_aft_get_online_info(player, body, bodylen);
		case cache_cmd_ol_req_users_info:
			return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
	}
	return 0;
}

int AddBlacklistCmdProcessor::proc_pkg_from_serv_aft_check_user_exist(
		player_t* player, const char* body, int bodylen)
{
	dbproto::sc_check_user_exist sc_check_user_exist_;
    PARSE_SVR_MSG(sc_check_user_exist_);

	add_blacklist_session_t* session = (add_blacklist_session_t*)player->session;

	switchproto::cs_sw_is_online cs_sw_is_online_;
	//用户存在，就向switch发包拉取用户是否在线信息
	if(sc_check_user_exist_.exist()) {
		uint32_t userid = session->blackid;
		uint32_t create_tm = session->create_tm;
		switchproto::sw_player_online_info_t* sw_player_online_info =
			cs_sw_is_online_.add_ol_info();
		sw_player_online_info->set_userid(userid);
		sw_player_online_info->set_u_create_tm(create_tm);
		sw_player_online_info->set_is_online(0);
		sw_player_online_info->set_team(1);
		return g_switch->send_msg(player, g_online_id, player->create_tm, sw_cmd_sw_is_player_online,
				cs_sw_is_online_, true);
	}
	//用户不存在，就返回错误码
	return send_err_to_player (player, player->cli_wait_cmd,                                                    
			cli_err_friend_id_not_exist);                                                                          
}

int AddBlacklistCmdProcessor::proc_pkg_from_serv_aft_get_userid_by_nick(
		player_t* player, const char* body, int bodylen)
{
	dbproto::sc_get_user_by_nick sc_get_userid_by_nick_;
    PARSE_SVR_MSG(sc_get_userid_by_nick_);

	uint32_t userid = sc_get_userid_by_nick_.userid();
	uint32_t create_tm = sc_get_userid_by_nick_.u_create_tm();
	//用户不存在
	if (userid == 0) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_friend_id_not_exist);
	}
	//不能拉自己到黑名单
	if (player->userid == userid && player->create_tm == create_tm) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_friend_add_self_attention);
	}

	add_blacklist_session_t* session = (
			add_blacklist_session_t*)player->session;
	memset(session, 0, sizeof(*session));
	//将id存入session
	session->blackid = sc_get_userid_by_nick_.userid();
	session->create_tm = sc_get_userid_by_nick_.u_create_tm();
	session->is_online = 0;

	switchproto::cs_sw_is_online cs_sw_is_online_;
	//向switch发包拉取用户是否在线信息
	switchproto::sw_player_online_info_t* sw_player_online_info =
		cs_sw_is_online_.add_ol_info();
	sw_player_online_info->set_userid(userid);
	sw_player_online_info->set_u_create_tm(create_tm);
	sw_player_online_info->set_is_online(0);
	sw_player_online_info->set_team(1);
	return g_switch->send_msg(player, g_online_id, player->create_tm, sw_cmd_sw_is_player_online,
			cs_sw_is_online_, 0);

}

int AddBlacklistCmdProcessor::proc_pkg_from_serv_aft_get_online_info(
		player_t* player, const char* body, int bodylen)
{
	cacheproto::cs_batch_get_users_info cs_batch_get_users_info_;
    switchproto::sc_sw_is_online online_infos;
    PARSE_SVR_MSG(online_infos);

	add_blacklist_session_t* session = (add_blacklist_session_t *)player->session;
	//将用户是否在线信息存入session
	for (int i = 0; i < online_infos.ol_info_size(); i++) {
		session->blackid = online_infos.ol_info(i).userid();
		session->create_tm = online_infos.ol_info(i).u_create_tm();
		session->is_online = online_infos.ol_info(i).is_online();
		session->team = online_infos.ol_info(i).team();
		// cs_batch_get_users_info_.add_uids(online_infos.ol_info(i).userid());
		commonproto::role_info_t *role_infos = cs_batch_get_users_info_.add_roles();
		role_infos->set_userid(online_infos.ol_info(i).userid()); 
		role_infos->set_u_create_tm(online_infos.ol_info(i).u_create_tm()); 
	}
	//准备向cache发包拉取信息
	return g_dbproxy->send_msg(player, player->userid, player->create_tm, 
            cache_cmd_ol_req_users_info, cs_batch_get_users_info_);
}

int AddBlacklistCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
		player_t* player, const char* body, int bodylen)
{
	cacheproto::sc_batch_get_users_info sc_batch_get_users_info_;
    PARSE_SVR_MSG(sc_batch_get_users_info_);

	//和客户端同步 并写入内存
	onlineproto::sc_0x0602_sync_friend_data sc_0x0602_sync_friend_data_;

	add_blacklist_session_t *session = (add_blacklist_session_t *)player->session;
	for (int i = 0; i < sc_batch_get_users_info_.user_infos_size(); i++) {
		commonproto::friend_info_t* friend_info = 
            sc_0x0602_sync_friend_data_.add_friend_list();
		const commonproto::player_base_info_t& base_info = 
            sc_batch_get_users_info_.user_infos(i).base_info();
		//将从cache服务器拉取到的信息打包到 online 对前端的回包中
		//如果前端面板今后需要展示更多cache拉取到的内容，都可以在此函数中添加
		FriendUtils::pack_friend_cache_data_to_online(base_info, friend_info);

		//将session中的信息存入online 对 前端的协议中
		friend_info->set_userid(session->blackid);
		friend_info->set_create_tm(session->create_tm);
		friend_info->set_is_online(session->is_online);
		friend_info->set_team(session->team);

		//将cache中拉到的信息写入内存
		//如果今后内存中需要更多从cache拉取的信息，都可以在此函数中添加，
		//并在结构体中添加相应字段
		black_t black_info;
		FriendUtils::pack_blacklist_data_to_player(base_info, &black_info);
		black_info.id = session->blackid;
		black_info.create_tm = session->create_tm;
		black_info.is_online = session->is_online;

		player->friend_info->add_black(black_info);

		//如果在好友列表中，删除掉
		//因为加入黑名单的用户已经不能是自己的好友
		if (player->friend_info->has_friend(session->blackid, session->create_tm)) {
			player->friend_info->remove_friend(session->blackid, session->create_tm);
		}
		
		//player->friend_info->remove_friend(session->blackid);
		//也从recentlist中删除
		recent_t recent;
		recent.id = session->blackid;
		recent.create_tm = session->create_tm;
		player->friend_info->remove_recent_element(recent);
		//同步更改后的recent list 给客户端
		std::vector<recent_t> recentlist;                                                                                           
		player->friend_info->get_all_recent(recentlist);
		for (int i = recentlist.size() - 1; i >= 0; i--) {
			// sc_0x0602_sync_friend_data_.add_recent_list(recentlist[i].id);
			commonproto::role_info_t *info = sc_0x0602_sync_friend_data_.add_recent_list();
			info->set_userid(recentlist[i].id);
			info->set_u_create_tm(recentlist[i].create_tm);
		}
	}
	send_msg_to_player(player, 
            cli_cmd_cs_0x0602_sync_friend_data, sc_0x0602_sync_friend_data_);

	//和db同步
	// dbproto::cs_save_friend cs_save_friend_;
	// cs_save_friend_.set_userid(player->userid);
	// cs_save_friend_.set_friendid(session->blackid);
	// cs_save_friend_.set_is_friend(0);
	// cs_save_friend_.set_is_recent(0);
	// cs_save_friend_.set_is_black(1);
	// cs_save_friend_.set_is_temp(0);

	dbproto::cs_save_friend cs_save_friend_;
	commonproto::friend_data_t finf;

	finf.set_userid(player->userid);
	finf.set_userid(player->create_tm);
	finf.set_friendid(session->blackid);
	finf.set_friendid(session->create_tm);
	finf.set_is_friend(0);
	finf.set_is_recent(0);
	finf.set_is_blacklist(1);
	finf.set_is_temp(0);

	cs_save_friend_.mutable_finf()->CopyFrom(finf);

	g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
			db_cmd_save_friend , cs_save_friend_);

	//回包提示操作完成
	cli_out_.Clear();
	cli_out_.set_result(0);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
//删除好友和删除黑名单都在这里处理
int RemoveFriendCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
	uint32_t friendid = cli_in_.userid();
	uint32_t create_tm = cli_in_.create_tm();

    bool is_friend = player->friend_info->has_friend(friendid, create_tm);
    bool is_black = player->friend_info->has_black(friendid,  create_tm);


	if (is_friend == false && is_black == false) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_not_friend);
	}

	//和客户端同步 并删除内存中的数据
	onlineproto::sc_0x0602_sync_friend_data sc_0x0602_sync_friend_data_;
	commonproto::friend_info_t* friend_info = sc_0x0602_sync_friend_data_.add_friend_list();

	friend_info->set_userid(friendid);
	friend_info->set_create_tm(create_tm);
	friend_info->set_team(2);
	
	if (is_friend) {
		player->friend_info->remove_friend(friendid, create_tm);
	}
	if (is_black) {
		player->friend_info->remove_black(friendid, create_tm);
	}
	recent_t recent;
	recent.id = friendid;
	recent.create_tm = create_tm;
	player->friend_info->remove_recent_element(recent);

	std::vector<recent_t> recentlist;                                                                                           
	player->friend_info->get_all_recent(recentlist);
	for (int i = recentlist.size() - 1; i >= 0; i--) {
		// sc_0x0602_sync_friend_data_.add_recent_list(recentlist[i].id);
		commonproto::role_info_t *info = sc_0x0602_sync_friend_data_.add_recent_list();
		info->set_userid(recentlist[i].id);
		info->set_u_create_tm(recentlist[i].create_tm);
	}

	send_msg_to_player(player, 
            cli_cmd_cs_0x0602_sync_friend_data, sc_0x0602_sync_friend_data_);

	//和db同步
	// dbproto::cs_remove_friend cs_remove_friend_;
	// cs_remove_friend_.set_userid(player->userid);
	// cs_remove_friend_.set_friendid(friendid);
	dbproto::cs_remove_friend cs_remove_friend_;
	commonproto::friend_data_t finf;
	
	finf.set_userid(player->userid);
	finf.set_u_create_tm(player->create_tm);
	finf.set_friendid(friendid);
	finf.set_f_create_tm(create_tm);
	finf.set_is_friend(0);
	finf.set_is_recent(0);
	finf.set_is_blacklist(0);
	finf.set_is_temp(0);

	cs_remove_friend_.mutable_finf()->CopyFrom(finf);

	return g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
			db_cmd_remove_friend , cs_remove_friend_);
}

int RemoveFriendCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	return 0;
}


//此部分与添加好友相同，
//唯一区别是添加好友拉取到最终信息后会发送添加好友申请
//而这里只会返回拉到的好友信息给客户端

int SearchFriendCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	uint32_t type = cli_in_.type();

	if (type == 0) {

		dbproto::cs_check_user_exist cs_check_user_exist_; 
		cs_check_user_exist_.set_server_id(g_server_id); 
		cs_check_user_exist_.set_is_init_server(false); 
		search_friend_session_t* search_friend_session = ( 
				search_friend_session_t*)player->session; 
		memset(search_friend_session, 0, sizeof(*search_friend_session)); 
		search_friend_session->friendid = cli_in_.userid(); 
		search_friend_session->create_tm = cli_in_.create_tm(); 

		return g_dbproxy->send_msg(player, cli_in_.userid(), cli_in_.create_tm(), 
				db_cmd_check_user_exist, cs_check_user_exist_); 
	}
	if (type == 1) {
		//add_friend_session_t* add_friend_session = (
		//		add_friend_session_t*)player->session;
		//memset(add_friend_session, 0, sizeof(*add_friend_session));
		//add_friend_session->friendid = cli_in_.userid();

		dbproto::cs_get_user_by_nick cs_get_userid_by_nick_;
		cs_get_userid_by_nick_.set_nick(cli_in_.nick());
		return g_dbproxy->send_msg(player, player->userid, player->create_tm,
				db_cmd_get_userid_by_nick , cs_get_userid_by_nick_);
	}
	else {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_proto_format_err);
	}

}

int SearchFriendCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen) {

	switch(player->serv_cmd) {
		case sw_cmd_sw_is_player_online:
			return proc_pkg_from_serv_aft_get_online_info(player, body, bodylen);
		case db_cmd_check_user_exist:
			return proc_pkg_from_serv_aft_check_user_exist(player, body, bodylen);
		case db_cmd_get_userid_by_nick:
			return proc_pkg_from_serv_aft_get_userid_by_nick(player, body, bodylen);
		case cache_cmd_ol_req_users_info:
			return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
	}
	return 0;
}

int SearchFriendCmdProcessor::proc_pkg_from_serv_aft_get_userid_by_nick(
		player_t* player, const char* body, int bodylen) 
{
	dbproto::sc_get_user_by_nick sc_get_userid_by_nick_;
    PARSE_SVR_MSG(sc_get_userid_by_nick_);

	uint32_t userid = sc_get_userid_by_nick_.userid();
	uint32_t create_tm = sc_get_userid_by_nick_.u_create_tm();
	if (userid == 0) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_friend_id_not_exist);
	}

	search_friend_session_t* search_friend_session = (
			search_friend_session_t*)player->session;
	memset(search_friend_session, 0, sizeof(*search_friend_session));

	search_friend_session->friendid = sc_get_userid_by_nick_.userid();
	search_friend_session->create_tm = sc_get_userid_by_nick_.u_create_tm();
	search_friend_session->is_online = 0;

	switchproto::cs_sw_is_online cs_sw_is_online_;

	switchproto::sw_player_online_info_t* sw_player_online_info =
		cs_sw_is_online_.add_ol_info();
	sw_player_online_info->set_userid(userid);
	sw_player_online_info->set_u_create_tm(create_tm);
	sw_player_online_info->set_is_online(0);
	sw_player_online_info->set_team(0);
	return g_switch->send_msg(player, g_online_id, player->create_tm,
            sw_cmd_sw_is_player_online, cs_sw_is_online_, 0);
}

int SearchFriendCmdProcessor::proc_pkg_from_serv_aft_check_user_exist(
		        player_t* player, const char* body, int bodylen) 
{
	dbproto::sc_check_user_exist sc_check_user_exist_;
    PARSE_SVR_MSG(sc_check_user_exist_);

	search_friend_session_t* search_friend_session = (search_friend_session_t*) player->session; 

	switchproto::cs_sw_is_online cs_sw_is_online_;

	if(sc_check_user_exist_.exist()) {
		uint32_t userid    = search_friend_session->friendid;
		uint32_t create_tm = search_friend_session->create_tm;

		switchproto::sw_player_online_info_t* sw_player_online_info =
			cs_sw_is_online_.add_ol_info();
		sw_player_online_info->set_userid(userid);
		sw_player_online_info->set_is_online(0);
		sw_player_online_info->set_team(0);
		sw_player_online_info->set_u_create_tm(create_tm);
		return g_switch->send_msg(player, g_online_id, player->create_tm,
			   	sw_cmd_sw_is_player_online, cs_sw_is_online_, 0);
	}

	commonproto::friend_info_t* friend_info = cli_out_.mutable_friend_info();

	friend_info->set_userid(0);
	friend_info->set_is_online(0);
	friend_info->set_team(0);
	friend_info->set_is_vip(0);
	friend_info->set_vip_grade(0);
	friend_info->set_nick("");

	return send_msg_to_player(player, cli_cmd_cs_0x0609_search_friend, cli_out_);
}

int SearchFriendCmdProcessor::proc_pkg_from_serv_aft_get_online_info(
		        player_t* player, const char* body, int bodylen) 
{
	cacheproto::cs_batch_get_users_info cs_batch_get_users_info_;
	switchproto::sc_sw_is_online online_infos;
    PARSE_SVR_MSG(online_infos);

	search_friend_session_t* session = (search_friend_session_t *)player->session;
	for (int i = 0; i < online_infos.ol_info_size(); i++) {
		session->friendid = online_infos.ol_info(i).userid();
		session->is_online = online_infos.ol_info(i).is_online();
		session->team = online_infos.ol_info(i).team();
		commonproto::role_info_t *role_infos = cs_batch_get_users_info_.add_roles();
		role_infos->set_userid(online_infos.ol_info(i).userid()); 
		role_infos->set_u_create_tm(online_infos.ol_info(i).u_create_tm()); 
		// cs_batch_get_users_info_.add_uids(online_infos.ol_info(i).userid());
	}

	return g_dbproxy->send_msg(player, player->userid, player->create_tm, 
            cache_cmd_ol_req_users_info, cs_batch_get_users_info_, 0);
}

int SearchFriendCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
		        player_t* player, const char* body, int bodylen) 
{
	cacheproto::sc_batch_get_users_info sc_batch_get_users_info_;
    PARSE_SVR_MSG(sc_batch_get_users_info_);

	//返回结果给客户端 
	search_friend_session_t *session = (search_friend_session_t *)player->session;
	for (int i = 0; i < sc_batch_get_users_info_.user_infos_size(); i++) {
		commonproto::friend_info_t* friend_info = cli_out_.mutable_friend_info();
		const commonproto::player_base_info_t& base_info 
            = sc_batch_get_users_info_.user_infos(i).base_info();
		//将从cache服务器拉取到的信息打包到 online 对前端的回包中
		//如果前端面板今后需要展示更多cache拉取到的内容，都可以在此函数中添加
		FriendUtils::pack_friend_cache_data_to_online(base_info, friend_info);
		
		//将session中的信息存入online 对 前端的协议中
		friend_info->set_userid(session->friendid);
		friend_info->set_is_online(session->is_online);
		friend_info->set_team(session->team);

	}
	return send_msg_to_player(player, cli_cmd_cs_0x0609_search_friend, cli_out_);
}

int RecommendationCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	
	switchproto::cs_sw_get_userid_list cs_sw_get_userid_list_;
	cs_sw_get_userid_list_.set_server_id(g_server_id);
	return g_switch->send_msg(player, g_online_id, player->create_tm,
			sw_cmd_sw_get_userid_list, cs_sw_get_userid_list_);

}

int RecommendationCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	switch(player->serv_cmd) {
	case sw_cmd_sw_get_userid_list:
		return proc_pkg_from_serv_aft_get_userid_list(player, body, bodylen);
	case cache_cmd_ol_req_users_info:
		return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
	}
	return 0;
}

int RecommendationCmdProcessor::proc_pkg_from_serv_aft_get_userid_list(
		player_t* player, const char* body, int bodylen)
{

	switchproto::sc_sw_get_userid_list sc_sw_get_userid_list_;
	cacheproto::cs_batch_get_users_info cs_batch_get_users_info_;
    PARSE_SVR_MSG(sc_sw_get_userid_list_);
	
	for (int i = 0; i < sc_sw_get_userid_list_.users().size(); i++) {
		//准备发包拉取cache信息
		cs_batch_get_users_info_.add_roles()->CopyFrom(sc_sw_get_userid_list_.users(i));
	}

	return g_dbproxy->send_msg(player, player->userid, player->create_tm, 
			cache_cmd_ol_req_users_info, cs_batch_get_users_info_, 0);
}


int RecommendationCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
		player_t* player, const char* body, int bodylen)
{
	cacheproto::sc_batch_get_users_info sc_batch_get_users_info_;
	PARSE_SVR_MSG(sc_batch_get_users_info_);

	std::map<uint32_t, recommendation_t, less<uint32_t> > m_recommendation;
	m_recommendation.clear();
	for (int i = 0; i < sc_batch_get_users_info_.user_infos_size(); i++) {
		//commonproto::friend_info_t* friend_info = cli_out_.add_friend_list();
		
		recommendation_t recommendation;	

		const commonproto::player_base_info_t& base_info = 
			sc_batch_get_users_info_.user_infos(i).base_info();
		//不能 推荐自己
		if (base_info.user_id() == player->userid && base_info.create_tm() == player->create_tm) {
			continue;
		}
		//必须有名字
		std::string nick = base_info.nick();
		if (nick.empty()) {
			continue;
		}
		//不推荐自己好友列表中的玩家
		if (player->friend_info->has_friend(base_info.user_id(), base_info.create_tm())) {
			continue;
		}
		//将cache中拉到的信息加入到协议中
		recommendation.team = 0;
		recommendation.userid = base_info.user_id();
		recommendation.create_tm = base_info.create_tm();
		recommendation.is_online = 1;

		int level = GET_A(kAttrLv);
		FriendUtils::pack_friend_cache_data_to_recommendation(base_info, recommendation);
		//存入map, 按照等级差由小到大排序
		uint32_t key = std::abs((int)base_info.level() - level);
		m_recommendation[key] = recommendation; 
		// m_recommendation[std::abs((int)base_info.level() - level)] = recommendation; 
	}
	cli_out_.Clear();
	uint32_t count = 0;
	//取出前10个返回给客户端
	for (std::map<uint32_t, recommendation_t, less<uint32_t> >::iterator it = m_recommendation.begin();
			it != m_recommendation.end() && count < 10; it++) {
		count++;
		commonproto::friend_info_t* friend_info = cli_out_.add_friend_list();
		friend_info->set_userid(it->second.userid);
		friend_info->set_create_tm(it->second.create_tm);
		friend_info->set_is_online(it->second.is_online);
		friend_info->set_nick(it->second.nick.c_str());
		friend_info->set_is_vip(it->second.is_vip);
		friend_info->set_vip_grade(it->second.vip_grade);
		friend_info->set_power(it->second.power);
		friend_info->set_team(it->second.team);
		friend_info->set_level(it->second.level);
		friend_info->set_sex(it->second.sex);
		friend_info->set_last_login_time(it->second.last_login_time);
	}

	return send_msg_to_player(player, cli_cmd_cs_0x060F_recommendation, cli_out_);
}

int GetFriendInfoCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{

	PARSE_MSG;
	//将前端发来的的id存入swich的协议中，准备拉取是否在线
	switchproto::cs_sw_is_online cs_sw_is_online_;
	for (uint32_t i = 0; i < (uint32_t)cli_in_.roles().size(); i++) {
		switchproto::sw_player_online_info_t* sw_player_online_info =
			cs_sw_is_online_.add_ol_info();
		const commonproto::role_info_t role = cli_in_.roles(i);
		sw_player_online_info->set_userid(role.userid());
		sw_player_online_info->set_u_create_tm(role.u_create_tm());
		sw_player_online_info->set_is_online(0);
		sw_player_online_info->set_team(0);
	}
	return g_switch->send_msg(player, g_online_id, player->create_tm, 
            sw_cmd_sw_is_player_online, cs_sw_is_online_);
}

int GetFriendInfoCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	switch(player->serv_cmd) {
	case sw_cmd_sw_is_player_online:
		return proc_pkg_from_serv_aft_get_online_info(player, body, bodylen);
	case cache_cmd_ol_req_users_info:
		return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
	}
	return 0;
}

int GetFriendInfoCmdProcessor::proc_pkg_from_serv_aft_get_online_info(
		player_t* player, const char* body, int bodylen)
{
	switchproto::sc_sw_is_online online_infos;
    PARSE_SVR_MSG(online_infos);

	cacheproto::cs_batch_get_users_info cs_batch_get_users_info_;
	cli_out_.Clear();

	for (int i = 0; i < online_infos.ol_info_size(); i++) {
		//把好友在线信息写入内存
		if (online_infos.ol_info(i).team() == 0){
			friend_t friend_data;
			player->friend_info->get_friend_by_id(online_infos.ol_info(i).userid(), online_infos.ol_info(i).u_create_tm(), friend_data);
			friend_data.is_online = online_infos.ol_info(i).is_online();
			player->friend_info->add_friend(friend_data);
		}
		//准备发包拉取cache信息
		// cs_batch_get_users_info_.add_uids(online_infos.ol_info(i).userid());
		commonproto::role_info_t *inf = cs_batch_get_users_info_.add_roles();
		inf->set_userid(online_infos.ol_info(i).userid());
		inf->set_u_create_tm(online_infos.ol_info(i).u_create_tm());
	}
	return g_dbproxy->send_msg(player, player->userid, player->create_tm, 
            cache_cmd_ol_req_users_info, cs_batch_get_users_info_, 0);
}


int GetFriendInfoCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
		player_t* player, const char* body, int bodylen)
{
	cacheproto::sc_batch_get_users_info sc_batch_get_users_info_;
    PARSE_SVR_MSG(sc_batch_get_users_info_);

	for (int i = 0; i < sc_batch_get_users_info_.user_infos_size(); i++) {
		commonproto::friend_info_t* friend_info = cli_out_.add_friend_list();
		const commonproto::player_base_info_t& base_info = 
            sc_batch_get_users_info_.user_infos(i).base_info();

		if (player->friend_info->has_friend(base_info.user_id(), base_info.create_tm())) {
			friend_info->set_team(0);
			//从内存中读取好友信息
			friend_t friend_data;
			player->friend_info->get_friend_by_id(base_info.user_id(), base_info.create_tm(), friend_data);

			//将内存中信息加入到协议中
			friend_info->set_userid(friend_data.id);
			friend_info->set_is_online(friend_data.is_online);
			friend_info->set_create_tm(friend_data.create_tm);
			//friend_info->set_send_gift_count(friend_data.gift_count);

			//将缓存中最近的时间
			friend_data.last_login_tm = base_info.last_login_tm();
			player->friend_info->add_friend(friend_data);

			friend_t tmp_test;
			player->friend_info->get_friend_by_id(base_info.user_id(), base_info.create_tm(), tmp_test);
		}
		//将cache中拉到的信息加入到协议中
		FriendUtils::pack_friend_cache_data_to_online(base_info, friend_info);
	}
	return send_msg_to_player(player, cli_cmd_cs_0x0610_get_friend_info, cli_out_);
}
