#include <boost/lexical_cast.hpp>
#include "arena_processor.h"
#include "cmd_processor_interface.h"
#include "rank_utils.h"
#include "service.h"
#include "global_data.h"
#include "data_proto_utils.h"
#include "common.h"
#include "prize.h"
#include "family_utils.h"
#include "player_manager.h"
#include "task_utils.h"
#include "player_utils.h"
#include "arena_conf.h"
#include "mail_utils.h"
#include "pet_utils.h"

int GetArenaPlayerDataCmdProcessor::proc_pkg_from_client(
		player_t*player, const char *body, int bodylen)
{
	PARSE_MSG;

	if (GET_A(kAttrLv) < ARENA_OPEN_LIMIT) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_player_lv_not_get_arena_limit);
	}
	if (NOW() > GET_A(kAttrArenaRefreshCdStartTm) &&
			NOW() - GET_A(kAttrArenaRefreshCdStartTm) < ARENA_REFRESH_TIMER_DIFF) {
		RET_ERR(cli_err_refresh_count_too_frequently);
	}
	//获取总榜与上周周榜的排名
	std::vector<rank_key_order_t> key_vec;
	struct rank_key_order_t tmp;
	tmp.key.key = commonproto::RANKING_ARENA;
	tmp.key.sub_key = 0;
    tmp.order = commonproto::RANKING_ORDER_ASC;
	key_vec.push_back(tmp);
	//uint32_t week_sub_key = 0;
	//ArenaUtils::get_last_week_arena_rank_sub_key(week_sub_key);
	uint32_t day_sub_key = 0;
	if (TimeUtils::test_gived_time_exceed_tm_point(NOW(), ARENA_DAILY_REWARD_TM_PONIT)) {
		day_sub_key = TimeUtils::time_to_date(NOW());
	} else {
		day_sub_key = TimeUtils::time_to_date(NOW() - DAY_SECS);
	}
	tmp.key.key = commonproto::RANKING_ARENA;
	tmp.key.sub_key = day_sub_key;
	tmp.order = commonproto::RANKING_ORDER_ASC;
	key_vec.push_back(tmp);
	//刷新次数增1，挑战一次竞技场后清零
	ADD_A(kDailyArenaRefreshCount, 1);
	if (GET_A(kDailyArenaRefreshCount) >= 3) {
		SET_A(kAttrArenaRefreshCdStartTm, NOW());
	}
	return RankUtils::get_user_rank_info_by_keys(
			player, key_vec, player->userid, player->create_tm);
}

int GetArenaPlayerDataCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char *body, int bodylen)
{
	switch (player->serv_cmd) {
		case ranking_cmd_get_user_multi_rank:
			return proc_pkg_from_serv_get_multi_rank(player, body, bodylen);

		case ranking_cmd_rank_insert_last:
			return proc_pkg_from_serv_rank_insert_last(player, body, bodylen);

		case ranking_cmd_get_ranking_users:
			return proc_pkg_from_serv_aft_get_rank_users(player, body, bodylen);

		case cache_cmd_ol_req_users_info:
			return proc_pkg_from_serv_aft_get_users_info(player, body, bodylen);

		case ranking_cmd_get_battle_report_key:
			return proc_pkg_from_serv_aft_get_btl_report_key(player, body, bodylen);

		default:
			return 0;
	}
	return 0;
}

int GetArenaPlayerDataCmdProcessor::proc_pkg_from_serv_get_multi_rank(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(multi_rank_out_);
	uint32_t rank = 0;
	uint32_t yesterday_rank = 0;
	const commonproto::rank_player_info_t& inf = multi_rank_out_.rank_info(0);
	rank = inf.rank();
	SET_A(kAttrArenaRank, rank);
	const commonproto::rank_player_info_t& inf2 = multi_rank_out_.rank_info(1);
	yesterday_rank = inf2.score();
	SET_A(kAttrArenaLastRank, yesterday_rank);
	player->temp_info.get_self_rank = rank;
	player->temp_info.arena_week_rank = yesterday_rank;
	//如果总榜排名为0,说明玩家还没参加过竞技场，则将玩家插入排名的最后
	if (rank == 0) {
		return RankUtils::rank_insert_last(
				player, player->userid, player->create_tm, 
				commonproto::RANKING_ARENA,
				0, commonproto::RANKING_ORDER_ASC);
	}

	std::vector<uint32_t> rank_list;
	//RankUtils::inter_partition_select_challenge_rank_list(rank, rank_list);
	RankUtils::select_challenge_rank_list(rank, rank_list, GET_A(kAttrArenaWinStreak));
	
	uint32_t type = commonproto::RANKING_ARENA;
	return RankUtils::get_user_id_by_rank(player, type, 0, rank_list);
}

int GetArenaPlayerDataCmdProcessor::proc_pkg_from_serv_rank_insert_last(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(insert_last_out_);
	uint32_t rank = insert_last_out_.rank_info().rank();
	//都已经插入排名了，所以这里排名一定存在，不存在则有问题了
	if (rank == 0) {
		ERROR_TLOG("insert rank, but rank still zero,uid=[%u],rank=[%u]", player->userid, rank);
	}
	assert(rank);
	SET_A(kAttrArenaRank, rank);
	player->temp_info.get_self_rank = rank;

	std::vector<uint32_t> rank_list;
	//RankUtils::inter_partition_select_challenge_rank_list(rank, rank_list);
	RankUtils::select_challenge_rank_list(rank, rank_list, GET_A(kAttrArenaWinStreak));
	
	uint32_t type = commonproto::RANKING_ARENA;
	return RankUtils::get_user_id_by_rank(player, type, 0, rank_list);
}

