#include "player.h"
#include "swim_processor.h"
#include "proto/client/pb0x08.pb.h"
#include "proto/client/cli_cmd.h"
#include "proto/client/cli_errno.h"
#include "item.h"
#include "map_utils.h"
#include "pet_utils.h"
#include "player_utils.h"
#include "global_data.h"
#include "swim.h"
#include "timer_procs.h"
#include "task_utils.h"

int StartSwimCmdProcessor::proc_pkg_from_client(
		player_t* player, const char *body, int bodylen)
{
	cli_in_.Clear();
	PARSE_MSG;

	bool has_luxury_suit = GET_A(kDailySwimHasLuxurySuit);
	bool has_suit= GET_A(kDailySwimHasSuit);
	//如果没有泳衣，报错
	if ((!has_luxury_suit) && (!has_suit)){
		return send_err_to_player(player, player->cli_wait_cmd, 
				cli_err_no_swim_suit);
	}
	int time_left = get_swim_left_time(player);
	//剩余时间为0，报错
	if (time_left == 0) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_no_swim_time);
	}
	bool is_start = GET_A(kDailySwimIsStart);
	if(is_start){
		//回包
		cli_out_.set_left_time(time_left);
		return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	}
	SET_A(kDailySwimIsStart, 1);
	//通知其他玩家自己开始游泳	
	notify_start_swim_to_map(player);
    player->temp_info.cur_swim_exp = 0;
    player->temp_info.total_swim_exp = 0;

	//置游泳时间戳
	AttrUtils::SET_A(kAttrSwimLastTime, NOW());

	//拉排行榜信息
	onlineproto::sc_0x082D_inform_dive_rank inform_dive_rank;
	for (uint32_t i = 0; i < g_dive_rank.get_size(); ++i) {
		onlineproto::diver_rank_info_t* rank_info = inform_dive_rank.add_rank_info();
		std::string nick;
		uint32_t ret = g_dive_rank.get_ith_nick(i, nick);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
		rank_info->set_nick(nick);
		rank_info->set_score(g_dive_rank.get_ith_score(i));
	}
	uint32_t self_score = GET_A(kDailyDiveSelfScore);	
	inform_dive_rank.set_self_score(self_score);
	//活动范围内记录游泳次数
	AttrUtils::add_attr_in_special_time_range(player,
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivSwimCnt);

	send_msg_to_player(player, cli_cmd_cs_0x082D_inform_dive_rank, inform_dive_rank);
	//回包
	cli_out_.set_left_time(time_left);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

void notify_start_swim_to_map(player_t* player)
{
	//NOW DO NOTHING
	return;
}

uint32_t get_swim_left_time(player_t* player)
{
	//uint32_t last_swim_time = 0;
	int time_left = 0;
	uint32_t swim_last_time = GET_A(kAttrSwimLastTime);
	int regular_left = 0;
	int extended_left = 0;

	if (swim_last_time == 0 || !TimeUtils::is_same_day(swim_last_time, NOW())) {
		//本日还没有游泳过
		SET_A(kDailyHasSwim, 1);
		if (is_vip(player)) {
			regular_left = commonproto::VIP_SWIM_TM;
			SET_A(kDailySwimRegularLeftTime, commonproto::VIP_SWIM_TM);
		}
		else {
			regular_left =  commonproto::REGULAR_SWIM_TM;
			SET_A(kDailySwimRegularLeftTime, commonproto::REGULAR_SWIM_TM);
		}

		extended_left = GET_A(kDailySwimExtendedLeftTime);
		time_left = regular_left + extended_left;
		return time_left;
	} else {
		//获取常规游泳时间
		regular_left = GET_A(kDailySwimRegularLeftTime);
		//获取延长时间

		extended_left = GET_A(kDailySwimExtendedLeftTime);
		time_left = regular_left + extended_left;
		return time_left;
	}
}

