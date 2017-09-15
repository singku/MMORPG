#include "mine_utils.h"
#include "pet.h"
#include "pet_utils.h"
#include "service.h"
#include "rank_utils.h"
#include "data_proto_utils.h"
#include "mail.h"
#include "mail_utils.h"
#include <boost/lexical_cast.hpp>

uint32_t MineUtils::req_dbsvr_to_search_mine(player_t* player,
		std::vector<uint64_t>& match_ids)
{
	const uint32_t PET_NUM = 5;
	std::set<uint32_t> create_tm_set;
	uint32_t total_power = 0;
	PetUtils::get_pets_n_topest_power(player, create_tm_set, PET_NUM, total_power);
	dbproto::cs_get_mine_info db_in;
	uint32_t low_power = total_power * 70 / 100.0;
	low_power = 0;
	uint32_t high_power = total_power * 110 / 100.0;
	high_power = 0x7FFFFFFF;
	db_in.set_low_power(low_power);
	db_in.set_high_power(high_power);
	db_in.set_time_stamp(NOW());
	db_in.set_server_id(g_server_id);
	uint32_t protect_duration = commonproto::MINE_PROTECT_DURATION;
	db_in.set_protect_duration(protect_duration);
	FOREACH(match_ids, it) {
		db_in.add_my_mine_id(*it);
	}
	return g_dbproxy->send_msg(player,
			player->userid, player->create_tm,
			db_cmd_search_occupyed_mine, db_in);
}

uint32_t MineUtils::req_redis_to_get_teams_info(player_t* player,
		std::vector<uint64_t>& mine_id_vec)
{
	rankproto::cs_hset_get_info rank_in;
	FOREACH(mine_id_vec, it) {
		uint32_t key = rankproto::HASHSET_MINE_ID_TEAM_INFO_MAP;
		uint32_t sub_key = *it;
		std::ostringstream redis_key;
		redis_key << key << ":" << sub_key;
		
		
		rank_in.add_keys(redis_key.str());
	}
	rank_in.set_server_id(g_server_id);
	return g_dbproxy->send_msg(player,
			player->userid, player->create_tm,
			ranking_cmd_hset_get_info, rank_in);
}

uint32_t MineUtils::save_new_mine_without_garrison(player_t* player, uint32_t mine_cnt)
{
	commonproto::mine_info_list_t pb_new_mine_info;
	for (uint32_t i = 0; i < mine_cnt; ++i) {
		commonproto::mine_info_t* pb_ptr = pb_new_mine_info.add_mine_info();
		struct mine_info_t mine_info;
		MineUtils::generate_new_mine(player, mine_info);
		player->mine_info->add_mine_tmp_info_map(mine_info);	
		MineUtils::pack_mine_info(mine_info, pb_ptr);
		//cli_out.add_mine_info()->CopyFrom(*pb_ptr);
	}
	PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
			dbproto::NEW_MINE_INFO, pb_new_mine_info, "0");
	return 0;
}

uint32_t MineUtils::pack_mine_info_for_client_v2(
		const struct mine_info_t& mine_info,
		commonproto::mine_info_t* mine_ptr)
{
	MineUtils::pack_mine_info(mine_info, mine_ptr);
	MineUtils::ex_pack_prize_elem_info_for_cli(*mine_ptr, mine_ptr);
	std::string str_mine_id = boost::lexical_cast<std::string>(mine_info.mine_id);
	mine_ptr->set_str_mine_id(str_mine_id);
	return 0;
}

uint32_t MineUtils::ex_pack_prize_elem_info_for_cli(
		const commonproto::mine_info_t& pb_mine,
		commonproto::mine_info_t* pb_ptr)
{
	mine_type_t mine_type = (mine_type_t)pb_mine.type();
	commonproto::prize_elem_type_t elem_type = get_mine_prize_elem_type(mine_type);
	pb_ptr->mutable_elem()->set_type(elem_type);
	if (pb_mine.elem_id()) {
		pb_ptr->mutable_elem()->set_id(pb_mine.elem_id());
	} else {
		uint32_t elem_id = MineUtils::get_mine_prize_elem_id(mine_type, (mine_size_t)pb_mine.size());
		pb_ptr->mutable_elem()->set_id(elem_id);
	}
	pb_ptr->mutable_elem()->set_count(0);
	return 0;
}

