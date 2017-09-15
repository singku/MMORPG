#ifndef __TIMER_PROCS_H__
#define __TIMER_PROCS_H__

#include "common.h"

enum timer_type_t {
    kTimerTypeReconnectServiceTimely = 1, // 重连service服务器
    kTimerTypeWaitingRsp             = 2, // 等待其他人拉取小屋信息不超过的时间
};

enum timer_interval_t {
    kTimerIntervalReconnectServiceTimely = 1, // 重连service服务器间隔
    kTimerIntervalWaitingRsp             = 2, // 拉取时间不超过2S
};

struct timer_head_t {
    list_head_t timer_list;
};

int register_timers();

extern timer_head_t g_reconnect_timer;
extern timer_head_t g_waiting_rsp_timer;

#endif