int SwimGetExpCmdProcessor::proc_pkg_from_client(
		player_t* player, const char *body, int bodylen)
{
	cli_in_.Clear();
	cli_out_.Clear();
	PARSE_MSG;
	uint32_t &freq_times = player->temp_info.swim_get_exp_freq_times;

	int time_left = get_swim_left_time(player); // 剩余时间
	int regular_left = GET_A(kDailySwimRegularLeftTime);
	int extended_left = GET_A(kDailySwimExtendedLeftTime);

	//剩余时间为0报错
	if (time_left <= 0) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_no_swim_time_left);
	}

	bool is_start = GET_A(kDailySwimIsStart);
	//游泳还没开始报错
	if (is_start == 0) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_swim_not_start);
	}

	//uint32_t last_submit = GET_A(kAttrSwimLastTime);
	uint32_t last_submit = GET_A(kAttrSwimLastExpTime);

	//如果两次收到该协议的时间间隔过短，就累计一次频繁操作
	if (last_submit >= NOW() 
		|| (NOW() - last_submit < SWIM_GET_EXP_INTERVAL - 1 
		&& time_left >= SWIM_GET_EXP_INTERVAL)) {
		freq_times++;
	} else {
		freq_times = 0;
	}
	//连续频繁操作超过限定，报错
	if (freq_times >= SWIM_GET_EXP_FREQ_ALLOW){
		freq_times = 0;
		// return send_err_to_player(player, player->cli_wait_cmd,
				// cli_err_swim_get_exp_too_fast);
				//前端不好控制时间，为了避免报错，直接return
		return 0;
	}

	int reduce_time = 0;
	reduce_time = SWIM_GET_EXP_INTERVAL;
	//设置剩余时间
	if (regular_left >= reduce_time) {
		SET_A(kDailySwimRegularLeftTime, regular_left - reduce_time);
	} else {
		SET_A(kDailySwimRegularLeftTime, 0);
		SET_A(kDailySwimExtendedLeftTime, extended_left - (reduce_time - regular_left));
	}

    std::vector<Pet*> fight_pets;
    PetUtils::get_fight_pets(player, fight_pets);
    
	bool has_luxury_suit = GET_A(kDailySwimHasLuxurySuit);
	//精灵获得经验
    FOREACH(fight_pets, it) {
        Pet *pet = *it;
		uint32_t real_exp = 0;
        uint32_t pet_get_exp = pet->level() * 17 * (has_luxury_suit ?2 :1);
        PetUtils::add_pet_exp(player, pet, pet_get_exp, real_exp);
        pet->total_swim_exp += real_exp;
        pet->cur_swim_exp = real_exp;
    }
	//玩家获得的经验
    uint32_t real_player_exp = 0;
	uint32_t player_get_exp = GET_A(kAttrLv) * 17 * (has_luxury_suit ?2 :1);
    PlayerUtils::add_player_exp(player, player_get_exp, &real_player_exp);
    player->temp_info.cur_swim_exp = real_player_exp;
    player->temp_info.total_swim_exp += real_player_exp;

	//如果时间耗尽
	regular_left = GET_A(kDailySwimRegularLeftTime);
	extended_left = GET_A(kDailySwimExtendedLeftTime);
	if (regular_left <= 0 && extended_left <= 0) {
		SET_A(kDailySwimIsStart, 0);
		onlineproto::sc_0x0826_inform_end_swim inform_end_swim;
		inform_end_swim.set_uid(player->userid);
		inform_end_swim.set_player_exp(player->temp_info.total_swim_exp);
        FOREACH(fight_pets, it) {
            Pet *pet = *it;
            onlineproto::pet_swim_exp_t *pet_exp = inform_end_swim.add_pet_exps();
            pet_exp->set_pet_create_tm(pet->create_tm());
            pet_exp->set_pet_exp(pet->cur_swim_exp);
            pet_exp->set_pet_total_exp(pet->total_swim_exp);
        }
		send_msg_to_player(player, player->cli_wait_cmd, inform_end_swim);
        Utils::write_msglog_new(player->userid, "功能", "游泳", "成功完成游泳");
	}

    //回包
    FOREACH(fight_pets, it) {
        Pet *pet = *it;
        onlineproto::pet_swim_exp_t *pet_exp = cli_out_.add_pet_exps();
        pet_exp->set_pet_create_tm(pet->create_tm());
        pet_exp->set_pet_exp(pet->cur_swim_exp);
        pet_exp->set_pet_total_exp(pet->total_swim_exp);
    }

        // 更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_DIVING, 1);

	cli_out_.set_player_exp(player->temp_info.cur_swim_exp);
	cli_out_.set_player_total_exp(player->temp_info.total_swim_exp);
	cli_out_.set_left_time(regular_left + extended_left);
	//重设经验获取时间
	SET_A(kAttrSwimLastExpTime, NOW());

	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int SwimPauseCmdProcessor::proc_pkg_from_client(
		player_t* player, const char *body, int bodylen)
{
	cli_in_.Clear();
	cli_out_.Clear();

	PARSE_MSG;
	
	notify_pause_swim_to_map(player);
	bool is_start = GET_A(kDailyDiveIsStart);
	//如果正在跳水队列中，就移出队列，并且把跳水状态改为没有开始
	if (is_start == true) {
		g_dive.remove_diver_by_userid(player->userid);
		SET_A(kDailyDiveIsStart, 0);
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);

}

