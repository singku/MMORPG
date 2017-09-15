#include "swim.h"
#include <algorithm>
bool diver_t::operator==(const diver_t& diver) {
	if (this->userid == diver.userid) {
		return true;
	} else {
		return false;
	}
}

//uint32_t Dive::remove_outdated_diver() {
//	for (std::list<diver_t>::iterator it = diver_list.begin(); it != diver_list.end(); it++) {
//		if (it->dive_time < NOW() - 10) {
//			diver_list.remove(*it);
//		}
//	}
//	return 0;
//}

uint32_t Dive::remove_diver_by_userid(uint32_t userid) {
	for (std::list<diver_t>::iterator it = diver_list.begin(); it != diver_list.end(); it++) {
		if (it->userid == userid) {
			diver_list.remove(*it);
			break;
		}
	}
	return 0;
}

uint32_t Dive::get_diver_by_userid(uint32_t userid, diver_t& diver) {
	for (std::list<diver_t>::iterator it = diver_list.begin(); it != diver_list.end(); it++) {
		if (it->userid == userid) {
			diver = *it;
			return 0;
		}
	}
	return cli_err_diver_not_in_list;
}

bool Dive::has_diver(uint32_t userid) {
	for (std::list<diver_t>::iterator it = diver_list.begin(); it != diver_list.end(); it++) {
		if (it->userid == userid) {
			return true;
		}
	}
	return false;
}

diver_t& diver_t::operator=(const diver_t& other)
{
	if (this == &other) {
		return *this;
	}
	userid = other.userid;
	dive_time = other.dive_time;
	return *this;
}

uint32_t DiveRank::get_minimum_score_in_rank()
{
	if (dive_rank.size() == 0) {
		return 0;
	} else {
		//return *min_element(dive_rank.begin(), dive_rank.end());
		return dive_rank.back().score;
	}
}

uint32_t DiveRank::insert_diver_and_sort(diver_score_info_t& diver_score_info)
{
	//如果发现要插入排行榜的玩家已经在排行榜中，就直接修改分数
	for (uint32_t i = 0; i < dive_rank.size(); i++) {
		if (diver_score_info.userid == dive_rank[i].userid) {
			dive_rank[i].score = diver_score_info.score;
			sort(dive_rank.begin(), dive_rank.end(), greater<diver_score_info_t>());
			return 0;
		}
	}
	//以下情况中，要插入排行榜的玩家必定不在排行榜中
	//如果排行榜已有5人，就把分数最低的踢出去
	if (dive_rank.size() == 5) {
		dive_rank.pop_back();
	}
	//加入新玩家
	dive_rank.push_back(diver_score_info);
	//对排行榜排序
	sort(dive_rank.begin(), dive_rank.end(), greater<diver_score_info_t>());
	return 0;
}

bool diver_score_info_t::operator> (const diver_score_info_t& diver_score_info) const {
	return	score > diver_score_info.score;
}
