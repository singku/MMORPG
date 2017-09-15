#include "mine_processor.h"
#include "player.h"
#include "mine_utils.h"
#include "player_utils.h"
#include "rank_utils.h"
#include "pet_utils.h"
#include "service.h"
#include "mail.h"
#include "mail_utils.h"
#include "player_manager.h"
#include <boost/lexical_cast.hpp>

int SearchMineCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	cli_out_.Clear();
	search_mine_session_t* session = (search_mine_session_t*)player->session;
	memset(session, 0, sizeof(*session));
	std::vector<uint64_t> my_mine_ids;
	player->mine_info->get_mine_ids_from_memory(my_mine_ids);

	std::vector<uint64_t> match_ids;
	player->mine_info->get_match_mine_ids_from_memory(match_ids);
	//重新搜索矿
	if (GET_A(kDailyMineRefreshTimes) == 0 || match_ids.empty()) {
		session->search_step = MATCH_NEW_OCCUPYED_MINE;

		PlayerUtils::delete_user_raw_data(player->userid,
				player->create_tm, dbproto::MINE_FIGHT_DEF_PET_HP, "0");
		//恢复精灵血量,与清除我更在攻打别人矿mine_flag记录
		MineUtils::reset_player_pets_attack_mine_info_relate(player);
		return MineUtils::req_dbsvr_to_search_mine(player, my_mine_ids);
	}
	
	//下面的逻辑是更新已经搜索到的待攻占的矿的信息
	//该矿有可能已经过期，而被该矿的矿主删除而收不到
	session->search_step = JUST_REFRESH_MINE_INFO;
	return MineUtils::get_player_mine_info_from_db(player, match_ids);
}

int SearchMineCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	switch (player->serv_cmd) {
		case db_cmd_search_occupyed_mine:
				return proc_pkg_from_serv_aft_search_mine_info(player, body, bodylen);
		case db_cmd_get_player_mine_info:
				return proc_pkg_from_serv_aft_open_mine_panel(player, body, bodylen);
		case db_cmd_user_raw_data_get:
			return proc_pkg_from_serv_aft_get_team_defeate_state(player, body, bodylen);
		case ranking_cmd_hset_get_info:
			search_mine_session_t* session = (search_mine_session_t*)player->session;
			search_step_t step = session->search_step;
			if (step == SEARCH_NEW_PET_INFO) {
				return proc_pkg_from_serv_aft_get_mine_pets_info(player, body, bodylen);
			} else if (step == JUST_REFRESH_PET_INFO) {
				return proc_pkg_from_serv_aft_refresh_pet_info(player, body, bodylen);
			}
	}
	return 0;
}

int SearchMineCmdProcessor::proc_pkg_from_serv_aft_search_mine_info(player_t* player,
		const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	player->mine_info->clear_mine_tmp_info();
	ADD_A(kDailyMineRefreshTimes, 1);
	uint32_t has_searched_cnt = db_out_.list().mine_info_size();
	if (has_searched_cnt == 0) {
		//在DB没有找到匹配的战力玩家，故随机出5个新矿给前端
		MineUtils::save_new_mine_without_garrison(player, NEED_SEARCH_MINE_COUNT);
		MineTmpInfoMap mine_tmp_info_map;
		player->mine_info->get_mine_tmp_map(mine_tmp_info_map);
		FOREACH(mine_tmp_info_map, it) {
			MineUtils::pack_mine_info_for_client_v2(it->second, cli_out_.add_mine_info());
		}
		//并给客端回包，结束该协议
		return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
	}

	uint32_t match_count = std::min(has_searched_cnt, MATCH_MINE_ID_COUNT);
	std::vector<uint32_t> index_vec;
	//随机出match_count 个矿
	Utils::rand_select_k(0, has_searched_cnt - 1, match_count, index_vec);
	//做个保护性措施
	if (index_vec.size() > match_count) {
		RET_ERR(cli_err_search_mine_err);
	}
	
	search_mine_session_t* session = (search_mine_session_t*)player->session;
	std::vector<uint64_t> mine_id_vec;
	for (uint32_t i = 0; i < match_count; ++i) {
		//DB中搜索出来的矿，经过随机挑选后，现在保存到内存中
		//并用这些矿id去redis中请求这些矿中的队伍信息
		const commonproto::mine_info_t& inf = db_out_.list().mine_info(index_vec[i]); 
		mine_id_vec.push_back(inf.mine_id());
		MineUtils::unpack_mine_info(inf, session->mine[i]);
	}
	session->search_step = SEARCH_NEW_PET_INFO;
	//去redis中获得待占领矿中的防守方战斗数据信息
	return MineUtils::req_redis_to_get_teams_info(player, mine_id_vec);
}

int SearchMineCmdProcessor::proc_pkg_from_serv_aft_get_mine_pets_info(player_t* player,
		const char* body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	//因为是重新搜矿，所以清除掉以前匹配到的对手矿信息
	player->mine_info->clear_match_mine_info();
	player->mine_info->clear_match_mine_ids();

	search_mine_session_t* session = (search_mine_session_t*)player->session;
	for (int i = 0; i < rank_out_.hset_infos_size(); ++i) {
		const rankproto::hset_info_t& inf = rank_out_.hset_infos(i);
		std::vector<string> list = split(inf.key(), ':');
		uint64_t mine_id = boost::lexical_cast<uint32_t>(list[1]);
		for (uint32_t j = 0; j < NEED_SEARCH_MINE_COUNT; ++j) {
			struct mine_info_t& mine = session->mine[j];
			if (mine.mine_id == mine_id) {
				//从redis中找到与内存中mind_id相同的矿，打包该矿下每个队伍的信息给前段
				//并重新将该矿 作为 匹配矿的结构 保存到内存中
				commonproto::mine_info_t* mine_ptr = cli_out_.add_mine_info();
				MineUtils::pack_mine_info_for_client_v2(mine, mine_ptr);
				for (int m = 0; m < inf.fields_size(); ++m) {
					commonproto::mine_team_info_t pb_team_info;
					pb_team_info.ParseFromString(inf.fields(m).value());
					mine_ptr->mutable_team_info()->add_team_list()->CopyFrom(pb_team_info);
				}
				player->mine_info->save_matched_mine_info_to_memory(mine);
				player->mine_info->save_match_mine_ids_to_memory(mine.mine_id);
				//处理完后，跳出本次
				break;
			}
		}
	}
	MineUtils::sync_match_mine_ids_to_db_rawdata(player);
	if (NEED_SEARCH_MINE_COUNT < (uint32_t)rank_out_.hset_infos_size()) {
		RET_ERR(cli_err_sys_err);
	}
	//需要生成新矿的数量
	uint32_t new_mine_count = NEED_SEARCH_MINE_COUNT - rank_out_.hset_infos_size();
	//生成新矿
	MineUtils::save_new_mine_without_garrison(player, new_mine_count);
	MineTmpInfoMap mine_tmp_info_map;
	player->mine_info->get_mine_tmp_map(mine_tmp_info_map);
	FOREACH(mine_tmp_info_map, it) {
		MineUtils::pack_mine_info_for_client_v2(it->second, cli_out_.add_mine_info());
	}
	//删除伙伴身上的守矿信息，（由别的玩家触发删除掉那些矿）
	//别的玩家触发删除掉那些矿，那么我就在这里清除我的伙伴守矿记录
	std::vector<uint64_t> my_mine_ids;
	player->mine_info->get_mine_ids_from_memory(my_mine_ids);
	if (my_mine_ids.empty()) {
		FOREACH(*player->pets, iter) {
			Pet& pet = iter->second;
			if (pet.defend_mine_id()) {
				pet.set_defend_mine_id(0);
				PetUtils::save_pet(player, pet, false, true);
			}
		}
	} else {
		FOREACH(*player->pets, iter) {
			Pet& pet = iter->second;
			std::vector<uint64_t>::iterator it;
			if (pet.defend_mine_id() == 0) {
				continue;
			}
			it = std::find(my_mine_ids.begin(), my_mine_ids.end(), pet.defend_mine_id());
			if (it == my_mine_ids.end()) {
				pet.set_defend_mine_id(0);
				PetUtils::save_pet(player, pet, false, true);
			}
		}
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int SearchMineCmdProcessor::proc_pkg_from_serv_aft_open_mine_panel(player_t* player,
		const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_player_mine_out_);
	search_mine_session_t* session = (search_mine_session_t*)player->session;
	player->mine_info->clear_match_mine_info();
	player->mine_info->clear_match_mine_ids();
	const commonproto::mine_info_list_t& pb_list = db_player_mine_out_.list();
	std::vector<uint64_t> mine_id_vec;
	for (int i = 0; i < pb_list.mine_info_size(); ++i) {
		MineUtils::unpack_mine_info(pb_list.mine_info(i), session->mine[i]);
		mine_id_vec.push_back(pb_list.mine_info(i).mine_id());
	}
	session->search_step = JUST_REFRESH_PET_INFO;
	return MineUtils::req_redis_to_get_teams_info(player, mine_id_vec);
}

int SearchMineCmdProcessor::proc_pkg_from_serv_aft_refresh_pet_info(player_t* player,
		const char* body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	search_mine_session_t* session = (search_mine_session_t*)player->session;
	for (int i = 0; i < rank_out_.hset_infos_size(); ++i) {
		const rankproto::hset_info_t& inf = rank_out_.hset_infos(i);
		std::vector<string> list = split(inf.key(), ':');
		uint64_t mine_id = boost::lexical_cast<uint32_t>(list[1]);
		for (uint32_t j = 0; j < NEED_SEARCH_MINE_COUNT; ++j) {
			struct mine_info_t& mine_info = session->mine[j];
			if (mine_info.mine_id == mine_id) {
				commonproto::mine_info_t* mine_ptr = cli_out_.add_mine_info();
				MineUtils::pack_mine_info_for_client_v2(mine_info, mine_ptr);
				for (int m = 0; m < inf.fields_size(); ++m) {
					commonproto::mine_team_info_t pb_team_info;
					pb_team_info.ParseFromString(inf.fields(m).value());
					mine_ptr->mutable_team_info()->add_team_list()->CopyFrom(pb_team_info);
				}
				player->mine_info->save_matched_mine_info_to_memory(mine_info);
				player->mine_info->save_match_mine_ids_to_memory(mine_info.mine_id);
				break;
			}
		}
	}
	MineUtils::sync_match_mine_ids_to_db_rawdata(player);
	if (NEED_SEARCH_MINE_COUNT < (uint32_t)rank_out_.hset_infos_size()) {
		RET_ERR(cli_err_sys_err);
	}

	//生成新矿
	MineTmpInfoMap mine_tmp_map;
	player->mine_info->get_mine_tmp_map(mine_tmp_map);
	FOREACH(mine_tmp_map, it) {
		MineUtils::pack_mine_info_for_client_v2(it->second, cli_out_.add_mine_info());
	}

	//删除过期矿的中伙伴身上的守矿信息
	//别的玩家触发删除掉那些矿，那么我就在这里清除我的伙伴守矿记录
	std::vector<uint64_t> my_mine_ids;
	player->mine_info->get_mine_ids_from_memory(my_mine_ids);
	if (my_mine_ids.empty()) {
		FOREACH(*player->pets, iter) {
			Pet& pet = iter->second;
			if (pet.defend_mine_id()) {
				pet.set_defend_mine_id(0);
				PetUtils::save_pet(player, pet, false, true);
			}
		}
	} else {
		FOREACH(*player->pets, iter) {
			Pet& pet = iter->second;
			std::vector<uint64_t>::iterator it;
			if (pet.defend_mine_id() == 0) {
				continue;
			}
			it = std::find(my_mine_ids.begin(), my_mine_ids.end(), pet.defend_mine_id());
			if (it == my_mine_ids.end()) {
				pet.set_defend_mine_id(0);
				PetUtils::save_pet(player, pet, false, true);
			}
		}
	}
	return PlayerUtils::get_user_raw_data(player, dbproto::MINE_FIGHT_DEF_PET_HP, "0");
}

int SearchMineCmdProcessor::proc_pkg_from_serv_aft_get_team_defeate_state(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_raw_date_out_);
	onlineproto::team_hp_info_list_t pb_raw_data;
	pb_raw_data.ParseFromString(db_raw_date_out_.raw_data());
	std::string debug_str = pb_raw_data.Utf8DebugString();
	std::string name = pb_raw_data.GetTypeName();
	commonproto::mine_team_info_list_t* pb_ptr;
	bool found_mine_id = false;
	for (int i = 0; i < cli_out_.mine_info_size(); ++i) {
		if (cli_out_.mine_info(i).mine_id() == pb_raw_data.mine_id()) {
			found_mine_id = true;	
			pb_ptr = cli_out_.mutable_mine_info(i)->mutable_team_info();
			break;
		}
	}

	if (found_mine_id) {
		for (int j = 0; j < pb_raw_data.team_list_size(); ++j) {
			const onlineproto::team_hp_info_t& pb_inf = pb_raw_data.team_list(j);
			for (int m = 0; m < (*pb_ptr).team_list_size(); ++m) {
				if ((*pb_ptr).team_list(m).userid() == pb_inf.user_id() &&
						(*pb_ptr).team_list(m).u_create_tm() == pb_inf.create_tm()) {
					if (pb_inf.fight_state() == onlineproto::MINE_HAS_DEAD) {
						TRACE_TLOG("This Team Is Defeated,uid=%u,ctm=%u",
								pb_inf.user_id(), pb_inf.create_tm());
						pb_ptr->mutable_team_list(m)->set_is_defeated(true);
					} else {
						pb_ptr->mutable_team_list(m)->set_is_defeated(false);
					}
					//跳出m循环
					break;
				}
			}
		}
	}
	//判断是否足够了5个，如果不满5个，从玩家自己的矿中补上，发给前端
	if (cli_out_.mine_info_size() < (int)NEED_SEARCH_MINE_COUNT) {
		uint32_t need_count = NEED_SEARCH_MINE_COUNT - cli_out_.mine_info_size();
		std::vector<uint64_t> my_mine_ids;
		player->mine_info->get_mine_ids_from_memory(my_mine_ids);
		uint32_t cnt = std::min(need_count, (uint32_t)my_mine_ids.size());
		for (uint32_t i = 0; i < cnt; ++i) {
			struct mine_info_t mine;
			player->mine_info->get_mine_info_from_memory(my_mine_ids[i], mine);
			commonproto::mine_info_t* pb_mine_ptr = cli_out_.add_mine_info();
			MineUtils::pack_mine_info_for_client_v2(mine, pb_mine_ptr);
			pb_mine_ptr->set_is_occupy_by_me(true);
		}
	}

	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int ExploitNewMineCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	struct mine_info_t mine;
	uint64_t tmp_mine_id = 0;
	try {
		tmp_mine_id = boost::lexical_cast<uint64_t>(cli_in_.str_index());
	} catch (const boost::bad_lexical_cast &) {
		RET_ERR(cli_err_you_are_hacker);
	}

	if (GET_A(kDailyMineHasChallengeTimes) >= 
			GET_A(kDailyMineBuyChallengeTimes) + commonproto::MINE_DAILY_OCCUPY_MINE_TIMES) {
		RET_ERR(cli_err_daily_occupy_mine_times_get_limit);
	}
	mine_id_t* session = (mine_id_t*)player->session;
	memset(session, 0x0, sizeof(*session));
	session->tmp_mine_id = tmp_mine_id;
	uint32_t ret = player->mine_info->get_mine_tmp_info(tmp_mine_id, mine);
	if (ret) {
		RET_ERR(ret);
	}
	//TODO kevin 正在攻打他矿的伙伴，不应该被派来守矿，如果存在，应该提示玩家，是否放弃攻打他矿

	for (int i = 0; i < cli_in_.create_tm_size(); ++i) {
		//pet_create_tm.push_back(cli_in_.create_tm(i));
		Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm(i));
		if (pet == NULL) {
			RET_ERR(cli_err_bag_pet_not_exist);
		}
		if (pet->defend_mine_id()) {
			RET_ERR(cli_err_exploiting_pet_exploit_other_mine);
		}
	}
	ret = MineUtils::check_mine_num_get_limit(player);
	if (ret) {
		RET_ERR(ret);
	}

	//记录开始采矿的时间
	mine.mine_create_tm = NOW();
	player->mine_info->save_mine_tmp_info_to_memory(tmp_mine_id, mine);
	dbproto::cs_save_mine_info db_in;
	MineUtils::pack_mine_info(mine, db_in.mutable_mine_info());
	return g_dbproxy->send_msg(player, player->userid, player->create_tm,
			db_cmd_save_one_new_mine, db_in);	
}

int ExploitNewMineCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	mine_id_t* session = (mine_id_t*)player->session;
	struct mine_info_t mine;
	player->mine_info->get_mine_tmp_info(session->tmp_mine_id, mine);
	mine.mine_id = db_out_.mine_id();
	uint32_t ret = player->mine_info->insert_player_mine_info_to_memory(
			mine.mine_id, mine);
	if (ret) {
		RET_ERR(ret);
	}
	ret = MineUtils::insert_mine_id_and_sync_db_rawdata(player, mine.mine_id);
	if (ret) {
		player->mine_info->delete_player_mine_info_from_memory(mine.mine_id);
		RET_ERR(ret);
	}
	std::vector<uint32_t> pet_create_tm;
	for (int i = 0; i < cli_in_.create_tm_size(); ++i) {
		pet_create_tm.push_back(cli_in_.create_tm(i));
	}
	ret = MineUtils::send_pets_start_defend_mine(player,
			mine.mine_id, pet_create_tm);
	if (ret) {
		MineUtils::del_mine_id_and_sync_db_rawdata(player, mine.mine_id);
		RET_ERR(ret);
	}
	MineUtils::set_pets_defend_mine_id(player, mine.mine_id, pet_create_tm);
	
	commonproto::mine_info_t *mine_ptr = cli_out_.mutable_mine_info();
	MineUtils::pack_mine_info_for_client_v2(mine, mine_ptr);
	ADD_A(kDailyMineHasChallengeTimes, 1);
	//后台帮前端计算的left_times
	uint32_t left_times = GET_A(kDailyMineBuyChallengeTimes) + 
		commonproto::MINE_DAILY_OCCUPY_MINE_TIMES - GET_A(kDailyMineHasChallengeTimes);
	if (GET_A(kDailyMineHasChallengeTimes) >= 
			GET_A(kDailyMineBuyChallengeTimes) + commonproto::MINE_DAILY_OCCUPY_MINE_TIMES )
	{
		left_times = 0;
	}

	SET_A(kDailyMineLeftChallengeTimes, left_times);

	//已经开采的矿，从我的临时待开采的矿场中删除
	player->mine_info->erase_single_mine_from_mine_tmp_info(session->tmp_mine_id);
	//TODO kevin 并且序列化到dbproto::raw_data中
	MineTmpInfoMap mine_tmp_info_map;
	player->mine_info->get_mine_tmp_map(mine_tmp_info_map);
	commonproto::mine_info_list_t pb_new_mine_info;
	FOREACH(mine_tmp_info_map, it) {
		commonproto::mine_info_t* pb_ptr = pb_new_mine_info.add_mine_info();
		MineUtils::pack_mine_info(it->second, pb_ptr);
	}
	if (!mine_tmp_info_map.empty()) {
		PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
				dbproto::NEW_MINE_INFO, pb_new_mine_info, "0");
	}

	ADD_A(kDailyMineHasExploitMineCnt, 1);
	uint32_t exploit_cnt = GET_A(kDailyMineHasExploitMineCnt);
	std::string stat_name = "每日占" + Utils::to_string(exploit_cnt) + "次矿人数";
	Utils::write_msglog_new(player->userid, "藏宝矿山", "矿战", stat_name);

	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int GetMyMineInfoCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	std::vector<uint64_t> mine_ids;
	player->mine_info->get_mine_ids_from_memory(mine_ids);
	return MineUtils::get_player_mine_info_from_db(player, mine_ids);
}

int GetMyMineInfoCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	switch(player->serv_cmd) {
		case db_cmd_get_player_mine_info:
			return proc_pkg_from_serv_aft_get_mine_info(player, body, bodylen);
		case ranking_cmd_hset_get_info:
			return proc_pkg_from_serv_aft_get_defend_pets_info(player, body, bodylen);
	}
	return 0;
}

int GetMyMineInfoCmdProcessor::proc_pkg_from_serv_aft_get_mine_info(
		player_t *player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	std::vector<uint64_t> tmp_mine_id_vec;
	//清掉内存中我的矿场id及其对应的信息，重新赋值（因为db中拉取出来的是最新的矿id以及对应的数据）
	//ps : db中拉取出来的矿，可能已经过期；
	player->mine_info->clear_player_mine_info_from_memory();
	const commonproto::mine_info_list_t& pb_list = db_out_.list();
	for (int i = 0; i < pb_list.mine_info_size(); ++i) {
		struct mine_info_t mine_info;
		MineUtils::unpack_mine_info(pb_list.mine_info(i), mine_info);
		//检查是否是过期矿，过期矿需要删除
		player->mine_info->insert_player_mine_info_to_memory(
				mine_info.mine_id, mine_info);

		tmp_mine_id_vec.push_back(mine_info.mine_id);
	}
	std::vector<uint64_t> mine_ids;
	player->mine_info->get_mine_ids_from_memory(mine_ids);
	//说明有矿过期已经被删除了
	//1. 矿时间到后，被其他协防此矿的玩家触发清除的;2. 是被他人攻占了
	if (mine_ids.size() != tmp_mine_id_vec.size()) {
		player->mine_info->clear_my_mine_ids();
		FOREACH(tmp_mine_id_vec, it) {
			player->mine_info->insert_my_mine_ids_to_memory(*it);
		}
		MineUtils::sync_mine_ids_to_db(player);
		mine_ids.clear();
		player->mine_info->get_mine_ids_from_memory(mine_ids);
	}
	//去redis中拉取出我所有矿场中的精灵信息
	return MineUtils::req_redis_to_get_teams_info(player, mine_ids);
}

