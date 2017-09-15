#include <vector>
#include "exped_processor.h"
#include "exped_utils.h"
#include "attr_utils.h"
#include "player.h"
#include "rank_utils.h"
#include "player_utils.h"
#include "data_proto_utils.h"
#include "player_manager.h"
#include "global_data.h"
#include "service.h"
#include "pet_utils.h"
#include "exped.h"
#include "exped_conf.h"
#include "task_utils.h"
#include <boost/lexical_cast.hpp>

int ExpedInfoSceneCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	cli_out_.Clear();

	uint32_t cur_card_id = player->expedtion->get_cur_card_id();
	if (cur_card_id == 0) {
		return PlayerUtils::get_user_raw_data(player, dbproto::EXPED_PETS_INFO);
	}
	bool need_redis = false;
	need_redis = player->expedtion->check_cur_card_kill_state();
	if (need_redis && cur_card_id < EXPED_HIGHEST_CARD_ID
		&& GET_A(kAttrExpedLastPrizeGotCardid) == cur_card_id) {
		//提取出现在已经pk过的玩家id号,避免在redis中读取到重复的
		//根据玩家最高战力的五伙伴之和,读配表范围
		uint32_t n_top_power = 0;
		std::set<uint32_t> create_tm_set;
		PetUtils::get_pets_n_topest_power(
				player, create_tm_set,
				commonproto::PET_N_TOPESET_POWER,
				n_top_power);
		exped_conf_t* ptr = g_exped_mgr.get_exped_conf_info(cur_card_id + 1);
		if (ptr == NULL) {
			RET_ERR(cli_err_exped_card_id_err);
		}
		uint32_t percent = ptr->power_percent / 1000.0 * 100;
		uint32_t low_percent = 0, high_percent = 0;
		if (percent <= 1) {
			low_percent = percent;
		} else {
			low_percent = percent - 1;
		}
		high_percent = percent + 1;
		uint64_t low_score = low_percent / 100.0 * n_top_power;
		uint64_t high_score = high_percent / 100.0 * n_top_power;
		uint64_t unit_power_val = ceil(n_top_power / 100.0);
		std::set<uint64_t> role_set;
		player->expedtion->get_exped_ai_uids(role_set);
		//把自己的也排除
		role_set.insert(ROLE_KEY(ROLE(player->userid, player->create_tm)));	
		return RankUtils::get_users_by_score_range(player,
				commonproto::RANKING_SPIRIT_TOP_5_POWER, 0, 
				low_score, high_score, role_set, unit_power_val);
	}

	//把内存中的信息返回给玩家
	player->expedtion->pack_all_exped_info_to_pbmsg(cli_out_.mutable_users_list());
	DataProtoUtils::pack_player_battle_exped_joined_pet_info(
			player, cli_out_.mutable_pet_list());
	DataProtoUtils::pack_player_battle_exped_fight_pet_info(
			player, cli_out_.mutable_fight_pets());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int ExpedInfoSceneCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	switch(player->serv_cmd) {
		case ranking_cmd_get_users_by_score:
			return proc_pkg_from_serv_aft_get_infos_from_redis(
					player, body, bodylen);
		case db_cmd_user_raw_data_get:
			return proc_pkg_from_serv_aft_get_infos_from_db(
					player, body, bodylen);
		case cache_cmd_ol_req_users_info:
			return proc_pkg_from_serv_aft_get_infos_from_cache(
				player, body, bodylen);
	}
	return 0;
}

