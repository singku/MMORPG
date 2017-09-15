#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include <map>
#include <stdint.h>
#include <vector>
#include <set>
#include <string>
#include "escort.h"
#include "utils.h"
#include "task_info.h"
#include "duplicate_world_boss.h"

extern "C" {
#include <libtaomee/list.h>
#include <libtaomee/project/types.h>
}

const uint32_t kMaxRuneExpType = 5;
const uint32_t kMaxRuneExpLevel = 5;
const uint32_t KMaxOffLineTime = 5 * 3600; // 最大离线时间，超过离线时间，清理防沉迷信息
const uint32_t kOnlineTimeThresholdone = 1 * 3600;//1小时提醒
const uint32_t kOnlineTimeThresholdHalf = 3 * 3600;//收益减半
const uint32_t kOnlineTimeThresholdNone = 5 * 3600;//没有收益

enum global_const_t {
    kMaxPetNum = 1000,
    kMaxMsgLength = 1900,//私聊信息的最大长度
    kMaxNameLength = 32,//姓名的最大长度
    kMinVerifyImageRate = 5,
    kDefaultVerifyImageRate = 20,
    kMaxVerifyImageRate = 50,
	kMaxTaskNum = 999999,
	// kOnlineTimeThresholdHalf = 3 * 3600,
    kMaxProtoQueueSize = 50,
    kCommandGetVersion = 6000,//平台获取版本号命令
};

const char kVerifyImageKey[50] = "UH.]4MjK:TqO'_Jb=;,x}";

const uint32_t kVowBottleCost[5]={1,2,5,10,15};

class PlayerManager;
class ProtoProcessor;
struct server_config_t;
class Service;
struct attr_config_t;
struct global_attr_config_t;
struct task_conf_t;
struct condition_conf_t;
struct player_t;
struct tower_conf_t;
struct map_conf_mgr_t;
struct map_user_manager_t;
class duplicate_conf_manager_t;
class pet_conf_manager_t;
class skill_conf_manager_t;
struct talent_improve_conf_t;
class prize_conf_manager_t;
class item_conf_mgr_t;
class task_conf_mgr_t;
class reward_task_conf_mgr_t;
class rune_conf_mgr_t;
class rune_exp_conf_mgr_t;
class rune_rate_conf_mgr_t;
class tran_card_conf_mgr_t;
class product_manager_t;
class market_manager_t;
class builder_conf_manager_t;
class exchange_conf_manager_t;
class Dive;
class DiveRank;
class arena_streak_reward_conf_mgr_t;
class arena_rank_reward_mgr_t;
class buff_conf_mgr_t;
class buff_handler_t;
class pet_quality_conf_manager_t;
class family_conf_manager_t;
class pet_group_manager_t;
class cultivate_equip_conf_manager_t;
class dup_area_prize_conf_manager_t;
class question_conf_manager_t ;
class srv_time_manager_t;
class bless_pet_conf_manager_t;
class equip_buff_rand_manager_t;
class pet_pass_dup_conf_mgr_t;
class suit_conf_manager_t;

struct timer_head_t {      
    list_head_t timer_list;
};
class module_manager_t;
class StatLogger;
class trans_prof_conf_manager_t;
class home_gift_conf_mgr_t;

class ShopRegisterFun;
class AchieveRegisterFunc;

class achieve_mgr_t;
//class EscortMgr;
class exped_conf_mgr_t;

class title_conf_mgr_t;
class rpvp_reward_mgr_t;

// 玩家管理器
extern PlayerManager* g_player_manager;
// 命令处理器
extern ProtoProcessor* g_proto_processor;
// online服务相关配置 
extern server_config_t g_server_config;
// dbproxy服务
extern Service* g_dbproxy;
// switch服务
extern Service* g_switch;
// battle服务
extern Service* g_battle;
extern Service* g_battle_center;

// 重连定时器
extern timer_head_t g_reconnect_timer;
// switch注册超时定时器
extern timer_head_t g_switch_reg_timer;
// 系统通知定时器
extern timer_head_t g_sys_noti_timer;
//运宝定时器
extern timer_head_t g_escort_timer;
// 世界boss定时器
extern timer_head_t g_world_boss_timer;
// 每日定时T人定时器
extern timer_head_t g_kick_off_timer;
// 每日0点定时清理跳水排行榜
extern timer_head_t g_reset_dive_timer;
// 每日21点定时dump某些排名服
extern timer_head_t g_dump_rank_timer;

extern task_conf_mgr_t g_task_conf_mgr;
extern reward_task_conf_mgr_t g_reward_task_conf_mgr;
//extern std::map<uint32_t, task_conf_t> g_task_configs;
extern std::map<uint32_t, attr_config_t> g_attr_configs;