int GetMyMineInfoCmdProcessor::proc_pkg_from_serv_aft_get_defend_pets_info(
		player_t *player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	cli_out_.Clear();
	//保存redis中临时的矿id
	std::vector<uint64_t> expire_mine_vec;
	for (int i = 0; i < rank_out_.hset_infos_size(); ++i) {
		const rankproto::hset_info_t& inf = rank_out_.hset_infos(i);
		std::vector<string> list = split(inf.key(), ':');
		uint64_t mine_id = boost::lexical_cast<uint32_t>(list[1]);
		struct mine_info_t mine;
		uint32_t ret = player->mine_info->get_mine_info_from_memory(mine_id, mine);
		if (ret) {
			ERROR_TLOG("Mine id Not Found In Memory,id:%u", mine_id);
			continue;
		}
		//对矿过期的处理
		if (mine.mine_create_tm + mine.duration_time < NOW()) {
			expire_mine_vec.push_back(mine.mine_id);

			std::vector<team_simple_info_t> tmp_def_team_vec;
			for (int index = 0; index < inf.fields_size(); ++index) {
				//能走到这里，说明，该矿虽然过期，但是存储在redis中的ttl时间依然在
				//从redis中拉取出该矿中每个玩家的信息
				commonproto::mine_team_info_t pb_team_info;
				pb_team_info.ParseFromString(inf.fields(index).value());
				team_simple_info_t  team_info;
				team_info.userid = pb_team_info.userid();
				team_info.u_create_tm = pb_team_info.u_create_tm();
				team_info.exploit_start_time = pb_team_info.exploit_start_time();
				const commonproto::pet_list_t& pb_pet_inf = pb_team_info.pet_list();
				//计算矿中该玩家的战力和
				uint32_t def_total_power = 0;
				for (int j = 0; j < pb_pet_inf.pet_list_size(); ++j) {
					uint32_t power = pb_pet_inf.pet_list(j).base_info().power();
					def_total_power += power;
				}
				team_info.total_power = def_total_power;
				tmp_def_team_vec.push_back(team_info);
			}
			if (!tmp_def_team_vec.empty()) {
				//给该矿所有玩家发奖励邮件
				MineUtils::calc_player_get_reward(player,
						tmp_def_team_vec, mine, MINE_TIME_IS_UP);
				//删除我在此矿中的精灵信息
				player->mine_info->delete_pet_info_in_memory(mine.mine_id);
				//MineUtils::sync_mine_pet_info_to_hset(player, mine.mine_id);
				//删除共同守矿的每个人的精灵信息
				std::vector<rankproto::hset_field_t> field_vec;
				FOREACH(tmp_def_team_vec, iter) {
					uint64_t uid_key = ROLE_KEY(ROLE(iter->userid, iter->u_create_tm));
					std::string uid_key_str = boost::lexical_cast<std::string>(uid_key);
					rankproto::hset_field_t pb_field_info;
					pb_field_info.set_name(uid_key_str);
					field_vec.push_back(pb_field_info);
				}
				uint32_t hashset_type = rankproto::HASHSET_MINE_ID_TEAM_INFO_MAP;
				std::string hashset_type_str = boost::lexical_cast<std::string>(hashset_type);
				std::string sub_type = boost::lexical_cast<std::string>(mine_id);
				RankUtils::hset_insert_or_update_str_version(NULL, player->userid,
						player->create_tm, g_server_id, hashset_type_str,
						sub_type, rankproto::REDIS_DELETE,
						&field_vec, DAY_SECS * 7);
			}
			continue;
		}
		commonproto::mine_info_t *mine_ptr = cli_out_.add_mine_info();
		MineUtils::pack_mine_info_for_client_v2(mine, mine_ptr);
		for (int i = 0; i < inf.fields_size(); ++i) {
			commonproto::mine_team_info_t pb_team_info;
			pb_team_info.ParseFromString(inf.fields(i).value());
			mine_ptr->mutable_team_info()->add_team_list()->CopyFrom(pb_team_info);
		}   
	}
	//找出redis中已经被删除，但是db中还没有被删除的矿
	dbproto::cs_del_mine_info db_in;
	FOREACH(expire_mine_vec, it) {
		//删除该矿信息
		//之所以不用del_mine_id_and_sync_db_rawdata
		//而选用先删除内存中 ids, 再调用一次sync_mine_ids_to_db
		//的原因是，这样可以降低与db通信次数
		//MineUtils::del_mine_id_and_sync_db_rawdata(player, *it);
		player->mine_info->delete_my_mine_id_from_memory(*it);

		player->mine_info->delete_player_mine_info_from_memory(*it);

		db_in.add_mine_id(*it);
	}
	if (db_in.mine_id_size()) {
		MineUtils::sync_mine_ids_to_db(player);
		g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
				db_cmd_del_mine_info, db_in);
	}
	//清除所有精灵身上过期矿的mine_fight_flag
	std::vector<uint64_t> mine_ids;
	player->mine_info->get_mine_ids_from_memory(mine_ids);
	FOREACH(*player->pets, iter) {
		Pet& pet = iter->second;
		if (pet.defend_mine_id()) {
			std::vector<uint64_t>::iterator it;
			it = std::find(mine_ids.begin(), mine_ids.end(), pet.defend_mine_id());
			if (it != mine_ids.end()) {
				//找到说明没过期，伙伴正在守矿
				continue;
			}
			pet.set_defend_mine_id(0);
			PetUtils::save_pet(player, pet, false, true);
		}
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int GiveUpMineCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	uint64_t mine_id = 0;
	try {
		mine_id = boost::lexical_cast<uint64_t>(cli_in_.str_mine_id());
	} catch (const boost::bad_lexical_cast &) {
		RET_ERR(cli_err_you_are_hacker);
	}
	mine_id_t* session = (mine_id_t*)player->session;
	memset(session, 0x0, sizeof(*session));
	session->mine_id = mine_id;
	/*
	try {
		mine_id = boost::lexical_cast<uint64_t>(cli_in_.mine_id());
	} catch (const boost::bad_lexical_cast &) {
		RET_ERR(cli_err_you_are_hacker);
	}
	*/
	//先去看看，该矿是否正在被攻击
	std::vector<uint64_t> mine_ids;
	mine_ids.push_back(mine_id);
	return MineUtils::get_player_mine_info_from_db(player, mine_ids);

}

int GiveUpMineCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	switch (player->serv_cmd) {
		case db_cmd_get_player_mine_info:
			return prog_pkg_from_serv_aft_get_attack_flag(player, body, bodylen);
		case ranking_cmd_hset_get_info:
			return proc_pkg_from_serv_aft_get_teams_info(player, body, bodylen);
	}
	return 0;
}

int GiveUpMineCmdProcessor::prog_pkg_from_serv_aft_get_attack_flag(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	if (db_out_.list().mine_info_size() == 0) {
		RET_ERR(cli_err_mine_has_expire);
	}
	const commonproto::mine_info_list_t& pb_list = db_out_.list();	
	uint32_t is_been_attacked = pb_list.mine_info(0).is_been_attacked();
	if (is_been_attacked) {
		RET_ERR(cli_err_can_give_up_mine_when_been_atk);
	}
	uint32_t last_fight_time = pb_list.mine_info(0).last_fight_time();
	if (last_fight_time && 
			last_fight_time + commonproto::MINE_FIGHT_DURATION_LIMIT + 5 > NOW()) {
		RET_ERR(cli_err_mine_last_fight_time_in_cd_time);
	}
	mine_id_t* session = (mine_id_t*)player->session;

	//应该先去redis中拉取下最新的该矿的成员信息
	std::vector<uint64_t> mine_ids;
	mine_ids.push_back(session->mine_id);
	return MineUtils::req_redis_to_get_teams_info(player, mine_ids);
}

int GiveUpMineCmdProcessor::proc_pkg_from_serv_aft_get_teams_info(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	if (rank_out_.hset_infos_size() == 0) {
		RET_ERR(cli_err_not_found_this_mine);
	}
	mine_id_t* session = (mine_id_t*)player->session;

	const rankproto::hset_info_t& inf = rank_out_.hset_infos(0);
	std::vector<string> list = split(inf.key(), ':');
	uint64_t mine_id = boost::lexical_cast<uint32_t>(list[1]);
	if (mine_id != session->mine_id) {
		RET_ERR(cli_err_sys_err);
	}
	//我自己的队伍信息
	std::vector<team_simple_info_t> team_vec;
	//除了我之外其他的队伍信息，key为各队伍开采该矿的时间戳
	TeamInfoMap other_team_map;
	for (int i = 0; i < inf.fields_size(); ++i) {
		commonproto::mine_team_info_t pb_team_info;
		pb_team_info.ParseFromString(inf.fields(i).value());
		team_simple_info_t  team_info;
		team_info.userid = pb_team_info.userid();
		team_info.u_create_tm = pb_team_info.u_create_tm();
		team_info.exploit_start_time = pb_team_info.exploit_start_time();
		if (player->userid == team_info.userid &&
				player->create_tm == team_info.u_create_tm) {
			const commonproto::pet_list_t& pb_pet_inf = pb_team_info.pet_list();
			for (int index = 0; index < pb_pet_inf.pet_list_size(); ++index) {
				team_info.total_power += pb_pet_inf.pet_list(index).base_info().power();
			}
			team_vec.push_back(team_info);
			continue;
		}
		team_info.total_power = pb_team_info.total_power();
		if (other_team_map.count(team_info.exploit_start_time)) {
			RET_ERR(cli_err_give_up_mine_meet_err);
		}
		other_team_map.insert(make_pair(team_info.exploit_start_time, team_info));
	}
	struct mine_info_t mine;
	player->mine_info->get_mine_info_from_memory(session->mine_id, mine);

	MineUtils::calc_player_get_reward(player, team_vec, mine, MINE_GIVE_UP);

	if (!other_team_map.empty()) {
		//更新新的矿主的战力，id号等信息到db中
		//根据map默认的弱排序规则，以及成为矿主的规则(选择队伍中最早时间采该矿)；
		//map中第一个元素，必为矿主的信息
		mine_info_t mine_info;
		player->mine_info->get_mine_info_from_memory(session->mine_id, mine_info);
		team_simple_info_t& team_info = (other_team_map.begin())->second;
		mine_info.user_id = team_info.userid;
		mine_info.u_create_tm = team_info.u_create_tm;
		mine_info.top_pet_power = team_info.total_power;
		//弃矿后，矿里的人数应该减少一个
		if (mine_info.def_player_cnt > 1) {
			mine_info.def_player_cnt = mine_info.def_player_cnt - 1;
		}

		dbproto::cs_update_mine_info db_in;
		MineUtils::pack_mine_info(mine_info, db_in.mutable_mine_info());
		//Confirm kevin 千万不要加return
		g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
				db_cmd_update_mine_info, db_in);
	} else {
		//矿中只剩下我一人，我也弃矿了，则删除该矿
		dbproto::cs_del_mine_info db_in;
		db_in.add_mine_id(session->mine_id);
		//Confirm kevin 千万不要加return
		g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
				db_cmd_del_mine_info, db_in);
	}
	//删除我的信息,从redis中 , raw_data中
	std::vector<rankproto::hset_field_t> field_vec;

	uint64_t uid_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	std::string uid_key_str = boost::lexical_cast<std::string>(uid_key);
	rankproto::hset_field_t pb_field_info;
	pb_field_info.set_name(uid_key_str);
	field_vec.push_back(pb_field_info);

	uint32_t hashset_type = rankproto::HASHSET_MINE_ID_TEAM_INFO_MAP;
	std::string hashset_type_str = boost::lexical_cast<std::string>(hashset_type);
	std::string sub_type = boost::lexical_cast<std::string>(session->mine_id);
	RankUtils::hset_insert_or_update_str_version(NULL, player->userid,
			player->create_tm, g_server_id, hashset_type_str,
			sub_type, rankproto::REDIS_DELETE,
			&field_vec, DAY_SECS * 7);
	


	MineUtils::del_mine_id_and_sync_db_rawdata(player, session->mine_id);
	player->mine_info->delete_player_mine_info_from_memory(session->mine_id);

	//我守护该矿的精灵信息,mine_id 清零,出战的血量不要恢复
	FOREACH(*player->pets, iter) {
		Pet& pet = iter->second;
		if (pet.defend_mine_id() == session->mine_id) {
			pet.set_defend_mine_id(0);
			PetUtils::save_pet(player, pet, false, true);
		}
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int AcceptDefendMineCmdProcessor::proc_pkg_from_client(
	player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	uint64_t tmp_mine_id = 0;
	try {
		tmp_mine_id = boost::lexical_cast<uint64_t>(cli_in_.str_mine_id());
	} catch (const boost::bad_lexical_cast &) {
		RET_ERR(cli_err_you_are_hacker);
	}

	if (GET_A(kDailyMineHasChallengeTimes) >= 
			GET_A(kDailyMineBuyChallengeTimes) + commonproto::MINE_DAILY_OCCUPY_MINE_TIMES) {
		RET_ERR(cli_err_daily_occupy_mine_times_get_limit);
	}

	uint32_t ret = MineUtils::check_mine_num_get_limit(player);
	if (ret) {
		RET_ERR(ret);
	}

	for (int i = 0; i < cli_in_.pet_create_tm_size(); ++i) {
		Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.pet_create_tm(i));
		if (pet == NULL) {
			RET_ERR(cli_err_bag_pet_not_exist);
		}
		if (pet->defend_mine_id()) {
			RET_ERR(cli_err_exploiting_pet_exploit_other_mine);
		}
	}

	//TODO kevin已经守矿的伙伴，不能再守其他的矿
	//尝试参与协防
	dbproto::cs_increment_defender_cnt db_in;
	db_in.set_mine_id(tmp_mine_id);
	return g_dbproxy->send_msg(player, player->userid, 
			player->create_tm, db_cmd_increment_defender_cnt, db_in);
}

int AcceptDefendMineCmdProcessor::proc_pkg_from_serv(player_t* player,
		const char *body, int bodylen)
{
	switch(player->serv_cmd) {
		case db_cmd_increment_defender_cnt:
			return proc_pkg_from_serv_aft_incr_def_cnt(player, body, bodylen);
		case db_cmd_get_player_mine_info:
			return proc_pkg_from_serv_aft_get_ply_mine_info(player, body, bodylen);
	}
	return 0;
}

