#ifndef TRANS_PROF
#define TRANS_PROF

#include "common.h"

struct trans_prof_conf_t {
	trans_prof_conf_t& operator= (const trans_prof_conf_t& other);
	uint32_t prof_id;
	uint32_t stage;
	uint32_t hp;
	uint32_t normal_atk;
	uint32_t normal_def;
	uint32_t skill_atk;
	uint32_t skill_def;
	uint32_t crit;
	uint32_t anti_crit;
	uint32_t hit;
	uint32_t dodge;
	uint32_t block;
	uint32_t break_block;
	uint32_t level;
    uint32_t gold;//消耗的金币
	std::vector<int> skill_list;
    //<id, cnt>
	std::map<uint32_t, uint32_t> consume_item_map;
};

class trans_prof_conf_manager_t {
public:
	trans_prof_conf_manager_t() {

	}
	~trans_prof_conf_manager_t() {

	}
	inline uint32_t add_trans_prof_conf(trans_prof_conf_t& trans_prof_conf) {
		trans_prof_conf_manager_.push_back(trans_prof_conf);
		return 0;
	}
	inline const std::vector<trans_prof_conf_t>& const_trans_prof_conf_manager() const {
		return trans_prof_conf_manager_;
	}
	inline void copy_from(trans_prof_conf_manager_t& m) {
		trans_prof_conf_manager_ = m.const_trans_prof_conf_manager();
	}
	uint32_t get_conf_by_prof_and_stage(uint32_t prof_id, uint32_t stage, trans_prof_conf_t& trans_prof_conf);
private:
	std::vector<trans_prof_conf_t> trans_prof_conf_manager_;
};
#endif
