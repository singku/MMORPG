#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "common.h"
#include "attr_utils.h"
#include "proto/client/common.pb.h"
#include "pet.h"
#include "package.h"
#include "shop.h"
#include "pet_conf.h"
#include "global_data.h"
#include "night_raid.h"
#include "mine.h"

class Attr;
struct temp_info_t;
class ProtoQueue;
struct player_t;
class product_t;
class RuneMeseum;
class TaskInfo;
class TranCard;
class home_data_t;
class FriendInfo;
struct Package;
class Achieve;
class Expedition;
class NightRaid;
class Title;

enum player_sex_t {
    kSexGirl = 0,
    kSexBoy = 1,
};

enum player_prof_t {
	kWarrior = 1,
	kWizard = 2,
	kArcher = 3,
  
};

enum player_dup_state_t {
    PLAYER_DUP_NONE = 0,
    PLAYER_DUP_PLAY = 2,
    PLAYER_DUP_OFFLINE = 3,
    PLAYER_DUP_END_WIN = 4,
    PLAYER_DUP_END_LOSE = 5,
};

enum {
    NORMAL_VP   = 200,
    VIP_VP      = 240,
    SVIP_VP     = 300,
    ADD_VP      = 50,
};

enum login_steps_t { //最多64个
    login_steps_check_session = 1,
    login_steps_get_active = 2,
    login_steps_get_login_info = 3,
    login_steps_get_family_info = 4,
    login_steps_get_money = 5,
    login_steps_get_snowball_rank = 6,
    login_steps_get_escort_info = 7,
    login_steps_get_blood_exp_rank = 8,
    login_steps_get_family_skills = 9,
    login_steps_get_hot_rich_rank = 10,
    login_steps_get_boss_info = 11,
	login_steps_get_arena_weekly_rank = 12,
	login_steps_get_arena_daily_rank = 13,
};

enum player_duplicate_status_t {
    DUPLICATE_NOT_READY   = 0,
    DUPLICATE_READY       = 1,
};

enum pvp_state_t {
	PLAYER_NOT_IN_PVP = 0,
	PLAYER_IN_PVP = 1,
};

enum player_ohter_common_data {
	YEARLY_VIP_RECHARGE_NEED_TIMES = 6,
};

//更新总战力榜，装备评分榜的间隔
const uint32_t UPDATE_POWER_TM_INTERVL = 60;

struct cache_prize_elem_t {
    cache_prize_elem_t() {
        clear();
    }
	//同种类型
	bool operator ==(const cache_prize_elem_t& other)const{
		//物品，属性，符文才可以合并
		if(type != 1 || type != 3 || type != 5){
			return false;
		}
		return (id == other.id) && (type == other.type) &&
		   	(level == other.level) && (expire_time == other.expire_time);
	}

	bool operator <(const cache_prize_elem_t& other) const{
		return ((type == other.type) && (id < other.id)) || (type < other.type);
	}

	cache_prize_elem_t & operator =(const cache_prize_elem_t& other){
		if(this == &other){
			return *this;
		}
        type = other.type;
        id = other.id;
        count = other.count;
        level =  other.level;
        talent_level = other.talent_level;
        expire_time = other.expire_time;
        show =  other.show;
        notice =  other.notice;
        pow = other.pow;
        price = other.price;
        price_type = other.price_type;
		return *this;
	}
    void clear() {
        type = 0;
        id = 0;
        count = 0;
        level = 0;
        talent_level = 0;
        expire_time = 0;
        show = 0;
        notice = 0;
        pow = 0;
        price = 0;
        price_type = 0;
    }
    uint32_t type;
    uint32_t id;
    int32_t count;
    uint32_t level;
    uint32_t talent_level;
    uint32_t expire_time;
    uint32_t show;
    uint32_t notice;
    uint32_t pow;
    uint32_t price_type;
    uint32_t price;
};

