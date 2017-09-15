#include <boost/lexical_cast.hpp>
#include "escort.h"
#include "macro_utils.h"
#include "player.h"
#include "global_data.h"
#include "service.h"
#include "player_manager.h"
#include "escort_utils.h"
#include "task_utils.h"

bool operator < (const route_info_t& lhs, const route_info_t& rhs) {
	return lhs.id < rhs.id;
}

uint32_t EscortMgr::add_player_to_escort(
		player_t* player, onlineproto::escort_switch_svr_t flag) 
{
	route_info_t tmp;
	if (route_set.empty()) {
		tmp.id = 1;
		tmp.num = 1;
		route_set.insert(tmp);
	} else {
		std::set<route_info_t>::iterator it;
		it = route_set.begin();
		bool found_flag = false; 
		for (; it != route_set.end(); ++it) {
			if (it->num < ROUTE_NUM_LIMIT) {
				tmp.id = it->id;
				tmp.num = it->num +1;
				route_set.erase(it);
				found_flag = true;
				break;
			}
		}
		if (!found_flag) {
			uint32_t size = route_set.size();
			tmp.id = size + 1;
			tmp.num = 1;
		}
		route_set.insert(tmp);
	}

	if (flag == onlineproto::SWITCH_SVR) {
		init_escort_info_switch_svr(player, tmp.id);
	} else {
		init_player_escort_info(player, tmp.id);
	}
	return 0;
}

uint32_t EscortMgr::init_escort_info_switch_svr(
		player_t* player, uint32_t route_id) 
{
	escort_info_t tmp;

	tmp.uid = player->userid;
	tmp.create_tm = player->create_tm;
	tmp.uid_name = player->nick;
	tmp.power = GET_A(kAttrBattleValueRecord);
	tmp.start_time = GET_A(kAttrEscortLastStarttime);
	tmp.route_id = route_id;
	if (GET_A(kAttrEscortBoatType)) {
		tmp.ship_type = GET_A(kAttrEscortBoatType);
	} else {
		SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_SENIOR);
		tmp.ship_type = GET_A(kAttrEscortBoatType);
	}
	tmp.ship_robberyed_times = GET_A(kAttrEscortShipRobberyedTimes);
	uint32_t money_num = 0;
	calc_ship_carry_money_num((onlineproto::airship_type_t)tmp.ship_type, money_num);
	tmp.ship_carry_money = money_num;
	tmp.pk_get_money_total = GET_A(kAttrEscortRobberyGetMoneyTotal);
	tmp.pk_lost_money_total = GET_A(kAttrEscortRobberyLostMoneyTotal);

	SET_A(kAttrEscortOnlineId, g_online_id);

	for (uint32_t i = 1; i <= 3; ++i) {
		uint32_t def_rob_id = kAttrDefRob1Id + (i - 1) * DEF_ROB_INFO_INTERVAL;
		if (GET_A((attr_type_t)def_rob_id)) {
			tmp.rob_info[i].atk_id = GET_A((attr_type_t)def_rob_id);
			tmp.rob_info[i].rob_start_time = GET_A((attr_type_t)(def_rob_id + 1));
			tmp.rob_info[i].pk_result = GET_A((attr_type_t)(def_rob_id + 2));
			tmp.rob_info[i].value = GET_A((attr_type_t)(def_rob_id + 3)); 
			tmp.rob_info[i].btl_key = boost::lexical_cast<string>(tmp.rob_info[i].rob_start_time) 
								+ boost::lexical_cast<string>(player->userid) 
								+ boost::lexical_cast<string>(tmp.rob_info[i].atk_id);
		}
	}

	uint64_t role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	escort_map[role_key] = tmp;

	return 0;
}

uint32_t EscortMgr::del_player_from_escort(uint32_t uid, uint32_t create_tm)
{
	uint32_t route_id;
	uint64_t role_key = ROLE_KEY(ROLE(uid, create_tm));
	EscortMap::iterator it = escort_map.find(role_key);
	if (it != escort_map.end()) {
		route_id = it->second.route_id;
		escort_map.erase(it);
	}

	FOREACH(route_set, iter_s) {
		if (iter_s->id == route_id) {
			route_info_t route_info(*iter_s);
			if (route_info.num > 0) {
				--route_info.num;
			}
			route_set.erase(iter_s);
			route_set.insert(route_info);
			break;
		}
	}
	RobInfoMap::iterator iter = rob_info_map.find(role_key);
	if (iter != rob_info_map.end()) {
		iter->second.clear();
		rob_info_map.erase(iter);
	}
	player_t *player = g_player_manager->get_player_by_userid(uid);
	if (player) {
		player->temp_info.escort_p_rob_id = 0;
	}
	return 0;
}

