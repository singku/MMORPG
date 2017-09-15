#ifndef __BUILDER_CONF_H__
#define __BUILDER_CONF_H__

#include "common.h"
#include "pet_conf.h"

//建筑物的配表几乎和精灵一样
struct builder_conf_t {
    builder_conf_t() {
        clear();
    }
    void clear() {
        id = 0; sex = 0; level = 1; mon_type = 0;
        elem_type = kPetElemTypeWater; growth_type = kPetGrowType1;
        memset(basic_normal_battle_values, 0, sizeof(basic_normal_battle_values));
        memset(basic_normal_battle_values_grow, 0, sizeof(basic_normal_battle_values_grow));
        memset(basic_hide_battle_values, 0, sizeof(basic_hide_battle_values));
        memset(basic_hide_battle_values_grow, 0, sizeof(basic_hide_battle_values_grow));
        is_level_add = 1; skill_ids.clear();
        req_power = 0;
        max_dp = 0;
    }
    uint32_t id; //精灵ID
    uint32_t sex; //性别
    uint32_t level; //野怪会有等级
    uint32_t mon_type; //野怪会配是否boss

    pet_elem_type_t elem_type; //元素属性: 水、火...
    pet_grow_type_t growth_type; //成长类型
    uint32_t basic_normal_battle_values[kMaxBattleValueTypeNum]; // 基础战斗属性
    uint32_t basic_normal_battle_values_grow[kMaxBattleValueTypeNum]; //基础战斗属性的等级成长率
    uint32_t basic_hide_battle_values[kMaxBattleValueHideTypeNum]; //基础的隐藏战斗属性值
    uint32_t basic_hide_battle_values_grow[kMaxBattleValueHideTypeNum]; //基础的隐藏战斗属性按等级的成长率
    uint32_t is_level_add; //是否按等级成长
    std::vector<uint32_t> skill_ids;
    uint32_t req_power;
    uint32_t max_dp;
};

//精灵配置管理器
class builder_conf_manager_t {
public:
    builder_conf_manager_t() {
        builder_conf_map_.clear();
    }
    ~builder_conf_manager_t() {
        builder_conf_map_.clear();
    }
public: //inline functions
    inline bool builder_conf_exist(uint32_t builder_id) {
        if (builder_conf_map_.count(builder_id) > 0) {
            return true;
        }
        return false;
    }
    inline bool add_builder_conf(builder_conf_t &builder_conf) {
        if (builder_conf_exist(builder_conf.id)) {
            return false;
        }
        builder_conf_map_[builder_conf.id] = builder_conf;
        return true;
    }
    const inline builder_conf_t *find_builder_conf(uint32_t builder_id) {
        if (builder_conf_map_.count(builder_id) == 0) {
            return NULL;
        }
        return &((builder_conf_map_.find(builder_id))->second);
    }
    inline void copy_from(const builder_conf_manager_t &m) {
        builder_conf_map_ = m.const_builder_conf_map();
    }
    const inline std::map<uint32_t, builder_conf_t>& const_builder_conf_map() const{
        return builder_conf_map_;
    }
private:
    std::map<uint32_t, builder_conf_t> builder_conf_map_;
};

extern builder_conf_manager_t g_builder_conf_mgr;

inline pet_elem_type_t get_builder_elem_type(uint32_t builder_id) 
{
    const builder_conf_t *builder_conf = g_builder_conf_mgr.find_builder_conf(builder_id);
    if (!builder_conf) {
        return kPetElemTypeWater;
    }
    return builder_conf->elem_type;
}

#endif //__BUILDER_CONF_H__