uint32_t MineUtils::get_mine_prize_elem_id(mine_type_t type, mine_size_t size)
{
	uint32_t elem_id = 0;
	switch(type) {
		case MINE_GOLD:
			elem_id = 31004;	
			return elem_id;
		case MINE_EXP:
			if (size == SMALL_MINE) {
				elem_id = 32001;
			} else if (size == MID_MINE) {
				elem_id = 32002;
			} else if (size == BIG_MINE) {
				elem_id = 32003;
			}
			return elem_id;
		case MINE_EFFORT:
			uint32_t rand_index = taomee::ranged_random(0, 9);
			if (rand_index % 2 != 0) {
				if (rand_index == 9) {
					--rand_index;
				} else {
					++rand_index;
				}
			}
			elem_id = 35100 + rand_index;
			return elem_id;
	}
	return 0;
}

commonproto::prize_elem_type_t MineUtils::get_mine_prize_elem_type(mine_type_t type)
{
	switch (type) {
		case MINE_GOLD:
		case MINE_EXP:
		case MINE_EFFORT:
			return commonproto::PRIZE_ELEM_TYPE_ITEM;
	}
	return commonproto::PRIZE_ELEM_NOT_DEFINED;
}

uint32_t MineUtils::generate_new_mine(player_t* player,
		struct mine_info_t& mine_info)
{
	uint32_t total_power = 0;
	std::set<uint32_t> create_tm_set;
	uint32_t PET_NUM = 5;
	PetUtils::get_pets_n_topest_power(player, create_tm_set, PET_NUM, total_power);
	std::map<uint32_t, uint32_t> mine_type_rate;
	mine_type_rate[1] = 100;
	mine_type_rate[2] = 70;
	mine_type_rate[3] = 30;
	mine_type_rate[4] = 100;
	mine_type_rate[5] = 70;
	mine_type_rate[6] = 30;
	mine_type_rate[7] = 50;
	mine_type_rate[8] = 35;
	mine_type_rate[9] = 15;
	std::set<uint32_t> mine_type_idx_set;
	Utils::rand_select_uniq_m_from_n_with_r(mine_type_rate, mine_type_idx_set, 1);
	uint32_t index = 0;
	if (mine_type_idx_set.size() == 1) {
		std::set<uint32_t>::iterator it = mine_type_idx_set.begin();
		index = *it;
	}
	mine_type_t mine_type;	
	mine_size_t mine_size;
	mine_size_t size_array[] = {SMALL_MINE, MID_MINE, BIG_MINE};
	uint32_t mine_exploit_times[] = {2, 4, 6};
	if (index >= 1 && index <= 3) {
		mine_type = MINE_GOLD;
		mine_size = size_array[index - 1];
	} else if (index >= 4 && index <= 6) {
		mine_type = MINE_EXP;
		mine_size = size_array[index - 1 - 3];
	} else if (index >= 7 && index <= 9) {
		mine_type = MINE_EFFORT;
		mine_size = size_array[index - 1 - 2 * 3];	
	}
	memset(&mine_info, 0, sizeof(mine_info));
	mine_info.mine_id = 0;
	mine_info.mine_type = mine_type;
	mine_info.mine_size = mine_size;
	mine_info.mine_create_tm = NOW();
	mine_info.user_id = player->userid;
	mine_info.u_create_tm = player->create_tm;
	mine_info.top_pet_power = total_power;
	mine_info.duration_time = mine_exploit_times[mine_size - 1] * 3600;
	mine_info.def_player_cnt = 1;
	mine_info.last_fight_time = 0;

	uint32_t elem_id = get_mine_prize_elem_id(mine_type, mine_size);
	mine_info.elem_id = elem_id;
	return 0;
}

uint32_t MineUtils::pack_mine_team_info(player_t* player,
		const std::vector<uint32_t>& pet_create_tm,
		commonproto::mine_team_info_t* pb_team_ptr)
{
	commonproto::pet_list_t* pb_ptr = pb_team_ptr->mutable_pet_list();
	FOREACH(pet_create_tm, it) {
		Pet* pet = PetUtils::get_pet_in_loc(player, *it);
		if (pet == NULL) {
			return cli_err_bag_pet_not_exist;
		}
		DataProtoUtils::pack_player_pet_info(player, pet, pb_ptr->add_pet_list());
	}
    std::set<uint32_t> create_tm_set;
    uint32_t total_power = 0; 
    PetUtils::get_pets_n_topest_power(player, create_tm_set, 5, total_power);

	pb_team_ptr->set_userid(player->userid);
	pb_team_ptr->set_u_create_tm(player->create_tm);
	pb_team_ptr->set_sex(GET_A(kAttrSex));
	pb_team_ptr->set_level(GET_A(kAttrLv));
	pb_team_ptr->set_nick(player->nick);
	pb_team_ptr->set_power(GET_A(kAttrCurBattleValue));
	pb_team_ptr->set_exploit_start_time(NOW());

	pb_team_ptr->set_total_power(total_power);
	return 0;
}

