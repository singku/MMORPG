#ifndef __SKILL_CONF_H__
#define __SKILL_CONF_H__

#include "common.h"

struct skill_conf_t {
    skill_conf_t() {
        skill_id = 0;
        is_parent = 0;
        skill_level = 0;
        learn_level = 0;
        cd = 0;
        sp = 0;
        normal_hurt_rate = 0;
        skill_hurt_rate = 0;
        hits = 0;
        hurt_rate.clear();
    }
    uint32_t skill_id; //技能id
    uint32_t is_parent; //是否父技能(不同等级的子技能参数不同)
    uint32_t skill_level; //技能等级
    uint32_t learn_level; //技能习得等级要求
    uint32_t cd; //技能cd
    uint32_t sp; //技能消耗魔力
    uint32_t normal_hurt_rate; //普通伤害率
    uint32_t skill_hurt_rate; //技功伤害率
    uint32_t hits; //有几段伤害
    std::vector<uint32_t> hurt_rate; //分段伤害率
};

//精灵配置管理器
class skill_conf_manager_t {
public:
    skill_conf_manager_t() {
        skill_conf_map_.clear();
    }
    ~skill_conf_manager_t() {
        skill_conf_map_.clear();
    }
public: //inline functions
    inline bool skill_conf_exist(uint32_t skill_id) {
        if (skill_conf_map_.count(skill_id) > 0) {
            return true;
        }
        return false;
    }
    inline bool add_skill_conf(skill_conf_t &skill_conf) {
        if (skill_conf_exist(skill_conf.skill_id)) {
            return false;
        }
        skill_conf_map_[skill_conf.skill_id] = skill_conf;
        return true;
    }
    const inline skill_conf_t *find_skill_conf(uint32_t skill_id) {
        if (skill_conf_map_.count(skill_id) == 0) {
            return NULL;
        }
        return &((skill_conf_map_.find(skill_id))->second);
    }
    inline skill_conf_t *find_mutable_skill_conf(uint32_t skill_id) {
        if (skill_conf_map_.count(skill_id) == 0) {
            return NULL;
        }
        return &((skill_conf_map_.find(skill_id))->second);
    }
    inline void copy_from(const skill_conf_manager_t &m) {
        skill_conf_map_ = m.const_skill_conf_map();
    }
    const inline std::map<uint32_t, skill_conf_t>& const_skill_conf_map() const{
        return skill_conf_map_;
    }
    inline std::map<uint32_t, skill_conf_t> &mutable_skill_conf_map() {
        return skill_conf_map_;
    }
    void remove_skill_conf(uint32_t skill_id) {
        skill_conf_map_.erase(skill_id);
    }
    void show() {
        FOREACH(skill_conf_map_, it) {
            const skill_conf_t &conf = it->second;
            TRACE_TLOG("Skill: id=%u parent=%u level=%u learn_level=%u cd=%u sp=%u nhurt_rate=%u shurt_rate=%u hits=%u hurt_rate_size=%u",
                    conf.skill_id, conf.is_parent, conf.skill_level, conf.learn_level, conf.cd, conf.sp, conf.normal_hurt_rate,
                    conf.skill_hurt_rate, conf.hits, conf.hurt_rate.size());
        }
    }
private:
    std::map<uint32_t, skill_conf_t> skill_conf_map_;
};

#endif
