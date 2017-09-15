#include "achieve.h"
#include "achieve_conf.h"
#include "global_data.h"
#include "player.h"
#include "player_utils.h"
#include "pet_utils.h"
#include "service.h"
#include "prize.h"

bool Achieve::is_exist_this_achieve(uint32_t achieve_id)
{
	if (achieve_map.count(achieve_id)) {
		return true;
	}
	return false;
}

uint32_t Achieve::add_achieves_info(player_t* player,
		const std::vector<achieve_info_t>& achv_vec)
{
	insert_achieves_to_memory(achv_vec);
	save_achieves_to_db(player, achv_vec);
	return 0;
}

uint32_t Achieve::insert_achieves_to_memory(const std::vector<achieve_info_t>& achv_vec)
{
	FOREACH(achv_vec, it) {
		achieve_map.insert(AchieveMap::value_type(it->id, *it));
	}
	return 0;
}

uint32_t Achieve::save_achieves_to_db(player_t* player,
		const std::vector<achieve_info_t>& achv_vec,
		bool need_sync_cli)
{
	dbproto::cs_save_achieves db_msg;
	commonproto::achieve_info_list_t* achv_ptr = db_msg.mutable_achv_info();
	FOREACH(achv_vec, it) {
		commonproto::achieve_info_t* ptr = achv_ptr->add_ach_info();
		ptr->set_achieve_id(it->id);
		ptr->set_get_time(it->get_time);
	}
	int ret = g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
			db_cmd_save_achieves, db_msg);
	if (ret != 0) {
		return cli_err_sys_err;
	}
	if (need_sync_cli) {
		onlineproto::sc_0x0150_notify_get_new_achieve noti_msg;
		noti_msg.mutable_achv_list()->CopyFrom(db_msg.achv_info());
		send_msg_to_player(player, cli_cmd_cs_0x0150_notify_get_new_achieve, noti_msg);
	}
	return 0;
}

uint32_t Achieve::pack_achieve_info_list(commonproto::achieve_info_list_t* pb_ptr)
{
	FOREACH(achieve_map, it) {
		commonproto::achieve_info_t *ach_ptr = pb_ptr->add_ach_info();
		ach_ptr->set_achieve_id(it->second.id);
		ach_ptr->set_get_time(it->second.get_time);
	}
	return 0;
}

uint32_t Achieve::get_achieve_step_info(uint32_t achieve_type, 
	commonproto::achieve_progress_list_t& ach_pro_list,
	uint32_t condition_value) 
{
	commonproto::achieve_progress_t* pro_ptr = ach_pro_list.add_ach_pro_list();
	pro_ptr->set_achieve_type(achieve_type);
	pro_ptr->set_step(condition_value);
	return 0;
}

uint32_t Achieve::init_one_achieve(player_t* player,
		uint32_t achieve_id, uint32_t get_time,
		std::vector<achieve_info_t>& achv_vec)
{
	achieve_info_t  achv_info;
	achv_info.id = achieve_id;
	achv_info.get_time = get_time;
	achv_vec.push_back(achv_info);
	return 0;
}

uint32_t Achieve::add_prize_and_point_by_achieve_id(player_t* player,
		uint32_t prize_id, uint32_t achieve_point)
{
	ADD_A(kAttrAchievePoint, achieve_point);
	onlineproto::sc_0x0112_notify_get_prize noti_prize_msg;
	transaction_proc_prize(player, prize_id,
			noti_prize_msg,
			commonproto::PRIZE_REASON_GET_ACHIEVE_ID,
			onlineproto::SYNC_REASON_PRIZE_ITEM);
	send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_prize_msg);
	return 0;
}

void AchieveRegisterFunc::register_listen_achieve_func()
{
	achieve_func_map_.clear();
	achieve_func_map_[(uint32_t)ACH_01_INC_POWER] = achieve_01_increase_power;
	achieve_func_map_[(uint32_t)ACH_02_INC_LEVEL] = achieve_02_increase_level;
	achieve_func_map_[(uint32_t)ACH_03_GET_EQUIP] = achieve_03_get_equip;
	achieve_func_map_[(uint32_t)ACH_05_GET_PET] = achieve_05_get_pet_num;
	achieve_func_map_[(uint32_t)ACH_06_GET_TELENT_PET] = achieve_06_get_telent_pet;
	achieve_func_map_[(uint32_t)ACH_07_GET_QUALITY_PET] = achieve_07_get_quality_pet;
	achieve_func_map_[(uint32_t)ACH_08_INC_PET_EFFORT] = achieve_08_inc_effort;
}

void AchieveRegisterFunc::call_achieve_register_func(player_t* player, uint32_t type,
		commonproto::achieve_progress_list_t& ach_list)
{
	if (achieve_func_map_.count(type)) {
		(achieve_func_map_.find(type)->second)(player, type, ach_list);
	}
}

#define GET_ACHIEVES_BY_TYPE(type) \
	const achieve_mgr_t::AchieveIdVec* ach_vec_ptr = g_achieve_mgr.get_achieve_id_vec(type); \
if (!ach_vec_ptr || ach_vec_ptr->empty()) { \
	ERROR_TLOG("achieve_type=[%u]has no achieves:uid=[%u]", type, player->userid); \
	return 0; \
} \


