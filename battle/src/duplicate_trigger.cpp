#include "player.h"
#include "pet_conf.h"
#include "xmlutils.h"
#include "duplicate_conf.h"
#include "duplicate_trigger.h"
#include "duplicate_entity.h"
#include "data_proto_utils.h"
#include "map_conf.h"
#include "builder_conf.h"
#include "global_data.h"
#include "timer_procs.h"
#include "affix_conf.h"

#define FLUSH_TYPE_SELF_CONTROL     (0) //全局自己决定
#define FLUSH_TYPE_TOTAL_CONTROL    (1) //全局控制刷怪
#define FLUSH_TYPE_TOTAL_RAND       (2) //全局随机刷怪
#define FLUSH_TYPE_SELF_CONTROLLED  (0) //子项由全局控制刷怪

char g_cond_name[dup_cond_type_max][32] = {
    "test", "CondIntoArea", "CondPhaseKill", "CondPhaseKillBoss", "CondAtPhase",
    "CondAtScene", "CondIntoScene", "CondStartScene", "CondStartPhase",
    "CondMonCountLow", "CondEndPhase", "CondEndScene", "CondAllReady",
    "CondAllPlayerDead", "CondReadyScene", "CondTimeUp", "CondMonsterKilled",
    "CondMonFlushTimerUp", "CondPhaseTimerUp", "CondBossHpLess", "CondPlayerHpLess",
    "CondFrontMonFlushReq", "CondUseSkill", "CondPlayerDead", "CondBossShowTimeUp",
    "CondActorBorn"
};

char g_action_name[dup_action_type_max][32] = {
    "test", "ActionStartDup", "ActionStartScene", "ActionStartPhase", "ActionActorBorn",
    "ActionEndPhase", "ActionEndScene", "ActionEndDup", "ActionReadyScene", "ActionRoleRecover"
};

const char *get_cond_name(uint32_t cond_type) 
{
    if (cond_type >= dup_cond_type_max) {
        return 0;
    }
    return g_cond_name[cond_type];
}

const char *get_action_name(uint32_t action_type)
{
    if (action_type >= dup_action_type_max) {
        return 0;
    }
    return g_action_name[action_type];
}

//NOTI(singku) 下面两个帮助函数可以用注册反射机制实现
//由于只是读取配置的时候需要调用 所以不会影响效率
DupCondBase* create_cond_by_name(string cond_name)
{
    if (cond_name == "CondIntoArea") {
        return new DupCondIntoArea();

    } else if (cond_name == "CondPhaseKillMonsterCount") {
        return new DupCondPhaseKillMonsterCount();

    } else if (cond_name == "CondPhaseKillBossMonster") {
        return new DupCondPhaseKillBossMonster();

    } else if (cond_name == "CondAtPhase") {
        return new DupCondAtPhase();

    } else if (cond_name == "CondAtScene") {
        return new DupCondAtScene();
    
    }
    else if (cond_name == "CondReadyScene") {
        return new DupCondReadyScene();
    
    } else if (cond_name == "CondIntoScene") {
        return new DupCondIntoScene();

    } else if (cond_name == "CondStartScene") {
        return new DupCondStartScene();

    } else if (cond_name == "CondStartPhase") {
        return new DupCondStartPhase();

    } else if (cond_name == "CondMonsterCountLow") {
        return new DupCondMonsterCountLow();

    } else if (cond_name == "CondEndPhase") {
        return new DupCondEndPhase();

    } else if (cond_name == "CondEndScene") {
        return new DupCondEndScene();

    } else if (cond_name == "CondAllReady") {
        return new DupCondAllReady();

    } else if (cond_name == "CondAllPlayerDead") {
        return new DupCondAllPlayerDead();
    
    } else if (cond_name == "CondPlayerDead") {
        return new DupCondPlayerDead();

    } else if (cond_name == "CondTimeUp") {
        return new DupCondTimeUp();

    } else if (cond_name == "CondMonsterKilled") {
        return new DupCondMonsterKilled();

    } else if (cond_name == "CondMonFlushTimerUp") {
        return new DupCondMonFlushTimerUp();

    } else if (cond_name == "CondPhaseTimerUp") {
        return new DupCondPhaseTimerUp();

    } else if (cond_name == "CondBossHpLess") {
        return new DupCondBossHpLess();

    } else if (cond_name == "CondPlayerHpLess") {
        return new DupCondPlayerHpLess();

    } else if (cond_name == "CondFrontMonFlushReq") {
        return new DupCondFrontMonFlushReq();

    } else if (cond_name == "CondUseSkill") {
        return new DupCondUseSkill();

    } else if (cond_name == "CondBossShowTimeUp") {
        return new DupCondBossShowTimeUp();

    } else if (cond_name == "CondActorBorn") {
        return new DupCondActorBorn();
    }

    return NULL;
}

DupActionBase* create_action_by_name(string action_name)
{
    if (action_name == "ActionStartDup") {
        return new DupActionStartDup();

    } else if (action_name == "ActionStartScene") {
        return new DupActionStartScene();

    } else if (action_name == "ActionStartPhase") {
        return new DupActionStartPhase();

    } else if (action_name == "ActionActorBorn") {
        return new DupActionActorBorn();

    } else if (action_name == "ActionEndPhase") {
        return new DupActionEndPhase();

    } else if (action_name == "ActionEndScene") {
        return new DupActionEndScene();

    } else if (action_name == "ActionEndDup") {
        return new DupActionEndDup();

    } else if (action_name == "ActionReadyScene") {
        return new DupActionReadyScene();

    } else if (action_name == "ActionRoleRecover") {
        return new DupActionRoleRecover();

    }

    return NULL;
}

/////////////////////////////////////////////////////
//------------member functions---------------------//
/////////////////////////////////////////////////////
bool DupCondBase::cond_fulfill(duplicate_entity_t *entity)
{
    return true;
}

bool DupCondBase::same_cond(DupCondBase *cond)
{
    if (cond->args.size() != this->args.size()) {
        return false;
    }
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] != cond->args[i]) {
            return false;
        }
    }
    return true;
}

bool DupCondBase::args_match(std::vector<uint32_t> &args)
{
    if (args.size() != this->args.size()) {
        return false;
    }
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] != this->args[i]) {
            return false;
        }
    }
    return true;
}

void DupCondBase::trig(duplicate_entity_t *entity)
{
    //一个条件的发生"可能"触发多个行为, 这里只是行为的某一个条件发生了
    FOREACH(listen_action, it1) {
        DupActionBase *action = *it1;
        if (action->actable(entity)) {
            action->act(entity);
        }
    }
}