int GetArenaPlayerDataCmdProcessor::proc_pkg_from_serv_aft_get_rank_users(
		player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(rank_userid_out_);

	uint32_t player_info_size = rank_userid_out_.player_info_size();

	(*player->temp_info.m_arena_index_info).clear();

	cacheproto::cs_batch_get_users_info cache_info_in_;
	for (uint32_t i = 0; i < player_info_size && i < ARENA_PLAYER_NUM; ++i) {
		uint32_t userid = rank_userid_out_.player_info(i).userid();
		uint32_t rank = rank_userid_out_.player_info(i).rank();
        // TODO kevin confrim
        uint32_t u_create_tm = rank_userid_out_.player_info(i).u_create_tm();
        //rank_userid_out_.player_info(i).u_create_tm();
		commonproto::role_info_t* pb_ptr = cache_info_in_.add_roles();
		pb_ptr->set_userid(userid);
		pb_ptr->set_u_create_tm(u_create_tm);
		//cache_info_in_.add_uids(userid);
		//跨协议的临时数据保存，通常存放在player_t::temp_info_t 中；
		//这里保存的挑战对手的信息，给挑战协议使用
		uint64_t role_key = ROLE_KEY(ROLE(userid, u_create_tm));
		(*player->temp_info.m_arena_index_info)[rank] = role_key;
	}
	g_dbproxy->send_msg(
			player, player->userid, player->create_tm,
			cache_cmd_ol_req_users_info, cache_info_in_);
	return 0;
}

int GetArenaPlayerDataCmdProcessor::proc_pkg_from_serv_aft_get_users_info(
		player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(cache_info_out_);
	cli_out_.Clear();
	uint32_t errs_size = cache_info_out_.errs_size();
	for (uint32_t i = 0; i < errs_size; ++i) {
		uint32_t userid = cache_info_out_.errs(i).role().userid();
		uint32_t create_tm = cache_info_out_.errs(i).role().u_create_tm();
		uint64_t role_key = ROLE_KEY(ROLE(userid, create_tm));
		FOREACH(*(player->temp_info.m_arena_index_info), m_iter) {
			if (m_iter->second == role_key) {
				player->temp_info.m_arena_index_info->erase(m_iter);
				break;
			}
		}
	}

	uint32_t cache_info_size = cache_info_out_.user_infos_size();
	for (uint32_t i = 0; i < cache_info_size; ++i) {
		const commonproto::battle_player_data_t &player_info = cache_info_out_.user_infos(i);
		onlineproto::arena_player_data_t *player_data = cli_out_.add_player_data_list();
		player_data->mutable_player_base_info()->CopyFrom(player_info.base_info());
		player_data->mutable_equip_list()->CopyFrom(player_info.equip_list());
		FOREACH(*(player->temp_info.m_arena_index_info), it) {
			uint32_t userid = player_info.base_info().user_id();
			uint32_t create_tm = player_info.base_info().create_tm();
			uint64_t role_key = ROLE_KEY(ROLE(userid, create_tm));
			if (it->second == role_key) {
				uint32_t rank = it->first;
				player_data->set_rank(rank);
				break;
			}
		}
	}

    CACHE_OUT_MSG(cli_out_);
	rankproto::cs_get_battle_report_key req_in_;
	req_in_.set_type(commonproto::RANKING_ARENA);
	req_in_.set_count(10); //默认拉取最近的10份战报key
	req_in_.mutable_role()->set_userid(player->userid);	
	req_in_.mutable_role()->set_u_create_tm(player->create_tm);
	req_in_.set_flag(1);	//拉取key的同时，也拉取出相应的战报
	req_in_.set_server_id(g_server_id);
	return g_dbproxy->send_msg(
			player, player->userid, player->create_tm,
			ranking_cmd_get_battle_report_key, 
			req_in_);
}

int GetArenaPlayerDataCmdProcessor::proc_pkg_from_serv_aft_get_btl_report_key(
		player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(rank_key_out_);
    PARSE_OUT_MSG(cli_out_);
	uint32_t btl_inf_size = rank_key_out_.btl_inf_size();
	for (uint32_t i = 0; i < btl_inf_size; ++i) {
		cli_out_.add_key(rank_key_out_.btl_inf(i).key());
	}
	for (uint32_t i = 0; i < btl_inf_size; ++i) {
		commonproto::battle_report_info_t * btl_ptr = cli_out_.add_user_btl_report();
		btl_ptr->ParseFromString(rank_key_out_.btl_inf(i).pkg());
	}
	Utils::write_msglog_new(player->userid, "功能", "排位赛", "排位赛进入");
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int ChallengeArenaPlayerCmdProcessor::proc_pkg_from_client(
		player_t*player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t ret = ArenaUtils::check_challenge_arena_condition(player);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	
	uint32_t rank = cli_in_.rank();
	std::map<uint32_t, uint64_t>::const_iterator it;
	it = (*player->temp_info.m_arena_index_info).find(rank);
	if (it == (*player->temp_info.m_arena_index_info).end()) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_challenge_arena_player_rank);
	}
	/*
	if (!is_valid_uid(it->second)) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_userid_not_find);
	}
	*/
	player->temp_info.ai_id = it->second;

    // 更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_CHALLENGE_ARENA, 1);

	challenge_arena_player_session_t* session = (challenge_arena_player_session_t*)player->session;
	memset(session, 0, sizeof(*session));
	session->atk_tick = cli_in_.atk_tick();
	session->def_tick = cli_in_.def_tick();

	player_t *ai_player_ptr = g_player_manager->get_player_by_userid(it->second);
	//若ai 同服在线:直接打包战斗信息，并保存战报
	if (ai_player_ptr) {
		player->temp_info.ai_nick->clear();
		*player->temp_info.ai_nick = ai_player_ptr->nick;
		cli_out_.Clear();
		DataProtoUtils::pack_pk_players_btl_info_include_tick(
				player, ai_player_ptr, 
				cli_in_.atk_tick(), cli_in_.def_tick(), 
				cli_out_.mutable_btl_data(),
				commonproto::GROUND_ARENA);
		return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
	}
	//若ai 不在线,或者在线却不同服：则先去缓存拉取ai 的战斗信息
	cacheproto::cs_batch_get_users_info cache_info_in_;
	commonproto::role_info_t* pb_ptr = cache_info_in_.add_roles();
	role_info_t role_info = KEY_ROLE(player->temp_info.ai_id);
	pb_ptr->set_userid(role_info.userid);
	pb_ptr->set_u_create_tm(role_info.u_create_tm);
	g_dbproxy->send_msg(
			player, player->userid, player->create_tm,
			cache_cmd_ol_req_users_info, 
			cache_info_in_);
	return 0;
}

int ChallengeArenaPlayerCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char *body, int bodylen)
{
	switch(player->serv_cmd) {
		case cache_cmd_ol_req_users_info:
			return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
		default:
			return 0;
	}
	return 0;
}

int ChallengeArenaPlayerCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(cache_info_out_);
	cli_out_.Clear();
	uint32_t user_infos_size = cache_info_out_.user_infos_size();
	if (user_infos_size == 0) {
		player->temp_info.ai_id = 0;
		player->temp_info.ai_nick->clear();
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_pk_data_info_not_found);
	}
	*player->temp_info.ai_nick = cache_info_out_.user_infos(0).base_info().nick();

	challenge_arena_player_session_t* session = (challenge_arena_player_session_t*)player->session;
	commonproto::battle_data_include_tick_t* btl_ptr = cli_out_.mutable_btl_data();
	commonproto::battle_player_data_t* def_btl_ptr = btl_ptr->add_players();
	def_btl_ptr->CopyFrom(cache_info_out_.user_infos(0));
	commonproto::battle_player_data_t* atk_btl_ptr = btl_ptr->add_players();
	DataProtoUtils::pack_player_battle_all_info(player, atk_btl_ptr);

	ArenaUtils::arena_modify_player_btl_attr(*def_btl_ptr);
	ArenaUtils::arena_modify_player_btl_attr(*atk_btl_ptr);

	RankUtils::save_arena_battle_report(player, 
			*def_btl_ptr, *atk_btl_ptr,	
			session->atk_tick, session->def_tick);

	btl_ptr->set_atk_tick(session->atk_tick);
	btl_ptr->set_def_tick(session->def_tick);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int GetRankListCmdProcessor::proc_pkg_from_client(
		player_t*player, const char *body, int bodylen)
{
	PARSE_MSG;

	uint32_t type = cli_in_.type();
	uint32_t sub_type = cli_in_.sub_type();
	uint32_t page_no = cli_in_.page_no();
	uint32_t page_size = cli_in_.page_size();
    uint32_t order = cli_in_.order();
	if (page_size > kMaxRankPlayerNum) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_req_rank_num_exceed_limit);
	}
	get_arena_ranking_session_t * session = (get_arena_ranking_session_t*)player->session;
    memset(session, 0, sizeof(*session));

	session->type = type;
	session->sub_type = sub_type;
	
	uint32_t start = (page_no - 1) * page_size;
	uint32_t end = page_no * page_size - 1;
	RankUtils::get_rank_list_info(player, type, sub_type, start, end, order);
	
	return 0;
}

int GetRankListCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char *body, int bodylen)
{
	switch(player->serv_cmd) {
	case ranking_cmd_get_ranking_list:
		return proc_pkg_from_serv_aft_get_rank_list(player, body, bodylen);
	case cache_cmd_ol_req_users_info:
		return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
    case db_cmd_family_get_info:
        return proc_pkg_from_serv_aft_get_family_info(player, body, bodylen);
	}
	return 0;
}

int GetRankListCmdProcessor::proc_pkg_from_serv_aft_get_rank_list(
        player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(rank_list_out_);

	get_arena_ranking_session_t * session = (get_arena_ranking_session_t*)player->session;

    if (RankUtils::get_ranking_info_type(session->type, session->sub_type) ==
            commonproto::RANKING_INFO_FAMILY) {
        session->total = rank_list_out_.total();
        session->count = rank_list_out_.count();

        if (session->total == 0 || session->count == 0) {
            cli_out_.Clear();
            cli_out_.set_total(session->total);
            cli_out_.set_count(session->count);
            cli_out_.set_type(session->type);
            cli_out_.set_sub_type(session->sub_type);
            return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
        }

        for (int i = 0; i < rank_list_out_.user_info_size(); ++i) {
            const commonproto::rank_player_info_t &rank_info = rank_list_out_.user_info(i);
            session->rank_info[i].user_id = rank_info.userid();
			session->rank_info[i].create_tm = rank_info.u_create_tm();
            session->rank_info[i].rank = rank_info.rank();
            session->rank_info[i].score = rank_info.score();
        }

        cli_out_.Clear();
        cli_out_.set_total(0);
        cli_out_.set_count(0);
        CACHE_OUT_MSG(cli_out_);
        uint32_t family_id = rank_list_out_.user_info(session->info_index).userid();
        db_family_info_in_.Clear();
        return g_dbproxy->send_msg(
                player, family_id, player->create_tm, 
                db_cmd_family_get_info, db_family_info_in_);
    } else {
        uint32_t count = rank_list_out_.count();
        uint32_t total = rank_list_out_.total();
        session->total = total;
        session->count = count;
        cacheproto::cs_batch_get_users_info  cache_in_;
        for (int i = 0; i < rank_list_out_.user_info_size(); ++i) {
            const commonproto::rank_player_info_t &rank_info = rank_list_out_.user_info(i);
            session->rank_info[i].user_id = rank_info.userid();
			session->rank_info[i].create_tm = rank_info.u_create_tm();
            session->rank_info[i].rank = rank_info.rank();
            session->rank_info[i].score = rank_info.score();

            //如果不是玩家自己 则去缓存拉详细信息
            if (!(rank_info.userid() == player->userid && 
				rank_info.u_create_tm() == player->create_tm)) {
                //cache_in_.add_roles(rank_info.userid());
				commonproto::role_info_t* pb_ptr = cache_in_.add_roles();
				pb_ptr->set_userid(rank_info.userid());
				pb_ptr->set_u_create_tm(rank_info.u_create_tm());
            }

            //如果是玩家自己 则额外保存一份数据
            if (rank_info.userid() == player->userid &&
				rank_info.u_create_tm() == player->create_tm) {
                session->self_rank_data.user_id = player->userid;
				session->self_rank_data.create_tm = player->create_tm;
                session->self_rank_data.rank = rank_info.rank();
                session->self_rank_data.score = rank_info.score();
            }
        }

        if (rank_list_out_.has_self_info()) {//玩家自己有排名且不在拉取列表中
            const commonproto::rank_player_info_t &self_info = rank_list_out_.self_info();
            session->self_rank_data.user_id = player->userid;
			session->self_rank_data.create_tm = player->create_tm;
            session->self_rank_data.rank = self_info.rank();
            session->self_rank_data.score = self_info.score();
        }

        return g_dbproxy->send_msg(
                player, player->userid, player->create_tm, 
                cache_cmd_ol_req_users_info, cache_in_);
    }

	return 0;
}

int GetRankListCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
		player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(cache_out_);
	cli_out_.Clear();
	get_arena_ranking_session_t * session = (get_arena_ranking_session_t*)player->session;

    uint32_t not_exist_cnt = 0;
    for (uint32_t j = 0; j < session->count; ++j) {
        user_rank_info_t *rinf = &(session->rank_info[j]);
        uint32_t uid = rinf->user_id;
		uint32_t create_tm = rinf->create_tm;
        uint32_t rank = rinf->rank;
        uint64_t score = rinf->score;
        bool found = false;
		commonproto::battle_player_data_t cache_player;
        for (int i = 0; i < cache_out_.user_infos_size(); i++) {
			cache_player.Clear();
            cache_player = cache_out_.user_infos(i);
			if (!(cache_player.base_info().user_id() == uid &&
					cache_player.base_info().create_tm() == create_tm)) {
				continue;
			} else {
				found = true;
				break;
			}
		}
        if (!found && (!(uid == player->userid && create_tm == player->create_tm))) {//没找到对应的玩家详细信息
			not_exist_cnt++;
			continue;
        }
        commonproto::rank_player_detail_data_t* player_data = cli_out_.add_player_list();

        if (uid == player->userid && create_tm == player->create_tm) { //如果是自己
            DataProtoUtils::pack_player_base_info(player, player_data->mutable_player_base_info());
            DataProtoUtils::pack_player_equip_info(player, player_data->mutable_equip_list());
            player_data->set_rank(rank);
            player_data->set_score(score);
            continue;
        }
		//找到对应的信息
		player_data->mutable_player_base_info()->CopyFrom(cache_player.base_info());
		player_data->mutable_equip_list()->CopyFrom(cache_player.equip_list());
		player_data->set_rank(rank);
		player_data->set_score(score);
    }

    session->count -= not_exist_cnt;

    if (session->self_rank_data.user_id) {//有自己的排名
        //commonproto::rank_player_info_t *self_inf = cli_out_.mutable_self_rank_data();
		commonproto::rank_player_detail_data_t *self_info = cli_out_.mutable_self_rank_data();
		DataProtoUtils::pack_player_base_info(player, self_info->mutable_player_base_info());
		DataProtoUtils::pack_player_equip_info(player, self_info->mutable_equip_list());
		
        self_info->set_rank(session->self_rank_data.rank);
        self_info->set_score(session->self_rank_data.score);
    }
    
	//Confirm kevin: 策划要求最多显示15页
	/*
	if (session->total > RANK_TOTAL_RANGE_LIMIT) {
		session->total = RANK_TOTAL_RANGE_LIMIT;
	}
	*/
	cli_out_.set_total(session->total);
	cli_out_.set_count(session->count);
	cli_out_.set_type(session->type);
	cli_out_.set_sub_type(session->sub_type);
	cli_out_.set_order((commonproto::rank_order_t)session->order);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int GetRankListCmdProcessor::proc_pkg_from_serv_aft_get_family_info(
        player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(db_family_info_out_);
    PARSE_OUT_MSG(cli_out_);

    get_arena_ranking_session_t * session = (get_arena_ranking_session_t*)player->session;
    const commonproto::family_info_t &info = db_family_info_out_.family_info();
    if (FamilyUtils::is_valid_family_id(info.family_id())) {
        commonproto::family_rank_info_t* family_data = cli_out_.add_family_list();

        family_data->set_rank(session->rank_info[session->info_index].rank);
        family_data->set_family_id(session->rank_info[session->info_index].user_id);
        family_data->set_family_name(info.family_name());
        family_data->set_member_num(info.member_num());
        family_data->set_total_battle_value(info.total_battle_value());
        family_data->set_family_level(info.level());
        family_data->set_join_type(info.join_type());
        family_data->set_base_join_value(info.base_join_value());

        // 同步家族排名
        if ((session->sub_type == commonproto::FAMILY_RANK_SUB_TYPE_8 || 
                session->sub_type == commonproto::FAMILY_RANK_SUB_TYPE_11) &&
                session->rank_info[session->info_index].score != (uint64_t)info.total_battle_value()) {
            FamilyUtils::update_family_rank_score(info.family_id(), info);
        } else if ((session->sub_type == commonproto::FAMILY_RANK_SUB_TYPE_9 ||
                    session->sub_type == commonproto::FAMILY_RANK_SUB_TYPE_12)  &&
                session->rank_info[session->info_index].score != (uint64_t)info.member_num()) {
            FamilyUtils::update_family_rank_score(info.family_id(), info);
        } else if ((session->sub_type == commonproto::FAMILY_RANK_SUB_TYPE_7 ||
                    session->sub_type == commonproto::FAMILY_RANK_SUB_TYPE_10)  &&
                session->rank_info[session->info_index].score != (uint64_t)info.level()) {
            FamilyUtils::update_family_rank_score(info.family_id(), info);
        } else if ((session->sub_type == commonproto::FAMILY_RANK_SUB_TYPE_100 ||
                    session->sub_type == commonproto::FAMILY_RANK_SUB_TYPE_200)  &&
                session->rank_info[session->info_index].score != (uint64_t)info.base_join_value()) {
            FamilyUtils::update_family_rank_score(info.family_id(), info);
        }
    } else {
        // 对应id的家族不存在，删除排名信息
        uint32_t invalid_family_id = session->rank_info[session->info_index].user_id;
        FamilyUtils::del_family_rank_info(invalid_family_id); 
    }
    session->info_index++;

    if (session->info_index >= session->count) {
        // 本页家族信息列表查询完成
        cli_out_.set_total(session->total);
        cli_out_.set_count(session->count);
        cli_out_.set_type(session->type);
        cli_out_.set_sub_type(session->sub_type);
        return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    } 

    // 继续查询
    cli_out_.set_total(session->total);
    cli_out_.set_count(session->count);
    CACHE_OUT_MSG(cli_out_);
    uint32_t family_id = session->rank_info[session->info_index].user_id;
    db_family_info_in_.Clear();
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_info, db_family_info_in_);
}

