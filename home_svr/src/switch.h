#ifndef __SWITCH_H__
#define __SWITCH_H__

#include "common.h"

/* @brief 处理switch返回的包，分发到相应的处理函数中
 * @param data 返回包
 * @param len switch返回包的长度 
*/
void handle_switch_notify(void* data, uint32_t len);

//发消息的时候发现玩家离线
void proc_player_offline(uint32_t uid, uint32_t u_create_tm);

//跨服发消息
int sw_send_msg_to_player(uint32_t uid, uint32_t create_tm,
        uint16_t cmd, const google::protobuf::Message *msg);

#endif