uint32_t MineUtils::sync_mine_pet_info_to_hset(player_t* player, uint64_t mine_id)
{
	struct mine_info_t mine;
	player->mine_info->get_mine_info_from_memory(mine_id, mine);
	vector<rankproto::hset_field_t> fields_vec;
	uint64_t uid_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	std::string uid_key_str = boost::lexical_cast<std::string>(uid_key);
	rankproto::hset_field_t pb_pet_info;
	pb_pet_info.set_name(uid_key_str);
	uint32_t type = 0;
	std::string pet_info_pkg;
	if (player->mine_info->get_pet_info_from_memory(mine_id, pet_info_pkg)) {
		type = rankproto::REDIS_DELETE;
	} else {
		type = rankproto::REDIS_INSERT_OR_UPDATE;
		pb_pet_info.set_value(pet_info_pkg);
	}
	fields_vec.push_back(pb_pet_info);
	uint32_t hashset_type = rankproto::HASHSET_MINE_ID_TEAM_INFO_MAP;
	std::string hashset_type_str = boost::lexical_cast<std::string>(hashset_type);
	std::string sub_type = boost::lexical_cast<std::string>(mine_id);
	if (type == rankproto::REDIS_DELETE) {
		return RankUtils::hset_insert_or_update_str_version(
				NULL,  player->userid, player->create_tm,
				g_server_id,
				hashset_type_str, sub_type, type, 
				&fields_vec, DAY_SECS * 7);
	}
	return RankUtils::hset_insert_or_update_str_version(
			NULL,  player->userid, player->create_tm,
			g_server_id,
			hashset_type_str, sub_type, type, 
			&fields_vec, DAY_SECS * 7);
}

uint32_t MineUtils::insert_mine_id_and_sync_db_rawdata(player_t* player, uint64_t mine_id)
{
	uint32_t ret = player->mine_info->insert_my_mine_ids_to_memory(mine_id);
	if (ret) {
		return ret;
	}
	MineUtils::sync_mine_ids_to_db(player);
	return 0;
}

uint32_t MineUtils::sync_mine_ids_to_db(player_t* player)
{
	commonproto::mine_id_list_t pb_ids;
	player->mine_info->pack_mine_ids(pb_ids);
	PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
			dbproto::MINE_IDS, pb_ids, "0");
	return 0;
}

uint32_t MineUtils::del_mine_id_and_sync_db_rawdata(player_t* player,
		uint64_t mine_id)
{
	uint32_t ret = player->mine_info->delete_my_mine_id_from_memory(mine_id);
	if (ret) {
		return ret;
	}
	MineUtils::sync_mine_ids_to_db(player);
	return 0;
	
}

uint32_t MineUtils::get_player_mine_info_from_db(player_t* player,
		const std::vector<uint64_t>& mine_ids)
{
	dbproto::cs_get_player_mine_info db_in;
	FOREACH(mine_ids, it) {
		db_in.add_mine_id(*it);
	}
	return g_dbproxy->send_msg(player, player->userid,
			player->create_tm,
			db_cmd_get_player_mine_info,
			db_in);	
}

uint32_t MineUtils::sync_match_mine_ids_to_db_rawdata(player_t* player)
{
	commonproto::mine_id_list_t pb_mine_ids;
	player->mine_info->pack_match_mine_ids(pb_mine_ids);
	PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
			dbproto::OPPONENT_MINE_IDS, pb_mine_ids, "0");
	return 0;
}

uint32_t MineUtils::pack_mine_info(const struct mine_info_t& mine_info,
		commonproto::mine_info_t* pb_ptr)
{
	pb_ptr->set_mine_id(mine_info.mine_id);
	pb_ptr->set_mine_create_tm(mine_info.mine_create_tm);
	pb_ptr->set_type((commonproto::mine_type_t)mine_info.mine_type);
	pb_ptr->set_size((commonproto::mine_size_t)mine_info.mine_size);
	pb_ptr->set_server_id(g_server_id);
	pb_ptr->set_user_id(mine_info.user_id);
	pb_ptr->set_u_create_tm(mine_info.u_create_tm);
	pb_ptr->set_top_pet_power(mine_info.top_pet_power);
	pb_ptr->set_duration_time(mine_info.duration_time);
	pb_ptr->set_def_player_cnt(mine_info.def_player_cnt);
	pb_ptr->set_is_been_attacked(mine_info.is_been_attacked);
	pb_ptr->set_last_fight_time(mine_info.last_fight_time);
	pb_ptr->set_elem_id(mine_info.elem_id);
	return 0;
}

