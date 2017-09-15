#ifndef __BTL_CMD_H__
#define __BTL_CMD_H__

enum btl_cmd_t {
    btl_cmd_start                   = 0x9000,
    btl_cmd_msg_relay               = 0x9000,   /*cli消息直接传递给btl*/
    btl_cmd_enter_duplicate         = 0x9001,   /*进入副本*/
    btl_cmd_notify_kill_character   = 0x9002,   /*客户端杀死人了*/
    btl_cmd_notify_end              = 0x9003,   /*副本结束了*/
    btl_cmd_revival                 = 0x9004,   /*复活*/
    btl_cmd_notify_msg_relay        = 0x9005,   /*battle给cli直接通知包*/
    btl_cmd_notify_match_result     = 0x9006,   /*battle_center给online回匹配进入回包*/
    btl_cmd_give_up_match           = 0x9007,   /*实时竞技场放弃对战匹配*/
    btl_cmd_notify_battle_down      = 0x9009,   /*btc通知online,btl down*/
    btl_cmd_duplicate_trig          = 0x900a,   /*online直接触发副本事件*/
    btl_cmd_end
};

#endif