int AcceptDefendMineCmdProcessor::proc_pkg_from_serv_aft_incr_def_cnt(
		player_t* player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(db_incr_out_);
	if (!db_incr_out_.operate_state()) {
		RET_ERR(cli_err_join_defend_failed);
	}
	dbproto::cs_get_player_mine_info db_in;
	db_in.add_mine_id(db_incr_out_.mine_id());
	return g_dbproxy->send_msg(player, player->userid, 
			player->create_tm, db_cmd_get_player_mine_info, db_in);
}

int AcceptDefendMineCmdProcessor::proc_pkg_from_serv_aft_get_ply_mine_info(
		player_t* player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	if (db_out_.list().mine_info_size() == 0) {
		RET_ERR(cli_err_not_found_this_mine);
	}
	struct mine_info_t mine_info;
	MineUtils::unpack_mine_info(db_out_.list().mine_info(0), mine_info);
	uint32_t ret = player->mine_info->insert_player_mine_info_to_memory(
			mine_info.mine_id, mine_info);
	if (ret) {
		RET_ERR(cli_err_single_user_single_team_mine);
	}
	ret = MineUtils::insert_mine_id_and_sync_db_rawdata(player, mine_info.mine_id);
	if (ret) {
		player->mine_info->delete_player_mine_info_from_memory(mine_info.mine_id);
		RET_ERR(ret);
	}
	std::vector<uint32_t> pet_create_tm;
	for (int i = 0; i < cli_in_.pet_create_tm_size(); ++i) {
		pet_create_tm.push_back(cli_in_.pet_create_tm(i));
	}
	ret = MineUtils::send_pets_start_defend_mine(player, mine_info.mine_id, pet_create_tm);
	if (ret) {
		MineUtils::del_mine_id_and_sync_db_rawdata(player, mine_info.mine_id);
		RET_ERR(ret);
	}
	MineUtils::set_pets_defend_mine_id(player, mine_info.mine_id, pet_create_tm);

	ADD_A(kDailyMineHasChallengeTimes, 1);
	//后台帮前端计算的left_times
	uint32_t left_times = GET_A(kDailyMineBuyChallengeTimes) + 
		commonproto::MINE_DAILY_OCCUPY_MINE_TIMES - GET_A(kDailyMineHasChallengeTimes);
	if (GET_A(kDailyMineHasChallengeTimes) >= 
			GET_A(kDailyMineBuyChallengeTimes) + commonproto::MINE_DAILY_OCCUPY_MINE_TIMES )
	{
		left_times = 0;
	}

	SET_A(kDailyMineLeftChallengeTimes, left_times);

	ADD_A(kDailyMineAcceptMineCnt, 1);
	uint32_t accept_cnt = GET_A(kDailyMineAcceptMineCnt);
	std::string stat_name = "每日协防" + Utils::to_string(accept_cnt) + "玩家人数";
	Utils::write_msglog_new(player->userid, "藏宝矿山", "矿战", stat_name);

	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int StartOccupyMineCmdProcessor::proc_pkg_from_client(
	player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	uint64_t tmp_mine_id = 0;
	try {
		tmp_mine_id = boost::lexical_cast<uint64_t>(cli_in_.str_mine_id());
	} catch (const boost::bad_lexical_cast &) {
		RET_ERR(cli_err_you_are_hacker);
	}
	if (!player->mine_info->find_mine_id_in_match_mine_ids(tmp_mine_id)) {
		RET_ERR(cli_err_not_found_this_mine);
	}
	if (!(GET_A(kAttrAttackMineId) == tmp_mine_id &&
			GET_A(kAttrAttackMineTime) + commonproto::MINE_FIGHT_DURATION_LIMIT > NOW())) {
		if (GET_A(kDailyMineHasChallengeTimes) >=
				GET_A(kDailyMineBuyChallengeTimes) + commonproto::MINE_DAILY_OCCUPY_MINE_TIMES) {
			RET_ERR(cli_err_daily_occupy_mine_times_get_limit);
		}
	}
	struct mine_info_t mine;

	uint32_t ret = player->mine_info->get_match_mine_info_from_memory(tmp_mine_id, mine);
	if (ret) {
		RET_ERR(ret);
	}

	////统计项
	std::string state_name;
	if (mine.mine_size == SMALL_MINE) {
		state_name = "小型";
	} else if (mine.mine_size == MID_MINE) {
		state_name = "中型";
	} else {
		state_name = "大型";
	}
	std::string sub_state_name = "挑战有人占领" + state_name + "矿山的总次数";
	Utils::write_msglog_new(player->userid, "藏宝矿山", "矿战", sub_state_name);
	////统计项

	ret = MineUtils::check_mine_num_get_limit(player);
	if (ret) {
		RET_ERR(ret);
	}
	//判断这些伙伴中，有没有已经在采其他矿的伙伴
	for (int i = 0; i < cli_in_.pet_create_tm_size(); ++i) {
		Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.pet_create_tm(i));
		if (pet == NULL) {
			RET_ERR(cli_err_bag_pet_not_exist);
		}
		if (pet->defend_mine_id() && pet->defend_mine_id() != tmp_mine_id) {
			RET_ERR(cli_err_mine_pet_in_mining_can_not_fight);
		}
	}
	start_occupy_step_session_t* session = (start_occupy_step_session_t*)player->session;
	memset(session, 0, sizeof(*session));
	session->mine_id = tmp_mine_id;
	std::vector<uint64_t> mine_ids;
	mine_ids.push_back(tmp_mine_id);
	return MineUtils::get_player_mine_info_from_db(player, mine_ids);
}

int StartOccupyMineCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	switch(player->serv_cmd) {
		case db_cmd_get_player_mine_info:
			return proc_pkg_from_serv_aft_get_mine_attack_info(player, body, bodylen);
		case ranking_cmd_hset_get_info:
			return  proc_pkg_from_serv_aft_get_all_info(player, body, bodylen);
		case db_cmd_user_raw_data_get:
			return  proc_pkg_from_serv_aft_get_mine_fight_info(player, body, bodylen);
	}
	return 0;
}

int StartOccupyMineCmdProcessor::proc_pkg_from_serv_aft_get_mine_attack_info(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_get_attack_out_);
	const commonproto::mine_info_list_t& pb_list = db_get_attack_out_.list();
	if (pb_list.mine_info_size() == 0) {
		RET_ERR(cli_err_will_attack_mine_disappear);
	}
	//正在有其人玩家攻打该矿
	//TODO kevin 这里应该将该值修改成记录攻打该矿的时间戳，这里判断时，判断该值存在的情况下，现在时间是大于当时的时间戳+攻打的时间
	uint32_t is_been_attacked = pb_list.mine_info(0).is_been_attacked();
	if (is_been_attacked) {
		RET_ERR(cli_err_other_player_is_attacking_now);
	}
	start_occupy_step_session_t* session = (start_occupy_step_session_t*)player->session;
	uint64_t mine_id = session->mine_id;
	//过滤掉自己的血量已经为0的伙伴
	std::vector<uint32_t> pet_ctm_vec;
	for (int i = 0; i < cli_in_.pet_create_tm_size(); ++i) {
		Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.pet_create_tm(i));
		if (pet == NULL) {
			RET_ERR(cli_err_bag_pet_not_exist);
		}
		if (pet->mine_attack_hp() == 0) {
			continue;
		}
		pet_ctm_vec.push_back(cli_in_.pet_create_tm(i));
	}
	//TODO kevin 这里应该做个判断，如果pet_ctm_vec为空,说明自己选派出场的伙伴血量全部为0
	if (pet_ctm_vec.empty()) {
		RET_ERR(cli_err_mine_player_all_fight_pet_hp_empty);
	}
	
	uint64_t def_user_key = ROLE_KEY(ROLE(cli_in_.user_id(), cli_in_.u_create_tm()));
	player->mine_info->def_uid_key_ = def_user_key;
	player->mine_info->mine_id_ = mine_id; 
	session->ack_tick = cli_in_.atk_tick();
	session->def_tick = cli_in_.def_tick();

	player->mine_info->clear_start_fight_pet_ctm();
	player->mine_info->set_start_fight_pets_ctm(pet_ctm_vec);
	//检查该矿中有多少人；到 redis中getall出所有防守的玩家
	//检查该玩家是否还在挖矿,并把目前该矿的人数等信息保存在内存中
	std::vector<uint64_t> mine_id_vec;
	mine_id_vec.push_back(mine_id);
	return MineUtils::req_redis_to_get_teams_info(player, mine_id_vec);
}

int StartOccupyMineCmdProcessor::proc_pkg_from_serv_aft_get_all_info(player_t* player,
		const char* body, int bodylen)
{
	PARSE_SVR_MSG(rank_get_info_);
	if (rank_get_info_.hset_infos_size() == 0) {
		player->mine_info->def_uid_key_ = 0;
		player->mine_info->mine_id_ = 0;
		player->mine_info->clear_start_fight_pet_ctm();
		RET_ERR(cli_err_mine_time_is_up);
	}
	//保存下该矿所有防守的玩家
	player->mine_info->ai_pet_hp_map_.clear();
	player->mine_info->ai_state_map_.clear();
	player->mine_info->def_team_vec_.clear();
	const rankproto::hset_info_t& hash_inf = rank_get_info_.hset_infos(0);

	bool found_def_user = false;
	
	commonproto::mine_team_info_list_t pb_team_list;
	for (int i = 0; i < hash_inf.fields_size(); ++i) {
		std::string role_key_str = hash_inf.fields(i).name();
		uint64_t role_key  = boost::lexical_cast<uint64_t>(role_key_str);
		role_info_t role = KEY_ROLE(role_key);
		//找出该role_key对应的防守精灵的create_tm
		commonproto::mine_team_info_t pb_team_info;
		pb_team_info.ParseFromString(hash_inf.fields(i).value());
		pb_team_list.add_team_list()->CopyFrom(pb_team_info);
		if (!(role.userid == pb_team_info.userid() &&
					role.u_create_tm == pb_team_info.u_create_tm())) {
			//理论上不会执行到这里
			//field中的role_key,与value中的userid,createtm组合一定是相等的
			RET_ERR(cli_err_get_mine_pet_info_err);
		}

		const commonproto::pet_list_t& pb_pet_inf = pb_team_info.pet_list();
		if (pb_team_info.userid() == cli_in_.user_id() &&
				pb_team_info.u_create_tm() == cli_in_.u_create_tm()) {
			//保存下待揍的玩家pet_info,用于最后打包并回包给前端
			player->mine_info->def_pet_info_.clear();
			std::string pkg;
			pb_pet_inf.SerializeToString(&pkg);
			player->mine_info->def_pet_info_ = pkg;
			found_def_user = true;
		}
		//将开采该矿的每支队伍中精灵信息,从redis中的message里抠取出来
		//并保存在临时内存ai_pet_hp_map_中
		std::vector<pet_simple_info_t> pet_vec;
		uint32_t def_total_power = 0;
		for (int j = 0; j < pb_pet_inf.pet_list_size(); ++j) {
			uint32_t create_tm = pb_pet_inf.pet_list(j).base_info().create_tm();
			struct pet_simple_info_t pet_hp;
			pet_hp.pet_create_tm = create_tm;
			//pet_cur_hp先赋值为max_hp
			//之后请求攻击者保存在raw_data中的战斗血量数据来修改
			pet_hp.pet_cur_hp = pb_pet_inf.pet_list(j).battle_info().max_hp();
			pet_hp.pet_power = pb_pet_inf.pet_list(j).base_info().power();
			def_total_power += pet_hp.pet_power;
			pet_vec.push_back(pet_hp);
		}

		team_simple_info_t def_team;
		def_team.userid = pb_team_info.userid();
		def_team.u_create_tm = pb_team_info.u_create_tm();
		def_team.exploit_start_time = pb_team_info.exploit_start_time();
		def_team.total_power = def_total_power;
		player->mine_info->def_team_vec_.push_back(def_team);

		//初始化将要攻击的目标矿里的队伍中属于role_key这支队伍的伙伴血量
		//以及该队伍的战斗状态
		player->mine_info->ai_pet_hp_map_[role_key] = pet_vec;
		player->mine_info->ai_state_map_[role_key] = onlineproto::MINE_NO_FIGHT;
	}
	player->mine_info->def_team_info_.clear();
	pb_team_list.SerializeToString(&(player->mine_info->def_team_info_));
		
	uint64_t cli_role_key = ROLE_KEY(ROLE(cli_in_.user_id(), cli_in_.u_create_tm()));
	PetInfoMap::iterator it;
	it = player->mine_info->ai_pet_hp_map_.find(cli_role_key);
	if (it == player->mine_info->ai_pet_hp_map_.end()) {
		player->mine_info->def_uid_key_ = 0;
		player->mine_info->mine_id_ = 0;
		player->mine_info->clear_start_fight_pet_ctm();
		player->mine_info->ai_pet_hp_map_.clear();
		player->mine_info->ai_state_map_.clear();
		player->mine_info->def_team_vec_.clear();
		RET_ERR(cli_err_this_team_not_mine);
	}
	//去 db raw_data中拉取该矿中被我揍的队伍信息
	return PlayerUtils::get_user_raw_data(player, dbproto::MINE_FIGHT_DEF_PET_HP, "0");
}

