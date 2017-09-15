#include "player.h"
#include "prize.h"
#include "prize_processor.h"
#include "arena.h"
#include "arena_conf.h"
#include "global_data.h"
#include "rank_utils.h"
#include "task_utils.h"
#include "time_utils.h"
#include "package.h"
#include "global_attr.h"
#include "common.h"
#include "exped.h"
#include "exped_conf.h"
#include "rune_utils.h"
#include "player_utils.h"
#include "sys_ctrl.h"
#include <boost/lexical_cast.hpp>

void RequirePrizeCmdProcessor::register_require_reason()
{
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_TEST] = proc_test_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_ARENA_GIFT] = proc_arena_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_ONLINE_REWARD] = proc_online_reward_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_ALCHEMY] = proc_alchemy_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_SIGN_IN_REWARD] = proc_sign_in_prize;
	prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_MONTH_SIGN_REWARD] = proc_month_sign_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_SEVEN_DAYS_REWARD] = proc_seven_days_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_WEI_XIN_REWARD] = proc_wei_xin_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_MONTH_CARD_REWARD] = proc_month_card_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_NORMAL_ONE_TIME_LOTTERY] = proc_normal_one_time_lottery_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_NORMAL_ONE_TIME_LOTTERY] = proc_normal_one_time_lottery_prize;
	prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_NORMAL_TEN_TIMES_LOTTERY] = proc_normal_ten_times_lottery_prize;
	prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_DIAMOND_ONE_TIME_LOTTERY] = proc_diamond_one_time_lottery_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_BUY_HP] = proc_buy_vp_prize;

	prize_srv_map_[(uint32_t)commonproto::PRIZE_REASON_NIGHT_RAID_REWARD] = proc_night_raid_prize_from_svr;

	prize_srv_map_[(uint32_t)commonproto::PRIZE_REASON_ARENA_GIFT] = proc_arena_prize_from_svr;
	prize_srv_map_[(uint32_t)commonproto::PRIZE_REASON_DIAMOND_TEN_TIMES_LOTTERY] = proc_diamond_ten_times_lottery_prize_form_cli;

	prize_srv_map_[(uint32_t)commonproto::PRIZE_REASON_GET_EXPED_REWARD] = proc_exped_prize_from_svr;
	//prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_GET_EXPED_REWARD] = proc_exped_prize;

	prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_CHANGE_CLOTHES_PLAN] = proc_change_clothes_plan;
	prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_FAVORITE_CT_REWARD] = proc_favorite_ct_reward;
	prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_GET_YEAR_VIP_REWARD] = proc_get_year_vip_reward;
	prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_BATTLE_VALUE_GIFT] = proc_battle_value_gift;
	prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_MARLY_SHOT_GIFT] = proc_marly_shot_gift;
	prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_MARY_QUESTION_PRIZE] = proc_marly_question_gift;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_SIGN_INVITE_CODE_PRIZE] = proc_sign_invite_code_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_INVITE_N_PLAYERS_PRIZE] = proc_invited_players_reach_n_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_SHARE_PRIZE] = proc_share_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_GET_FIRST_RECHARGE] = proc_get_first_recharge_gift;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_CONSUME_DIAMOND] = proc_consume_diamond_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_EVIL_KNIFE_LEGEND] = proc_evil_knife_legend;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_VIP_USER_GET_YAYA_FRAGMENT] = proc_vip_get_yaya_fragment;
    //prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_MAYIN_BUCKET_PRIZE] = proc_mayin_bucket_prize;
    
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_SURVEY] = proc_survey_prize;
    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_CHARGE_DIAMOND_GET_GIFT] = proc_charge_diamond_get_gift_prize;

    prize_func_map_[(uint32_t)commonproto::PRIZE_REASON_SUMMER_WEEKLY_SIGNED] = proc_summer_weekly_signed;
}

bool RequirePrizeCmdProcessor::check_lottery(
            player_t *player, onlineproto::cs_0x0113_require_prize cli_in)
{
    if (!(cli_in.reason() == commonproto::PRIZE_REASON_NORMAL_ONE_TIME_LOTTERY ||
            cli_in.reason() == commonproto::PRIZE_REASON_NORMAL_TEN_TIMES_LOTTERY ||
            cli_in.reason() == commonproto::PRIZE_REASON_DIAMOND_ONE_TIME_LOTTERY ||
            cli_in.reason() == commonproto::PRIZE_REASON_DIAMOND_TEN_TIMES_LOTTERY)) {
        return true;
    } else {
        // 更新悬赏任务
        TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_RECRUIT, 1);
    }

    // 抽卡玩法必须先完成新手任务
    bool flag = false;
    if (TaskUtils::is_task_accept(player, 51013)) {
        flag = true;
    }
    return flag;
}

int RequirePrizeCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    if (!check_lottery(player, cli_in_)) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_cannot_lottery);
    }

	if (prize_srv_map_.count((uint32_t)cli_in_.reason())) {
		prize_proc_func_need_get_info_from_serv  funsvr;
		required_prize_session_t*  session = (required_prize_session_t*)player->session;
		memset(session, 0, sizeof(*session));
		session->prize_id = cli_in_.prize_id();
		session->reason = cli_in_.reason();
		funsvr = (prize_srv_map_.find((uint32_t)cli_in_.reason()))->second;
		int ret = funsvr(player);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
		
		return 0;
	}


    player->temp_info.req_prize_id = cli_in_.prize_id();
    int ret = 0;
    noti_.Clear();
    if (prize_func_map_.count((uint32_t)cli_in_.reason()) == 0) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_prize_id_not_exist);
    }
    prize_proc_func_t fun;
    fun = (prize_func_map_.find((uint32_t)cli_in_.reason()))->second;
    ret = fun(player, cli_in_.prize_id(), noti_);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    cli_out_.Clear();
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_);
    return 0;
}

int RequirePrizeCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
	switch (player->serv_cmd) {
		case ranking_cmd_get_users_rank:
			return get_player_arena_week_rank(
				player, body, bodylen);
		case db_cmd_update_global_attr:
			return proc_diamond_ten_times_lottery_prize_form_svr(
				player, body, bodylen);
		case db_cmd_user_raw_data_get:
			return proc_raw_data_from_svr(
					player, body, bodylen);
	default:
		return 0;
	}
    //TODO
    return 0;
}

int RequirePrizeCmdProcessor::get_player_arena_week_rank(
		player_t* player, const char* body, int bodylen)
{
	/*
	rankproto::sc_get_user_rank  user_rank_out;
	PARSE_SVR_MSG(user_rank_out);
	uint32_t week_rank = user_rank_out.rank_info(0).week_rank();
	player->temp_info.arena_week_rank = week_rank;
    int ret = 0;
    noti_.Clear();
	required_prize_session_t* session = (required_prize_session_t*)player->session;
    if (prize_func_map_.count(session->reason) == 0) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_prize_id_not_exist);
    }
    prize_proc_func_t fun;
    fun = (prize_func_map_.find(session->reason))->second;
    ret = fun(player, session->prize_id, noti_);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }
    cli_out_.Clear();
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_);
	*/
    return 0;
}

struct magic_word_use_req_t
{
    char magic_word[32];
    uint32_t userid;
    uint32_t userip;
    uint16_t gift_count;
}__attribute__((packed)); 

struct magic_word_gift_t
{
    uint32_t gift_id;
    uint32_t gift_count;
}__attribute__((packed)); 

struct magic_word_use_rsp_t
{
    uint32_t userid;
    uint8_t gift_right;
    uint16_t gift_count;
}__attribute__((packed)); 

struct magic_word_query_req_t
{
    char magic_word[32];
}__attribute__((packed));

struct magic_word_query_rsp_t
{
    uint16_t can_choose_count;
    uint16_t used_count;
    uint16_t has_gift_count;
}__attribute__((packed));

int UseMagicWordCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    magic_word_use_req_t req;
    memset(&req, 0, sizeof(req));
    STRCPY_SAFE(req.magic_word, cli_in_.gift_code().c_str());
    req.userid = player->userid;
    req.userip = get_cli_ip(player->fdsess);
    req.gift_count = 0;

    return g_dbproxy->send_to_act(player, player->userid, 2502,
            (const char*)&req, sizeof(req));
}

int UseMagicWordCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    magic_word_use_rsp_t *rsp = (magic_word_use_rsp_t*)body;
    if (rsp->gift_right) {
        RET_ERR(cli_err_magic_word_arg_err);
    }
    for (int i = 0; i < rsp->gift_count; i++) {
        magic_word_gift_t *gift = (magic_word_gift_t*)(body + 
                sizeof(magic_word_use_rsp_t) + i*sizeof(magic_word_gift_t));
        DEBUG_TLOG("U:%u MagicWordGift:%u", player->userid, gift->gift_id);
        onlineproto::sc_0x0112_notify_get_prize noti_msg;
        int ret = transaction_proc_prize(player, gift->gift_id, noti_msg, 
                commonproto::PRIZE_REASON_GIFT_CODE,onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
        if (ret) {
            RET_ERR(ret);
        }
        send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_msg);
    }
    RET_MSG;
}

int UseGiftCodeCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    db_in_.Clear();
    db_in_.set_code(cli_in_.gift_code());
    db_in_.set_svr_id(g_server_id);
    return g_dbproxy->send_msg(player, player->userid, 
            player->create_tm, db_cmd_use_gift_code, db_in_);
}

int UseGiftCodeCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);
    cli_out_.Clear();
    int ret;
    switch (db_out_.status()) {
    case 0:
        ret = 0;
        break;
    case 1:
        ret = cli_err_gift_code_used;
        break;
    case 2:
        ret = cli_err_gift_code_not_exist;
        break;
    case 3:
        ret = cli_err_gift_code_deleted;
        break;
    case 4:
        ret = cli_err_gift_code_expired;
        break;
    }
    if (ret) {
        RET_ERR(ret);
    }
    uint32_t prize_id = db_out_.prize_id();
    onlineproto::sc_0x0112_notify_get_prize noti_msg;
    ret = transaction_proc_prize(player, prize_id, noti_msg, 
            commonproto::PRIZE_REASON_GIFT_CODE, onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
    if (ret) {
        RET_ERR(ret);
    }
    send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_msg);
    RET_MSG;
}

int GetPrizeBulletinCmdProcessor::proc_pkg_from_client(player_t *player,
	   	const char *body, int bodylen) 
{
    PARSE_MSG;
	uint32_t type = boost::lexical_cast<uint32_t>(cli_in_.reason());
	player->temp_info.prize_reason = type;
	int32_t start = cli_in_.start();
	int32_t end = cli_in_.end();
	RankUtils::get_bulletin_list_info(player, type, 5, start, end);
	return 0;
}

int GetPrizeBulletinCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
	switch (player->serv_cmd) {
		case ranking_cmd_list_get_range_member:
			return proc_get_members_from_ranking(
				player, body, bodylen);
	default:
		return 0;
	}
    return 0;
}

int GetPrizeBulletinCmdProcessor::proc_get_members_from_ranking(
		player_t *player, const char* body, int bodylen)
{
	rankproto::sc_list_get_range_member rank_out_;	
	rank_out_.Clear();
	PARSE_SVR_MSG(rank_out_);
	cli_out_.Clear();
	commonproto::prize_reason_t reason = (commonproto::prize_reason_t)player->temp_info.prize_reason;
	cli_out_.set_reason(reason);
	for(int32_t i =0 ; i < rank_out_.value_size(); i++){
		cli_out_.add_elem()->ParseFromString(rank_out_.value(i));
	}
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

void ResourceRetrieveCmdProcessor::register_resource_type()
{
	retrieve_func_map_[(uint32_t)commonproto::RESOURCE_SWIM] = proc_swim_resource_retrieve;
	retrieve_func_map_[(uint32_t)commonproto::RESOURCE_ESCORT] = proc_escort_resource_retrieve;
	retrieve_func_map_[(uint32_t)commonproto::RESOURCE_MON_CRISIS] = proc_mon_crisis_resource_retrieve;
}

void ResourceRetrieveCmdProcessor::get_user_retrieve_value_map(player_t *player)
{
	value_map_[(uint32_t)commonproto::RESOURCE_SWIM] = GET_A(kDailyRetrieveSwimResource);
	value_map_[(uint32_t)commonproto::RESOURCE_ESCORT] = GET_A(kDailyRetrieveEscortResource);
	value_map_[(uint32_t)commonproto::RESOURCE_MON_CRISIS] = GET_A(kDailyRetrieveMonCrisisResource);
}

void ResourceRetrieveCmdProcessor::clear_user_retrieve_value(player_t *player, commonproto::resource_type_t type)
{
	if(type == commonproto::RESOURCE_SWIM){
	  SET_A(kDailyRetrieveSwimResource, 0);
	} else if(type == commonproto::RESOURCE_ESCORT){
	  SET_A(kDailyRetrieveEscortResource, 0);
	} else if(type == commonproto::RESOURCE_MON_CRISIS){
	  SET_A(kDailyRetrieveMonCrisisResource, 0);
	}
}

int ResourceRetrieveCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;

	noti_.Clear();
	value_map_.clear();
	std::map<uint32_t, uint32_t>().swap(value_map_);
	noti_.set_reason(commonproto::PRIZE_REASON_RESOURCE_RETRIEVE);
	get_user_retrieve_value_map(player);

	int ret = 0;
	uint32_t cnt = 0;
	retrieve_proc_func_t fun;
	if(retrieve_func_map_.count((uint32_t)cli_in_.resource()) == 0 ||
			value_map_.count((uint32_t)cli_in_.resource()) == 0){
		//无法找回该资源
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_can_not_retrieve_this_resource);
	} 

	cnt = (value_map_.find((uint32_t)cli_in_.resource()))->second;
	fun = (retrieve_func_map_.find((uint32_t)cli_in_.resource()))->second;
	ret = fun(player, cnt, cli_in_.type(), noti_);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	cli_out_.Clear();
	send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_);
	clear_user_retrieve_value(player, cli_in_.resource());
	send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