int ArenaResultCmdProcessor::proc_pkg_from_client(
		player_t*player, const char *body, int bodylen)
{
	PARSE_MSG;
	if (player->temp_info.ai_id == 0) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_arena_result_must_after_pk);
	}
	role_info_t role_info = KEY_ROLE(player->temp_info.ai_id);
	if (!is_valid_uid(role_info.userid)) {
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_uid_invalid);
	}

	if (player->temp_info.tmp_btl_info == NULL) {
		player->temp_info.ai_id = 0;
		player->temp_info.ai_nick->clear();
		return send_err_to_player(
				player, player->cli_wait_cmd, player->create_tm,
				cli_err_pk_data_info_not_found);
	}

	if (GET_A(kDailyArenaRefreshCount)) {
		SET_A(kDailyArenaRefreshCount, 0);
	}
	SET_A(kAttrArenaRefreshCdStartTm, 0);

	AttrUtils::add_attr_in_special_time_range(player, 
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivArenaCnt);

	dbproto::cs_get_attr db_msg;
	db_msg.add_type_list(kAttrArenaWinStreak);
	return g_dbproxy->send_msg(
			player, role_info.userid, role_info.u_create_tm,
			db_cmd_get_attr, db_msg);
}

int ArenaResultCmdProcessor::proc_pkg_from_serv(player_t* player, const char *body, int bodylen)
{
	switch (player->serv_cmd) {
		case ranking_cmd_get_users_rank:
			return proc_pkg_from_serv_aft_get_users_rank(player, body, bodylen);

		case db_cmd_get_attr:
			return proc_pkg_from_serv_aft_get_ai_attr(player, body, bodylen);
		default:
			return 0;
	}
	return 0;
}

int ArenaResultCmdProcessor::proc_pkg_from_serv_aft_get_ai_attr(
		player_t* player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);
	uint32_t ai_streak_count = db_out_.attrs(0).value();

	uint32_t win_streak = GET_A(kAttrArenaWinStreak);
	uint32_t result = cli_in_.type();
	if (result == commonproto::WIN) {
		++win_streak;
		if (win_streak >= ARENA_STREAK_FLAG) {
			//跨服通知
			ArenaUtils::arena_push_killing_spree(player, win_streak);
			//连胜发奖励
			ArenaUtils::win_streak_get_reward(player, win_streak);
		}
		SET_A(kAttrArenaWinStreak, win_streak);
		player->temp_info.arena_result = commonproto::WIN;

		//终结ai的连胜次数
		if (ai_streak_count) {
			role_info_t ai_role_info = KEY_ROLE(player->temp_info.ai_id);
			AttrUtils::change_other_attr_value_pub(
					ai_role_info.userid, ai_role_info.u_create_tm,
					kAttrArenaWinStreak, ai_streak_count, true);
		}
	} else {
		//推送最高连胜被终结
		if (win_streak >= ARENA_STREAK_FLAG) {
			ArenaUtils::arena_push_streak_end(player);
		}
		SET_A(kAttrArenaWinStreak, 0);
		player->temp_info.arena_result = commonproto::LOSE;
	}
	std::vector<role_info_t> role_vec;
	role_info_t role_info;
	role_info.userid= player->userid;
	role_info.u_create_tm = player->create_tm;
	role_vec.push_back(role_info);

	role_info = KEY_ROLE(player->temp_info.ai_id);
	role_vec.push_back(role_info);
	return RankUtils::get_user_rank_info(
			player, commonproto::RANKING_ARENA, 
			0, role_vec, commonproto::RANKING_ORDER_ASC);
}

