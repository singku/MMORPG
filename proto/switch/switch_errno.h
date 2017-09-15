#ifndef __SWITCH_ERRNO_H__
#define __SWITCH_ERRNO_H__

enum switch_errno_t {
    sw_err_no_err                   = 0,
//  平台错误码
	sw_not_role_info				= 1,	//平台拉取角色信息不存在

    sw_err_sys_err                  = 50001,
    sw_err_sys_busy                 = 50002,
    sw_err_wrong_passwd             = 50003, //第三方发送通知需要验证密码
    sw_err_player_not_exist         = 50004,
    sw_err_server_not_exist         = 50005,
    sw_err_server_reg_already_exist = 50006, //server已经存在 重复注册
    sw_err_need_register            = 50007, //需要先注册才能操作
    sw_err_server_conflict          = 50008, //svrid冲突 又不是注册协议
    sw_err_proto_format_err         = 50009, 
    sw_err_server_type_invalid      = 50010, //服务器类型不对
    sw_err_invalid_transmit_type    = 50011, //传递消息的类型不对
    sw_err_cmd_not_found            = 50012,
    sw_err_proc_time_out            = 50013, //其他服务器处理超时
	sw_err_account_has_frozen		= 50014, //帐号已经被封
	sw_err_frozen_ope_err			= 50015, //封号操作有误，封号与解封不能同时设置
	sw_err_must_give_svr_id			= 50016, //平台那边必须选择指定的服务器
};

#endif

