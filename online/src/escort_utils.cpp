#include "escort_utils.h"
#include "global_data.h"
#include "service.h"
#include "player_manager.h"
#include "rank_utils.h"
#include "task_utils.h"
#include "prize.h"

/*
//uint32_t EscortUtils::check_robbery_condition(player_t *player, dbproto::sc_get_attr& db_out_, uint32_t &ship_robberyed_times)
{
	uint32_t attr_size = db_out_.attrs_size();
	ship_robberyed_times = 0;

	//玩家自身要处于运宝状态下,方可打劫
	if (GET_A(kAttrEscortLastStarttime) == 0) {
		ERROR_TLOG("uid=[%u],user not escort", player->userid);
		return cli_err_player_not_escort_yet;
	}

	for (uint32_t i = 0; i < attr_size; ++i) {
		uint32_t type = db_out_.attrs(i).type();
		if (type == kAttrEscortBoatType) {
			uint32_t value = db_out_.attrs(i).value();
			if (value == (uint32_t)onlineproto::AIRSHIP_SENIOR) {
				ERROR_TLOG("uid=[%u]:senior ship can not robbery,ship_type=[%u]", player->userid, value);
				return cli_err_senior_ship_not_robbry;
			}
		} else if (type == kAttrEscortLastStarttime) {
			uint32_t value = db_out_.attrs(i).value();
			if (value == 0) {
				ERROR_TLOG("uid=[%u]:suf id not escort yet", player->userid);
				return cli_err_escort_has_finish;
			}
		} else if (type == kAttrEscortShipRobberyedTimes) {
			ship_robberyed_times = db_out_.attrs(i).value();
		}
	}
	if (ship_robberyed_times == 0) {
		ERROR_TLOG("uid=[%u]:ship robberyed times get over", player->userid);
		return cli_err_ship_robberyed_times_over;
	}
	return 0;
}
*/