int AchieveRegisterFunc::achieve_01_increase_power(ACHIEVE_FUN_ARG)
{
	GET_ACHIEVES_BY_TYPE(type);	
	uint32_t cur_btl_value = GET_A(kAttrBattleValueRecord);
	check_achieve_condition(player, ach_vec_ptr, cur_btl_value);
	return 0;
}

int AchieveRegisterFunc::achieve_02_increase_level(ACHIEVE_FUN_ARG)
{
	GET_ACHIEVES_BY_TYPE(type);	
	uint32_t cur_level = GET_A(kAttrLv);
	check_achieve_condition(player, ach_vec_ptr, cur_level);
	return 0;
}

int AchieveRegisterFunc::achieve_03_get_equip(ACHIEVE_FUN_ARG)
{
	GET_ACHIEVES_BY_TYPE(type);	
	//计算出我拥有最高品级的战装值
	uint32_t begin = EQUIP_QUALITY_WHITE;
	uint32_t end = EQUIP_QUALITY_GOLD;
	for (uint32_t i = begin; i <= end; ++i) {
		uint32_t count = 0;
		player->package->get_equip_count_by_quality(player, (equip_quality_t)i, count);
		if (count) {
			check_achieve_condition_with_2_args(player, ach_vec_ptr, i, count);
		}
	}
	return 0;
}

int AchieveRegisterFunc::achieve_05_get_pet_num(ACHIEVE_FUN_ARG)
{
	GET_ACHIEVES_BY_TYPE(type);
	uint32_t pet_total_count = GET_A(kAttrPetCount);
	check_achieve_condition(player, ach_vec_ptr, pet_total_count);
	return 0;
}

int AchieveRegisterFunc::achieve_06_get_telent_pet(ACHIEVE_FUN_ARG)
{
	GET_ACHIEVES_BY_TYPE(type);
	uint32_t begin = kPetTalentLevelOne;
	uint32_t end = kPetTalentLevelFull;
	for (uint32_t i = begin; i <= end; ++i) {
		uint32_t count = 0;
		PetUtils::get_pet_count_by_telent(player, (pet_talent_level_t)i, count);
		if (count) {
			check_achieve_condition_with_2_args(player, ach_vec_ptr, i, count);
		}
	}
	return 0;
}

int AchieveRegisterFunc::achieve_07_get_quality_pet(ACHIEVE_FUN_ARG)
{
	GET_ACHIEVES_BY_TYPE(type);
	uint32_t begin = commonproto::MIN_PET_QUALITY;
	uint32_t end = commonproto::MAX_PET_QUALITY;
	for (uint32_t i = begin; i <= end; ++i) {
		uint32_t count = 0;
		PetUtils::get_pet_count_by_quality(player, i, count);
		if (count) {
			check_achieve_condition_with_2_args(player, ach_vec_ptr, i, count);
		}
	}
	return 0;
}

int AchieveRegisterFunc::achieve_08_inc_effort(ACHIEVE_FUN_ARG)
{
	GET_ACHIEVES_BY_TYPE(type);
	//找出特训等级最高的伙伴
	uint32_t max_effort_lv = GET_A(kAttrMaxEffortLv);
	check_achieve_condition(player, ach_vec_ptr, max_effort_lv);
	return 0;
}

int AchieveRegisterFunc::check_achieve_condition(
		player_t* player, const achieve_mgr_t::AchieveIdVec *achieves, uint32_t value)
{
	std::vector<achieve_info_t> achv_vec;
	FOREACH(*achieves, it) {
		achieve_conf_t* conf_ptr = g_achieve_mgr.get_achieve_info_conf(*it);
		if (conf_ptr == NULL) {
			continue;
		}
		//判断是否已经拥有这个成就id,若有了，就continue;
		if (player->achieve->is_exist_this_achieve(*it)) {
			continue;
		}
		//
		if (value >= conf_ptr->track_target) {
			player->achieve->init_one_achieve(player, *it, NOW(), achv_vec);
			player->achieve->add_prize_and_point_by_achieve_id(player,
					conf_ptr->prize_id, conf_ptr->point);
		}
	}
	if (!achv_vec.empty()) {
		player->achieve->add_achieves_info(player, achv_vec);
	}
	return 0;
}

int AchieveRegisterFunc::check_achieve_condition_with_2_args(
		player_t* player,
		const achieve_mgr_t::AchieveIdVec* achieves,
		uint32_t arg1, uint32_t arg2)
{
	std::vector<achieve_info_t> achv_vec;
	FOREACH(*achieves, it) {
		achieve_conf_t* conf_ptr = g_achieve_mgr.get_achieve_info_conf(*it);
		if (conf_ptr == NULL) {
			continue;
		}
		//判断是否已经拥有这个成就id,若有了，就continue;
		if (player->achieve->is_exist_this_achieve(*it)) {
			continue;
		}

		if (arg1 >= conf_ptr->track_target && arg2 >= conf_ptr->target_2_value) {
			player->achieve->init_one_achieve(player, *it, NOW(), achv_vec);
			player->achieve->add_prize_and_point_by_achieve_id(player,
				conf_ptr->prize_id, conf_ptr->point);
		}
	}
	if (!achv_vec.empty()) {
		player->achieve->add_achieves_info(player, achv_vec);
	}
	return 0;
}
