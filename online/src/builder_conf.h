#ifndef __BUILDER_CONF_H__
#define __BUILDER_CONF_H__

#include "common.h"

struct builder_conf_t {
    uint32_t id;
    int32_t points;
    std::vector<uint32_t> prize_id_list;
    uint32_t hit_prize_id; //受击掉落奖励
    uint32_t hit_prize_count; //受击X次掉落奖励
};

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
    
    inline void show() {
        FOREACH(builder_conf_map_, it) {
            const builder_conf_t &bd = it->second;
            string prize;
            FOREACH(bd.prize_id_list, it2) {
                prize.append(Utils::to_string(*it2));
                prize.append(" ");
            }
            TRACE_TLOG("Builder:%u points:%u prize:%s", bd.id, bd.points, prize.c_str());
        }
    }
private:
    std::map<uint32_t, builder_conf_t> builder_conf_map_;
};

#endif