int ArenaResultCmdProcessor::proc_pkg_from_serv_aft_get_users_rank(
		player_t* player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(user_rank_out_);
	arena_result_session_t* session = (arena_result_session_t*)player->session;
	memset(session, 0, sizeof(*session));
	uint32_t rank_info_size = user_rank_out_.rank_info_size();
	role_info_t role_info = KEY_ROLE(player->temp_info.ai_id);
	for (uint32_t i = 0; i < rank_info_size; ++i) {
		const commonproto::rank_player_info_t &player_info = user_rank_out_.rank_info(i);
		if (player->userid == player_info.userid() &&
			player->create_tm == player_info.u_create_tm()) {
			session->rank_info[0].user_id = player_info.userid();
			session->rank_info[0].create_tm = player_info.u_create_tm();
			session->rank_info[0].rank = player_info.rank();
			session->rank_info[0].score = player_info.score();
		} else if (role_info.userid == player_info.userid() &&
				role_info.u_create_tm == player_info.u_create_tm()) {
			session->rank_info[1].user_id = player_info.userid();
			session->rank_info[1].create_tm = player_info.u_create_tm();
			session->rank_info[1].rank = player_info.rank();
			session->rank_info[1].score = player_info.score();
		} else {
			return send_err_to_player(player, player->cli_wait_cmd,
					cli_err_data_error);
		}
	}

	SET_A(kDailyArenaLastChallengeEndTime, NOW());
	ADD_A(kDailyArenaChallengeTimes, 1);
	//若胜利：交换排名，发奖励
	uint32_t day_sub_key = 0;
	if (TimeUtils::test_gived_time_exceed_tm_point(NOW(), ARENA_DAILY_REWARD_TM_PONIT)) {
		day_sub_key = TimeUtils::time_to_date(NOW() + DAY_SECS);
	} else {
		day_sub_key = TimeUtils::time_to_date(NOW());
	}
	uint32_t old_rank = 0, new_rank = 0;
	if (player->temp_info.arena_result == commonproto::WIN) {
		if (session->rank_info[0].rank > session->rank_info[1].rank) {
			//
			uint32_t history_rank = GET_A(kAttrArenaHistoryMaxRank);
			if (session->rank_info[1].rank < history_rank) {
				SET_A(kAttrArenaHistoryMaxRank, session->rank_info[1].rank);
			}
			role_info_t role_info = KEY_ROLE(player->temp_info.ai_id);
			RankUtils::switch_user_rank(
					commonproto::RANKING_ARENA, day_sub_key, 
					player->userid, player->create_tm,
					role_info.userid, role_info.u_create_tm);
			if (session->rank_info[1].rank <= ARENA_RANK_THRESHOLD) {
				static char msg[256];
				snprintf(msg, sizeof(msg), " [pi=%u|%u]%s[/pi]击败了[pi=%u|%u]%s[/pi]成为了排位赛第%u名", 
						player->userid, player->create_tm, player->nick,
						session->rank_info[1].user_id, session->rank_info[1].create_tm,
						(*player->temp_info.ai_nick).c_str(), session->rank_info[1].rank);

				std::string noti_msg; 
				noti_msg.assign(msg);
				onlineproto::sc_0x012A_system_notice msg_out_;
				msg_out_.set_type(0);
				msg_out_.set_content(msg);
				std::vector<uint32_t> svr_ids;
				svr_ids.push_back(g_server_id);
				Utils::switch_transmit_msg(
						switchproto::SWITCH_TRANSMIT_SERVERS,
						cli_cmd_cs_0x012A_system_notice, msg_out_,
						0, &svr_ids);
			}
			//获得本次挑战的奖励（排名上升）
			ArenaUtils::get_reward_after_challenge(player, session->rank_info[1].rank);
			//修改ai玩家记录在属性表中的排名
			role_info_t ai_role_info = KEY_ROLE(player->temp_info.ai_id);
			AttrUtils::change_other_attr_value_pub(
					ai_role_info.userid, ai_role_info.u_create_tm, kAttrArenaRank, 
					session->rank_info[0].rank - session->rank_info[1].rank, 
					false);
			SET_A(kAttrArenaRank, session->rank_info[1].rank);
			old_rank = session->rank_info[0].rank;
			new_rank = session->rank_info[1].rank;
			//通知ai玩家，被揍了，让他刷新0x217协议
			/*
			player_t* ai_player = g_player_manager->get_player_by_userid(
					ai_role_info.userid);
			if (ai_player) {
				onlineproto::sc_0x0252_notify_update_arena_data noti_ai_msg;
				send_msg_to_player(ai_player, 
						cli_cmd_cs_0x0252_notify_update_arena_data,
						noti_ai_msg);
			} else {
				//尝试跨线通知
				onlineproto::sc_0x0252_notify_update_arena_data noti_ai_msg;
				std::vector<uint32_t> recv_ids;
				recv_ids.push_back(ai_role_info.userid);
				Utils::switch_transmit_msg(
					switchproto::SWITCH_TRANSMIT_USERS,
					cli_cmd_cs_0x0252_notify_update_arena_data,
					noti_ai_msg, &recv_ids);
			}
			*/
		} else {
			//获得本次挑战的奖励（排名不变）
			ArenaUtils::get_reward_after_challenge(player, session->rank_info[0].rank);
			//在周榜中记录玩家原来的排名
			RankUtils::rank_user_insert_score(
					player->userid, player->create_tm,
					commonproto::RANKING_ARENA, day_sub_key,
					session->rank_info[0].rank);
			//新旧排名不变
			old_rank = session->rank_info[0].rank;
			new_rank = session->rank_info[0].rank;
			if (GET_A(kAttrArenaHistoryMaxRank) == 0) {
				SET_A(kAttrArenaHistoryMaxRank, new_rank);
			}
		}
	} else {
		//在周榜中记录玩家以及ai原来的排名
		RankUtils::rank_user_insert_score(
				player->userid, player->create_tm,
				commonproto::RANKING_ARENA, day_sub_key, 
				session->rank_info[0].rank);

		onlineproto::sc_0x0112_notify_get_prize noti_prize_msg;
		transaction_proc_prize(
				player, ARENA_COMFORT_PRIZE, noti_prize_msg, 
				commonproto::PRIZE_REASON_ARENA_GIFT);

		old_rank = session->rank_info[0].rank;
		new_rank = session->rank_info[0].rank;
		if (GET_A(kAttrArenaHistoryMaxRank) == 0) {
			SET_A(kAttrArenaHistoryMaxRank, new_rank);
		}
	}
	//保存战报
	uint32_t result = cli_in_.type();
	std::string btl_key;
	RankUtils::save_btl_report_to_redis(
			player, commonproto::ARENA, player->temp_info.ai_id,
			(commonproto::challenge_result_type_t)result, btl_key,
			old_rank, new_rank);
	//只要不失败，都清零
	if (player->temp_info.arena_result !=  commonproto::LOSE) {
		SET_A(kDailyArenaLastChallengeEndTime, 0);
	}
	player->temp_info.ai_id = 0;
	player->temp_info.tmp_btl_info->clear();
	player->temp_info.ai_nick->clear();

	cli_out_.Clear();
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int GetRankInfoCmdProcessor::proc_pkg_from_client(
		player_t*player, const char *body, int bodylen)
{
	PARSE_MSG;

    if (cli_in_.role_size() > MAX_GET_RANK_INFO_USER_NUM) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_too_many_rank_info_user);
    }

    std::vector<role_info_t> role_vec;
    int i = 0;
    for (; i < cli_in_.role_size(); i++) {
	    uint32_t userid = cli_in_.role(i).userid();
		uint32_t create_tm = cli_in_.role(i).u_create_tm();
		role_info_t role_info;
        if (!is_valid_uid(userid)) {
		    return send_err_to_player(player, player->cli_wait_cmd, cli_err_userid_not_find);
        }
		role_info.userid = userid;
		role_info.u_create_tm = create_tm;
        role_vec.push_back(role_info);
    }
    get_rank_info_session_t * session = (get_rank_info_session_t*)player->session;
	session->total_user_count = i;
    session->type = cli_in_.type();
    session->sub_type = cli_in_.sub_type();
	
	return RankUtils::get_user_rank_info(player, 
			cli_in_.type(), 
			cli_in_.sub_type(), 
			role_vec, 
			cli_in_.order()); 
}

int GetRankInfoCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char *body, int bodylen)
{
	switch(player->serv_cmd) {
		case ranking_cmd_get_users_rank:
			return proc_pkg_from_serv_aft_get_rank_info(player, body, bodylen);
		case cache_cmd_ol_req_users_info:
			return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
	}
	return 0;
}

int GetRankInfoCmdProcessor::proc_pkg_from_serv_aft_get_rank_info(
		player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(rank_info_out_);

    // 查询单个家族排名时不拉取附加信息
    if (RankUtils::get_ranking_info_type(cli_in_.type(), cli_in_.sub_type()) ==
            commonproto::RANKING_INFO_FAMILY) {

        cli_out_.Clear();
        for(int i = 0; i < rank_info_out_.rank_info_size();i++) {
		    const commonproto::rank_player_info_t &rank_info = rank_info_out_.rank_info(i);
            commonproto::rank_player_detail_data_t *player_data = cli_out_.add_rank_info_list();
            //player_data->mutable_player_base_info()->CopyFrom(player_info.base_info());
            //player_data->mutable_equip_list()->CopyFrom(player_info.equip_list());
            player_data->set_rank(rank_info.rank());
            player_data->set_score(rank_info.score());
        }

        cli_out_.set_type(cli_in_.type());
        cli_out_.set_sub_type(cli_in_.sub_type());
        return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    } 

    get_rank_info_session_t * session = (get_rank_info_session_t*)player->session;
    cacheproto::cs_batch_get_users_info  cache_in_;

    // 缓存取到的排名信息
    for(int i = 0; i < rank_info_out_.rank_info_size();i++) {
		const commonproto::rank_player_info_t &rank_info = rank_info_out_.rank_info(i);
        session->rank_info[i].user_id = rank_info.userid();
        session->rank_info[i].rank = rank_info.rank();
        session->rank_info[i].score = rank_info.score();

        //cache_in_.add_uids(rank_info.userid());
		commonproto::role_info_t* pb_ptr = cache_in_.add_roles();
		pb_ptr->set_userid(rank_info.userid());
		pb_ptr->set_u_create_tm(0);
    }

    // 拉取玩家base_info和装备
    return g_dbproxy->send_msg(
			player, player->userid, player->create_tm,
			cache_cmd_ol_req_users_info, 
			cache_in_);
}

int GetRankInfoCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
		player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(cache_out_);
	cli_out_.Clear();
	get_rank_info_session_t * session = (get_rank_info_session_t*)player->session;
	for (int i = 0; i < cache_out_.user_infos_size(); ++ i) {
		const commonproto::battle_player_data_t &player_info = cache_out_.user_infos(i);
		uint32_t userid = player_info.base_info().user_id();
		uint32_t rank = 0;
		uint64_t score = 0;
		int user_count = session->total_user_count;
		for (int j = 0; j < user_count; ++j) {
			if (session->rank_info[j].user_id != userid) {
				continue;
			} else {
				rank = session->rank_info[j].rank;
				score = session->rank_info[j].score;
				break;
			}
		}
		commonproto::rank_player_detail_data_t *player_data = cli_out_.add_rank_info_list();
		player_data->mutable_player_base_info()->CopyFrom(player_info.base_info());
		player_data->mutable_equip_list()->CopyFrom(player_info.equip_list());
		player_data->set_rank(rank);
		player_data->set_score(score);
	}
    cli_out_.set_type(session->type);
    cli_out_.set_sub_type(session->sub_type);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int ViewBtlReportCmdProcessor::proc_pkg_from_client(player_t*player, const char *body, int bodylen)
{
	PARSE_MSG;
	std::string key = cli_in_.key();
	rankproto::cs_get_battle_report rank_in_;
	rank_in_.set_key(key);
	int ret = g_dbproxy->send_msg(
            player, player->userid, player->create_tm, 
            ranking_cmd_get_battle_report, rank_in_);
	if (ret) {
		return ret;
	}
	return 0;
}

