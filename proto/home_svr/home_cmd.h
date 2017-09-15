#ifndef __HOME_CMD_H__
#define __HOME_CMD_H__

enum home_cmd_t {
    /* 小屋命令号段 */
    home_cmd_report_onoff           = 0x8800,
    home_cmd_sync_map_player_info   = 0x8801,
    home_cmd_enter_home             = 0x8802,   //进入小屋
    home_cmd_notify_entered         = 0x8803,   //通知房主有人进入
    home_cmd_exit_home              = 0x8804,   //离开小屋
    home_cmd_gen_visit_log          = 0x8805,   //online通知小屋 产生一条访问日志
    home_cmd_notify_visited         = 0x8806,   //home通知屋主  产生了访问日志
    home_cmd_update_home_info       = 0x8807,   //online通知小屋更新小屋信息
    home_cmd_change_state           = 0x8808,   //改变坐标
    home_cmd_family_hall_sync_map_player_info       = 0x8809,   //online通知更新家族大厅信息
    home_cmd_family_hall_state_change               = 0x880a,   //改变坐标
    home_cmd_enter_family_hall      = 0x880b,   // 进入家族大厅
    home_cmd_leave_family_hall      = 0x880c,   // 离开家族大厅
	home_cmd_notify_pet_exe_state_change	= 0x880d,	//屋主锻炼的精灵召回，通知小屋里其他玩家
};

#endif
