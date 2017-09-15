#ifndef __EXPED_UTILS_H__
#define __EXPED_UTILS_H__
#include <vector>
#include "common.h"
struct player_t;

class ExpedUtils { 
public:
	static uint32_t check_exped_cond(
			player_t* player, 
			std::vector<uint32_t> &create_tm_vec);

	//pet_inf 需要保证是来自,缓存中拉取出来的commonproto::battle_player_data_t中
	//    	  的power_pet_list字段（因为该字段已经在DB中按照精灵战力降序排好的）
	//		  所以无需再调用PetUtils::get_pets_n_topest_power_from_cache
	static uint32_t pack_battle_exped_n_topest_pet_info(
			onlineproto::exped_pet_info_list* exp_pet_list,
			const commonproto::battle_pet_list_t& pet_inf);

	static uint32_t pack_exped_unit_pet_info(
			onlineproto::exped_pet_info* pb_pet_ptr,
			const commonproto::pet_info_t& pet_inf);

	/*
	static uint32_t save_cur_hp_to_string(player_t* player,
			const onlineproto::expedition_pet_cur_hp_list& pet_hp_list);
	*/

	//根据前端要求，回包之前将exped_hp 设置给 hp
	//warning:不可保存到内存以及持久化到数据库
	//        所以必须紧跟在send_msg_to_player的上一行调用
	//warning: 仅仅传给前端时调用
	  static uint32_t exped_modify_player_pet_hp_for_cli(
			commonproto::battle_pet_list_t* btl_ptr,
			const commonproto::battle_pet_list_t& btl_pets);

	//不是打包给前端用的
	  static uint32_t exped_modify_ai_pet_hp(
			commonproto::battle_pet_list_t& pb_btl_inf,
			const onlineproto::exped_pet_info_list& pb_inf);

	  static uint32_t exped_modify_ai_pet_hp_for_cli(
			commonproto::battle_pet_list_t& pb_btl_inf,
			const onlineproto::exped_pet_info_list& pb_inf);

	  static uint32_t pack_all_exped_info_and_update_db(player_t* player);
};
#endif