int ResourceRetrieveCmdProcessor::proc_pkg_from_serv(player_t *player, const char *body, int bodylen)
{
	return 0;
}

int proc_swim_resource_retrieve(player_t *player, uint32_t count,
	   	uint32_t type, onlineproto::sc_0x0112_notify_get_prize &noti)
{
	if(0 == count){
		return cli_err_can_not_retrieve_this_resource;
	}
	int ret = 0;
	uint32_t need_cnt = 0;
	uint32_t level = GET_A(kAttrLv);
	uint32_t add_exp = level * 3060;
	uint32_t item_id = 1031002;
	if(1 == type){
		//扣金币
		add_exp *= 0.5;
		need_cnt = level * 100;
		ret = AttrUtils::sub_player_gold(player, need_cnt, "资源找回之游泳找回");
		if (ret) {
			return ret;
		}
	} else if (2 == type){
		//扣钻石
        attr_type_t attr_type = kServiceBuySwimRetrieve;	
        const uint32_t product_id = 90040;	
		need_cnt = 1;
        ret = buy_attr_and_use(player, attr_type, product_id, need_cnt);
		if (ret) {
			return ret;
		}
	}
	//玩家获得的经验
    uint32_t real_player_exp = 0;
    PlayerUtils::add_player_exp(player, add_exp, &real_player_exp);
	google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> *pb_prize_list_ptr;
	pb_prize_list_ptr = noti.mutable_award_elems();
	commonproto::prize_elem_t* pb_prize_ptr = pb_prize_list_ptr->Add();
	pb_prize_ptr->set_type(commonproto::PRIZE_ELEM_TYPE_ITEM);
	pb_prize_ptr->set_id(item_id);
	pb_prize_ptr->set_count(add_exp);
	return 0;
}

int proc_escort_resource_retrieve(player_t *player, uint32_t count,
	   	uint32_t type, onlineproto::sc_0x0112_notify_get_prize &noti)
{
	if(0 == count){
		return cli_err_can_not_retrieve_this_resource;
	}

	int ret = 0;
	uint32_t need_cnt = 0;
	//钻石购买
	bool USE_DIAMOND = false;
	if(1 == type){
		//扣金币
		need_cnt = count * 1000;
		ret = AttrUtils::sub_player_gold(player, need_cnt, "资源找回之运宝找回");
		if (ret) {
			return ret;
		}
	} else if (2 == type){
		//扣钻石
        attr_type_t attr_type = kServiceBuyEscortRetrieve;	
        const uint32_t product_id = 90041;	
		need_cnt = count;
        ret = buy_attr_and_use(player, attr_type, product_id, need_cnt);
		if (ret) {
			return ret;
		}
		USE_DIAMOND = true;
	}
	//加金币
	// uint32_t gold_cnt = count * 600;
	// AttrUtils::add_player_gold(player, gold_cnt, false, "资源找回之运宝找回");
	// google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> *pb_prize_list_ptr;
	// pb_prize_list_ptr = noti.mutable_award_elems();
	// commonproto::prize_elem_t* pb_prize_ptr = pb_prize_list_ptr->Add();
	// pb_prize_ptr->set_type(commonproto::PRIZE_ELEM_TYPE_ATTR);
	// pb_prize_ptr->set_id(kAttrGold);
	// pb_prize_ptr->set_count(gold_cnt);
	//加prize
	std::vector<cache_prize_elem_t> result;
	std::vector<cache_prize_elem_t> tmp;
	uint32_t prize_id = USE_DIAMOND ? 4002 : 4001;
	while(count){
		tmp.clear();
		transaction_pack_prize(player, prize_id, tmp);
		result.insert(result.end(), tmp.begin(), tmp.end());
		count --;
	}
	merge_cache_prize(result);
    if (result.size()) {
        transaction_proc_packed_prize(player, result, &noti, 
                commonproto::PRIZE_REASON_NO_REASON, "资源找回之找回运宝奖励");
    }
	return 0;
}

int proc_mon_crisis_resource_retrieve(player_t *player, uint32_t count,
		uint32_t type, onlineproto::sc_0x0112_notify_get_prize &noti)
{
	//最高通关关卡dupid
	uint32_t unlock_dupid = GET_A(kAttrDupHighestUnlock1Id);
	uint32_t init_dupid = 701;
	if(unlock_dupid < init_dupid || 0 == count){
		return cli_err_can_not_retrieve_this_resource;
	}
	//当前阶段dup
	// uint32_t cur_dupid = GET_A(kAttrDupLowestLock1Id);
	//解锁的关卡数
	uint32_t unlock_id = (unlock_dupid - init_dupid + 1) >= 50 ? 50 : (unlock_dupid - init_dupid + 1);

	int ret = 0;
	uint32_t need_cnt = 0;
	// uint32_t base_prize[][2] = {
		// //金币，钻石
		// {50,   4},
		// {100,  6},
		// {150,  8},
		// {200, 10},
		// {250, 12}
	// };

	//计算消耗数量金币或者钻石
	if(unlock_id <= 10){
		need_cnt = (type == 1) ? unlock_id * 50 : 4; 
	} else if (unlock_id <= 20){
		need_cnt = (type == 1) ? (unlock_id % 10) * 100 + 500 : 10;
	} else if (unlock_id <= 30){
		need_cnt = (type == 1) ? (unlock_id % 20) * 150 + 1500 : 18;
	} else if (unlock_id <= 40){
		need_cnt = (type == 1) ? (unlock_id % 30) * 250 + 3000 : 28;
	} else if (unlock_id <= 50){
		need_cnt = (type == 1) ? (unlock_id % 40) * 500 + 5500 : 40;
	}
	//钻石购买
	bool USE_DIAMOND = false;
	if(1 == type){
		//扣金币
		// need_cnt = unlock_id * 100;
		ret = AttrUtils::sub_player_gold(player, need_cnt, "资源找回之怪物危机找回");
		if (ret) {
			return ret;
		}
	} else if (2 == type){
		//扣钻石
        attr_type_t attr_type = kServiceBuyMonCrisisRetrieve;	
        const uint32_t product_id = 90042;	
		// need_cnt = 1;
        ret = buy_attr_and_use(player, attr_type, product_id, need_cnt);
		if (ret) {
			return ret;
		}
		USE_DIAMOND = true;
	}
	//加prize
	uint32_t prize_id = 60001;
	uint32_t end_prize_id = 60050;
	std::vector<cache_prize_elem_t> result;
	std::vector<cache_prize_elem_t> tmp;
	for(uint32_t i = 0; i < unlock_id && prize_id <= end_prize_id; i ++){
		tmp.clear();
		transaction_pack_prize(player, prize_id, tmp);
		result.insert(result.end(), tmp.begin(), tmp.end());
		prize_id ++;
	}
	merge_cache_prize(result, count);
	//减半
	if(!USE_DIAMOND){
		half_cache_prize(result);
	}	
    if (result.size()){
        transaction_proc_packed_prize(player, result, &noti,
               commonproto::PRIZE_REASON_NO_REASON, "资源找回之怪物危机找回奖励");
    }

	return 0;
}



//----------业务处理函数-------//
int proc_test_prize(player_t *player, uint32_t prize_id, 
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
	/*
    if (prize_id != 1) {//奖励测试1
        return cli_err_prize_id_invalid;
    }
	*/
    //测试奖励没判断领奖条件 直接发奖
    int ret = transaction_proc_prize(player, prize_id, noti,
            commonproto::PRIZE_REASON_ARENA_GIFT,onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
    if (ret) {
        return ret;
    }
    return 0;
}

int proc_arena_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	uint32_t flag = GET_A(kWeeklyArenaBonusStatus);
	if (flag) {
		ERROR_TLOG("uid=[%u]:arena get prize time not come", player->userid);
		return cli_err_arena_prize_time_not_come;
	}
	rank_reward_t *rank_reward_ptr = g_arena_rank_reward_conf_mgr.get_arena_rank_reward(
			player->temp_info.arena_week_rank);
	if (rank_reward_ptr == NULL) {
		ERROR_TLOG("week_rank not found inarenas.xml week_rank=[%u]", 
				player->temp_info.arena_week_rank);
		return cli_err_rank_not_fount_in_table;
	}
	prize_id = rank_reward_ptr->bonus_id;
	
    int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_ARENA_GIFT,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
    if (ret) {
        return ret;
    }
	SET_A(kWeeklyArenaBonusStatus, 1);
	player->temp_info.arena_week_rank = 0;
    return 0;
}

int proc_arena_prize_from_svr(player_t* player)
{
	/*
	std::vector<uint32_t> user_ids;
	user_ids.push_back(player->userid);
	std::vector<commonproto::role_info_t> roles;
    commonproto::role_info_t role;
    role.set_userid(player->userid);
    role.set_u_create_tm(player->create_tm);
    roles.push_back(role);
	uint32_t week_sub_key;
	ArenaUtils::get_last_week_arena_rank_sub_key(week_sub_key);
	return RankUtils::get_user_rank_info(
		player, commonproto::RANKING_ARENA,
		week_sub_key, roles, commonproto::RANKING_ORDER_ASC);
	*/
	return 0;
}

int proc_online_reward_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
	//防沉迷设置
	if( check_player_addicted_threshold_none(player)){
		ERROR_TLOG("%u exceed online time threshold time, can't get online_reward", player->userid);
		return cli_err_addicted_time_threshold_max;
	}	
    static uint32_t def_prize_id[2][6] = {
        {8600,8601,8602,8603,8604,8605},
        {8650,8651,8652,8653,8654,8655}
    };
    static uint32_t prize_interval[6] = {
        0, 5*60, 10*60, 15*60, 25*60, 35*60
        //0, 30, 30, 30, 30, 30
    };

    uint32_t req_prize_id = prize_id;
    uint32_t com_idx = GET_A(kDailyOnlineRewardPrizeProgress);
    uint32_t vip_idx = GET_A(kDailyOnlineRewardVipPrizeProgress);
    bool req_vip_prize;
    if (req_prize_id >= def_prize_id[1][0] && req_prize_id <= def_prize_id[1][5]) {
        req_vip_prize = true;
        if (vip_idx > 5) {
            return cli_err_all_prize_get;
        }
        if (!is_vip(player)) {
            return cli_err_not_vip;
        }
    } else if (com_idx > 5) {
        return cli_err_all_prize_get;
    }

    uint32_t can_get_com_prize = com_idx + def_prize_id[0][0];
    uint32_t can_get_vip_prize = vip_idx + def_prize_id[1][0];

    uint32_t max_idx = com_idx >= vip_idx ?com_idx :vip_idx;
    uint32_t need_time = max_idx <= 5 ?prize_interval[max_idx] :0;

    uint32_t cur_idx;
    attr_type_t cur_type;
    uint32_t can_prize_id;
    if (req_vip_prize) {
        cur_idx = vip_idx;
        cur_type = kDailyOnlineRewardVipPrizeProgress;
        can_prize_id = can_get_vip_prize;
    } else {
        cur_idx = com_idx;
        cur_type = kDailyOnlineRewardPrizeProgress;
        can_prize_id = can_get_com_prize;
    }

    //后领取
    if (max_idx > cur_idx) {
        if (req_prize_id != can_prize_id) {
            return cli_err_prize_id_invalid;//按顺序领
        }
        int ret = transaction_proc_prize(player, req_prize_id, noti,
                commonproto::PRIZE_REASON_ONLINE_REWARD);
        if (ret) {
            return ret;
        }
        ADD_A(cur_type, 1);
        return 0;
    }

    //先领取则需判断时间是否到
    if (GET_A(kDailyOnlineRewardLastPrizeTm) == 0) {
        //跨天置0
        if (req_prize_id != can_prize_id) {
            return cli_err_prize_id_invalid;
        } 
    } else {
        int diff;
        // if (GET_A(kDailyOnlineRewardLastPrizeTm) < GET_A(kDailyLastLoginTime)) {
            // diff = NOW() - GET_A(kDailyLastLoginTime);
        // } else {
            // diff = NOW() - GET_A(kDailyOnlineRewardLastPrizeTm);
        // }
        // uint32_t past_time = GET_A(kDailyOnlineRewardPrizePastTm);
        // if (diff > 0) {
            // past_time += diff;
        // }
        // if (past_time < need_time) {
            // return cli_err_can_not_get_prize_yet;
        // }
		diff = NOW() - GET_A(kDailyLastLoginTime);
		uint32_t past_time = diff < 0 ? 0:diff;
		if (past_time < need_time) {
			return cli_err_can_not_get_prize_yet;
		}
    }

    int ret = transaction_proc_prize(player, req_prize_id, noti,
            commonproto::PRIZE_REASON_ONLINE_REWARD);
    if (ret) {
        return ret;
    }
    ADD_A(cur_type, 1);
    // SET_A(kDailyOnlineRewardPrizePastTm, 0);
    // SET_A(kDailyOnlineRewardLastPrizeTm, NOW());

    // 更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_ONLINE_REWARD, 1);

    return 0;
}