// 不需要存档的一些内存数据
struct temp_info_t {
    //online向svr发起请求的时间(计算svr响应时间)
    struct timeval svr_req_start; 
    //client向onl发起请求的时间(计算cli响应时间)
    struct timeval cli_req_start; 


    //获取排名时的临时信息
    //uint32_t get_rank_start;
    //uint32_t get_rank_cnt;
    uint32_t get_self_rank;
	uint32_t arena_week_rank;
    std::string *rank_info;
	//竞技排名中，rank对应的玩家role_key
	std::map<uint32_t, uint64_t> *m_arena_index_info;
	//玩家当前准备挑战的ai玩家米米号(注:竞技场专用)
	uint64_t ai_id;
	//竞技场当前挑战的ai玩家的昵称
	std::string *ai_nick;
	//竞技场玩家自身的tick
	uint32_t arena_atk_tick;
	//竞技场ai的tick
	uint32_t arena_def_tick;
	//竞技场，玩家输赢状态
	uint32_t arena_result;

    std::string* state_buf; //玩家角色状态
    //登录时的临时信息
    bitset<64> *login_steps;
    bool login_too_much; // 是否登录过多，需要回答问题
    uint32_t login_too_much_answer;  // 登录次数过多问题答案
    uint32_t login_too_much_answer_times; // 回答次数

    //能否消耗钻石
    bool can_use_diamond;

    //当前使用物品的slot_id
    uint32_t cur_use_item_slot_id;

    //临时状态值
    bitset<1024> *bit_vec;

    //副本掉路可捡物品缓存, 客户端捡物品数据包校验使用
    //indexing, prizes
    std::map<uint32_t, cache_prize_elem_t> *cache_dup_drop_prize;
    uint32_t dup_id; //玩家如果在副本的话 保存副本id
    uint32_t dup_map_id; //副本的场景id
    uint32_t dup_kill; //是否杀死了怪(几个)
    player_dup_state_t dup_state;
    uint32_t dup_enter_tm; //进入副本的时间
    uint32_t ready_enter_dup_id; //预备进入的副本id
    uint32_t dup_phase;//当前阶段
    uint32_t dup_kill_mon_id; //当前击杀的怪的ID
    uint32_t dup_ready_seq;

    //请求奖励
    uint32_t req_prize_id; //客户端请求奖励缓存ID
	//全服限制，发奖等回调
    std::vector<uint32_t> *cache_prize_id;
    std::vector<cache_prize_elem_t> *cache_prize_vec;
    std::vector<cache_prize_elem_t> *cache_tmp_vec;

    std::vector<uint32_t> *cache_question_vec;
	//伙伴祈福，缓存队友battle信息
    std::vector<std::string> *bless_team_member_info;

	//上次聊天时间
	uint32_t last_chat_time;
	//运宝过程中，被打劫的玩家id(uid+create_tm)
	uint64_t escort_def_id;
	//被我打劫的玩家的此轮运宝，我是第几个打劫该玩家的标志
	uint32_t escort_rob_id;
	//我的此轮运宝，开始打劫的第几个玩家的标志
	uint32_t escort_p_rob_id;
	//我现在正在打劫其他玩家的标志
	//打劫时，被打劫的玩家直接用钻石结束了运宝
	//那么打劫结束的结算协议中，会在运宝管理器中找不到玩家的信息，而出错的处理
	uint32_t escort_robbing_flag;
	//运宝打劫时，保存被打劫的米米号，以及在被打劫方属性表中的我作为打劫方的序号
	std::map<uint32_t, uint32_t>* atk_no_map;
	//运宝，打劫开始的时间戳
	uint32_t escort_rob_tm;
	
	//玩家PVP的状态(1.战斗中；0.未战斗)
	//uint32_t pvp_state;
	//战报临时存放
	std::string* tmp_btl_info;

