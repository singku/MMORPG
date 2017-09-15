#include "common.h"
#include "timer_procs.h"

#include "player_manager.h"
#include "server_manager.h"

#define REGISTER_TIMER_TYPE(nbr_, cb_) \
    do { \
        if (register_timer_callback(nbr_, cb_) == -1) { \
            ERROR_LOG("register timer type error\t[%u]", nbr_); \
            return -1; \
        }\
    } while(0)


timer_head_t g_reconnect_timer;
timer_head_t g_match_timer;

int reconnect_service_timely(void* owner, void* data)
{
    return 0;
}

int battle_match_timely(void *owner, void *data)
{
    PLAYER_MGR.do_rpvp_match();
    PLAYER_MGR.do_ppve_match();

    ADD_TIMER_EVENT_EX(&g_match_timer,
            kTimerTypeBattleMatch,
            0,
            NOW() + kTimerIntervalMatchTimely);
    return 0;
}

int register_timers()
{
    REGISTER_TIMER_TYPE(kTimerTypeBattleMatch, battle_match_timely);
    return 0;
}