void notify_pause_swim_to_map(player_t* player)
{

	onlineproto::sc_0x0829_inform_pause_swim inform_pause_swim_;
	//讲游泳状态设置为不在游泳
	SET_A(kDailySwimIsStart, 0);
	if (!MapUtils::is_player_in_map(player)) {
		//不在度假场景，打错误信息 
		ERROR_TLOG("player:%u change vocation status"
				" but he/she is not int the vocation scen", player->userid);      
	} else {
		//通知地图玩家，自己已经不在游泳了
		inform_pause_swim_.set_uid(player->userid);
		inform_pause_swim_.set_luxury_suit(false);

		bool has_luxury_suit = GET_A(kDailySwimHasLuxurySuit);
		if (has_luxury_suit) {
			inform_pause_swim_.set_luxury_suit(true);
		}
		else {
			inform_pause_swim_.set_luxury_suit(false);
		}


		MapUtils::send_msg_to_map_users_except_self(player,       
				cli_cmd_cs_0x0829_inform_pause_swim, inform_pause_swim_);
	}
}

int UseChairCmdProcessor::proc_pkg_from_client(
		player_t* player, const char *body, int bodylen)
{
	PARSE_MSG;


	bool has_chair = GET_A(kDailySwimHasChair);

	if (has_chair == false) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_no_swim_chair);
	}
	
	// bool is_swimming = GET_A(kDailySwimIsStart);
	// if (is_swimming == false) {
		// return send_err_to_player(player, player->cli_wait_cmd, cli_err_swim_not_start);
	// }
	

	cli_out_.set_uid(player->userid);
	if (!MapUtils::is_player_in_map(player)) {
		send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	} else {
		MapUtils::send_msg_to_map_users(player,
				cli_cmd_cs_0x0827_use_chair, cli_out_);
	}

	return 0;
}

int WithdrawChairCmdProcessor::proc_pkg_from_client(
		player_t* player, const char *body, int bodylen)
{
	PARSE_MSG;

	cli_out_.set_uid(player->userid);
	if (!MapUtils::is_player_in_map(player)) {
		send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	} else {
		MapUtils::send_msg_to_map_users(player,
				cli_cmd_cs_0x0827_use_chair, cli_out_);
	}

	return 0;
}

int ApplyDiveCmdProcessor::proc_pkg_from_client(
		player_t* player, const char *body, int bodylen)
{
	PARSE_MSG;
	diver_t last_diver;
	diver_t diver;
	diver.userid = player->userid;

	//每天只能游泳5次
	uint32_t dive_times = GET_A(kDailyDiveTimes);
	if (dive_times >= commonproto::DIVING_DAILY_MAX) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_no_dive_times);
	}

    //CD一分钟
    if (GET_A(kDailyLastDiveTime) + commonproto::DIVING_COOLDOWN > NOW()) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_in_cd_time);
    }

	//获取上个申请跳水的人将要跳水的时间
	uint32_t ret = g_dive.get_last_diver(last_diver);
	//如果没有人在排队，就把跳水时间设置为现在
	if (ret == cli_err_no_diver_in_list) {
		diver.dive_time = NOW();
	} else {
        //如果队列中有人，把将要跳水时间设定为最后跳水的人跳水时间后的10秒
		diver.dive_time = last_diver.dive_time + 10;
	}
	//将该人加在跳水队列的最后(内部有判断是否已经在跳水了)
	ret = g_dive.add_diver_to_end(player, diver);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

    player->dive_timer = ADD_TIMER_EVENT_EX(
            player,
            kTimerTypeClearDive,
            (void*)(player->userid),
            diver.dive_time + 10
            );

	cli_out_.set_userid(player->userid);
	cli_out_.set_dive_time(diver.dive_time - NOW());
	SET_A(kDailyDiveIsStart, 1);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);

}

