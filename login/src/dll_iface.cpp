#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <google/protobuf/stubs/common.h>
#include <async_serv/service.h>

extern "C" {
#include <libtaomee/log.h>
#include <libtaomee/conf_parser/config.h>
#include <libtaomee/timer.h>
#include <libtaomee/tlog/tlog_macros.h>
#include <libtaomee/tm_dirty/tm_dirty.h>
}
#include <libtaomee++/utils/strings.hpp>

#include "proto_processor.h"
#include "proto/client/cli_cmd.h"
#include "dll_iface.h"
#include "timer_procs.h"
#include "service.h"
#include "client.h"
#include "login_cmd_processor.h"
#include "xmlutils.h"

// 全局变量
server_config_t g_server_config;
Service* g_dbproxy;
timer_head_t g_reconnect_timer;
ProtoProcessor* g_proto_processor;
// 随机昵称的三个位置，位置拼起来就是随机昵称
std::vector<std::string> rand_nick_pos1[2]; // 性别区分
std::vector<std::string> rand_nick_pos2;
std::vector<std::string> rand_nick_pos3;
std::map<uint32_t, csvr_info_t> g_servers_map; //当前有哪些服
std::map<uint32_t, StatLogger*> g_stat_logger_map; //每个服一个统计对象

uint32_t incoming_packet_max_size = 8192;
uint32_t g_need_active_code = config_get_intval("need_active_code", 0);
const char *encrypt_code = config_get_strval("cli_proto_encrypt_code");
bool cli_proto_encrypt = config_get_intval("cli_proto_encrypt", 0);
uint32_t encrypt_code_len = strlen(encrypt_code);

#define CONFIG_READ_INTVAL(data, name) \
    do { \
        int ret = -1; \
        ret = config_get_intval(#name, ret); \
        if (ret == -1) { \
            ERROR_LOG("not find config '%s'", #name); \
            return -1; \
        } \
        data.name = ret; \
        DEBUG_LOG("set config '%s' = %d", \
#name, data.name); \
    } while (0);

#define CONFIG_READ_STRVAL(data, name) \
    do { \
        const char *conf_str = NULL; \
        conf_str = config_get_strval(#name); \
        if (conf_str == NULL) { \
            ERROR_LOG("not find config '%s'", #name); \
            return -1; \
        } \
        strncpy(data.name, conf_str, sizeof(data.name) - 1); \
        DEBUG_LOG("set config '%s' = '%s'",  \
#name, data.name); \
    } while (0);

static int init_processors();
static int init_connections();
static int load_nick_config(xmlNodePtr root);
static int load_server_config(xmlNodePtr root);

void proc_cached_proto();

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
        incoming_packet_max_size = config_get_intval("incoming_packet_max_size", 1000000);

        INIT_LIST_HEAD(&g_reconnect_timer.timer_list);
        setup_timer();
        init_processors();
        register_timers();

        memset(&g_server_config, 0, sizeof(g_server_config));

        CONFIG_READ_STRVAL(g_server_config, dbproxy_name);
        CONFIG_READ_STRVAL(g_server_config, security_code);
        CONFIG_READ_INTVAL(g_server_config, idc_zone);
        CONFIG_READ_INTVAL(g_server_config, gameid);
        CONFIG_READ_INTVAL(g_server_config, verifyid);
        CONFIG_READ_STRVAL(g_server_config, statistic_file);
        CONFIG_READ_STRVAL(g_server_config, conf_path);

        // 初始化网络连接
        if (init_connections() != 0) {
            KERROR_LOG(0, "init server connections failed"); 
            return -1;
        }

        std::string full_path = std::string(g_server_config.conf_path) + "/nick.xml";
        if (load_xmlconf(full_path.c_str(), load_nick_config)) {
            KERROR_LOG(0, "load nick config failed"); 
            return -1;
        }

        full_path = std::string(g_server_config.conf_path) + "/server.xml";
        if (load_xmlconf(full_path.c_str(), load_server_config)) {
            KERROR_LOG(0, "load server config failed"); 
            return -1;
        }

        char str[] = "你好, 大坏蛋, fuck, 中国,  靠你大爷的 你怎么了?";
        if (tm_dirty_check(0, str)) {
            tm_dirty_replace(str);
            DEBUG_TLOG("DIRTY_TEST:[%s]", str);
        }
        DEBUG_TLOG("Encrypt Code len:%u code:[%s]", encrypt_code_len, encrypt_code);
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
        typeof(g_stat_logger_map.begin()) it;
        for (it = g_stat_logger_map.begin(); it != g_stat_logger_map.end(); it++) {
            delete it->second;
        }
        g_stat_logger_map.clear();
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
    g_proto_processor->proc_pkg_from_serv(fd, data, len);
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void on_client_conn_closed(int fd)
{
    client_info_t* client = get_client(fd);

    if (client) {
        TRACE_TLOG("on client userid %u conn closed", client->userid);
        destroy_client(fd);
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
            DEBUG_LOG("add timer");
            ADD_TIMER_EVENT_EX(&g_reconnect_timer,
                    kTimerTypeReconnectServiceTimely,
                    g_dbproxy,
                    get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely);  
        }
    } else {
        KERROR_LOG(0, "unknown server fd %d closed", fd); 
    }
}

void proc_cached_proto()
{   
    std::map<int, client_info_t*>::iterator ptr = 
        g_pending_proto_clients.begin();

    for (; ptr != g_pending_proto_clients.end(); ptr++) {

        client_info_t* client = ptr->second;
        ProtoQueue* proto_queue = client->proto_queue;

        if (proto_queue->empty()) {
            g_pending_proto_clients.erase(client->fd);
            continue;
        }

        if (client->wait_serv_cmd) {
            continue ; 
        }

        ProtoQueue::proto_t proto = {0};
        int ret = proto_queue->pop_proto(proto);
        if (ret) {
            KERROR_LOG(client->userid, "pop proto from queue failed");
            continue; 
        }
       
        FATAL_TLOG("proc_cached_proto:%u %u",client->userid, client->cli_cmd);
        g_proto_processor->proc_pkg_from_client(
            proto.data, proto.len, client->fdsession, true);


        if (proto_queue->empty()) {
            g_pending_proto_clients.erase(client->fd); 
        }
    }
}





int init_processors()
{
    g_proto_processor->register_cmd(cli_cmd_cs_0x0001_login, new LoginCmdProcessor());
    g_proto_processor->register_cmd(cli_cmd_cs_0x0002_require_random_nick, new RandomNickCmdProcessor());
    g_proto_processor->register_cmd(cli_cmd_cs_0x0003_create_role, new CreateRoleCmdProcessor());
    g_proto_processor->register_cmd(cli_cmd_cs_0x0004_get_svr_list, new GetSvrListCmdProcessor());
    g_proto_processor->register_cmd(cli_cmd_cs_0x0005_get_verify_image, new GetVerifyImageCmdProcessor());
    g_proto_processor->register_cmd(cli_cmd_cs_0x0006_session_login, new SessionLoginCmdProcessor());
    g_proto_processor->register_cmd(cli_cmd_cs_0x0007_active_user, new ActiveUserCmdProcessor());
    g_proto_processor->register_cmd(cli_cmd_cs_0x0009_cli_stat, new StatCmdProcessor());
    g_proto_processor->register_cmd(cli_cmd_cs_0x000B_cli_choose_server, new ChooseServerCmdProcessor());

    return 0;
}

int init_connections()
{
    // 初始化dbproxy
    g_dbproxy = new Service(std::string(g_server_config.dbproxy_name));

    ADD_TIMER_EVENT_EX(&g_reconnect_timer, 
            kTimerTypeReconnectServiceTimely, 
            g_dbproxy,
            get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely); 

    return 0;
}

int load_server_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar*)"server")) {
            cur = cur->next;
            continue;
        }
        uint32_t init_svr_id = 0;
        uint32_t cur_svr_id = 0;
        DECODE_XML_PROP_UINT32(init_svr_id, cur, "init_svr_id");
        DECODE_XML_PROP_UINT32(cur_svr_id, cur, "cur_svr_id");
        csvr_info_t inf;
        inf.init_svr_id = init_svr_id;
        inf.cur_svr_id = cur_svr_id;
        g_servers_map[init_svr_id] =  inf;
		TRACE_TLOG("Load_server_config,init_svr_id=%u,cur_svr_id=%u", init_svr_id, cur_svr_id);
		DEBUG_TLOG("Load_server_config,init_svr_id=%u,cur_svr_id=%u", init_svr_id, cur_svr_id);
        cur = cur->next;
    }
    return 0;
}

