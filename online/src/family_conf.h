#ifndef __FAMILY_CONF_H__
#define __FAMILY_CONF_H__

#include "common.h"

struct family_dup_boss_conf_t {
    uint32_t lv;
    uint32_t hp;
};

struct family_level_conf_t {
    family_level_conf_t() {
        /*level = 0;*/
        need_construct_value = 0;
        max_member_num = 0;
        max_vice_leader_num = 0;
    }
    /*uint32_t level;*/
    uint32_t need_construct_value;
    uint32_t max_member_num;
    uint32_t max_vice_leader_num;
};

/** 
 * @brief 家族技能
 */
struct family_tech_conf_t {
    family_tech_conf_t (){
        id = 0;
        attr_id = 0;
        base_value = 0;
        lv_attr = 0;
        coefficient = 0;
        req = 0;
    }
    uint32_t id;
    uint32_t attr_id;       // 加成属性id
    uint32_t base_value;    // 每级加成
    uint32_t lv_attr;       // 当前加成等级的记录属性
    uint32_t coefficient;   // 消耗贡献系数
    uint32_t req;           // 需要消耗贡献的基数
};

/** 
 * @brief 家族贡献
 */
struct family_contribute_conf_t {
    uint32_t type_id;       // commonproto::family_construct_type_t
    uint32_t req_add;       // 每次升级需要额外的消耗值，实际消耗值 = require + current_lv * req_add
    uint32_t contribution;  // 增加的贡献值
    uint32_t construct_value;           // 增加的建设值
    uint32_t cost_item;                 // 建设消耗物品id
    uint32_t base_cost_value;                // 建设消耗基础值
};

// <player_lv, info>
typedef std::map<uint32_t, family_dup_boss_conf_t> player_lv_boss_info_map_t;

class family_conf_manager_t {
    public:
        family_conf_manager_t () {
        };

        ~family_conf_manager_t () {
        }

        // 家族副本
        inline int add_stage_boss_info(
                uint32_t id, player_lv_boss_info_map_t &lv_boss_info) {
            if (stage_boss_info_map_.find(id) != stage_boss_info_map_.end()) {
                return 0;
            }

            stage_boss_info_map_.insert(
                    std::pair<uint32_t, player_lv_boss_info_map_t>(id, lv_boss_info));
            return 0;
        }
        bool add_lv_boss_info(
                uint32_t stage_id, uint32_t lv, family_dup_boss_conf_t &boss_conf);

        inline const family_dup_boss_conf_t *get_family_dup_boss_conf(
                uint32_t stage_id, uint32_t lv) {
            std::map<uint32_t, player_lv_boss_info_map_t >::iterator iter = 
                stage_boss_info_map_.find(stage_id); 
            if (iter != stage_boss_info_map_.end()) {
                std::map<uint32_t, family_dup_boss_conf_t>::iterator it = 
                    iter->second.find(lv);
                if (it != iter->second.end()) {
                    return &(it->second);
                } else {
                    // 没有对应的玩家等级配置，取缺省值
                    return &(iter->second[0]);
                }
            }
            return NULL;
        }

        const inline std::map<uint32_t, player_lv_boss_info_map_t > const_stage_boss_info_map() const {
            return stage_boss_info_map_;
        } 
        inline void copy_from_stage_config(const family_conf_manager_t &m) {
            stage_boss_info_map_ = m.const_stage_boss_info_map();
        }

        // 家族杂项配置
        inline bool add_common_config(uint32_t id, uint32_t value) {
            if (common_config_map_.find(id) != common_config_map_.end()) {
                return false;
            }

            common_config_map_.insert(std::pair<uint32_t, uint32_t>(id, value));
            return true;
        }

        inline uint32_t get_common_config(uint32_t id, uint32_t default_val) {
            std::map<uint32_t, uint32_t>::iterator iter = common_config_map_.find(id);
            if (iter == common_config_map_.end()) {
                return default_val;
            }
            return  iter->second;
        }

