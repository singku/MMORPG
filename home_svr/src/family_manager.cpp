#include "family_manager.h"

#include "family_manager.h"

FamilyManager *g_family_mgr;
FamilyHallManager *g_family_hall_mgr;

int FamilyHallManager::add_player(
        player_t *player, uint32_t family_id, const commonproto::map_player_data_t &mp)
{
    family_hall_map_iter_t iter = family_hall_info_map_.find(family_id);
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return 0;
    }

    uint32_t line_id = 0;
    if (iter == family_hall_info_map_.end()) {
        // 第一个进入家族大厅的玩家,新增大厅信息
       family_hall_info_t hall_info; 
       hall_info.max_line_id = get_init_line_id();
       family_hall_line_user_map_t line_user_map;
       family_hall_player_info_t p_info;
       //p_info.player_info.CopyFrom(mp);
       mp.SerializeToString(&p_info.player_info_str);
       line_user_map.insert(
               std::pair<uint64_t, family_hall_player_info_t>(player->role_key(), p_info));
       hall_info.line_info_map_.insert(
               std::pair<uint32_t, family_hall_line_user_map_t>(hall_info.max_line_id, line_user_map));
       family_hall_info_map_.insert(
               std::pair<uint32_t, family_hall_info_t>(family_id, hall_info));
       line_id = hall_info.max_line_id;
    } else {
        // 加入到空余大厅分组中
       family_hall_player_info_t p_info;
       family_hall_info_t &hall_info = iter->second;
       FOREACH(hall_info.line_info_map_, line_iter) {
           family_hall_line_user_map_t &line_user_map = line_iter->second;
           if (line_user_map.size() < 
                   commonproto::FAMILY_HALL_MAP_LINE_PLAYER_NUM) {
               //p_info.player_info.CopyFrom(mp);
               mp.SerializeToString(&p_info.player_info_str);
               line_user_map.insert(
                       std::pair<uint64_t, family_hall_player_info_t>(player->role_key(), p_info));
               line_id = line_iter->first;
               return line_id;
           }        
       }

       // 新增大厅分组
       hall_info.max_line_id++;
       //p_info.player_info.CopyFrom(mp);
       mp.SerializeToString(&p_info.player_info_str);
       family_hall_line_user_map_t line_user_map;
       line_user_map.insert(
               std::pair<uint64_t, family_hall_player_info_t>(player->role_key(), p_info));
       hall_info.line_info_map_.insert(
               std::pair<uint32_t, family_hall_line_user_map_t>(hall_info.max_line_id, line_user_map));
       line_id = hall_info.max_line_id;
    }

    return line_id;
}

int FamilyHallManager::remove_player(uint64_t role_key, uint32_t family_id)
{
    family_hall_map_iter_t iter = family_hall_info_map_.find(family_id);
    if (iter == family_hall_info_map_.end()) {
        return 0;
    }

    family_hall_info_t &hall_info = iter->second;
    FOREACH_NOINCR_ITER(hall_info.line_info_map_, it) {
        family_hall_line_user_map_t &line_user_map = it->second;
        line_user_map.erase(role_key);
        if (line_user_map.empty()) {
            // map类型删除节点不会对其他节点迭代器照成影响
            hall_info.line_info_map_.erase(it++);
        } else {
            it++;
        }
    }

    if (hall_info.line_info_map_.empty()) {
        family_hall_info_map_.erase(family_id);
    }

    return 0;
}
