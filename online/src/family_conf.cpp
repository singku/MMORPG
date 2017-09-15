#include "family_conf.h"

bool family_conf_manager_t::add_lv_boss_info(
        uint32_t stage_id, uint32_t lv, family_dup_boss_conf_t &boss_conf) 
{
    std::map<uint32_t, player_lv_boss_info_map_t >::iterator iter = 
        stage_boss_info_map_.find(stage_id);
    if (iter == stage_boss_info_map_.end()) {
        player_lv_boss_info_map_t lv_boss_map;
        lv_boss_map.insert(
                std::pair<uint32_t, family_dup_boss_conf_t >(lv, boss_conf));
        stage_boss_info_map_.insert(
                std::pair<uint32_t, player_lv_boss_info_map_t>(stage_id, lv_boss_map));
    } else {
        if (iter->second.find(lv) == iter->second.end()) {
            iter->second.insert(
                    std::pair<uint32_t, family_dup_boss_conf_t >(lv, boss_conf));
        } else {
            ERROR_TLOG("family dup repeat player stage_id:%u, lv:%u", stage_id, lv);
            return false;
        }
    }

    return true;;
}