//检查是否已经结束运宝：0:已结束；非0:错误码
uint32_t EscortMgr::check_escort_finish(uint32_t uid, uint32_t create_tm)
{
	uint64_t role_key = ROLE_KEY(ROLE(uid, create_tm));
	EscortMap::iterator it = escort_map.find(role_key);
	if (it == escort_map.end()) {
		return cli_err_escort_has_finish;
	}
	uint32_t time;
	uint32_t ret = calc_escort_need_time(it->second.ship_type, time);
	if (ret) {
		return ret;
	}
	if (it->second.start_time + time > NOW()) {
		return cli_err_escort_still;
	}
	bool rob_state = check_player_rob_others_state(uid, create_tm);
	if (rob_state) {
		return cli_err_can_not_cal_result_when_robbing;
	}
	if (it->second.in_pk_flag) {
		return cli_err_can_not_cal_result_when_be_robbed;
	}
	
	return 0;
}

uint32_t EscortMgr::calc_escort_need_time(uint32_t ship_type, uint32_t &time)
{
	time = 0;
	if (ship_type == (uint32_t)onlineproto::AIRSHIP_ROOKIE) {
		time = (uint32_t)ESCORT_ROOKIE_TIME;
	} else if (ship_type == (uint32_t)onlineproto::AIRSHIP_JUNIOR) {
		time = (uint32_t)ESCORT_JUNIOR_TIME;
	} else if (ship_type == (uint32_t)onlineproto::AIRSHIP_SENIOR) {
		time = (uint32_t)ESCORT_SENIOR_TIME;
	} else {
		return cli_err_no_such_ship_type;
	}
	return 0;
}

uint32_t EscortMgr::check_finish_escort_players(std::set<uint64_t>& uids)
{
	FOREACH(escort_map, it) {
		role_info_t role_info = KEY_ROLE(it->first);
		uint32_t ret = check_escort_finish(role_info.userid, role_info.u_create_tm);
		if (ret) {
			continue;
		}
		uids.insert(it->first);
	}
	return 0;
}

uint32_t EscortMgr::check_robbery_condition(
		player_t *player, uint32_t def_id,
		uint32_t def_create_tm)
{
	//如果运宝开始时间戳不为0，则要检验自己是否处于运宝状态
	uint32_t ret = 0;
	if (GET_A(kAttrEscortLastStarttime)) {
		ret = check_escort_finish(player->userid, player->create_tm);
		if (ret != cli_err_escort_still) {
			ERROR_TLOG("uid=[%u],escort finish condition not match =[%u]", 
					player->userid);
			return ret;
		}
	}
	//被打劫方必须处于运宝状态
	ret = check_escort_finish(def_id, def_create_tm);
	if (ret != cli_err_escort_still) {
		ERROR_TLOG("def_id,escort finish condition not match =[%u]", 
				def_id);
		return ret;
	}


	uint64_t def_role_key = ROLE_KEY(ROLE(def_id, def_create_tm));
	escort_info_t *e_ptr = get_user_escort_info(def_role_key);
	if (e_ptr == NULL) {
		return cli_err_check_rob_cond_but_suf_not_ext;
	}

	//计算双方战斗力差值
	uint32_t flag;
	uint32_t power_diff = EscortUtils::cal_power_diff(
			GET_A(kAttrBattleValueRecord), e_ptr->power, flag);

	const uint32_t POWER_DIFF_LEVEL1 = 1000 + 0.5 * GET_A(kAttrBattleValueRecord);
	const uint32_t POWER_DIFF_LEVEL2 = 1500 + 1 * GET_A(kAttrBattleValueRecord);
	if (flag == 1 && power_diff > (uint32_t)POWER_DIFF_LEVEL1) {
		return cli_err_def_id_power_too_high;
	} else if (flag == 0 && power_diff > (uint32_t)POWER_DIFF_LEVEL2) {
		//return cli_err_def_id_power_too_low;
		return cli_err_sys_busy;
	}

	//被打劫方的飞船已被打劫的次数
	uint32_t ship_robberyed_times = e_ptr->ship_robberyed_times;
	uint32_t rob_num;
	get_ship_robberyed_count_limit(e_ptr->ship_type, rob_num);
	if (ship_robberyed_times >= rob_num) {
		ERROR_TLOG("ship robbery has no times "
				"ship_rob_times=[%u],rob_num=[%u]", 
				ship_robberyed_times, rob_num);
		return cli_err_ship_robberyed_times_over;
	}
	
	if (rob_info_map.count(def_role_key)) {
		uint32_t cur_rob_size = rob_info_map[def_role_key].size();
		if (cur_rob_size >= rob_num) {
			ERROR_TLOG("ship robbery has no times "
					"cur_rob_size=[%u],rob_num=[%u]", 
					cur_rob_size, rob_num);
			return cli_err_ship_robberyed_times_over;
		}
	}

	if (check_repeat_rob(def_id, def_create_tm, player->userid)) {
		return cli_err_def_id_has_been_rob;
	}

	//判断自己是否还有打劫次数
	uint32_t robbery_times = GET_A(kDailyEscortRobberyTimes);
	uint32_t buy_times = GET_A(kDailyEscortBuyRobberyTimes);
	if (robbery_times >= buy_times + DAILY_FREE_ROBBERY_TIMES) {
		return cli_err_robbery_times_get_limit;
	}

	//判断这一轮运宝是否还有打劫次数(一轮运宝最多可以打劫3次)
	if (GET_A(kAttrEscortRobTimes) >= ROUND_ROB_MAX_TIMES) {
		ERROR_TLOG("escort rob times get limit:rob_time=[%u],uid=[%u]",
			GET_A(kAttrEscortRobTimes), player->userid);
		return cli_err_escort_rob_get_limit_round;
	}

	//若玩家自己正处于打劫状态，则不能再打劫别人
	if (check_player_rob_others_state(player->userid, player->create_tm)) {
		return cli_err_can_not_rob_two_person;
	}

	return 0;
}

