#include "common.h"
#include "server_manager.h"
#include "player_manager.h"
#include "proto_processor.h"
#include "player_processor.h"
#include "timer_procs.h"

char g_send_buf[1000000];

static int init_processors();

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
    max_incoming_packets_len = config_get_intval("incoming_packet_max_size", 1000000);

    if (!isparent) {
#ifdef ENABLE_TRACE_LOG
#ifdef USE_TLOG
        SET_LOG_LEVEL(tlog_lvl_trace);
        SET_TIME_SLICE_SECS(86400);
#endif
#endif
        srand(time(NULL));
        srandom(time(NULL));
        SetLogHandler(pb_log_handler);

        INIT_LIST_HEAD(&g_reconnect_timer.timer_list);
        INIT_LIST_HEAD(&g_match_timer.timer_list);

        setup_timer();
        init_processors();
        register_timers();

        //开启匹配定时器
        ADD_TIMER_EVENT_EX(&g_match_timer,
                kTimerTypeBattleMatch,
                0,
                NOW() + kTimerIntervalMatchTimely);

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
    PLAYER_MGR.batch_destroy_players(fd);
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void on_fd_closed(int fd)
{
    //处理服务器对端断开
    server_t *svr = SERVER_MGR.get_server_by_fd(fd);
    if (svr) {
        SERVER_MGR.del_server(svr);
    }
    DEBUG_TLOG("svr fd:%u closed svr=%p", fd, svr);
}

int init_processors()
{
    g_proto_processor->register_command(btl_cmd_enter_duplicate, new BtlEnterCmdProcessor());
    g_proto_processor->register_command(btl_cmd_give_up_match, new BtlQuitMatchCmdProcessor());

    return 0;
}
