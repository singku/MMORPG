#ifndef __DUP_TRIGGER_H__
#define __DUP_TRIGGER_H__

#include "common.h"

struct duplicate_actor_t;
struct duplicate_t;
struct duplicate_entity_t;
struct duplicate_born_area_t;
struct duplicate_map_pet_t;

//副本中的条件类型
enum dup_cond_type_t {
    dup_cond_type_into_area             = 1,
    dup_cond_type_phase_kill            = 2,
    dup_cond_type_phase_kill_boss       = 3,
    dup_cond_type_at_phase              = 4,
    dup_cond_type_at_scene              = 5,
    dup_cond_type_into_scene            = 6,
    dup_cond_type_start_scene           = 7,
    dup_cond_type_start_phase           = 8,
    dup_cond_type_mon_count_low         = 9, //废弃
    dup_cond_type_end_phase             = 10,
    dup_cond_type_end_scene             = 11,
    dup_cond_type_all_ready             = 12,
    dup_cond_type_all_player_dead       = 13,
    dup_cond_type_ready_scene           = 14,
    dup_cond_type_time_up               = 15,
    dup_cond_type_mon_killed            = 16, //怪被杀死
    dup_cond_type_mon_flush_timer_up    = 17, //刷怪定时器到时
    dup_cond_type_phase_timer_up        = 18, //阶段定时器到
    dup_cond_type_boss_hp_less          = 19, //boss血量少于xx
    dup_cond_type_player_hp_less        = 20, //player血量少于xx
    dup_cond_type_front_mon_flush_req   = 21, //前端请求刷怪
    dup_cond_type_use_skill             = 22, //释放某个技能
    dup_cond_type_player_dead           = 23, //有玩家死亡
    dup_cond_type_boss_show_time_up     = 24, //BOSS展示时间到期
    dup_cond_type_actor_born            = 25, //特定的怪物出生 参数为unique_key
    dup_cond_type_max,
};

//副本中的行为类型
enum dup_action_type_t {
    dup_action_type_start_dup           = 1,
    dup_action_type_start_scene         = 2,
    dup_action_type_start_phase         = 3,
    dup_action_type_actor_born          = 4,
    dup_action_type_end_phase           = 5,
    dup_action_type_end_scene           = 6,
    dup_action_type_end_dup             = 7,
    dup_action_type_ready_scene         = 8,
    dup_action_type_role_recover        = 9,
    dup_action_type_max,
};

class DupCondBase;
class DupActionBase;

class DupCondBase {
public:
    DupCondBase(dup_cond_type_t type) {
        this->type = type;
    }
    virtual ~DupCondBase() {}

    //条件是否满足
    virtual bool cond_fulfill(duplicate_entity_t *entity);

    //是否同一个条件(一个副本中的条件可能有很多相同的(条件的参数也相同))
    virtual bool same_cond(DupCondBase *cond);

    //条件触发一个行为
    virtual void trig(duplicate_entity_t *entity);
    
    //条件参数是否吻合
    virtual bool args_match(std::vector<uint32_t> &args);

    //从配置文件加载条件
    virtual int load_cond_info(duplicate_t &dup, xmlNodePtr cur);

public:
    //返回条件的类型
    inline dup_cond_type_t get_type() {
        return type;
    }
public:
    dup_cond_type_t type;
    std::vector<uint32_t> args;
    std::set<DupActionBase*> listen_action;
};

class DupActionBase {
public:
    DupActionBase(dup_action_type_t type) {
        this->type = type;
        cond_group.clear();
    }
    virtual ~DupActionBase() {}
 
    //是否可以执行这个行为
    virtual bool actable(duplicate_entity_t *entity);
    //执行这个行为(特别注意 行为处理函数中 不能删除entity对象)
    virtual int act(duplicate_entity_t *entity) = 0;
    //从配置文件加载行为信息
    virtual int load_action_info(duplicate_t &dup, xmlNodePtr cur);
    //从配置文件加载行为的参数
    virtual int load_action_args(xmlNodePtr cur);
    //从配置文件加载行为的条件
    virtual int load_action_cond(duplicate_t &dup, xmlNodePtr cur);
    //从配置文件加载刷怪的表项(只有刷怪的子类才真实调用,其他子类调用这个默认函数)
    virtual int load_actor_info(duplicate_t &dup, xmlNodePtr cur) {
        return 0; //do noting
    }

public:
    //返回行为的类型
    inline dup_action_type_t get_type() {
        return type;
    }

public:
    typedef std::vector<DupCondBase*> cond_vec_t;
    typedef std::vector<cond_vec_t> cond_group_t;
    cond_group_t cond_group;
    dup_action_type_t type;
    std::vector<uint32_t> args;
};