int StartOccupyMineCmdProcessor::proc_pkg_from_serv_aft_get_mine_fight_info(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	cli_out_.Clear();
	onlineproto::team_hp_info_list_t pb_raw_data_hp;
	pb_raw_data_hp.ParseFromString(db_out_.raw_data());
	start_occupy_step_session_t* session = (start_occupy_step_session_t*)player->session;
	uint64_t mine_id = session->mine_id;
	if (pb_raw_data_hp.team_list_size() == 0) {
		//空旷
		commonproto::pet_list_t pb_pet_list;
		pb_pet_list.ParseFromString(player->mine_info->def_pet_info_);
		MineUtils::convert_pb_pet_list_to_btl_pet_list(pb_pet_list,
				cli_out_.mutable_def_fight_pets());

		//空旷，需要恢复我自己的伙伴的血量
		MineUtils::reset_player_pets_attack_mine_info_relate(player);

		ADD_A(kDailyMineHasChallengeTimes, 1);
		//后台帮前端计算的left_times
		uint32_t left_times = GET_A(kDailyMineBuyChallengeTimes) + 
			commonproto::MINE_DAILY_OCCUPY_MINE_TIMES - GET_A(kDailyMineHasChallengeTimes);
		if (GET_A(kDailyMineHasChallengeTimes) >= 
				GET_A(kDailyMineBuyChallengeTimes) + commonproto::MINE_DAILY_OCCUPY_MINE_TIMES )
		{
			left_times = 0;
		}
		SET_A(kDailyMineLeftChallengeTimes, left_times);
	} else {
		if (pb_raw_data_hp.mine_id() != mine_id) {
			//说明玩家换矿挑战
			//更新现在的矿中防守方的信息到db_raw_data中,这样原来矿中精灵的血量也恢复了
			//更新精灵的mine_flag
			commonproto::pet_list_t pb_pet_list;
			pb_pet_list.ParseFromString(player->mine_info->def_pet_info_);
			MineUtils::convert_pb_pet_list_to_btl_pet_list(pb_pet_list,
					cli_out_.mutable_def_fight_pets());

			//空旷，需要恢复我自己的伙伴的血量
			MineUtils::reset_player_pets_attack_mine_info_relate(player);

			ADD_A(kDailyMineHasChallengeTimes, 1);
			//后台帮前端计算的left_times
			uint32_t left_times = GET_A(kDailyMineBuyChallengeTimes) + 
				commonproto::MINE_DAILY_OCCUPY_MINE_TIMES - GET_A(kDailyMineHasChallengeTimes);
			if (GET_A(kDailyMineHasChallengeTimes) >= 
					GET_A(kDailyMineBuyChallengeTimes) + commonproto::MINE_DAILY_OCCUPY_MINE_TIMES )
			{
				left_times = 0;
			}
			SET_A(kDailyMineLeftChallengeTimes, left_times);
		} else {
			uint32_t mine_fight_time = pb_raw_data_hp.mine_fight_time();
			if (mine_fight_time && 
				mine_fight_time + commonproto::MINE_FIGHT_DURATION_LIMIT < NOW()) {
				//同一个矿，但是，该矿已经有十分钟没有攻打了,所以要重新打
				commonproto::pet_list_t pb_pet_list;
				pb_pet_list.ParseFromString(player->mine_info->def_pet_info_);
				MineUtils::convert_pb_pet_list_to_btl_pet_list(pb_pet_list,
						cli_out_.mutable_def_fight_pets());

				//空旷，需要恢复我自己的伙伴的血量
				MineUtils::reset_player_pets_attack_mine_info_relate(player);

				ADD_A(kDailyMineHasChallengeTimes, 1);
				//后台帮前端计算的left_times
				uint32_t left_times = GET_A(kDailyMineBuyChallengeTimes) + 
					commonproto::MINE_DAILY_OCCUPY_MINE_TIMES - GET_A(kDailyMineHasChallengeTimes);
				if (GET_A(kDailyMineHasChallengeTimes) >= 
						GET_A(kDailyMineBuyChallengeTimes) + commonproto::MINE_DAILY_OCCUPY_MINE_TIMES )
				{
					left_times = 0;
				}
				SET_A(kDailyMineLeftChallengeTimes, left_times);
			} else {
				//同一个矿，还在十分钟范围内
				commonproto::battle_pet_list_t pb_for_cli;
				for (int i = 0; i < pb_raw_data_hp.team_list_size(); ++i) {
					uint32_t userid = pb_raw_data_hp.team_list(i).user_id();
					uint32_t create_tm = pb_raw_data_hp.team_list(i).create_tm();
					uint32_t fight_state = pb_raw_data_hp.team_list(i).fight_state();
					//找到对应的玩家
					if (userid == cli_in_.user_id() 
							&& create_tm == cli_in_.u_create_tm()) {
						//对方若已经挂了，就不能再出战
						if (fight_state == onlineproto::MINE_HAS_DEAD) {
							ERROR_TLOG("Mine Team Dead, userid=[%u],create_tm=[%u]",
									userid, create_tm);
							player->mine_info->def_uid_key_ = 0;
							player->mine_info->mine_id_ = 0;
							player->mine_info->clear_start_fight_pet_ctm();
							player->mine_info->ai_pet_hp_map_.clear();
							player->mine_info->ai_state_map_.clear();
							player->mine_info->def_team_vec_.clear();
							RET_ERR(cli_err_dead_mine_team_can_not_fight);
						}
					}
					uint64_t role_key = ROLE_KEY(ROLE(userid, create_tm));
					const onlineproto::team_hp_info_t& pb_inf = pb_raw_data_hp.team_list(i);
					const commonproto::pet_cur_hp_list_t& pb_pet_inf = pb_inf.hp_list();

					//处理历史上占领该矿的伙伴血量问题
					//分两种情况： 1.待被揍的玩家实际已经弃矿；2.该玩家还在继续挖矿
					PetInfoMap::iterator it;
					it = player->mine_info->ai_pet_hp_map_.find(role_key);
					if (it == player->mine_info->ai_pet_hp_map_.end()) {
						//说明该玩家已经弃矿了
						TRACE_TLOG("Mine System,some user have droped Mine,"
								"uid=[%u],create_tm=[%u],mine_id=[%lu]",
								userid, create_tm, mine_id);
						//
						std::vector<pet_simple_info_t> pet_vec;
						for (int m = 0; m < pb_pet_inf.hp_list_size(); ++m) {
							struct pet_simple_info_t  pet_info;
							pet_info.pet_create_tm = pb_pet_inf.hp_list(m).create_tm();
							pet_info.pet_cur_hp = 0;
							pet_info.pet_power = 0;
							pet_vec.push_back(pet_info);
						}
						player->mine_info->ai_state_map_[role_key] = onlineproto::MINE_HAS_DEAD;
						player->mine_info->ai_pet_hp_map_[role_key] = pet_vec;
						continue;
					}
					//若到了这里，说明待被揍的玩家还在继续挖矿,更新该玩家守矿的伙伴血量
					std::vector<pet_simple_info_t>& tmp_vec = it->second;
					uint32_t zero_hp_count = 0;
					for (int j = 0; j < pb_pet_inf.hp_list_size(); ++j) {
						const commonproto::pet_cur_hp_t& mine_pet = pb_pet_inf.hp_list(j);
						uint32_t create_tm = mine_pet.create_tm();
						uint32_t cur_hp = mine_pet.cur_hp();
						if (cur_hp == 0) {
							++zero_hp_count;
						}
						FOREACH(tmp_vec, iter) {
							if (iter->pet_create_tm == create_tm) {
								//血量继承
								//继承的血量不可能大于伙伴最大的血量
								if (cur_hp > iter->pet_cur_hp) {
									//继承的血量是后台保存的，理论上不会执行到这里
									RET_ERR(cli_err_mine_pet_hp_save_err);
								}
								iter->pet_cur_hp = cur_hp;
								break;
							}
						}
					}
					//更新继承后的值
					player->mine_info->ai_pet_hp_map_[role_key] = tmp_vec;
					player->mine_info->ai_state_map_[role_key] = fight_state;
					//说明对手玩家全部都挂了，报错，不让进入场景
					if (userid == cli_in_.user_id() 
							&& create_tm == cli_in_.u_create_tm()) {
						if (zero_hp_count == (uint32_t)pb_pet_inf.hp_list_size()) {
							player->mine_info->ai_state_map_[role_key] = onlineproto::MINE_HAS_DEAD;
							player->mine_info->def_pk_time_map_[role_key] = NOW();
							onlineproto::team_hp_info_list_t pb_team_hp;
							MineUtils::pack_all_def_pet_hp(player, pb_team_hp);
							pb_team_hp.set_mine_id(player->mine_info->mine_id_);
							PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
									dbproto::MINE_FIGHT_DEF_PET_HP, pb_team_hp, "0");
							player->mine_info->def_uid_key_ = 0;
							player->mine_info->mine_id_ = 0;
							player->mine_info->clear_start_fight_pet_ctm();
							player->mine_info->ai_pet_hp_map_.clear();
							player->mine_info->ai_state_map_.clear();
							player->mine_info->def_team_vec_.clear();
							ERROR_TLOG("Mine Def Pet Hp all Empty,def_uid=%u,def_ctm=%u"
									"p_uid=%u,p_ctm=%u,zero_cnt=%u,mine_id=%u",
									userid, create_tm, player->userid, player->create_tm,
									zero_hp_count, player->mine_info->mine_id_);
							RET_ERR(cli_err_mine_def_pet_hp_empty);
						}
					}

					commonproto::battle_pet_list_t pb_btl_pets;
					commonproto::pet_list_t pb_list;
					pb_list.ParseFromString(player->mine_info->def_pet_info_);
					MineUtils::convert_pb_pet_list_to_btl_pet_list(pb_list, &pb_btl_pets);
					uint32_t index = 0;
					FOREACH(tmp_vec, iter02) {
						++index;
						uint32_t pet_create_tm = iter02->pet_create_tm;
						uint32_t cur_hp = iter02->pet_cur_hp;
						if (cur_hp == 0) {
							continue;
						}
						for (int i = 0; i < pb_btl_pets.pet_list_size(); ++i) {
							const commonproto::pet_info_t& pb_inf = 
								pb_btl_pets.pet_list(i).pet_info();

							if (pb_inf.base_info().create_tm() == pet_create_tm) {
								///TRACE_TLOG
								uint32_t pet_id = pb_inf.base_info().pet_id();
								uint32_t old_cur_hp = pb_inf.battle_info().cur_hp();
								uint32_t max_hp = pb_inf.battle_info().max_hp();
								uint32_t old_mine_hp = pb_inf.mine_fight_hp();
								//pet_id = 0;old_cur_hp=0;max_hp=0;old_mine_hp=0;
								///TRACE_TLOG

								TRACE_TLOG("Modify Hp For Cli,pet_id=[%u],"
										"old_cur_hp=[%u],max_hp=[%u],"
										"old_mine_hp=[%u],new_cur_hp=[%u],"
										"uid=[%u],create_tm=[%u]",
										pet_id, old_cur_hp, max_hp, old_mine_hp, cur_hp,
										player->userid, player->create_tm);

								commonproto::pet_info_t* ptr = 
									pb_btl_pets.mutable_pet_list(i)->mutable_pet_info();
								ptr->set_mine_fight_hp(cur_hp);
								ptr->mutable_battle_info()->set_cur_hp(cur_hp);

								pb_for_cli.add_pet_list()->mutable_pet_info()->CopyFrom(*ptr);
								//跳出 for  i 循环
								//继续遍历tmp_vec，在待揍的玩家pb_msg结构中找到匹配的伙伴
								//修改他们的mine_fight_hp,与cur_hp为前端而改
								break;
							}
						}
					}
					std::string debug_str = pb_for_cli.Utf8DebugString();
					std::string name = pb_for_cli.GetTypeName();
					TRACE_TLOG("clibtl_data:'%s'\nmsg:[%s]\n",
							name.c_str(), debug_str.c_str());
					cli_out_.mutable_def_fight_pets()->CopyFrom(pb_for_cli);
				}
			}
		}
	}

	std::vector<uint32_t> pet_ctm_vec;
	player->mine_info->get_start_fight_pet_ctm(pet_ctm_vec);

	MineUtils::pack_player_start_fight_pet_btl_info(player,
			pet_ctm_vec, cli_out_.mutable_atk_fight_pets());
	cli_out_.set_atk_tick(session->ack_tick);
	cli_out_.set_def_tick(session->def_tick);
	//设置我与该矿玩家pk的战斗锁
	MineUtils::set_mine_attack_state(player, MINE_BE_ATTACKED);

	MineUtils::mine_modify_player_pet_hp_for_cli(
			cli_out_.mutable_atk_fight_pets(),
			cli_out_.atk_fight_pets());

	MineUtils::mine_modify_player_pet_hp_for_cli(
			cli_out_.mutable_def_fight_pets(),
			cli_out_.def_fight_pets());

	if (GET_A(kAttrAttackMineId) != mine_id) {
		SET_A(kAttrAttackMineId, mine_id);
	}
	SET_A(kAttrAttackMineTime, NOW());

	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int OccupyMineRetCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	cli_out_.Clear();
	//首先去db里把 锁 给释放,并在函数里设置攻打该矿的时间戳
	MineUtils::set_mine_attack_state(player, MINE_NOT_BE_ATTACKED);
	//设置最近攻打该矿的时间戳
	//如果我赢了
	//更新对手精灵的血量，并保存在redis中 hash里
	if (cli_in_.type() == commonproto::WIN) {
		//检查客端传来的精灵，是否为我参加攻击的精灵
		std::vector<uint32_t> my_pet_c_tm;
		player->mine_info->get_start_fight_pet_ctm(my_pet_c_tm);
		std::vector<uint32_t> cli_pet_ctm;
		for (int i = 0; i < cli_in_.hp_list().hp_list_size(); ++i) {
			uint32_t create_tm = cli_in_.hp_list().hp_list(i).create_tm();
			std::vector<uint32_t>::iterator it;
			it = std::find(my_pet_c_tm.begin(), my_pet_c_tm.end(), create_tm);
			if (it == my_pet_c_tm.end()) {
				RET_ERR(cli_err_bag_pet_not_exist);
			}
			cli_pet_ctm.push_back(create_tm);
		}
		if (cli_pet_ctm.size() != my_pet_c_tm.size()) {
			RET_ERR(cli_err_mine_fight_pet_escape);
		}
		for (int i = 0; i < cli_in_.hp_list().hp_list_size(); ++i) {
			uint32_t create_tm = cli_in_.hp_list().hp_list(i).create_tm();
			uint32_t cur_hp = cli_in_.hp_list().hp_list(i).cur_hp();
			cur_hp = ceil(cur_hp / (MINE_HP_COE * 1.0));
			Pet* pet = PetUtils::get_pet_in_loc(player, create_tm);
			if (pet == NULL) {
				RET_ERR(cli_err_bag_pet_not_exist);
			}
			//uint32_t pet_cur_hp = pet->mine_fight_hp();
			/*
			if (pet_cur_hp < cur_hp) {
				ERROR_TLOG("My Pet Hp Err,uid=%u,cli_hp=%u,real_hp=%u",
						player->userid, cur_hp, pet_cur_hp);
				RET_ERR(cli_err_fight_pets_cur_hp_err);
			}
			*/
			//更新我精灵的血量，以及设置这些精灵正在攻打该矿
			pet->set_mine_attack_hp(cur_hp);
			pet->set_mine_flag(player->mine_info->mine_id_);
			PetUtils::save_pet(player, *pet, false, true);
		}
		//更新被揍玩家的伙伴血量，以及被揍玩家的mine_fight_state_t状态
		uint64_t role_key = player->mine_info->def_uid_key_;
		PetInfoMap::iterator it;
		it = player->mine_info->ai_pet_hp_map_.find(role_key);
		if (it == player->mine_info->ai_pet_hp_map_.end()) {
			RET_ERR(cli_err_mine_fight_process_illegal);
		}
		std::vector<pet_simple_info_t>& tmp_vec = it->second;
		FOREACH(tmp_vec, it) {
			it->pet_cur_hp = 0;
		}
		player->mine_info->ai_state_map_[role_key] = onlineproto::MINE_HAS_DEAD;
		player->mine_info->def_pk_time_map_[role_key] = NOW();
		onlineproto::team_hp_info_list_t pb_team_hp;
		MineUtils::pack_all_def_pet_hp(player, pb_team_hp);
		pb_team_hp.set_mine_id(player->mine_info->mine_id_);
		pb_team_hp.set_mine_fight_time(NOW());

		std::string debug_str = pb_team_hp.Utf8DebugString();
		std::string name = pb_team_hp.GetTypeName();
		TRACE_TLOG("occupy,result,uid=%u\n"
				" PARSE MSG:'%s',\nmsg:[%s]\n", player->userid,
				name.c_str(), debug_str.c_str());
		PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
				dbproto::MINE_FIGHT_DEF_PET_HP, pb_team_hp, "0");
		//判断是否该矿山全军覆没,生成战报
		bool generate_report = true;
		FOREACH(player->mine_info->ai_state_map_, it) {
			if (it->second != (uint32_t)onlineproto::MINE_HAS_DEAD) {
				generate_report = false;
				break;
			}
		}
		if (generate_report) {
			std::vector<rob_resource_info_t> rob_res_vec;
			uint32_t id = 0, total_num = 0;
			MineUtils::calc_mine_fight_get_resource(player,
					player->mine_info->mine_id_, rob_res_vec, id, total_num);
			std::vector<attachment_t> attach_vec;
			attachment_t attach;
			attach.type = kMailAttachItem;
			attach.id = id;
			attach.count = total_num;
			attach_vec.push_back(attach);
			new_mail_t new_mail;
			new_mail.sender.assign("系统邮件");
			new_mail.title.assign("抢矿成功");
			new_mail.content.assign("你的小伙伴们为你占领了一座矿，你收获了许多奖励，请点击附件领取！");
			std::string attachment;
			MailUtils::serialize_attach_to_attach_string(attach_vec, attachment);
			new_mail.attachment = attachment;
			MailUtils::add_player_new_mail(player, new_mail);

			//保存战报
			std::string pkg;
			std::vector<rob_resource_info_t> tmp_vec;
			MineUtils::generate_mine_btl_report(player,
					player->mine_info->mine_id_, pkg,
					commonproto::LOSE, tmp_vec);
			std::vector<role_info_t> role_vec;
			FOREACH(rob_res_vec, iter) {
				role_info_t roles;
				roles.userid = iter->userid;
				roles.u_create_tm = iter->u_create_tm;
				role_vec.push_back(roles);
			}
			RankUtils::save_battle_simple_report(player, role_vec,
					commonproto::MINE_FIGHT, pkg, DAY_SECS * 7);

			//矿将沦陷被占领，计算出守矿中的每个玩家获得奖励
			//发放邮件给防守方的玩家
			struct mine_info_t mine;
			player->mine_info->get_match_mine_info_from_memory(
					player->mine_info->mine_id_, mine);
			MineUtils::calc_player_get_reward(player,
					player->mine_info->def_team_vec_,
					mine, MINE_IS_BE_OCCUPY);

			//删除攻击的矿 ，并生成属于我的新矿
			dbproto::cs_del_mine_info db_del_in;
			db_del_in.add_mine_id(mine.mine_id);
			g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
					db_cmd_del_mine_info, db_del_in);

			//将该对手矿id从 我内存match_mine_ids_ 结构中删除，并保存到raw_data中
			player->mine_info->delete_match_mine_ids_from_memory(mine.mine_id);
			MineUtils::sync_match_mine_ids_to_db_rawdata(player);

			//将我的精灵mine_flag全部清掉,并恢复血量
			std::vector<uint32_t> pet_ctm_vec;
			player->mine_info->get_start_fight_pet_ctm(pet_ctm_vec);
			MineUtils::reset_player_pets_attack_mine_info_relate(player);
			
			//生成属性我自己的矿，并重新计算时间,矿的类型不变
			mine.mine_create_tm = NOW();
			mine.user_id = player->userid;
			mine.u_create_tm = player->create_tm;
			uint32_t total_power = 0;
			std::set<uint32_t> create_tm_set;
			uint32_t PET_NUM = 5;
			PetUtils::get_pets_n_topest_power(player, create_tm_set,
					PET_NUM, total_power);
			mine.top_pet_power = total_power;
			mine.def_player_cnt = 1;
			mine.is_been_attacked = MINE_NOT_BE_ATTACKED;
			player->mine_info->save_mine_tmp_info_to_memory(mine.mine_id, mine);
			dbproto::cs_save_mine_info db_save_in;
			MineUtils::pack_mine_info(mine, db_save_in.mutable_mine_info());
			return g_dbproxy->send_msg(player, player->userid, player->create_tm,
					db_cmd_save_one_new_mine, db_save_in);
		}
	} else {
		uint64_t role_key = player->mine_info->def_uid_key_;	
		PetInfoMap::iterator it;
		it = player->mine_info->ai_pet_hp_map_.find(role_key);
		if (it == player->mine_info->ai_pet_hp_map_.end()) {
			//由于def_uid_key 是内存中保存的，故理论上不会执行到这里
			RET_ERR(cli_err_user_not_found_in_this_mine);
		}
		uint32_t ret = MineUtils::modify_pet_hp_for_memory(
				cli_in_.hp_list(), it->second);
		if (ret) {
			RET_ERR(ret);
		}
		player->mine_info->ai_state_map_[role_key] = (uint32_t)onlineproto::MINE_HAS_FIGHT;
		player->mine_info->def_pk_time_map_[role_key] = NOW();
		onlineproto::team_hp_info_list_t pb_team_hp;
		MineUtils::pack_all_def_pet_hp(player, pb_team_hp);
		pb_team_hp.set_mine_id(player->mine_info->mine_id_);
		pb_team_hp.set_mine_fight_time(NOW());
		PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
				dbproto::MINE_FIGHT_DEF_PET_HP, pb_team_hp, "0");

		//设置我当前参战的精灵血量为空,以及设置这些精灵正在攻击该矿
		std::vector<uint32_t> pet_create_tm;
		player->mine_info->get_start_fight_pet_ctm(pet_create_tm);
		FOREACH(pet_create_tm, it) {
			Pet* pet = PetUtils::get_pet_in_loc(player, *it);
			if (pet == NULL) {
				return cli_err_bag_pet_not_exist;
			}
			pet->set_mine_attack_hp(0);
			pet->set_mine_flag(player->mine_info->mine_id_);
			PetUtils::save_pet(player, *pet, false, true);
		}
		std::string pkg;
		std::vector<rob_resource_info_t> tmp_vec;
		MineUtils::generate_mine_btl_report(player,
				player->mine_info->mine_id_, pkg,
				commonproto::WIN, tmp_vec);
		std::vector<role_info_t> role_vec;
		role_info_t roles = KEY_ROLE(player->mine_info->def_uid_key_);
		role_vec.push_back(roles);
		RankUtils::save_battle_simple_report(player, role_vec,
				commonproto::MINE_FIGHT, pkg, DAY_SECS * 7);

	}
	struct mine_info_t mine_info;
	player->mine_info->get_match_mine_info_from_memory(
			player->mine_info->mine_id_,
			mine_info);
	MineUtils::pack_mine_info_for_client_v2(mine_info, cli_out_.mutable_mine_info());
	commonproto::mine_team_info_list_t pb_tm_list;
	pb_tm_list.ParseFromString(player->mine_info->def_team_info_);
	cli_out_.mutable_mine_info()->mutable_team_info()->CopyFrom(pb_tm_list);
	cli_out_.set_result(false);
	if (cli_in_.type() == commonproto::WIN) {
		MineUtils::modify_def_team_btl_result(player->mine_info->def_uid_key_,
				cli_out_.mutable_mine_info()->mutable_team_info(), true);
	} else {
		MineUtils::modify_def_team_btl_result(player->mine_info->def_uid_key_,
				cli_out_.mutable_mine_info()->mutable_team_info(), false);
	}

	player->mine_info->clear_start_fight_pet_ctm();
	player->mine_info->def_uid_key_ = 0;
	player->mine_info->mine_id_ = 0; 
	player->mine_info->def_team_info_.clear();
	//将该矿被攻占的信息清除
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int OccupyMineRetCmdProcessor::proc_pkg_from_serv(player_t *player,
		const char *body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	struct mine_info_t mine_info;
	player->mine_info->get_mine_tmp_info(player->mine_info->mine_id_, mine_info);
	//保存占领后的矿（新的id,矿的类型等数据不变）到内存
	mine_info.mine_id = db_out_.mine_id();
	uint32_t ret = player->mine_info->insert_player_mine_info_to_memory(
			mine_info.mine_id, mine_info);
	if (ret) {
		RET_ERR(ret);
	}
	ret = MineUtils::insert_mine_id_and_sync_db_rawdata(player, mine_info.mine_id);
	if (ret) {
		player->mine_info->delete_player_mine_info_from_memory(mine_info.mine_id);
		RET_ERR(ret);
	}
	//参加攻击的精灵保存到redis中
	std::vector<uint32_t> pet_create_tm;
	player->mine_info->get_start_fight_pet_ctm(pet_create_tm);
	ret = MineUtils::send_pets_start_defend_mine(player,
			mine_info.mine_id, pet_create_tm);
	if (ret) {
		MineUtils::del_mine_id_and_sync_db_rawdata(player, mine_info.mine_id);
		RET_ERR(ret);
	}

	MineUtils::set_pets_defend_mine_id(player, mine_info.mine_id, pet_create_tm);

	player->mine_info->clear_start_fight_pet_ctm();
	player->mine_info->def_uid_key_ = 0;
	player->mine_info->mine_id_ = 0; 
	//增加矿战次数
	AttrUtils::add_attr_in_special_time_range(player,
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivWarOfMine);

	cli_out_.set_result(true);
	MineUtils::pack_mine_info_for_client_v2(mine_info, cli_out_.mutable_mine_info());
	commonproto::mine_team_info_list_t pb_tm_list;
	pb_tm_list.ParseFromString(player->mine_info->def_team_info_);
	cli_out_.mutable_mine_info()->mutable_team_info()->CopyFrom(pb_tm_list);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int GetMineBtlReportCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	rankproto::cs_get_btl_report req_in;
	req_in.set_userid(player->userid);
	req_in.set_create_tm(player->create_tm);
	req_in.set_server_id(g_server_id);
	req_in.set_type(commonproto::MINE_FIGHT);
	return g_dbproxy->send_msg(player, player->userid, player->create_tm,
			ranking_cmd_get_common_btl_report, req_in);
}

int GetMineBtlReportCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	cli_out_.Clear();
	onlineproto::mine_fight_btl_report_list_t* ptr = cli_out_.mutable_btl_report_list();
	for (int i = 0; i < rank_out_.btl_inf_size(); ++i) {
		const rankproto::btl_report_info& pb_btl = rank_out_.btl_inf(i);
		onlineproto::mine_fight_btl_report_t mine_report;
		mine_report.ParseFromString(pb_btl.pkg());
		const onlineproto::mine_lost_num_info_list_t& pb_inf = mine_report.lost_list();
		TRACE_TLOG("get_mine_btl_report,uid=%u,size=%u", player->userid, pb_inf.lost_info_size());
		for (int j = 0; j < pb_inf.lost_info_size(); ++j) {
			const onlineproto::mine_lost_num_info_t& pb_inf02 = pb_inf.lost_info(j);
			TRACE_TLOG("get_btl_report.uid=%u,ctm=%u", pb_inf02.uid(), pb_inf02.create_tm());
			if (pb_inf02.uid() == player->userid && 
				pb_inf02.create_tm() == player->create_tm) {
				mine_report.set_lost_money(pb_inf02.lost_money());
				break;
			}
		}
		ptr->add_btl_report()->CopyFrom(mine_report);
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int GetMineInfoByMineIdCmdProcessor::proc_pkg_from_client(
	player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	uint64_t mine_id = 0;
	try {
		mine_id = boost::lexical_cast<uint64_t>(cli_in_.str_mine_id());
	} catch (const boost::bad_lexical_cast &) {
		RET_ERR(cli_err_you_are_hacker);
	}
	/*
	try {
		mine_id = boost::lexical_cast<uint64_t>(cli_in_.mine_id());
	} catch (const boost::bad_lexical_cast &) {
		RET_ERR(cli_err_you_are_hacker);
	}
	*/
	std::vector<uint64_t> mine_id_vec;
	mine_id_vec.push_back(mine_id);
	return MineUtils::get_player_mine_info_from_db(player, mine_id_vec);
}

int GetMineInfoByMineIdCmdProcessor::proc_pkg_from_serv_aft_get_mine_info(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	cli_out_.Clear();
	if (db_out_.expire_mine_ids_size() || db_out_.list().mine_info_size() == 0) {
		RET_ERR(cli_err_not_found_this_mine);
	}
	cli_out_.mutable_mine_list()->CopyFrom(db_out_.list());
	std::vector<uint64_t> mine_id_vec;
	mine_id_vec.push_back(db_out_.list().mine_info(0).mine_id());
	return MineUtils::req_redis_to_get_teams_info(player, mine_id_vec);
}

int GetMineInfoByMineIdCmdProcessor::proc_pkg_from_serv_aft_get_pet_info(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	commonproto::mine_info_list_t* pb_ptr = cli_out_.mutable_mine_list();
	for (int i = 0; i < rank_out_.hset_infos_size(); ++i) {
		const rankproto::hset_info_t& inf = rank_out_.hset_infos(i);
		commonproto::mine_info_t *mine_ptr = pb_ptr->mutable_mine_info(i);
		for (int j = 0; j < inf.fields_size(); ++j) {
			commonproto::mine_team_info_t pb_team_info;
			pb_team_info.ParseFromString(inf.fields(j).value());
			mine_ptr->mutable_team_info()->add_team_list()->CopyFrom(pb_team_info);
		}
		MineUtils::ex_pack_prize_elem_info_for_cli(*mine_ptr, mine_ptr);
		std::string str_mine_id = boost::lexical_cast<std::string>(mine_ptr->mine_id());
		mine_ptr->set_str_mine_id(str_mine_id);
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int GetMineInfoByMineIdCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	switch(player->serv_cmd) {
		case db_cmd_get_player_mine_info:
			return proc_pkg_from_serv_aft_get_mine_info(player, body, bodylen);
		case ranking_cmd_hset_get_info:
			return proc_pkg_from_serv_aft_get_pet_info(player, body, bodylen);
	}
	return 0;
}
int AnewSearchMineCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	//刷矿时，同时要更新raw_data中 buff_type:NEW_MINE_INFO
	//						与OPPONENT_MINE_IDS 这两项
	PARSE_MSG;
	cli_out_.Clear();
	if (GET_A(kAttrMineFightRefreshCdStartTm) && 
			GET_A(kAttrMineFightRefreshCdStartTm) + commonproto::MINE_REFRESH_CD_TIME > NOW()) {
		RET_ERR(cli_err_in_cd_time);
	}

	PlayerUtils::delete_user_raw_data(player->userid,
			player->create_tm, dbproto::MINE_FIGHT_DEF_PET_HP, "0");
	//恢复精灵血量,与清除我更在攻打别人矿mine_flag记录
	MineUtils::reset_player_pets_attack_mine_info_relate(player);

	std::vector<uint64_t> my_mine_ids;
	player->mine_info->get_mine_ids_from_memory(my_mine_ids);
	return MineUtils::req_dbsvr_to_search_mine(player, my_mine_ids);
}

int AnewSearchMineCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	switch(player->serv_cmd) {
		case db_cmd_search_occupyed_mine:
			return proc_pkg_from_serv_aft_anew_search_mine_info(player, body, bodylen);
		case ranking_cmd_hset_get_info:
			return proc_pkg_from_serv_aft_anew_get_mine_pets_info(player, body, bodylen);
	}
	return 0;
}

