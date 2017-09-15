
extern "C" {
#include <libtaomee/log.h>
#include <libtaomee/timer.h>
}

#include "timer_procs.h"
#include "service.h"
#include "dll_iface.h"
#include "client.h"
#include "proto/client/cli_errno.h"

#define REGISTER_TIMER_TYPE(nbr_, cb_) \
    do { \
        if (register_timer_callback(nbr_, cb_) == -1) { \
            ERROR_LOG("register timer type error\t[%u]", nbr_); \
            return -1; \
        }\
    } while(0)

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

int check_db_timeout(void* owner, void* data)
{
    client_info_t* client = (client_info_t *)data;
    client->wait_serv_cmd = 0;
    return send_err_to_client(client, cli_err_svr_time_out);
}

int register_timers()
{
    REGISTER_TIMER_TYPE(kTimerTypeReconnectServiceTimely, reconnect_service_timely);
    return 0;
}
