#ifndef __question_utils_h__
#define __question_utils_h__
#include "common.h"
#include "question.h"
#include "question_conf.h"
#include "global_data.h"
#include "data_proto_utils.h"
#include <math.h>
#include "player.h"
#include "utils.h"

enum{
	SEQUENCE = 0,
	RANDOM   = 1,
};

const uint32_t kMaxRandomSeed = 99999;
static const uint32_t kMaxRange = 1U << 31;

class Random{
	public:
		Random(){ state_ = 1;}

		// Random(uint32_t random_seed){ state_ = (random_seed) == 0 ?
			   // 1 : random_seed;}

		inline uint32_t generate(uint32_t range){
			state_ = (1103515245U*state_ + 12345U) % kMaxRange;
			CONDITION_CHECK(range > 0);
			CONDITION_CHECK(range <= kMaxRange);
			return state_ % range;
		}

		inline uint32_t set_random_seed(uint32_t seed){
		   	state_ = seed % kMaxRandomSeed;
			return 0;
		}

		inline uint32_t get_next_random_seed(uint32_t seed) {
			// CONDITION_CHECK(1 <= seed && seed <= kMaxRandomSeed);
			const uint32_t next_seed = seed + 1;
			return next_seed % kMaxRandomSeed;
		}

		inline void set_next_random_seed(){
			state_ = get_next_random_seed(state_ );
		}
	private:
		uint32_t state_;
};

class QuestionUtils{
	public:
    const static inline question_conf_t* get_question_conf(uint32_t question_id) {
        return g_question_conf_mgr.find_question_conf(question_id);
    }

	//判断答案
	static inline bool get_question_result(uint32_t question_id,
			uint32_t answer_id){
		const question_conf_t * ptr = get_question_conf(question_id);
		if(ptr != NULL){
			uint32_t num = 0;
			num = std::count(ptr->correct_vec.begin(), ptr->correct_vec.end(), answer_id);
			if(num >= 1){
				return true;
			} 
			return false;
		}
		return false;
	}

	//当前题库的题目数
	static inline uint32_t get_question_max_count(){
		return g_question_conf_mgr.map_size();
	}

	//num是题目的个数,rule 0顺序1随机
	static int get_question_vector(player_t *player, uint32_t cnt, uint32_t rule, std::vector<uint32_t>  &ques_vec);

	static int get_random_question_vector(uint32_t cnt, Random *random, std::vector<uint32_t> &ques_vec);

};
#endif