int ViewBtlReportCmdProcessor::proc_pkg_from_serv(player_t* player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	cli_out_.Clear();
	cli_out_.mutable_user_btl_report()->ParseFromString(rank_out_.pkg());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int BuyChallengeTimesCmdProcessor::proc_pkg_from_client(player_t*player, const char *body, int bodylen)
{
	PARSE_MSG;
	const uint32_t ARGS = 3;
	const uint32_t BASE_VALUE = 10;
	uint32_t max_times = 0;
	max_times = AttrUtils::get_attr_max_limit(player, kDailyBuyArenaTimesLimit);
	uint32_t buy_times = GET_A(kDailyArenaBuyChallengeTimes);
	if (buy_times >= max_times) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_buy_arean_challenge_count_get_limit);
	}
	
	int consume_diamond = (1 + (buy_times + 1) / ARGS) * BASE_VALUE; 
	uint32_t product_id = 90001;
	const product_t *pd = g_product_mgr.find_product(product_id);
    if (!pd) {
        RET_ERR(cli_err_product_not_exist);
    }
	int ret =  player_chg_diamond_and_sync(player, -consume_diamond, pd, 1, 
            dbproto::CHANNEL_TYPE_BUY_REDUCE, "竞技场购买挑战次数");
	if (ret != 0) {
		RET_ERR(ret);
	}
	ADD_A(kDailyArenaBuyChallengeTimes, 1);
	cli_out_.Clear();
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int RankPraiseFunCmdProcessor::proc_pkg_from_client(
		player_t*player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t praise_count = GET_A(kDailyRankPraiseCount);
	if (praise_count >= (uint32_t)onlineproto::PLAYER_DAILY_PRAISE_OTHER) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_rank_praise_count_get_limit);
	}
	if (!is_valid_uid(cli_in_.uid())) {
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_uid_invalid);
	}
	AttrUtils::change_other_attr_value_pub(
			cli_in_.uid(), cli_in_.create_tm(),
			kAttrPraiseCount, 1, false);

	ADD_A(kAttrCurVp, PRAISE_ADD_HP_COUNT);
	uint32_t coins = PRAISE_ADD_COINS;
	//防沉迷
	if(check_player_addicted_threshold_none(player)){
		coins = 0;
	} else if (check_player_addicted_threshold_half(player)){
		coins /= 2;
	}
	AttrUtils::add_player_gold(player, coins, false, "排行榜点赞获得");
	ADD_A(kDailyRankPraiseCount, 1);

	cli_out_.Clear();
	cli_out_.set_uid(cli_in_.uid());
	cli_out_.set_create_tm(cli_in_.create_tm());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int PraiseCountExgCmdProcessor::proc_pkg_from_client(
	player_t *player, const char *body, int bodylen)
{
	uint32_t praise_count = GET_A(kAttrPraiseCount);
	uint32_t PLAYER_BE_PRAISED_COUNT = onlineproto::PLAYER_BE_PRAISED_COUNT;
	if (praise_count < PLAYER_BE_PRAISED_COUNT) {
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_player_be_praised_not_get_limit);
	}
	uint32_t EXCHG_COUNT_FOR_GOLD = onlineproto::EXCHG_COUNT_FOR_GOLD;
	//防沉迷
	if(check_player_addicted_threshold_none(player)){
		 EXCHG_COUNT_FOR_GOLD= 0;
	} else if (check_player_addicted_threshold_half(player)){
		 EXCHG_COUNT_FOR_GOLD/= 2;
	}
    AttrUtils::add_player_gold(player, EXCHG_COUNT_FOR_GOLD, false, "排行榜点赞兑换");
	SET_A(kAttrPraiseCount, 0);
	cli_out_.Clear();
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

//Corfirm: 此协议函数处理的是竞技场日排名奖励
int FakeCmdForArenaDailyRankCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
    ArenaUtils::proc_arena_rank_prize(player, rank_out_);
	return 0;
}

int FakeCmdForOpenSrvRankCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{

	uint32_t reward_flag = GET_A(kAttrOpenServRankingFlag);
	// //领过奖励了
	// if (taomee::test_bit_on(reward_flag, 1)){
		// return 0;
	// }	

	PARSE_SVR_MSG(user_rank_out_);
	const commonproto::rank_player_info_t &player_info = user_rank_out_.rank_info(0);
	//key
	std::string str_rank_key = player_info.rank_key();
	std::vector<std::string> keys_vec = split(str_rank_key, ':');
	uint32_t key = 0, sub_key = 0;
	if (keys_vec.size() >= 2) {
		key = boost::lexical_cast<uint32_t>(keys_vec[0]);
		sub_key = boost::lexical_cast<uint32_t>(keys_vec[1]);
	}

	if (player->userid == player_info.userid() &&
			player->create_tm == player_info.u_create_tm()) {
		uint32_t rank = player_info.rank();
		uint32_t prize_id = 0;
		if(key == commonproto::RANKING_TL_DIAMOND_RECHARGE){
			if(rank == 1){
				prize_id = 11610;
			} else if (rank >= 2 && rank <= 3){
				prize_id = 11611;
			} else if (rank >= 4 && rank <= 6){
				prize_id = 11612;
			} else if (rank >= 7 && rank <= 10){
				prize_id = 11613;
			}
			if(prize_id != 0 ){
				std::string content = "恭喜您在钻石大富豪活动中获得了第" + boost::lexical_cast<std::string>(rank) +
					"名，请领取奖励！";
				PlayerUtils::generate_new_mail(player, "获得钻石大富豪活动奖励", content,
						prize_id);
				reward_flag = taomee::set_bit_on(reward_flag, 1);
				SET_A(kAttrOpenServRankingFlag, reward_flag );
			}

		} else if (key == commonproto::RANKING_TL_TOTAL_POWER){
			if(rank == 1){
				prize_id = 11800;
			} else if (rank >= 2 && rank <= 3){
				prize_id = 11801;
			} else if (rank >= 4 && rank <= 6){
				prize_id = 11802;
			} else if (rank >= 7 && rank <= 10){
				prize_id = 11803;
			}
			if(prize_id != 0 ){
				std::string content = "恭喜您在全民战力天梯活动中获得了第" + boost::lexical_cast<std::string>(rank) +
					"名，请领取奖励！";
				PlayerUtils::generate_new_mail(player, "获得全民战力天梯活动奖励", content,
						prize_id);
				reward_flag = taomee::set_bit_on(reward_flag, 2);
				SET_A(kAttrOpenServRankingFlag, reward_flag);
			}
		} else if (key == commonproto::RANKING_TL_GOLD_CONSUME){
			if(rank == 1){
				prize_id = 11810;
			} else if (rank >= 2 && rank <= 3){
				prize_id = 11811;
			} else if (rank >= 4 && rank <= 6){
				prize_id = 11812;
			} else if (rank >= 7 && rank <= 10){
				prize_id = 11813;
			}
			if(prize_id != 0 ){
				std::string content = "恭喜您在金币大富豪活动中获得了第" + boost::lexical_cast<std::string>(rank) +
					"名，请领取奖励！";
				PlayerUtils::generate_new_mail(player,
					   	"获得金币大富豪活动奖励", content,
						prize_id);
				reward_flag = taomee::set_bit_on(reward_flag, 3);
				SET_A(kAttrOpenServRankingFlag, reward_flag);
			}
		}
		return 0;
	} else {
		ERROR_TLOG("get diamond recharge rank from serv err!"
				"uid=[%u] create_tm=[%u]", 
				player->userid, player->create_tm);
	}
	return 0;
}