escort_info_t* EscortMgr::get_user_escort_info(uint64_t role_key)
{
	EscortMap::iterator it = escort_map.find(role_key);
	if (it == escort_map.end()) {
		return NULL;
	}
	return &it->second;
}

uint32_t EscortMgr::deal_with_robbery(
		player_t* player, uint32_t def_uid, uint32_t def_create_tm) 
{
	uint64_t def_role_key = ROLE_KEY(ROLE(def_uid, def_create_tm));
	escort_info_t* def_ptr = get_user_escort_info(def_role_key);	
	if (def_ptr == NULL) {
		return cli_err_deal_rob_result_but_suf_not_ext;
	}

	uint32_t atk_no = 0;
	uint32_t ret = get_next_atk_no(
			kAttrEscortBoatType, def_ptr->ship_robberyed_times, atk_no);
	if (ret) {
		return ret;
	}
	(*player->temp_info.atk_no_map)[def_uid] = atk_no;

	def_ptr->ship_robberyed_times += 1;

	//这里对于kAttrEscortShipRobberyedTimes ,atk_no对应的type,打劫时间 等属性
	//没有同步跟新到对方的attr内存中,也没同步通知对方客户端
	//但是同步到了运宝管理器的内存，即
	//使用运宝管理器EscortMgr::RobInfoMap rob_info_map进行管理
	std::vector<attr_data_info_t> attr_vec;
	attr_data_info_t attr_data;

	attr_data.type = kAttrEscortShipRobberyedTimes;
	attr_data.value = def_ptr->ship_robberyed_times;
	attr_vec.push_back(attr_data);

	attr_data.type = kAttrEscortBoatType;
	attr_data.value = def_ptr->ship_type;
	attr_vec.push_back(attr_data);

	attr_data.type = atk_no;
	attr_data.value = player->userid;
	attr_vec.push_back(attr_data);

	uint32_t rob_start_time = NOW();
	player->temp_info.escort_rob_tm = rob_start_time;
	attr_data.type = atk_no + 1;
	attr_data.value = rob_start_time;
	attr_vec.push_back(attr_data);

	ret = set_def_user_attr(def_uid, def_create_tm, attr_vec);
	if (ret) {
		return ret;
	}

	uint32_t base_num = 0;
	g_escort_mgr.calc_ship_carry_money_num(
			(onlineproto::airship_type_t)def_ptr->ship_type, base_num);
	uint32_t flag, robbery_money = 0;
	const uint32_t POWER_DIFF_LEVEL1 = 1000 + 0.5 * GET_A(kAttrBattleValueRecord);
	const uint32_t POWER_DIFF_LEVEL2 = 1500 + 1 * GET_A(kAttrBattleValueRecord);
	uint32_t power_diff = EscortUtils::cal_power_diff(
			GET_A(kAttrBattleValueRecord), def_ptr->power, flag);	
	//打劫低战力太低，没有收益
	if (flag == 0 && power_diff > POWER_DIFF_LEVEL2) {
		robbery_money = 0;
	} else if (flag == 0 && 
			(power_diff < POWER_DIFF_LEVEL2 && 
			 power_diff > POWER_DIFF_LEVEL1)) {
		robbery_money = (uint32_t)(base_num * 0.1);
	} else if (power_diff < POWER_DIFF_LEVEL1) {
		robbery_money = (uint32_t)(base_num * 0.2);
	}
	SET_A(kAttrEscortRobberyGetMoney, robbery_money);

	//玩家自己的打劫次数+1
	ADD_A(kDailyEscortRobberyTimes, 1);

	if (GET_A(kAttrEscortLastStarttime)) {
		//玩家本轮运宝打劫次数+1
		//我去主动打劫别人，我肯定在线，则用GET_A,可以方便取内存属性
		//所以运宝管理器中无需加相应的内存变量
		ADD_A(kAttrEscortRobTimes, 1);


		uint32_t def_no = 0;
		//计算被我打劫的玩家编号(在kAttrDefRob1Id， kAttrDefRob2Id， kAttrDefRob3Id中按顺序取值)
		ret = get_next_def_no(GET_A(kAttrEscortRobTimes), def_no);
		if (ret) {
			return ret;
		}
		//存储被我打劫的玩家id号，与打劫该玩家的时间戳
		SET_A((attr_type_t)def_no, def_uid);
		SET_A((attr_type_t)(def_no + 1), rob_start_time);
		//添加被我打劫的玩家信息到运宝管理器的内存 rob_info中
		robberyed_info_t tmp_def_rob_info;
		tmp_def_rob_info.atk_id = def_uid;
		tmp_def_rob_info.atk_name = def_ptr->uid_name; 
		tmp_def_rob_info.rob_start_time = rob_start_time;
		tmp_def_rob_info.value = robbery_money;

		uint64_t atk_role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
		escort_info_t* player_ptr = get_user_escort_info(atk_role_key);	
		if (player_ptr == NULL) {
			return cli_err_escort_player_info_not_found;
		}
		uint32_t tmp_size = player_ptr->rob_info.size();
		player_ptr->rob_info[tmp_size + 1] = tmp_def_rob_info;

		player->temp_info.escort_p_rob_id = tmp_size + 1;
	}

	
	//添加玩家被打劫的信息到运宝管理器 内存 rob_info_map中
	robberyed_info_t tmp_rob_info;
	tmp_rob_info.atk_id = player->userid;
	tmp_rob_info.rob_start_time = rob_start_time;
	tmp_rob_info.atk_name = player->nick;
	tmp_rob_info.value = robbery_money;
	uint32_t size = rob_info_map[def_role_key].size();	
	rob_info_map[def_role_key][size + 1] = tmp_rob_info;
	player->temp_info.escort_rob_id = size + 1;

	return 0;
}

