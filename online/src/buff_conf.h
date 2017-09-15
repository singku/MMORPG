#ifndef __SERVER_BUFF__
#define __SERVER_BUFF__

#include "common.h"

enum buff_effect_type_t {
    server_buff_effect_none         = 0,
    server_buff_effect_exp_multi    = 1,
    server_buff_effect_gold_multi   = 2,
    server_buff_effect_max,
};

class buff_effect_t {
public:
    buff_effect_t() {
        effect_type = server_buff_effect_none;
        args.clear();
    }
    buff_effect_type_t effect_type;
    std::vector<int32_t> args;
};

struct buff_conf_t {
public:
    bool has_effect_type(buff_effect_type_t type) {
        return (effects.count((uint32_t)type) > 0);
    }
    uint32_t buff_id;
    //<effect_type, effect>
    std::map<uint32_t, buff_effect_t> effects;
    uint32_t over_type_id;
};

class buff_conf_mgr_t {
public:
    buff_conf_mgr_t() {
        clear();
        init_effect_name_table();
    }
    ~buff_conf_mgr_t() {
        clear();
    }
    void clear() {
        buff_conf_map_.clear();
        effect_name_table_.clear();
    }

    void init_effect_name_table() {
        effect_name_table_["_exp_mul"] = 1;
        effect_name_table_["_gold_mul"] = 2;
    }

public:
    inline const std::map<uint32_t, buff_conf_t> &const_buff_conf_map() const {
        return buff_conf_map_;
    }

    inline void copy_from(const buff_conf_mgr_t &m) {
        buff_conf_map_ = m.const_buff_conf_map();
    }

    bool is_buff_conf_exist(uint32_t buff_id) {
        if (buff_conf_map_.count(buff_id) > 0) return true;
        return false;
    }

    inline bool add_buff_conf(const buff_conf_t &buff) {
        if (is_buff_conf_exist(buff.buff_id)) return false;
        buff_conf_map_[buff.buff_id] = buff; return true;
    }

    inline const buff_conf_t *find_buff_conf(uint32_t buff_id) {
        if (!is_buff_conf_exist(buff_id)) return 0;
        return &((buff_conf_map_.find(buff_id))->second);
    }

    inline bool is_server_effect_name(const char *name) {
        string tmp(name);
        if (effect_name_table_.count(tmp) > 0) {
            return true;
        }
        return false;
    }

    inline bool is_server_effect_type(uint32_t type) {
        return (type > server_buff_effect_none && type < server_buff_effect_max);
    }

    buff_effect_type_t get_effect_type_by_name(const char *name) {
        if (!is_server_effect_name(name)) {
            return server_buff_effect_none;
        }
        string tmp(name);
        return (buff_effect_type_t)((effect_name_table_.find(tmp))->second);
    }

private:
    std::map<uint32_t, buff_conf_t> buff_conf_map_;
    std::map<string, uint32_t> effect_name_table_;
};

#endif
