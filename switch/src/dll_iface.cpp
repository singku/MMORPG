#include "common.h"
#include "proto_processor.h"
#include "service.h"
#include "timer_procs.h"
#include "server_manager.h"
#include "switch_proto.h"
#include "switch.h"
#include "timer_procs.h"

static int init_processors();
static int init_connections();
static int init_report_timer();
static void proc_cached_proto();

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
        SET_LOG_LEVEL(tlog_lvl_trace);
        SET_TIME_SLICE_SECS(86400);
#endif
#endif
        srand(time(NULL));
        srandom(time(NULL));
        SetLogHandler(pb_log_handler);
        max_incoming_packets_len = config_get_intval("incoming_packet_max_size", 1000000);
        INIT_LIST_HEAD(&g_reconnect_timer.timer_list);
        INIT_LIST_HEAD(&g_waiting_rsp_timer.timer_list);
        INIT_LIST_HEAD(&g_global_timer.timer_list);

        setup_timer();
        init_processors();
        register_timers();
        
        // 初始化网络连接
        if (init_connections() != 0) {
            ERROR_TLOG("init server connections failed"); 
            return -1;
        }
        init_report_timer();
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
        FOREACH(g_stat_logger_map, it) {
            delete it->second;
        }
        g_stat_logger_map.clear();
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
    proc_cached_proto();
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
    if (fd == g_dbproxy->fd()) {
        g_proto_processor->proc_pkg_from_serv(fd, data, len);
    }
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void on_client_conn_closed(int fd)
{
#ifdef DEBUG
    DEBUG_LOG("on client conn closed");
#endif
    server_t *svr = SERVER_MGR.get_server_by_fd(fd);
    if (svr) {
        SERVER_MGR.del_server(svr);
    }
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
    g_proto_processor->register_command(sw_cmd_register_server, new ServerRegCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_online_sync_player_info, new OnlineSyncPlayerInfoCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_get_server_list, new GetSvrListCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_online_report_player_onoff, new OnlineReportPlayerStateCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_sw_transmit_only, new TransmitMsgCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_sw_is_player_online, new IsPlayerOnlineCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_sw_get_userid_list, new GetUseridListCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_sw_req_erase_player_escort_info, new EraseEscortInfoCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_sw_change_other_attr, new ChangeOtherAttrCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_sw_gm_new_mail, new GmNewMailCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_sw_gm_frozen_account, new FrozenAccountCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_sw_gm_new_mail_to_svr, new GmNewMailToSvrCmdProcessor()); 
	//帐号平台充值相关
    g_proto_processor->register_command(sw_cmd_sw_get_role_info, new ActGetRoleInfoCmdProcessor()); 
    g_proto_processor->register_command(sw_cmd_sw_recharge_diamond, new ActRechargeDiamondCmdProcessor()); 

    g_proto_processor->register_command(sw_cmd_sw_if_role_login_during_tm, new PlatformIfRoleLoginDuringTmCmdProcessor()); 

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

int init_report_timer()
{
    ADD_TIMER_EVENT_EX(&g_global_timer,
            kTimerTypeOnlineReport,
            0,
            get_now_tv()->tv_sec + kTimerInterValReport);  
    return 0;
}

// 处理客户端缓存的协议
void proc_cached_proto()
{
    std::map<int, server_t*>::iterator ptr = 
        g_pending_proto_svrs.begin();

    for (; ptr != g_pending_proto_svrs.end(); ) {
        server_t *svr = ptr->second;
        ptr++;
        server_t *expect_svr = SERVER_MGR.get_server_by_fd(svr->fdsess()->fd);
        if (expect_svr == NULL) {
            WARN_TLOG("cached proto not find svr fd:%d", svr->fdsess()->fd); 
            continue;
        }

        if (expect_svr->fdsess()->fd != svr->fdsess()->fd) {
            WARN_TLOG("cached proto not find svr fd:%d", svr->fdsess()->fd); 
            continue;
        }

        ProtoQueue* proto_queue = svr->proto_queue;

        if (proto_queue->empty()) {
            g_pending_proto_svrs.erase(svr->fdsess()->fd);
            continue;
        }

        if (svr->waiting_cmd()) {
            continue ; 
        }

        ProtoQueue::proto_t proto = {0};
        int ret = proto_queue->pop_proto(proto);

        if (ret != 0) {
            ERROR_TLOG("pop proto from queue failed");
            continue; 
        }

        g_proto_processor->proc_pkg_from_client(
                proto.data, proto.len, proto.fdsession, true);

        free(proto.data);

        if (proto_queue->empty()) {
            g_pending_proto_svrs.erase(svr->fdsess()->fd); 
        }
    }
}
