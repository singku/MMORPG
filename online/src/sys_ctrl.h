#ifndef __SYS_CTRL_H__
#define __SYS_CTRL_H__

#include "common.h"

enum module_type_t {
    module_type_activity_open = 1, //活动是否开启
    module_type_sys_opt = 2, //系统优化
    module_type_sys_ctrl = 3, //系统控制
    module_type_daily_shop = 4, //每日商店
    module_type_world_boss = 5, //世界boss
	module_type_gold_vip_reset = 6, //年费vip重置(黄金勋章180天) 
    module_type_change_clothes_plan = 7, //换装计划
    module_type_evil_knife_legend = 8, //妖刀传奇任务
    //新加的模块依次放在下面 也可以根据配表直接调hardcode调用
};

struct module_conf_t {
    uint32_t module_id;
    //<key, val>
    map<string, string> conf_map;
};

class module_manager_t {
public:
    module_manager_t() {
        clear();
    }
    ~module_manager_t() {
        clear();
    }
public:
    inline const std::map<uint32_t, module_conf_t> &const_module_map() const {
        return module_map_;
    }
    inline void copy_from(const module_manager_t &m) {
        module_map_ = m.const_module_map();
    }
    inline void clear() {
        module_map_.clear();
    }
    inline bool is_module_exist(uint32_t module_id) {
        if (module_map_.count(module_id) > 0) return true;
        return false;
    }
    inline bool add_module_conf(module_conf_t &m_conf) {
        if (is_module_exist(m_conf.module_id)) return false;
        module_map_[m_conf.module_id] = m_conf; return true;
    }
    inline const module_conf_t *find_module(uint32_t module_id) {
        if (is_module_exist(module_id) == false) return 0;
        return &((module_map_.find(module_id))->second);
    }
    inline bool get_module_conf_uint32(uint32_t module_id, const string &key, uint32_t &val) {
        const module_conf_t *m = find_module(module_id);
        if (!m) return false;
        if (m->conf_map.count(key) == 0) return false;
        const string &val_ = (m->conf_map.find(key))->second;
        val = atoi(val_.c_str());
        return true;
    }
    inline bool get_module_conf_string(uint32_t module_id, const string &key, string &val) {
        const module_conf_t *m = find_module(module_id);
        if (!m) return false;
        if (m->conf_map.count(key) == 0) return false;
        val = (m->conf_map.find(key))->second; 
        return true;
    }
    inline uint32_t get_module_conf_uint32_def(uint32_t module_id, const string &key, uint32_t def) {
        uint32_t val = 0;
        bool ret = get_module_conf_uint32(module_id, key, val);
        if (ret == false) return def;
        return val;
    }
    inline string get_module_conf_string_def(uint32_t module_id, const string &key, const string &def) {
        string val_;
        val_.clear();
        bool ret = get_module_conf_string(module_id, key, val_);
        if (ret == false) return def;
        return val_;
    }

    inline bool is_activity_open(const string &key)
    {
        uint32_t val = 0;
        this->get_module_conf_uint32(5, key, val);
        if (val) {
            return true;
        } else {
            return false;
        }
    }

private:
    std::map<uint32_t, module_conf_t> module_map_;
};

#endif