uint32_t EscortMgr::get_next_atk_no(
		uint32_t boat_type, uint32_t rob_times, uint32_t& atk_no)
{
	uint32_t robberyed_num;
	get_ship_robberyed_count_limit(boat_type, robberyed_num);
	if (rob_times <= robberyed_num) {
		atk_no = kAttrRob1Id + rob_times * ROB_INFO_INTERVAL;
	} else {
		return cli_err_ship_robberyed_times_over;
	}
	if (atk_no != kAttrRob1Id && atk_no != kAttrRob2Id && 
			atk_no != kAttrRob3Id && atk_no != kAttrRob4Id && 
			atk_no != kAttrRob5Id && atk_no != kAttrRob6Id) {
		return cli_err_data_error;
	}
	return 0;
}

uint32_t EscortMgr::get_next_def_no(
		uint32_t round_rob_times, uint32_t& def_no)
{
	if (round_rob_times <= ROUND_ROB_MAX_TIMES) {
		def_no = kAttrDefRob1Id + (round_rob_times - 1) * DEF_ROB_INFO_INTERVAL;
	} else {
		return cli_err_escort_rob_get_limit_round;
	}
	return 0;
}



//批量设置被打劫的玩家的一批属性
uint32_t EscortMgr::set_def_user_attr(
		uint32_t suffer_id, uint32_t suf_create_tm,
		std::vector<attr_data_info_t>& attr_vec) 
{
	dbproto::cs_set_attr db_in;
	FOREACH(attr_vec, it) {
		commonproto::attr_data_t *attr_ptr = db_in.add_attrs();
		attr_ptr->set_type(it->type);
		attr_ptr->set_value(it->value);
	}
	int ret = g_dbproxy->send_msg(NULL, suffer_id, suf_create_tm, db_cmd_set_attr, db_in);
	if (ret) {
		return ret;
	}
	return 0;
}

