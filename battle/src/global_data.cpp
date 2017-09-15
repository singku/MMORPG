#include "common.h"
#include "duplicate_conf.h"
#include "skill_conf.h"
#include "pet_conf.h"
#include "affix_conf.h"
#include "builder_conf.h"
#include "player_manager.h"
#include "duplicate_entity.h"
#include "map_conf.h"
#include "builder_conf.h"
#include "dll_iface.h"
#include "global_data.h"

player_manager_t* g_player_manager;
ProtoProcessor* g_proto_processor;
server_config_t g_server_config;
timer_head_t g_reconnect_timer;

uint32_t g_svr_pkg_max_size = 5000000;
duplicate_conf_manager_t g_duplicate_conf_mgr;

skill_conf_manager_t g_skill_conf_mgr;
pet_conf_manager_t g_pet_conf_mgr;
builder_conf_manager_t g_builder_conf_mgr;
map_conf_manager_t g_map_conf_mgr;
affix_conf_manager_t g_affix_conf_mgr;

dup_entity_mgr_t *g_dup_entity_mgr;

uint32_t g_load_conf_cnt;
uint32_t g_born_area_idx = 0;