int proc_buy_vp_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
    uint32_t cnt = GET_A(kDailyVpDayBuyCount);
    uint32_t flag = GET_A(kDailyVpDayPrizeFlag);
    uint32_t base_prize_id = 8800;
    uint32_t idx = prize_id - base_prize_id;
    if (idx < 1 || idx > 3) {
        return cli_err_prize_id_invalid;
    }
    if (idx > cnt / 3) {
        return cli_err_can_not_get_prize_yet;
    }
    if (taomee::test_bit_on(flag, idx)) {
        return cli_err_prize_already_get;
    }
    int ret = transaction_proc_prize(player, prize_id, noti,
            commonproto::PRIZE_REASON_BUY_HP,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
    if (ret) {
        return ret;
    }
    flag = taomee::set_bit_on(flag, idx);
    SET_A(kDailyVpDayPrizeFlag, flag);
    return 0;
}

int proc_alchemy_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
    uint32_t cnt = GET_A(kDailyAlchemyTimes);
    uint32_t flag = GET_A(kDailyAlchemyPrizeFlag);
    uint32_t base_prize_id = 8800;
    uint32_t idx = prize_id - base_prize_id;
    if (idx < 1 || idx > 3) {
        return cli_err_prize_id_invalid;
    }
    if (idx > cnt / 3) {
        return cli_err_can_not_get_prize_yet;
    }
    if (taomee::test_bit_on(flag, idx)) {
        return cli_err_prize_already_get;
    }
    int ret = transaction_proc_prize(player, prize_id, noti,
            commonproto::PRIZE_REASON_ALCHEMY, onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
    if (ret) {
        return ret;
    }
    flag = taomee::set_bit_on(flag, idx);
    SET_A(kDailyAlchemyPrizeFlag, flag);
    return 0;
}
//  奖励Id必须连续,签到标志位type:5501
int proc_sign_in_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	//防沉迷设置
	if( check_player_addicted_threshold_none(player)){
		ERROR_TLOG("%u exceed online time threshold time, can't enter sign_in", player->userid);
		return cli_err_addicted_time_threshold_max;
	}	

	uint32_t data[][4] = {
		/* 奖励Id  物品Id 物品类型 物品数量 */
		{8701, 90016, kServiceBuySignIn1, 1},
		{8702, 90017, kServiceBuySignIn2, 1},
		{8703, 90018, kServiceBuySignIn3, 1},
		{8704, 90019, kServiceBuySignIn4, 1},
		{8705, 90020, kServiceBuySignIn5, 1},
		{8706, 90021, kServiceBuySignIn6, 1},
		{8707, 90022, kServiceBuySignIn7, 1},
	};

	if (g_server_config.version_department) {
		uint32_t prizeid_start = data[0][0];
		uint32_t idx = prize_id - prizeid_start;
		uint32_t flag = GET_A(kAttrSignInFlag); 
		uint32_t day_sign = idx + 1;               //签到第几天
		uint32_t ret = transaction_proc_prize(player, prize_id, noti,
				commonproto::PRIZE_REASON_SIGN_IN_REWARD);
		if (ret) {
			return ret;
		}
		flag = taomee::set_bit_on(flag, day_sign);
		SET_A(kAttrSignInFlag, flag);
		return 0;
	}

	/* 活动时间id, tid */
	uint32_t activ_time[] = {2, 1};
	/* 标志位key */
	attr_type_t key = kAttrSignInFlag;
	
	int ret = 0;
	uint32_t size = array_elem_num(data);    
	uint32_t prizeid_start = data[0][0];
	uint32_t prizeid_end   = data[size - 1][0];

	//检查prize_id是否合法
	if(prize_id > prizeid_end || prize_id < prizeid_start){ 
		ERROR_TLOG("prizeid invalid uid=[%u]:prizeid=[%u]", player->userid, prize_id); 
        return cli_err_prize_id_invalid; 
	} 

	uint32_t idx = prize_id - prizeid_start;
	uint32_t flag = GET_A(key); 
	uint32_t day_sign = idx + 1;               //签到第几天
	uint32_t time_id = activ_time[0];
	uint32_t time_tid = activ_time[1];
	uint32_t item_id = data[idx][1]; 
	uint32_t item_cnt = data[idx][3]; 
	attr_type_t item_type = attr_type_t(data[idx][2]); 

	//检测活动时间是否合法
	bool is_cur_time = false ;
	is_cur_time = TimeUtils::is_current_time_valid(time_id, time_tid);
	if (is_cur_time == false){
		return cli_err_activity_time_invalid;
	}

	//检测是否重复签到
	if (!taomee::test_bit_on(flag, day_sign)){
		uint32_t time_now = NOW();
		uint32_t time_start = TimeUtils::get_start_time(time_id, time_tid); //活动开始时间
		uint32_t diff_day = TimeUtils::get_days_between(time_start, time_now) + 1; //正常签到的天数
		if (day_sign <= diff_day){
			string stat;
			//VIP可以随意签到和补签,非vip补签消耗钻石
			if (!is_vip(player) && day_sign < diff_day){
				ret = buy_attr_and_use(player, item_type, item_id, item_cnt);
				if (ret) {
					return ret;
				}
			}
			if(day_sign == diff_day){
				switch(idx){
					case 0:
						stat = "已签到第一天";
						break;
					case 1:
						stat = "已签到第二天";
						break;
					case 2:
						stat = "已签到第三天";
						break;
					case 3:
						stat = "已签到第四天";
						break;
					case 4:
						stat = "已签到第五天";
						break;
					case 5:
						stat = "已签到第六天";
						break;
					case 6:
						stat = "已签到第七天";
						break;
					default:
						stat = "";
				}	
			} else {
				switch(idx){
					case 0:
						stat = "补签到第一天";
						break;
					case 1:
						stat = "补签到第二天";
						break;
					case 2:
						stat = "补签到第三天";
						break;
					case 3:
						stat = "补签到第四天";
						break;
					case 4:
						stat = "补签到第五天";
						break;
					case 5:
						stat = "补签到第六天";
						break;
					case 6:
						stat = "补签到第七天";
						break;
					default:
						stat = "";
				}	
			}

			Utils::write_msglog_new(player->userid, "奖励", "签到", stat);
			ret = transaction_proc_prize(player, prize_id, noti,
					commonproto::PRIZE_REASON_SIGN_IN_REWARD);
			if (ret) {
				return ret;
			}
			flag = taomee::set_bit_on(flag, day_sign);
			SET_A(key, flag);
			return 0;
		} else {
			return cli_err_sign_in_day_not_come;
		}	
	}else {
		return cli_err_repeat_sign_in;
	}
}
//普通单抽
int proc_normal_one_time_lottery_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
    //SET_A(kAttrRookieGuide7PetLottery, 1);

	/* 固定次数，prize_id */
	uint32_t fix_prize[][2] = {
		{1, 201},
		{2, 202},
		{3, 203}
	};
	/* prize_id ，概率 */
	uint32_t rand_prize[][2] = {
		{210, 50},
		{211, 35},
		{212, 15},
	};
	/* 抽奖次数，上次免费抽奖时间 */
	attr_type_t attr_value[] = {kAttrNormalLotteryTimes, kAttrFreeNormalLotteryLastTime};
	/* 消耗物品id，物品数量，免费抽奖间隔时间(小时) */
	uint32_t conf[] = {33000, 1, 8};

	uint32_t lottery_times = GET_A(attr_value[0]) + 1;
	uint32_t last_tm = GET_A(attr_value[1]);
	uint32_t cd_tm = conf[2]*3600;
	uint32_t item_id = conf[0];
	uint32_t item_cnt = conf[1];
	uint32_t ret = 0;
	uint32_t fix_size = array_elem_num(fix_prize);    
	uint32_t rand_size = array_elem_num(rand_prize);    
	uint32_t prize = 0;

	/* 免费次数有cd时间 */
	if(last_tm != 0 && !Utils::is_cool_down(last_tm, cd_tm)){
		/* 消耗物品 */
		ret = reduce_single_item(player, item_id, item_cnt);
	}

	if (ret) {
		return ret;
	}

	/* 固定次数奖励 */
	for(uint32_t i = 0; i < fix_size; i++){
		if (lottery_times == fix_prize[i][0]){
			prize = fix_prize[i][1];
			break;
		}
	}
	if(0 == prize){
		/* 根据概率随机奖励 */
		std::vector<uint32_t> rate_list;
		for(uint32_t i = 0; i < rand_size ; i++){
			rate_list.push_back(rand_prize[i][1]);
		}
		uint32_t rand = Utils::select_from_rate_list(rate_list);
		prize = rand_prize[rand][0];
	}
	ret = transaction_proc_prize(player, prize, noti,
			commonproto::PRIZE_REASON_NORMAL_ONE_TIME_LOTTERY,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret == 0) {
		//免费抽奖cd时间
		if(last_tm == 0 || Utils::is_cool_down(last_tm, cd_tm)){		
			last_tm = NOW();
			SET_A(attr_value[1], last_tm);
		}
		SET_A(attr_value[0], lottery_times);
	} 
	return ret;
}
//普通十连抽
int proc_normal_ten_times_lottery_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	//prize_id 品质从低到高,概率
	uint32_t rand_prize[][2] = {
		{210, 50},
		{211, 35},
		{212, 15},
	};
	//最高奖励随机到的个数min，max
	uint32_t count[] = {1, 1};
	//消耗物品id，物品数量
	uint32_t item[] = {33000, 9};
	//连抽次数
	uint32_t lottery_times = 10;

	uint32_t item_id = item[0];
	uint32_t item_cnt = item[1];
	uint32_t min_cnt = count[0];
	uint32_t max_cnt = count[1];
	uint32_t rand_size = array_elem_num(rand_prize);    
	uint32_t prize = 0;
	uint32_t ret = 0;

	//消耗物品
	ret = reduce_single_item(player, item_id, item_cnt);
	if (ret) {
		return ret;
	}

	std::vector<uint32_t> vec_prize;
	vec_prize.clear();
	//先随机出最高奖的数量
	uint32_t best_cnt = taomee::ranged_random(min_cnt, max_cnt);
	for(uint32_t i = 0; i < best_cnt; i++){
		//最高奖
		vec_prize.push_back(rand_prize[rand_size - 1][0]);
	}
	/* 根据概率随机普通奖励 */
	uint32_t normal_cnt = lottery_times - best_cnt;
	std::vector<cache_prize_elem_t> add_vec;
	std::vector<uint32_t> rate_list;
	/*加入概率 */
	for(uint32_t i = 0; i < rand_size; i++){
		rate_list.push_back(rand_prize[i][1]);
	}
	for(uint32_t i = 0; i < normal_cnt; i++){
		uint32_t rand = Utils::select_from_rate_list(rate_list);
		vec_prize.push_back(rand_prize[rand][0]);
	}
	FOREACH(vec_prize, it){
		prize = *it;
		std::vector<cache_prize_elem_t> tmp;
		tmp.clear();
		transaction_pack_prize(player, prize, tmp, NO_ADDICT_DETEC);
		add_vec.insert(add_vec.begin(), tmp.begin(), tmp.end());
	}
	ret = transaction_proc_packed_prize(player, add_vec, &noti,
           commonproto::PRIZE_REASON_NO_REASON, "金币十连抽奖励");
    noti.set_reason(commonproto::PRIZE_REASON_NORMAL_TEN_TIMES_LOTTERY);
	return ret;
}