//////////////////////////////////////////////////////
int DupCondBase::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32(arg, cur, "id");
    this->args.push_back(arg);

    bool id_is_map_id = false;
    switch (this->get_type()) {
    case dup_cond_type_at_scene:
    case dup_cond_type_into_scene:
    case dup_cond_type_start_scene:
    case dup_cond_type_end_scene:
        id_is_map_id = true;
        break;
    default:
        break;
    }
    if (id_is_map_id) {
        if (!g_map_conf_mgr.is_map_conf_exist(arg)) {
            ERROR_TLOG("Dup:%u Load Cond[%s] Failed: map_id_not_exist :%u",
                    dup.duplicate_id, get_cond_name(this->type), arg);
            return -1;
        }
        dup.map_ids.insert(arg);
    }
    return 0;
}


int DupActionBase::load_action_args(xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32(arg, cur, "id");
    this->args.push_back(arg);
    return 0;
}

int DupActionBase::load_action_cond(duplicate_t &dup, xmlNodePtr cur)
{
    cond_vec_t cond_vec[MAX_ACTION_CONDS];
    for (int i = 0; i < MAX_ACTION_CONDS; i++) {
        cond_vec[i].clear();
    }
    char tmp[MAX_ACTION_CONDS][16] = {"cond1", "cond2", "cond3", "cond4", "cond5", 
        "cond6", "cond7", "cond8", "cond9"
    };
    bool found;
    int i;
    xmlNodePtr child =  cur->xmlChildrenNode;
    while (child) {
        found = false;
        i = 0;
        for (; i < MAX_ACTION_CONDS; i++) {
            if (xmlStrEqual(child->name, (const xmlChar*)(tmp[i]))) {
                found = true;
                break;
            }
        }
        if (!found) {
            child = child->next;
            continue;
        }
        //根据type找到类名
        char cond_name[64] = {0};
        DECODE_XML_PROP_STR(cond_name, child, "type");
        DupCondBase *new_cond = create_cond_by_name(string(cond_name));
        if (!new_cond) {
            ERROR_TLOG("Alloc Cond[%s] Failed[dup:%u]", cond_name, dup.duplicate_id);
            return -1;
        }
        int ret = new_cond->load_cond_info(dup, child);
        if (ret) {
            return ret;
        }
        //判断有没有相同的cond
        DupCondBase *exist_cond = 0;
        bool cond_type_exist;
        if (dup.cond_map.count((uint32_t)(new_cond->get_type())) == 0) {
            exist_cond = 0;
            cond_type_exist = false;
        } else {
            cond_type_exist = true;
            std::vector<DupCondBase*> &cond_vec = 
                (dup.cond_map.find((uint32_t)(new_cond->get_type())))->second;
            FOREACH(cond_vec, it) {
                if (new_cond->same_cond(*it)) {
                    exist_cond = *it;
                    break;
                }
            }
        }

        //找到已存在的一样的cond
        if (exist_cond) { 
            delete new_cond;//删除刚创建的cond
            //将这个action加进exist_cond的监听列表
            exist_cond->listen_action.insert(this);
            //将这个cond加入本action的触发条件
            cond_vec[i].push_back(exist_cond);

        //新的cond且type存在
        } else if (cond_type_exist){
            //将这个action加进new_cond的监听列表
            new_cond->listen_action.insert(this);
            //将这个new_cond加入本action的触发条件
            cond_vec[i].push_back(new_cond);
            //将这个new_cond加入本dup的cond总表
            std::vector<DupCondBase*> &vec = 
                (dup.cond_map.find((uint32_t)(new_cond->get_type())))->second;
            vec.push_back(new_cond);

        //新的cond且没有同类型的cond
        } else if (!cond_type_exist) {
            //将本acton加入这个new_cond的监听列表
            new_cond->listen_action.insert(this);
            //将这个new_cond加入触发本action的条件
            cond_vec[i].push_back(new_cond);
            //将这个new_cond加入本dup的cond总表
            std::vector<DupCondBase*> vec;
            vec.push_back(new_cond);
            dup.cond_map[(uint32_t)(new_cond->get_type())] = vec;
        }

        child = child->next;
    }

    this->cond_group.clear();
    for (int i = 0; i < MAX_ACTION_CONDS; i++) {
        if (cond_vec[i].empty()) {
            continue;
        }
        this->cond_group.push_back(cond_vec[i]);
    }
    return 0;
}

int DupActionBase::load_action_info(duplicate_t &dup, xmlNodePtr cur)
{
    int ret = 0;
    //读取action的参数(下面这个也是虚函数 默认调用基类的,有意外的参数需要自己实现上面的类)
    ret = load_action_args(cur);
    if (ret) {
        return ret;
    }

    //读取action的刷怪的信息(只有刷怪的子类重载了这个虚函数，其他都是默认调用)
    ret = load_actor_info(dup, cur);
    if (ret) {
        return ret;
    }
    //读取cond
    ret = load_action_cond(dup, cur);
    if (ret) {
        return ret;
    }
    return 0;
}

bool DupActionBase::actable(duplicate_entity_t *entity)
{
    //一个行为可能有多组条件都支持
    bool actable = false;
    FOREACH(cond_group, it) {
        std::vector<DupCondBase*> &cond_vec = *it;
        actable = true;
        //一组条件中的条件全部满足才能触发这个行为
        FOREACH(cond_vec, it2) {
            DupCondBase *cond = *it2;
            if (!cond->cond_fulfill(entity)) {
                actable = false;//只要一个条件不满足则不能触发
                break;
            }
        }
        if (actable == true) {//某组条件满足即可触发
            break;
        }
    }
    return actable;
}

bool DupCondIntoArea::cond_fulfill(duplicate_entity_t *entity)
{
    return true;
}

int DupCondIntoArea::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t x, y, r;
    DECODE_XML_PROP_UINT32_DEFAULT(x, cur, "x", 0);
    this->args.push_back(x);
    DECODE_XML_PROP_UINT32_DEFAULT(y, cur, "y", 0);
    this->args.push_back(y);
    DECODE_XML_PROP_UINT32_DEFAULT(r, cur, "r", 0);
    this->args.push_back(r);

    if (x == 0 || y == 0) {
        ERROR_TLOG("Dup:%u CondIntoArea args invalid(x y must nonzero) x=%u, y=%u, r=%u",
                dup.duplicate_id, x, y, r);
        return -1;
    }
    return 0;
}

bool DupCondPhaseKillMonsterCount::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t count = this->args.size() == 0 ?0 :this->args[0];
    if (entity->cur_phase_dead_pet_num >= count) {
        return true;
    }
    return false;
}

int DupCondPhaseKillMonsterCount::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "count", 0);
    if (arg == 0) {
        ERROR_TLOG("Dup:%u CondPhaseKillMonsterCount args invalid 'count' not set", dup.duplicate_id);
        return -1;
    }
    this->args.push_back(arg);

    return 0;
}

bool DupCondPhaseKillBossMonster::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t boss_id = this->args.size() == 0 ?0 :this->args[0];

    if (entity->cur_map_dead_boss_set->count(boss_id)) {
        return true;
    }
    return false;
}

int DupCondPhaseKillBossMonster::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "id", 0xFFFFFFFF);
    if (arg == 0xFFFFFFFF) {
        ERROR_TLOG("Dup:%u CondPhaseKillBossMonster args invalid 'id' not set",
                dup.duplicate_id);
        return -1;
    }

    this->args.push_back(arg);
    return 0;
}

