#ifndef __SWITCH_PROTO_H__
#define __SWITCH_PROTO_H__

#include "common.h"
#include "cmd_processor_interface.h"

//=================================//
//==PROTO PROCESSOR================//
//=================================//
class TransmitMsgCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);

private:
    switchproto::cs_sw_transmit_only in_;
    switchproto::sc_sw_transmit_only out_;
};

class ServerRegCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
private:
    switchproto::cs_register_server in_;
    switchproto::sc_register_server out_;
};

class OnlineSyncPlayerInfoCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
private:
    switchproto::cs_online_sync_player_info in_;
};

class OnlineReportPlayerStateCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
private:
    switchproto::cs_online_report_player_onoff in_;
};

class GetSvrListCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
private:
    switchproto::cs_get_server_list in_;
    switchproto::sc_get_server_list out_;
};

class IsPlayerOnlineCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
private:
    switchproto::cs_sw_is_online in_;
    switchproto::sc_sw_is_online out_;
};

class GetUseridListCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
private:
    switchproto::cs_sw_get_userid_list in_;
    switchproto::sc_sw_get_userid_list out_;
};


class EraseEscortInfoCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
private:
	switchproto::cs_sw_req_svr_erase_player_escort_info req_in_;
	switchproto::sc_sw_notify_svr_erase_player_escort_info noti_out_;
};

class ChangeOtherAttrCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
private:
	switchproto::cs_sw_change_other_attr req_in_;
	switchproto::sc_sw_notify_attr_changed_by_other noti_out_;
};

class GmNewMailCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
	int proc_pkg_from_serv(server_t *svr, const char* body, int bodylen);
private:
	switchproto::cs_sw_gm_new_mail  in_;
	switchproto::sc_sw_gm_new_mail  out_;
	dbproto::sc_mail_new db_out_;
};

class FrozenAccountCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
	int proc_pkg_from_serv(server_t *svr, const char* body, int bodylen);
private:
	int proc_pkg_from_get_attr(server_t *svr, const char* body, int bodylen);
	int proc_pkg_from_change_other_attr(server_t *svr, const char* body, int bodylen);
	switchproto::cs_sw_gm_frozen_account in_;
	switchproto::sc_sw_gm_frozen_account out_;
	dbproto::sc_get_attr db_out_;
	uint32_t frozen_end_time;
};

class ActRechargeDiamondCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
	int proc_pkg_from_serv(server_t *svr, const char* body, int bodylen);
private:
	int proc_pkg_from_svr_after_get_base_info(
			server_t *svr, const char* body, int bodylen);
	int proc_pkg_from_svr_after_insert_new_trans(
			server_t *svr, const char* body, int bodylen);
	int proc_pkg_from_svr_after_recharge_diamond(
			server_t *svr, const char* body, int bodylen);
	switchproto::sc_sw_notify_attr_changed_by_other noti_out_;
	dbproto::sc_get_base_info db_out_;
	switchproto::platform_recharge_diamond in_;
	dbproto::sc_change_attr_value db_chg_value_;
};

class ActGetRoleInfoCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
	int proc_pkg_from_serv(server_t *svr, const char* body, int bodylen);
private:
	dbproto::sc_get_base_info db_out_;
};

class GmNewMailToSvrCmdProcessor : public CmdProcessorInterface
{
	int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
private:
	switchproto::cs_sw_gm_new_mail_to_svr in_;
	switchproto::sc_sw_gm_new_mail_to_svr out_;
};

class PlatformGetRoleInfoExCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
    int proc_pkg_from_serv(server_t *svr, const char *body, int bodylen);
private:
    dbproto::cs_get_base_info db_in_;
    dbproto::sc_get_base_info db_out_;
};

class PlatformIfRoleLoginDuringTmCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(server_t *svr, const char *body, int bodylen);
    int proc_pkg_from_serv(server_t *svr, const char *body, int bodylen);
private:
    dbproto::cs_get_login_tm_info db_in_;
    dbproto::sc_get_login_tm_info db_out_;
};

#if 0
// FOR GM TOOLS
// 禁言请求
int forbid_player_talk_cmd(server_t *svr,
                        ISeer20SWProto::sw_msg_head_t *msghead,
                        ISeer20SWProto::sw_gm_forbid_talk_in *msg);
// DB禁言回包
int db_forbid_player_talk_rsp(server_t *svr, 
                            ISeer20DBProto::db_msg_head_t *msghead,
                            ISeer20DBProto::db_forbid_player_talk_out *msg);
// 通知online(如果在线的话)用户被禁言
int notify_player_forbid_talk(uint32_t userid, int32_t role_tm, int32_t dur);


// 封停账号请求
int frozen_player_account_cmd(server_t *svr,
                            ISeer20SWProto::sw_msg_head_t *msghead,
                            ISeer20SWProto::sw_gm_frozen_account_in *msg);
// DB封停回包
int db_frozen_account_rsp(server_t *svr, 
                        ISeer20DBProto::db_msg_head_t *msghead,
                        ISeer20DBProto::db_frozen_account_out *msg);
//通知online(如果在线的话 在线还会被T下线)和DB用户账户被封停
int notify_player_frozen_account(uint32_t userid, int32_t role_tm, int32_t dur);


//拉取玩家全量信息请求
int get_player_total_info_cmd(server_t *svr,
                            ISeer20SWProto::sw_msg_head_t *msghead,
                            ISeer20SWProto::sw_gm_get_player_total_in *msg);
int db_get_player_total_req(server_t *svr, uint32_t uid, int32_t role_tm);
//拉取玩家全量信息回包
int db_get_player_total_info_rsp(server_t *svr, 
                                ISeer20DBProto::db_msg_head_t *msghead,
                                ISeer20DBProto::get_player_total_out *msg);

// 更新邀请玩家的数据回包
int db_update_inv_code_invitee_rsp(server_t *svr, 
                                   ISeer20DBProto::db_msg_head_t *msghead,
                                   ISeer20DBProto::db_update_inv_code_invitee_out *msg);

//广播
int gm_send_noti_cmd(server_t *svr,
                    ISeer20SWProto::sw_msg_head_t *msghead,
                    ISeer20SWProto::sw_gm_send_noti_in *msg);

//补偿物品
int gm_add_item_cmd(server_t *svr,
                    ISeer20SWProto::sw_msg_head_t *msghead,
                    ISeer20SWProto::sw_gm_add_item_in *msg);
//补偿物品邮件回包
int db_send_mail_rsp(server_t *svr,
                    ISeer20DBProto::db_msg_head_t *msghead,
                    ISeer20DBProto::db_send_mail_out *msg);

//改变属性
int gm_change_attr_cmd(server_t *svr,
                    ISeer20SWProto::sw_msg_head_t *msghead,
                    ISeer20SWProto::sw_gm_change_attr_in *msg);
//DB改变属性回包
int db_change_attr_rsp(server_t *svr,
                    ISeer20DBProto::db_msg_head_t *msghead,
                    ISeer20DBProto::db_update_player_basic_info_out *msg);

int gm_get_role_list_cmd(server_t *svr,
                        ISeer20SWProto::sw_msg_head_t *msghead,
                        ISeer20SWProto::sw_gm_get_role_list_in *msg);
//DB拉取角色列表
int db_get_role_list_rsp(server_t *svr,
                        ISeer20DBProto::db_msg_head_t *msghead,
                        ISeer20DBProto::db_get_role_list_out *msg);
#endif


#endif//END OF SWITCH_PROTO_H