    //一桶天下中打破的桶的数量
    uint32_t bucket_cnt;
    //一桶天下周打破的桶的积分
    uint32_t bucket_score;
	//玛音试练打破桶的数量(暂时用不到)
	uint32_t mayin_bucket_cnt;
	//玛音试练本次副本获得的能量
	uint32_t mayin_bucket_cur_energy;

    //游泳
    uint32_t total_swim_exp;
    uint32_t cur_swim_exp;
    uint32_t dive_exp;
	//伙伴祈福
	string *bless_team_info;
	uint32_t bless_pet_id;

    //家族
    /*uint32_t family_leader_reassign_uid;*/
    role_info_t family_leader_reassign_role;
    bool family_leader_reassign_response;
    uint32_t family_dup_boss_lv;
    uint32_t family_dup_boss_hp;
    uint32_t family_dup_boss_maxhp;

    //手动设置服务器超时时间
    uint32_t my_svr_time_out;

	//怪物危机:一关中，mon_cris_hp改变的精灵ctm --> hp
	std::map<uint32_t, int> * mon_cris_pets;

	//夜袭:一关中，mon_cris_hp改变的精灵ctm --> hp
	std::map<uint32_t, int> * night_raid_pets;
	std::map<uint32_t, int> * night_raid_op_pets; //对手
	std::set<uint64_t> * dirty_users; //脏数据对手

    int tmp_max_hp; //临时保存最大血量

	uint32_t time_use;//每日答题活动用时
	uint32_t score_double;//每日答题活动使用得分翻倍

	//夜袭：玩家血量改变值
	int night_raid_player_hp;
	int night_raid_op_player_hp;//对手

    userid_t other_userid; //查询用户id
    uint32_t other_create_tm; //查询用户createtm
	uint32_t get_other_inf_reason;//获取原因
	
	uint32_t prize_reason;//奖励原因

    // 在当前服参加的世界boss玩法开始时间,判断跨服重复奖励用
    uint32_t world_boss_dup_start_time;

    // 缓存当前请求包的信息
    std::string *cache_string;

    // 缓存当前返回包的信息
    std::string *cache_out_string;
	//获取玩家游戏经验频繁次数
	uint32_t swim_get_exp_freq_times;
	//资源找回
	//前一天是否游泳
	uint32_t last_day_swim_times;
	//前一天运宝次数
	uint32_t last_day_escort_times;
	//前一天怪物危机是否重置
	uint32_t last_day_mon_crisis_reset_times;

    uint32_t last_get_session_time;

    //玩家进入副本前出战位精灵的创建时间
    uint32_t tmp_fight_pos_pet_create_tm[MAX_FIGHT_POS];

    //是否老机器
    uint32_t is_old_machine;
    //本次是否换线登陆
    uint32_t is_switch_online;

    // 玩家队伍
    uint32_t team;

	//一次副本中，复活的次数
	uint32_t revival_cnt;

    //玩家缓存的临时等级
    uint32_t cache_level;

    //对手的RPVP积分
    uint32_t op_rpvp_score;
};

struct player_t
{
    userid_t userid; // 米米号
    bool is_login; // 是否已经登录
    uint32_t seqno; // 协议序列号
    uint32_t pkg_idx; //包序号
    uint32_t create_tm; // 创建时间
    uint32_t server_id; //当前服务器ID
    uint32_t init_server_id; //原始服务器ID
    char nick[50];
    char family_name[commonproto::FAMILY_NAME_MAX_LEN]; // 家族名
    uint32_t family_hall_line_id;       // 当前所在家族大厅分线
    uint32_t family_hall_map_id;        // 当前所在家族大厅id
    std::vector<std::string> *family_lock_sets;  // 用户操作锁集合
    std::set<uint32_t> *family_apply_record;  // 玩家申请的家族记录
    // player_t里面不能用protobuf结构定义,重载时会卸载protobuf lib库,在player保存的数据中会留有野指针
    std::map<uint32_t, ol_market_item_t> *elem_dup_shop_items;   // 元素挑战商店刷新物品
    std::map<uint32_t, ol_market_item_t> *arena_shop_items;      // 竞技场商店刷新物品
    std::map<uint32_t, ol_market_item_t> *family_shop_items;      // 家族商店刷新物品
	std::map<uint32_t, ol_market_item_t> *exped_shop_items;		//远征商店刷新物品
	std::map<uint32_t, ol_market_item_t> *night_raid_shop_items;		//远征商店刷新物品
	std::map<uint32_t, ol_market_item_t> *daily_shop_items;		//每日商店刷新物品
	std::map<uint32_t, ol_market_item_t> *smelter_money_shop_items;
	std::map<uint32_t, ol_market_item_t> *smelter_gold_shop_items;

