#ifndef __QUESTION_PROCESSOR_H__
#define __QUESTION_PROCESSOR_H__
#include "common.h"
#include "question.h"
#include "question_utils.h"
#include "player.h"
#include "proto/client/pb0x06.pb.h"
#include "cmd_processor_interface.h"
#include "rank_utils.h"

const uint32_t mary_question_time_limit = 12;//玛茵每天答题12次
const uint32_t daily_question_time_limit = 30;//答题活动每天答题30次
const uint32_t base_time_use = 13;//答题用时
const uint32_t max_score = 12;//每次最高得分
const uint32_t score_gap = 1;//得分时间间隔
const uint32_t succession_add  = 1;//连续答对得分加成
//客户端请求答题
class GetQuestionInfoCmdProcessor : public CmdProcessorInterface{
	public:
		typedef int (*question_proc_func_t)(player_t*, uint32_t, uint32_t, 
				onlineproto::sc_0x0612_get_question_info&);
		GetQuestionInfoCmdProcessor() {
			register_question_type();
		};
		int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
		int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
		void register_question_type();
	private:
		onlineproto::cs_0x0612_get_question_info cli_in_;
		onlineproto::sc_0x0612_get_question_info cli_out_;

		std::map<uint32_t, question_proc_func_t> question_func_map_;
};

int proc_require_mary_qusetion(player_t *player, uint32_t count,
	   	uint32_t rule, onlineproto::sc_0x0612_get_question_info& noti);

int proc_require_daily_qusetion(player_t *player, uint32_t count,
		   uint32_t rule, onlineproto::sc_0x0612_get_question_info& noti);

//客户端返回题目答案
class SubmitUserAnswerCmdProcessor : public CmdProcessorInterface{
	public:
		typedef int (*answer_proc_func_t)(player_t*, uint32_t, uint32_t, bool, uint32_t &);
		SubmitUserAnswerCmdProcessor(){
			register_question_type();
		};	
		int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
		int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
		void register_question_type();
	private:
		onlineproto::cs_0x0613_submit_user_answer cli_in_;
		onlineproto::sc_0x0613_submit_user_answer cli_out_;
		std::map<uint32_t, answer_proc_func_t> answer_func_map_;
};

int proc_answer_mary_qusetion(player_t *player, uint32_t question_id,
		uint32_t question_num, bool result, uint32_t &score);

int proc_answer_daily_qusetion(player_t *player, uint32_t question_id,
		uint32_t question_num, bool result, uint32_t &score);

class RequireDailyQuestionRewardCmdProcessor : public CmdProcessorInterface{
	public:
		int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
		int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	private:
		onlineproto::cs_0x061A_require_daily_question_reward cli_in_;
		onlineproto::sc_0x061A_require_daily_question_reward cli_out_;
		rankproto::sc_get_user_rank  user_rank_out_;
};

#endif
