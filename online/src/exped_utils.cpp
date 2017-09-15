#include "exped_utils.h"
#include "pet_utils.h"
#include "common.h"
#include "player.h"
#include "exped.h"
#include "pet_utils.h"
#include "player_utils.h"

uint32_t ExpedUtils::check_exped_cond(
		player_t* player, 
		std::vector<uint32_t> &create_tm_vec)
{
	FOREACH(create_tm_vec, it) {
		Pet* pet = PetUtils::get_pet_in_loc(player, *it);
		if (pet == NULL) {
			return cli_err_bag_pet_not_exist;
		}
		if (pet->level() < commonproto::PET_LEVEL_LIMIT) {
			return cli_err_pet_level_can_join_exped;
		}
	}
	return 0;
}

uint32_t ExpedUtils::exped_modify_player_pet_hp_for_cli(
		commonproto::battle_pet_list_t* btl_ptr,
		const commonproto::battle_pet_list_t& btl_pets)
{
	for (int i = 0; i < btl_pets.pet_list_size(); ++i) {
		uint32_t exped_hp = btl_pets.pet_list(i).pet_info().exped_hp();
		uint32_t max_hp = btl_pets.pet_list(i).pet_info().battle_info().max_hp();
		btl_ptr->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_cur_hp(exped_hp * EXPED_HP_COE);
		btl_ptr->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_max_hp(max_hp * EXPED_HP_COE);
	}
	return 0;
}

uint32_t ExpedUtils::exped_modify_ai_pet_hp(
		commonproto::battle_pet_list_t& pb_btl_inf,
		const onlineproto::exped_pet_info_list& pb_inf)
{
	uint32_t exped_pet_size = pb_inf.exped_pets_size();
	for (uint32_t idx = 0; idx < exped_pet_size; ++idx) {
		uint32_t create_tm = pb_inf.exped_pets(idx).create_tm();
		uint32_t exped_hp = pb_inf.exped_pets(idx).exped_hp();
		for (int j = 0; j < pb_btl_inf.pet_list_size(); ++j) {
			uint32_t c_tm = pb_btl_inf.pet_list(j).pet_info().base_info().create_tm();
			if (create_tm != c_tm) {
				continue;
			}
			pb_btl_inf.mutable_pet_list(j)->mutable_pet_info()->set_exped_hp(exped_hp);
		}
	}
	return 0;
}

uint32_t ExpedUtils::exped_modify_ai_pet_hp_for_cli(
		commonproto::battle_pet_list_t& pb_btl_inf,
		const onlineproto::exped_pet_info_list& pb_inf)
{
	uint32_t exped_pet_size = pb_inf.exped_pets_size();
	for (uint32_t idx = 0; idx < exped_pet_size; ++idx) {
		uint32_t create_tm = pb_inf.exped_pets(idx).create_tm();
		uint32_t exped_hp = pb_inf.exped_pets(idx).exped_hp();
		uint32_t max_hp = pb_inf.exped_pets(idx).max_hp();
		for (int j = 0; j < pb_btl_inf.pet_list_size(); ++j) {
			uint32_t c_tm = pb_btl_inf.pet_list(j).pet_info().base_info().create_tm();
			if (create_tm != c_tm) {
				continue;
			}
			pb_btl_inf.mutable_pet_list(j)->mutable_pet_info()->mutable_battle_info()->set_cur_hp(exped_hp * EXPED_HP_COE);
			pb_btl_inf.mutable_pet_list(j)->mutable_pet_info()->mutable_battle_info()->set_max_hp(max_hp * EXPED_HP_COE);
		}
	}
	return 0;
}

//pet_inf 需要保证是来自,缓存中拉取出来的commonproto::battle_player_data_t中
//    	  的power_pet_list字段（因为该字段已经在DB中精灵战力降序排好的）
//		  s所以无需再调用PetUtils::get_pets_n_topest_power_from_cache
uint32_t ExpedUtils::pack_battle_exped_n_topest_pet_info(
		onlineproto::exped_pet_info_list* pb_pet_ptr,
		const commonproto::battle_pet_list_t& pb_pet_inf)
{
	for (int i = 0; i < pb_pet_inf.pet_list_size(); ++i) {
		pack_exped_unit_pet_info(pb_pet_ptr->add_exped_pets(),
			pb_pet_inf.pet_list(i).pet_info());
	}
	return 0;
}

uint32_t ExpedUtils::pack_exped_unit_pet_info(
	onlineproto::exped_pet_info* pb_pet_ptr,
	const commonproto::pet_info_t& pet_inf)
{
	pb_pet_ptr->set_pet_id(pet_inf.base_info().pet_id());
	pb_pet_ptr->set_level(pet_inf.base_info().level());
	pb_pet_ptr->set_talent_level(pet_inf.talent_level());
	pb_pet_ptr->set_create_tm(pet_inf.base_info().create_tm());
	pb_pet_ptr->set_exped_hp(pet_inf.battle_info().max_hp());
	pb_pet_ptr->set_max_hp(pet_inf.battle_info().max_hp());
	//计算特训等级之和
	uint32_t total_effect_lv = 0;
	total_effect_lv = pet_inf.effort_lv().effort_hp_lv() + 
		pet_inf.effort_lv().effort_normal_atk_lv() + 
		pet_inf.effort_lv().effort_normal_def_lv() + 
		pet_inf.effort_lv().effort_skill_atk_lv() + 
		pet_inf.effort_lv().effort_skill_def_lv();
	pb_pet_ptr->set_effect_lv_sum(total_effect_lv);
	//战斗力信息
	pb_pet_ptr->set_power(pet_inf.base_info().power());
	//打包符文的信息
	pb_pet_ptr->mutable_rune_info()->CopyFrom(pet_inf.rune_info());
	return 0;
}

uint32_t ExpedUtils::pack_all_exped_info_and_update_db(player_t* player)
{
	onlineproto::expedition_users_pet_info_list exped_list;
	player->expedtion->pack_all_exped_info_to_pbmsg(&exped_list);
	PlayerUtils::update_user_raw_data(player->userid,player->create_tm, 
			dbproto::EXPED_PETS_INFO, exped_list, "0");
	return 0;
}