bool DupCondAtPhase::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t phase_id = this->args.size() == 0 ?0 :this->args[0];
    if (entity->cur_map_phase == phase_id) {
        return true;
    }
    return false;
}

int DupCondAtScene::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "id", 0);
    this->args.push_back(arg);
    return 0;
}

bool DupCondAtScene::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t map_id = this->args.size() == 0 ?0 :this->args[0];
    if (map_id == 0) {
        return true;
    }
    if (entity->cur_map_id == map_id) {
        return true;
    }
    return false;
}

bool DupCondReadyScene::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t map_id = this->args.size() == 0 ?0 :this->args[0];
    if (entity->ready_map_id == map_id) {
        return true;
    }
    return false;
}

bool DupCondIntoScene::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t map_id = this->args.size() == 0 ?0 :this->args[0];
    if (entity->ready_map_id == map_id) {
        return true;
    }
    return false;
}

bool DupCondStartScene::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t map_id = this->args.size() == 0 ?0 :this->args[0];
    if (entity->cur_map_id == map_id) {
        return true;
    }
    return false;
}

bool DupCondStartPhase::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t phase_id = this->args.size() == 0 ?0 :this->args[0];
    if (entity->cur_map_phase == phase_id) {
        return true;
    }
    return false;
}

bool DupCondMonsterCountLow::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t lower_bound = this->args.size() == 0 ?0 :this->args[0];
    if (entity->cur_map_enemy->size() < lower_bound) {
        return true;
    }
    return false;
}

int DupCondMonsterCountLow::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "count", 0);
    if (arg == 0) {
        ERROR_TLOG("Dup:%u CondMonsterCountLow args invalid 'count' not set");
        return -1;
    }

    this->args.push_back(arg);

    return 0;
}

bool DupCondEndPhase::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t phase_id = this->args.size() == 0 ?0 :this->args[0];
    if (entity->cur_map_phase == phase_id) {
        return true;
    }
    return false;
}

bool DupCondEndScene::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t map_id = this->args.size() == 0 ?0 :this->args[0];
    if (entity->cur_map_id == map_id) {
        return true;
    }
    return false;
}

bool DupCondAllReady::cond_fulfill(duplicate_entity_t *entity)
{
    FOREACH((*(entity->battle_players)), it) {
        std::set<player_t *> &p_set = it->second;
        FOREACH(p_set, it2) {
            player_t *player = *it2;
            if (player->player_dup_state != PLAYER_READY) {
                return false;
            }
        }
    }
    return true;
}

bool DupCondPlayerDead::cond_fulfill(duplicate_entity_t *entity)
{
    bool has_player = false;
    FOREACH((*(entity->battle_players)), it) {
        std::set<player_t *> &p_set = it->second;
        FOREACH(p_set, it2) {
            has_player = true;
            player_t *player = *it2;
            if (player->cur_hp == 0) {
                return true;
            }
        }
    }
    if (!has_player) {//player都没有了 肯定死了
        return true;
    }
    return false;
}

bool DupCondAllPlayerDead::cond_fulfill(duplicate_entity_t *entity)
{
    FOREACH((*(entity->battle_players)), it) {
        std::set<player_t *> &p_set = it->second;
        FOREACH(p_set, it2) {
            player_t *player = *it2;
            if (player->cur_hp != 0) {
                return false;
            }
        }
    }
    return true;
}

bool DupCondTimeUp::cond_fulfill(duplicate_entity_t *entity)
{
    return entity->time_up;
}

bool DupCondMonsterKilled::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t key = this->args[0];
    if (!key) {
        return (entity->cur_phase_dead_pet_num > 0);
    }

    if (key == entity->cur_dead_obj_key) {
        return true;
    }
    return false;
}

int DupCondMonsterKilled::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "unique_key", 0);
    this->args.push_back(arg);
    return 0;
}

bool DupCondBossHpLess::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t percent = this->args.size() == 0 ?0 :this->args[0];
    FOREACH((*(entity->cur_map_enemy)), it) {
        const duplicate_map_pet_t &dpet = it->second;
        if (dpet.mon_type != MON_TYPE_BOSS) {
            continue;
        }
        if (dpet.cur_hp * 100 / dpet.max_hp < percent) {
            return true;
        }
    }
    return false;
}

int DupCondBossHpLess::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "percent", 0);
    if (arg == 0 || arg >= 100) {
        ERROR_TLOG("Dup:%u CondBossHpLess args invalid 'percent'(%u) not (0-100)",
                dup.duplicate_id, arg);
        return -1;
    }

    this->args.push_back(arg);

    return 0;
}

bool DupCondPlayerHpLess::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t percent = this->args.size() == 0 ?0 :this->args[0];
    FOREACH((*(entity->battle_players)), itp) {
        std::set<player_t*> &p_set = itp->second;
        FOREACH(p_set, it) {
            player_t *player = *it;
            if (player->cur_hp * 100 / player->max_hp < percent) {
                return true;
            }
        }
    }

    return false;
}

int DupCondPlayerHpLess::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "percent", 0);
    if (arg == 0 || arg >= 100) {
        ERROR_TLOG("Dup:%u CondPlayerHpLess args invalid 'percent' not (0-100)");
        return -1;
    }

    this->args.push_back(arg);

    return 0;
}

int DupCondFrontMonFlushReq::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "index", 0);
    this->args.push_back(arg);

    return 0;
}

int DupCondUseSkill::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "skill_id", 0);
    this->args.push_back(arg);

    return 0;
}

int DupCondActorBorn::load_cond_info(duplicate_t &dup, xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "unique_key", 0);
    this->args.push_back(arg);

    return 0;
}