int PrepareDiveCmdProcessor::proc_pkg_from_client(
		player_t* player, const char *body, int bodylen)
{
	PARSE_MSG;
	diver_t diver;
	//取队头的玩家跳水信息
	uint32_t ret = g_dive.get_first_diver(diver);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	//判断当前玩家是否是队头玩家
	if (player->userid != diver.userid) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_invalid_dive_player);
	}
	
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	cli_out_.set_userid(player->userid);
	return MapUtils::send_msg_to_map_users(player,       
			cli_cmd_cs_0x082B_prepare_dive, cli_out_);
}

int StartDiveCmdProcessor::proc_pkg_from_client(
		player_t* player, const char *body, int bodylen)
{
	PARSE_MSG;
    diver_t diver;
    //取队头的玩家跳水信息
    uint32_t ret = g_dive.get_first_diver(diver);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }
	//判断当前玩家是否是队头玩家
	if (player->userid != diver.userid) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_invalid_dive_player);
	}

	uint32_t score = cli_in_.score();
	//跳水加经验
	uint32_t level = GET_A(kAttrLv);
	uint32_t dive_exp = score * level * 2; 
	uint32_t real_exp = 0;
	PlayerUtils::add_player_exp(player, dive_exp, &real_exp);
    player->temp_info.dive_exp = real_exp;

	uint32_t self_score = GET_A(kDailyDiveSelfScore);	
	uint32_t total_score = score + self_score;
	//取自己的分数
	SET_A(kDailyDiveSelfScore, total_score);	
	//如果自己分数比榜上最低分低，则加入到排行榜中
	uint32_t min_score = g_dive_rank.get_minimum_score_in_rank();
	if (total_score > min_score || g_dive_rank.get_size() < 5) {
		diver_score_info_t diver_score_info;
		diver_score_info.userid = player->userid;
		diver_score_info.score = total_score;
		diver_score_info.nick = player->nick;
		g_dive_rank.insert_diver_and_sort(diver_score_info);
	}

	//拉排行榜信息
	onlineproto::sc_0x082D_inform_dive_rank inform_dive_rank;
	for (uint32_t i = 0; i < g_dive_rank.get_size(); ++i) {
		onlineproto::diver_rank_info_t* rank_info = inform_dive_rank.add_rank_info();

		std::string nick;
		uint32_t ret = g_dive_rank.get_ith_nick(i, nick);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
		rank_info->set_nick(nick);
		rank_info->set_score(g_dive_rank.get_ith_score(i));
	}
	//保存自己获得的总分数
	inform_dive_rank.set_self_score(total_score);

	send_msg_to_player(player, cli_cmd_cs_0x082D_inform_dive_rank, inform_dive_rank);
	cli_out_.set_userid(player->userid);
	cli_out_.set_score(score);
	if (score > GET_A(kAttrSingleDiveScore)) {
		SET_A(kAttrSingleDiveScore, score);
	}

    //结束跳水
	//增加已经跳水次数，将该玩家从跳水队列删除
	ADD_A(kDailyDiveTimes, 1);
	ret = g_dive.remove_front_diver();
	SET_A(kDailyDiveIsStart, 0);
    SET_A(kDailyLastDiveTime, NOW());
    if (player->dive_timer) {
        REMOVE_TIMER(player->dive_timer);
    }

	//广播给地图上的人
	return MapUtils::send_msg_to_map_users(player,
			cli_cmd_cs_0x082C_start_dive, cli_out_);

}

int InformDiveRankCmdProcessor::proc_pkg_from_client(
		player_t* player, const char *body, int bodylen)
{
	PARSE_MSG;

	//拉排行榜信息
	onlineproto::sc_0x082D_inform_dive_rank inform_dive_rank;
	for (uint32_t i = 0; i < g_dive_rank.get_size(); ++i) {
		onlineproto::diver_rank_info_t* rank_info = inform_dive_rank.add_rank_info();
		std::string nick;
		uint32_t ret = g_dive_rank.get_ith_nick(i, nick);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
		rank_info->set_nick(nick);
		rank_info->set_score(g_dive_rank.get_ith_score(i));
	}
	uint32_t self_score = GET_A(kDailyDiveSelfScore);	
	//读自己的分数
	inform_dive_rank.set_self_score(self_score);
	return send_msg_to_player(player, cli_cmd_cs_0x082D_inform_dive_rank, inform_dive_rank);
}