int load_nick_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;

    while (cur) {

        if (!xmlStrcmp(cur->name, (const xmlChar *)"item")) {

            char tag[64];
            uint32_t pos = 0;
            uint32_t sex = 0;
            DECODE_XML_PROP_STR(tag, cur, "tag");
            DECODE_XML_PROP_UINT32(pos, cur, "pos");

            if (pos == 1) {
                DECODE_XML_PROP_UINT32(sex, cur, "sex");
                if (sex == 1) {
                    rand_nick_pos1[0].push_back(tag); 
                } else if (sex == 2) {
                    rand_nick_pos1[1].push_back(tag); 
                } else if (sex == 0) {
                    rand_nick_pos1[0].push_back(tag); 
                    rand_nick_pos1[1].push_back(tag); 
                }
            } else if (pos == 2) {
                rand_nick_pos2.push_back(tag); 
            } else {
                rand_nick_pos3.push_back(tag); 
            }

            TRACE_TLOG("tag = '%s', pos = %d, sex = %d", tag, pos, sex);
        }
    
        cur = cur->next;
    }

    return 0;
}

StatLogger *get_stat_logger(uint32_t svr_id)
{
    static StatLogger default_logger;
    static bool default_logger_init = false;
    if (unlikely(default_logger_init == false)) {
        default_logger.init(g_server_config.gameid);
        default_logger_init = true;
    }

    if (svr_id == 0) {
        return &default_logger;
    }
    if (g_stat_logger_map.count(svr_id) == 0) {
        StatLogger *stat_logger = new StatLogger(g_server_config.gameid, -1, svr_id);
        if (!stat_logger) {
            return &default_logger;
        }
        g_stat_logger_map[svr_id] = stat_logger;
        return stat_logger;
    }
    return g_stat_logger_map.find(svr_id)->second;
}
