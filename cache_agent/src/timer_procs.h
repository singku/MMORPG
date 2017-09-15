#ifndef __TIMER_PROCS_H__
#define __TIMER_PROCS_H__

#include "common.h"

enum timer_type_t {
    kTimerTypeReconnectServiceTimely    = 1, // 重连service服务器
    kTimerTypeWaitingRsp                = 2, // 发出请求后进入超时等待过程
};

enum timer_interval_t {
    kTimerIntervalReconnectServiceTimely = 1, // 重连service服务器间隔
    kTimerIntervalWaitingRsp             = 2, // 请求缓存数据 最多延迟2秒
};

struct timer_head_t {
    list_head_t timer_list;
};

int register_timers();

extern timer_head_t g_reconnect_timer;
extern timer_head_t g_waiting_rsp_timer;

#endif
