#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <vector>
#include "common.h"
//目前最大拉取排名的数量
const uint32_t kMaxRankPlayerNum = 50;

//最多15页 page_no [1, 15] page_size <= 10
const uint32_t RANK_TOTAL_RANGE_LIMIT = 150;

//排行榜点赞相关
//点赞获得体力增量
const uint32_t PRAISE_ADD_HP_COUNT = 1;

const uint32_t PRAISE_ADD_COINS = 50;

enum arena_player_com_data_t 
{
	ARENA_PLAYER_INDEX_BEGIN = 0,
	ARENA_PLAYER_INDEX_END = 5,	//索引最大值的下一个
	ARENA_DAILY_REWARD_TM_PONIT = 21, //每日奖励结算的时间点
	ARENA_INSERT_RANK_BASED_LV = 10, //玩家升到第10级时,在竞技场中插入排名
	ARENA_PLAYER_NUM = 5,	//排位赛刷新拉取对手的数量
	ARENA_FIGHT_CD = commonproto::ARENA_FIGHT_CD,
	ARENA_STREAK_FLAG = 3, //竞技场连胜次数的下限	
	ARENA_FREE_CHALLENGE_CNT = 5, //竞技场每日免费的挑战次数
	ARENA_COMFORT_PRIZE = commonproto::ARENA_COMFORT_PRIZE,//挑战失败后的固定奖励9022
	ARENA_OPEN_LIMIT = commonproto::ARENA_LV_LIMIT,	//竞技场16级开放
	//竞技场战斗属性血量与双防（普防，技防）的公式系数
	ARENA_COF1 = 60000,
	ARENA_COF2 = 2000,
	ARENA_RANK_THRESHOLD = 5,	//前N名需要全服通告
	ARENA_REFRESH_TIMER_DIFF = commonproto::ARENA_REFRESH_TIMER_DIFF,
	ARENA_REFRESH_CNT = 3,
};

struct weekly_rank_t
{
	uint32_t weekly_sub_key;
	uint64_t rank;
};

struct player_t;
struct streak_info_t 
{
	uint32_t count;
	uint32_t prize_id;
	/*
	uint32_t item_id;
	uint32_t item_count;
	uint32_t add_exp;
	uint32_t coin;
	uint32_t arenacoin;
	*/
};

struct rank_reward_t
{
	uint32_t id;
	uint32_t start_rank;
	uint32_t end_rank;
	uint32_t bonus_id;
	uint32_t single_reward;
	/*
	uint32_t item_id;
	uint32_t item_count;
	uint32_t add_exp;
	uint32_t coin;
	uint32_t arenacoin;
	*/
};

class ArenaUtils
{
public:
	static uint32_t check_challenge_arena_condition(player_t* player);

	//获取上周四的日期作为竞技场排名的sub_key：(年月日)；例如：20141030
	//用于领取竞技场周排名奖励时用
	static uint32_t get_last_week_arena_rank_sub_key(uint32_t& sub_key);
	
	//获取本周四的日期作为竞技场排名的sub_key
	//用于交换竞技场排名; 拉取玩家排名时用
	static uint32_t get_next_week_arena_rank_sub_key(uint32_t& sub_key);

	//竞技场修改玩家战斗属性值
	static uint32_t arena_modify_player_btl_attr(
			commonproto::battle_player_data_t& battle_all_info);

	static int arena_push_streak_end(player_t* player);

	static int arena_push_killing_spree(player_t* player, uint32_t win_streak_count);

	static int win_streak_get_reward(
			player_t* player, uint32_t win_streak_count);

	static int get_reward_after_challenge(
			player_t* player, uint32_t new_rank);

    //处理自动竞技场排名奖励和手动竞技场积分奖励
    static int proc_arena_rank_prize(player_t *player, rankproto::sc_get_user_multi_rank &rank_out);
};

#endif
