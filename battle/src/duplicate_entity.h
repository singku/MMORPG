#ifndef __BATTLE_ENTITY_H__
#define __BATTLE_ENTITY_H__

#include "common.h"

#define DUP_OBJ_TYPE_PLAYER     (1)
#define DUP_OBJ_TYPE_PET        (2)
#define DUP_OBJ_TYPE_MON        (3)
#define DUP_OBJ_TYPE_BUILDER     (4)

#define DUP_REVIVAL_TYPE_PLAYER (1)
#define DUP_REVIVAL_TYPE_PET (2)
#define DUP_REVIVAL_TYPE_ALL (3)

#include "duplicate_conf.h"

enum player_duplicate_status_t {
    PLAYER_NOTHING      = 0, //啥也没干
    PLAYER_IN           = 1, //进入
    PLAYER_READY        = 2, //准备
    PLAYER_PLAY         = 3, //进行
    PLAYER_DEAD         = 4, //死亡
    PLAYER_EXIT         = 5, //退出
    PLAYER_LEAVE        = 6, //离开地图(有可能离线)
};

enum player_duplicate_side_t {
    SIDE0           = 0, //没有边
    SIDE1           = 1,
    SIDE2           = 2,
};

//副本的状态
enum duplicate_entity_status_t {
    DUPLICATE_STATE_CREATE                  = 1, //副本被创建
    DUPLICATE_STATE_WAIT_PLAYER_READY       = 2, //等待玩家准备
    DUPLICATE_STATE_ALL_READY               = 3, //所有人都准备好了
    DUPLICATE_STATE_START                   = 4, //副本开始了
    DUPLICATE_STATE_CLEAR                   = 5, //副本怪全清了
    DUPLICATE_STATE_ALL_LEAVE               = 6, //所有人都离开了
    DUPLICATE_STATE_CAN_END                 = 7, //副本可以结束了
};

//副本上的怪物信息
struct duplicate_map_pet_t {
    duplicate_map_pet_t() {
        clear();
    }
    void clear() {
        type = DUP_ACTOR_TYPE_PET;
        team = DUP_ACTOR_TEAM_ENEMY;
        pet_id = 0;
        pet_level = 0;
        create_tm = 0;
        cur_hp = 0;
        max_hp = 0;
        pos_x = 0;
        pos_y = 0;
        mon_type = 0;
        patrol_paths.clear();
        born_effect = 0;
        born_action = 0;
        float_height = 0;
        around_type = 0;
        around_create_tm = 0;
        around_radius = 0;
        heading = 0;
        born_area_idx = 0;
        phase = 0;
        born_action_args.clear();
        ai_start_delay = 0;
        life_time = 0;
        uniq_key = 0;
        dynamic_params.clear();
        no_stat = 1;
        is_pet = 1;
        req_power = 0;
        hit_cnt = 0;
    }
    uint32_t is_pet;
    uint32_t type;
    uint32_t team;
    uint32_t pet_id;
    uint32_t pet_level;
    uint32_t create_tm;
    uint32_t cur_hp;
    uint32_t max_hp;
    uint32_t pos_x;
    uint32_t pos_y;
    uint32_t mon_type;
    std::vector<uint32_t> patrol_paths;
    std::set<uint32_t> affix_list; // 词缀列表
    uint32_t born_effect;
    uint32_t born_action;
    uint32_t float_height;
    uint32_t around_type;
    uint32_t around_create_tm;
    uint32_t around_radius;
    uint32_t heading;
    uint32_t born_area_idx; //记录是哪个出生点的,死亡之后判定是否要刷怪
    uint32_t phase;//记录是哪个阶段出生的
    std::string born_action_args; //出生后的行为参数
    uint32_t ai_start_delay; //出生后ai延迟行动的时间(毫秒)
    uint32_t life_time; //出生后的存在时长
    uint32_t uniq_key; // 怪的另一种标识 用于前端识别
    std::string dynamic_params; //前端用的解析参数
    uint32_t no_stat; //杀怪后是否计数
    uint32_t req_power; //战力压制时的要求战力
    uint32_t hit_cnt; //受击次数
};