//仅供insert_to_route函数调用或者在选择航线后调用
uint32_t EscortMgr::init_player_escort_info(player_t* player, uint32_t route_id)
{
	uint32_t start_time = NOW();

	escort_info_t tmp;

	tmp.uid = player->userid;
	tmp.create_tm = player->create_tm;
	tmp.uid_name = player->nick;
	tmp.power = GET_A(kAttrBattleValueRecord);
	tmp.start_time = start_time;
	tmp.route_id = route_id;
	if (GET_A(kAttrEscortBoatType)) {
		tmp.ship_type = GET_A(kAttrEscortBoatType);
	} else {
		SET_A(kAttrEscortBoatType, (uint32_t)onlineproto::AIRSHIP_SENIOR);
		tmp.ship_type = GET_A(kAttrEscortBoatType);
	}
	uint32_t money_num = 0;
	calc_ship_carry_money_num((onlineproto::airship_type_t)tmp.ship_type, money_num);
	tmp.ship_robberyed_times = 0;
	tmp.ship_carry_money = money_num;
	g_escort_mgr.get_ship_robberyed_count_limit(tmp.ship_type, tmp.ship_robberyed_max);

	uint64_t role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	escort_map[role_key] = tmp;

	SET_A(kAttrEscortLastStarttime, start_time);
	return 0;
}

//同步运宝过程中状态改变的玩家信息
uint32_t EscortMgr::sync_player_info_other(
		uint32_t uid, uint32_t create_tm,
		onlineproto::escort_sync_ship_reason_t reason)
{
	uint64_t role_key = ROLE_KEY(ROLE(uid, create_tm));
	escort_info_t* user_ptr = get_user_escort_info(role_key);	
	if (user_ptr == NULL) {
		return cli_err_will_send_escort_info_err;
	}
	onlineproto::sc_0x0807_escort_sync_other_airship sync_msg;
	pack_one_escort_info(sync_msg.mutable_playerinfo(), user_ptr);

	sync_msg.set_sync_type(reason);

	FOREACH(escort_map, it) {
		if (it->second.route_id != user_ptr->route_id) {
			continue;
		}
		//uint32_t user_id = it->first;
		role_info_t role_info = KEY_ROLE(it->first);
		player_t* p = g_player_manager->get_player_by_userid(role_info.userid);
		if (p == NULL) {
			continue;
		}
		send_msg_to_player(p, cli_cmd_cs_0x0807_escort_sync_other_airship, sync_msg);
	}
	return 0;
}

uint32_t EscortMgr::calc_ship_carry_money_num(onlineproto::airship_type_t boat_type, uint32_t& money_num)
{
	if (boat_type == onlineproto::AIRSHIP_ROOKIE) {
		money_num = (uint32_t)ROOKIE_CARRY_NUM;
	} else if (boat_type == onlineproto::AIRSHIP_JUNIOR) {
		money_num = (uint32_t)JUNIOR_CARRY_NUM;
	} else if (boat_type == onlineproto::AIRSHIP_SENIOR) {
		money_num = (uint32_t)SENIOR_CARRY_NUM;
	}
	return 0;
}

uint32_t EscortMgr::calc_ship_rob_base_num(onlineproto::airship_type_t boat_type, uint32_t& base_num)
{
	if (boat_type == onlineproto::AIRSHIP_ROOKIE) {
		base_num = (uint32_t)ROOKIE_SHIP_BASE;
	} else if (boat_type == onlineproto::AIRSHIP_JUNIOR) {
		base_num = (uint32_t)JUNIOR_SHIP_BASE;
	} else if (boat_type == onlineproto::AIRSHIP_SENIOR) {
		base_num = (uint32_t)SENIOR_SHIP_BASE;
	}
	return 0;
}

