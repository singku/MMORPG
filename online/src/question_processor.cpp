#include "question_processor.h"
#include "player_utils.h"
#include <boost/lexical_cast.hpp>
#include "prize.h"

void GetQuestionInfoCmdProcessor::register_question_type()
{
	question_func_map_[(uint32_t)commonproto::QUESTION_MARY] = proc_require_mary_qusetion;
	question_func_map_[(uint32_t)commonproto::QUESTION_DAILY] = proc_require_daily_qusetion;
}

int GetQuestionInfoCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t max_cnt = QuestionUtils::get_question_max_count();
	if(cli_in_.count() > max_cnt){
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_require_question_count_over_limit);
	}
	int ret = 0;
	cli_out_.Clear();
	cli_out_.set_type(cli_in_.type());

    if (question_func_map_.count((uint32_t)cli_in_.type()) == 0) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_question_type_invalid);
    }
	question_proc_func_t fun;
    fun = (question_func_map_.find((uint32_t)cli_in_.type()))->second;
    ret = fun(player, cli_in_.count(), cli_in_.rule(), cli_out_);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

//每日答题
int proc_require_daily_qusetion(player_t *player, uint32_t count,
	   	uint32_t rule, onlineproto::sc_0x0612_get_question_info& cli_out_)
{
	std::vector<uint32_t> tmp_vec;
	tmp_vec.clear();

	Random daily_question_random;
	uint32_t day = TimeUtils::day_align_low(NOW());
	daily_question_random.set_random_seed(day);
	QuestionUtils::get_random_question_vector(count, &daily_question_random, tmp_vec);
	if(tmp_vec.empty()){
		ERROR_TLOG("daily question vector is null");
		return -1;
	}

	FOREACH(tmp_vec, it){
		cli_out_.add_num(*it);
	}
	return 0;
}

//玛茵答题
int proc_require_mary_qusetion(player_t *player, uint32_t count,
	   	uint32_t rule, onlineproto::sc_0x0612_get_question_info& cli_out_)
{
	uint32_t times = GET_A(kDailyAnswerMaryQuestionTimes);
	if(times >= mary_question_time_limit ){
        return cli_err_question_times_get_limit;
	}
	std::vector<uint32_t> tmp_vec;
	tmp_vec.clear();
	QuestionUtils::get_question_vector(player, count, rule, tmp_vec);
	if(tmp_vec.empty()){
		ERROR_TLOG("question vector is null");
		return -1;
	}

	FOREACH(tmp_vec, it){
		cli_out_.add_num(*it);
	}
	ADD_A(kDailyAnswerMaryQuestionTimes, 1);
	return 0;
}
 
int GetQuestionInfoCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	return 0;
}

/* //答题/////////////////////////////////////////////////////////////答题 */
void SubmitUserAnswerCmdProcessor::register_question_type()
{
	answer_func_map_[(uint32_t)commonproto::QUESTION_MARY] = proc_answer_mary_qusetion;
	answer_func_map_[(uint32_t)commonproto::QUESTION_DAILY] = proc_answer_daily_qusetion;
}

int SubmitUserAnswerCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;

	uint32_t max_cnt = QuestionUtils::get_question_max_count();

	if(cli_in_.question_num() < 1 || cli_in_.question_num() > max_cnt){
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_question_number_invalid);
	}

	int ret = 0;
	cli_out_.Clear();
	cli_out_.set_type(cli_in_.type());

	uint32_t score = 0;
	//序号
	uint32_t question_id = cli_in_.question_id();
	//编号
	uint32_t question_num = cli_in_.question_num();
	uint32_t answer_num = cli_in_.answer_num();
	//答题用时
	player->temp_info.time_use = cli_in_.time_use();
	//是否要求使用双倍卡
	player->temp_info.score_double  = cli_in_.score_double();
	bool result = false;
	result = QuestionUtils::get_question_result(question_num, answer_num);

	if (answer_func_map_.count((uint32_t)cli_in_.type()) != 0) {
		answer_proc_func_t fun;
		fun = (answer_func_map_.find((uint32_t)cli_in_.type()))->second;
		ret = fun(player, question_id, question_num, result, score);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
	}

	cli_out_.set_question_num(question_num);
	cli_out_.set_is_correct(result);
	cli_out_.set_score(score);
	cli_out_.set_is_double(player->temp_info.score_double);
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

