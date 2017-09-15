#include "trans_prof.h"
#include "cli_errno.h"


uint32_t trans_prof_conf_manager_t::get_conf_by_prof_and_stage(
		uint32_t prof_id, uint32_t stage, trans_prof_conf_t& trans_prof_conf) {
	for (uint32_t i = 0; i < trans_prof_conf_manager_.size(); i++) {
		if (trans_prof_conf_manager_[i].prof_id == prof_id 
			&& trans_prof_conf_manager_[i].stage == stage) {
			trans_prof_conf = trans_prof_conf_manager_[i];
			return 0;
		}
	}
	return cli_err_no_match_trans_prof;
}

trans_prof_conf_t& trans_prof_conf_t::operator= (
		const trans_prof_conf_t& other)
{
	if (this == &other) {
		return *this;
	}
	prof_id = other.prof_id;
	stage = other.stage;
	hp  = other.hp;
	normal_atk = other.normal_atk;
	normal_def = other.normal_def;
	skill_atk = other.skill_atk;
	skill_def = other.skill_def; 
	crit = other.crit;
	anti_crit = other.anti_crit;
	hit = other.hit;
	dodge = other.dodge;
	block = other.block;
	break_block = other.break_block;                                                                                         
	level = other.level;                                                                                               
	skill_list = other.skill_list;
	consume_item_map = other.consume_item_map;
    gold = other.gold;
	return *this;
}
