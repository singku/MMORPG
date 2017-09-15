#ifndef __TIMER_PROCS_H__
#define __TIMER_PROCS_H__

#include "common.h"

enum timer_type_t {
    kTimerTypeReconnectServiceTimely        = 1, // 重连service服务器
    kTimerTypeWaitingRsp                    = 2, // 清理等待时间过长的conn
};

enum timer_interval_t {
    kTimerIntervalReconnectServiceTimely    = 1, // 重连service服务器间隔
    kTimerIntervalWaitingRsp                = 2, // 请求超过2s 则算超时
};

struct timer_head_t {
    list_head_t timer_list;
};

int register_timers();

extern timer_head_t g_reconnect_timer;
extern timer_head_t g_waiting_rsp_timer;

#endif
