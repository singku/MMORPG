#ifndef __ESCORT_PROCESSOR_H__
#define __ESCORT_PROCESSOR_H__
#include "common.h"
#include "cmd_processor_interface.h"

//刷新飞船类型
class EscortRefreshShipCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen) {
		return 0;
	}
private:
	onlineproto::cs_0x0801_escort_refresh_airship cli_in_;
	onlineproto::sc_0x0801_escort_refresh_airship cli_out_;
};

class EscortBuyShipCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x081A_buy_ship cli_in_;
	onlineproto::sc_0x081A_buy_ship cli_out_;
};

class EscortGetOtherShipCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen) {
		return 0;
	}
private:
	onlineproto::cs_0x0802_escort_get_other_airship cli_in_;
	onlineproto::sc_0x0802_escort_get_other_airship cli_out_;
};

class EscortStartCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen) {
		return 0;
	}
private:
	onlineproto::cs_0x0803_escort_start cli_in_;
	onlineproto::sc_0x0803_escort_start cli_out_;
};

//打劫
class EscortRobberyCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	struct escort_robbery_session_t
	{
		uint32_t atk_tick;
		uint32_t def_tick;
	};
private:
	int proc_pkg_from_serv_aft_get_cache_info(player_t *player, const char *body, int bodylen); 
	onlineproto::cs_0x0805_escort_robbery cli_in_;
	onlineproto::sc_0x0805_escort_robbery cli_out_;
	dbproto::sc_get_attr db_out_;
	cacheproto::sc_batch_get_users_info  cache_info_out_;
};

//打劫结果
class EscortRobberyResultCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen) {
		return 0;
	}
private:
	onlineproto::cs_0x0810_robbery_result cli_in_;
	onlineproto::sc_0x0810_robbery_result cli_out_;
};

//获得运宝奖励
class EscortGetRewardCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen) {
		return 0;
	}
private:
	onlineproto::cs_0x0808_escort_get_reward cli_in_;
	onlineproto::sc_0x0808_escort_get_reward cli_out_;	
};

#endif
