#ifndef __HOME_PROCESSOR_H__
#define __HOME_PROCESSOR_H__

#include "cmd_processor_interface.h"
#include "common.h"

//进入小屋
class AccessHomeCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	int proc_pkg_from_serv_aft_get_home_info(player_t* player, const char *body, int bodylen);
	int proc_pkg_from_serv_aft_get_host_pet_info(player_t* player, const char* body, int bodylen);

	int proc_pkg_from_serv_aft_exit_home(player_t* player, const char *body, int bodylen);
	onlineproto::cs_0x0122_access_home cli_in_;
	onlineproto::sc_0x0122_access_home cli_out_;
	homeproto::sc_enter_home home_out_;
	dbproto::sc_pet_list_get_info db_out_;
};

//新增一条访客日志
class HmAddVisitLogCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0133_add_visit_log cli_in_;
	onlineproto::sc_0x0133_add_visit_log cli_out_;
	dbproto::sc_get_attr db_out_;
};

class HmGetVisitLogCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0132_home_get_visit_log_list cli_in_;
	onlineproto::sc_0x0132_home_get_visit_log_list cli_out_;
	dbproto::sc_get_visit_log db_out_;
};

//小屋送礼(小屋送碎片)： 向好友发索要礼物的通知
class HmAskForGiftCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen) {
		return 0;
	}
private:
	onlineproto::cs_0x013B_hm_ask_for_gift cli_in_;
	onlineproto::sc_0x013B_hm_ask_for_gift cli_out_;
};

class HmGiftExchgCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0143_hm_gift_exchange cli_in_;
	onlineproto::sc_0x0143_hm_gift_exchange cli_out_;	
};

#endif