	std::vector<cache_prize_elem_t> *daily_charge_diamond_draw_cards_info;//充钻抽卡的卡牌信息

    //uint32_t sex; // 1男 0女
    fdsession_t* fdsess; // async_serv 和客户端通信session
    uint32_t cli_wait_cmd; // 客户端请求命令号
    uint32_t wait_svr_cmd; // 等待服务端的命令号
    uint32_t serv_cmd; // 请求服务端命令号
    Package* package;
	TaskInfo* task_info;
	FriendInfo* friend_info;
    uint32_t cur_map_id; // 当前地图id
    uint32_t cur_map_line_id; //当前所在的地图分线
    uint32_t map_x; // 地图坐标x
    uint32_t map_y; // 地图坐标y
    uint32_t heading;
    uint32_t last_map_id; //上一次所在地图ID

    std::map<uint32_t, Pet> *pets; // 所有精灵
    std::map<uint32_t, Pet*> *bag_pets; //背包精灵
    std::map<uint32_t, Pet*> *store_pets; //仓库精灵
    std::map<uint32_t, Pet*> *elite_pets; //精英精灵
    std::map<uint32_t, Pet*> *room_pets; //精灵屋精灵
    std::map<uint32_t, std::vector<Pet*> > *pet_id_pets; // petid对应的精灵
    Pet *fight_pet[MAX_FIGHT_POS];    //跟随精灵
	RuneMeseum* rune_meseum;	//符文背包
	TranCard* m_tran_card;	//变身卡信息
	home_data_t* home_data;
	Achieve* achieve;	//成就信息
	Expedition* expedtion;	//远征中，对手的信息
	NightRaid*  nightraid; //夜袭对手信息
    
    Attr *attrs; //各种属性数据
    ProtoQueue* proto_queue; //协议缓存队列
	//std::set<uint32_t> *praise_uid;	//今日已经点赞的米米号
	Title* title;
	MineInfo* mine_info;
	
    //<over_type, buff>
    std::map<uint32_t, uint32_t> *buff_id_map; //玩家拥有的所有后台购买的buff 同一种overtype只能有一个
    std::map<uint32_t, uint32_t> *suit_buff_map; //玩家拥有的套装buff: suitid, buffid

    list_head_t timer_list; // 用户定时器列表
    timer_struct_t* sync_client_time_timer; // 同步客户端时间定时器
    timer_struct_t* daily_op_timer; //每日12点做一些操作(如果在线的话)
    timer_struct_t* svr_request_timer; // 请求svr的定时器
    timer_struct_t* clear_daily_attr_timer; // 清理每日属性定时器
    timer_struct_t* check_money_return; //检查向DB请求的钱是否正常返回
    timer_struct_t* clean_expired_items_timer; //定时清理道具的定时器
    timer_struct_t* vp_flush_timer; //每日六点恢复体力.
    timer_struct_t* shop_flush_timer; //特殊商店刷新定时器
    timer_struct_t* dive_timer; //跳水清理定时器
    timer_struct_t* vp_add_timer; //体力恢复定时器
	timer_struct_t* weekly_arena_reward_timer;	//竞技场周排行奖励
	timer_struct_t* exercise_pets_add_exp;	//锻炼中的精灵定时加经验定时器
	timer_struct_t* check_addiction_noti;	//锻炼中的精灵定时加经验定时器
	timer_struct_t* daily_arena_reward_timer; //竞技场日排名奖励
	timer_struct_t* diamond_recharge_reward_timer; //充值排行奖励
	timer_struct_t* open_srv_power_reward_timer; //限时战力榜奖励
	timer_struct_t* open_srv_gold_consume_reward_timer; //限时战力榜奖励
	timer_struct_t* get_dump_rank_tm;	//获取该区服复制排行榜的时间戳
	timer_struct_t* weekly_activity_rank_reward_timer;	//获取该区服复制排行榜的时间戳

