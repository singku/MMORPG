#ifndef TIMER_PROCS_H
#define TIMER_PROCS_H

enum timer_type_t {
    kTimerTypeDupCountDownTimer = 1,
    kTimerTypeDupMonFlushTimer = 2,
    kTimerTypeDupPhaseTimer = 3,
    kTimerTypeDupDestroyTimer = 4,
    kTimerTypeBossSHowTimer = 5,
    kTimerTypeForceReadyTimer = 6,
};

enum timer_interval_t {
};

int register_timers();

#endif