uint32_t EscortMgr::inform_player_escort_end(uint32_t uid, uint32_t create_tm) 
{
	uint64_t role_key = ROLE_KEY(ROLE(uid, create_tm));
	escort_info_t* es_ptr = get_user_escort_info(role_key);
	if (es_ptr == NULL) {
		ERROR_TLOG("player has finish escort,uid=[%u]", uid);
		return cli_err_escort_has_finish;
	}
	onlineproto::sc_0x0804_inform_escort_end cli_msg;
	if (es_ptr->ship_type) {
		cli_msg.set_type((onlineproto::airship_type_t)es_ptr->ship_type);
	} else {
		cli_msg.set_type(onlineproto::AIRSHIP_SENIOR);
	}
	
	if (rob_info_map.count(role_key)) {
		FOREACH(rob_info_map[role_key], it) {
			onlineproto::escort_robbery_info *rob_ptr = cli_msg.add_robbery_info();
			rob_ptr->set_robber_uid(it->second.atk_id);
			rob_ptr->set_robber_name(it->second.atk_name);
			rob_ptr->set_result((commonproto::challenge_result_type_t)it->second.pk_result);
			if ((commonproto::challenge_result_type_t)it->second.pk_result == commonproto::WIN) {
				rob_ptr->set_lost_symbol(0);
			} else {
				rob_ptr->set_lost_symbol(1);
			}
			rob_ptr->set_lost(it->second.value);
			rob_ptr->set_key(it->second.btl_key);
			rob_ptr->set_def_robber_uid(uid);
			rob_ptr->set_def_robber_name(es_ptr->uid_name);
		}
	}
	player_t* player = g_player_manager->get_player_by_userid(uid);
	if (player) {
		FOREACH(es_ptr->rob_info, iter) {
			onlineproto::escort_robbery_info *rob_ptr = cli_msg.add_robbery_info();
			rob_ptr->set_robber_uid(uid);
			rob_ptr->set_robber_name(player->nick);
			rob_ptr->set_result((commonproto::challenge_result_type_t)iter->second.pk_result);
			if ((commonproto::challenge_result_type_t)iter->second.pk_result == commonproto::WIN) {
				rob_ptr->set_lost_symbol(0);
			} else {
				rob_ptr->set_lost_symbol(1);
			}
			rob_ptr->set_lost(iter->second.value);
			rob_ptr->set_key(iter->second.btl_key);
			rob_ptr->set_def_robber_uid(iter->second.atk_id);
			rob_ptr->set_def_robber_name(iter->second.atk_name);
		}
	}

	uint32_t total_reward = 0;
	if (es_ptr->ship_carry_money + es_ptr->pk_get_money_total > es_ptr->pk_lost_money_total) {
		total_reward = es_ptr->ship_carry_money + es_ptr->pk_get_money_total - es_ptr->pk_lost_money_total;
	}
	// 将玩家的运宝收益保存下来
	std::vector<attr_data_info_t> attr_vec;
	attr_data_info_t attr_data1;
	attr_data1.type = kAttrEscortEarning;
	attr_data1.value = total_reward;
	attr_vec.push_back(attr_data1);

	//设置未领取奖励的标志
	attr_data_info_t attr_data2;
	attr_data2.type = kAttrEscortReward;
	attr_data2.value = 1;
	attr_vec.push_back(attr_data2);
	if (player) {
		Attr* attr = player->attrs;
		attr->put_attr(attr_data1);
		attr->put_attr(attr_data2);
		onlineproto::sc_0x0111_sync_attr msg;
		commonproto::attr_data_t *data_ptr = msg.add_attr_list();
		data_ptr->set_type(kAttrEscortEarning);
		data_ptr->set_value(total_reward);
		commonproto::attr_data_t *data_ptr2 = msg.add_attr_list();
		data_ptr2->set_type(kAttrEscortReward);
		data_ptr2->set_value(1);
		send_msg_to_player(player, cli_cmd_cs_0x0111_sync_attr, msg);	
	}
		
	uint32_t ret = g_escort_mgr.set_def_user_attr(uid, es_ptr->create_tm, attr_vec);
	if (ret) {
		ERROR_TLOG("uid=[%u]set def user attr err", uid);
		return ret;
	}
	//给客端回包
	if (player) {
		cli_msg.set_get_total(total_reward);
		send_msg_to_player(player, cli_cmd_cs_0x0804_inform_escort_end, cli_msg);	
	}
	return 0;
}

