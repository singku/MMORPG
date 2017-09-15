#include "common.h"
#include "timer_procs.h"
#include "service.h"
#include "switch.h"
#include "server_manager.h"
#include "player_manager.h"
#include "statlogger/statlogger.h"

#define REGISTER_TIMER_TYPE(nbr_, cb_) \
    do { \
        if (register_timer_callback(nbr_, cb_) == -1) { \
            ERROR_LOG("register timer type error\t[%u]", nbr_); \
            return -1; \
        }\
    } while(0)


timer_head_t g_reconnect_timer;
timer_head_t g_waiting_rsp_timer;
timer_head_t g_global_timer;

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
    uint64_t tmp = (uint64_t)data;
    uint32_t svr_fd = (uint32_t)tmp;

TRACE_TLOG("Waiting Timer Up");

    server_t *svr = SERVER_MGR.get_server_by_fd(svr_fd);
    if (svr) {
        ERROR_TLOG("Svr %u wait svr return time up[cli_cmd:%u cli_hex_cmd:0x%04X, serv_cmd:%u, serv_hex_cmd:0x%04X]",
                svr->server_id(), svr->waiting_cmd(), svr->waiting_cmd(), 
                svr->waiting_serv_cmd(), svr->waiting_serv_cmd());
        send_err_to_server(svr, svr->waiting_cmd(), sw_err_proc_time_out);
        svr->clear_waiting_cmd();
        svr->clear_waiting_serv_cmd();
    }
    return 0;
}

int report_stat_online(void *owner, void *data)
{

    get_stat_logger(0)->online_count(PLAYER_MGR.get_total_player_num(0));
    get_stat_logger(0)->online_count(PLAYER_MGR.get_total_net_player_num(0), "网通");
    get_stat_logger(0)->online_count(PLAYER_MGR.get_total_tel_player_num(0), "电信");

    FOREACH(SERVER_MGR.all_svrid_set(), it) {
        get_stat_logger(*it)->online_count(PLAYER_MGR.get_total_player_num(*it));
        get_stat_logger(*it)->online_count(PLAYER_MGR.get_total_net_player_num(*it), "网通");
        get_stat_logger(*it)->online_count(PLAYER_MGR.get_total_tel_player_num(*it), "电信");
    }

    ADD_TIMER_EVENT_EX(&g_global_timer,
            kTimerTypeOnlineReport,
            0,
            get_now_tv()->tv_sec + kTimerInterValReport);  

    return 0;
}


int register_timers()
{
    REGISTER_TIMER_TYPE(kTimerTypeReconnectServiceTimely, reconnect_service_timely);
    REGISTER_TIMER_TYPE(kTimerTypeWaitingRsp, StopWaiting);
    REGISTER_TIMER_TYPE(kTimerTypeOnlineReport, report_stat_online);

    return 0;
}