uint32_t MineUtils::unpack_mine_info(const commonproto::mine_info_t& inf,
		struct mine_info_t& mine_info)
{
	mine_info.mine_id = inf.mine_id();
	mine_info.mine_create_tm = inf.mine_create_tm();
	mine_info.mine_type = (mine_type_t)inf.type();
	mine_info.mine_size = (mine_size_t)inf.size();
	mine_info.user_id = inf.user_id();
	mine_info.u_create_tm = inf.u_create_tm();
	mine_info.top_pet_power = inf.top_pet_power();
	mine_info.duration_time = inf.duration_time();
	mine_info.def_player_cnt = inf.def_player_cnt();
	mine_info.is_been_attacked = inf.is_been_attacked();
	mine_info.last_fight_time = inf.last_fight_time();
	mine_info.elem_id = inf.elem_id();
	return 0;
}

uint32_t MineUtils::pack_player_start_fight_pet_btl_info(player_t* player,
		std::vector<uint32_t>& pet_create_tm_vec,
		commonproto::battle_pet_list_t *pet_list)
{
	FOREACH(pet_create_tm_vec, it) {
		Pet* pet = PetUtils::get_pet_in_loc(player, *it);
		if (pet == NULL) {
			continue;
		}
		commonproto::battle_pet_data_t* btl_pet = pet_list->add_pet_list();
		DataProtoUtils::pack_player_pet_info(player, pet, btl_pet->mutable_pet_info());
	}
	return 0;
}

uint32_t MineUtils::daily_reset_mine_system_related_info(player_t* player)
{
	uint32_t last_date = TimeUtils::time_to_date(GET_A(kAttrLastLoginTm));
	//if 逻辑里 ，说明是今日第一次登录
	if (last_date != TimeUtils::get_today_date()) {
		PlayerUtils::delete_user_raw_data(player->userid, player->create_tm,
				dbproto::NEW_MINE_INFO, "0");
		PlayerUtils::delete_user_raw_data(player->userid, player->create_tm,
				dbproto::OPPONENT_MINE_IDS, "0");

		PlayerUtils::delete_user_raw_data(player->userid, player->create_tm,
				dbproto::MINE_FIGHT_DEF_PET_HP, "0");

		player->mine_info->clear_mine_tmp_info();
		player->mine_info->clear_match_mine_ids();
		MineUtils::reset_player_pets_attack_mine_info_relate(player, false);

		uint32_t left_times = GET_A(kDailyMineBuyChallengeTimes) + 
			commonproto::MINE_DAILY_OCCUPY_MINE_TIMES - 
			GET_A(kDailyMineHasChallengeTimes);

		if (GET_A(kDailyMineHasChallengeTimes) >= 
				GET_A(kDailyMineBuyChallengeTimes) + 
				commonproto::MINE_DAILY_OCCUPY_MINE_TIMES)
		{
			left_times = 0;
		}
		SET_A(kDailyMineLeftChallengeTimes, left_times);

	}
	return 0;
}

uint32_t MineUtils::reset_player_pets_attack_mine_info_relate(player_t* player,
		bool is_login)
{
	FOREACH(*player->pets, it) {
		Pet& pet = it->second;
		bool need_sync_db = false;
		if (pet.mine_attack_hp() < pet.max_hp()) {
			pet.set_mine_attack_hp(pet.max_hp());
			need_sync_db = true;
		}
		if (pet.pet_mine_flag()) {
			pet.set_mine_flag(0);
			need_sync_db = true;
		}
		if (need_sync_db) {
			PetUtils::save_pet(player, pet, false, is_login);
		}
	}
	return 0;
}

uint32_t MineUtils::pack_all_def_pet_hp(player_t* player,
		onlineproto::team_hp_info_list_t& pb_inf)
{
	FOREACH(player->mine_info->ai_pet_hp_map_, it) {
		onlineproto::team_hp_info_t* pb_ptr = pb_inf.add_team_list();
		uint64_t role_key  = it->first;
		uint32_t user_id = KEY_ROLE(role_key).userid;
		uint32_t create_tm = KEY_ROLE(role_key).u_create_tm;
		TRACE_TLOG("pack_all_def_pet_hp,role_key=%u, %u",
			user_id, player->mine_info->ai_state_map_[role_key]);
		pb_ptr->set_user_id(user_id);
		pb_ptr->set_create_tm(create_tm);
		pb_ptr->set_fight_state((onlineproto::mine_fight_state_t)player->mine_info->ai_state_map_[role_key]);
		std::map<uint64_t, uint32_t>::iterator iter;
		iter = player->mine_info->def_pk_time_map_.find(role_key);
		if (iter != player->mine_info->def_pk_time_map_.end()) {
			pb_ptr->set_fight_time(iter->second);
		}
		commonproto::pet_cur_hp_list_t* pb_pet_ptr = pb_ptr->mutable_hp_list();
		std::vector<pet_simple_info_t>& pet_hp_vec = it->second;
		FOREACH(pet_hp_vec, iter) {
			commonproto::pet_cur_hp_t* cur_ptr = pb_pet_ptr->add_hp_list();
			cur_ptr->set_create_tm(iter->pet_create_tm);
			cur_ptr->set_cur_hp(iter->pet_cur_hp);
		}
	}
	return 0;
}

