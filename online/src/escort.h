#ifndef __ESCORT_H__
#define __ESCORT_H__
#include "common.h"
#include "attr.h"
#include <string>

//属性表中打劫者信息的间隔
const uint32_t ROB_INFO_INTERVAL = 4;
//属性表中被打劫者的信息间隔
const uint32_t DEF_ROB_INFO_INTERVAL = 4;
//每日可以免费打劫的次数
//const uint32_t DAILY_FREE_ROBBERY_TIMES = 3;
//const uint32_t DAILY_FREE_ESCORE_TIMES = 2;
enum escort_common_data_t {
	DAILY_FREE_ESCORE_TIMES =  commonproto::DAILY_FREE_ESCORE_TIMES, //每日免费运宝的次数
	DAILY_FREE_ROBBERY_TIMES =	commonproto::DAILY_FREE_ROBBERY_TIMES, //每日可免费打劫的次数 
	REFRESH_SHIP_TIMES = commonproto::REFRESH_SHIP_TIMES,	//每次运宝前，可以免费刷新飞船的次数
	ROUND_ROB_MAX_TIMES = commonproto::ROUND_ROB_MAX_TIMES, //一次运宝可以打劫的次数
};
//一次运宝可以打劫的次数
//const uint32_t ROUND_ROB_MAX_TIMES = 3;
//每次运宝前，可以免费刷新飞船的次数
//const uint32_t REFRESH_SHIP_TIMES = 2;

enum ship_rate_t
{
	ESCORT_SENIOR_RATE = 5,
	ESCORT_JUNIOR_RATE = 30,
	ESCORT_ROOKIE_RATE = 60,
};

//飞船可被打劫的次数
enum ship_robbery_times_t
{
	ESCORT_JUNIOR_TIMES = 3,
	ESCORT_ROOKIE_TIMES = 6,
};

//不同的飞船运行的时间(10,15,20) (暂时5,6,7，)
enum {
	ESCORT_SENIOR_TIME = 600,
	ESCORT_JUNIOR_TIME = 900,
	ESCORT_ROOKIE_TIME = 1200,
};

//不同类型飞船运载的金钱
enum escort_ship_carry_money_num_t
{
    ROOKIE_CARRY_NUM = 800,
    JUNIOR_CARRY_NUM = 3200,
    SENIOR_CARRY_NUM = 6800,
};

//不同类型飞船被打劫时的基数
enum escort_ship_rob_base_money_t
{
	ROOKIE_SHIP_BASE = 600,
	JUNIOR_SHIP_BASE = 800,
	SENIOR_SHIP_BASE = 1000,
};

/*
enum escort_pk_power_con_t 
{
	POWER_DIFF_LEVEL1 = 2000,
	POWER_DIFF_LEVEL2 = 3000,
};
*/

enum {
	ROUTE_NUM_LIMIT = 30, //一条航线中飞船数量的上限
};

//被人掠夺的信息
struct robberyed_info_t 
{
	robberyed_info_t() : 
		atk_id(0),
		rob_start_time(0),
		pk_result(3),
		value(0) { 
			atk_name.clear();
			btl_key.clear();
		}
	uint32_t atk_id;
	uint32_t rob_start_time;
	std::string atk_name;
	uint32_t pk_result;	//0:平局;1:胜利;2输;3:未pk 
	uint32_t value;	//此次pk的收益值（胜负由lost_symbol指出）
	std::string btl_key;	//此次打劫的战报key
};

struct escort_info_t
{
	escort_info_t() : 
		uid(0), 
		create_tm(0),
		power(0),
		start_time(0),
		route_id(0),
		ship_type(0),
		ship_robberyed_times(0),
		ship_robberyed_max(0),
		ship_carry_money(0),
		pk_get_money_total(0),
		pk_lost_money_total(0),
		in_pk_flag(0)
		{ uid_name.clear();}
		
	uint32_t uid;
	uint32_t create_tm;
	std::string  uid_name;
	uint32_t power;	//玩家最高记录的战斗力，属性表中id为178值
	uint32_t start_time;
	uint32_t route_id;	//航线id
	uint32_t ship_type; //飞船类型
	uint32_t ship_robberyed_times; //该类型飞船已经被打劫的次数
	uint32_t ship_robberyed_max;	//该类型飞船可被打劫的最大次数
	uint32_t ship_carry_money;	//飞船承运的money数量
	uint32_t pk_get_money_total;	//因打劫而pk后获得的money累积
	uint32_t pk_lost_money_total;	//因打劫而pk后丢失的money累积
	uint32_t in_pk_flag;	//是否正处于pk中 0:未 pk; 1:pk中,数值即为同时pk的人数
	std::set<uint32_t> uids; //当前打劫我的人的米米号,打劫开始时插入，打劫结束后删除
 
	//rob_info, 记录着打劫我的玩家： 米米号-->基本信息 
	std::map<uint32_t, robberyed_info_t> rob_info;
};

struct route_info_t
{
	route_info_t() :
		id(0),
		num(0) {}
	uint32_t id;	//航线 ： 1,2,3...
	uint32_t num;	//该航线上玩家数量
};

bool operator < (const route_info_t& lhs, const route_info_t& rhs);


class EscortMgr
{
public:
	typedef std::map<uint64_t, escort_info_t> EscortMap;
	
	typedef std::map<uint64_t/*被打劫者米米号*/, std::map<uint32_t/*打劫序号*/, robberyed_info_t> > RobInfoMap;

	//检查是否可以打劫
	uint32_t check_robbery_condition(
			player_t *player,
			uint32_t def_id,
			uint32_t def_create_tm);

