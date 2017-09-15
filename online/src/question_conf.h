#ifndef __QUESTION_CONF_H_
#define __QUESTION_CONF_H_

#include "common.h"

struct question_conf_t {
	question_conf_t(){
		clear();
	}
	void clear(){
		id= 0;
		answer_cnt = 0;
		correct_vec.clear();
	}
	uint32_t id;
	uint32_t answer_cnt ;
	std::vector<uint32_t> correct_vec;
};

class question_conf_manager_t {
	public:
		question_conf_manager_t(){
			clear();
		}
		~question_conf_manager_t(){
			clear();
		}
	public:
		void clear(){
			question_conf_map_.clear();
			std::map<uint32_t, question_conf_t>().swap(question_conf_map_);
		}
		inline bool question_conf_exist(uint32_t question_id) {
			if (question_conf_map_.count(question_id) > 0) {
				return true;
			}
			return false;
		}
		inline bool add_question_conf(question_conf_t &question_conf) {
			if (question_conf_exist(question_conf.id)) {
				return false;
			}
			question_conf_map_[question_conf.id] = question_conf;
			return true;
		}
		const inline question_conf_t *find_question_conf(uint32_t question_id) {
			if (question_conf_map_.count(question_id) == 0) {
				return NULL;
			}
			return &((question_conf_map_.find(question_id))->second);
		}
		inline uint32_t map_size(){ return question_conf_map_.size();}
		inline void copy_from(const question_conf_manager_t &m) {
			question_conf_map_ = m.const_question_conf_map();
		}
		const inline std::map<uint32_t, question_conf_t>& const_question_conf_map() const{
			return question_conf_map_;
		}
	private:
		std::map<uint32_t, question_conf_t> question_conf_map_;
};
#endif
