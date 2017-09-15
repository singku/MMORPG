#include "escort_processor.h"
#include "macro_utils.h"
#include "escort.h"
#include "escort_utils.h"
#include "service.h"
#include "global_data.h"
#include "player_manager.h"
#include "rank_utils.h"
#include "data_proto_utils.h"
#include "task_utils.h"
#include "arena.h"
#include "prize.h"
#include "pet_utils.h"

int EscortRefreshShipCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	//奖励未领不能刷新飞船
	if (GET_A(kAttrEscortReward)) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_start_escort_must_get_last_reward);
	}
	//正在运宝时，不能刷新飞船（前端没有接口，这里是为了防外挂扰乱后台数据）
	if (GET_A(kAttrEscortLastStarttime)) {
		return send_err_to_player(player, player->cli_wait_cmd, 
				cli_err_can_not_refresh_ship_when_escort);
	}
	if (GET_A(kDailyEscortTimes) >= GET_A(kDailyEscortBuyTimes) + DAILY_FREE_ESCORE_TIMES) {
		ERROR_TLOG("escort daily times get limit: uid=[%u], has_use_times=[%u],buy_times=[%u]",
			player->userid, GET_A(kDailyEscortTimes), GET_A(kDailyEscortBuyTimes));
		return send_err_to_player(player, player->cli_wait_cmd, 
				cli_err_refresh_must_buy_escort_times);
	}
	//const uint32_t CONSUME_DIAMOND = 3;
	uint32_t product_id = 0;
	uint32_t refresh_times = GET_A(kAttrEscortRefreshShip);
	if (GET_A(kAttrEscortBoatType) == (uint32_t)onlineproto::AIRSHIP_ROOKIE && refresh_times >= 2) {
		product_id = 90007;
	} else if (GET_A(kAttrEscortBoatType) == (uint32_t)onlineproto::AIRSHIP_JUNIOR && refresh_times >= 2) {
		product_id = 90034;
	}
	if (product_id) {
		const uint32_t cnt = 1;
		uint32_t ret = buy_attr_and_use(player, kServiceBuyEscortRefreshBoatType, product_id, cnt);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
	}
	ADD_A(kAttrEscortRefreshShip, 1);
	if (GET_A(kAttrEscortBoatType) == (uint32_t)onlineproto::AIRSHIP_JUNIOR) {
		ADD_A(kAttrEscortRefreshSeniorShip, 1);
		if (GET_A(kAttrEscortRefreshSeniorShip) >= 4) {
			SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_SENIOR);
			cli_out_.Clear();
			cli_out_.set_type((onlineproto::airship_type_t)GET_A(kAttrEscortBoatType));
			return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
		}
	}
	refresh_times = GET_A(kAttrEscortRefreshShip);
	//如果是第一次，必定是低级飞船
	uint32_t rate = taomee::ranged_random(1, 100);
	if (1 == refresh_times) {
		SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_ROOKIE);
	} else if (2 == refresh_times) {
		if (rate <= 20) {
			SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_JUNIOR);
		}
	} else if (3 == refresh_times) {
		if (GET_A(kAttrEscortBoatType) == (uint32_t)onlineproto::AIRSHIP_ROOKIE) {
			if (rate <= 50){
				SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_JUNIOR);
			}
		} else if (GET_A(kAttrEscortBoatType) == (uint32_t)onlineproto::AIRSHIP_JUNIOR) {
			if (rate <= 33) {
				SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_SENIOR);
			}
		}
	} else if (refresh_times == 4) {
		if (GET_A(kAttrEscortBoatType) == (uint32_t)onlineproto::AIRSHIP_ROOKIE) {
			SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_JUNIOR);
		} else if (GET_A(kAttrEscortBoatType) == (uint32_t)onlineproto::AIRSHIP_JUNIOR) {
			if (rate <= 33) {
				SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_SENIOR);
			}
		}
	} else if (refresh_times >= 5 && refresh_times <= 7) {
		if (rate <= 33) {
			SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_SENIOR);
		}
	} else {
		SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_SENIOR);
	}

	cli_out_.Clear();
	if (GET_A(kAttrEscortBoatType) == 0) {
		SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_SENIOR);
	}
	cli_out_.set_type((onlineproto::airship_type_t)GET_A(kAttrEscortBoatType));
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int EscortBuyShipCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	//奖励未领不能购买飞船
	if (GET_A(kAttrEscortReward)) {
		RET_ERR(cli_err_start_escort_must_get_last_reward);
	}
	//正在运宝时，不能购买飞船（前端没有接口，这里是为了防外挂扰乱后台数据）
	if (GET_A(kAttrEscortLastStarttime)) {
		RET_ERR(cli_err_can_not_refresh_ship_when_escort);
	}
	if (GET_A(kDailyEscortTimes) >= GET_A(kDailyEscortBuyTimes) + DAILY_FREE_ESCORE_TIMES) {
		ERROR_TLOG("escort daily times get limit: uid=[%u], has_use_times=[%u],buy_times=[%u]",
			player->userid, GET_A(kDailyEscortTimes), GET_A(kDailyEscortBuyTimes));
		RET_ERR(cli_err_refresh_must_buy_escort_times);
	}

	uint32_t product_id = 0;
	if (cli_in_.ship_type() == onlineproto::AIRSHIP_JUNIOR) {
		product_id = 90007;
	} else if (cli_in_.ship_type() == onlineproto::AIRSHIP_SENIOR) {
		product_id = 90034;
	}
	if (product_id) {
		uint32_t ret = buy_attr_and_use(player, kServiceBuyEscortRefreshBoatType, product_id, 1);
		if (ret) {
			RET_ERR(ret);
		}
	}
	SET_A(kAttrEscortOwnShipType, cli_in_.ship_type());
	cli_out_.Clear();
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int EscortGetOtherShipCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	cli_out_.Clear();


	EscortMgr::EscortMap tmp_escort_map;
	g_escort_mgr.get_escort_info(tmp_escort_map);
	uint64_t role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	escort_info_t* es_ptr = g_escort_mgr.get_user_escort_info(role_key);
	if (es_ptr && GET_A(kAttrEscortLastStarttime)) {
		FOREACH(tmp_escort_map, it) {
			if (es_ptr->route_id == it->second.route_id) {
				g_escort_mgr.pack_one_escort_info(cli_out_.add_player_info_list(), &(it->second));
			}
		}
	} else {
		std::set<route_info_t> route_s;
		g_escort_mgr.get_route_set(route_s);
		uint32_t opt_route_id = EscortUtils::get_optional_route_id(route_s);
		if (opt_route_id == 0) {
			opt_route_id = 1;
		}
		FOREACH(tmp_escort_map, it) {
			if (opt_route_id == it->second.route_id) {
				g_escort_mgr.pack_one_escort_info(cli_out_.add_player_info_list(), &(it->second));
			}
		}
	}

	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int EscortStartCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	cli_out_.Clear();
	//是否还在运宝
	if (GET_A(kAttrEscortLastStarttime)) {
		RET_ERR(cli_err_escort_on_the_way);
	}
	//多余判断（有了kAttrEscortLastStarttime,无须此判断，后续考虑去掉）
	if (GET_A(kAttrEscortReward)) {
		RET_ERR(cli_err_start_escort_must_get_last_reward);
	}
	//运宝时，检查玩家每日运宝上限
	if (GET_A(kDailyEscortTimes) >= GET_A(kDailyEscortBuyTimes) + DAILY_FREE_ESCORE_TIMES) {
		ERROR_TLOG("escort daily times get limit: uid=[%u],"
				"has_use_times=[%u],buy_times=[%u]",
			player->userid, GET_A(kDailyEscortTimes), GET_A(kDailyEscortBuyTimes));
		RET_ERR(cli_err_escort_daily_get_limit);
	}
	//检查刷新过飞船
	//如果玩家拥有想到运输的飞船,则优化消耗该拥有的飞船，而不使用船票
	if (GET_A(kAttrEscortOwnShipType) == (uint32_t)cli_in_.type()) {
		SET_A(kAttrEscortOwnShipType, 0);
	} else {
		//如果想要运输的飞船，玩家目前没有，则使用船票
		uint32_t item_id = 0;
		if (cli_in_.type() == onlineproto::AIRSHIP_JUNIOR) {
			item_id = 37005;
		} else if (cli_in_.type() == onlineproto::AIRSHIP_SENIOR) {
			item_id = 37006;
		}
		if (item_id) {
			uint32_t ret = reduce_single_item(player, item_id, 1);
			if (ret) {
				RET_ERR(ret);
			}
		}
	}
	SET_A(kAttrEscortBoatType, cli_in_.type());
    // 更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_ESCORT, 1);

	g_escort_mgr.add_player_to_escort(player);
	SET_A(kAttrEscortOnlineId, g_online_id);
	g_escort_mgr.sync_player_info_other(player->userid, player->create_tm, onlineproto::ES_SHIP_APPEAR);
	ADD_A(kDailyEscortTimes, 1);

	AttrUtils::add_attr_in_special_time_range(player, 
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,		
			kAttrActivEscortCnt);

	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int EscortRobberyCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint64_t suf_role_key = ROLE_KEY(ROLE(cli_in_.suffer_uid(), cli_in_.suf_create_tm()));
	//判断米米号是否合法
	uint32_t atk_tick = cli_in_.atk_tick();
	uint32_t def_tick = cli_in_.def_tick();
	escort_robbery_session_t* session = (escort_robbery_session_t*)player->session;
	memset(session, 0, sizeof(*session));
	session->atk_tick = atk_tick;
	session->def_tick = def_tick;
	player->temp_info.escort_def_id = suf_role_key;

	uint32_t ret = g_escort_mgr.check_robbery_condition(
			player, cli_in_.suffer_uid(), cli_in_.suf_create_tm());
	if (ret) {
		ERROR_TLOG("uid=[%u]robbery condition not get", player->userid);
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	ret = g_escort_mgr.deal_with_robbery(player, cli_in_.suffer_uid(), cli_in_.suf_create_tm());
	if (ret) {
		ERROR_TLOG("uid=[%u]deal with robbery err:suffer_uid=[%u]", 
				player->userid, cli_in_.suffer_uid());
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	g_escort_mgr.insert_player_uid_to_uid_set(player->userid, player->create_tm);

	escort_info_t* suf_ptr = g_escort_mgr.get_user_escort_info(suf_role_key);
	if (suf_ptr == NULL) {
		RET_ERR(cli_err_will_rob_but_suf_id_not_ext);
	}
	suf_ptr->in_pk_flag += 1;
	
	//广播被打劫的玩家的状态信息
	g_escort_mgr.sync_player_info_other(cli_in_.suffer_uid(), cli_in_.suf_create_tm(), onlineproto::ES_SHIP_UPDATE);
	//判断def_uid是否同服在线；若是则无需到缓存拉信息
	//后期用打包好的函数 取代上面的代码
	player_t *ai_player_ptr = g_player_manager->get_player_by_userid(cli_in_.suffer_uid());
	if (ai_player_ptr) {
		cli_out_.Clear();
		DataProtoUtils::pack_pk_players_btl_info_include_tick(
				player, ai_player_ptr,
				atk_tick, def_tick,
				cli_out_.mutable_btl_data(),
				commonproto::GROUND_ESCORT);
		return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
	}

	cacheproto::cs_batch_get_users_info cache_info_in_;
	commonproto::role_info_t* pb_ptr = cache_info_in_.add_roles();
	pb_ptr->set_userid(cli_in_.suffer_uid());
	pb_ptr->set_u_create_tm(cli_in_.suf_create_tm());

	g_dbproxy->send_msg(
			player, player->userid, player->create_tm,
			cache_cmd_ol_req_users_info, cache_info_in_);
	return 0;	
}

int EscortRobberyCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	switch (player->serv_cmd) {
		case cache_cmd_ol_req_users_info:
			return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
		default:
			return 0;
	}
	return 0;
}

int EscortRobberyCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
		player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(cache_info_out_);

	uint32_t user_infos_size = cache_info_out_.user_infos_size();
	if (user_infos_size == 0) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_pk_data_info_not_found);
	}

	escort_robbery_session_t* session = (escort_robbery_session_t*)player->session;

	cli_out_.Clear();
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

//打劫的结果
int EscortRobberyResultCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	//打劫者为主动方
	commonproto::challenge_result_type_t  pk_result = cli_in_.result();
	if (pk_result != commonproto::WIN) {
		pk_result = commonproto::LOSE;
	}
	EscortUtils::deal_when_rob_result_come_out(player, pk_result);
	//找出出战的精灵
	//PetUtils::sync_fight_pet_info_to_client(player);
	cli_out_.Clear();
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int EscortGetRewardCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	if (!GET_A(kAttrEscortReward)) {
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_reward_mush_after_escort_finish);
	}
	uint32_t total_money = GET_A(kAttrEscortEarning);
	//防沉迷
	if(check_player_addicted_threshold_none(player)){
		total_money = 0;
	} else if (check_player_addicted_threshold_half(player)){
		 total_money /= 2;
	}
    AttrUtils::add_player_gold(player, total_money, false, "运宝结算");
	cli_out_.Clear();
	//领取完奖励后，清除一批属性
	AttrUtils::ranged_clear(player, kAttrRob1Id, kAttrDefRob3Earning, false);

	uint32_t ship_type = GET_A(kAttrEscortBoatType);
	uint32_t prize_ids[] = {4001, 4002, 4003};
	onlineproto::sc_0x0112_notify_get_prize noti_prize_msg;
	if (ship_type >= 1 && ship_type <= 3) {
		transaction_proc_prize(player, prize_ids[ship_type - 1],
				noti_prize_msg,
				commonproto::PRIZE_REASON_ESCORT_RESULT_REWARD,
				onlineproto::SYNC_REASON_PRIZE_ITEM);
		google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> *pb_prize_list_ptr;
		pb_prize_list_ptr = noti_prize_msg.mutable_award_elems();
		commonproto::prize_elem_t* pb_prize_ptr = pb_prize_list_ptr->Add();
		pb_prize_ptr->set_type(commonproto::PRIZE_ELEM_TYPE_ATTR);
		pb_prize_ptr->set_id(kAttrGold);
		pb_prize_ptr->set_count(total_money);
		send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_prize_msg);
	}
	
	SET_A(kAttrEscortEarning, 0);
	SET_A(kAttrEscortReward, 0);
	SET_A(kAttrEscortBoatType, 0);
//	 奇怪的问题， GET_A(i)为0； 但实际上数据库中相应的字段不为0
//   原因是内存中数据存放在运宝管理器的EscortMgr::escort_map,   rob_info_map中
//	 所以：当GET_A(i)时，从内存player->attr中find找不到
//   替代方案：其实这里也无需用if(GET_A(i))来判断，用 ranged_clear代替即可
	/*
	for (uint32_t i = (uint32_t)kAttrRob1Id; i <= (uint32_t)kAttrRob6Earning; ++i) {
		if (GET_A(i)) {
			SET_A(i, 0);
		}
	}
	*/
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
