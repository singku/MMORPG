#ifndef __EXCHANGE_H__
#define __EXCHANGE_H__

#include "common.h"

struct exchange_item_config_t {
    uint32_t id;
    uint32_t count;
    uint32_t rate;
    uint32_t expire_time;
};

struct exchange_attr_config_t {
    uint32_t id;
    uint32_t count;
    uint32_t rate;
};

struct exchange_pet_config_t {
    uint32_t id;
    uint32_t level;
    uint32_t rate;
};

struct exchange_in_config_t {
    std::vector<exchange_item_config_t> item_list;
    std::vector<exchange_attr_config_t> attr_list;
};

struct exchange_out_config_t {
    std::vector<exchange_item_config_t> item_list;
    std::vector<exchange_attr_config_t> attr_list;
    std::vector<exchange_pet_config_t> pet_list;
};

struct exchange_config_t
{
    uint32_t id;
    exchange_in_config_t in;
    exchange_out_config_t out;
    uint32_t forever_key;
    uint32_t daily_key;
    uint32_t weekly_key;
    uint32_t monthly_key;
    uint32_t rand_mode; //0 不随机 1: N选1 m: N选m
    uint32_t must_vip; //是否vip才能兑换这个东东
    uint32_t addiction; // 0 不防沉迷 1 防沉迷
    std::string msg_dir;
    std::string msg_name; //
    std::string msg_sub_name; //
    std::string desc;
};

class exchange_conf_manager_t {
public:
    exchange_conf_manager_t() {
        clear();
    }
    ~exchange_conf_manager_t() {
        clear();
    }
    void clear() {
        exchg_map_.clear();
    }
public:
    inline bool exchg_exist(uint32_t exchg_id) {
        return (exchg_map_.count(exchg_id) > 0);
    }
    const inline exchange_config_t *get_exchg(uint32_t exchg_id) {
        if (!exchg_exist(exchg_id)) {
            return 0;
        }
        return &(exchg_map_.find(exchg_id)->second);
    }
    inline bool add_exchg(const exchange_config_t &exchg) {
        if (exchg_exist(exchg.id)) {
            return false;
        }
        exchg_map_[exchg.id] = exchg;
        return true;
    }
    const inline std::map<uint32_t, exchange_config_t> &const_exchg_map() {
        return exchg_map_;
    }
    inline void copy_from(exchange_conf_manager_t &emgr) {
        exchg_map_ = emgr.const_exchg_map();
    }
    inline void show() {
        FOREACH(exchg_map_, it) {
            const exchange_config_t &exchg = it->second;
            TRACE_TLOG("#exchange:%u rand_mode:%u must_vip:%u", 
                    it->first, exchg.rand_mode, exchg.must_vip);
            FOREACH(exchg.in.item_list, it) {
                const exchange_item_config_t &item = *it;
                TRACE_TLOG("\t In Item id:%u count:%u rate:%u", item.id, item.count, item.rate);
            }
            FOREACH(exchg.in.attr_list, it) {
                const exchange_attr_config_t &attr = *it;
                TRACE_TLOG("\t In Attr id:%u count:%u rate:%u", attr.id, attr.count, attr.rate);
            }
            FOREACH(exchg.out.item_list, it) {
                const exchange_item_config_t &item = *it;
                TRACE_TLOG("\t Out Item id:%u count:%u rate:%u", item.id, item.count, item.rate);
            }
            FOREACH(exchg.out.attr_list, it) {
                const exchange_attr_config_t &attr = *it;
                TRACE_TLOG("\t Out Attr id:%u count:%u rate:%u", attr.id, attr.count, attr.rate);
            }
            FOREACH(exchg.out.pet_list, it) {
                const exchange_pet_config_t &pet = *it;
                TRACE_TLOG("\t Out Pet id:%u count:%u rate:%u", pet.id, pet.level, pet.rate);
            }
        }
    }
private:
    std::map<uint32_t, exchange_config_t> exchg_map_;
};
#endif
