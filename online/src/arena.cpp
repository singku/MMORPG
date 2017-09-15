#include <boost/lexical_cast.hpp>
#include "player_utils.h"
#include "arena.h"
#include "macro_utils.h"
#include "player.h"
#include "service.h"
#include "global_data.h"
#include "common.h"
#include "arena_conf.h"
#include "prize.h"

uint32_t ArenaUtils::check_challenge_arena_condition(player_t* player)
{
	//检查玩家cd时间
	uint32_t last_time = GET_A(kDailyArenaLastChallengeEndTime);
	if (last_time && (last_time + ARENA_FIGHT_CD) > NOW()) {
		ERROR_TLOG("uid=[%u],still in cd; last_time=[%u],now=[%u]", 
				player->userid, last_time, NOW());
		return cli_err_in_cd_time;
	}
	//检查玩家是否还有挑战次数
	if (ARENA_FREE_CHALLENGE_CNT + GET_A(kDailyArenaBuyChallengeTimes) <= 
			GET_A(kDailyArenaChallengeTimes)) {
		ERROR_TLOG("uid=[%u],challenge times get limit:buy_times=[%u],"
				"cur_challenge_time=[%u]", 
				player->userid, GET_A(kDailyArenaBuyChallengeTimes), 
				GET_A(kDailyArenaChallengeTimes));
		return cli_err_arena_has_no_challenge_times;
	}
	
	return 0;
}

//获取上次周五的日期作为sub_key: (年月日)；例如：20141031
//如果今天是周五，获取的就是今天的时间日期
//如果今天是周六，获取的就是昨天的时间日期
//如果今天是周四，获取的就是上周五的时间日期
uint32_t ArenaUtils::get_last_week_arena_rank_sub_key(uint32_t& sub_key)
{
	sub_key = TimeUtils::get_prev_friday_date();
	return 0;
}

//获取下次周五的日期作为sub_key
uint32_t ArenaUtils::get_next_week_arena_rank_sub_key(uint32_t& sub_key)
{
	time_t now = get_now_tv()->tv_sec;
	time_t next_time = TimeUtils::get_next_x_time(now, 5);
	sub_key = TimeUtils::time_to_date(next_time);
	return 0;
}

//竞技场修改玩家战斗属性值
uint32_t ArenaUtils::arena_modify_player_btl_attr(
		commonproto::battle_player_data_t& battle_all_info)
{
	//uint32_t arena_hp = 0;
	const uint32_t COF = 20;
	uint32_t max_hp = battle_all_info.battle_info().max_hp() * COF;
	battle_all_info.mutable_battle_info()->set_cur_hp(max_hp);
	battle_all_info.mutable_battle_info()->set_max_hp(max_hp);

	uint32_t pet_list_size = battle_all_info.pet_list().pet_list_size();
	for (uint32_t i = 0; i < pet_list_size; ++i) {
		uint32_t pet_max_hp = battle_all_info.pet_list().pet_list(i).pet_info().battle_info().max_hp() * COF;
		battle_all_info.mutable_pet_list()->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_max_hp(pet_max_hp);
		battle_all_info.mutable_pet_list()->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_cur_hp(pet_max_hp);
	}
	
	/*
	uint32_t btl_power = battle_all_info.base_info().power();
	if (ARENA_COF1 > btl_power) {
		const double ARENA_BTL_PARA = (ARENA_COF1 - btl_power) / ARENA_COF2;
		arena_hp = ARENA_BTL_PARA * battle_all_info.battle_info().max_hp();
		uint32_t max_hp = battle_all_info.battle_info().max_hp();
		//如果公式计算出的血量 arena_hp 大于 max_hp 则，修改最大血量
		if (arena_hp > max_hp) {
			max_hp = arena_hp;
			battle_all_info.mutable_battle_info()->set_max_hp(max_hp);
		}
		battle_all_info.mutable_battle_info()->set_cur_hp(arena_hp);
	}
	uint32_t pet_list_size = battle_all_info.pet_list().pet_list_size();
	for (uint32_t i = 0; i < pet_list_size; ++i) {
		uint32_t pet_max_hp = battle_all_info.pet_list().pet_list(i).pet_info().battle_info().max_hp();
		uint32_t pet_power = battle_all_info.pet_list().pet_list(i).pet_info().base_info().power();
		const double PET_BTL_PARA = (ARENA_COF1 - pet_power) / ARENA_COF2;
		uint32_t pet_cur_hp = 0;
		if (ARENA_COF1 > pet_power) {
			pet_cur_hp = PET_BTL_PARA * pet_max_hp;
			if (pet_cur_hp > pet_max_hp) {
				pet_max_hp = pet_cur_hp;
				battle_all_info.mutable_pet_list()->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_max_hp(pet_max_hp);
			}
			battle_all_info.mutable_pet_list()->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_cur_hp(pet_cur_hp);
		}
	}
	*/
	return 0;
}

int ArenaUtils::arena_push_streak_end(player_t* player)
{
	onlineproto::sc_0x0222_arena_killing_stream_end msg_out_;
	role_info_t ai_role_info = KEY_ROLE(player->temp_info.ai_id);
	msg_out_.set_atk_userid(ai_role_info.userid);
	msg_out_.set_atk_create_tm(ai_role_info.u_create_tm);
	msg_out_.set_atk_nick(*player->temp_info.ai_nick);
	msg_out_.set_def_userid(player->userid);
	msg_out_.set_def_create_tm(player->create_tm);
	msg_out_.set_def_nick(player->nick);
	uint16_t cmd = cli_cmd_cs_0x0222_arena_killing_stream_end;
	Utils::switch_transmit_msg(
			switchproto::SWITCH_TRANSMIT_WORLD, cmd,
			msg_out_);
	return 0;
}

