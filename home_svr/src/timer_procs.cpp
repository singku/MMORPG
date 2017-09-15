#include "common.h"
#include "timer_procs.h"
#include "service.h"
#include "player.h"
#include "player_manager.h"

#define REGISTER_TIMER_TYPE(nbr_, cb_) \
    do { \
        if (register_timer_callback(nbr_, cb_) == -1) { \
            ERROR_LOG("register timer type error\t[%u]", nbr_); \
            return -1; \
        }\
    } while(0)


timer_head_t g_reconnect_timer;
timer_head_t g_waiting_rsp_timer;

int reconnect_service_timely(void* owner, void* data)
{
    Service* service = (Service *)data;
    if (service->connect() != 0) {
        ADD_TIMER_EVENT_EX(&g_reconnect_timer,
                kTimerTypeReconnectServiceTimely,
                service,
                get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely);  
    }
    
    return 0;
}

int StopWaiting(void* owner, void* data)
{ 
    //定时器超时
    uint64_t host_key = (uint64_t)data;

TRACE_TLOG("Waiting Timer Up");

    if (PLAYER_MGR.timer_map.count(host_key) != 0) {
        PLAYER_MGR.timer_map.erase(host_key);
    }
    if (PLAYER_MGR.home_info_is_waiting(KEY_ROLE(host_key))) {
        std::set<uint64_t> wait_set;
        PLAYER_MGR.delete_from_home_waiting_list(KEY_ROLE(host_key), wait_set);
        FOREACH(wait_set, it) {
            player_t *dest = PLAYER_MGR.get_player_by_role(KEY_ROLE(*it));
            if (!dest) continue;
            send_err_to_player(dest, dest->wait_cmd_, home_err_cmd_overtime);
            dest->set_wait_host(ROLE(0, 0));
        }
    }
    return 0;
}


int register_timers()
{
    REGISTER_TIMER_TYPE(kTimerTypeReconnectServiceTimely, reconnect_service_timely);
    REGISTER_TIMER_TYPE(kTimerTypeWaitingRsp, StopWaiting);
    return 0;
}