int EscortUtils::clear_other_attr(const std::set<uint64_t>& role_s, uint32_t flag) 
{
	if (role_s.empty()) {
		return 0;
	}
	std::vector<attr_data_info_t> mutable_attr_list;
	attr_data_info_t mutable_attr;
	//===============一轮运宝后：要清零的属性 start============================ //
	mutable_attr.type = kAttrEscortLastStarttime;
	mutable_attr.value = 0;
	mutable_attr_list.push_back(mutable_attr);

	/* 飞船类型 移到 领取奖励时清零
	mutable_attr.type = kAttrEscortBoatType;
	mutable_attr.value = 0;
	mutable_attr_list.push_back(mutable_attr);
	*/
	if (flag) {
		mutable_attr.type = kAttrEscortBoatType;
		mutable_attr.value = 0;
		mutable_attr_list.push_back(mutable_attr);
	}

	mutable_attr.type = kAttrEscortRefreshShip;
	mutable_attr.value = 0;
	mutable_attr_list.push_back(mutable_attr);

	mutable_attr.type = kAttrEscortRefreshSeniorShip;
	mutable_attr.value = 0;
	mutable_attr_list.push_back(mutable_attr);

	//一次运宝中：打劫别人的次数
	mutable_attr.type = kAttrEscortRobTimes;
	mutable_attr.value = 0;
	mutable_attr_list.push_back(mutable_attr);
	
	mutable_attr.type = kAttrEscortShipRobberyedTimes;
	mutable_attr.value = 0;
	mutable_attr_list.push_back(mutable_attr);

	mutable_attr.type = kAttrEscortRobberyGetMoneyTotal;
	mutable_attr.value = 0;
	mutable_attr_list.push_back(mutable_attr);

	mutable_attr.type = kAttrEscortRobberyLostMoneyTotal;
	mutable_attr.value = 0;
	mutable_attr_list.push_back(mutable_attr);
	//===============一轮运宝后：要清零的属性 end ============================ //

	

	FOREACH(role_s, it) {
		role_info_t role_info = KEY_ROLE(*it);
		player_t* player = g_player_manager->get_player_by_userid(role_info.userid);
		//如果这些玩家们在线，则修改他们的属性内存
		if (player) {
			Attr* attr = player->attrs;
			for (uint32_t i = 0; i < mutable_attr_list.size(); i++) {
				attr_data_info_t mutable_attr = mutable_attr_list[i];
				attr->put_attr(mutable_attr);
			}
			

		//通知给他们的前端
			onlineproto::sc_0x0111_sync_attr cli_noti;
			FOREACH(mutable_attr_list, it01) {
				commonproto::attr_data_t *attr_ptr = cli_noti.add_attr_list();
				attr_ptr->set_type(it01->type);
				attr_ptr->set_value(it01->value);
			}
			send_msg_to_player(player, cli_cmd_cs_0x0111_sync_attr, cli_noti);
		}
		//同步到DB
		dbproto::cs_set_attr db_in;
		FOREACH(mutable_attr_list, it02) {
			commonproto::attr_data_t *attr_ptr = db_in.add_attrs();
			attr_ptr->set_type(it02->type);
			attr_ptr->set_value(it02->value);
		}
		int ret = g_dbproxy->send_msg(NULL, role_info.userid,
				role_info.u_create_tm, db_cmd_set_attr, db_in);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

//flag :0 玩家自己的战力大； 1 对方的战力大
uint32_t EscortUtils::cal_power_diff(
		uint32_t atk_power, uint32_t def_power,
		uint32_t& flag) {
	if (atk_power > def_power) {
		flag = 0;
		return atk_power - def_power;
	} else {
		flag = 1;
		return def_power - atk_power;
	}
}

uint32_t EscortUtils::deal_when_rob_result_come_out(
	player_t* player, commonproto::challenge_result_type_t pk_result)
{
	//首先将自己打劫别人的状态给删除
	g_escort_mgr.erase_player_uid_from_uid_set(player->userid, player->create_tm);
	//被打劫的米米号
	//存放在 player->temp_info.escort_def_id中，有防外挂的作用
	//防止 玩家 绕过0x0805打劫协议，直接发0x0810打劫结果协议 

	escort_info_t* def_ptr = g_escort_mgr.get_user_escort_info(
			player->temp_info.escort_def_id);	
	if (def_ptr == NULL) {
		if (pk_result != commonproto::QUIT) {
			return cli_err_rob_suf_id_not_exist;
		} else {
			return 0;
		}
	}
	if (def_ptr->ship_robberyed_times < 1) {
		return cli_err_escort_result_before_rob;
	}
	if (def_ptr->in_pk_flag) {
		def_ptr->in_pk_flag -= 1;
	}
	uint64_t role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	escort_info_t* player_ptr = g_escort_mgr.get_user_escort_info(role_key);	
	if (GET_A(kAttrEscortLastStarttime)) {
		if (player_ptr == NULL) {
			return cli_err_rob_user_id_not_exist;
		}
	}
	uint32_t robbery_money = GET_A(kAttrEscortRobberyGetMoney);

	//如果我赢了
	if (pk_result == commonproto::WIN) {
		if (GET_A(kAttrEscortLastStarttime) && player_ptr != NULL) {
			//我获得一笔钱
			player_ptr->pk_get_money_total += robbery_money;
		}
		//被打劫的人损失一笔钱
		def_ptr->pk_lost_money_total = def_ptr->pk_lost_money_total + robbery_money/3.0;
		onlineproto::sc_0x0112_notify_get_prize noti_prize_msg;
		noti_prize_msg.Clear();
		transaction_proc_prize(player, 4004,
				noti_prize_msg,
				commonproto::PRIZE_REASON_ESCORT_ROB_REWARD,
				onlineproto::SYNC_REASON_PRIZE_ITEM);
		send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_prize_msg);
	}
	uint32_t ret = 0;
	if (GET_A(kAttrEscortLastStarttime)) {
		uint32_t def_no;
		ret = g_escort_mgr.get_next_def_no(GET_A(kAttrEscortRobTimes), def_no);
		if (ret) {
			return ret;
		}
		SET_A((attr_type_t)(def_no + 2), pk_result);
		SET_A((attr_type_t)(def_no + 3), robbery_money);
	}
	std::map<uint32_t, uint32_t>::iterator m_iter = (player->temp_info.atk_no_map)->find(def_ptr->uid);
	if (m_iter == (player->temp_info.atk_no_map)->end()) {
		ERROR_TLOG("def_id not exsit:def_id=[%u],uid=[%u]", def_ptr->uid, player->userid);
		return cli_err_cal_atk_no_but_suf_id_not_exsit;
	}
	uint32_t atk_no = m_iter->second;
	//更新玩家pk后获得（丢失）的money总量
	//这里更新的目的，是为了切服后能够使得切服前打劫的成果不丢失
	//一轮运宝结束后，记得将 kAttrEscortRobberyGetMoneyTotal，
	//	kAttrEscortRobberyLostMoneyTotal 清零
	if (GET_A(kAttrEscortLastStarttime) && player_ptr != NULL) {
		SET_A(kAttrEscortRobberyGetMoneyTotal, player_ptr->pk_get_money_total);
		SET_A(kAttrEscortRobberyLostMoneyTotal, player_ptr->pk_lost_money_total);
	}
	//重置单次打劫的收入
	SET_A(kAttrEscortRobberyGetMoney, 0);

	std::vector<attr_data_info_t> attr_vec;
	attr_data_info_t attr_data;
	attr_data.type = kAttrEscortRobberyGetMoneyTotal;
	attr_data.value = def_ptr->pk_get_money_total;
	attr_vec.push_back(attr_data);
	attr_data.type = kAttrEscortRobberyLostMoneyTotal;
	attr_data.value = def_ptr->pk_lost_money_total;
	attr_vec.push_back(attr_data);
	attr_data.type = atk_no;
	attr_data.value = player->userid;
	attr_vec.push_back(attr_data);
	attr_data.type = atk_no + 2;
	attr_data.value = pk_result;
	//attr_data.value = pk_result;
	attr_vec.push_back(attr_data);
	attr_data.type = atk_no + 3;
	attr_data.value = robbery_money;
	attr_vec.push_back(attr_data);

	ret = g_escort_mgr.set_def_user_attr(def_ptr->uid, def_ptr->create_tm, attr_vec);
	if (ret) {
		ERROR_TLOG("uid=[%u]set def user attr err", player->userid);
		return ret;
	}

	std::string btl_key;
	uint64_t def_role_key = ROLE_KEY(ROLE(def_ptr->uid, def_ptr->create_tm));
	RankUtils::save_btl_report_to_redis(
			player, commonproto::ESCORT, def_role_key, pk_result, btl_key);

	g_escort_mgr.add_pk_result_to_rob_info_map(
			def_ptr->uid, def_ptr->create_tm,
			player->temp_info.escort_rob_id, pk_result, btl_key);

	
	g_escort_mgr.add_pk_result_to_rob_info(
			player, player->temp_info.escort_p_rob_id, pk_result, btl_key);

	//g_escort_mgr.erase_player_uid_from_uid_set(player->userid, player->create_tm);

	player->temp_info.escort_rob_id = 0;

	player->temp_info.escort_def_id = 0;

	player->temp_info.escort_rob_tm = 0;


	///////////////////////////////////////////////
	//如果玩家没有参与运宝，此刻把打劫的收益给他
	if (GET_A(kAttrEscortLastStarttime) == 0) {
		if (pk_result == commonproto::WIN) {
            AttrUtils::add_player_gold(player, robbery_money, false, "运宝打劫获得");
		}
	}
	return 0;
}

uint32_t EscortUtils::deal_with_escort_relate_when_login(player_t* player)
{
	//尝试 清除我打劫别人的状态
	g_escort_mgr.erase_player_uid_from_uid_set(player->userid, player->create_tm);
	//如果该玩家正在运宝
	if (GET_A(kAttrEscortLastStarttime)) {
		uint32_t time;
		int ret = g_escort_mgr.calc_escort_need_time(GET_A(kAttrEscortBoatType), time);
        if (ret) {
            ERROR_TLOG("P:%u login err when calc_escort_need_time:%d[no_such_ship_type]", 
					player->userid, ret);
        } else {
            if (GET_A(kAttrEscortLastStarttime) + time > NOW() 
					&& GET_A(kAttrEscortOnlineId) != g_online_id) {
                switchproto::cs_sw_req_svr_erase_player_escort_info  sw_msg;
                sw_msg.set_old_online_id(GET_A(kAttrEscortOnlineId));
                sw_msg.set_uid(player->userid);
				sw_msg.set_u_create_tm(player->create_tm);
                g_switch->send_msg(
						NULL, 0, 0,
						sw_cmd_sw_req_erase_player_escort_info, 
						sw_msg);

                g_escort_mgr.add_player_to_escort(player, onlineproto::SWITCH_SVR);
                g_escort_mgr.add_player_rob_info(player);
				g_escort_mgr.sync_player_info_other(
						player->userid, player->create_tm, 
						onlineproto::ES_SHIP_APPEAR_SWCH_SVR);

            } else if (GET_A(kAttrEscortLastStarttime) + time < NOW()) {
				std::set<uint64_t> uids;
				uint64_t role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
				uids.insert(role_key);
				EscortUtils::clear_other_attr(uids, true);
			}
        }
	}
	if (GET_A(kAttrEscortLastStarttime) == 0 && GET_A(kAttrEscortReward)) {
		g_escort_mgr.deal_with_after_fin_escort_when_login(player);
	}
	return 0;
}

uint32_t EscortUtils::get_optional_route_id(const std::set<route_info_t>& route_s)
{
	uint32_t route_id = 0;
	std::set<route_info_t>::iterator it;
	it = route_s.begin();
	for (; it != route_s.end(); ++it) {
		if (it->num < ROUTE_NUM_LIMIT) {
			route_id = it->id;	
			break;
		}
	}
	return route_id;
}
