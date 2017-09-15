#ifndef SWIM_DATA_H
#define SWIM_DATA_H
#include <list>
#include <libtaomee/project/types.h>
#include "proto/client/cli_errno.h"
#include "macro_utils.h"
#include "player.h"

struct diver_t {
	diver_t() {
	}
	diver_t(uint32_t userid, uint32_t dive_time) {
		this->userid = userid;
		this->dive_time = dive_time;
	}
	uint32_t userid;
	uint32_t dive_time;
	bool operator ==(const diver_t& diver);
	diver_t& operator=(const diver_t& other);
};

struct diver_score_info_t {
	uint32_t userid;
	uint32_t score;
	std::string nick;
	bool operator> (const diver_score_info_t& diver_score_info) const;
};

class Dive 
{
public:
	inline uint32_t remove_front_diver() {
		if (diver_list.size() == 0) {
			return cli_err_no_diver_in_list;
		} else {
			diver_list.pop_front();
			return 0;
		}
	}
	inline uint32_t add_diver_to_end(player_t* player, diver_t& diver) {
		bool is_start = GET_A(kDailyDiveIsStart);
		if (is_start == false) {
			diver_list.push_back(diver);
			return 0;
		} else {
			return cli_err_already_in_dive_list;
		}
	}
	
	inline uint32_t get_last_diver(diver_t& diver) {
		if (diver_list.empty()) {
			return cli_err_no_diver_in_list;
		} else {
			diver = diver_list.back();
			return 0;
		}
	}

	inline uint32_t get_first_diver(diver_t& diver) {
		if (diver_list.empty()) {
			return cli_err_no_diver_in_list;
		} else {
			diver = diver_list.front();
			return 0;
		}
	}
	//uint32_t remove_outdated_diver();
	uint32_t remove_diver_by_userid(uint32_t userid);
	uint32_t get_diver_by_userid(uint32_t userid, diver_t& diver);
	bool has_diver(uint32_t userid);
private:
	std::list<diver_t> diver_list;
};

class DiveRank
{
public:
	uint32_t get_minimum_score_in_rank();
	uint32_t insert_diver_and_sort(diver_score_info_t& diver_score_info);
	inline uint32_t get_size() {
		return dive_rank.size();
	}
	inline void clear_dive_rank() {
		dive_rank.clear();
	}
	inline uint32_t get_ith_nick(uint32_t i, std::string& nick) {
		if (i >= dive_rank.size()) {
			return cli_err_invalid_dive_index;
		} else {
			nick = dive_rank[i].nick;
			return 0; 
		}
	}

	inline uint32_t get_ith_score(uint32_t i) {
		if (i >= dive_rank.size()) {
			return cli_err_invalid_dive_index;
		} else {
			return dive_rank[i].score; 
		}
	}
private:
	std::vector<diver_score_info_t> dive_rank;
};


#endif