int AnewSearchMineCmdProcessor::proc_pkg_from_serv_aft_anew_search_mine_info(
		player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	player->mine_info->clear_mine_tmp_info();
	player->mine_info->clear_match_mine_info();
	player->mine_info->clear_match_mine_ids();
	SET_A(kAttrMineFightRefreshCdStartTm, NOW());
	ADD_A(kDailyMineRefreshTimes, 1);
	uint32_t has_searched_cnt = db_out_.list().mine_info_size();
	if (has_searched_cnt == 0) {
		//在DB没有找到匹配的战力玩家，故随机出5个新矿给前端
		MineUtils::save_new_mine_without_garrison(player, NEED_SEARCH_MINE_COUNT);
		MineTmpInfoMap mine_tmp_info_map;
		player->mine_info->get_mine_tmp_map(mine_tmp_info_map);
		FOREACH(mine_tmp_info_map, it) {
			MineUtils::pack_mine_info_for_client_v2(it->second, cli_out_.add_mine_info());
		}
		return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
	}

	uint32_t match_count = std::min(has_searched_cnt, MATCH_MINE_ID_COUNT);
	std::vector<uint32_t> index_vec;
	//随机出match_count 个矿
	Utils::rand_select_k(0, has_searched_cnt - 1, match_count, index_vec);
	//做个保护性措施
	if (index_vec.size() > match_count) {
		RET_ERR(cli_err_search_mine_err);
	}
	
	refresh_mine_session_t* session = (refresh_mine_session_t*)player->session;
	memset(session, 0, sizeof(*session));
	std::vector<uint64_t> mine_id_vec;
	for (uint32_t i = 0; i < match_count; ++i) {
		const commonproto::mine_info_t& inf = db_out_.list().mine_info(index_vec[i]); 
		mine_id_vec.push_back(inf.mine_id());
		//把db中的匹配出的对手矿矿放到内存中
		MineUtils::unpack_mine_info(inf, session->mine_info[i]);
	}
	//去redis中获得对手矿的战斗数据信息
	return MineUtils::req_redis_to_get_teams_info(player, mine_id_vec);
}