uint32_t MineUtils::check_pets_can_join_fight(player_t* player,
		const std::vector<uint32_t>& pet_create_tm)
{
	FOREACH(pet_create_tm, it) {
		Pet* pet = PetUtils::get_pet_in_loc(player, *it);
		if (pet == NULL) {
			return cli_err_pet_not_exist;
		}
		//空血量不能出战
		if (pet->mine_attack_hp() == 0) {
			return cli_err_exped_pets_has_died;
		}
	}
	return 0;
}

uint32_t MineUtils::convert_pb_pet_list_to_btl_pet_list(
		const commonproto::pet_list_t& pb_list,
		commonproto::battle_pet_list_t* btl_list)
{
	for (int i = 0; i < pb_list.pet_list_size(); ++i) {
		const commonproto::pet_info_t& pb_inf = pb_list.pet_list(i);
		btl_list->add_pet_list()->mutable_pet_info()->CopyFrom(pb_inf);
	}
	return 0;
}

uint32_t MineUtils::modify_pet_hp_for_memory(
		const commonproto::pet_cur_hp_list_t& pb_cli_pet_hp,
		std::vector<pet_simple_info_t>& ai_pet_hp_vec)
{
	uint32_t zero_hp_count = 0;
	std::vector<uint32_t> tmp_zero_hp_pet;
	for (int i = 0; i < pb_cli_pet_hp.hp_list_size(); ++i) {
		const commonproto::pet_cur_hp_t& pb_inf = pb_cli_pet_hp.hp_list(i);
		bool found_pet = false;
		FOREACH(ai_pet_hp_vec, it) {
			if (it->pet_create_tm == pb_inf.create_tm()) {
				found_pet = true;
				uint32_t cur_hp = ceil(pb_inf.cur_hp() / (MINE_HP_COE * 1.0));
				/*
				if (pb_inf.cur_hp() >= it->pet_cur_hp) {
					return cli_err_fight_pets_cur_hp_err;
				}
				*/
				if (cur_hp == 0) {
					++zero_hp_count;
					tmp_zero_hp_pet.push_back(it->pet_create_tm);
				}
				it->pet_cur_hp = cur_hp;
				break;
			}
		}
		if (found_pet == false) {
			return cli_err_client_send_pet_ctm_err;
		}
	}
	//如果防守方的精灵血量全部为空，则做个处理：使得其中一个精灵血量值为1
	if (zero_hp_count == (uint32_t)pb_cli_pet_hp.hp_list_size()) {
		if (!tmp_zero_hp_pet.empty()) {
			FOREACH(ai_pet_hp_vec, it) {
				if (it->pet_create_tm == tmp_zero_hp_pet[0]) {
					it->pet_cur_hp = 1;
					break;
				}
			}
		}
	}
	return 0;
}