        const inline std::map<uint32_t, uint32_t > const_common_config_map() const {
            return common_config_map_;
        } 
        inline void copy_from_common_config(const family_conf_manager_t &m) {
            common_config_map_ = m.const_common_config_map();
        }

        // 家族等级
        inline bool add_level_config(uint32_t level, family_level_conf_t &level_conf) {
            if (level_config_map_.find(level) != level_config_map_.end()) {
                return false;
            }

            level_config_map_.insert(
                    std::pair<uint32_t, family_level_conf_t>(level, level_conf));
            return true;
        }

        inline const family_level_conf_t * get_level_config(uint32_t level) {
            std::map<uint32_t, family_level_conf_t>::iterator iter = 
                level_config_map_.find(level);
            if (iter == level_config_map_.end()) {
                return NULL;
            }
            return &(iter->second);
        }

        inline const std::map<uint32_t, family_level_conf_t > & get_level_conf_map() {
            return level_config_map_;
        }

        const inline std::map<uint32_t, family_level_conf_t > const_level_config_map() const {
            return level_config_map_;
        } 
        inline void copy_from_level_config(const family_conf_manager_t &m) {
            level_config_map_ = m.const_level_config_map();
        }

        // 家族技能
        inline bool add_tech_config(uint32_t tech_id, family_tech_conf_t &tech_conf) {
            if (tech_config_map_.find(tech_id) != tech_config_map_.end()) {
                return false;
            }

            tech_config_map_.insert(
                    std::pair<uint32_t, family_tech_conf_t>(tech_id, tech_conf));
            return true;
        }

        inline const family_tech_conf_t * get_tech_config(uint32_t tech_id) {
            std::map<uint32_t, family_tech_conf_t>::iterator iter = 
                tech_config_map_.find(tech_id);
            if (iter == tech_config_map_.end()) {
                return NULL;
            }
            return &(iter->second);
        }

        inline const std::map<uint32_t, family_tech_conf_t > & get_tech_conf_map() {
            return tech_config_map_;
        }

        const inline std::map<uint32_t, family_tech_conf_t > const_tech_config_map() const {
            return tech_config_map_;
        } 
        inline void copy_from_tech_config(const family_conf_manager_t &m) {
            tech_config_map_ = m.const_tech_config_map();
        }


        // 家族贡献(建设)
        inline bool add_contribute_config(uint32_t type_id, family_contribute_conf_t &contribute_conf) {
            if (contribute_config_map_.find(type_id) != contribute_config_map_.end()) {
                return false;
            }

            contribute_config_map_.insert(
                    std::pair<uint32_t, family_contribute_conf_t>(type_id, contribute_conf));
            return true;
        }

        inline const family_contribute_conf_t * get_contribute_config(uint32_t type_id) {
            std::map<uint32_t, family_contribute_conf_t>::iterator iter = 
                contribute_config_map_.find(type_id);
            if (iter == contribute_config_map_.end()) {
                return NULL;
            }
            return &(iter->second);
        }

        inline const std::map<uint32_t, family_contribute_conf_t > & get_contribute_conf_map() {
            return contribute_config_map_;
        }

        const inline std::map<uint32_t, family_contribute_conf_t > const_contribute_config_map() const {
            return contribute_config_map_;
        } 
        inline void copy_from_contribute_config(const family_conf_manager_t &m) {
            contribute_config_map_ = m.const_contribute_config_map();
        }

    private:
        // <stage_id, info>
        std::map<uint32_t, player_lv_boss_info_map_t > stage_boss_info_map_;

        // <id, info>
        std::map<uint32_t, uint32_t> common_config_map_;

        // <level, info> 
        std::map<uint32_t, family_level_conf_t> level_config_map_;

        // <id, info>
        std::map<uint32_t, family_tech_conf_t> tech_config_map_;

        // <id, info>
        std::map<uint32_t, family_contribute_conf_t> contribute_config_map_;
};

#endif //__FAMILY_CONF_H__
