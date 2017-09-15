#ifndef __MAP_CONF_H__
#define __MAP_CONF_H__

#include "common.h"

struct map_conf_t {
    uint32_t id; // 地图id
};

class map_conf_manager_t {
public:
    map_conf_manager_t() {
        clear();
    }
    ~map_conf_manager_t() {
        clear();
    }
    inline void clear() {
        map_conf_map_.clear();
    }
    inline const std::map<uint32_t, map_conf_t> &const_map_conf_map() const {
        return map_conf_map_;
    }
    inline void copy_from(const map_conf_manager_t &m) {
        map_conf_map_ = m.const_map_conf_map();
    }
    inline bool is_map_conf_exist(uint32_t map_id) {
        if (map_conf_map_.count(map_id) > 0) return true;
        return false;
    }
    inline bool add_map_conf(const map_conf_t &map) {
        if (is_map_conf_exist(map.id)) return false;
        map_conf_map_[map.id] = map; return true;
    }
    inline const map_conf_t *find_map_conf(uint32_t map_id) {
        if (!is_map_conf_exist(map_id)) return 0;
        return &((map_conf_map_.find(map_id))->second);
    }

private:
    std::map<uint32_t, map_conf_t> map_conf_map_;
};

#endif //__MAP_CONF_H__

