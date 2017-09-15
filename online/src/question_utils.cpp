#include "question_utils.h"
int QuestionUtils::get_question_vector(player_t *player, uint32_t cnt, uint32_t rule, std::vector<uint32_t> &ques_vec)
{

	std::vector<uint32_t> &except_vec = *(player->temp_info.cache_question_vec);
	//题库最大数量
	uint32_t max_cnt = get_question_max_count();
	CONDITION_CHECK(max_cnt > 0);
	CONDITION_CHECK(cnt > 0);
	CONDITION_CHECK(cnt <= max_cnt);

	ques_vec.clear();
	uint32_t repeat_times = 0;
	for(uint32_t i = 1; i <= cnt; i++){
		uint32_t num = i;
		if(RANDOM  == rule){
			num = (rand() % max_cnt) + 1;
		}
		//重复次数过多，说明题库数量太少，不做处理
		if(std::count(except_vec.begin(), except_vec.end(), num)
				&& (repeat_times <= max_cnt )){
			repeat_times ++;
			i--;
			continue;
		}
		ques_vec.push_back(num);
		except_vec.push_back(num);
	}
	//打乱顺序
	std::random_shuffle(ques_vec.begin(), ques_vec.end());
	return cnt;
}

int  QuestionUtils::get_random_question_vector(uint32_t cnt, 
		Random *random, std::vector<uint32_t> &ques_vec)
{
	ques_vec.clear();

	//题库数量max
	uint32_t max_cnt = get_question_max_count();
	CONDITION_CHECK(max_cnt > 0);
	CONDITION_CHECK(cnt > 0);
	CONDITION_CHECK(cnt <= max_cnt);

	//记录重复次数
	uint32_t repeat_times = 0;
	for(uint32_t i = 1; i <= cnt; i++){
		uint32_t question_id = random->generate(max_cnt) + 1;

		//避免重复
		if(std::count(ques_vec.begin(), ques_vec.end(), question_id)
				&& repeat_times <= max_cnt){

			random->set_next_random_seed();
			repeat_times ++;
			i--;
			continue;
		}

		ques_vec.push_back(question_id);
		random->set_next_random_seed();
	}
	return 0;
}
