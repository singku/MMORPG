
#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <map>
extern "C" {
#include <libtaomee/list.h>
}
#include "statlogger/statlogger.h"

struct server_config_t
{
    uint32_t gameid; 
    uint32_t verifyid;
    char security_code[10];
    uint32_t idc_zone;
    char dbproxy_name[256];
    char switch_name[256];
    char statistic_file[256];
    char conf_path[256];
};

struct timer_head_t {      
    list_head_t timer_list;
};

struct rand_nick_item_t
{
    std::string tag; 
};

struct csvr_info_t {
    uint32_t init_svr_id; //初始服务器ID
    uint32_t cur_svr_id; //当前服务器ID
};

class Service;
class StatLogger;

extern server_config_t g_server_config;
extern Service* g_dbproxy;

// 重连定时器
extern timer_head_t g_reconnect_timer;
extern std::vector<std::string> rand_nick_pos1[2]; // 性别区分
extern std::vector<std::string> rand_nick_pos2;
extern std::vector<std::string> rand_nick_pos3;
extern std::map<uint32_t, csvr_info_t> g_servers_map; //当前有哪些服
extern std::map<uint32_t, StatLogger*> g_stat_logger_map; //每个服一个统计对象
extern uint32_t incoming_packet_max_size;
extern uint32_t g_need_active_code;

StatLogger *get_stat_logger(uint32_t svr_id);

#endif