struct  battle_side_players_t{
    uint32_t max_line_id;
    // <line_id, info>
    std::map<uint32_t, std::set<player_t *> > player_map;
};

//保存当前所打的副本的信息
struct duplicate_entity_t {
    uint32_t dup_id;
    uint32_t cur_map_id; //当前场景地图ID(副本里的场景是按顺序进行)
    uint32_t ready_map_id; //准备进入的下一个场景
    uint32_t cur_map_phase; //当前场景所处的阶段
    uint32_t cur_phase_born_obj_cnt; //当前阶段刷了多少对象出来
    uint32_t cur_phase_dead_pet_num; //当前阶段打死敌人的怪的数量
    uint32_t cur_phase_enemy_num; //当前阶段场景上存活的敌人数量
    uint32_t cur_phase_boss_out; //当前阶段的boss有没有刷出来
    std::set<uint32_t> *cur_map_dead_boss_set; //当前场景bosss是否被打死
    
    //<出生点下标, 出生数量>
    std::map<uint32_t, uint32_t> *born_area_born_record; //记录每阶段出生点的怪物刷出来的数量
    std::map<uint32_t, uint32_t> *born_area_dead_record; //记录每阶段出生点的怪物刷出来的数量

    //uint32_t 记录当前死亡的怪的出生点下标
    uint32_t cur_dead_obj_idx;
    uint32_t cur_dead_obj_key;
    //副本是否已经开打
    uint32_t start;

    //create_tm, dup_map_pet_t
    std::map<uint32_t, duplicate_map_pet_t> *cur_map_enemy; //当前场景中残余的敌人 NPC.野怪等
    std::map<uint32_t, duplicate_map_pet_t> *cur_map_non_enemy; //当前场景中的非敌人 机关,阻挡等

    std::set<uint32_t> *fini_map_id; //当前副本已经通过的场景
    uint32_t life_counter; //野怪计数器
    duplicate_entity_status_t state;
    //副本上的玩家按敌我分边 SIDE1 SIDE2
    std::map<uint32_t, std::set<player_t*> > *battle_players; //当前副本上所有玩家的信息

    std::map<uint32_t, battle_side_players_t> *line_players; //当前副本上所有玩家的分组信息

    //副本倒计时定时器
    list_head_t timer_list;
    timer_struct_t *dup_time_limit_timer;
    //副本刷怪定时器
    timer_struct_t *flush_mon_timer;
    //副本阶段时长定时器
    timer_struct_t *phase_timer;
    //副本自毁定时器(某些副本结束不是玩家导致的结束 而且玩家没有发退出副本的协议)
    timer_struct_t *destroy_dup_timer;
    //副本boss出现后倒计时
    timer_struct_t *boss_show_timer;
    //副本强制所有人准备的定时器
    timer_struct_t *force_ready_timer;

    bool time_up; //是否时间到

    //技能特效缓存相关
    uint32_t skill_affect_obj_type;
    uint32_t skill_affect_obj_create_tm;
};

class dup_entity_mgr_t {
public:
    dup_entity_mgr_t() {
        clear();
        //entity_map_.clear();
    }
    ~dup_entity_mgr_t() {
        //entity_map_.clear();
        clear();
    }
    void clear();
public:
    //玩家进入副本时创建一个实体
    duplicate_entity_t* create_entity(uint32_t dup_id, uint32_t init_map_id);
    //销毁实体
    void destroy_entity(duplicate_entity_t *dup_entity);
    //有玩家都离开后 尝试销毁副本实体
    void try_destroy_entity(duplicate_entity_t *dup_entity);

    //获取副本中一个玩家的uid
    static uint32_t get_entity_one_uid(duplicate_entity_t *dup_entity);