//钻石单抽
int proc_diamond_one_time_lottery_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
    SET_A(kAttrRookieGuide7PetLottery, 1);

	/* 第一个是首抽必中，3,10 ....每10次 */
	uint32_t fix_prize[][2] =
	{
		{1, 204},
		{3, 223},
		{10, 223},
		{20, 223}
	};
	/* prize_id ，概率 */
	uint32_t rand_prize[][2] = {
		{220, 64},
		{221, 33},
		{222, 4},
	};
	/* 抽奖次数，上次免费抽奖时间 */
	attr_type_t attr_value[] = {kAttrDiamondLotteryTimes, kAttrFreeDiamondLotteryLastTime};
	/* 消耗物品id，物品数量，免费抽奖间隔时间(小时) */
	uint32_t conf[] = {33001, 1, 46};

	uint32_t lottery_times = GET_A(attr_value[0]) + 1;
	uint32_t last_tm = GET_A(attr_value[1]);
	uint32_t cd_tm = conf[2]*3600;
	uint32_t item_id = conf[0];
	uint32_t item_cnt = conf[1];
	uint32_t ret = 0;
	uint32_t fix_size = array_elem_num(fix_prize);    
	uint32_t rand_size = array_elem_num(rand_prize);    
	uint32_t prize = 0;

	/* 免费次数有cd时间 */
	if(last_tm != 0 && !Utils::is_cool_down(last_tm, cd_tm)){
		/* 消耗物品 */
		ret = reduce_single_item(player, item_id, item_cnt);
	}

	if (ret) {
		return ret;
	}
	/* 固定次数奖励 */
	uint32_t reward_cnt = 0;
	for(uint32_t i = 0; i < fix_size; i++){
		reward_cnt = fix_prize[i][0];
		if ( lottery_times == reward_cnt){
			prize = fix_prize[i][1];
			if(20 == lottery_times) {
				/* 避免抽奖次数过大 */
				lottery_times -= 10;
			}
			break;
		} 
	}
	if(0 == prize){
		/* 根据概率随机奖励 */
		std::vector<uint32_t> rate_list;
		for(uint32_t i = 0; i < rand_size ; i++){
			rate_list.push_back(rand_prize[i][1]);
		}
		uint32_t rand = Utils::select_from_rate_list(rate_list);
		prize = rand_prize[rand][0];
	}
	ret = transaction_proc_prize(player, prize, noti,
			commonproto::PRIZE_REASON_DIAMOND_ONE_TIME_LOTTERY,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);

    if (ret == 0) {
		//免费抽奖加次数和cd时间
		if(last_tm == 0 || Utils::is_cool_down(last_tm, cd_tm)){		
			last_tm = NOW();
			SET_A(attr_value[1], last_tm);
		} else {
			SET_A(attr_value[0], lottery_times);
		}
	}
	AttrUtils::add_attr_in_special_time_range(player, 
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivZhaoMu);
	
	return ret;
}

int proc_diamond_ten_times_lottery_prize_form_cli(player_t* player)
{
	/* 奖励id，概率 */
	uint32_t rand_prize[][2] = {
		{220, 54},
		{221, 33},
		{222, 13},//先抽出的奖励
		{223,  0},//固定次数奖励
		{224, 10},//
		{230,  0}//
	};
	/* 抽奖次数，上次钻石十连抽的时间, 购买钻石抽卡服务 */
	attr_type_t attr_value[] = {
		kAttrDiamondTenLotteriesTimes,
	   	kDailyDiamondTenLotteriesLastTime, 
		kServiceBuyDiamondLottery,
	   	kDailyDiamondTenLotteriesTimes,
		kAttrDiamondLotteryTime
	};
	/* 消耗物品id，钻石物品id，物品数量，间隔时间(分钟) */
	uint32_t conf[] = {33001, 90024, 9, 30};
	/* 高奖励随机到的个数min，max */
	uint32_t count[] = {1, 1};
	//全服属性限制
	global_attr_type_t g_item_limit = gDiamondTenTimesLottery; 

	uint32_t lottery_times = GET_A(attr_value[0]) + 1;
	//每日
	uint32_t last_tm = GET_A(attr_value[1]);
	uint32_t daily_times = GET_A(attr_value[3]) + 1;
	//永久
	uint32_t last_attr_tm = GET_A(attr_value[4]);
	// attr_type_t  service_id = attr_value[2];
	uint32_t cd_tm = conf[3]*60;
	uint32_t item_id = conf[0];
	// uint32_t product_id = conf[1];
	uint32_t item_cnt = conf[2];
	// if(is_gold_vip(player) && 1 == daily_times){
	if( 1 == daily_times){
		item_cnt = 5;
	}
	uint32_t min_cnt = count[0];
	uint32_t max_cnt = count[1];
	uint32_t good_prize = rand_prize[3][0];
	// uint32_t better_prize = rand_prize[3][0];
	// uint32_t better_rate = rand_prize[3][1];
	uint32_t best_prize = rand_prize[4][0];
	uint32_t ret = 0;
	// uint32_t fix_size = array_elem_num(fix_prize);    
	uint32_t rand_size = array_elem_num(rand_prize);    
	bool is_best = false;
	uint32_t prize = 0;

	/* 耗物品 */
	ret = reduce_single_item(player, item_id, item_cnt);

	if (ret) {
		return ret;
	}

	uint32_t left_cont = 10;
	std::vector<uint32_t> vec_prize;
	vec_prize.clear();
	//固定奖励
	bool is_fix = false;
	uint32_t time_conf[] = {12, 1};
	uint32_t fix_prize = rand_prize[5][0];
	if(!TimeUtils::is_time_valid(last_attr_tm, time_conf[0], time_conf[1])){
		vec_prize.push_back(fix_prize);
		is_fix = true;
		left_cont --;
	}

	/* 先随机出高奖的数量good */
	uint32_t good_cnt = taomee::ranged_random(min_cnt, max_cnt);
	for(uint32_t i = 0; i < good_cnt; i++){
		vec_prize.push_back(good_prize);
	}
	left_cont -= good_cnt;
	/* 最高奖励best */
	if(last_tm != 0 && !Utils::is_cool_down(last_tm, cd_tm)){
		/* 加入best_prize概率 */
		is_best = true;
	}
	/* 根据概率随机奖励 */
	std::vector<uint32_t> rate_list;
	for(uint32_t i = 0; i < rand_size; i++){
		if(rand_prize[i][0] == best_prize && is_best == false){ 
			continue;
		}
		rate_list.push_back(rand_prize[i][1]);
	}
	for(uint32_t i = 0; i < left_cont; i++){
		uint32_t rand = Utils::select_from_rate_list(rate_list);
		vec_prize.push_back(rand_prize[rand][0]);
	}
	//全服限制
	const global_attr_config_t *global_conf = GlobalAttrUtils::get_global_attr_config(g_item_limit);
	const uint32_t limit_item_id = global_conf->target_id;
	const uint32_t global_attr = global_conf->id;
	const uint32_t time_tid = global_conf->time_id;
	const uint32_t global_limit_cnt = global_conf->max;

	std::vector<cache_prize_elem_t> &add_vec = *(player->temp_info.cache_prize_vec);
	std::vector<cache_prize_elem_t> &tmp_vec = *(player->temp_info.cache_tmp_vec);
	std::vector<uint32_t> &cache_vec_prize = *(player->temp_info.cache_prize_id);
	add_vec.clear();
	tmp_vec.clear();
	std::vector<cache_prize_elem_t>().swap(tmp_vec);
	cache_vec_prize.clear(); 
	std::vector<uint32_t>().swap(cache_vec_prize);

	uint32_t add_item_cnt = 0;

	FOREACH(vec_prize, it){
		prize = *it;
		uint32_t tmp_id = 0;
		std::vector<cache_prize_elem_t> tmp;
		tmp.clear();
		transaction_pack_prize(player, prize, tmp, NO_ADDICT_DETEC);
		FOREACH(tmp, it){
			uint32_t item_id = (*it).id;
			if(item_id == limit_item_id){
				tmp_id = prize;
				cache_vec_prize.push_back(prize);
				add_item_cnt += (*it).count;
				break;
			}
		}
		if(tmp_id != 0){
			tmp_vec.insert(tmp_vec.begin(), tmp.begin(), tmp.end());
			continue;
		}
		add_vec.insert(add_vec.begin(), tmp.begin(), tmp.end());
	}
	//抽到全服限制的物品请求db，并等待返回结果
	if(!cache_vec_prize.empty()){
		return GlobalAttrUtils::add_attr_with_limit(player, global_attr, time_tid, add_item_cnt, global_limit_cnt);   
	}
    onlineproto::sc_0x0112_notify_get_prize noti;
    onlineproto::sc_0x0113_require_prize cli_out;
	cli_out.Clear();
	ret = transaction_proc_packed_prize(player, add_vec, &noti,
           commonproto::PRIZE_REASON_NO_REASON, "钻石十连抽奖励");
    noti.set_reason(commonproto::PRIZE_REASON_DIAMOND_TEN_TIMES_LOTTERY);
	if (ret == 0) {
		//修改时间
		last_tm = NOW();
		SET_A(attr_value[1], last_tm);
		SET_A(attr_value[4], last_tm);
		//修改次数	
		SET_A(attr_value[0], lottery_times);
		SET_A(attr_value[3], daily_times);
		send_msg_to_player(player, player->cli_wait_cmd, cli_out);
		send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti);
	}
	//赠送友情招募券
	if(2 == daily_times){
		noti.Clear();
		ret = transaction_proc_prize(player, 5028, noti,
				commonproto::PRIZE_REASON_NORMAL_LOTTERY_TICKET,
				onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
		if (ret == 0) {
			send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti);
		}
	}
	AttrUtils::add_attr_in_special_time_range(player, 
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivZhaoMu);

	return ret;
}

int RequirePrizeCmdProcessor:: proc_diamond_ten_times_lottery_prize_form_svr(player_t* player, const char* body, int bodylen)
{
	/* 抽奖次数，上次钻石十连抽的时间, 购买钻石抽卡服务 */
	attr_type_t attr_value[] = {kAttrDiamondTenLotteriesTimes, kDailyDiamondTenLotteriesLastTime, 
		kServiceBuyDiamondLottery, kDailyDiamondTenLotteriesTimes};
	//全服属性限制
	global_attr_type_t g_item_limit = gDiamondTenTimesLottery; 

	uint32_t lottery_times = GET_A(attr_value[0]) + 1;
	uint32_t last_tm = GET_A(attr_value[1]);
	uint32_t daily_times = GET_A(attr_value[3]) + 1;
	uint32_t ret = 0;

	const global_attr_config_t *global_conf = GlobalAttrUtils::get_global_attr_config(g_item_limit);
	const uint32_t limit_item_id = global_conf->target_id;
	std::vector<cache_prize_elem_t> &add_vec = *(player->temp_info.cache_prize_vec);
	std::vector<cache_prize_elem_t> &tmp_vec = *(player->temp_info.cache_tmp_vec);
	std::vector<uint32_t> &cache_vec_prize = *(player->temp_info.cache_prize_id);
	
	dbproto::sc_update_global_attr get_result;
	PARSE_SVR_MSG(get_result);

	int32_t is_succ = false;
	is_succ = get_result.is_succ();
	if(is_succ == 1){
		add_vec.insert(add_vec.begin(), tmp_vec.begin(), tmp_vec.end());
	} else {
		FOREACH(cache_vec_prize, it){
			uint32_t prize = *it;
			tmp_vec.clear();
			transaction_pack_prize_except_item(player, prize, tmp_vec, 0, false, limit_item_id);
			add_vec.insert(add_vec.begin(), tmp_vec.begin(), tmp_vec.end());
		}
	}
    onlineproto::sc_0x0112_notify_get_prize noti;
    onlineproto::sc_0x0113_require_prize cli_out;
	cli_out.Clear();
	ret = transaction_proc_packed_prize(player, add_vec, &noti,
           commonproto::PRIZE_REASON_NO_REASON, "钻石十连抽奖励");
    noti.set_reason(commonproto::PRIZE_REASON_DIAMOND_TEN_TIMES_LOTTERY);
	if (ret == 0) {
		//修改时间
		last_tm = NOW();
		SET_A(attr_value[1], last_tm);
		SET_A(attr_value[4], last_tm);
		//修改次数	
		SET_A(attr_value[0], lottery_times);
		SET_A(attr_value[3], daily_times);
		send_msg_to_player(player, player->cli_wait_cmd, cli_out);
		send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti);
	}

	//赠送友情招募券
	if(2 == daily_times){
		noti.Clear();
		ret = transaction_proc_prize(player, 5028, noti,
				commonproto::PRIZE_REASON_NORMAL_LOTTERY_TICKET,
				onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
		if (ret == 0) {
			send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti);
		}
	}
	AttrUtils::add_attr_in_special_time_range(player, 
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivZhaoMu);

	return ret;
}

int RequirePrizeCmdProcessor::proc_raw_data_from_svr(player_t* player,
		const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_raw_data_);
	if (db_raw_data_.type() == dbproto::EXPED_TOTAL_PRIZE) {
		return proc_expedition_prize(player, body, bodylen);
	} else if (db_raw_data_.type() == dbproto::NIGHT_RAID_TOTAL_PRIZE){
		return proc_night_raid_prize(player, db_raw_data_);
	}	
	return 0;
}

int proc_exped_prize_from_svr(player_t* player)
{
	//去DB里拉取查询
	return PlayerUtils::get_user_raw_data(player, dbproto::EXPED_TOTAL_PRIZE);
}

int proc_night_raid_prize_from_svr(player_t* player)
{
	//去DB里拉取查询
	return PlayerUtils::get_user_raw_data(player, dbproto::NIGHT_RAID_TOTAL_PRIZE);
}