bool DupCondActorBorn::cond_fulfill(duplicate_entity_t *entity)
{
    uint32_t unique_key = this->args.size() == 0 ?0: this->args[1];
    if (unique_key == 0) {//任意怪刷出
        if (entity->cur_phase_born_obj_cnt == 0) {
            return false;
        } else {
            return true;
        }
    }

    FOREACH((*(entity->cur_map_enemy)), it) {
        if (it->second.uniq_key == unique_key) {
            return true;
        }
    }
    FOREACH((*(entity->cur_map_non_enemy)), it) {
        if (it->second.uniq_key == unique_key) {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
//-------------------------行为定义-----------------------------------------///
///////////////////////////////////////////////////////////////////////////////
int DupActionStartDup::act(duplicate_entity_t *entity)
{
    //NOTI(singku) 外部触发 由进入副本地图的协议触发
    DEBUG_TLOG("P:%u, ACTION: start dup:%u", 
            dup_entity_mgr_t::get_entity_one_uid(entity), this->args[0]);

    //准备开始阶段 做初始化工作
    entity->state = DUPLICATE_STATE_WAIT_PLAYER_READY;

    duplicate_battle_type_t type = g_duplicate_conf_mgr.get_duplicate_type(entity->dup_id);

    if (type == DUP_BTL_TYPE_PVEP) { //玩家打AI玩家
        entity->state = DUPLICATE_STATE_START;
        
    } else if (type == DUP_BTL_TYPE_PPVE || type == DUP_BTL_TYPE_RPVP) {//真人组队
        entity->force_ready_timer = ADD_TIMER_EVENT_EX(entity,
                kTimerTypeForceReadyTimer,
                (void*)(entity->dup_id),
                NOW() + 10);

    } else if (type == DUP_BTL_TYPE_PEVE){//玩家及AI玩家组队
		//设置ai队友为准备状态
		FOREACH((*(entity->battle_players)), it) {
			std::set<player_t*> &p_set = it->second;
			FOREACH(p_set, it2) {
				player_t *player = *it2;
				if(player->is_artifacial){
					player->player_dup_state = PLAYER_READY;
				}
			}
		}
		duplicate_entity_trig(entity, (uint32_t)dup_cond_type_all_ready);
	}


    return 0;
}

int DupActionStartScene::act(duplicate_entity_t *entity)
{
    //NOTI(singku) 外部触发 由玩家准备好的协议触发
    DEBUG_TLOG("P:%u, ACTION: start scene:%u", 
            dup_entity_mgr_t::get_entity_one_uid(entity), this->args[0]);
    entity->state = DUPLICATE_STATE_START;
    entity->cur_map_id = this->args[0];
    //发通知包可以开始战斗了
    onlineproto::sc_0x0207_duplicate_notify_battle_start noti_msg;
    relay_notify_msg_to_entity_except(entity, cli_cmd_cs_0x0207_duplicate_notify_battle_start, noti_msg);
    FOREACH((*(entity->battle_players)), it) {
        std::set<player_t*> &p_set = it->second;
        FOREACH(p_set, it2) {
            (*it2)->player_dup_state = PLAYER_PLAY;
        }
    }
    entity->start = 1;
    if (entity->force_ready_timer) {
        REMOVE_TIMER(entity->force_ready_timer);
        entity->force_ready_timer = 0;
    }
    //抛出start_scene条件
    std::vector<uint32_t> params;
    params.push_back(this->args[0]);
    duplicate_entity_trig(entity, (uint32_t)dup_cond_type_start_scene, &params);

    return 0;
}

int DupActionStartPhase::load_action_args(xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32(arg, cur, "id");
    this->args.push_back(arg);

    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "duration", 0);
    this->args.push_back(arg);

    return 0;
}

int DupActionStartPhase::act(duplicate_entity_t *entity)
{
    //NOTI(singku) 内部触发 由startscene条件触发
    DEBUG_TLOG("P:%u, ACTION: start phase:%u", 
            dup_entity_mgr_t::get_entity_one_uid(entity), this->args[0]);
    entity->cur_map_phase = this->args[0];

    if (entity->cur_map_phase == 1) {//副本开始第一个阶段的时候开始倒计时
        //如果副本有时间限制 开启倒计时功能
        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(entity->dup_id);
        if (dup->time_limit) {
            entity->dup_time_limit_timer = ADD_TIMER_EVENT_EX(entity,
                    kTimerTypeDupCountDownTimer,
                    (void*)(entity->dup_id),
                    NOW() + dup->time_limit);
        }
    }

    //抛出start_phase条件
    std::vector<uint32_t> params;
    params.push_back(this->args[0]);

    //到下一阶段的通知包
    onlineproto::sc_0x0213_duplicate_notify_to_next_phase noti_msg;
    noti_msg.set_new_phase(entity->cur_map_phase);
    noti_msg.set_duration(this->args[1]);
    relay_notify_msg_to_entity_except(entity, 
            cli_cmd_cs_0x0213_duplicate_notify_to_next_phase, noti_msg);

    if (entity->phase_timer) {
        REMOVE_TIMER(entity->phase_timer);
        entity->phase_timer = NULL;
    }

    if (this->args[1]) {//有阶段的时长限制
        DEBUG_TLOG("P:%u, ACTION: add phase timer(%u sec)",
                dup_entity_mgr_t::get_entity_one_uid(entity), this->args[1]);
        entity->phase_timer = ADD_TIMER_EVENT_EX(entity,
                kTimerTypeDupPhaseTimer,
                (void*)(entity->dup_id),
                NOW() + this->args[1]);
    }

    //entity->born_area_born_record->clear();
    //entity->born_area_dead_record->clear();
    duplicate_entity_trig(entity, (uint32_t)dup_cond_type_start_phase, &params);
    return 0;
}

int DupActionActorBorn::act(duplicate_entity_t *entity)
{
    uint32_t action_flush_method = this->args[0];
    uint32_t flush_duration = this->args[1];
    uint32_t screen_max = this->args[2];
    uint32_t action_flush_max = this->args[3];
    uint32_t idx = 0;
    std::set<uint32_t> flush_idx_set;

    if (this->born_areas.size() == 0) {
        return 0;
    }
    if (screen_max && entity->cur_phase_enemy_num >= screen_max) {
        //指定了屏幕最大怪 且屏幕上的怪够 则不刷
        return 0;
    }
    if (action_flush_max && entity->cur_phase_born_obj_cnt >= action_flush_max) {
        return 0;
    }

    if (flush_duration) {
        entity->flush_mon_timer = ADD_TIMER_EVENT_EX(entity,
                kTimerTypeDupMonFlushTimer,
                (void*)(entity->dup_id),
                NOW() + flush_duration);
    }
    uint32_t boss_show_time = 0;
    const duplicate_t *dup_conf = g_duplicate_conf_mgr.find_duplicate(entity->dup_id);
    if (dup_conf) {
        boss_show_time = dup_conf->boss_show_time_limit;
    } 

    //如果是全局随机刷 则建立可随机列表
    std::vector<uint32_t> tmp_flush_idx_vec;
    if (action_flush_method == FLUSH_TYPE_TOTAL_RAND) {
        FOREACH((this->born_areas), it) {
            const duplicate_born_area_t &dba = it->second;
            idx = it->first;
            if (dba.flush_max && (*(entity->born_area_born_record))[idx] >= dba.flush_max) {
                continue;
            }
            tmp_flush_idx_vec.push_back(idx);
        }
    }

    FOREACH((this->born_areas), it) {
        const duplicate_born_area_t &dba = it->second;
        idx = it->first;
        if (entity->born_area_born_record->count(idx) == 0) {
            //这个点还没刷怪记录 初始化记录
            entity->born_area_born_record->insert(make_pair(idx, 0));
        }

        if (dba.flush_method != FLUSH_TYPE_SELF_CONTROLLED) {//不受全局控制
            flush_idx_set.insert(idx);
            continue;
        }

        //受全局控制的情况下
        if (action_flush_method == FLUSH_TYPE_SELF_CONTROL) {//全局出生点自己决定刷怪
            flush_idx_set.insert(idx);

        } else if (action_flush_method == FLUSH_TYPE_TOTAL_CONTROL) { //全局刷一波怪
            if (entity->cur_phase_enemy_num == 0) {//敌人全死了才刷
                flush_idx_set.insert(idx);
            }
        }
    }

    if (action_flush_method == FLUSH_TYPE_TOTAL_RAND) {//全局随机刷怪
        if (tmp_flush_idx_vec.size()) {
            uint32_t hit = ranged_random(0, tmp_flush_idx_vec.size() - 1);
            uint32_t hit_idx = tmp_flush_idx_vec[hit];
            flush_idx_set.insert(hit_idx);
        }
    }

    uint32_t dup_req_power = g_duplicate_conf_mgr.get_dup_req_power(entity->dup_id);
    //对每一个flush_idx_set集合中的出怪点 尝试刷怪

    //NOTI(singku) 内部触发 由startphase及monsterkill触发
    DEBUG_TLOG("P:%u, ACTION: actor born", 
            dup_entity_mgr_t::get_entity_one_uid(entity));

#define CREATE_OBJ(mon_type) \
    duplicate_map_pet_t dpet; \
    do { \
        dpet.type = actor.type; \
        dpet.team = actor.team; \
        dpet.no_stat = actor.no_stat; \
        dpet.pet_id = conf->id; \
        dpet.pet_level = conf->level; \
        dpet.create_tm = ++(entity->life_counter); \
        dpet.mon_type = mon_type; \
        dpet.patrol_paths = actor.patrol_paths; \
        dpet.born_effect = actor.born_effect; \
        dpet.born_action = actor.born_action; \
        dpet.float_height = actor.float_height; \
        dpet.around_type = entity->skill_affect_obj_type; \
        dpet.around_create_tm = entity->skill_affect_obj_create_tm; \
        dpet.born_action_args = actor.born_action_args; \
        dpet.ai_start_delay = actor.ai_start_delay; \
        dpet.uniq_key = actor.uniq_key; \
        dpet.dynamic_params = actor.dynamic_params; \
        dpet.heading = actor.heading; \
        dpet.life_time = actor.life_time; \
        dpet.born_area_idx = area_idx;\
        dpet.phase = entity->cur_map_phase;\
        Pet pet; \
        pet.init(dpet.pet_id, dpet.pet_level, dpet.create_tm); \
        pet.calc_battle_value(); \
        dpet.cur_hp = pet.hp(); \
        dpet.max_hp = pet.hp(); \
        init_pet_dynamic(actor, entity, dpet); \
        dpet.pos_x = (int)dba.born_x + (ranged_random(-(dba.born_radius), dba.born_radius)); \
        dpet.pos_y = (int)dba.born_y + (ranged_random(-(dba.born_radius), dba.born_radius)); \
        dpet.around_radius = dba.born_radius; \
        dpet.req_power = pet.req_power() ?pet.req_power() :dup_req_power; \
        entity->cur_phase_born_obj_cnt++; \
    } while(0)

#define CREATE_PET(mon_type) \
    do { \
        CREATE_OBJ(mon_type); \
        dpet.is_pet = 1; \
        new_pets.push_back(dpet); \
        /*新刷出来的怪加到玩家的对战怪物列表中*/ \
        DEBUG_TLOG("P:%u, DUP_STAT: Mon_born:%u born_area_idx:%u, id:%u no_stat:%u, lv:%u, hp:%u, maxhp:%u", \
                dup_entity_mgr_t::get_entity_one_uid(entity), dpet.create_tm, area_idx, dpet.pet_id, dpet.no_stat, \
                dpet.pet_level, dpet.cur_hp, dpet.max_hp);\
        if (dpet.team == DUP_ACTOR_TEAM_ENEMY) {\
            (*(entity->cur_map_enemy))[dpet.create_tm] = dpet; \
            entity->cur_phase_enemy_num++;\
        } else {\
            (*(entity->cur_map_non_enemy))[dpet.create_tm] = dpet; \
        }\
    } while(0)

#define CREATE_BUILDER(mon_type) \
    do { \
        CREATE_OBJ(mon_type); \
        dpet.is_pet = 0; \
        new_builders.push_back(dpet); \
        /*新刷出来的怪加到玩家的对战怪物列表中*/ \
        DEBUG_TLOG("P:%u, DUP_STAT: Builder_born:%u, born_area_idx:%u, id:%u no_stat:%u",\
               dup_entity_mgr_t::get_entity_one_uid(entity),  dpet.create_tm, area_idx, dpet.pet_id, dpet.no_stat);\
        if (dpet.team == DUP_ACTOR_TEAM_ENEMY) {\
            (*(entity->cur_map_enemy))[dpet.create_tm] = dpet; \
            entity->cur_phase_enemy_num++;\
        } else {\
            (*(entity->cur_map_non_enemy))[dpet.create_tm] = dpet; \
        }\
    } while(0)

    std::vector<duplicate_map_pet_t> new_pets;
    new_pets.clear();
    std::vector<duplicate_map_pet_t> new_builders;
    new_builders.clear();

    bool boss_born = false;
    FOREACH(flush_idx_set, it) {
        uint32_t area_idx = *it;
        const duplicate_born_area_t &dba = this->born_areas[area_idx];
        if (dba.born_actors.size() == 0) {
            continue;//没有可刷的
        }
        if (dba.flush_max && (*(entity->born_area_born_record))[area_idx] >= dba.flush_max) {
            continue;//已经刷完了
        }

        if (!dba.succeed && (*(entity->born_area_born_record))[area_idx] 
            > (*(entity->born_area_dead_record))[area_idx]) {
            continue; //不可连续出且出生点还没死
        }
        if (dba.born_rate < 10000 && ranged_random(0, 10000) > (int)dba.born_rate) {
            continue; //出生点概率刷怪不满足
        }
        uint32_t size = dba.born_actors.size();
        uint32_t actor_idx = 0;
        if (dba.flush_method == 0 || dba.flush_method == 1) {
            //随机
            actor_idx = ranged_random(0, size-1);
        } else {
            //顺序
            if (entity->born_area_born_record->count(area_idx) == 0) {
                actor_idx = 0;
            } else {
                actor_idx = (*(entity->born_area_born_record))[area_idx];
            }
        }
        if (actor_idx >= size) {
            WARN_TLOG("Dup:%u Area:%u Born Actor actor_idx:%u >= size:%u",
                    entity->dup_id, area_idx+1, actor_idx, size);
            continue;
        }
        const duplicate_actor_t &actor = dba.born_actors[actor_idx];
        uint32_t mon_type = 0;
        if (actor.type == DUP_ACTOR_TYPE_PET) {
            uint32_t pet_id = get_pet_id_by_actor_idx(entity->dup_id, actor.id);
            const pet_conf_t *conf = g_pet_conf_mgr.find_pet_conf(pet_id);
            if (!conf) {
                WARN_TLOG("Dup:%u Area:%u Born Actor idx:%u is not valid pet",
                        entity->dup_id, area_idx+1, actor_idx);
                continue;
            }
            if (conf->mon_type == MON_TYPE_BOSS) {
                entity->cur_phase_boss_out++;
                boss_born = true;
            }
            mon_type = conf->mon_type;
            CREATE_PET(mon_type);
        } else if (actor.type == DUP_ACTOR_TYPE_BUILDER) {
            uint32_t builder_id = get_builder_id_by_actor_idx(entity->dup_id, actor.id);
            const builder_conf_t *conf = g_builder_conf_mgr.find_builder_conf(builder_id);
            if (!conf) {
                WARN_TLOG("Dup:%u Area:%u Born Actor idx:%u is not valid builder",
                        entity->dup_id, area_idx+1, actor_idx);
                continue;
            }
            mon_type = conf->mon_type;
            if (conf->mon_type == MON_TYPE_BOSS) {
                entity->cur_phase_boss_out++;
                boss_born = true;
            }
            CREATE_BUILDER(mon_type);
        }
        uint32_t cnt = (*(entity->born_area_born_record))[area_idx];
        (*(entity->born_area_born_record))[area_idx] = cnt+1;
    }
    
    if (boss_born && boss_show_time) {
        if (entity->boss_show_timer) {//以新出的boss作为定时器标准
            REMOVE_TIMER(entity->boss_show_timer);
        }
        DEBUG_TLOG("P:%u, AddBossShowTimer", dup_entity_mgr_t::get_entity_one_uid(entity));
        entity->boss_show_timer = ADD_TIMER_EVENT_EX(entity,
                kTimerTypeBossSHowTimer,
                (void*)(entity->dup_id),
                NOW() + boss_show_time + 3);
    }

    onlineproto::sc_0x020C_duplicate_notify_monster_born noti_new_msg;
    //打包新出生的怪
	TRACE_TLOG("zjun0723,before_pack_mon");
    DataProtoUtils::pack_duplicate_map_born_pet_info(new_pets, noti_new_msg.mutable_monsters());
	TRACE_TLOG("zjun0723,after_pack_mon");

    //打包新出生的阻挡
    DataProtoUtils::pack_duplicate_map_born_pet_info(new_builders, noti_new_msg.mutable_builders());

    if (entity->start == 1) {
        //通知包entity中的玩家有新的怪物出生
        relay_notify_msg_to_entity_except(entity, cli_cmd_cs_0x020C_duplicate_notify_monster_born,
                noti_new_msg);
    }
    //抛出怪物出生的条件
    duplicate_entity_trig(entity, dup_cond_type_actor_born);
    return 0;
}

/** 
 * @brief 动态生成boss属性
 * 
 * @param entity 
 * @param dpet 
 * 
 * @return 
 */
int DupActionActorBorn::init_pet_dynamic(
        const duplicate_actor_t &actor, 
        duplicate_entity_t *entity, duplicate_map_pet_t &dpet)
{
    if (entity == NULL) {
        return 0;
    }

    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(entity->dup_id);
    if (dup == NULL) {
        return 0;
    }

    // 家族副本boss属性适配
    if (dup->mode == onlineproto::DUP_MODE_TYPE_FAMILY) {
        // 取玩家属性
        std::map<uint32_t, std::set<player_t*> >::iterator p_set = 
            entity->battle_players->find(SIDE1);
        FOREACH(p_set->second, iter) {
            if ((*iter)->family_dup_boss_lv > 0) {
                dpet.pet_level = (*iter)->family_dup_boss_lv;
                dpet.cur_hp = (*iter)->family_dup_boss_hp;
                dpet.max_hp = (*iter)->family_dup_boss_maxhp;
            }
            break;
        }
    } else if (dup->mode == onlineproto::DUP_MODE_TYPE_BLESS_PET){
		uint32_t mean_lv = get_players_mean_level_by_side(entity, SIDE1);
		dpet.pet_level = mean_lv;
	} else if (dup->mode == onlineproto::DUP_MODE_TYPE_STAR_PET) {
		if (entity->battle_players->count(SIDE1)) {
			std::map<uint32_t, std::set<player_t*> >::iterator p_set = 
				entity->battle_players->find(SIDE1);
			FOREACH(p_set->second, iter) {
				dpet.pet_level = (*iter)->level;
				break;
			}
		} else {
			ERROR_TLOG("Start Act Born Pet,But Can Not Found SIDE1 Player");
			TRACE_TLOG("Start Act Born Pet,But Can Not Found SIDE1 Player");
			DEBUG_TLOG("Start Act Born Pet,But Can Not Found SIDE1 Player");
		}
	} else if (dup->mode == onlineproto::DUP_MODE_TYPE_CHALLENGE_DEMON) {
		std::map<uint32_t, std::set<player_t*> >::iterator p_set = 
			entity->battle_players->find(SIDE1);
		FOREACH(p_set->second, iter) {
			dpet.pet_level = (*iter)->level + dup->add_fight_level;
			break;
		}
	}

    // 生成词缀
    dpet.affix_list.clear();
    const pet_conf_t *conf = g_pet_conf_mgr.find_pet_conf(dpet.pet_id);
	if (conf) {
		FOREACH(conf->affix_list_pool, iter) {
			if (iter->size() > 0) {
				std::vector<uint32_t> affix_id_vec = *iter;
				std::random_shuffle(affix_id_vec.begin(), affix_id_vec.end());
				uint32_t affix_id = affix_id_vec[0];
				if (g_affix_conf_mgr.is_affix_type(affix_id)) {
					uint32_t id = g_affix_conf_mgr.get_rand_affix_same_type(affix_id);
					if (!g_affix_conf_mgr.affix_conf_exist(id)) {
						continue;
					}
					affix_id = id;
				}            

				if (affix_id) {
					dpet.affix_list.insert(affix_id);
				}
			}
		}
	}

    return 0;
}


int DupActionActorBorn::load_action_args(xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "flush_method", 0);
    this->args.push_back(arg);
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "flush_duration", 0);
    this->args.push_back(arg);
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "screen_max", 0);
    this->args.push_back(arg);
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "flush_max", 0);
    this->args.push_back(arg);
    return 0;
}