    // 获取单服共享世界BOSS副本实体 
    duplicate_entity_t *get_world_boss_entity();

    // 向副本实体中增加玩家记录
    uint32_t add_battle_player(duplicate_entity_t *dup_entity, uint32_t side, player_t *player);

    // 移除玩家记录
    int remove_battle_player(duplicate_entity_t *dup_entity, player_t *player);

    // 获取同分线玩家
    std::set<player_t *> * get_line_players(
            duplicate_entity_t *entity, uint32_t side, uint32_t line_id);

private:
    std::set<duplicate_entity_t*> entity_map_;
    duplicate_entity_t *world_boss_entity_;
};

//副本中同步通知消息
int relay_notify_msg_to_entity_except(duplicate_entity_t *dup_entity, 
        uint32_t cmd, const google::protobuf::Message &msg, player_t *except = 0);

//副本中同步通知消息(ppve玩法可分线)
int relay_notify_msg_to_entity_line_except(duplicate_entity_t *dup_entity, uint32_t line_id,
        uint32_t cmd, const google::protobuf::Message &msg, player_t *except);

//副本中推送通知消息
int send_msg_to_entity_except(duplicate_entity_t *dup_entity, 
        uint32_t cmd, const google::protobuf::Message &msg, player_t *except = 0);

//副本实体触发type类型的条件
//该函数一般放在执行函数的最后一句 或者直接return duplicate_entity_trig
//否则在触发完这个函数后 继续处理要判断副本是否结束了
int duplicate_entity_trig(duplicate_entity_t *entity, uint32_t type, std::vector<uint32_t> *args = 0);

//返回获胜的一方
uint32_t duplicate_entity_win_side(duplicate_entity_t *entity);

//删除副本实体中的玩家索引信息
void duplicate_entity_del_player(duplicate_entity_t *entity, player_t *player);
void duplicate_entity_new_captain(duplicate_entity_t *entity);

//根据脚本中怪的索引id在副本中找到对应的pet_id
uint32_t get_pet_id_by_actor_idx(uint32_t dup_id, uint32_t idx);
//根据脚本中阻挡的索引id在副本中找到对应的builder_id
uint32_t get_builder_id_by_actor_idx(uint32_t dup_id, uint32_t idx);

//打包副本中的玩家怪和阻挡
uint32_t pack_duplicate_object(duplicate_entity_t *entity);

//获取副本队伍的平均等级
uint32_t get_players_mean_level_by_side(duplicate_entity_t *entity, player_duplicate_side_t side);

//动态生成map_pet信息
uint32_t init_map_pet_info_dynamic(duplicate_entity_t *entity, duplicate_map_pet_t &dpet);

inline duplicate_map_pet_t *get_entity_map_pet_by_create_tm(duplicate_entity_t *entity, uint32_t create_tm) 
{
    duplicate_map_pet_t *dpet = 0;
    //敌人
    if (entity->cur_map_enemy->count(create_tm) != 0) {
        dpet = &((entity->cur_map_enemy->find(create_tm))->second);
        //非敌人
    } else {
        dpet = &((entity->cur_map_non_enemy->find(create_tm))->second);
    }
    return dpet;
}

inline duplicate_actor_team_t get_team_by_type(duplicate_entity_t *entity, uint32_t type, uint32_t create_tm = 0)
{
    duplicate_map_pet_t *dpet = 0;
    switch (type) {
    case DUP_OBJ_TYPE_PLAYER:
    case DUP_OBJ_TYPE_PET:
        return DUP_ACTOR_TEAM_FRIEND;
    case DUP_OBJ_TYPE_MON:
        dpet = get_entity_map_pet_by_create_tm(entity, create_tm);
        return (duplicate_actor_team_t)(dpet->team);
    default:
        return DUP_ACTOR_TEAM_FRIEND;
    }
}

#endif
