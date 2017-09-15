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
    TRACE_TLOG("Waiting Timer Up!"); 
    conn_info_t *conn = (conn_info_t*)data;
    //如果conn已经不存在了
    if (!CONN_MGR.is_conn_in_waiting(conn)) return 0;
    TRACE_TLOG("Waitor [u:%u cmd:%u hex_cmd:0x%04x seq:%d]",
            conn->op_uid, conn->wait_cmd, conn->wait_cmd, conn->wait_uid);
    //否则
    uint32_t uid = conn->op_uid;
    uint32_t u_create_tm = conn->op_u_create_tm;
    uint32_t cmd = conn->wait_cmd;
    send_err_to_conn(conn, cache_err_req_time_up);
    std::set<conn_info_t*> w_set;
    CONN_MGR.del_from_wait_map(uid, u_create_tm, cmd, w_set);
    FOREACH(w_set, it) {
        conn_info_t *w_conn = *it;
        send_err_to_conn(w_conn, cache_err_req_time_up);
    }

    return 0;
}

int register_timers()
{
    REGISTER_TIMER_TYPE(kTimerTypeReconnectServiceTimely, reconnect_service_timely);
    REGISTER_TIMER_TYPE(kTimerTypeWaitingRsp, StopWaiting);

    return 0;
}
