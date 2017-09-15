#ifndef __CULTIVATE_EQUIP_CONF_H__
#define __CULTIVATE_EQUIP_CONF_H__

#include "common.h"

enum {
    CULTIVATE_TYPE_MOUNT = 1,
    CULTIVATE_TYPE_WING = 2,
};

struct step_prob_t {
    step_prob_t() {
        step = 0;
        prob = 0;
    }
    uint32_t step;
    uint32_t prob;
};

struct add_attr_value_t {
    add_attr_value_t() {
        type = 0;
        value = 0;
    }
    uint32_t type;
    uint32_t value;
};

struct cost_item_t {
    uint32_t item_id;
    uint32_t num;
};

struct cultivate_equip_level_info_t {
    cultivate_equip_level_info_t() {
        level = 0;
        need_cultivate_value = 0;
    }
    uint32_t level;
    uint32_t need_cultivate_value;
    std::vector<step_prob_t> step_probs_vec_;
    // <percent,prob>
    std::map<uint32_t, uint32_t> step_probs_map_;
    std::vector<add_attr_value_t> add_attrs_vec_;
    std::vector<cost_item_t> cost_item_vec_;
    uint32_t lv_reward_item;
};

struct cultivate_equip_conf_t {
    cultivate_equip_conf_t() {
        type = 0;
        init_item = 0;
        init_level = 0;
        max_level = 0;
        per_level_score = 0;
        default_buff_id = 0;
    }
    uint32_t type;
    uint32_t init_item;
    uint32_t init_level;
    uint32_t max_level;
    uint32_t per_level_score;
    uint32_t default_buff_id;
    uint32_t svip_trial_id;
    // <level, info>
    std::map<uint32_t, cultivate_equip_level_info_t> equip_info_map_;
};

//可养成装备管理器(坐骑,翅膀)
class cultivate_equip_conf_manager_t {
public:
    cultivate_equip_conf_manager_t() {
    }
    ~cultivate_equip_conf_manager_t() {
    }
public: //inline functions
    inline bool is_cultivate_equip_conf_exist(uint32_t type) {
        if (cultivate_equip_conf_map_.count(type) > 0) {
            return true;
        }
        return false;
    }
    inline bool add_cultivate_equip_conf(cultivate_equip_conf_t &cultivate_equip_conf) {
        if (is_cultivate_equip_conf_exist(cultivate_equip_conf.type)) {
            return false;
        }
        cultivate_equip_conf_map_[cultivate_equip_conf.type] = cultivate_equip_conf;
        return true;
    }
    inline cultivate_equip_conf_t *find_cultivate_equip_conf(uint32_t type) {
        if (cultivate_equip_conf_map_.count(type) == 0) {
            return NULL;
        }
        return &((cultivate_equip_conf_map_.find(type))->second);
    }
    inline void copy_from(const cultivate_equip_conf_manager_t &m) {
        cultivate_equip_conf_map_ = m.const_cultivate_equip_conf_map();
    }
    const inline std::map<uint32_t, cultivate_equip_conf_t>& const_cultivate_equip_conf_map() const{
        return cultivate_equip_conf_map_;
    }
private:
    std::map<uint32_t, cultivate_equip_conf_t> cultivate_equip_conf_map_;
};

#endif //__CULTIVATE_EQUIP_CONF_H__