int DupActionActorBorn::load_actor_info(duplicate_t &dup, xmlNodePtr cur)
{
    xmlNodePtr child = cur->xmlChildrenNode;
    while (child) {
        if (!xmlStrEqual(child->name, (const xmlChar*)("born_area"))) {
            child = child->next;
            continue;
        }
        duplicate_born_area_t born_area;
        born_area.g_area_idx = ++g_born_area_idx;
        DECODE_XML_PROP_UINT32(born_area.born_x, child, "x");
        DECODE_XML_PROP_UINT32(born_area.born_y, child, "y");
        DECODE_XML_PROP_UINT32_DEFAULT(born_area.born_radius, child, "r", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(born_area.flush_method, child, "flush_method", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(born_area.flush_max, child, "flush_max", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(born_area.succeed, child, "succeed", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(born_area.born_rate, child, "rate", 10000);

        xmlNodePtr actor_child = child->xmlChildrenNode;
        while (actor_child) {
            if (!xmlStrEqual(actor_child->name, (const xmlChar*)("actor"))) {
                actor_child = actor_child->next;
                continue;
            }
            duplicate_actor_t dup_actor;
            uint32_t type;
            uint32_t team;
            DECODE_XML_PROP_UINT32_DEFAULT(type, actor_child, "type", 0);
            dup_actor.type = (duplicate_actor_type_t)type;
            DECODE_XML_PROP_UINT32_DEFAULT(team, actor_child, "team", 0);
            dup_actor.team = (duplicate_actor_team_t)team;
            DECODE_XML_PROP_UINT32_DEFAULT(dup_actor.no_stat, actor_child, "no_stat", 0);

            DECODE_XML_PROP_UINT32(dup_actor.id, actor_child, "id");
            char patrol_path_str[256] = {0};
            DECODE_XML_PROP_STR_DEFAULT(patrol_path_str, actor_child, "patrol_paths", "");
            std::vector<std::string> path_list = split(patrol_path_str, ',');
            FOREACH(path_list, it) {
                uint32_t dot = atoi((*it).c_str());
                dup_actor.patrol_paths.push_back(dot);
            }
            DECODE_XML_PROP_UINT32_DEFAULT(dup_actor.born_effect, actor_child, "born_effect", 0);
            DECODE_XML_PROP_UINT32_DEFAULT(dup_actor.born_action, actor_child, "born_action", 0);
            DECODE_XML_PROP_UINT32_DEFAULT(dup_actor.float_height, actor_child, "float_height", 0);

            char born_action_args[256] = {0};
            DECODE_XML_PROP_STR_DEFAULT(born_action_args, actor_child, "born_action_args", "");
            dup_actor.born_action_args.assign(born_action_args, strlen(born_action_args) + 1);

            char dynamic_params[256] = {0};
            DECODE_XML_PROP_STR_DEFAULT(dynamic_params, actor_child, "dynamic_param", "");
            dup_actor.dynamic_params.assign(dynamic_params, strlen(dynamic_params) + 1);

            DECODE_XML_PROP_UINT32_DEFAULT(dup_actor.heading, actor_child, "heading", 0);
            dup_actor.heading *= 1000;

            DECODE_XML_PROP_UINT32_DEFAULT(dup_actor.uniq_key, actor_child, "unique_key", 0);
            DECODE_XML_PROP_UINT32_DEFAULT(dup_actor.ai_start_delay, actor_child, "ai_start_delay", 0);
            DECODE_XML_PROP_UINT32_DEFAULT(dup_actor.life_time, actor_child, "life_time", 0);

            if (team != (uint32_t)DUP_ACTOR_TEAM_ENEMY 
                && team != (uint32_t)DUP_ACTOR_TEAM_FRIEND
                && team != (uint32_t)DUP_ACTOR_TEAM_NON_COUNT) {
                ERROR_TLOG("Load action born actor team of dup:%u, team:%u invalid",
                        dup.duplicate_id, team);
                return -1;
            }
            if (dup_actor.type == DUP_ACTOR_TYPE_PET) {
                //actor.id是索引id
                if (dup_actor.id > dup.mon_vec.size()) {
                    ERROR_TLOG("Load action born actor mon index of dup:%u, id:%u[max=%u] invalid",
                            dup.duplicate_id, dup_actor.id, dup.mon_vec.size());
                    return -1;
                }
            } else if (dup_actor.type == DUP_ACTOR_TYPE_BUILDER) {
                if (dup_actor.id > dup.builder_vec.size()) {
                    ERROR_TLOG("Load action born actor builder index of dup:%u, id:%u[max=%u] invalid",
                            dup.duplicate_id, dup_actor.id, dup.builder_vec.size());
                    return -1;
                }
#if 0
            } else if (dupactor.type == DUP_ACTOR_TYPE_PLAYER) {
                //判断角色是否存在
            } else if (dupactor.type == DUP_ACTOR_TYPE_NPC) {
                //判断NPC是否存在
#endif
            } else {
                ERROR_TLOG("Load action born actor of dup:%u, invalid type:%u",
                        dup.duplicate_id, type);
                return -1;
            }
            born_area.born_actors.push_back(dup_actor);

            actor_child = actor_child->next;
        }
        (this->born_areas)[born_area.g_area_idx] = born_area;
        child = child->next;
    }
    return 0;
}

int DupActionEndPhase::act(duplicate_entity_t *entity)
{
    //NOTI(singku) 内部触发 由打怪协议杀死怪后判断杀怪数量后kill_monster_count条件触发
    DEBUG_TLOG("P:%u, ACTION: end phase:%u", 
            dup_entity_mgr_t::get_entity_one_uid(entity), entity->cur_map_phase);

    //结束当前阶段
    entity->cur_phase_dead_pet_num = 0;
    entity->cur_phase_enemy_num = 0;
    entity->cur_phase_born_obj_cnt = 0;
    entity->born_area_born_record->clear();
    entity->born_area_dead_record->clear();
    entity->cur_phase_boss_out = 0;

    //删除刷怪定时器
    if (entity->flush_mon_timer) {
        REMOVE_TIMER(entity->flush_mon_timer);
        entity->flush_mon_timer = NULL;
    }
    //删除阶段定时器
    if (entity->phase_timer) {
        REMOVE_TIMER(entity->phase_timer);
        entity->phase_timer = NULL;
    }

    //抛出end_phase_条件
    std::vector<uint32_t> params;
    params.push_back(entity->cur_map_phase);
    duplicate_entity_trig(entity, (uint32_t)dup_cond_type_end_phase, &params);
    return 0;
}

int DupActionEndScene::act(duplicate_entity_t *entity)
{
    //NOTI(singku) 内部触发 由最后一个阶段结束end_phase 触发
    DEBUG_TLOG("P:%u, ACTION: end scene:%u", 
            dup_entity_mgr_t::get_entity_one_uid(entity), entity->cur_map_id);

    //当前场景进入完成场景集合。
    entity->fini_map_id->insert(entity->cur_map_id);
    entity->cur_map_dead_boss_set->clear();
    //场景结束时 发battle_end的通知包
    onlineproto::sc_0x020D_duplicate_notify_battle_end noti_msg;
    uint32_t win_side = duplicate_entity_win_side(entity);
    FOREACH((*(entity->battle_players)), it) {
        std::set<player_t *> &p_set = it->second;
        FOREACH(p_set, it2) {
            if ((uint32_t)(*it2)->side == win_side) {
                noti_msg.set_win(true);
            } else {
                noti_msg.set_win(false);
            }
            (*it2)->player_dup_state = PLAYER_LEAVE;

            noti_msg.set_scene_id(entity->cur_map_id);
            relay_notify_msg_to_player(*it2, cli_cmd_cs_0x020D_duplicate_notify_battle_end, noti_msg);
        }
    }
    //抛出end_scene_条件
    std::vector<uint32_t> params;
    params.push_back(entity->cur_map_id);
    entity->cur_map_enemy->clear();
    entity->cur_map_non_enemy->clear();

    duplicate_entity_trig(entity, (uint32_t)dup_cond_type_end_scene, &params);
    entity->cur_map_id = 0;

    return 0;
}

int DupActionEndDup::act(duplicate_entity_t *entity)
{
    //NOTI(singku) 内部外部都可以触发 当场景结束或者所有玩家都离开副本时触发
    uint32_t win = this->args[1];
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(entity->dup_id);
    //通知所有玩家
    battleproto::sc_battle_duplicate_notify_end noti_msg;
    uint32_t win_side = duplicate_entity_win_side(entity);
    FOREACH((*(entity->battle_players)), it) {
        std::set<player_t *> &p_set = it->second;
        FOREACH(p_set, it2) {
            uint32_t my_win;
            if (dup->battle_type == DUP_BTL_TYPE_RPVP
                || dup->battle_type == DUP_BTL_TYPE_PVEP) {
                if ((uint32_t)(*it2)->side == win_side) {
                    my_win = 1;
                } else {
                    my_win = 0;
                }
            } else {
                my_win = win;
            }
            noti_msg.set_win(my_win ?true :false);
            DEBUG_TLOG("P:%u, ACTION: end dup:%u win:%u", (*it2)->uid, entity->dup_id, my_win);
            send_msg_to_player(*it2, btl_cmd_notify_end, noti_msg);
        }
    }

    //置副本可以结束
    entity->state = DUPLICATE_STATE_CAN_END;
    entity->cur_map_enemy->clear();
    entity->cur_map_non_enemy->clear();

    //设置副本自毁定时器
    entity->destroy_dup_timer = ADD_TIMER_EVENT_EX(entity,
            kTimerTypeDupDestroyTimer,
            (void*)(entity->dup_id),
            NOW() + 1);
    return 0;
}

int DupActionEndDup::load_action_args(xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32(arg, cur, "id");
    this->args.push_back(arg);
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "win", 0);
    this->args.push_back(arg);
    return 0;
}

int DupActionReadyScene::act(duplicate_entity_t *entity)
{
    //NOTI(singku) 内部触发 结束场景后触发 end_scene
    DEBUG_TLOG("P:%u, ACTION: ready scene:%u", 
            dup_entity_mgr_t::get_entity_one_uid(entity), this->args[0]);

    entity->ready_map_id = this->args[0];
    entity->state = DUPLICATE_STATE_WAIT_PLAYER_READY;
    duplicate_battle_type_t type = g_duplicate_conf_mgr.get_duplicate_type(entity->dup_id);
    if (type == DUP_BTL_TYPE_PPVE || type == DUP_BTL_TYPE_RPVP) {//真人组队
        if (!entity->force_ready_timer) {
            entity->force_ready_timer = ADD_TIMER_EVENT_EX(entity,
                    kTimerTypeForceReadyTimer,
                    (void*)(entity->dup_id),
                    NOW() + 10);
        }
    }
    return 0;
}

int DupActionRoleRecover::act(duplicate_entity_t *entity)
{
    DEBUG_TLOG("P:%u, ACTION: role recover", dup_entity_mgr_t::get_entity_one_uid(entity));
    uint32_t type = this->args[0];
    uint32_t hp_percent = this->args[1];
    uint32_t tp_percent = this->args[2];

    onlineproto::sc_0x022A_duplicate_notify_role_recover noti_msg;
    noti_msg.set_type(type);

    if (type == 0 || type == 1) { //玩家回||精灵回
        FOREACH((*(entity->battle_players)), it) {
            const std::set<player_t *> &p_set = it->second;
            FOREACH(p_set, itp) {
                player_t *dest = *itp;
                noti_msg.set_id(dest->uid);
                if (type == 0) {
                    uint32_t new_hp = dest->max_hp * hp_percent / 100;
                    if (dest->cur_hp < new_hp) {
                        dest->cur_hp = new_hp;
                    }
                    uint32_t new_tp = dest->max_tp * tp_percent / 100;
                    if (dest->cur_tp < new_tp) {
                        dest->cur_tp = new_tp;
                    }
                    noti_msg.set_create_tm(dest->create_tm);
                    noti_msg.set_hp(dest->cur_hp);
                    noti_msg.set_tp(dest->cur_tp);
                    //relay_notify_msg_to_entity_except(entity,
                            //cli_cmd_cs_0x022A_duplicate_notify_role_recover,
                            //noti_msg, 0);

                    // TODO toby 分线test
                    relay_notify_msg_to_entity_line_except(entity, dest->line_id,
                            cli_cmd_cs_0x022A_duplicate_notify_role_recover, noti_msg, 0);

                } else if (type == 1) {
                    FOREACH((dest->fight_pets), itm) {
                        Pet &pet = itm->second;
                        uint32_t new_hp = pet.max_hp() * hp_percent / 100;
                        if (pet.hp() < (int)new_hp) {
                            pet.set_hp(new_hp);
                        }
                        noti_msg.set_create_tm(pet.create_tm());
                        noti_msg.set_hp(pet.hp());
                        //relay_notify_msg_to_entity_except(entity,
                                //cli_cmd_cs_0x022A_duplicate_notify_role_recover,
                                //noti_msg, 0);

                        // TODO toby 分线test
                        relay_notify_msg_to_entity_line_except(entity, dest->line_id,
                                cli_cmd_cs_0x022A_duplicate_notify_role_recover, noti_msg, 0);
                    }
                }
            }
        }
 
    } else if (type == 2) {//所有boss回
        FOREACH((*(entity->cur_map_enemy)), it) {
            duplicate_map_pet_t &dpet = it->second;
            if (dpet.mon_type != MON_TYPE_BOSS) {
                continue;
            }
            uint32_t new_hp = dpet.max_hp * hp_percent / 100;
            if (dpet.cur_hp < new_hp) {
                dpet.cur_hp = new_hp;
            }
            noti_msg.set_id(dpet.pet_id);
            noti_msg.set_create_tm(dpet.create_tm);
            noti_msg.set_hp(dpet.cur_hp);
            //relay_notify_msg_to_entity_except(entity,
                    //cli_cmd_cs_0x022A_duplicate_notify_role_recover,
                    //noti_msg, 0);

            // TODO toby 分线test
            relay_notify_msg_to_entity_line_except(entity, 0,
                    cli_cmd_cs_0x022A_duplicate_notify_role_recover, noti_msg, 0);
        }
    }
    return 0;
}

int DupActionRoleRecover::load_action_args(xmlNodePtr cur)
{
    uint32_t arg;
    DECODE_XML_PROP_UINT32(arg, cur, "role");
    this->args.push_back(arg);
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "hp_percent", 0);
    this->args.push_back(arg);
    DECODE_XML_PROP_UINT32_DEFAULT(arg, cur, "tp_percent", 0);
    this->args.push_back(arg);

    return 0;
}


