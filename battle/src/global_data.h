#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include <map>
#include <stdint.h>
#include <vector>
#include <set>
#include <string>

#include "common.h"

class player_manager_t;
class ProtoProcessor;
struct server_config_t;
class Service;
struct player_t;
class duplicate_conf_manager_t;
class skill_conf_manager_t;
class pet_conf_manager_t;
class map_conf_manager_t;
class dup_entity_mgr_t;
class builder_conf_manager_t;
class affix_conf_manager_t;

struct timer_head_t {      
    list_head_t timer_list;
};

// 玩家管理器
extern player_manager_t* g_player_manager;
// 命令处理器
extern ProtoProcessor* g_proto_processor;
// online服务相关配置 
extern server_config_t g_server_config;
// 重连定时器
extern timer_head_t g_reconnect_timer;
extern uint32_t g_svr_pkg_max_size;
extern duplicate_conf_manager_t g_duplicate_conf_mgr;
extern skill_conf_manager_t g_skill_conf_mgr;
extern pet_conf_manager_t g_pet_conf_mgr;
extern affix_conf_manager_t g_affix_conf_mgr;
extern builder_conf_manager_t g_builder_conf_mgr;
extern map_conf_manager_t g_map_conf_mgr;
extern dup_entity_mgr_t *g_dup_entity_mgr;
extern uint32_t g_load_conf_cnt;
extern uint32_t g_born_area_idx;

#endif