//添加被打劫的玩家，打劫结果信息，以及战报key
uint32_t EscortMgr::add_pk_result_to_rob_info_map(uint32_t def_uid, uint32_t def_create_tm, 
		uint32_t rob_id, commonproto::challenge_result_type_t type, 
		std::string& btl_key)
{
	uint64_t def_role_key = ROLE_KEY(ROLE(def_uid, def_create_tm));
	//若使用 tmp_size做索引，则必须加一限制：飞船被打劫时，其他玩家不可打劫
	if (rob_info_map.count(def_role_key)) {
		rob_info_map[def_role_key][rob_id].pk_result = (uint32_t)type;
		rob_info_map[def_role_key][rob_id].btl_key = btl_key;
	}
	return 0;
}

//添加我打劫别人的信息，打劫结果信息，以及战报key
uint32_t EscortMgr::add_pk_result_to_rob_info(
		player_t* player, uint32_t p_rob_id, 
		commonproto::challenge_result_type_t type, std::string& btl_key)
{
	uint64_t role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	escort_info_t* p_ptr = get_user_escort_info(role_key);	
	if (p_ptr == NULL) {
		return cli_err_save_rob_info_but_user_not_ext;
	}
	//uint32_t tmp_size = p_ptr->rob_info.size();
	p_ptr->rob_info[p_rob_id].pk_result = (uint32_t)type;
	p_ptr->rob_info[p_rob_id].btl_key = btl_key;
	return 0;
}

uint32_t EscortMgr::add_player_rob_info(player_t* player)
{
	if (player == NULL) {
		return 0;
	}
	uint64_t role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	RobInfoMap::iterator it = rob_info_map.find(role_key);
	if (it != rob_info_map.end()) {
		ERROR_TLOG("player has in escort in this svr:uid=[%u]", player->userid);
		return 0;
	}
	uint32_t index = 1;
	for (uint32_t i = kAttrRob1Id; i <= kAttrRob6Id; i += ROB_INFO_INTERVAL) {
		if (!GET_A((attr_type_t)i)) {
			continue;
		}

		robberyed_info_t rob_info;
		rob_info.atk_id = GET_A((attr_type_t)i);
		rob_info.rob_start_time = GET_A((attr_type_t)(i + 1));
		rob_info.pk_result = GET_A((attr_type_t)(i + 2));
		rob_info.value = GET_A((attr_type_t)(i + 3));
		rob_info.btl_key = boost::lexical_cast<std::string>(rob_info.rob_start_time) 
			+ boost::lexical_cast<std::string>(rob_info.atk_id) 
			+ boost::lexical_cast<std::string>(player->userid);

		rob_info_map[role_key][index] = rob_info;

		++index;
	}
	return 0;
}

uint32_t EscortMgr::pack_one_escort_info(
	onlineproto::escort_player_info* escort_ptr, escort_info_t* info_ptr)
{
	/*
	if (escort_ptr == NULL || info_ptr == NULL) {
		return cli_err_pack_one_escort_err;
	}
	*/
	if (!(escort_ptr && info_ptr)) {
		return cli_err_pack_one_escort_err;
	}
	escort_ptr->set_uid(info_ptr->uid);
	escort_ptr->set_u_create_tm(info_ptr->create_tm);
	escort_ptr->set_name(info_ptr->uid_name);
	escort_ptr->set_power(info_ptr->power);
	escort_ptr->set_type((onlineproto::airship_type_t)info_ptr->ship_type);
	escort_ptr->set_robbery_count(info_ptr->ship_robberyed_times);
	const uint32_t CUR_TIME = NOW();
	if (CUR_TIME >= info_ptr->start_time) {
		escort_ptr->set_progress(CUR_TIME - info_ptr->start_time);
	} else { //理论上不会执行到此
		ERROR_TLOG("p=[%u],user start time,[%u],[%u]",
				info_ptr->uid, info_ptr->start_time, info_ptr->power);
	}
	std::set<uint32_t> uids;
	get_rob_uids(info_ptr->uid, info_ptr->create_tm, uids);
	if (!uids.empty()) {
		FOREACH(uids, it) {
			escort_ptr->add_atk_uids(*it);
		}
	}
	return 0;
}

