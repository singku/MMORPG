#ifndef __SWITCH_CMD_H__
#define __SWITCH_CMD_H__

enum switch_cmd_t {
    sw_cmd_register_server                      = 0x8000,
    sw_cmd_online_sync_player_info              = 0x8001,
    sw_cmd_get_server_list                      = 0x8002,
    sw_cmd_online_report_player_onoff           = 0x8003,
    sw_cmd_sw_notify_kick_player_off            = 0x8004,
    sw_cmd_sw_transmit_only                     = 0x8005,
    sw_cmd_sw_relay_cs                          = 0x8006,
    sw_cmd_sw_gm_forbid_talk                    = 0x8007,
    sw_cmd_sw_notify_player_forbid_talk         = 0x8008,
    sw_cmd_sw_gm_frozen_account                 = 0x8009,
    sw_cmd_sw_notify_player_frozen_account      = 0x800A,
    sw_cmd_sw_gm_get_player_total               = 0x800B,
    sw_cmd_sw_gm_send_noti                      = 0x800C,
    sw_cmd_sw_notify_gm_noti                    = 0x800D,
    sw_cmd_sw_gm_new_mail                       = 0x800E,
    sw_cmd_sw_notify_new_mail                   = 0x800F,
    sw_cmd_sw_gm_change_attr                    = 0x8010,
    sw_cmd_sw_notify_change_attr                = 0x8011,
    sw_cmd_sw_gm_get_role_list                  = 0x8012,
    sw_cmd_sw_is_player_online                  = 0x8013,
	sw_cmd_sw_req_erase_player_escort_info	    = 0x8015,	//请求旧服删除玩家的运宝信息
	sw_cmd_sw_notify_erase_player_escort_info	= 0x8016,	//通知旧服删除玩家的运宝信息
	sw_cmd_sw_change_other_attr					= 0x8017,	//设置其他玩家的属性
    sw_cmd_sw_notify_attr_changed_by_other      = 0x8018,   //通知玩家属性被他人修改
	sw_cmd_sw_get_userid_list                   = 0x8019,
    sw_cmd_sw_notify_server_stat                = 0x8020,   //通知其他服某服断线挂掉/启动了
	sw_cmd_sw_gm_new_mail_to_svr				= 0x801A,	//按服发邮件
	sw_cmd_sw_notify_new_mail_to_svr				= 0x801B,	//全服通知新邮件
	sw_cmd_sw_only_notify_player_attr_changed	= 0x801C,	//仅仅通知玩家属性被修改
	sw_cmd_gm_modify_vip_time					= 0x801D,	//客服修改vip时间


	//WARNING!!!!   0x8050 - 0x8200提供给运营开发平台与switch之间的通信
	sw_cmd_sw_recharge_diamond					= 0x8050,	//平台通知给玩家充钻石协议
	sw_cmd_sw_get_role_info						= 0x8051,	//平台拉取玩家角色信息
    sw_cmd_sw_get_role_info_ex				    = 0x8052,	//平台拉取玩家角色信息
    sw_cmd_sw_if_role_login_during_tm	        = 0x8053,	//平台请求玩家在给定的时间段是否登录

};

#endif
