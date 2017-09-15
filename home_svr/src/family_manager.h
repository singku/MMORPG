#ifndef __FAMILY_MANAGER_H__
#define __FAMILY_MANAGER_H__

#include "common.h"
#include "player.h"

typedef struct family_hall_player_info {
    // 如果使用Protobuf定义结构不能重载
    /*commonproto::map_player_data_t player_info;*/
    std::string player_info_str;
}family_hall_player_info_t;

// <role_key, info>
typedef std::map<uint64_t , family_hall_player_info_t > family_hall_line_user_map_t;
typedef std::map<uint64_t , family_hall_player_info_t >::iterator family_hall_line_user_iter_t;

// <lineid, info>
typedef std::map<uint32_t , family_hall_line_user_map_t > family_hall_line_map_t;
typedef std::map<uint32_t , family_hall_line_user_map_t >::iterator family_hall_line_iter_t;

typedef struct family_hall_info {
    uint32_t max_line_id;
    family_hall_line_map_t line_info_map_;
}family_hall_info_t;

// <family_id,info>
typedef std::map<uint32_t, family_hall_info_t > family_hall_map_t;
typedef std::map<uint32_t, family_hall_info_t >::iterator family_hall_map_iter_t;

class FamilyHallManager
{
    public:
        inline family_hall_map_t* get_family_hall_info() {
            return &family_hall_info_map_;
        }

        inline uint32_t get_init_line_id(){
            return 1;
        }

        inline uint32_t get_player_line_id(uint64_t role_key, uint32_t family_id){
            return 1;
        }

        /** 
         * @brief 取家族大厅中同一个分组的玩家列表
         * 
         * @param player 
         * @param family_id 
         * @param line_id 
         * @param player_set 
         * 
         * @return 
         */
        inline uint32_t get_line_players(
                player_t *player, uint32_t family_id, uint32_t line_id, 
                std::set<std::string> &player_set) {
            family_hall_map_iter_t iter = family_hall_info_map_.find(family_id);
            if (iter == family_hall_info_map_.end()) {
                return 0;
            }

            family_hall_line_iter_t line_iter = iter->second.line_info_map_.find(line_id);
            if (line_iter == iter->second.line_info_map_.end()) {
                return 0;
            }

            FOREACH(line_iter->second, user_iter) {
                player_set.insert(user_iter->second.player_info_str);
            }

            return 0;
        }

        inline uint32_t update_player_info(
                uint32_t family_id, uint32_t line_id, uint64_t role_key,
                const commonproto::map_player_data_t &player_info) {
            family_hall_map_iter_t iter = family_hall_info_map_.find(family_id);
            if (iter == family_hall_info_map_.end()) {
                return 0;
            }

            family_hall_line_iter_t line_iter = iter->second.line_info_map_.find(line_id);
            if (line_iter == iter->second.line_info_map_.end()) {
                return 0;
            }

            FOREACH(line_iter->second, user_iter) {
                if (user_iter->first == role_key) {
                    player_info.SerializeToString(&user_iter->second.player_info_str);
                }
            }

            return 0;
        }

 

        /** 
         * @brief 取家族大厅中的所有玩家列表
         * 
         * @param player 
         * @param player_set 
         * 
         * @return 
         */
        inline uint32_t get_family_hall_all_players(
                player_t *player, uint32_t family_id,
                std::set<std::string> &player_set) {
            family_hall_map_iter_t iter = family_hall_info_map_.find(family_id);
            if (iter == family_hall_info_map_.end()) {
                return 0;
            }

            FOREACH(iter->second.line_info_map_, line_iter) {
                FOREACH(line_iter->second, user_iter) {
                    player_set.insert(user_iter->second.player_info_str);
                }
            }

            return 0;
        }

        int add_player(
            player_t *player, uint32_t family_id, const commonproto::map_player_data_t &mp);

        int remove_player(uint64_t role_key, uint32_t family_id);

    private:
        family_hall_map_t family_hall_info_map_;
};

class FamilyManager
{
    public:
        inline FamilyHallManager *get_family_hall_manager() {
            return &hall_manager_;
        }

    private:
        FamilyHallManager hall_manager_;
};

class FamilyUtils
{
    public:
        static bool is_valid_family_id(uint32_t family_id)
        {
            if (family_id >= commonproto::FAMILY_ID_START) {
                return true;
            }       

            return false;
        }
};


extern FamilyManager *g_family_mgr;
extern FamilyHallManager *g_family_hall_mgr;
#endif //__FAMILY_MANAGER_H__
