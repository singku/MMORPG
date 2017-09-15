#ifndef __AFFIX_H__
#define __AFFIX_H__

#include "common.h"

struct affix_conf_t {
    affix_conf_t() {
        affix_id = 0;
        type = 0;
        buff_id = 0;
    }

    uint32_t affix_id;
    uint32_t type;
    uint32_t buff_id;
};

class affix_conf_manager_t {
public:
    affix_conf_manager_t() {
        affix_conf_map_.clear();
        affix_type_map_.clear();
    }
    
    ~affix_conf_manager_t() {
        affix_conf_map_.clear();
        affix_type_map_.clear();
    }

public:
    inline bool affix_conf_exist(uint32_t affix_id) {
        if (affix_conf_map_.count(affix_id) > 0) {
            return true;
        }
        return false;
    }

    inline bool affix_type_exist(uint32_t type) {
        if (affix_type_map_.count(type) > 0) {
            return true;
        }
        return false;
    }

    inline bool add_affix_conf(affix_conf_t &affix_conf) {
        if (affix_conf_exist(affix_conf.affix_id)) {
            return false;
        }
        affix_conf_map_[affix_conf.affix_id] = affix_conf;
        return true;
    }

    inline bool is_affix_type(uint32_t id) {
        if (id > 0 && id <= commonproto::MAX_AFFIX_TYPE) {
            return true;
        }

        return false;
    }

    const inline affix_conf_t *find_affix_conf(uint32_t affix_id) {
        if (affix_conf_map_.count(affix_id) == 0) {
            return NULL;
        }
        return &((affix_conf_map_.find(affix_id))->second);
    }

    bool init_type_map() {
        FOREACH(affix_conf_map_, iter) {
            const affix_conf_t *conf = &(iter->second);
            std::map<uint32_t, std::vector<affix_conf_t> >::iterator iter = 
                affix_type_map_.find(conf->type);
            if (iter == affix_type_map_.end()) {
                std::vector<affix_conf_t> affix_type_pool;
                affix_type_pool.push_back(*conf);
                affix_type_map_.insert(
                        std::pair<uint32_t, std::vector<affix_conf_t> >(
                            conf->type, affix_type_pool));
            } else {
                iter->second.push_back(*conf);
            }
        }

        return true;
    }

    /** 
     * @brief 从相同type中的词缀随机取一个
     * 
     * @param type 词缀类型 affix.xml中的type
     * 
     * @return 
     */
    uint32_t get_rand_affix_same_type(uint32_t type) {
        std::map<uint32_t, std::vector<affix_conf_t> >::iterator iter = 
                affix_type_map_.find(type);
        if (iter == affix_type_map_.end()) {
            return NULL;
        }

        std::vector<affix_conf_t> affix_conf_vec = iter->second;
        if (affix_conf_vec.size() == 0) {
            return NULL;
        }
        std::random_shuffle(affix_conf_vec.begin(), affix_conf_vec.end());
        return affix_conf_vec[0].affix_id;
    }

    const inline std::map<uint32_t, affix_conf_t>& const_affix_conf_map() const {
        return affix_conf_map_;
    }

    const std::map<uint32_t, std::vector<affix_conf_t> >& const_affix_type_map() const {
        return affix_type_map_;
    }

    inline void copy_from(const affix_conf_manager_t &m) {
        affix_conf_map_ = m.const_affix_conf_map();
        affix_type_map_ = m.const_affix_type_map();
    }

private:
    // <affix_id, config>
    std::map<uint32_t, affix_conf_t> affix_conf_map_;
    // <type, <affix_id> >
    std::map<uint32_t, std::vector<affix_conf_t> > affix_type_map_;
};

#endif
