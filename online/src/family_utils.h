#ifndef __FAMILY_UTILS_H__
#define __FAMILY_UTILS_H__
#include "common.h"
#include "data_proto_utils.h"

class FamilyUtils {
public:
    static bool is_valid_family_id(uint32_t family_id);
    static bool is_valid_family_rank_type(uint32_t type);
    static bool is_valid_family_title(uint32_t title);

    static int update_family_info(
        player_t *p, const dbproto::family_info_table_t &info, uint32_t flag);
    static int update_family_member_info(
        player_t *p, const dbproto::family_member_table_t &info, uint32_t flag);

    static int compute_family_level(
        const uint32_t construct_value, uint32_t &level);

    static int update_family_rank_score(
        const uint32_t family_id, 
        const commonproto::family_info_t &family_info);

    static int del_family_rank_info(uint32_t family_id);

    static int member_join_family(
       player_t *player,
       const commonproto::family_info_t &family_info,
       const commonproto::family_member_info_t &member_info);
 
    static int member_leave_family(
       player_t *player,
       const uint32_t family_id, 
       const commonproto::family_member_info_t &member_info);

    static bool is_family_member_full(
            const commonproto::family_info_t &family_info, uint32_t type);


    static bool is_valid_family_level(uint32_t level);

    static int insert_family_log(
        uint32_t family_id, uint32_t type, 
        std::vector<commonproto::family_log_para_t> &paras);

    static int leader_reassign(
        player_t *player,
        uint32_t family_id, uint32_t old_leader_id, uint32_t old_create_tm, 
        uint32_t new_leader_id, uint32_t new_create_tm);

    static int dismiss_family(player_t *player, uint32_t family_id);
    static int clear_self_family_info(player_t *player);

    static int update_family_match_info(const commonproto::family_info_t &info);
    static int change_family_match_info(uint32_t family_id, int member_num);

    static int del_match_info(uint32_t family_id);
    
    static int family_lock_release(player_t *player, std::string &lock_key_str);

    static bool is_family_out_of_date(commonproto::family_info_t &family_info);

	static int send_family_msg_notice( player_t *player,
			uint32_t reciever, uint32_t create_tm, commonproto::family_msg_type_t type, 
			std::string family_name, uint32_t family_id);

    static int send_offline_family_msg_notice(
        player_t *player,
        const dbproto::sc_get_login_info &login_info);

    static int family_attr_addition(
            player_t *player, std::map<uint32_t, uint32_t> &attr_map);

    static int get_family_level_addition(uint32_t level);

    static int family_del_event(
            uint32_t family_id, uint32_t userid, uint32_t create_tm, uint32_t type);

    static bool is_valid_family_dup_stage_id(uint32_t stage_id);

    static int refresh_family_member_info(player_t *player);
};

#endif


