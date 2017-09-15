#ifndef __DUPLICATE_H__
#define __DUPLICATE_H__

#include "common.h"
#include "duplicate_trigger.h"

#define MAX_ACTION_CONDS    (9)

enum duplicate_actor_team_t {
    DUP_ACTOR_TEAM_ENEMY = 0, //敌军
    DUP_ACTOR_TEAM_FRIEND = 1, //友军
    DUP_ACTOR_TEAM_NON_COUNT = 2, //非敌非友，可以打，但打死不计入杀怪
};

enum duplicate_actor_type_t {
    DUP_ACTOR_TYPE_PET = 0, //精灵
    DUP_ACTOR_TYPE_PLAYER = 1, //角色
    DUP_ACTOR_TYPE_NPC = 2, //NPC
    DUP_ACTOR_TYPE_BUILDER = 3, //builder
};

enum duplicate_battle_type_t {
    DUP_BTL_TYPE_ERR = 0,
    DUP_BTL_TYPE_PVE = 1,   //pve
    DUP_BTL_TYPE_PPVE = 2, //多人pve
    DUP_BTL_TYPE_PVP = 3, //pvp
    DUP_BTL_TYPE_WORLD_BOSS = 4, //世界BOSS
    DUP_BTL_TYPE_RPVP = 5, //手动竞技场
    DUP_BTL_TYPE_PVEP = 6, //手动pvep
    DUP_BTL_TYPE_PEVE = 7, //单人带ai队友打怪
	DUP_BTL_TYPE_END  
};

enum duplicate_difficulty_type_t {
    DUP_DIF_TYPE_NORMAL = 1, //普通
    DUP_DIF_TYPE_ELITE  = 2, //精英
};

enum duplicate_pet_flush_type_t {
    DUP_PET_FLUSH_TYPE_DEFAULT = 0, //不按波刷怪
    DUP_PET_FLUSH_TYPE_PHASE = 1, //按波刷怪
};

enum duplicate_pet_flush_trigger_t {
    DUP_PET_FLUSH_TRIG_TYPE_SVR = 0, //服务器自动刷怪
    DUP_PET_FLUSH_TRIG_TYPE_CLI = 1, //前端请求刷怪
};

//副本开放类型
enum duplicate_open_type_t {
    DUP_OPEN_TYPE_ALL = 0, //开放
    DUP_OPEN_TYPE_DATE = 1, //日期内开放
    DUP_OPEN_TYPE_PHASE = 2, //每日几点到几点开放
};


//副本->场景->怪
struct duplicate_actor_t {
    duplicate_actor_team_t team; //敌我类型
    duplicate_actor_type_t type; //精灵还是NPC还是角色
    uint32_t id; //各种id
    std::vector<uint32_t> patrol_paths; //出生后的行走轨迹点
    uint32_t born_effect; //出生特效
    uint32_t born_action; //出生后的行为
    uint32_t heading; //出生方向
    std::string born_action_args; //出生后的行为参数
    uint32_t ai_start_delay; //出生后ai延迟行动的时间(毫秒)
    uint32_t life_time; //出生后的时长，默认为0, 打死才消失
    uint32_t uniq_key; // 怪的另一种标识 用于前端识别
    std::string dynamic_params; //前端用的解析参数
    uint32_t no_stat; //杀怪后是否计数
    uint32_t float_height; //漂浮高度
};

struct duplicate_born_area_t {
    uint32_t born_x; //出生点x
    uint32_t born_y; //出生点y
    uint32_t born_radius; //出生点半径
    uint32_t flush_method; //0表示死一个随机刷一个 1表示死一个顺序刷下一个
    uint32_t flush_max; //如果flush_method为1 则flush_max被忽略
    uint32_t g_area_idx; //全局的序标
    uint32_t succeed; //可否连续刷怪
    uint32_t born_rate; //刷怪概率 10000为基数
    std::vector<duplicate_actor_t> born_actors; //该出生点下的怪
};