int RequirePrizeCmdProcessor::proc_expedition_prize(player_t* player, 
		const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_raw_data_);
	//
	commonproto::prize_elem_list_t p_list;
	
	p_list.ParseFromString(db_raw_data_.raw_data());
	noti_.Clear();
	uint32_t card_id = player->expedtion->get_cur_card_id();
	uint32_t last_card_id = GET_A(kAttrExpedLastPrizeGotCardid);
	if (!(card_id >= 1 && card_id <= EXPED_HIGHEST_CARD_ID)) {
		return cli_err_exped_can_not_get_this_prize;
	}
	if (player->expedtion->check_kill_state_by_card(card_id) == false) {
		ERROR_TLOG("Must Pass Card Before Get Reward,card_id=[%u],p=[%u],c_tm=[%u]", 
				card_id, player->userid, player->create_tm);
		return cli_err_exped_get_reward_must_pass_card;
	}

	if (!(last_card_id == card_id - 1)) {
		ERROR_TLOG("Current Card Id Not Match Last Has Get Prize Card id 02:%u, %u, %u",
				player->userid, card_id, last_card_id);
		return cli_err_exped_card_id_err;
	}
	exped_conf_t* ptr = g_exped_mgr.get_exped_conf_info(card_id);
	if (ptr == NULL) {
		return cli_err_exped_card_id_err;
	}
    int ret = transaction_proc_prize(player, ptr->prize_id, noti_,
			commonproto::PRIZE_REASON_GET_EXPED_REWARD,
			onlineproto::SYNC_REASON_PRIZE_ITEM);
    if (ret) {
        return ret;
    }
	/*
	//金币
	const uint32_t GOLD_NUM = card_id * GET_A(kAttrCurBattleValue) / 120.0;
		// taomee::ranged_random(900, 1100) / 1000.0;
	commonproto::prize_elem_t* pb_prize_ptr = pb_prize_list_ptr->Add();
	pb_prize_ptr->set_type(commonproto::PRIZE_ELEM_TYPE_ATTR);
	pb_prize_ptr->set_id(kAttrGold);
	pb_prize_ptr->set_count(GOLD_NUM);
	AttrUtils::add_player_gold(player, GOLD_NUM, false, "伙伴激战奖励");	
	*/
	if (card_id % 3 == 0) {
		uint32_t index = card_id / 3;
		const uint32_t EXPED_MONEY = 150 + 50 * index;
		ADD_A(kAttrExpeditionMoney, EXPED_MONEY);
		google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> *pb_prize_list_ptr;
		pb_prize_list_ptr = noti_.mutable_award_elems();
		commonproto::prize_elem_t* pb_prize_ptr = pb_prize_list_ptr->Add();
		pb_prize_ptr->set_type(commonproto::PRIZE_ELEM_TYPE_ITEM);
		pb_prize_ptr->set_id(31009);
		pb_prize_ptr->set_count(EXPED_MONEY);
	}
    cli_out_.Clear();
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_);

	//更新累积奖励
	for (int i = 0; i < noti_.award_elems_size(); ++i) {
		p_list.add_prize_list()->CopyFrom(noti_.award_elems(i));	
	}
	PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
			dbproto::EXPED_TOTAL_PRIZE, p_list, "0");

	SET_A(kAttrExpedLastPrizeGotCardid, last_card_id + 1);
    return 0;
}

int proc_exped_prize(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	uint32_t card_id = player->expedtion->get_cur_card_id();
	uint32_t last_card_id = GET_A(kAttrExpedLastPrizeGotCardid);
	if (!(card_id >= 1 && card_id <= EXPED_HIGHEST_CARD_ID)) {
		return cli_err_exped_can_not_get_this_prize;
	}
	if (card_id == 1 && player->expedtion->check_cur_card_kill_state() == false) {
		return cli_err_exped_can_not_get_this_prize;
	}
	//非第12关的情况
	if (!(last_card_id == card_id - 1)) {
		ERROR_TLOG("Current Card Id Not Match Last Has Get Prize Card id 02:%u, %u, %u",
				player->userid, card_id, last_card_id);
		return cli_err_exped_card_id_err;
	}
	exped_conf_t* ptr = g_exped_mgr.get_exped_conf_info(last_card_id + 1);
	if (ptr == NULL) {
		return cli_err_exped_card_id_err;
	}
	prize_id = ptr->prize_id;
    int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_GET_EXPED_REWARD,
			onlineproto::SYNC_REASON_PRIZE_ITEM);
    if (ret) {
        return ret;
    }
	/*
	//金币
	const uint32_t GOLD_NUM = (last_card_id + 1) * GET_A(kAttrCurBattleValue) / 120.0
		* taomee::ranged_random(900, 1100) / 1000.0;
	commonproto::prize_elem_t* pb_prize_ptr = pb_prize_list_ptr->Add();
	pb_prize_ptr->set_type(commonproto::PRIZE_ELEM_TYPE_ATTR);
	pb_prize_ptr->set_id(kAttrGold);
	pb_prize_ptr->set_count(GOLD_NUM);
	AttrUtils::add_player_gold(player, GOLD_NUM, false, "伙伴激战奖励");	
	*/
	if ((last_card_id + 1) % 3 == 0) {
		uint32_t index = (last_card_id + 1) / 3;
		const uint32_t EXPED_MONEY = 100 * index;
		ADD_A(kAttrExpeditionMoney, EXPED_MONEY);
		google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> *pb_prize_list_ptr;
		pb_prize_list_ptr = noti.mutable_award_elems();
		commonproto::prize_elem_t* pb_prize_ptr = pb_prize_list_ptr->Add();
		pb_prize_ptr->set_type(commonproto::PRIZE_ELEM_TYPE_ATTR);
		pb_prize_ptr->set_id(kAttrExpeditionMoney);
		pb_prize_ptr->set_count(EXPED_MONEY);
	}
	SET_A(kAttrExpedLastPrizeGotCardid, last_card_id + 1);
	//把奖励的内容添加到 raw_data中
	return 0;
}

int proc_change_clothes_plan(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
    //防沉迷设置
	if( check_player_addicted_threshold_none(player)){
		ERROR_TLOG("%u exceed online time threshold time, can't get online_reward", player->userid);
		return cli_err_addicted_time_threshold_max;
	}	

    uint32_t reward_mask = 0;
    int ret = can_get_change_clothes_plan_reward(player, prize_id, reward_mask);
    if(ret) {
        return ret;
    }

    ret = transaction_proc_prize(player, prize_id, noti,
            commonproto::PRIZE_REASON_CHANGE_CLOTHES_PLAN, onlineproto::SYNC_REASON_PRIZE_ITEM,NO_ADDICT_DETEC);
    if (ret) {
        return ret;
    }

    if (reward_mask) {
        SET_A(kAttrChangeClothesPlan, reward_mask);

        uint32_t cnt = 0;
        for (int i = 1; i <= 32; i++) {
            if (taomee::test_bit_on(GET_A(kAttrChangeClothesPlan), i)) {
                cnt++;
            }
        }

        if (cnt == 1) {
            Utils::write_msglog_new(player->userid, "奖励", "换装计划", "只完成一个任务人数");
        } else if (cnt == 2) {
            Utils::write_msglog_new(player->userid, "奖励", "换装计划", "完成两个任务人数");
        } else if (cnt == 3) {
            Utils::write_msglog_new(player->userid, "奖励", "换装计划", "完成三个任务人数");
        } else if (cnt == 4) {
            Utils::write_msglog_new(player->userid, "奖励", "换装计划", "完成四个任务人数");
        }
    }
    
	return 0;
}

int update_change_clothes_plan_record(player_t *player, commonproto::pet_info_t *pet_info) 
{
    if (player  == NULL || pet_info == NULL) {
        return 0;
    }

    FOREACH(pet_info->rune_info(), iter) {
        rune_t rune;
        RuneUtils::get_rune(player, iter->runeid(), rune);
        rune_conf_t *rune_conf = g_rune_conf_mgr.get_rune_conf_t_info(rune.conf_id);
        if (rune_conf && 
                rune_conf->rune_type == kRunePurple) {
            SET_A(kAttrChangeClothesPlanPurpleRune, 1);
            break;
        }
    }

    return 0;
}

int update_change_clothes_plan_start_time(player_t *player)
{
    // TODO toby leonxu confirm
    return 0;

    uint32_t start_time = GET_A(kAttrChangeClothesPlanStartTime);
    if (start_time) {
        return 0;
    }
    uint32_t level = GET_A(kAttrLv);
    uint32_t start_level = g_module_mgr.get_module_conf_uint32_def(
            module_type_change_clothes_plan, "start_level", 17);
    if (level >= start_level) {
        SET_A(kAttrChangeClothesPlanStartTime, NOW());
    }
    return 0;
}

int proc_favorite_ct_reward(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	if (GET_A(kAttrFavoriteCtRewardFlag)) {
		return cli_err_has_get_favorite_ct_reward;
	}
	if (prize_id != 2002) {
		return cli_err_prize_id_invalid;
	}

    int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_FAVORITE_CT_REWARD,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
    if (ret) {
        return ret;
    }

	SET_A(kAttrFavoriteCtRewardFlag, 1);
	
	return 0;
}

int can_get_change_clothes_plan_reward(
        player_t *player, uint32_t prize_id, uint32_t &reward_record)
{
    if (player == NULL) {
        return cli_err_user_not_exist;
    }
    
    // TODO toby leonxu confirm
    //uint32_t start_time = GET_A(kAttrChangeClothesPlanStartTime);
    //if (start_time == 0) {
        //return  cli_err_change_clothes_plan_not_start;
    //}
    //uint32_t gap_days = TimeUtils::get_days_between(start_time, NOW()); 

    uint32_t gap_days = TimeUtils::get_days_between(player->create_tm, NOW()); 
    uint32_t gap_config = g_module_mgr.get_module_conf_uint32_def(
            module_type_change_clothes_plan, "activity_time", 14);
    if (gap_days >= gap_config) {
        return  cli_err_change_clothes_plan_over_time;
    }

    // 换装计划 
    uint32_t male_prize_start = g_module_mgr.get_module_conf_uint32_def(
            module_type_change_clothes_plan, "male_prize_start_id", 10101);;
    uint32_t female_prize_start = g_module_mgr.get_module_conf_uint32_def(
            module_type_change_clothes_plan, "female_prize_start_id", 10105);
    uint32_t prize_cnt = g_module_mgr.get_module_conf_uint32_def(
            module_type_change_clothes_plan, "prize_cnt", 4);

    uint32_t sex = GET_A(kAttrSex);
    if (!(sex == commonproto::MALE && 
                prize_id >= male_prize_start && 
                prize_id < male_prize_start + prize_cnt) &&
            !(sex == commonproto::FEMALE && 
                prize_id >= female_prize_start && 
                prize_id < female_prize_start + prize_cnt)) {
        return cli_err_prize_id_invalid;
    }

    uint32_t reward_mask = GET_A(kAttrChangeClothesPlan);
    uint32_t reward_idx = prize_id - male_prize_start + 1;
    if (taomee::test_bit_on(reward_mask, reward_idx)) {
        return cli_err_change_clothes_plan_repeat_reward;
    }

    uint32_t cond_idx = 0;
    if (sex == commonproto::FEMALE) {
        cond_idx = prize_id - female_prize_start + 1; 
    } else {
        cond_idx = prize_id - male_prize_start + 1; 
    }

    uint32_t cond_mask = 0;
    FOREACH(*(player->pets), iter) {
        // 任意伙伴身上出现紫色符文
        if (cond_idx == 1) {
            for (uint32_t i = 0; i < kMaxEquipRunesNum; i++) {
                uint32_t rune_id = iter->second.get_rune_array(i);
                rune_t rune;
                RuneUtils::get_rune(player, rune_id, rune);
                rune_conf_t *rune_conf = g_rune_conf_mgr.get_rune_conf_t_info(rune.conf_id);
                if (rune_conf && rune_conf->rune_type >= kRunePurple) {
                    cond_mask = taomee::set_bit_on(cond_mask, cond_idx);

                    Utils::write_msglog_new(player->userid, "奖励", "换装计划", "完成装备紫色符文人数");
                    break;
                }
            }

            uint32_t rune_flag = GET_A(kAttrChangeClothesPlanPurpleRune);
            if (rune_flag) {
                cond_mask = taomee::set_bit_on(cond_mask, cond_idx);
                Utils::write_msglog_new(player->userid, "奖励", "换装计划", "完成装备紫色符文人数");
            }

            if (taomee::test_bit_on(cond_mask, cond_idx)) {
                break;
            }
        }

        // 任意伙伴的五项属性总和达到50
        if (cond_idx == 2 && iter->second.get_effort_lv_sum() >= 50) {
            cond_mask = taomee::set_bit_on(cond_mask, cond_idx);

            Utils::write_msglog_new(player->userid, "奖励", "换装计划", "完成特训50级人数");
            break;
        }

        // 任意伙伴为蓝色品质
        if (cond_idx == 3 && iter->second.quality() >= kPetQualityBlue) {
            cond_mask = taomee::set_bit_on(cond_mask, cond_idx);

            Utils::write_msglog_new(player->userid, "奖励", "换装计划", "完成伙伴进阶蓝色人数");
            break;
        }

        // 任意伙伴为三星
        if (cond_idx == 4 && iter->second.talent_level() >= kPetTalentLevelThree) {
            cond_mask = taomee::set_bit_on(cond_mask, cond_idx);

            Utils::write_msglog_new(player->userid, "奖励", "换装计划", "完成赤瞳升三星人数");
            break;
        }
    }

    if (taomee::test_bit_on(cond_mask, cond_idx)) {
        reward_mask = taomee::set_bit_on(reward_mask, reward_idx);
        reward_record = reward_mask;
        return 0;
    }
    
   return cli_err_not_meet_the_conditions_of_prize;
}

