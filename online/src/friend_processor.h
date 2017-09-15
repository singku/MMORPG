#ifndef __FRIEND_PROCESSOR_H__
#define __FRIEND_PROCESSOR_H__

#include "common.h"
#include "global_data.h"
#include "cmd_processor_interface.h"
#include "interface_manager.h"
#include "proto/client/pb0x06.pb.h"

class RefreshFriendDataCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_online_info(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_cache_info(player_t* player, const char* body, int bodylen);

	struct get_friend_data_base_info_t 
	{
		uint32_t userid;
		uint32_t create_tm;
		bool is_online;
		uint32_t team;
	};

private:
	onlineproto::cs_0x0601_refresh_friend_data cli_in_;
	onlineproto::sc_0x0601_refresh_friend_data cli_out_;
};

class AddFriendCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_online_info(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_check_user_exist(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_userid_by_nick(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_cache_info(player_t* player, const char* body, int bodylen);
	struct add_friend_session_t {
		uint32_t friendid;
		uint32_t create_tm;
		bool is_online;
		uint32_t team;
	};
private:
	onlineproto::cs_0x0603_add_friend cli_in_;
	onlineproto::sc_0x0603_add_friend cli_out_;
};
class SendPersonalMsgCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_online_info(player_t* player, const char* body, int bodylen);
	struct msg_friend_session_t {
		uint32_t friendid;
		uint32_t create_tm;
		char content[kMaxMsgLength];
	};
private:
	onlineproto::cs_0x0604_send_personal_msg cli_in_;
	onlineproto::sc_0x0604_send_personal_msg cli_out_;
};


class AddBlacklistCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_online_info(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_check_user_exist(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_userid_by_nick(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_cache_info(player_t* player, const char* body, int bodylen);
	struct add_blacklist_session_t {
		uint32_t blackid;
		uint32_t create_tm;
		bool is_online;
		uint32_t team;
	};
private:
	onlineproto::cs_0x0607_add_blacklist cli_in_;
	onlineproto::sc_0x0607_add_blacklist cli_out_;
};

class RemoveFriendCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
	onlineproto::cs_0x0608_remove_friend cli_in_;
	onlineproto::sc_0x0608_remove_friend cli_out_;
};

class SearchFriendCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_online_info(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_check_user_exist(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_userid_by_nick(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_cache_info(player_t* player, const char* body, int bodylen);
	struct search_friend_session_t {
		uint32_t friendid;
		uint32_t create_tm;
		bool is_online;
		uint32_t team;
	};
private:
	onlineproto::cs_0x0609_search_friend cli_in_;
	onlineproto::sc_0x0609_search_friend cli_out_;
};

class RecommendationCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_userid_list(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_cache_info(player_t* player, const char* body, int bodylen);

	struct get_friend_data_base_info_t 
	{
		uint32_t userid;
		uint32_t create_tm;
		bool is_online;
		uint32_t team;
	};

private:
	onlineproto::cs_0x060F_recommendation cli_in_;
	onlineproto::sc_0x060F_recommendation cli_out_;
};

class GetFriendInfoCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_online_info(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_cache_info(player_t* player, const char* body, int bodylen);

private:
	onlineproto::cs_0x0610_get_friend_info cli_in_;
	onlineproto::sc_0x0610_get_friend_info cli_out_;
};
#endif