	//增加一个玩家参加运宝活动
	uint32_t add_player_to_escort(player_t *player, 
			onlineproto::escort_switch_svr_t flag = onlineproto::NO_SWITCH_SVR);
	//删除一个玩家运宝信息
	uint32_t del_player_from_escort(uint32_t uid, uint32_t create_tm);

	//计算此类型飞船需要的运输时间
	uint32_t calc_escort_need_time(uint32_t ship_type, uint32_t &time);

	//获取运宝的总人数
	uint32_t get_escort_num(uint32_t& num) {
		num = escort_map.size();
		return 0;
	}

	//获得所有玩家的运宝信息结构
	uint32_t get_escort_info(EscortMap& tmp_map) {
		tmp_map = escort_map;
		return 0;
	}

	//清除掉运宝时间已经到的玩家；（定时器处理函数中调用）
	uint32_t check_finish_escort_players(std::set<uint64_t>& uids);

	//获得玩家的运宝信息
	escort_info_t* get_user_escort_info(uint64_t role_key);

	//获得该飞船能够被打劫的上限
	uint32_t get_ship_robberyed_count_limit(
			uint32_t type, uint32_t& robberyed_max_num) 
	{
		robberyed_max_num = 3;
		return 0;
	}

	uint32_t deal_with_robbery(player_t* player, uint32_t def_uid, uint32_t def_create_tm);
	//批量设置被打劫的玩家的一批属性
	uint32_t set_def_user_attr(
			uint32_t suffer_id, uint32_t suf_create_tm,
			std::vector<attr_data_info_t>& attr_vec);

	//获得下一轮打劫者在属性表中的type
	uint32_t get_next_atk_no(uint32_t boat_type, uint32_t rob_times, uint32_t& atk_no);

	uint32_t get_next_def_no(uint32_t round_rob_times, uint32_t& def_no);

	//同步运宝过程中状态改变的玩家信息
	uint32_t sync_player_info_other(
			uint32_t uid, uint32_t create_tm,
			onlineproto::escort_sync_ship_reason_t reason);	

	//计算该类型飞船可运载的金币数量
	uint32_t calc_ship_carry_money_num(onlineproto::airship_type_t boat_type, uint32_t& money_num);
	//计算该类型飞船打劫时的计算收益的基数
	uint32_t calc_ship_rob_base_num(onlineproto::airship_type_t boat_type, uint32_t& base_num);

	uint32_t add_pk_result_to_rob_info_map(
			uint32_t def_uid, uint32_t def_create_tm,
			uint32_t rob_id, 
			commonproto::challenge_result_type_t type,
			std::string& btl_key);

	uint32_t inform_player_escort_end(uint32_t uid, uint32_t create_tm);
	uint32_t add_player_rob_info(player_t* player);

	uint32_t add_pk_result_to_rob_info(
		player_t* player, uint32_t p_rob_id, 
		commonproto::challenge_result_type_t type, std::string& btl_key);

	//打包玩家的运宝信息
	uint32_t pack_one_escort_info(onlineproto::escort_player_info* escort_ptr, escort_info_t* info_ptr);

	//
	uint32_t deal_with_after_fin_escort_when_login(player_t* player);

	//获得现在有几条航线
	uint32_t get_route_num() {
		return route_set.size();
	}

	//将参与打劫的玩家米米号删除
	uint32_t erase_player_uid_from_uid_set(uint32_t uid, uint32_t create_tm) {
		uint64_t role_key = ROLE_KEY(ROLE(uid, create_tm));
		if (uid_set.count(role_key) == 0) {
			return cli_err_no_uid_will_be_erase;
		}
		uid_set.erase(role_key);
		return 0;
	}
	//添加玩家米米号到 运宝全局管理器的set中
	uint32_t insert_player_uid_to_uid_set(uint32_t uid, uint32_t create_tm) {
		uint64_t role_key = ROLE_KEY(ROLE(uid, create_tm));
		uid_set.insert(role_key);
		return 0;
	}

	//检查玩家是否正在打劫他人
	bool check_player_rob_others_state(uint32_t uid, uint32_t create_tm) {
		uint64_t role_key = ROLE_KEY(ROLE(uid, create_tm));
		if (uid_set.count(role_key)) {
			return true;
		}
		return false;
	}

	//从我自己的set中,删除打劫我的玩家id
	//id:打劫我的玩家 id
	//puid:我自己的 id
	uint32_t erase_rob_id_from_user_set(uint32_t id, uint32_t puid) {
		escort_info_t* e_ptr = get_user_escort_info(puid);
		if (e_ptr == NULL) {
			return cli_err_erase_rob_id_from_set_but_me;
		}
		e_ptr->uids.erase(id);
		return 0;
	}

	void get_route_set(std::set<route_info_t>& route_s) {
		route_s = route_set;
	}

	//找出该角色一轮运宝中被哪些帐号打劫过
	uint32_t get_rob_uids(uint32_t uid, uint32_t u_create_tm, std::set<uint32_t>& uids); 

private:
	//检查uid 玩家运宝是否结束
	uint32_t check_escort_finish(uint32_t uid, uint32_t create_tm);

	//初始化玩家运宝信息
	uint32_t init_player_escort_info(player_t* player, uint32_t route_id);

	//切换服务后，初始化玩家运宝信息
	uint32_t init_escort_info_switch_svr(player_t* player, uint32_t route_id);

	//检查def_id在一轮运宝中，是否被atk_id重复打劫
	bool check_repeat_rob(uint32_t def_id, uint32_t def_create_tm, uint32_t atk_id);

	EscortMap  escort_map;
	//玩家参加运宝时用来分线
	std::set<route_info_t> route_set;
	//被人掠夺的信息
	RobInfoMap rob_info_map;
	//参加打劫的玩家uid+create_tm
	std::set<uint64_t> uid_set;
};

#endif