//签到标志位type:5507
//七日豪礼
int proc_seven_days_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	//防沉迷设置
	// if( check_player_addicted_threshold_none(player)){
		// ERROR_TLOG("%u exceed online time threshold time, can't enter sign_in", player->userid);
		// return cli_err_addicted_time_threshold_max;
	// }	

	uint32_t data[] = {
		     10001,
			 10002,
			 10003,
			 10004,
			 10005,
			 10006,
			 10007
	};


	/* 活动时间id, tid */
	// uint32_t activ_time[] = {2, 1};
	/* 标志位key */
	attr_type_t key = kAttrSevenDaysFlag;
	attr_type_t daily_times = kDailySevenSignTimes;
	
	int ret = 0;
	uint32_t size = array_elem_num(data);    
	uint32_t prizeid_start = data[0];
	uint32_t prizeid_end   = data[size - 1];

	//检查prize_id是否合法
	if(prize_id > prizeid_end || prize_id < prizeid_start){ 
		ERROR_TLOG("prizeid invalid uid=[%u]:prizeid=[%u]", player->userid, prize_id); 
        return cli_err_prize_id_invalid; 
	} 

	uint32_t idx = prize_id - prizeid_start;
	uint32_t flag = GET_A(key); 
	uint32_t daily_tm = GET_A(daily_times); 
	uint32_t day_sign = idx + 1;               //签到第几天
	// uint32_t time_id = activ_time[0];
	// uint32_t time_tid = activ_time[1];

	//检测活动时间是否合法
	// bool is_cur_time = false ;
	// is_cur_time = TimeUtils::is_current_time_valid(time_id, time_tid);
	// if (is_cur_time == false){
		// return cli_err_activity_time_invalid;
	// }
	//今日是否领取	
	if(daily_tm >= 1){
		return cli_err_repeat_sign_in;
	}	
	//检查之前是否领取
	for(uint32_t i = 1; i < day_sign ; i++){
		if (!taomee::test_bit_on(flag, i)){
			ERROR_TLOG("prizeid invalid uid=[%u]:prizeid=[%u]", player->userid, prize_id); 
			return cli_err_sign_in_day_not_come; 
		}
	}

	if (!taomee::test_bit_on(flag, day_sign)){
		ret = transaction_proc_prize(player, prize_id, noti,
				commonproto::PRIZE_REASON_SEVEN_DAYS_REWARD);
		if (ret) {
			return ret;
		}
		flag = taomee::set_bit_on(flag, day_sign);
		SET_A(key, flag);
		daily_tm += 1;
		SET_A(daily_times, daily_tm);
	} else {
		return cli_err_repeat_sign_in;
	}
	return 0;
}
//  奖励Id必须连续,签到标志位type: 30001
int proc_month_sign_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	//防沉迷设置
	if( check_player_addicted_threshold_none(player)){
		ERROR_TLOG("%u exceed online time threshold time, can't enter sign_in", player->userid);
		return cli_err_addicted_time_threshold_max;
	}	

	//奖励id  1-31天
	uint32_t day_prize_id[] = {10301, 10331};
	//累计签到奖励
	uint32_t accumulative_reward[][2] = {
		//签到天数，奖励id
		{2,  10350},
		{5,  10351},
		{10, 10352},
		{15, 10353},
		{23, 10354},
		{31, 10355},
	};

	//月奖励id偏移
	uint32_t offset = 0;
	time_t time = NOW();
	if(!TimeUtils::is_even_number_month(time)){
		offset = 100;
	}
	//补签道具
	uint32_t item_conf[] = {90031, kServiceBuyMonthSign, 1};
	// uint32_t vip_free_times[] = {1, 3, 5};
	uint32_t vip_free_times[] = {1, 3};

	/* 标志位key */
	attr_type_t sign_key = kMonthlySignInFlag;
	//免费补签次数
	attr_type_t free_sign_times = kMonthlyFreeSignTimes;
	//累计签到奖励标志位
	attr_type_t accumulative_sign_key = kMonthlyAccumulativeSignReward;
	attr_type_t diamond_sign_key = kMonthlyDiamondSignTimes;
	
	int ret = 0;
	uint32_t time_now = NOW();
	uint32_t month_max_num = TimeUtils::max_day_of_month(time_now);
	uint32_t size = array_elem_num(accumulative_reward);    
	uint32_t prizeid_start = day_prize_id[0] + offset;
	uint32_t prizeid_end = day_prize_id[0] + offset + month_max_num - 1;
	//累计签到
	uint32_t reward_start = accumulative_reward[0][1] + offset;
	uint32_t reward_end = accumulative_reward[size - 1][1] + offset;
	uint32_t sign_flag = GET_A(sign_key); 

	//检查prize_id是否合法
	if (prize_id >= prizeid_start && prize_id <= prizeid_end){
		//签到逻辑
		uint32_t day_sign = prize_id - prizeid_start + 1; 
		uint32_t item_id = item_conf[0]; 
		uint32_t item_cnt = item_conf[2]; 
		attr_type_t item_type = attr_type_t(item_conf[1]); 
		uint32_t free_signed_times = GET_A(free_sign_times); 
		uint32_t diamond_signed_times = GET_A(diamond_sign_key);//第几次补签 
		bool free_sign = false;

		//检测是否重复签到
		if (!taomee::test_bit_on(sign_flag, day_sign)){
			uint32_t time_start = TimeUtils::get_cur_month_first_time(time_now); //本月开始时间
			uint32_t diff_day = TimeUtils::get_days_between(time_start, time_now) + 1; //正常签到的天数
			if (day_sign <= diff_day){
				if(day_sign < diff_day){
					//免费补签
					if( (is_silver_vip(player) && free_signed_times < vip_free_times[0]) || 
							(is_gold_vip(player) && free_signed_times < vip_free_times[1])){

						free_sign = true;
					}

					if(free_sign != true){
						if(diamond_signed_times > 0){
							uint32_t cnt = uint32_t((diamond_signed_times + 1) / 2);
							item_cnt = pow(2.0, cnt*1.0);
						} 
						ret = buy_attr_and_use(player, item_type, item_id, item_cnt);
						if (ret) {
							return ret;
						}
						diamond_signed_times++;
						SET_A(diamond_sign_key, diamond_signed_times);
					}

				}
				ret = transaction_proc_prize(player, prize_id, noti,
						commonproto::PRIZE_REASON_MONTH_SIGN_REWARD);
				if (ret) {
					return ret;
				}
				sign_flag = taomee::set_bit_on(sign_flag, day_sign);
				SET_A(sign_key, sign_flag);
				if (free_sign) {
					free_signed_times += 1;
					SET_A(free_sign_times, free_signed_times);
				}
				return 0;
			} else {
				return cli_err_sign_in_day_not_come;
			}	
		}else {
			return cli_err_repeat_sign_in;
		}

	} else if(prize_id >= reward_start && prize_id <= reward_end){
		//领取累计签到奖励逻辑
		//累计签到奖励标志位
		uint32_t accumulative_sign_flag = GET_A(accumulative_sign_key); 
		uint32_t idx = prize_id - reward_start + 1; 
		//要求累计天数
		// uint32_t need_days = idx < size ? accumulative_reward[idx -1][0] : month_max_num;
		uint32_t need_days = accumulative_reward[idx -1][0];
		if(need_days == 31){
			need_days = month_max_num;
		}
		uint32_t sign_days = 0;
		for (uint32_t i = 1; i <= 32 ; i++){
			if (taomee::test_bit_on(sign_flag, i)){
				sign_days ++;
			}
		}
		if (!taomee::test_bit_on(accumulative_sign_flag, idx)){
			if(sign_days >= need_days){
				ret = transaction_proc_prize(player, prize_id, noti,
						commonproto::PRIZE_REASON_MONTH_SIGN_REWARD);
				if (ret) {
					return ret;
				}
				accumulative_sign_flag = taomee::set_bit_on(accumulative_sign_flag , idx);
				SET_A(accumulative_sign_key, accumulative_sign_flag);
				return 0;
			} else {
				return cli_err_not_meet_the_conditions_of_prize; 
			}

		} else {
			return cli_err_prize_already_get;
		}
	} else {
		ERROR_TLOG("prizeid invalid uid=[%u]:prizeid=[%u]", player->userid, prize_id); 
		return cli_err_prize_id_invalid; 
	}	
}

int RequirePrizeCmdProcessor::proc_night_raid_prize(player_t* player, const dbproto::sc_user_raw_data_get &msg)
{
	//防沉迷设置
	// if( check_player_addicted_threshold_none(player)){
		// ERROR_TLOG("%u exceed online time threshold time, can't enter sign_in", player->userid);
		// return cli_err_addicted_time_threshold_max;
	// }	

	uint32_t data[] = {
		     9851,
			 9852,
			 9853,
			 9854,
			 9855,
			 9856,
			 9857,
			 9858,
			 9859,
			 9860,
	};

	/* 活动时间id, tid */
	// uint32_t activ_time[] = {2, 1};
	/* 标志位key */
	attr_type_t prize_key = kAttrNightRaidGotPrizeFlag;
	attr_type_t unlock_key = kAttrNightRaidUnlockFlag;
	// attr_type_t daily_times = kDailySevenSignTimes;
	
	int ret = 0;
	uint32_t size = array_elem_num(data);    
	uint32_t prize_flag = GET_A(prize_key ); 
	uint32_t prize_idx = 0;
	uint32_t unlock_flag = GET_A(unlock_key); 
	uint32_t unlock_id  = 0; 

	for(uint32_t i = 1; i <= 10; i++){
		if (taomee::test_bit_on(unlock_flag, i)) {
			unlock_id ++; 
		} else {
			break;
		}
	}

	for(uint32_t i = 1; i <= 10; i++){
		if (taomee::test_bit_on(prize_flag, i)) {
			prize_idx ++; 
		} else {
			break;
		}
	}

	if(unlock_id > size || unlock_id < 1){
		ERROR_TLOG("prizeid invalid uid=[%u]:unlockid=[%u]", player->userid, unlock_id); 
		return cli_err_prize_id_invalid; 
	}
	uint32_t prizeid = data[unlock_id - 1]; 
	if(unlock_id == prize_idx){
		return cli_err_repeat_get_reward;
	} else if ((unlock_id - prize_idx) == 1){

		onlineproto::sc_0x0112_notify_get_prize noti_;
		onlineproto::sc_0x0113_require_prize cli_out_;
		noti_.Clear();
		cli_out_.Clear();

		ret = transaction_proc_prize(player, prizeid, noti_,
				commonproto::PRIZE_REASON_NIGHT_RAID_REWARD);
		if (ret) {
			return ret;
		}
		//金币输出
		const uint32_t GOLD_NUM = unlock_id * GET_A(kAttrCurBattleValue) / 120.0;
		commonproto::prize_elem_t *add_elem = noti_.add_award_elems();
		add_elem->set_type(commonproto::PRIZE_ELEM_TYPE_ATTR);
		add_elem->set_id(kAttrGold);
		add_elem->set_count(GOLD_NUM);

		prize_flag = taomee::set_bit_on(prize_flag, unlock_id);
		SET_A(prize_key, prize_flag);

		send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
		send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_);

		commonproto::prize_elem_list_t p_list;
		p_list.Clear();
		p_list.ParseFromString(msg.raw_data());
		//更新累积奖励
		for (int i = 0; i < noti_.award_elems_size(); ++i) {
			p_list.add_prize_list()->CopyFrom(noti_.award_elems(i));	
		}
		PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
				dbproto::NIGHT_RAID_TOTAL_PRIZE, p_list, "0");

	} else {
		ERROR_TLOG("prizeid invalid uid=[%u]:unlockid=[%u]", player->userid, unlock_id); 
		return cli_err_exped_can_not_get_this_prize;

	}	

	return 0;
}