int AnewSearchMineCmdProcessor::proc_pkg_from_serv_aft_anew_get_mine_pets_info(
		player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	refresh_mine_session_t* session = (refresh_mine_session_t*)player->session;
	//因为是重新搜矿，所以清除掉以前匹配到的对手矿信息
	player->mine_info->clear_match_mine_info();
	player->mine_info->clear_match_mine_ids();
	for (int i = 0; i < rank_out_.hset_infos_size(); ++i) {
		const rankproto::hset_info_t& inf = rank_out_.hset_infos(i);
		for (uint32_t j = 0; j < NEED_SEARCH_MINE_COUNT; ++j) {
			struct mine_info_t& mine_info = session->mine_info[j];
			std::vector<string> list = split(inf.key(), ':');
			uint64_t mine_id = boost::lexical_cast<uint32_t>(list[1]);
			if (mine_info.mine_id == mine_id) {
				//应该去除匹配到自己的矿
				commonproto::mine_info_t* mine_ptr = cli_out_.add_mine_info();
				MineUtils::pack_mine_info_for_client_v2(mine_info, mine_ptr);
				for (int m = 0; m < inf.fields_size(); ++m) {
					commonproto::mine_team_info_t pb_team_info;
					pb_team_info.ParseFromString(inf.fields(m).value());
					mine_ptr->mutable_team_info()->add_team_list()->CopyFrom(pb_team_info);
				}
				player->mine_info->save_matched_mine_info_to_memory(mine_info);
				player->mine_info->save_match_mine_ids_to_memory(mine_info.mine_id);
				break;
			}
		}
	}
	MineUtils::sync_match_mine_ids_to_db_rawdata(player);
	if (NEED_SEARCH_MINE_COUNT < (uint32_t)rank_out_.hset_infos_size()) {
		RET_ERR(cli_err_sys_err);
	}
	//需要生成新矿的数量
	uint32_t new_mine_count = NEED_SEARCH_MINE_COUNT - rank_out_.hset_infos_size();
	TRACE_TLOG("uid=%u,Will Generate New Mine,Cnt=%u", player->userid, new_mine_count);
	//生成新矿
	MineUtils::save_new_mine_without_garrison(player, new_mine_count);
	MineTmpInfoMap mine_tmp_info_map;
	player->mine_info->get_mine_tmp_map(mine_tmp_info_map);
	TRACE_TLOG("uid=%u,tmp_mine_info=%u", player->userid, mine_tmp_info_map.size());
	FOREACH(mine_tmp_info_map, it) {
		MineUtils::pack_mine_info_for_client_v2(it->second, cli_out_.add_mine_info());
	}


	//删除伙伴身上的，（由别的玩家触发删除掉那些矿）
	//别的玩家触发删除掉那些矿，那么我就在这里清除我的伙伴守矿记录
	std::vector<uint64_t> my_mine_ids;
	player->mine_info->get_mine_ids_from_memory(my_mine_ids);
	if (my_mine_ids.empty()) {
		FOREACH(*player->pets, iter) {
			Pet& pet = iter->second;
			if (pet.defend_mine_id()) {
				pet.set_defend_mine_id(0);
				PetUtils::save_pet(player, pet, false, true);
			}
		}
	} else {
		FOREACH(*player->pets, iter) {
			Pet& pet = iter->second;
			std::vector<uint64_t>::iterator it;
			if (pet.defend_mine_id() == 0) {
				continue;
			}
			it = std::find(my_mine_ids.begin(), my_mine_ids.end(), pet.defend_mine_id());
			if (it == my_mine_ids.end()) {
				pet.set_defend_mine_id(0);
				PetUtils::save_pet(player, pet, false, true);
			}
		}
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int MineSendkMsgCmdProcessor::proc_pkg_from_client(player_t* player,
		const char* body, int bodylen)
{
	PARSE_MSG;
	onlineproto::sc_0x083B_mine_notify_personal_message noti_msg;
	noti_msg.set_sender(player->userid);
	noti_msg.set_s_create_tm(player->create_tm);
	noti_msg.set_msg(cli_in_.content());
	noti_msg.set_sender_nick(player->nick);
	std::vector<uint32_t> recv_id;
	for (int i = 0; i < cli_in_.role_size(); ++i) {
		recv_id.clear();
		if (cli_in_.role(i).userid() == player->userid) {
			continue;
		}
		player_t* ply = g_player_manager->get_player_by_userid(cli_in_.role(i).userid());
		if (ply == NULL) {
			recv_id.push_back(cli_in_.role(i).userid());
			continue;
		}
		//同线的玩家直接通知，不走switch
		send_msg_to_player(ply, cli_cmd_cs_0x083B_mine_notify_personal_message, noti_msg);
	}
	if (!recv_id.empty()) {
		Utils::switch_transmit_msg(switchproto::SWITCH_TRANSMIT_USERS,
				cli_cmd_cs_0x083B_mine_notify_personal_message,
				noti_msg, &recv_id);
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int CheckMyMineExpireCmdProcessor::proc_pkg_from_client(player_t* player,
		const char* body, int bodylen)
{
	PARSE_MSG;
	std::vector<uint64_t> mine_ids;
	player->mine_info->get_mine_ids_from_memory(mine_ids);
	return MineUtils::get_player_mine_info_from_db(player, mine_ids);
}

int CheckMyMineExpireCmdProcessor::proc_pkg_from_serv(player_t* player,
		const char* body, int bodylen)
{
	switch(player->serv_cmd) {
		case db_cmd_get_player_mine_info:
			return proc_pkg_from_serv_aft_get_mine_info(player, body, bodylen);
		case ranking_cmd_hset_get_info:
			return proc_pkg_from_serv_aft_get_defend_pets_info(player, body, bodylen);
	}
	return 0;
}

int CheckMyMineExpireCmdProcessor::proc_pkg_from_serv_aft_get_mine_info(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	std::vector<uint64_t> tmp_mine_id_vec;
	//清掉内存中我的矿场id及其对应的信息，重新赋值（因为db中拉取出来的是最新的矿id以及对应的数据）
	//ps : db中拉取出来的矿，可能已经过期；
	player->mine_info->clear_player_mine_info_from_memory();
	const commonproto::mine_info_list_t& pb_list = db_out_.list();
	for (int i = 0; i < pb_list.mine_info_size(); ++i) {
		struct mine_info_t mine_info;
		MineUtils::unpack_mine_info(pb_list.mine_info(i), mine_info);
		//检查是否是过期矿，过期矿需要删除
		player->mine_info->insert_player_mine_info_to_memory(
				mine_info.mine_id, mine_info);

		tmp_mine_id_vec.push_back(mine_info.mine_id);
	}
	std::vector<uint64_t> mine_ids;
	player->mine_info->get_mine_ids_from_memory(mine_ids);
	//说明有矿过期已经被删除了
	//1. 矿时间到后，被其他协防此矿的玩家触发清除的;2. 是被他人攻占了
	if (mine_ids.size() != tmp_mine_id_vec.size()) {
		player->mine_info->clear_my_mine_ids();
		FOREACH(tmp_mine_id_vec, it) {
			player->mine_info->insert_my_mine_ids_to_memory(*it);
		}
		MineUtils::sync_mine_ids_to_db(player);
		mine_ids.clear();
		player->mine_info->get_mine_ids_from_memory(mine_ids);
	}
	//去redis中拉取出我所有矿场中的精灵信息
	return MineUtils::req_redis_to_get_teams_info(player, mine_ids);
}

int CheckMyMineExpireCmdProcessor::proc_pkg_from_serv_aft_get_defend_pets_info(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	cli_out_.Clear();
	//保存redis中临时的矿id
	std::vector<uint64_t> expire_mine_vec;
	for (int i = 0; i < rank_out_.hset_infos_size(); ++i) {
		const rankproto::hset_info_t& inf = rank_out_.hset_infos(i);
		std::vector<string> list = split(inf.key(), ':');
		uint64_t mine_id = boost::lexical_cast<uint32_t>(list[1]);
		struct mine_info_t mine;
		uint32_t ret = player->mine_info->get_mine_info_from_memory(mine_id, mine);
		if (ret) {
			ERROR_TLOG("Mine id Not Found In Memory,id:%u", mine_id);
			continue;
		}
		//对矿过期的处理
		if (mine.mine_create_tm + mine.duration_time < NOW()) {
			expire_mine_vec.push_back(mine.mine_id);

			std::vector<team_simple_info_t> tmp_def_team_vec;
			for (int index = 0; index < inf.fields_size(); ++index) {
				//能走到这里，说明，该矿虽然过期，但是存储在redis中的ttl时间依然在
				//从redis中拉取出该矿中每个玩家的信息
				commonproto::mine_team_info_t pb_team_info;
				pb_team_info.ParseFromString(inf.fields(index).value());
				team_simple_info_t  team_info;
				team_info.userid = pb_team_info.userid();
				team_info.u_create_tm = pb_team_info.u_create_tm();
				team_info.exploit_start_time = pb_team_info.exploit_start_time();
				const commonproto::pet_list_t& pb_pet_inf = pb_team_info.pet_list();
				//计算矿中该玩家的战力和
				uint32_t def_total_power = 0;
				for (int j = 0; j < pb_pet_inf.pet_list_size(); ++j) {
					uint32_t power = pb_pet_inf.pet_list(j).base_info().power();
					def_total_power += power;
				}
				team_info.total_power = def_total_power;
				tmp_def_team_vec.push_back(team_info);
			}
			if (!tmp_def_team_vec.empty()) {
				//给该矿所有玩家发奖励邮件
				MineUtils::calc_player_get_reward(player,
						tmp_def_team_vec, mine, MINE_TIME_IS_UP);
				//删除我在此矿中的精灵信息
				player->mine_info->delete_pet_info_in_memory(mine.mine_id);
				//MineUtils::sync_mine_pet_info_to_hset(player, mine.mine_id);
				//删除共同守矿的每个人的精灵信息
				std::vector<rankproto::hset_field_t> field_vec;
				FOREACH(tmp_def_team_vec, iter) {
					uint64_t uid_key = ROLE_KEY(ROLE(iter->userid, iter->u_create_tm));
					std::string uid_key_str = boost::lexical_cast<std::string>(uid_key);
					rankproto::hset_field_t pb_field_info;
					pb_field_info.set_name(uid_key_str);
					field_vec.push_back(pb_field_info);
				}
				uint32_t hashset_type = rankproto::HASHSET_MINE_ID_TEAM_INFO_MAP;
				std::string hashset_type_str = boost::lexical_cast<std::string>(hashset_type);
				std::string sub_type = boost::lexical_cast<std::string>(mine_id);
				RankUtils::hset_insert_or_update_str_version(NULL, player->userid,
						player->create_tm, g_server_id, hashset_type_str,
						sub_type, rankproto::REDIS_DELETE,
						&field_vec, DAY_SECS * 7);
			}
			continue;
		}
		/*
		commonproto::mine_info_t *mine_ptr = cli_out_.add_mine_info();
		MineUtils::pack_mine_info_for_client_v2(mine, mine_ptr);
		for (int i = 0; i < inf.fields_size(); ++i) {
			commonproto::mine_team_info_t pb_team_info;
			pb_team_info.ParseFromString(inf.fields(i).value());
			mine_ptr->mutable_team_info()->add_team_list()->CopyFrom(pb_team_info);
		}
		*/
	}
	//找出redis中已经被删除，但是db中还没有被删除的矿
	dbproto::cs_del_mine_info db_in;
	FOREACH(expire_mine_vec, it) {
		//删除该矿信息
		//之所以不用del_mine_id_and_sync_db_rawdata
		//而选用先删除内存中 ids, 再调用一次sync_mine_ids_to_db
		//的原因是，这样可以降低与db通信次数
		//MineUtils::del_mine_id_and_sync_db_rawdata(player, *it);
		player->mine_info->delete_my_mine_id_from_memory(*it);

		player->mine_info->delete_player_mine_info_from_memory(*it);

		db_in.add_mine_id(*it);
	}
	if (db_in.mine_id_size()) {
		MineUtils::sync_mine_ids_to_db(player);
		g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
				db_cmd_del_mine_info, db_in);
	}
	//清除所有精灵身上过期矿的mine_fight_flag
	std::vector<uint64_t> mine_ids;
	player->mine_info->get_mine_ids_from_memory(mine_ids);
	FOREACH(*player->pets, iter) {
		Pet& pet = iter->second;
		if (pet.defend_mine_id()) {
			std::vector<uint64_t>::iterator it;
			it = std::find(mine_ids.begin(), mine_ids.end(), pet.defend_mine_id());
			if (it != mine_ids.end()) {
				//找到说明没过期，伙伴正在守矿
				continue;
			}
			pet.set_defend_mine_id(0);
			PetUtils::save_pet(player, pet, false, true);
		}
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}
