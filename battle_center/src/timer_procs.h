#ifndef __TIMER_PROCS_H__
#define __TIMER_PROCS_H__

#include "common.h"

enum timer_type_t {
    kTimerTypeReconnectServiceTimely = 1, // 重连service服务器
    kTimerTypeBattleMatch = 2, //战斗匹配定时器
};

enum timer_interval_t {
    kTimerIntervalReconnectServiceTimely = 1, // 重连service服务器间隔
    kTimerIntervalMatchTimely = 1, //每隔1秒钟尝试匹配一次
};

struct timer_head_t {
    list_head_t timer_list;
};

int register_timers();

extern timer_head_t g_reconnect_timer;
extern timer_head_t g_match_timer;

#endif