//签到标志位type:5510
int proc_wei_xin_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{

	/* 活动时间id, tid */
	// uint32_t activ_time[] = {2, 1};
	/* 标志位key */
	attr_type_t key = kAttrWeiXinRewardFlag;
	
	int ret = 0;
	uint32_t prize = 2001;

	//检查prize_id是否合法
	if(prize_id != prize){ 
		ERROR_TLOG("prizeid invalid uid=[%u]:prizeid=[%u]", player->userid, prize_id); 
        return cli_err_prize_id_invalid; 
	} 

	uint32_t flag = GET_A(key); 

	//今日是否领取	
	if(flag != 0){
		return cli_err_repeat_get_reward;
	}	

	ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_WEI_XIN_REWARD);
	if (ret) {
		return ret;
	}
	flag = taomee::set_bit_on(flag, 1);
	SET_A(key, flag);
	return 0;
}
//月卡领奖5513
int proc_month_card_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{

	attr_type_t timekey = kAttrMonthCardEndTime;
	attr_type_t flagkey = kDailyMonthCardRewardFlag;
	
	int ret = 0;
	uint32_t prize = 2050;

	uint32_t endtime = GET_A(timekey); 
	uint32_t flag = GET_A(flagkey); 

	if (endtime < NOW()) {
		return cli_err_month_card_time_out;
	} 
	//今日是否领取	
	if(flag != 0){
		return cli_err_repeat_get_reward;
	}	

	ret = transaction_proc_prize(player, prize, noti,
			commonproto::PRIZE_REASON_MONTH_CARD_REWARD, onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}

	SET_A(flagkey, 1);
    // 支线任务记录
    SET_A(kAttrRookieGuide15BuyMonthCard, 1);
	return 0;
}

int proc_get_year_vip_reward(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	prize_id = 2200;
	if (GET_A(kAttrVipGiftBagGetFlag) != commonproto::VIP_GIFT_CAN_GET) {
		return cli_err_can_get_year_vip_gift;
	}
	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_GET_YEAR_VIP_REWARD,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}
	SET_A(kAttrVipGiftBagGetFlag, commonproto::VIP_GIFT_HAS_GET);

	static char msg[256];
	snprintf(msg, sizeof(msg), "[pi= + %u + | + %u + ] %s [/pi]购买了[cl=0x00DE00]黄金勋章[/cl],获得了[cl=0xE500FF]8888钻礼包[/cl]和[cl=0xE500FF]艾丝[/cl]！", 
			player->userid, player->create_tm, player->nick);

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
	return 0;
}

int proc_battle_value_gift(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
    const uint32_t prize_id_base = 2401;
    uint32_t battle_values[] = {
        3000, 5000, 8000, 10000, 12000,
	   	15000, 20000, 25000, 30000, 35000, 40000,
	   	2000, 4000, 6500, 45000, 50000
    };
    const uint32_t levels = sizeof(battle_values) / sizeof(uint32_t);

    if (prize_id < prize_id_base || prize_id > prize_id_base + levels - 1) {
        return cli_err_prize_id_invalid;
    }
    uint32_t idx = prize_id - prize_id_base;
    if (GET_A(kAttrBattleValueRecord) < battle_values[idx]) {
        return cli_err_can_not_get_prize_yet;
    }
    uint32_t prize_flag = GET_A(kAttrBattleValueGiftGetFlag);
    if (taomee::test_bit_on(prize_flag, idx+1)) {
        return cli_err_prize_already_get;
    }
	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_BATTLE_VALUE_GIFT);
	if (ret) {
		return ret;
	}
    prize_flag = taomee::set_bit_on(prize_flag, idx+1);
    SET_A(kAttrBattleValueGiftGetFlag, prize_flag);
    return 0;
}

int proc_sign_invite_code_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
    prize_id = 2020;
    if (GET_A(kAttrSignInviteCode) == 0) {
        return cli_err_can_not_get_prize_yet;
    }
    if (GET_A(kAttrInviteCodePrizeGetFlag)) {
        return cli_err_prize_already_get;
    }
	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_SIGN_INVITE_CODE_PRIZE,onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}
    SET_A(kAttrInviteCodePrizeGetFlag, NOW());
    return 0;
}

int proc_invited_players_reach_n_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
    uint32_t base_prize_id = 2021;
    uint32_t level = 7;
    uint32_t requires[] = {1, 2, 5, 10, 20, 30, 50};

    if (prize_id < base_prize_id || prize_id > base_prize_id + level - 1) {
        return cli_err_prize_id_invalid;
    }
    uint32_t idx = prize_id - base_prize_id;
    uint32_t prize_flag = GET_A(kAttrInvitedPlayersPrizeGetFlag);
    if (taomee::test_bit_on(prize_flag, idx+1)) {
        return cli_err_prize_already_get;
    }

    if (GET_A(kAttrInvitedPlayers) < requires[idx]) {
        return cli_err_can_not_get_prize_yet;
    }
	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_INVITE_N_PLAYERS_PRIZE,onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}
    prize_flag = taomee::set_bit_on(prize_flag, idx+1);
    SET_A(kAttrInvitedPlayersPrizeGetFlag, prize_flag);
    return 0;
}

int proc_share_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
    if (GET_A(kDailyShareGetPrizeFlag)) {
        return cli_err_prize_already_get;
    }
    prize_id = 2019;
	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_SHARE_PRIZE);
	if (ret) {
		return ret;
	}
    SET_A(kDailyShareGetPrizeFlag, 1);
	
	//更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_DAILY_SHARE, 1);
    return 0;
}

int proc_get_first_recharge_gift(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
    if (GET_A(kAttrGetFirstRechargeGiftFlag)) {
        return cli_err_prize_already_get;
    }
	if (GET_A(kAttrFirstRechargeFlag) == 0) {
		return cli_err_must_recharge_to_get_reward;
	}
    prize_id = 2003;
	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_GET_FIRST_RECHARGE,onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}
    SET_A(kAttrGetFirstRechargeGiftFlag, NOW());

	static char msg[256];
	snprintf(msg, sizeof(msg), "[pi= + %u + | + %u + ] %s [/pi]在[cl=0x00DE00]首充奖励[/cl]中获得了超强伙伴[cl=0xE500FF]蕾欧奈[/cl]和[cl=0xE500FF]紫色武器[/cl]！", 
			player->userid, player->create_tm, player->nick);

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
    return 0;
}

int proc_consume_diamond_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
    uint32_t base_prize_id = 2004;
    uint32_t level = 6;
    uint32_t requires[] = {200, 500, 1000, 3000, 5000, 10000};

    if (prize_id < base_prize_id || prize_id > base_prize_id + level - 1) {
        return cli_err_prize_id_invalid;
    }
    uint32_t idx = prize_id - base_prize_id;
    uint32_t prize_flag = GET_A(kAttrConsumeDiamondGiftGetFlag);
    if (taomee::test_bit_on(prize_flag, idx+1)) {
        return cli_err_prize_already_get;
    }

    if (GET_A(kAttrConsumeDiamondCnt) < requires[idx]) {
        return cli_err_can_not_get_prize_yet;
    }
	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_CONSUME_DIAMOND,onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}
    prize_flag = taomee::set_bit_on(prize_flag, idx+1);
    SET_A(kAttrConsumeDiamondGiftGetFlag, prize_flag);
    return 0;
}

int proc_evil_knife_legend(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
    uint32_t conf_id = g_module_mgr.get_module_conf_uint32_def(
            module_type_evil_knife_legend, "prize_id", 1010);

    if (prize_id != conf_id) {
        return cli_err_prize_id_invalid;
    }

    uint32_t reward_flag = GET_A(kAttrEvilKnifeLegendRewardFlag);
    if (reward_flag) {
        return cli_err_repeat_get_reward;
    }

    uint32_t time_gap = g_module_mgr.get_module_conf_uint32_def(
            module_type_evil_knife_legend, "time_gap", 3600 * 12);
    uint32_t start_time = GET_A(kAttrEvilKnifeLegendRewardTime);
    if (start_time == 0) {
        return cli_err_evil_knife_lengend_task_not_finish;
    }

    if (!is_vip(player)) {//如果不是VIP
        if (start_time + time_gap > NOW()) {
            return cli_err_in_cd_time;
        }
    }


	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_EVIL_KNIFE_LEGEND, onlineproto::SYNC_REASON_PRIZE_ITEM);
	if (ret) {
		return ret;
	}

    SET_A(kAttrEvilKnifeLegendRewardFlag, 1);
    return 0;
}

int proc_mayin_bucket_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	uint32_t base_prize_id = 11201;
	uint32_t idx = prize_id - base_prize_id;
	if (!(idx >= 0 && idx <= 3)) {
		return cli_err_prize_id_invalid;
	}
	if (GET_A(attr_type_t(kAttrMayinGift01RecvState + idx)) != 1) {
		return cli_err_not_meet_the_conditions_of_prize;
	}
    int ret = transaction_proc_prize(player, prize_id, noti,
            commonproto::PRIZE_REASON_MAYIN_BUCKET_PRIZE,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
    if (ret) {
        return ret;
    }
	SET_A(attr_type_t(kAttrMayinGift01RecvState + idx), 2);
	if (idx == 3) {
		static char msg[256];
		snprintf(msg, sizeof(msg), "恭喜[pi=%u|%u]%s[/pi]已为帝具浪漫炮台充满能量，获得[cl=0xe36c0a]能量大礼包[/cl]！[op=%u|%u]我要参加[/op]", 
				player->userid, player->create_tm, player->nick, player->userid,
				(uint32_t)commonproto::PRIZE_REASON_MAYIN_BUCKET_PRIZE);

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

	//添加统计项
	if (idx == 0) {
		Utils::write_msglog_new(player->userid, "收费", "玛音浪漫试练:上", "150点能量领取礼包");
	} else if (idx == 1) {
		Utils::write_msglog_new(player->userid, "收费", "玛音浪漫试练:上", "300点能量领取礼包");
	} else if (idx == 2) {
		Utils::write_msglog_new(player->userid, "收费", "玛音浪漫试练:上", "500点能量领取礼包");
	} else if (idx == 3) {
		Utils::write_msglog_new(player->userid, "收费", "玛音浪漫试练:上", "能量大礼包领取");
	}
	return 0;
}

//玛茵射击奖励
int proc_marly_shot_gift(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
	int ret = 0;
    uint32_t prize[2] = {11001, 11002};	
	if(prize_id == prize[0]){
		//玛茵意念
		// uint32_t item_id =  71000;
		// 黑瞳
		// uint32_t item_id =  71001;
		//
		uint32_t item_id =  71004;
		uint32_t item_cnt = 1;
		ret = reduce_single_item(player, item_id, item_cnt);
		if (ret) {
			return ret;
		}
	} else if (prize_id == prize[1]){
        attr_type_t attr_type = kServiceBuyOutburstShot;	
        uint32_t product_id = 90043;	
        uint32_t item_cnt = 1;	
        ret = buy_attr_and_use(player, attr_type, product_id, item_cnt);
		if (ret) {
			return ret;
		}
	} else {
        return cli_err_prize_id_invalid;
	}

	ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_MARLY_SHOT_GIFT,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}
    return 0;
}