uint32_t MineUtils::calc_mine_fight_get_resource(player_t* player, uint64_t mine_id,
		std::vector<rob_resource_info_t>& rob_res_vec,
		uint32_t& id, uint32_t& total_num)
{
	total_num = 0;
	struct mine_info_t mine;
	
	uint32_t ret = player->mine_info->get_match_mine_info_from_memory(mine_id, mine);
	if (ret) {
		return ret;
	}
	if (mine.mine_create_tm < 1435248000) {
		id = MineUtils::get_mine_prize_elem_id(mine.mine_type, mine.mine_size);
	} else {
		if (mine.elem_id) {
			id = mine.elem_id;
		} else {
			id = MineUtils::get_mine_prize_elem_id(mine.mine_type, mine.mine_size);
		}
	}
	const uint32_t protect_time = commonproto::MINE_PROTECT_DURATION;
	FOREACH(player->mine_info->def_team_vec_, it) {
		uint32_t uid = it->userid;
		uint32_t u_create_tm = it->u_create_tm;
		uint64_t role_key = ROLE_KEY(ROLE(uid, u_create_tm));
		std::map<uint64_t, uint32_t>::iterator iter;
		iter = player->mine_info->ai_state_map_.find(role_key);
		if (iter == player->mine_info->ai_state_map_.end()) {
			continue;
		}
		uint32_t real_calc_time = 0;
		if (mine.mine_create_tm + protect_time > it->exploit_start_time) {
			if (mine.mine_create_tm + protect_time < NOW()) {
				real_calc_time = NOW() - mine.mine_create_tm - protect_time;
			}
		} else {
			if (it->exploit_start_time < NOW()) {
				real_calc_time = NOW() - it->exploit_start_time;
			}
		}
		uint32_t mine_speed = MineUtils::calc_mine_product_resource_speed(
				it->total_power, mine.mine_type, mine.mine_size);
		//被掠夺的玩家的守矿精灵战力和 * 实际开采的时间 
		uint32_t real_rob_num = ceil(mine_speed * real_calc_time / 3600 * 0.4);
		//uint32_t real_rob_num = it->total_power * real_calc_time * 30.0 / 100;
		TRACE_TLOG("Calc Rob Num mine_create_tm=[%u],real_calc_time=[%u],now_time=[%u]"
				"rob_num=[%u], id=[%u],total_power=%u,mine_speed=%u",
				mine.mine_create_tm, real_calc_time, NOW(),
				real_rob_num, id, it->total_power, mine_speed);
		rob_resource_info_t rob_res_info;
		rob_res_info.userid = uid;
		rob_res_info.u_create_tm = u_create_tm;
		rob_res_info.rob_num = real_rob_num;
		rob_res_info.rob_id = id;
		rob_res_vec.push_back(rob_res_info);	
		total_num += real_rob_num;
	}
	return 0;
}

uint32_t MineUtils::generate_mine_btl_report(player_t* player, uint64_t mine_id,
		std::string& pkg, commonproto::challenge_result_type_t result,
		std::vector<rob_resource_info_t>& rob_res_vec)
{
	struct mine_info_t mine;
	uint32_t ret = player->mine_info->get_match_mine_info_from_memory(mine_id, mine);
	if (ret) {
		return ret;
	}
	onlineproto::mine_fight_btl_report_t pb_btl;
	//MineUtils::pack_mine_info(mine, pb_btl.mutable_mine_info());
	MineUtils::pack_mine_info_for_client_v2(mine, pb_btl.mutable_mine_info());
	pb_btl.set_nick(player->nick);
	pb_btl.set_fight_time(NOW());
	pb_btl.set_result(result);
	onlineproto::mine_lost_num_info_list_t* pb_ptr = pb_btl.mutable_lost_list();
	FOREACH(rob_res_vec, it) {
		onlineproto::mine_lost_num_info_t* pb_ptr02 = pb_ptr->add_lost_info();
		TRACE_TLOG("Generate Btl Report,uid=%u,create_tm=%u, rob_num=%u", it->userid, it->u_create_tm, it->rob_num);
		pb_ptr02->set_uid(it->userid);
		pb_ptr02->set_create_tm(it->u_create_tm);
		pb_ptr02->set_lost_money(it->rob_num);
	}
	pb_btl.SerializeToString(&pkg);
	return 0;
}

uint32_t MineUtils::set_pets_defend_mine_id(player_t* player, uint64_t mine_id,
		std::vector<uint32_t>& pet_ctm_vec)
{
	FOREACH(*player->pets, iter) {
		Pet& pet = iter->second;
		uint32_t create_tm = pet.create_tm();
		std::vector<uint32_t>::iterator it;
		it = std::find(pet_ctm_vec.begin(), pet_ctm_vec.end(), create_tm);
		if (it != pet_ctm_vec.end()) {
			pet.set_defend_mine_id(mine_id);
			PetUtils::save_pet(player, pet, false, true);
		}
	}
	return 0;
}

uint32_t MineUtils::calc_mine_product_resource_speed(uint32_t def_pets_total_power,
		mine_type_t type, mine_size_t size)
{
	uint32_t conf_array_1[] = {300, 600, 1000, 50, 40, 12, 1, 2, 5}; 
	uint32_t conf_array_2[] = {80, 60, 40, 1000, 1000, 4000, 40000, 20000, 10000};
	//uint32_t mine_exploit_times[] = {2, 4, 6};
	uint32_t index = 1;
	//TODO kevin 其实这里可以写成 二维数组[][] = { {1, 2, 3}, {4, 5, 6}, {7,8,9}}
	//然后以 mine_type, mine_size 作为索引； 来获得index 的值
	//但是设计的元素数量不多，性能影响忽略不计;故保险点，写成if 逻辑形式
	if (type == MINE_GOLD) {
		if (size == SMALL_MINE) {
			index = 1;
		} else if (size == MID_MINE) {
			index = 2;
		} else {
			index = 3;
		}
	} else if (type == MINE_EXP) {
		if (size == SMALL_MINE) {
			index = 4;
		} else if (size == MID_MINE) {
			index = 5;
		} else {
			index = 6;
		}
	} else {
		if (size == SMALL_MINE) {
			index = 7;
		} else if (size == MID_MINE) {
			index = 8;
		} else {
			index = 9;
		}
	}
	uint32_t mine_speed = floor(conf_array_1[index - 1] + 
			(def_pets_total_power / (conf_array_2[index - 1] * 1.0)));
	return mine_speed;
}