//玛茵答题
int proc_answer_mary_qusetion(player_t *player, uint32_t question_id,
		uint32_t question_num, bool result, uint32_t & score)
{
	if(result){
		ADD_A(kAttrAnswerMaryQuestionCorrectCnt, 1);
		ADD_A(kDailyAnswerMaryQuestionCorrectCnt, 1);
	}
	//全部答对广播
	uint32_t count = 0;
	uint32_t type = 44;
	count = GET_A(kAttrAnswerMaryQuestionCorrectCnt);
	bool broadcast = false;
	switch(count){
		case 10:
		case 20:
		case 30:
		case 40:
		case 50:
		case 60:
		case 70:
			broadcast = true;
			break;
		default:
			broadcast = false;
	}

	if(broadcast){
		static char msg[256];

		snprintf(msg, sizeof(msg), "[pi=%u|%u] %s [/pi]成功解答今日傲娇提问,获得[cl=0xe36c0a]傲娇礼包[/cl]![op=%u|%u]我也要参加[/op]", 
				player->userid, player->create_tm, player->nick, player->userid, type);
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

	return 0;
}

//每日答题
int proc_answer_daily_qusetion(player_t *player, uint32_t question_id,
		uint32_t question_num, bool result, uint32_t &score)
{
	if(question_id < 1 || question_id > daily_question_time_limit){
        return cli_err_question_times_get_limit;
	}


	//判断答题次数
	uint32_t times = GET_A(kDailyAnswerDailyQuestionTimes);
	if(times >= daily_question_time_limit){
        return cli_err_question_times_get_limit;
	}
	ADD_A(kDailyAnswerDailyQuestionTimes, 1);

	//使用双倍卡
	uint32_t ret = 0;
	uint32_t is_double = player->temp_info.score_double;
	uint32_t time_use = player->temp_info.time_use;
	if(is_double){
		attr_type_t attr_type = kServiceBuyAnswerQuestionScoreDouble;	
		const uint32_t product_id = 90063;	
		ret = buy_attr_and_use(player, attr_type, product_id, 1);
		if(0 != ret){
			//购买失败
			player->temp_info.score_double = 0;
			is_double = 0;
		}
	}

	if(result && time_use <= 16){
		//设置答题标记位
		uint32_t correct_flag =  GET_A(kDailyAnswerDailyQuestionFlag);
		if(!taomee::test_bit_on(correct_flag, question_id)){
			correct_flag = taomee::set_bit_on(correct_flag, question_id);
			SET_A(kDailyAnswerDailyQuestionFlag, correct_flag);
		} else {
			return cli_err_answer_question_number_repeat;
		}
		//连续打对次数
		uint32_t succession_count = 0;
		for(uint32_t i = question_id; i > 0; i--){
			if(taomee::test_bit_on(correct_flag, i)){
				succession_count ++;
			} else { 
				break;
			}
		}

		SET_A(kDailyAnswerDailyQuestionSuccessionCount, succession_count);
		
		//计算得分
		if(time_use >= base_time_use ){
			score = 1;
		} else if(time_use >= 1){
			score = max_score  - (time_use - 1) / score_gap;
		} else {
			score = 0;
		}
		//连续答对加成
		score += succession_count > 1? succession_count * succession_add : 0; 
		//double 加成
		score *= is_double ? 2 : 1;		
		//更新排行榜
		uint32_t total_score = GET_A(kDailyAnswerDailyQuestionTotalScore);
		uint32_t new_score = total_score + score;
		if(new_score > total_score){
			SET_A(kDailyAnswerDailyQuestionTotalScore, new_score);
			uint32_t day = TimeUtils::day_align_low(NOW());

			RankUtils::rank_user_insert_score(
					player->userid, player->create_tm,
					commonproto::RANKING_DAILY_QUESTION_SCORE, 
					day, new_score);
		} else if(new_score < total_score){
			ERROR_TLOG("anwer daily question score err ! score = %u",score);
			return -1;
		}
	} else {
		score = 0;
	}
	return 0;
}

int SubmitUserAnswerCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	return 0;
}

int RequireDailyQuestionRewardCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;

	uint32_t day = TimeUtils::day_align_low(NOW());

	std::vector<role_info_t> role_vec;
	role_info_t role_info;
	role_info.userid= player->userid;
	role_info.u_create_tm = player->create_tm;
	role_vec.push_back(role_info);

	return RankUtils::get_user_rank_info(
			player, commonproto::RANKING_DAILY_QUESTION_SCORE, 
			day, role_vec, commonproto::RANKING_ORDER_DESC);
}

int RequireDailyQuestionRewardCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	uint32_t reward_flag = GET_A(kDailyQuestionRewardFlag);
	if(reward_flag){
		ERROR_TLOG("get daily questiong rank reward repeat!"
				"uid=[%u] create_tm=[%u]", 
				player->userid, player->create_tm);
		return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	}

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
		if(rank == 1){
			prize_id = 11900;
		} else if (rank >= 2 && rank <= 4){
			prize_id = 11901;
		} else if (rank >= 5 && rank <= 10){
			prize_id = 11902;
		} 

		if(prize_id != 0 ){
			std::string content = "你在本次答题排行榜中，达到了第" + boost::lexical_cast<std::string>(rank) +
				"名，发放以下奖励";
			PlayerUtils::generate_new_mail(player, "获得了答题排名奖励", content,
					prize_id);
			SET_A(kDailyQuestionRewardFlag, 1);
		}
		if(rank == 1){
			//发称号
			prize_id = 11904;
			onlineproto::sc_0x0112_notify_get_prize noti_;
			int ret = transaction_proc_prize(player, prize_id, noti_, 
					commonproto::PRIZE_REASON_DAILY_QUESTION_TITLE_PRIZE,onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
			if (ret) {
				RET_ERR(ret);
			}
			send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_);
			static char msg[256];
			snprintf(msg, sizeof(msg), "[pi=%u|%u] %s [/pi]获得今日答题排行榜第一名,获得[cl=0xe36c0a]万事通[/cl]称号!", 
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
		}

	} else {
		ERROR_TLOG("get daily questiong rank from serv err!"
				"uid=[%u] create_tm=[%u]", 
				player->userid, player->create_tm);
	}

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