int ArenaUtils::arena_push_killing_spree(
		player_t* player, uint32_t win_streak_count)
{
	onlineproto::sc_0x0220_arena_push_killing_spree msg_out_;
	msg_out_.set_userid(player->userid);
	msg_out_.set_create_tm(player->create_tm);
	std::string nick(player->nick);
	msg_out_.set_nick(nick);
	msg_out_.set_stream_kill(win_streak_count);
	uint16_t cmd = cli_cmd_cs_0x0220_arena_push_killing_spree;
	Utils::switch_transmit_msg(
			switchproto::SWITCH_TRANSMIT_WORLD, cmd,
			msg_out_);
	return 0;
}

int ArenaUtils::win_streak_get_reward(
		player_t* player, uint32_t win_streak_count)
{
	assert(player);
	streak_info_t* streak_ptr = g_arena_streak_reward_conf_mgr.get_streak_conf_info(win_streak_count);
	if (streak_ptr == NULL) {
		ERROR_TLOG("win streak count err:value=[%u]", win_streak_count);
		return 0;
	}
	onlineproto::sc_0x0112_notify_get_prize noti_prize_msg;
	transaction_proc_prize(player, streak_ptr->prize_id, 
			noti_prize_msg, commonproto::PRIZE_REASON_ARENA_GIFT);
	send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_prize_msg);
	return 0;
}

//竞技场胜利，获得本次奖励
int ArenaUtils::get_reward_after_challenge(
		player_t* player, uint32_t new_rank)
{
	assert(player);
	rank_reward_t* reward_ptr = g_arena_rank_reward_conf_mgr.get_arena_rank_reward(new_rank);
	if (reward_ptr == NULL) {
		ERROR_TLOG("New Rank Not Found;new_rank:[%u]", new_rank);
		return cli_err_rank_not_fount_in_table;
	}
	onlineproto::sc_0x0112_notify_get_prize noti_prize_msg;
	int ret = transaction_proc_prize(
			player, reward_ptr->single_reward, 
			noti_prize_msg, commonproto::PRIZE_REASON_ARENA_GIFT);
	if (ret) {
		return ret;
	}
	send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_prize_msg);
	return 0;
}

int ArenaUtils::proc_arena_rank_prize(player_t *player, rankproto::sc_get_user_multi_rank &rank_out)
{
	uint32_t rank_info_size = rank_out.rank_info_size();
	for (uint32_t i = 0; i < rank_info_size; ++i) {
		const commonproto::rank_player_info_t& inf = rank_out.rank_info(i);
		uint32_t daily_rank = inf.score();
		if (0 == daily_rank) {
			continue;
		}
		std::string str_rank_key = inf.rank_key();
		std::vector<std::string> keys_vec = split(str_rank_key, ':');
		uint32_t key = 0, daily_key = 0;
		if (keys_vec.size() >= 2) {
			key = boost::lexical_cast<uint32_t>(keys_vec[0]);
			daily_key = boost::lexical_cast<uint32_t>(keys_vec[1]);
		} else {
            continue;
        }
        bool today_is_sunday = false;
        uint32_t last_sunday_time = TimeUtils::get_last_x_time(NOW(), 0);
        if (TimeUtils::time_to_date(last_sunday_time) == daily_key) {
            today_is_sunday = true;
        }
        std::string mail_content;
        std::string mail_title;
        uint32_t prize_id = 0;
        if (key == commonproto::RANKING_ARENA) {
            rank_reward_t *reward_ptr = g_arena_rank_reward_conf_mgr.get_arena_rank_reward(
                    daily_rank);
            if (reward_ptr == NULL) {
                ERROR_TLOG("daily_rank not found in arenas.xml daily_rank=[%u]", 
                        daily_rank);
                continue;
            }
            mail_content = "由于你在:" + boost::lexical_cast<std::string>(daily_key) + 
                "当日排位赛中的出色表现，达到了第" + boost::lexical_cast<std::string>(daily_rank) +
                "名，因此引导者决定给你发放以下奖励";
            mail_title = "获得了排位赛日排名奖励";
            prize_id = reward_ptr->bonus_id;

        } else if (key == commonproto::RANKING_RPVP){
            if (today_is_sunday) {//周排名奖励
                mail_content = "由于你在:" + boost::lexical_cast<std::string>(daily_key) + 
                    "本周竞技场中的出色表现, 排名为" + boost::lexical_cast<std::string>(inf.rank()) +
                    ", 因此引导者决定给你发放以下奖励";
                mail_title = "获得了竞技场排名奖励";
                prize_id = g_rpvp_reward_conf_mgr.find_rpvp_prize_id_by_rank(inf.rank());
                PlayerUtils::generate_new_mail(player, mail_title, mail_content, prize_id);
                SET_A(kAttrRpvpScore, 1500);
            }
            mail_content = "由于你在:" + boost::lexical_cast<std::string>(daily_key) + 
                "当日竞技场中的出色表现, 引导者决定给你发放以下奖励";
            mail_title = "获得了竞技场段位奖励";
            prize_id = g_rpvp_reward_conf_mgr.find_rpvp_prize_id_by_score(inf.score());
        }
        PlayerUtils::generate_new_mail(player, mail_title, mail_content, prize_id);

	}

    SET_A(kAttrArenaLastGetRewardTm, NOW());
    return 0;
}