//计算出每个玩家获得物品数量
uint32_t MineUtils::calc_player_get_reward(player_t* player, 
		const std::vector<team_simple_info_t>& def_team_vec,
		mine_info_t& mine_info, mine_reward_type_t reward_type)
{
	uint32_t total_power = 0;
	FOREACH(def_team_vec, it) {
		//计算矿中对应玩家的采矿速度
			total_power = it->total_power;
		
		uint32_t mine_speed = MineUtils::calc_mine_product_resource_speed(
			it->total_power, mine_info.mine_type,
			mine_info.mine_size);
		uint32_t del_times = 0;
		if (reward_type == MINE_TIME_IS_UP) {
			del_times = mine_info.duration_time + mine_info.mine_create_tm;
			if (mine_info.duration_time + mine_info.mine_create_tm < it->exploit_start_time) {
				return 0;
			}
		} else {
			if (mine_info.duration_time + mine_info.mine_create_tm < NOW()) {
				del_times = mine_info.duration_time + mine_info.mine_create_tm;
			} else {
				del_times = NOW();
			}
		}
		if (mine_info.duration_time + mine_info.mine_create_tm)
		if (NOW() < it->exploit_start_time) {
			continue;
		}
		uint32_t mine_time = del_times - it->exploit_start_time;
		uint32_t total_reward = ceil(mine_time * mine_speed / 3600.0);
		TRACE_TLOG("NorMal_Calc Mine Reward,uid=%u,exploit_start_time=[%u]"
				"now_time=[%u],mine_time=[%u],mine_speed=[%u],reward_cnt=[%u],"
				"total_power=[%u]",
				player->userid, it->exploit_start_time, NOW(),
				mine_time, mine_speed, total_reward, total_power);
		const uint32_t protect_time = commonproto::MINE_PROTECT_DURATION;

		uint32_t real_calc_time = 0;
		uint32_t real_rob_num = 0;
		if (reward_type == MINE_IS_BE_OCCUPY) {
			if (mine_info.mine_create_tm + protect_time > it->exploit_start_time) {
				if (mine_info.mine_create_tm + protect_time < NOW()) {
					real_calc_time = NOW() - mine_info.mine_create_tm - protect_time;
				}
			} else {
				if (it->exploit_start_time < NOW()) {
					real_calc_time = NOW() - it->exploit_start_time;
				}
			}
			real_rob_num = floor(mine_speed * real_calc_time / 3600 * 0.4);
			TRACE_TLOG("Mine Be Occupyed,uid=[%u],real_rob_num=[%u], reward_num=[%u]",
					player->userid, real_rob_num, total_reward);
			if (total_reward <= real_rob_num) {
				ERROR_TLOG("Mine Reward Num Can not less than Rob Num"
						" total_reward=[%u], rob_num=[%u],uid=[%u]",
						total_reward, real_rob_num, player->userid);
				total_reward = 0;
			} else {
				total_reward = total_reward - real_rob_num;
			}
		}
		if (reward_type == MINE_GIVE_UP) {
			total_reward = ceil(total_reward / 2.0);
		}

		std::vector<attachment_t> attach_vec;
		struct attachment_t attach;
		//attach.id = MineUtils::get_mine_prize_elem_id(mine_info.mine_type,
		//		mine_info.mine_size);
		if (mine_info.elem_id) {
			attach.id = mine_info.elem_id;
		} else {
			attach.id = MineUtils::get_mine_prize_elem_id(mine_info.mine_type,
					mine_info.mine_size);
		}
		attach.count = total_reward;
		attach.type = kMailAttachItem;
		attach_vec.push_back(attach);
        new_mail_t new_mail;
        new_mail.sender.assign("系统邮件");
		new_mail.title.assign("领取采矿奖励");
		if (reward_type == MINE_TIME_IS_UP) {
			new_mail.content.assign("该矿开采时间已经到了，请领取奖励");
		} else if (reward_type == MINE_IS_BE_OCCUPY) {
			new_mail.content.assign("你的矿被偷袭了，你的小伙伴们全力作战但还是可惜的失败了，但是他们仍然为你夺回了一些奖励，请点击附件领取！");
		} else {
			new_mail.content.assign("你主动放弃了矿，在你的小伙伴们的全力守护下，你收获了一些奖励，请点击附件领取！");
		}
		std::string attachment;
		MailUtils::serialize_attach_to_attach_string(attach_vec, attachment);
		new_mail.attachment = attachment;
		MailUtils::send_mail_to_player(player, it->userid,
				it->u_create_tm, new_mail, false);
	}
	return 0;
}