//副本
struct duplicate_t {
    duplicate_t() {
        duplicate_id = 0;
        battle_type = DUP_BTL_TYPE_ERR;
        time_limit = 0;
        boss_show_time_limit = 0;
        mode = onlineproto::DUP_MODE_TYPE_NORMAL;
        req_power = 0;
        action_vec.clear();
        cond_map.clear();
        map_ids.clear();
        mon_vec.clear();
        builder_vec.clear();
    }
    uint32_t duplicate_id; //副本id
    duplicate_battle_type_t battle_type; //副本战斗类型 pvp pve ppve
    uint32_t time_limit; //副本完成时间限制
    uint32_t mode;      // 副本类型
    uint32_t boss_show_time_limit; //boss出现的时长
    std::vector<DupActionBase*> action_vec; //该副本所有的行为
    //该副本中所有条件 按条件的类型分类
    std::map<uint32_t, std::vector<DupCondBase*> > cond_map;
    //副本中的所有场景
    std::set<uint32_t> map_ids;
    //副本中的所有怪的列表
    std::vector<uint32_t> mon_vec;
    //副本中的所有阻挡的列表
    std::vector<uint32_t> builder_vec;
    //战力压制时的推荐战力
    uint32_t req_power;
	//副本中精灵的等级在基于玩家等级基础上所增加的值
	uint32_t add_fight_level;
};

//副本配置管理器
class duplicate_conf_manager_t {
public:
    void clear() {
        FOREACH(dup_map_, it1) {
            duplicate_t &dup = it1->second;
            FOREACH(dup.action_vec, it2) {
                DupActionBase *action = *it2;
                delete action;
            }
            dup.action_vec.clear();
            FOREACH(dup.cond_map, it3) {
                std::vector<DupCondBase*> &cond_vec = it3->second;
                FOREACH(cond_vec, it4) {
                    DupCondBase *cond = *it4;
                    delete cond;
                }
                cond_vec.clear();
            }
            dup.cond_map.clear();
        }
        dup_map_.clear();
    }
    duplicate_conf_manager_t() {
        dup_map_.clear();
    }
    ~duplicate_conf_manager_t() {}

public: //inline functions
    inline bool duplicate_exist(uint32_t dup_id) {
        if (dup_map_.count(dup_id) > 0) {
            return true;
        }
        return false;
    }
    inline bool add_duplicate(duplicate_t &dup) {
        if (duplicate_exist(dup.duplicate_id)) {
            return false;
        }
        dup_map_[dup.duplicate_id] = dup;
        return true;
    }
    const inline duplicate_t *find_duplicate(uint32_t dup_id) {
        if (dup_map_.count(dup_id) == 0) {
            return NULL;
        }
        return &((dup_map_.find(dup_id))->second);
    }
    duplicate_battle_type_t get_duplicate_type(uint32_t dup_id) {
        const duplicate_t *dup = find_duplicate(dup_id);
        if (!dup) {
            return DUP_BTL_TYPE_ERR;
        }
        return dup->battle_type;
    }
    uint32_t get_dup_req_power(uint32_t dup_id) {
        const duplicate_t *dup = find_duplicate(dup_id);
        if (dup) {
            return dup->req_power;
        } else {
            return 0;
        }
    }
    inline bool dup_has_map(uint32_t dup_id, uint32_t map_id) {
        const duplicate_t *dup = find_duplicate(dup_id);
        if (!dup) {
            return false;
        }
        if (dup->map_ids.count(map_id) == 0) {
            return false;
        }
        return true;
    }
    inline void copy_from(const duplicate_conf_manager_t &m) {
        dup_map_ = m.const_dup_map();
    }
    const inline std::map<uint32_t, duplicate_t>& const_dup_map() const{
        return dup_map_;
    }
private:
    std::map<uint32_t, duplicate_t> dup_map_;
};

extern void show_dup(uint32_t dup_id);
    
#endif