    temp_info_t temp_info; // 临时数据
    /* 缓存session放在最后一个字段 */
    char session[4096];  // 缓存
};

/////////////////////////////////////////helper functions//////////////////////
//----------------------------------------------------------------------------
//玩家登录的某个流程是否完成
bool is_login_step_finished(player_t *player, login_steps_t step);
void set_login_step_finished(player_t *player, login_steps_t step);
// 发送二进制流到客户端
int send_buff_to_player(player_t *player, uint32_t cmd, const char *body, uint32_t bodylen, uint32_t seq = 0, int ret = 0);
// 发送protobuf到客户端
int send_msg_to_player(player_t* player, uint32_t cmd, const google::protobuf::Message& msg);
// 发送错误码到客户端
int send_err_to_player(player_t* player, uint32_t cmd, int ret, uint32_t seq = 0);
int send_err_to_fdsess(fdsession_t *fdsess, uint32_t cmd, uint32_t uid, int err, uint32_t seq = 0);
// 玩家离开服务器
int player_leave_server(player_t* player);
//获取购买+赠送的money
uint32_t player_get_diamond(player_t *player);
//改变钻石，赠送的钻石直接改变到属性值值 
//扣减时先扣购买的钻石 再扣赠送的钻石
//赠送钻石不走这个逻辑
int player_chg_diamond_and_sync(player_t *player, int32_t diff, 
        const product_t *pd, uint32_t pd_cnt, 
        dbproto::channel_type_t chn, string default_pd_name="商品购买", 
        bool sync_db=true, bool add_paid_diamond = false);

//设置购买的钻石
void player_set_diamond(player_t *player, uint32_t diamond);
// 计算当天用户上线时间
uint32_t calc_daily_online_time(player_t* player);
//计算玩家每日防沉迷在线的时间,离线时间达到规定时间，会清零
uint32_t calc_daily_addiction_online_time(player_t* player);
// 计算当天用户下线时间
uint32_t calc_daily_offline_time(player_t* player);
//是否是vip(白银或者黄金)
bool is_vip(const player_t* player);
//是否是白银勋章vip
bool is_silver_vip(const player_t* player);
//是否是黄金勋章vip
bool is_gold_vip(const player_t* player);
//判断vip类型
player_vip_type_t get_player_vip_flag(const player_t* player);

// 根据base_info判断vip类型
player_vip_type_t get_player_base_info_vip_flag(
        const commonproto::player_base_info_t &info);
void encrypt_msg(unsigned char* msg, int msglen, uint32_t uid = 0);
void decrypt_msg(unsigned char* msg, int msglen, uint32_t uid = 0);
uint32_t calc_seqno(uint32_t pkg_len, uint32_t seqno, uint16_t cmd);
void finish_module(player_t *player);
void try_flush_shop(player_t *player, bool immediatly, onlineproto::market_type_t market_type);
#if 0
void try_flush_daily_shop(player_t *player, bool immediatly);
void try_flush_elem_dup_shop(player_t *player, bool immediatly);
void try_flush_arena_shop(player_t *player, bool immediatly);
void try_flush_exped_shop(player_t *player, bool immediatly);
void try_flush_night_raid_shop(player_t *player, bool immediatly);
void try_flush_family_shop(player_t *player, bool immediatly);
#endif
void try_setup_shop_timer(player_t *player);
void try_flush_all_shops(player_t *player, bool imeediatly);

