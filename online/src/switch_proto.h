#ifndef __SWITCH_PROTO_H__
#define __SWITCH_PROTO_H__

//===========================SWITCH 协议函数=============//
#include "common.h"
#include "cmd_processor_interface.h"

//请求注册到sw(分主动调用和定时器超时再次调用)
int reg_to_switch_req(void *owner, void *data);

//online同步玩家信息
int online_sync_player_info();
//online报告玩家上下线
int online_report_player_onoff(player_t *p, uint8_t login_or_logout);

//NOTI(singku) 由于switch只和online打交道,不和玩家交互,下面回调
//函数中的p都是0值,但为了统一的的回调注册handler,加上了这个参数
//函数中注意不要使用

//switch 返回的协议回调函数
//注册回包
class SwitchRegCmdProcessor : public CmdProcessorInterface 
{
public:
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
};

//获取列表回包
class SwitchGetSvrListCmdProcessor : public CmdProcessorInterface 
{
public:
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    switchproto::sc_get_server_list sw_out_;
};

//swtich T人通知包
class SwitchNotifyKickPlayerCmdProcessor : public CmdProcessorInterface 
{
public:
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    switchproto::sc_sw_notify_kick_player_off sw_out_;
};

//跨服消息
class SwitchTransmitCmdProcessor : public CmdProcessorInterface 
{
public:
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    switchproto::sc_sw_transmit_only sw_out_;
};

class SwitchNotifyEraseEscortInfoCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	switchproto::sc_sw_notify_svr_erase_player_escort_info sw_out_;
};

class SwitchNotifyAttrChangedCmdProcessor: public CmdProcessorInterface
{
public:
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	switchproto::sc_sw_notify_attr_changed_by_other sw_out_;
};

class SwitchNotifyNewMailCmdProcessor :public CmdProcessorInterface
{
public:
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	switchproto::sc_sw_notify_new_mail sw_out_;
};

class SwNotifyNewMailToSvrCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	switchproto::sc_sw_notify_new_mail sw_out_;
};

class SwNtfFrozenAccountCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	switchproto::sc_sw_notify_player_frozen_account sw_out_;
};

class SwOnlyNotifyAttrChangedCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	switchproto::sc_sw_only_notify_attr_changed sw_out_;
};

#if 0
//禁言通知
int sw_notify_player_forbid_talk(player_t *p,
                                ISeer20SWProto::sw_msg_head_t *msghead,
                                ISeer20SWProto::sw_notify_player_forbid_talk_out *msg);
//封号通知
int sw_notify_player_frozen_account(player_t *p,
                                ISeer20SWProto::sw_msg_head_t *msghead,
                                ISeer20SWProto::sw_notify_player_frozen_account_out *msg);
//GM改属性通知
int sw_notify_gm_change_attr(player_t *p,
                            ISeer20SWProto::sw_msg_head_t *msghead,
                            ISeer20SWProto::sw_notify_change_attr_out *msg);
//GM发物品补偿邮件通知
int sw_notify_gm_new_mail(player_t *p,
                        ISeer20SWProto::sw_msg_head_t *msghead,
                        ISeer20SWProto::sw_notify_new_mail_out *msg);

//广播通知
int sw_notify_gm_send_noti(player_t *p,
                        ISeer20SWProto::sw_msg_head_t *msghead,
                        ISeer20SWProto::sw_notify_gm_noti_out *msg);
#endif

#endif//__SWITCH_PROTO_H__
