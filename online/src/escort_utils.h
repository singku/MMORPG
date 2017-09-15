#ifndef __ESCORT_UTILS_H__
#define __ESCORT_UTILS_H__
#include "common.h"
#include "player.h"

class EscortUtils {
public:
	/*
	static uint32_t check_robbery_condition(
			player_t *player, 
			dbproto::sc_get_attr& db_out_, 
			uint32_t &ship_robberyed_times);
	*/

	static int clear_other_attr(const std::set<uint64_t>& uids, uint32_t flag = false);
	//用于计算出两个玩家的战斗力差值
	static uint32_t cal_power_diff(uint32_t atk_power, uint32_t def_power, uint32_t& flag);
	static uint32_t deal_when_rob_result_come_out(
			player_t* player, commonproto::challenge_result_type_t pk_result);

	static uint32_t deal_with_escort_relate_when_login(player_t* player);

	static uint32_t get_optional_route_id(const std::set<route_info_t>& route_s);
};

#endif


