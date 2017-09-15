#ifndef __MINE_UTILS_H__
#define __MINE_UTILS_H__
#include <stdint.h>
#include <common.h>
#include "mine.h"

class Pet;
class MineUtils {
public:
	/*根据玩家战力的比例范围获得矿的信息
	*/
	static uint32_t req_dbsvr_to_search_mine(player_t* player,
			std::vector<uint64_t>& match_ids);

	static uint32_t req_redis_to_get_teams_info(player_t* player,
			std::vector<uint64_t>& mine_id_vec);
	
	static uint32_t pack_mine_team_info(player_t* player,
			const std::vector<uint32_t>& pet_create_tm,
			commonproto::mine_team_info_t* pb_team_ptr);

	static uint32_t ex_pack_prize_elem_info_for_cli(
			const commonproto::mine_info_t& pb_mine,
			commonproto::mine_info_t* pb_ptr);

	static uint32_t save_new_mine_without_garrison(player_t* player, uint32_t mine_cnt);

	static commonproto::prize_elem_type_t get_mine_prize_elem_type(
			mine_type_t type);

	static uint32_t get_mine_prize_elem_id(mine_type_t type,
			mine_size_t size);

	static uint32_t generate_new_mine(player_t* player,
		struct mine_info_t& mine_info);

	static uint32_t sync_mine_pet_info_to_hset(player_t* player, uint64_t mine_id);

	static uint32_t insert_mine_id_and_sync_db_rawdata(player_t* player, uint64_t mine_id);

	static uint32_t del_mine_id_and_sync_db_rawdata(player_t* player, uint64_t mine_id);

	static uint32_t sync_mine_ids_to_db(player_t* player);

	static uint32_t get_player_mine_info_from_db(player_t* player,
			const std::vector<uint64_t>& mine_ids);

	static uint32_t sync_match_mine_ids_to_db_rawdata(player_t* player);

	static uint32_t pack_mine_info(const struct mine_info_t& mine_info,
			commonproto::mine_info_t* pb_ptr);

	static uint32_t unpack_mine_info(const commonproto::mine_info_t& inf,
			struct mine_info_t& mine_info);

	static uint32_t pack_player_start_fight_pet_btl_info(player_t* player,
			std::vector<uint32_t>& pet_create_tm_vec,
			commonproto::battle_pet_list_t *pet_list);

	static uint32_t reset_player_pets_attack_mine_info_relate(player_t* player,
			bool is_login = true);

	static uint32_t daily_reset_mine_system_related_info(player_t* player);

	static uint32_t pack_all_def_pet_hp(player_t* player,
			onlineproto::team_hp_info_list_t& pb_inf);

	static uint32_t check_pets_can_join_fight(player_t* player,
			const std::vector<uint32_t>& pet_create_tm);

	static uint32_t convert_pb_pet_list_to_btl_pet_list(
			const commonproto::pet_list_t& pb_list,
			commonproto::battle_pet_list_t* btl_list);

	static uint32_t modify_pet_hp_for_memory(
			const commonproto::pet_cur_hp_list_t& pb_cli_pet_hp,
			std::vector<pet_simple_info_t>& ai_pet_hp_vec);

	static uint32_t calc_mine_fight_get_resource(player_t* player, uint64_t mine_id,
			std::vector<rob_resource_info_t>& rob_res_vec,
			uint32_t& id, uint32_t& total_num);

	static uint32_t generate_mine_btl_report(player_t* player, uint64_t mine_id,
			std::string& pkg, commonproto::challenge_result_type_t result,
			std::vector<rob_resource_info_t>& rob_res_vec);

	static uint32_t calc_mine_product_resource_speed(uint32_t def_pets_total_power,
			mine_type_t type, mine_size_t size);

	static uint32_t calc_player_get_reward(player_t* player,
			const std::vector<team_simple_info_t>& def_team_vec,
			mine_info_t& mine_info,
			mine_reward_type_t reward_type= MINE_IS_BE_OCCUPY);

	static uint32_t set_mine_attack_state(player_t* player, mine_attack_t type);

	static uint32_t pack_mine_info_for_client_v2(
			const struct mine_info_t& mine_info,
			commonproto::mine_info_t* mine_ptr);

	static uint32_t send_pets_start_defend_mine(player_t* player, uint64_t mine_id,
			const std::vector<uint32_t>& pet_ctm_vec);

	static uint32_t set_pets_defend_mine_id(player_t* player, uint64_t mine_id,
			std::vector<uint32_t>& pet_ctm_vec);

	static uint32_t check_mine_num_get_limit(player_t* player);

	static uint32_t mine_modify_player_pet_hp_for_cli(
			commonproto::battle_pet_list_t* btl_ptr,
			const commonproto::battle_pet_list_t& btl_pets);

	static uint32_t modify_def_team_btl_result(uint64_t def_role_key,
			commonproto::mine_team_info_list_t* pb_ptr, bool is_defeated);
};

#endif