extern std::map<uint32_t, global_attr_config_t> g_global_attr_configs;
// 有缓存客户端协议的用户
extern std::map<userid_t, player_t*> g_pending_proto_players;
extern uint32_t g_test_for_robot;
//user-action-log配置
extern std::set<uint32_t> g_user_action_log_config;
extern module_manager_t g_module_mgr;
extern map_conf_mgr_t g_map_conf_mgr;
extern map_user_manager_t g_map_user_mgr;
//跳水信息
extern Dive g_dive;
extern DiveRank g_dive_rank;


extern std::map<uint32_t, condition_conf_t> g_condition_configs;

// 统计项对象
extern StatLogger* g_stat_logger;

//属性值统计函数表
//属性值的统计项函数
typedef void (*attr_stat_func_t)(player_t *player, uint32_t from, uint32_t to);
extern std::map<uint32_t, attr_stat_func_t> g_attr_stat_func_map;

extern uint32_t g_cli_pkg_max_size;
extern uint32_t g_svr_pkg_max_size;
extern bool g_svr_loginable;
extern duplicate_conf_manager_t g_duplicate_conf_mgr;
extern pet_conf_manager_t g_pet_conf_mgr;
extern skill_conf_manager_t g_skill_conf_mgr;
extern prize_conf_manager_t g_prize_conf_mgr;
extern item_conf_mgr_t g_item_conf_mgr;
extern float g_effort_grow_point[kMaxBattleValueTypeNum];
extern rune_conf_mgr_t g_rune_conf_mgr;
extern rune_exp_conf_mgr_t g_rune_exp_conf_mgr;
extern rune_rate_conf_mgr_t g_rune_rate_conf_mgr;
extern tran_card_conf_mgr_t g_tran_card_conf_mgr;
extern product_manager_t g_product_mgr;
extern market_manager_t g_market_mgr;
extern builder_conf_manager_t g_builder_conf_mgr;
extern exchange_conf_manager_t g_exchg_conf_mgr;
extern arena_streak_reward_conf_mgr_t g_arena_streak_reward_conf_mgr;
extern arena_rank_reward_mgr_t g_arena_rank_reward_conf_mgr;
extern rpvp_reward_mgr_t g_rpvp_reward_conf_mgr;

extern std::map< uint32_t, rand_name_pool_t > g_rand_name_pool;
typedef std::map<uint32_t, time_limit_t> TIME_CONFIG_LIMIT_T;
extern std::map< uint32_t,TIME_CONFIG_LIMIT_T > g_time_config;
extern pet_group_manager_t g_pet_group_mgr;

typedef bool (*condition_fun)(player_t*, condition_conf_t, uint32_t); 
extern std::map<uint32_t, condition_fun> g_condition_fun;

extern uint32_t g_all_conf_loaded;

extern EscortMgr g_escort_mgr;
extern trans_prof_conf_manager_t g_trans_prof_conf_manager;

extern home_gift_conf_mgr_t g_hm_gift_mgr;
extern ShopRegisterFun g_shop_reg_fun;
extern AchieveRegisterFunc g_achieve_reg_func;

extern achieve_mgr_t g_achieve_mgr;
extern buff_conf_mgr_t g_buff_conf_mgr;
extern buff_handler_t g_buff_handler;

extern pet_quality_conf_manager_t g_pet_quality_conf_mgr;
extern family_conf_manager_t g_family_conf_mgr;
extern duplicate_world_boss_mgr_t g_world_boss_mgr;

extern std::vector<std::string> g_rand_nick_pos1[2]; // 性别区分
extern std::vector<std::string> g_rand_nick_pos2;
extern std::vector<std::string> g_rand_nick_pos3;

extern bool g_svr_msg_locked;
extern bool g_cli_msg_locked;

extern uint32_t g_svr_close_before_zero;
extern uint32_t g_svr_open_after_zero;

extern exped_conf_mgr_t g_exped_mgr;

extern cultivate_equip_conf_manager_t g_cultivate_equip_mgr;
extern dup_area_prize_conf_manager_t g_dup_area_prize_conf_mgr;
extern question_conf_manager_t g_question_conf_mgr;
extern suit_conf_manager_t g_suit_conf_mgr;
extern srv_time_manager_t g_srv_time_mgr;
extern bless_pet_conf_manager_t g_bless_pet_conf_mgr;

extern title_conf_mgr_t g_title_conf_mgr;

extern uint32_t g_server_id;
extern uint32_t g_online_id;
extern uint32_t g_cur_max_dup_id;

extern equip_buff_rand_manager_t g_equip_buff_rand_mgr;
extern std::set<uint32_t> g_client_buff_set;
extern pet_pass_dup_conf_mgr_t g_pet_pass_dup_conf_mgr;

#endif
