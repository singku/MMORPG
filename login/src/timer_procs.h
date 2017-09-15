
#ifndef TIMER_PROCS_H
#define TIMER_PROCS_H

enum timer_type_t
{
    kTimerTypeReconnectServiceTimely = 1, // 重连service服务器
    kTimerTypeCheckDbTimeout = 2, // db超时
};

enum timer_interval_t
{
    kTimerIntervalReconnectServiceTimely = 1, // 重连service服务器间隔
    kTimerIntervalCheckDbTimeout = 3, // db超时通知
};

int register_timers();

#endif
