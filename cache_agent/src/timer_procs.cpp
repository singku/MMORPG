#include "common.h"
#include "timer_procs.h"
#include "service.h"
#include "conn_manager.h"

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
    uint64_t tmp = (uint64_t)data;
    uint32_t uid, u_create_tm;
    decomp_u64(tmp, uid, u_create_tm);

TRACE_TLOG("Waiting Timer Up");

    conn_info_t *conn = CONN_MGR.del_from_wait_map(uid, u_create_tm);
    if (!conn) return 0;
TRACE_TLOG("Waitor: [u:%u cmd:%u hex_cmd:0x%04X seq:%u]", 
        conn->uid, conn->wait_cmd, conn->wait_cmd, conn->cur_seq);

    finish_conn_msg(conn);
    return 0;
}

int register_timers()
{
    REGISTER_TIMER_TYPE(kTimerTypeReconnectServiceTimely, reconnect_service_timely);
    REGISTER_TIMER_TYPE(kTimerTypeWaitingRsp, StopWaiting);

    return 0;
}