class DupCondIntoArea : public DupCondBase {
public:
    DupCondIntoArea()
        :DupCondBase(dup_cond_type_into_area) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
};

class DupCondPhaseKillMonsterCount : public DupCondBase {
public:
    DupCondPhaseKillMonsterCount()
        :DupCondBase(dup_cond_type_phase_kill) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
};

class DupCondPhaseKillBossMonster : public DupCondBase {
public:
    DupCondPhaseKillBossMonster()
        :DupCondBase(dup_cond_type_phase_kill_boss) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
};

class DupCondAtPhase : public DupCondBase {
public:
    DupCondAtPhase()
        :DupCondBase(dup_cond_type_at_phase) {}

    bool cond_fulfill(duplicate_entity_t *entity);
};

class DupCondAtScene : public DupCondBase {
public:
    DupCondAtScene()
        :DupCondBase(dup_cond_type_at_scene) {}

    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
    bool cond_fulfill(duplicate_entity_t *entity);
};


class DupCondIntoScene : public DupCondBase {
public:
    DupCondIntoScene()
        :DupCondBase(dup_cond_type_into_scene) {}

    bool cond_fulfill(duplicate_entity_t *entity);
};

class DupCondStartScene : public DupCondBase {
public:
    DupCondStartScene()
        :DupCondBase(dup_cond_type_start_scene) {}

    bool cond_fulfill(duplicate_entity_t *entity);
};

class DupCondStartPhase : public DupCondBase {
public:
    DupCondStartPhase()
        :DupCondBase(dup_cond_type_start_phase) {}

    bool cond_fulfill(duplicate_entity_t *entity);
};

class DupCondMonsterCountLow : public DupCondBase {
public:
    DupCondMonsterCountLow()
        :DupCondBase(dup_cond_type_mon_count_low) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
};

class DupCondEndPhase : public DupCondBase {
public:
    DupCondEndPhase()
        :DupCondBase(dup_cond_type_end_phase) {}

    bool cond_fulfill(duplicate_entity_t *entity);
};

class DupCondEndScene : public DupCondBase {
public:
    DupCondEndScene()
        :DupCondBase(dup_cond_type_end_scene) {}

    bool cond_fulfill(duplicate_entity_t *entity);
};

class DupCondAllReady : public DupCondBase {
public:
    DupCondAllReady()
        :DupCondBase(dup_cond_type_all_ready) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur) {
        return 0;
    }
};

class DupCondPlayerDead : public DupCondBase {
public:
    DupCondPlayerDead()
        :DupCondBase(dup_cond_type_player_dead) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur) {
        return 0;
    }
};

class DupCondAllPlayerDead : public DupCondBase {
public:
    DupCondAllPlayerDead()
        :DupCondBase(dup_cond_type_all_player_dead) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur) {
        return 0;
    }
};

class DupCondReadyScene : public DupCondBase {
public:
    DupCondReadyScene()
        :DupCondBase(dup_cond_type_ready_scene) {}

    bool cond_fulfill(duplicate_entity_t *entity);
};

class DupCondTimeUp : public DupCondBase {
public:
    DupCondTimeUp()
        :DupCondBase(dup_cond_type_time_up) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur) {
        return 0;
    }
};

class DupCondMonsterKilled : public DupCondBase {
public:
    DupCondMonsterKilled()
        :DupCondBase(dup_cond_type_mon_killed) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
};

class DupCondMonFlushTimerUp : public DupCondBase {
public:
    DupCondMonFlushTimerUp()
        :DupCondBase(dup_cond_type_mon_flush_timer_up) {}
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur) {
        return 0;
    }
};