uint32_t EscortMgr::deal_with_after_fin_escort_when_login(player_t* player)
{
	if (0 == GET_A(kAttrEscortBoatType) || 
			GET_A(kAttrEscortLastStarttime) || 
			!GET_A(kAttrEscortReward)) {
		return 0;
	}
	onlineproto::sc_0x0804_inform_escort_end cli_msg;
	cli_msg.set_type((onlineproto::airship_type_t)GET_A(kAttrEscortBoatType));
	for (uint32_t i = (uint32_t)kAttrRob1Id; i <= (uint32_t)kAttrRob3Id; i += 4) {
		if (GET_A((attr_type_t)i) == 0) { 
			continue;
		}
		onlineproto::escort_robbery_info *rob_ptr = cli_msg.add_robbery_info();
		uint32_t atk_id = GET_A((attr_type_t)i);
		uint32_t pk_time = GET_A((attr_type_t)(i + 1));
		/*
		uint32_t result = GET_A((attr_type_t)(i + 2));
		uint32_t lost_money = GET_A((attr_type_t)(i + 3));
		*/
		rob_ptr->set_robber_uid(atk_id);
		rob_ptr->set_result((commonproto::challenge_result_type_t)GET_A((attr_type_t)(i + 2)));
		if ((commonproto::challenge_result_type_t)GET_A((attr_type_t)(i + 2)) == commonproto::WIN) {
			rob_ptr->set_lost_symbol(0);
		} else {
			rob_ptr->set_lost_symbol(1);
		}
		rob_ptr->set_lost(GET_A((attr_type_t)(i + 3)));
		std::string prefix("btlkey");
		std::string key = prefix + boost::lexical_cast<std::string>(pk_time) +
			boost::lexical_cast<std::string>(atk_id) +
			boost::lexical_cast<std::string>(player->userid);
		rob_ptr->set_key(key);
		rob_ptr->set_def_robber_uid(player->userid);
	}
	for (uint32_t j = (uint32_t)kAttrDefRob1Id; j <= (uint32_t)kAttrDefRob3Id; j += 4) {
		if (GET_A((attr_type_t)j) == 0) {
			continue;
		}
		onlineproto::escort_robbery_info *rob_ptr = cli_msg.add_robbery_info();
		uint32_t def_id = GET_A((attr_type_t)j);
		uint32_t pk_time = GET_A((attr_type_t)(j + 1));
		/*
		uint32_t result = GET_A((attr_type_t)(j + 2));
		uint32_t lost_money = GET_A((attr_type_t)(j + 3));
		*/
		rob_ptr->set_robber_uid(player->userid);
		rob_ptr->set_result((commonproto::challenge_result_type_t)GET_A((attr_type_t)(j + 2)));
		if ((commonproto::challenge_result_type_t)GET_A((attr_type_t)(j + 2)) == commonproto::WIN) {
			rob_ptr->set_lost_symbol(0);
		} else {
			rob_ptr->set_lost_symbol(1);
		}
		rob_ptr->set_lost(GET_A((attr_type_t)(j + 3)));
		std::string prefix("btlkey");
		std::string key = prefix + boost::lexical_cast<std::string>(pk_time) +
					boost::lexical_cast<std::string>(player->userid) +
					boost::lexical_cast<std::string>(def_id);
		rob_ptr->set_key(key);
		rob_ptr->set_def_robber_uid(def_id);
	}
	cli_msg.set_get_total(GET_A(kAttrEscortEarning));
	send_msg_to_player(player, cli_cmd_cs_0x0804_inform_escort_end, cli_msg);	
	return 0;
}

bool EscortMgr::check_repeat_rob(uint32_t def_uid, uint32_t def_create_tm, uint32_t atk_uid)
{
	uint64_t def_role_key = ROLE_KEY(ROLE(def_uid, def_create_tm));
	RobInfoMap::iterator it1 = rob_info_map.find(def_role_key);
	if (it1 != rob_info_map.end()) {
		std::map<uint32_t, robberyed_info_t> &rob_infos = it1->second;
		FOREACH(rob_infos, it2) {
			if (it2->second.atk_id == atk_uid) {
				return true;
			}
		}
	}
	return false;	
}

//找出该角色一轮运宝中被哪些帐号打劫过
uint32_t EscortMgr::get_rob_uids(uint32_t uid, uint32_t u_create_tm, std::set<uint32_t>& uids)
{
	uint64_t role_key = ROLE_KEY(ROLE(uid, u_create_tm));
	if (rob_info_map.count(role_key)) {
		FOREACH(rob_info_map[role_key], it) {
			uids.insert(it->second.atk_id);
		}
	}
	return 0;
}