int ExpedInfoSceneCmdProcessor::proc_pkg_from_serv_aft_get_infos_from_db(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	onlineproto::expedition_users_pet_info_list pets_info;
	pets_info.ParseFromString(db_out_.raw_data());

	player->expedtion->init_all_cards_exped_info(pets_info);

	bool need_redis = false, need_redis2 = false;
	uint32_t cur_card_id = player->expedtion->get_cur_card_id();
	if (cur_card_id == 0) {
		need_redis = true;
	}
	need_redis2 = player->expedtion->check_cur_card_kill_state();
	if ((need_redis || (need_redis2 && cur_card_id == GET_A(kAttrExpedLastPrizeGotCardid))) && 
			cur_card_id < EXPED_HIGHEST_CARD_ID) {
		//提取出现在已经pk过的玩家id号,避免在redis中读取到重复的
		//根据玩家最高战力的五伙伴之和,读配表范围
		uint32_t n_top_power = 0;
		std::set<uint32_t> create_tm_set;
		PetUtils::get_pets_n_topest_power(
				player, create_tm_set, 
				commonproto::PET_N_TOPESET_POWER, 
				n_top_power);
		exped_conf_t* ptr = g_exped_mgr.get_exped_conf_info(cur_card_id + 1);
		if (ptr == NULL) {
			RET_ERR(cli_err_exped_card_id_err);
		}
		uint32_t percent = ptr->power_percent / 1000.0 * 100;
		uint32_t low_percent = 0, high_percent = 0;
		if (percent <= 1) {
			low_percent = percent;
		} else {
			low_percent = percent - 1;
		}
		high_percent = percent + 1;
		uint64_t low_score = low_percent / 100.0 * n_top_power;
		uint64_t high_score = high_percent / 100.0 * n_top_power;
		std::set<uint64_t> role_set;
		uint64_t unit_power_val = ceil(n_top_power / 100.0);
		player->expedtion->get_exped_ai_uids(role_set);
		//把自己的也排除
		role_set.insert(ROLE_KEY(ROLE(player->userid, player->create_tm)));	
		return RankUtils::get_users_by_score_range(player,
				commonproto::RANKING_SPIRIT_TOP_5_POWER, 0, 
				low_score, high_score, role_set, unit_power_val);
	}

	cli_out_.mutable_users_list()->CopyFrom(pets_info);
	//若不需要走redis，那么就直接返回给前端
	DataProtoUtils::pack_player_battle_exped_joined_pet_info(
			player, cli_out_.mutable_pet_list());
	DataProtoUtils::pack_player_battle_exped_fight_pet_info(
			player, cli_out_.mutable_fight_pets());

	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int ExpedInfoSceneCmdProcessor::proc_pkg_from_serv_aft_get_infos_from_redis(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	uint32_t ai_id = rank_out_.player_info().userid();
	uint32_t ai_create_tm = rank_out_.player_info().u_create_tm();

	//远征玩法，决定了，不要去内存中拉取ai对手的实时信息
	//去缓存中拉取精灵信息
	cacheproto::cs_batch_get_users_info cache_info_in_;
	commonproto::role_info_t* pb_ptr = cache_info_in_.add_roles();
	pb_ptr->set_userid(ai_id);
	pb_ptr->set_u_create_tm(ai_create_tm);
	return g_dbproxy->send_msg(player, player->userid, player->create_tm,
			cache_cmd_ol_req_users_info, cache_info_in_);
}

int ExpedInfoSceneCmdProcessor::proc_pkg_from_serv_aft_get_infos_from_cache(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(cache_info_out_);
	uint32_t cache_info_size = cache_info_out_.user_infos_size();
	if (cache_info_size == 0) {
		return send_err_to_player(
			player, player->cli_wait_cmd,
			cli_err_exped_can_not_get_player_info);
	}
	//==========更新精灵cur_hp为max_hp=============
	commonproto::battle_pet_list_t* tmp_pb_btl_ptr = cache_info_out_.mutable_user_infos(0)->mutable_power_pet_list();
	const commonproto::battle_pet_list_t& tmp_pb_btl_inf = cache_info_out_.user_infos(0).power_pet_list();
	for (int i = 0; i < tmp_pb_btl_inf.pet_list_size(); ++i) {
		uint32_t max_hp = tmp_pb_btl_inf.pet_list(i).pet_info().battle_info().max_hp();
		tmp_pb_btl_ptr->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_cur_hp(max_hp);
	}
	//=======================

	const commonproto::battle_player_data_t &player_info = cache_info_out_.user_infos(0);
	//Confirm kevin: 匹配到新的对手并成功获取其数据，在此将card_id增一，并将数据添加进内存
	player->expedtion->add_new_card_exped_info(player_info);

	player->expedtion->btl_pet_info_.clear();
	player_info.power_pet_list().SerializeToString(&player->expedtion->btl_pet_info_);
	//打包精灵详细到buff 
	PlayerUtils::update_user_raw_data(
			player->userid, player->create_tm, dbproto::EXPED_CUR_CARD_PETS,
			player_info.power_pet_list(), "0");

	// 下面是将当前所有关卡信息打包给前端
	player->expedtion->pack_all_exped_info_to_pbmsg(cli_out_.mutable_users_list());
	// 将所有关卡信息保存至DB
	PlayerUtils::update_user_raw_data(player->userid, player->create_tm, 
			dbproto::EXPED_PETS_INFO, cli_out_.users_list(), "0");

	//打包我参加远征的伙伴信息和参战的伙伴信息
	DataProtoUtils::pack_player_battle_exped_joined_pet_info(
			player, cli_out_.mutable_pet_list());
	DataProtoUtils::pack_player_battle_exped_fight_pet_info(
			player, cli_out_.mutable_fight_pets());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	

}

int ExpedStartCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	bool need_redis = false;
	need_redis = player->expedtion->check_cur_card_kill_state();
	if (need_redis) {
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_exped_cur_card_ai_has_dead);
	}
	uint32_t cur_card_id = player->expedtion->get_cur_card_id();
	if (cur_card_id == 0) {
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_exped_must_fight_after_into_scene);
	}

    // 悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_EXPDITION, 1);

	cli_out_.Clear(); 
	exped_start_tick_t* session = (exped_start_tick_t*)player->session;
	memset(session, 0, sizeof(*session));
	session->atk_tick = cli_in_.atk_tick();
	session->def_tick = cli_in_.def_tick();

	std::vector<uint32_t> create_tm_vec;
	for (int i = 0; i < cli_in_.create_tm_size(); ++i) {
		uint32_t create_tm = cli_in_.create_tm(i);
		create_tm_vec.push_back(create_tm);
	}
	uint32_t ret = PetUtils::pick_pets_into_exped_fight(player, create_tm_vec);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	//没打死当前关卡的ai,在下一次上线后，会走到下面的if里
	if (player->expedtion->btl_pet_info_.empty()) {
		//去缓存中拉取精灵信息
		return PlayerUtils::get_user_raw_data(player, dbproto::EXPED_CUR_CARD_PETS);
	}
	//Confirm kevin: btl_pet_info 不为空
	player->expedtion->pack_cur_card_exped_info(cli_out_.mutable_pet_info());
	//自己参战的精灵
	DataProtoUtils::pack_player_battle_exped_fight_pet_info(
			player, cli_out_.mutable_fight_pets());
	cli_out_.set_atk_tick(session->atk_tick);
	cli_out_.set_def_tick(session->def_tick);

	std::string str = "进入第" + boost::lexical_cast<std::string>(cur_card_id) + "激战副本";
	Utils::write_msglog_new(player->userid, "功能", "伙伴激战", str);

	//根据前端要求，回包之前将exped_hp 设置给 hp
	//warning:仅仅回包给前端时使用，不可保存到内存以及持久化到数据库
	ExpedUtils::exped_modify_ai_pet_hp_for_cli(
			*(cli_out_.mutable_pet_info()->mutable_power_pet_list()), 
			cli_out_.pet_info().exped_pet_list());
	ExpedUtils::exped_modify_player_pet_hp_for_cli(
			cli_out_.mutable_fight_pets(), 
			cli_out_.fight_pets());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int ExpedStartCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	commonproto::battle_pet_list_t pb_btl_pets;
	pb_btl_pets.ParseFromString(db_out_.raw_data());
	player->expedtion->btl_pet_info_.clear();
	pb_btl_pets.SerializeToString(&player->expedtion->btl_pet_info_);

	//Confirm kevin: btl_pet_info 不为空
	player->expedtion->pack_cur_card_exped_info(cli_out_.mutable_pet_info());
	//自己参战的精灵
	DataProtoUtils::pack_player_battle_exped_fight_pet_info(
			player, cli_out_.mutable_fight_pets());

	exped_start_tick_t* session = (exped_start_tick_t*)player->session;
	cli_out_.set_atk_tick(session->atk_tick);
	cli_out_.set_def_tick(session->def_tick);

	uint32_t cur_card_id = player->expedtion->get_cur_card_id();
	std::string str = "进入第" + boost::lexical_cast<std::string>(cur_card_id) + "激战副本";
	Utils::write_msglog_new(player->userid, "功能", "伙伴激战", str);

	//根据前端要求，回包之前将exped_hp 设置给 hp
	//warning:仅仅回包给前端时使用，不可保存到内存以及持久化到数据库
	ExpedUtils::exped_modify_ai_pet_hp_for_cli(
			*(cli_out_.mutable_pet_info()->mutable_power_pet_list()), 
			cli_out_.pet_info().exped_pet_list());
	ExpedUtils::exped_modify_player_pet_hp_for_cli(
			cli_out_.mutable_fight_pets(), 
			cli_out_.fight_pets());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int ExpedResultCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	/*
	if (GET_A(kAttrLv) < 49) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_level_limit_can_not_exped_start);
	}
	*/
	commonproto::challenge_result_type_t result_type = cli_in_.type();
	//如果玩家赢了，保存玩家的远征精灵血量，并跟新对方被干掉的信息到buff中
	if (result_type == commonproto::WIN) {
		uint32_t ret = PetUtils::update_pets_expedtion_hp(player, cli_in_.cur_hp_list());	
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
		player->expedtion->set_killed();
		player->expedtion->clear_ai_pets_hp(player);
		uint32_t cur_card_id = player->expedtion->get_cur_card_id();
		std::string str = "完成第" + boost::lexical_cast<std::string>(cur_card_id) + "激战副本";
		Utils::write_msglog_new(player->userid, "功能", "伙伴激战", str);
		//更新历史通过的最高关卡
		if (cur_card_id > GET_A(kAttrExpeditionUnlockCardid)) {
			SET_A(kAttrExpeditionUnlockCardid, cur_card_id);
		}
	} else if (result_type == commonproto::QUIT) {
		PetUtils::deal_pets_after_died_in_exped(player);
	} else {
		//如果对手赢了，将玩家出战的精灵血量设置为0，以及由出战状态改为参加远征状态
		PetUtils::deal_pets_after_died_in_exped(player);
		//并更新对手的血量信息
		uint32_t ret = player->expedtion->change_ai_pets_hp(player, cli_in_.cur_hp_list());
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
	}
	AttrUtils::add_attr_in_special_time_range(player, 
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,		
			kAttrActivExpedCnt);

	ExpedUtils::pack_all_exped_info_and_update_db(player);

	DataProtoUtils::pack_player_battle_exped_joined_pet_info(
			player, cli_out_.mutable_pet_list());

	//根据前端要求，回包之前将exped_hp 设置给 hp
	//warning:仅仅回包给前端时使用，不可保存到内存以及持久化到数据库
	ExpedUtils::exped_modify_player_pet_hp_for_cli(
			cli_out_.mutable_pet_list(), 
			cli_out_.pet_list());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int ExpedPickJoinedPetCmdProcessor::proc_pkg_from_client(
	player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	//检查是否存在这些精灵
	std::vector<uint32_t> create_tm_vec;
	for (int i = 0; i < cli_in_.create_tm_size(); ++i) {
		create_tm_vec.push_back(cli_in_.create_tm(i));
	}
	if (create_tm_vec.size() > EXPED_JOINED_PETS_NUM) {
		return send_err_to_player(
			player, player->cli_wait_cmd, cli_err_exped_joined_pest_num_exceed);
	}
	uint32_t ret = PetUtils::pick_pets_joined_exped(player, create_tm_vec);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	cli_out_.Clear();
	DataProtoUtils::pack_player_battle_exped_joined_pet_info(
			player, cli_out_.mutable_pet_list());
		
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int ExpedResetCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	if (GET_A(kDailyExpedReset) >= EXPED_RESET_CNT_LIMIT) {
		RET_ERR(cli_err_exped_reset_cnt_get_limit);
	}
	//清除raw_data中所有的信息
	player->expedtion->clear_exped_info(player);
	//清除玩家精灵的远征信息
	PetUtils::deal_pets_after_reset_exped(player);
	SET_A(kAttrExpedLastPrizeGotCardid, 0);
	ADD_A(kDailyExpedReset, 1);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int ExpedPrizeTotalProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PlayerUtils::get_user_raw_data(player, dbproto::EXPED_TOTAL_PRIZE);
	return 0;
}

int ExpedPrizeTotalProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	cli_out_.Clear();
	cli_out_.mutable_list()->ParseFromString(db_out_.raw_data());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}
