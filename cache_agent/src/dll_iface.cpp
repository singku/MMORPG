#include "common.h"
#include "proto_processor.h"
#include "service.h"
#include "conn_manager.h"
#include "timer_procs.h"
#include "cache_processor.h"

static int init_processors();
static int init_connections();

void pb_log_handler(google::protobuf::LogLevel level,
        const char *filename, int line, const std::string &message)
{
    static const char *level_names[] = {"INFO", "WARNING", "ERROR", "FATAL" };
    ERROR_TLOG("[%s %s:%d] %s",
            level_names[level], filename, line, message.c_str());
    DEBUG_TLOG("[%s %s:%d] %s",
            level_names[level], filename, line, message.c_str());
}

extern "C" int  init_service(int isparent)
{
    g_proto_processor = new ProtoProcessor();

	if (!isparent) {
#ifdef ENABLE_TRACE_LOG
#ifdef USE_TLOG
        SET_LOG_LEVEL((tlog_lvl_t)/*tlog_lvl_trace*/config_get_intval("log_level", 6));
        SET_TIME_SLICE_SECS(86400);
#endif
#endif
        srand(time(NULL));
        srandom(time(NULL));

        SetLogHandler(pb_log_handler);

        INIT_LIST_HEAD(&g_reconnect_timer.timer_list);
        INIT_LIST_HEAD(&g_waiting_rsp_timer.timer_list);

        setup_timer();
        init_processors();
        register_timers();
        
        // 初始化网络连接
        if (init_connections() != 0) {
            ERROR_TLOG("init server connections failed"); 
            return -1;
        }
	}

	return 0;
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" int  fini_service(int isparent)
{
    delete g_proto_processor;

	if (!isparent) {
        delete g_dbproxy;
        destroy_timer();
	}

	return 0;
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void proc_events()
{
    handle_timer();
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" int get_pkg_len(int fd, const void* avail_data, int avail_len, int isparent)
{
    if (isparent) {
        return g_proto_processor->get_pkg_len(fd, avail_data, avail_len, PROTO_FROM_CLIENT); 
    } else {
        return g_proto_processor->get_pkg_len(fd, avail_data, avail_len, PROTO_FROM_SERV); 
    }
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" int  proc_pkg_from_client(void* data, int len, fdsession_t* fdsess)
{
	//bin2hex(hex_buf,(char*)data,len,500);
	//DEBUG_LOG("I[%s]", hex_buf );

    return g_proto_processor->proc_pkg_from_client(data, len, fdsess);
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void proc_pkg_from_serv(int fd, void* data, int len)
{
    g_proto_processor->proc_pkg_from_serv(fd, data, len);
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void on_client_conn_closed(int fd)
{
    CONN_MGR.del_conn_when_fd_closed(fd);
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void on_fd_closed(int fd)
{
    if (fd == g_dbproxy->fd()) {
        DEBUG_LOG("dbproxy closed");
        g_dbproxy->close(); 
        if (g_dbproxy->connect() != 0) {
            DEBUG_TLOG("add proxy conn timer");
            ADD_TIMER_EVENT_EX(&g_reconnect_timer,
                    kTimerTypeReconnectServiceTimely,
                    g_dbproxy,
                    get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely);  
        }
    }
}

int init_processors()
{
    g_proto_processor->register_command(cache_cmd_ol_req_users_info, new GetPlayerBaseInfoCmdProcessor()); 
    g_proto_processor->register_command(cache_cmd_ag_req_users_info, new GetPlayerBaseCacheCmdProcessor()); 

    return 0;
}

int init_connections()
{
    // 初始化dbproxy
    char *dbproxy_name = config_get_strval("dbproxy_name");
    if (!dbproxy_name) {
        ERROR_TLOG("NO DBPROXY CONFIG FOUND IN BENCH.CONF");
        return -1;
    }
    string tmp = string(dbproxy_name);
    g_dbproxy = new Service(tmp);
    ADD_TIMER_EVENT_EX(&g_reconnect_timer, 
            kTimerTypeReconnectServiceTimely, 
            g_dbproxy,
            get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely); 
    return 0;
}
