#include <execinfo.h>
#include "common.h"
#include "proto.h"
#include "proto_processor.h"
#include "dll_iface.h"
#include "global_data.h"
#include "player.h"
#include "timer_procs.h"
#include "xmlutils.h"
#include "xml_configs.h"
#include "player_manager.h"
#include "duplicate_conf.h"
#include "duplicate_processor.h"
#include "mcast_utils.h"

#define CONFIG_READ_INTVAL(data, name) \
    do { \
        int ret = -1; \
        ret = config_get_intval(#name, ret); \
        if (ret == -1) { \
            ERROR_LOG("not find config '%s'", #name); \
            return -1; \
        } \
        data.name = ret; \
    } while (0);

#define CONFIG_READ_STRVAL(data, name) \
    do { \
        const char *conf_str = NULL; \
        conf_str = config_get_strval(#name); \
        if (conf_str == NULL) { \
            ERROR_LOG("not find config '%s'", #name); \
            return -1; \
        } \
        STRCPY_SAFE(data.name, conf_str); \
    } while (0);

static int init_processors();
static bool load_configs();

std::string stack_trace()
{
    // 打印堆栈信息
    void* buffs[100];
    int num_ptrs;
    num_ptrs = backtrace(buffs, array_elem_num(buffs));

    char** strings;
    strings = backtrace_symbols(buffs, num_ptrs);

    std::string stack;
    if (strings) {
        for (int i = 1; i < num_ptrs; i++) {
            stack += std::string(strings[i]) + "\n"; 
        }
        free(strings);
    }

    return stack;
}

void pb_log_handler(google::protobuf::LogLevel level,
        const char *filename, int line, const std::string &message)
{
    static const char *level_names[] = {"INFO", "WARNING", "ERROR", "FATAL" };

    std::string stack = stack_trace();

    ERROR_TLOG("[%s %s:%d] %s\n stack: '%s'",
            level_names[level], filename, line, message.c_str(),
            stack.c_str());
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
        srand(NOW());
        srandom(NOW());

        SetLogHandler(pb_log_handler);

        g_player_manager = new player_manager_t();
        g_dup_entity_mgr = new dup_entity_mgr_t();
        setup_timer();
        init_processors();
        register_timers();
        memset(&g_server_config, 0, sizeof(g_server_config));

        CONFIG_READ_STRVAL(g_server_config, conf_path);

        if (!load_configs()) {
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
        delete g_dup_entity_mgr;
        g_duplicate_conf_mgr.clear();
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
    return g_proto_processor->proc_pkg_from_client(data, len, fdsess, false);
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
    g_player_manager->batch_del_players(fd);
}

extern "C" int before_reload(int isparent)
{
    google::protobuf::ShutdownProtobufLibrary();
    delete g_proto_processor;
    g_proto_processor = 0;
    unregister_timers_callback();

    return 0;
}

extern "C" int reload_global_data()
{
    unregister_timers_callback();
    g_proto_processor = new ProtoProcessor();
    init_processors();
    register_timers();
    SetLogHandler(pb_log_handler);

    refresh_timers_callback();
    return 0;
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void proc_mcast_pkg(const void* data, int len)
{
    const comm_proto_header_t* header = static_cast<const comm_proto_header_t *>(data);
    const char* body = static_cast<const char *>(data) + sizeof(comm_proto_header_t);
    int bodylen = header->len - sizeof(comm_proto_header_t);

    if (header->cmd == 0) {
        svrcommproto::mcast_reload_conf mcast_reload_conf;
        if (parse_message(body, bodylen, &mcast_reload_conf) != 0) {
            ERROR_TLOG("decode mcast_proto  failed");
            return ;
        }
        std::string conf_name = mcast_reload_conf.conf_name();
        uint32_t server_id = mcast_reload_conf.serverid();
        if (server_id != 0 && server_id != get_server_id()) {
            return;
        }

        INFO_TLOG("reload conf %s", conf_name.c_str());
        INFO_TLOG("reload server_id %d", server_id);

        if (conf_name.compare("all") == 0) {
            bool flag = load_configs();
            if (!flag) {
                ERROR_TLOG("reload conf %s failed", conf_name.c_str());
            } else {
                INFO_TLOG("reload conf %s success!", conf_name.c_str());
            }
            return;
        }

        int ret = McastUtils::reload_configs(conf_name);
        if (ret != 0) {
            ERROR_TLOG("reload conf %s failed", conf_name.c_str());
        } else {
            INFO_TLOG("reload conf %s success!", conf_name.c_str());
        }
    }
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void on_fd_closed(int fd)
{
}

int init_processors()
{
    g_proto_processor->register_command(btl_cmd_msg_relay, new DuplicateRelayCmdProcessor());
    g_proto_processor->register_command(btl_cmd_enter_duplicate, new EnterDuplicateCmdProcessor());
    g_proto_processor->register_command(btl_cmd_revival, new RevivalDuplicateCmdProcessor());
    g_proto_processor->register_command(btl_cmd_duplicate_trig, new DuplicateTrigCmdProcessor());

    return 0;
}

const char *gen_full_path(const char *base_path, const char *file_name)
{
    static char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, file_name);
    return full_path;
}

bool load_configs()
{
    bool succ = (1
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "map.xml"), load_map_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "skill_parent.xml"), load_skill_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "affix.xml"), load_affix_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "pet.xml"), load_pet_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "builder.xml"), load_builder_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "duplicate.xml"), load_duplicate_config) == 0
            );
    g_load_conf_cnt++;
    return succ;
}