//在登录时或者在线跨天时调用玩家的每日/月/周数据初始化
void try_init_player_daily_data(player_t *player);

// 是否是本次活动的年费vip
bool is_this_activity_year_vip(const player_t* player);

// 登录后处理年费vip
uint32_t deal_with_year_vip_state_when_login(player_t* player);

//检查上次签到是否在本轮活动范围内
uint32_t clean_summer_signed_total_times(player_t* player);

/////////////////////////////////////////inline functions////////////
//-----------------------------------------------------------////////
// 发送空包头到客户端
inline int send_header_to_player(player_t *player, uint32_t cmd) {
    return send_err_to_player(player, cmd, 0);
}

inline uint32_t get_vip_level(const player_t* player)
{
    if (!is_vip(player)) {
        return 0;
    }
    return GET_A(kAttrVipLevel);
}

bool is_this_activity_vip(const player_t* player);

inline void flush_vp(player_t *player)
{
    uint32_t cur_vp = GET_A(kAttrCurVp);
    uint32_t limit = 0;
	/*
    if (is_this_year_vip(player)) {
        limit = SVIP_VP;
    } else if (is_vip(player)) {
        limit = VIP_VP;
    } else {
        limit = NORMAL_VP;
    }
	*/
	if (is_gold_vip(player)) {
		limit = SVIP_VP;
	} else if (is_silver_vip(player)) {
		limit = VIP_VP;
	} else {
		limit = VIP_VP;
	}
    uint32_t new_vp = cur_vp + ADD_VP;
    if (new_vp >= limit) {
        SET_A(kAttrCurVp, limit);
    } else {
        SET_A(kAttrCurVp, new_vp);
    }
    SET_A(kDailyVpGet, NOW());
}


inline bool player_has_fight_pet(player_t *player)
{
    for (int i = 0; i < MAX_FIGHT_POS; i++) {
        if (player->fight_pet[i] != 0) {
            return true;
        }
    }
    return false;
}

// inline bool check_player_addicted_threshold_one_hour(player_t* player)
// {
	// uint32_t online_time = calc_daily_addiction_online_time(player);
	
	// if (online_time >= kOnlineTimeThresholdone) {
		// return true; 
	// } else {
		// return false; 
	// }
// }

inline bool check_player_addicted_threshold_half(player_t* player)
{
	bool TIME1 = TimeUtils::is_current_time_valid(11, 1);
	bool TIME2 = TimeUtils::is_current_time_valid(11, 2);
	bool TIME3 = TimeUtils::is_current_time_valid(11, 3);
    // 防沉迷开关
	if (g_server_config.shut_addiction || TIME1 || TIME2 || TIME3){
		return false;
	}

    uint32_t online_time = calc_daily_addiction_online_time(player);
    
    if (online_time >= kOnlineTimeThresholdHalf) {
        return true; 
    } else {
        return false; 
    }
}

inline bool check_player_addicted_threshold_none(player_t* player)
{
	bool TIME1 = TimeUtils::is_current_time_valid(11, 1);
	bool TIME2 = TimeUtils::is_current_time_valid(11, 2);
	bool TIME3 = TimeUtils::is_current_time_valid(11, 3);
    // 防沉迷开关
	if (g_server_config.shut_addiction || TIME1 || TIME2 || TIME3){
		return false;
	}

    uint32_t online_time = calc_daily_addiction_online_time(player);
    
    if (online_time >= kOnlineTimeThresholdNone) {
        return true; 
    } else {
        return false; 
    }
}

inline bool check_player_clear_addicted(player_t* player)
{
    uint32_t offline_time = calc_daily_offline_time(player);
    
    if (offline_time >= KMaxOffLineTime) {
        return true; 
    } else {
        return false; 
    }
}
#endif