class DupCondPhaseTimerUp : public DupCondBase {
public:
    DupCondPhaseTimerUp()
        :DupCondBase(dup_cond_type_phase_timer_up) {}
    //int load_cond_info(duplicate_t &dup, xmlNodePtr cur) {
    //    return 0;
    //}
};

class DupCondBossHpLess : public DupCondBase {
public:
    DupCondBossHpLess()
        :DupCondBase(dup_cond_type_boss_hp_less) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
};

class DupCondPlayerHpLess : public DupCondBase {
public:
    DupCondPlayerHpLess()
        :DupCondBase(dup_cond_type_player_hp_less) {}

    bool cond_fulfill(duplicate_entity_t *entity);
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
};

class DupCondFrontMonFlushReq : public DupCondBase {
public:
    DupCondFrontMonFlushReq()
        :DupCondBase(dup_cond_type_front_mon_flush_req) {}

    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
};

class DupCondUseSkill : public DupCondBase {
public:
    DupCondUseSkill()
        :DupCondBase(dup_cond_type_use_skill) {}

    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
};

class DupCondBossShowTimeUp : public DupCondBase {
public:
    DupCondBossShowTimeUp()
        :DupCondBase(dup_cond_type_boss_show_time_up) {}
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur) {
        return 0;
    }
};

class DupCondActorBorn : public DupCondBase {
public:
    DupCondActorBorn()
        :DupCondBase(dup_cond_type_actor_born) {}
    int load_cond_info(duplicate_t &dup, xmlNodePtr cur);
    bool cond_fulfill(duplicate_entity_t *entity);
};

///////////////////////////////////////////////////
/////----------------Action----------------------//
///////////////////////////////////////////////////
class DupActionStartDup : public DupActionBase {
public:
    DupActionStartDup() 
        :DupActionBase(dup_action_type_start_dup) {}

    int act(duplicate_entity_t *entity); //do nothing
};

class DupActionStartScene : public DupActionBase {
public:
    DupActionStartScene() 
        :DupActionBase(dup_action_type_start_scene) {}

    int act(duplicate_entity_t *entity); //抛出startscene条件
};

class DupActionStartPhase : public DupActionBase {
public:
    DupActionStartPhase() 
        :DupActionBase(dup_action_type_start_phase) {}

    int load_action_args(xmlNodePtr cur);
    int act(duplicate_entity_t *entity); //抛出startphase条件
};

class DupActionActorBorn : public DupActionBase {
public:
    DupActionActorBorn() 
        :DupActionBase(dup_action_type_actor_born) {}

    int act(duplicate_entity_t *entity);
    int load_actor_info(duplicate_t &dup, xmlNodePtr cur);
    int load_action_args(xmlNodePtr cur);
    int init_pet_dynamic(
        const duplicate_actor_t &actor, 
        duplicate_entity_t *entity, duplicate_map_pet_t &dpet);
    std::map<uint32_t, duplicate_born_area_t> born_areas;
};

class DupActionEndPhase : public DupActionBase {
public:
    DupActionEndPhase() 
        :DupActionBase(dup_action_type_end_phase) {}

    int act(duplicate_entity_t *entity); //抛出endphase条件
};

class DupActionEndScene : public DupActionBase {
public:
    DupActionEndScene() 
        :DupActionBase(dup_action_type_end_scene) {}

    int act(duplicate_entity_t *entity); //抛出endscene条件
};

class DupActionEndDup : public DupActionBase {
public:
    DupActionEndDup() 
        :DupActionBase(dup_action_type_end_dup) {}

    int act(duplicate_entity_t *entity);
    int load_action_args(xmlNodePtr cur);
};

class DupActionReadyScene : public DupActionBase {
public:
    DupActionReadyScene() 
        :DupActionBase(dup_action_type_ready_scene) {}

    int act(duplicate_entity_t *entity); //抛出endscene条件
};

class DupActionRoleRecover : public DupActionBase {
public:
    DupActionRoleRecover() 
        :DupActionBase(dup_action_type_role_recover) {}

    int act(duplicate_entity_t *entity);
    int load_action_args(xmlNodePtr cur);
};

//helper functions
DupCondBase* create_cond_by_name(string cond_name);
DupActionBase* create_action_by_name(string action_name);
const char *get_cond_name(uint32_t cond_type);
const char *get_action_name(uint32_t action_type);

#endif
