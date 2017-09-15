#include "common.h"
#include "global_data.h"
#include "dll_iface.h"
#include "attr.h"
#include "sys_ctrl.h"
#include "map_conf.h"
#include "item_conf.h"
#include "player_conf.h"
#include "duplicate_conf.h"
#include "skill_conf.h"
#include "pet_conf.h"
#include "prize_conf.h"
#include "map_user_manager.h"
#include "item.h"
#include "rune.h"
#include "task_info.h"
#include "tran_card_conf.h"
#include "shop_conf.h"
#include "builder_conf.h"
#include "exchange_conf.h"
#include "arena_conf.h"
#include "swim.h"
#include "trans_prof.h"
#include "home_gift_conf.h"
#include "achieve_conf.h"
#include "achieve.h"
#include "buff_conf.h"
#include "buff.h"
#include "family_conf.h"
#include "duplicate_world_boss.h"
#include "global_attr.h"
#include "exped_conf.h"
#include "cultivate_equip_conf.h"
#include "question_conf.h"
#include "title_conf.h"
#include "time_utils.h"
#include "bless_pet_conf.h"
#include "pet_pass_dup_conf.h"
#include "suit_conf.h"

PlayerManager* g_player_manager;
ProtoProcessor* g_proto_processor;
server_config_t g_server_config;
Service* g_dbproxy;
Service* g_switch;
Service* g_battle;
Service* g_battle_center;



timer_head_t g_reconnect_timer;
timer_head_t g_switch_reg_timer;
timer_head_t g_sys_noti_timer;
timer_head_t g_escort_timer;
timer_head_t g_world_boss_timer;
timer_head_t g_kick_off_timer;
timer_head_t g_reset_dive_timer;
timer_head_t g_dump_rank_timer;

task_conf_mgr_t g_task_conf_mgr;
reward_task_conf_mgr_t g_reward_task_conf_mgr;

std::map<uint32_t, attr_config_t> g_attr_configs;
std::map<uint32_t, global_attr_config_t> g_global_attr_configs;

std::map<uint32_t, condition_conf_t> g_condition_configs;
std::map<uint32_t, condition_fun> g_condition_fun;
std::map<userid_t, player_t*> g_pending_proto_players;
uint32_t g_test_for_robot = config_get_intval("test_for_robot", 0);
const char *encrypt_code = config_get_strval("cli_proto_encrypt_code");
bool cli_proto_encrypt = config_get_intval("cli_proto_encrypt", 0);
uint32_t encrypt_code_len = strlen(encrypt_code);
bool need_active_code = config_get_intval("need_active_code", 0);

std::set<uint32_t> g_user_action_log_config;
module_manager_t g_module_mgr;
map_conf_mgr_t g_map_conf_mgr;
item_conf_mgr_t g_item_conf_mgr;
player_conf_mgr_t g_player_conf_mgr;
player_power_rank_conf_mgr_t g_ply_power_rank_conf_mgr;
joined_test_userid_conf_mgr_t g_joined_test_uid_conf_mgr;
map_user_manager_t g_map_user_mgr;
StatLogger* g_stat_logger;
std::map<uint32_t, attr_stat_func_t> g_attr_stat_func_map;
uint32_t g_cli_pkg_max_size = 4096;
uint32_t g_svr_pkg_max_size = 5000000;
bool g_svr_loginable = false;
duplicate_conf_manager_t g_duplicate_conf_mgr;
pet_conf_manager_t g_pet_conf_mgr;
skill_conf_manager_t g_skill_conf_mgr;
prize_conf_manager_t g_prize_conf_mgr;
rune_conf_mgr_t g_rune_conf_mgr;
rune_exp_conf_mgr_t g_rune_exp_conf_mgr;
rune_rate_conf_mgr_t g_rune_rate_conf_mgr;
tran_card_conf_mgr_t g_tran_card_conf_mgr;
product_manager_t g_product_mgr;
market_manager_t g_market_mgr;
builder_conf_manager_t g_builder_conf_mgr;
exchange_conf_manager_t g_exchg_conf_mgr;
Dive g_dive;
DiveRank g_dive_rank;
arena_streak_reward_conf_mgr_t g_arena_streak_reward_conf_mgr;
arena_rank_reward_mgr_t g_arena_rank_reward_conf_mgr;
rpvp_reward_mgr_t g_rpvp_reward_conf_mgr;

pet_group_manager_t g_pet_group_mgr;
cultivate_equip_conf_manager_t g_cultivate_equip_mgr;

EscortMgr g_escort_mgr;

//float g_effort_grow_point[kMaxBattleValueTypeNum] = { 3, 1, 1, 1, 1};
float g_effort_grow_point[kMaxBattleValueTypeNum] = { 30, 10, 4, 10, 4};
uint32_t g_all_conf_loaded = 0;
trans_prof_conf_manager_t g_trans_prof_conf_manager;

home_gift_conf_mgr_t g_hm_gift_mgr;

std::map< uint32_t, rand_name_pool_t > g_rand_name_pool;
std::map< uint32_t,TIME_CONFIG_LIMIT_T > g_time_config;

ShopRegisterFun g_shop_reg_fun;
AchieveRegisterFunc g_achieve_reg_func;

achieve_mgr_t g_achieve_mgr;

buff_conf_mgr_t g_buff_conf_mgr;
buff_handler_t g_buff_handler;

pet_quality_conf_manager_t g_pet_quality_conf_mgr;
family_conf_manager_t g_family_conf_mgr;
duplicate_world_boss_mgr_t g_world_boss_mgr;
dup_area_prize_conf_manager_t g_dup_area_prize_conf_mgr;
question_conf_manager_t g_question_conf_mgr;
srv_time_manager_t g_srv_time_mgr;
bless_pet_conf_manager_t g_bless_pet_conf_mgr;
suit_conf_manager_t g_suit_conf_mgr;


std::vector<std::string> g_rand_nick_pos1[2]; // 性别区分
std::vector<std::string> g_rand_nick_pos2;
std::vector<std::string> g_rand_nick_pos3;

bool g_cli_msg_locked = false;
bool g_svr_msg_locked = false;

//0点之前多少秒关服
uint32_t g_svr_close_before_zero = config_get_intval("svr_close_before_zero", 0);
//0点之后多少秒开服
uint32_t g_svr_open_after_zero = config_get_intval("svr_open_after_zero", 0);

exped_conf_mgr_t g_exped_mgr;

uint32_t g_server_id = 0;
uint32_t g_online_id = 0;
uint32_t g_cur_max_dup_id = 0;

title_conf_mgr_t  g_title_conf_mgr;
equip_buff_rand_manager_t g_equip_buff_rand_mgr;
std::set<uint32_t> g_client_buff_set;
pet_pass_dup_conf_mgr_t g_pet_pass_dup_conf_mgr;