//玛茵答题奖励
int proc_marly_question_gift(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
	int ret = 0;
    uint32_t prize[][2] = {
		{10, 11211},
		{20, 11212},
		{30, 11213},
		{40, 11214},
		{50, 11215},
		{60, 11216},
		{70, 11217},
	};	
	attr_type_t attr_value[] = {
		kAttrAnswerMaryQuestionCorrectCnt, 
		kAttrGetMaryQuestionRewardFlag,
		kDailyAnswerMaryQuestionTimes,
		kDailyGetMaryQuestionParticipationGift
	};
	
	uint32_t prize_maxid = prize[6][1];
	uint32_t prize_minid = prize[0][1];
	//每日参与奖励
	uint32_t participation_prizeid = 11218;

	//累积奖励check
	if(prize_id <= prize_maxid && prize_id >= prize_minid){
		uint32_t idx = prize_id - prize_minid;
		uint32_t times = GET_A(attr_value[0]);
		uint32_t reward_flag = GET_A(attr_value[1]);
		uint32_t need_times = prize[idx][0];

		if(taomee::test_bit_on(reward_flag, idx + 1)){
			return cli_err_repeat_get_reward;
		}

		if(times < need_times){
			return cli_err_can_not_get_prize_yet;
		}	
		ret = transaction_proc_prize(player, prize_id, noti,
				commonproto::PRIZE_REASON_MARY_QUESTION_PRIZE,
				onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
		if (ret) {
			return ret;
		}

		reward_flag = taomee::set_bit_on(reward_flag, idx + 1);
		SET_A(attr_value[1], reward_flag);
		//统计
		string stat;
		switch(idx){
			case 0:
				stat = "答对10题领取";
				break;
			case 1:
				stat = "答对20题领取";
				break;
			case 2:
				stat = "答对30题领取";
				break;
			case 3:
				stat = "答对40题领取";
				break;
			case 4:
				stat = "答对50题领取";
				break;
			case 5:
				stat = "答对60题领取";
				break;
			case 6:
				stat = "答对70题领取";
				break;
			default:
				stat = "";
		}	

		Utils::write_msglog_new(player->userid, "玛音浪漫试炼（上）", "傲娇属性问答", stat);
	} else if (prize_id == participation_prizeid){
		//参与奖励check
		uint32_t question_times = GET_A(attr_value[2]);
		uint32_t get_gift_times = GET_A(attr_value[3]);
		if(get_gift_times != 0){
			return cli_err_repeat_get_reward;
		}
		if(question_times == 0){
			return cli_err_can_not_get_prize_yet;
		}

		ret = transaction_proc_prize(player, prize_id, noti,
				commonproto::PRIZE_REASON_MARY_QUESTION_PRIZE,
				onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
		if (ret) {
			return ret;
		}

		SET_A(attr_value[3], 1);
	} else {
		return cli_err_prize_id_invalid;
	}

    return 0;
}

int proc_survey_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
    uint32_t base_prize_id = 12001;
    uint32_t max_prize_id = 12050;
    if (prize_id < base_prize_id || prize_id > max_prize_id) {
        return cli_err_prize_id_invalid;
    }
    if (GET_A((attr_type_t)(prize_id - base_prize_id + kAttrSurveyVipPrizeGet)) != 0) {
        return cli_err_prize_already_get;
    }
    int ret = transaction_proc_prize(player, prize_id, noti,
            commonproto::PRIZE_REASON_SURVEY,
            onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
    if (ret) {
        return ret;
    }
    SET_A((attr_type_t)(prize_id - base_prize_id + kAttrSurveyVipPrizeGet), NOW());
    return 0;
}

int proc_vip_get_yaya_fragment(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	if (!(prize_id == 11958 || prize_id == 11959)) {
		return cli_err_prize_id_invalid;
	}
	uint32_t pos = 1;
	if (prize_id == 11959) {
		pos = 2;
	}
	if (!is_vip(player)) {
		return cli_err_not_vip;
	} else if (!is_gold_vip(player) && prize_id == 11959) {
		return cli_err_gold_vip_can_get_this_prize;
	}
	uint32_t flag = GET_A(kAttrVipGetYayaFragmentFlag);
	if (taomee::test_bit_on(flag, pos)) {
		return cli_err_prize_already_get;
	}
	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_VIP_USER_GET_YAYA_FRAGMENT,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}
	flag = taomee::set_bit_on(flag, pos);
	SET_A(kAttrVipGetYayaFragmentFlag, flag);
	return 0;
}

int proc_charge_diamond_get_gift_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti)
{
    uint32_t base_prize_id = 2151;
    uint32_t required_diamonds[] = {
        150,300,500,1000,3000
    };
    if (prize_id < base_prize_id || prize_id > base_prize_id + 4) {
        return cli_err_prize_id_invalid;
    }
    uint32_t prize_flag = GET_A(kAttrChargeDiamondGetGiftPrizeFlag);
    uint32_t idx = prize_id - base_prize_id + 1;
    if (taomee::test_bit_on(prize_flag, idx)) {
        return cli_err_prize_already_get;
    }

    if (GET_A(kAttrChargeDiamondGetGiftChargeCnt) < required_diamonds[idx - 1]) {
        return cli_err_can_not_get_prize_yet;
    }
	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_CHARGE_DIAMOND_GET_GIFT,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}
	prize_flag = taomee::set_bit_on(prize_flag, idx);
	SET_A(kAttrChargeDiamondGetGiftPrizeFlag, prize_flag);
    Utils::write_msglog_new(player->userid, "充值", "充值钻石礼", 
            "成功领取" + Utils::to_string(required_diamonds[idx-1]) + "钻石礼包");
    return 0;
}

int proc_summer_weekly_signed(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti)
{
	//判断签到时间是否在总的活动时间范围内
	if (!TimeUtils::is_current_time_valid(TM_CONF_KEY_SUMMER_WEEKLY_SIGN, 0)) {
		return cli_err_activity_time_invalid;
	}

	uint32_t key = TM_CONF_KEY_SUMMER_WEEKLY_SIGN;
	if (g_time_config.count(key) == 0) {
		return cli_err_sys_err;
	}
	uint32_t prize_index[] = {8711, 8721, 8731, 8741};
	TIME_CONFIG_LIMIT_T& time_config_map = g_time_config.find(key)->second;
	if (time_config_map.size() != array_elem_num(prize_index)) {
		return cli_err_summer_signed_time_cofing_err;
	}
	uint32_t index = 0;
	bool found_valid_time = false;
	for (uint32_t i = 1; i <= time_config_map.size(); ++i) {
		if (TimeUtils::is_time_valid(NOW(), key, i)) {
			index = i;
			found_valid_time = true;
			break;
		}
	}
	uint32_t has_open_days = 0;
	if (found_valid_time) {
		time_limit_t& time_info = time_config_map[index];
		//计算本活动已经开启的天数
		has_open_days = TimeUtils::get_days_between(time_info.start_time, NOW()) + 1;
		TRACE_TLOG("Summer Weekly Signed,open_days=[%u],index=[%u],start_time=%u", has_open_days, index, time_info.start_time);
	}


	uint32_t has_signed_total_times= GET_A(kAttrSummerHasSignedTotalTimes);
	if (!(has_signed_total_times >= 0 && has_signed_total_times <= 6)) {
		return cli_err_summer_signed_this_round_fin;
	}
	if (has_open_days <= has_signed_total_times) {
		return cli_err_summer_no_need_signed;
	}
	uint32_t free_times = GET_A(kDailySummerWeeklySignedTimes);
	uint32_t buy_cnt = GET_A(kDailySummerWeeklyBuySignCnt);
	if (free_times == 0 && buy_cnt == 0) {
		return cli_err_summer_signed_cnt_has_used;
	}
	if (free_times && buy_cnt == 0) {
		SUB_A(kDailySummerWeeklySignedTimes, 1);
	} else if (free_times == 0 && buy_cnt) {
		SUB_A(kDailySummerWeeklyBuySignCnt, 1);
	} else {
		return cli_err_summer_signed_meet_unknow_mistake;
	}
	//计算应该获得奖励的索引
	//用本轮活动已经签到的次数，来作为
	prize_id = prize_index[index - 1] + has_signed_total_times;
	int ret = transaction_proc_prize(player, prize_id, noti,
			commonproto::PRIZE_REASON_SUMMER_WEEKLY_SIGNED,
			onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}
	ADD_A(kAttrSummerHasSignedTotalTimes, 1);
	//设置本次签到的时间戳
	SET_A(kAttrSummerLastSignTimestamp, NOW());
	return 0;
}


//=================================================================
int StarLotteryCmdProcessor::proc_pkg_from_client(player_t *player,
	   	const char *body, int bodylen) 
{
	PARSE_MSG;
	// uint32_t number = cli_in_.number();
	uint32_t prize_flag = GET_A(kDailyStarLotteryPurchaseFlag);
	// if(taomee::test_bit_on(prize_flag, number)){
	if(prize_flag != 0){
		return send_err_to_player(
				player, player->cli_wait_cmd, cli_err_repeat_get_star_lottery_prize);
	}
	//去拉数据
	return RankUtils::get_star_lottery_info_from_redis(player);
}

int StarLotteryCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_get_field_info_out_);
	uint32_t number = cli_in_.number();
	uint32_t prize_flag = GET_A(kDailyStarLotteryPurchaseFlag);
	if(prize_flag != 0){
		return send_err_to_player(
				player, player->cli_wait_cmd, cli_err_repeat_get_star_lottery_prize);
	}

    if (rank_get_field_info_out_.fields_size() == 0) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

	commonproto::star_lottery_elem_list elem_list;
	elem_list.Clear();
	string pkg = rank_get_field_info_out_.fields(0).value();

	std::string name = elem_list.GetTypeName();
	if(!elem_list.ParseFromString(pkg)){
		std::string errstr = elem_list.InitializationErrorString();
		ERROR_TLOG("PARSE MSG '%s' failed, err = '%s'", 
				name.c_str(), errstr.c_str());
	}

	uint32_t price_type = 0;
	uint32_t price      = 0;
	std::vector<cache_prize_elem_t> award_elems;
	commonproto::prize_elem_t elem;
	cache_prize_elem_t tmp;
	award_elems.clear();
	elem.Clear();
	for(int i = 0; i < elem_list.elem_size(); i++){
		tmp.clear();
		if(number == elem_list.elem(i).number()){
			elem.CopyFrom(elem_list.elem(i).elem());
			tmp.type         = (uint32_t)elem.type();
			tmp.id           = elem.id();
			tmp.count        = elem.count();
			tmp.level        = elem.level();
			tmp.talent_level = elem.talent_level();
			tmp.show         = elem.show_type();
			price_type       = elem_list.elem(i).price_type();
			price            = elem_list.elem(i).price();

			award_elems.push_back(tmp);
			break;
		}
	}
	if(award_elems.empty()){
		ERROR_TLOG("star lottry info is NULL userid = %u create_tm = %u'", 
				player->userid, player->create_tm);
	}

	int ret = 0;
	//扣招募券
	if(0 == price_type){
		const uint32_t item_id = 33001;	
		ret = reduce_single_item(player, item_id, price);
	} else if (1 == price_type){//扣砖石
		const uint32_t product_id = 90067;	
		attr_type_t attr_type = kServiceBuyStarLotteryPrize;	
		ret = buy_attr_and_use(player, attr_type, product_id, price);
	} else {
		ERROR_TLOG("star lottry price_type err type = %u userid = %u create_tm = %u'", 
				price_type, player->userid, player->create_tm);
		return 0;
	}

	if(ret){
		return send_err_to_player(
				player, player->cli_wait_cmd, ret);
	}
	
	//标记获得记录
	SET_A(kDailyStarLotteryPurchaseFlag, 1);

    onlineproto::sc_0x0112_notify_get_prize noti;
	ret = transaction_proc_packed_prize(player, award_elems, &noti,
           commonproto::PRIZE_REASON_NO_REASON, "明星招募");
    noti.set_reason(commonproto::PRIZE_REASON_STAR_LOTTERY);


	AttrUtils::add_attr_in_special_time_range(player,
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivStarLottery);

	cli_out_.Clear();
	send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti);
	send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, cli_out_);

    Utils::write_msglog_count(player->userid, "招募", "明星招募", "消耗招募令", price);
    return 0;
}

int GetStarLotteryInfoCmdProcessor::proc_pkg_from_client(player_t *player,
	   	const char *body, int bodylen) 
{
	PARSE_MSG;
	//numberid，pirzeid
#if 0
	const uint32_t prize_id[][2] = {
		{1, 240},
		{2, 241},
		{3, 242}
	};
#endif
	const uint32_t prize_id = 240;

	uint32_t refresh = GET_A(kDailyStarLotteryRefresh);
	// uint32_t size = array_elem_num(prize_id);    
	//去刷新
	if(0 == refresh){
		std::vector<cache_prize_elem_t> temp_vec;
		commonproto::star_lottery_elem_list elem_list;
		elem_list.Clear();
		//刷新出新的prize
		// for(uint32_t i = 0; i < size; i++){
		temp_vec.clear();
		transaction_pack_prize_except_item(player,
				prize_id, temp_vec, 0, NO_ADDICT_DETEC);
		// transaction_pack_prize_except_item(player,
		// prize_id[i][1], temp_vec, 0, NO_ADDICT_DETEC);

		uint32_t num = 1;
		FOREACH(temp_vec, it){
			const cache_prize_elem_t &elem = *it;
			commonproto:: star_lottery_elem_t *inf = elem_list.add_elem();
			inf->set_number(num);
			inf->set_price_type(elem.price_type);
			inf->set_price(elem.price);     
			inf->mutable_elem()->set_type((commonproto::prize_elem_type_t) elem.type);
			inf->mutable_elem()->set_id(elem.id);
			inf->mutable_elem()->set_count(elem.count);
			inf->mutable_elem()->set_level(elem.level);
			inf->mutable_elem()->set_talent_level(elem.talent_level);
			inf->mutable_elem()->set_show_type(elem.show);
			num++;
		}
		// }
		//更新redis
		int ret = RankUtils::update_star_lottery_info_to_redis(player, elem_list);
		if(ret){
			return send_err_to_player(
					player, player->cli_wait_cmd, ret);
		}
		//设置已刷新
		SET_A(kDailyStarLotteryRefresh, 1);
		cli_out_.Clear();
		cli_out_.mutable_elem_list()->CopyFrom(elem_list);
		send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	} else {
		//去redis拉信息
		return RankUtils::get_star_lottery_info_from_redis(player);
	}
	return 0;
}

int GetStarLotteryInfoCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_get_field_info_out_);

    if (rank_get_field_info_out_.fields_size() == 0) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

	commonproto::star_lottery_elem_list elem_list;
	elem_list.Clear();
	string pkg = rank_get_field_info_out_.fields(0).value();

	std::string name = elem_list.GetTypeName();
	if(!elem_list.ParseFromString(pkg)){
		std::string errstr = elem_list.InitializationErrorString();
		ERROR_TLOG("PARSE MSG '%s' failed, err = '%s'", 
				name.c_str(), errstr.c_str());
	}

	cli_out_.Clear();
	cli_out_.mutable_elem_list()->CopyFrom(elem_list);
	send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    return 0;
}