uint32_t MineUtils::set_mine_attack_state(player_t* player, mine_attack_t type)
{
	if (player->mine_info->mine_id_ == 0) { return 0;}
	struct mine_info_t mine_info;
	player->mine_info->get_match_mine_info_from_memory(
			player->mine_info->mine_id_, mine_info);
	mine_info.is_been_attacked = type;
	//按照函数功能独立性来讲，这个函数功能当type为MINE_NOT_BE_ATTACKED是释放战斗锁
	//但是这个顺便设置了,最近攻打该矿的时间戳，可以避免向DB再发一次包
	if (type == MINE_NOT_BE_ATTACKED) {
		mine_info.last_fight_time = NOW();
	}
	dbproto::cs_update_mine_info db_up_in;
	MineUtils::pack_mine_info(mine_info, db_up_in.mutable_mine_info());
	return g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
			db_cmd_update_mine_info, db_up_in);
}

uint32_t MineUtils::send_pets_start_defend_mine(player_t* player, uint64_t mine_id,
		const std::vector<uint32_t>& pet_ctm_vec)
{
	commonproto::mine_team_info_t pb_team_info;
	MineUtils::pack_mine_team_info(player, pet_ctm_vec, &pb_team_info);
	std::string team_pkg;
	pb_team_info.SerializeToString(&team_pkg);

	std::vector<rankproto::hset_field_t> field_vec;
	uint64_t uid_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	std::string uid_key_str = boost::lexical_cast<std::string>(uid_key);

	rankproto::hset_field_t pb_field;
	pb_field.set_name(uid_key_str);
	pb_field.set_value(team_pkg);
	field_vec.push_back(pb_field);

	uint32_t hashset_type = rankproto::HASHSET_MINE_ID_TEAM_INFO_MAP;
	std::string hashset_type_str = boost::lexical_cast<std::string>(hashset_type);
	std::string sub_type = boost::lexical_cast<std::string>(mine_id);
	return RankUtils::hset_insert_or_update_str_version(
			NULL,  player->userid, player->create_tm,
			g_server_id,
			hashset_type_str, sub_type, rankproto::REDIS_INSERT_OR_UPDATE, 
			&field_vec, DAY_SECS * 7);
}

uint32_t MineUtils::check_mine_num_get_limit(player_t* player)
{
	//开采矿的数量是否到达限制
	std::vector<uint64_t> mine_ids;
	player->mine_info->get_mine_ids_from_memory(mine_ids);
	uint32_t mine_num_limit = 0;
	if (is_vip(player)) {
		mine_num_limit = commonproto::MINE_VIP_MINE_NUM_LIMIT;
	} else {
		mine_num_limit = commonproto::MINE_NO_VIP_MINE_NUM_LIMIT;
	}
	if (mine_ids.size() + 1 > mine_num_limit) {
		return cli_err_mine_num_get_limit;
	}
	return 0;
}

uint32_t MineUtils::mine_modify_player_pet_hp_for_cli(
		commonproto::battle_pet_list_t* btl_ptr,
		const commonproto::battle_pet_list_t& btl_pets) 
{	
	for (int i = 0; i < btl_pets.pet_list_size(); ++i) {
		uint32_t mine_fight_hp = btl_pets.pet_list(i).pet_info().mine_fight_hp();
		uint32_t max_hp = btl_pets.pet_list(i).pet_info().battle_info().max_hp();
		btl_ptr->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_cur_hp(mine_fight_hp * MINE_HP_COE);
		btl_ptr->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_max_hp(max_hp * MINE_HP_COE);

	}
	return 0;
}

uint32_t MineUtils::modify_def_team_btl_result(uint64_t def_role_key,
		commonproto::mine_team_info_list_t* pb_ptr, bool is_defeated)
{
	role_info_t role_info = KEY_ROLE(def_role_key);
	for (int i = 0; i < pb_ptr->team_list_size(); ++i) {
		const commonproto::mine_team_info_t& pb_inf = pb_ptr->team_list(i);
		if (pb_inf.userid() == role_info.userid 
				&& pb_inf.u_create_tm() == role_info.u_create_tm) {

			pb_ptr->mutable_team_list(i)->set_is_defeated(is_defeated);
			break;
		}
	}
	return 0;
}
