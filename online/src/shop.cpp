#include "player.h"
#include "global_data.h"
#include "shop.h"
#include "utils.h"
#include "attr_utils.h"
#include "item.h"
#include "pet.h"
#include "pet_utils.h"
#include "exchange_conf.h"
#include "statlogger/statlogger.h"
#include "escort.h"
#include "escort_utils.h"
#include "player_utils.h"
#include "prize.h"
#include "equip_utils.h"
#include "service.h"
#include "map_utils.h"
#include "duplicate_utils.h"
#include "task_utils.h"

void ShopRegisterFun::register_buy_product_proc_func()
{
	after_buy_pd_map_.clear();
	before_buy_pd_map_.clear();
	after_buy_pd_map_[(uint32_t)FIN_ESCORT_IMMEDIATELY] = after_clear_escort_last_tm;
	before_buy_pd_map_[(uint32_t)FIN_ESCORT_IMMEDIATELY] = before_clear_escort_last_tm;

	after_buy_pd_map_[(uint32_t)GOLD_VIP_180_DAYS] = after_buy_gold_vip_180_days;
	before_buy_pd_map_[(uint32_t)GOLD_VIP_180_DAYS] = before_buy_gold_vip_180_days;

	after_buy_pd_map_[(uint32_t)GOLD_VIP_30_DAYS_0611_DISCOUNT] = after_buy_gold_vip_30_days;
	//before_buy_pd_map_[(uint32_t)GOLD_VIP_30_DAYS_0611_DISCOUNT] = before_buy_gold_vip_30_days_0611;

	after_buy_pd_map_[(uint32_t)GOLD_VIP_30_DAYS] = after_buy_gold_vip_30_days;
	before_buy_pd_map_[(uint32_t)GOLD_VIP_30_DAYS] = before_buy_gold_vip_30_days;

	after_buy_pd_map_[(uint32_t)SILVER_VIP_30_DAYS] = after_buy_silver_vip_30_days;
	before_buy_pd_map_[(uint32_t)SILVER_VIP_30_DAYS] = before_buy_silver_vip_30_days;

	before_buy_pd_map_[(uint32_t)BUY_SWIM_CHAIR] = before_buy_swim_chair;

	after_buy_pd_map_[(uint32_t)MONTH_CARD_30_DAYS] = after_buy_month_card_30_days;
	before_buy_pd_map_[(uint32_t)MONTH_CARD_30_DAYS] = before_buy_month_card_30_days;

	after_buy_pd_map_[(uint32_t)BUY_LUXURY_SUIT] = after_buy_luxury_suit;
	before_buy_pd_map_[(uint32_t)OPEN_HM_BOX_POS_3] = before_buy_open_hm_box_pos_3;
	before_buy_pd_map_[(uint32_t)OPEN_HM_BOX_POS_4] = before_buy_open_hm_box_pos_4;
	before_buy_pd_map_[(uint32_t)OPEN_HM_BOX_POS_5] = before_buy_open_hm_box_pos_5;

	after_buy_pd_map_[(uint32_t)OPEN_HM_BOX_POS_3] = after_buy_open_hm_box_pos_3;
	after_buy_pd_map_[(uint32_t)OPEN_HM_BOX_POS_4] = after_buy_open_hm_box_pos_4;
	after_buy_pd_map_[(uint32_t)OPEN_HM_BOX_POS_5] = after_buy_open_hm_box_pos_5;

	before_buy_pd_map_[(uint32_t)OPEN_EXERCISE_POS_3] = before_buy_open_exercise_3;
	before_buy_pd_map_[(uint32_t)OPEN_EXERCISE_POS_4] = before_buy_open_exercise_4;
	before_buy_pd_map_[(uint32_t)OPEN_EXERCISE_POS_5] = before_buy_open_exercise_5;

	after_buy_pd_map_[(uint32_t)OPEN_EXERCISE_POS_3] = after_buy_open_exercise_3;
	after_buy_pd_map_[(uint32_t)OPEN_EXERCISE_POS_4] = after_buy_open_exercise_4;
	after_buy_pd_map_[(uint32_t)OPEN_EXERCISE_POS_5] = after_buy_open_exercise_5;

	after_buy_pd_map_[(uint32_t)MAYIN_BUCKET_RECHARGE_ENERGY] = after_mayin_recharge_energy;
	before_buy_pd_map_[(uint32_t)MAYIN_DEFEAT_EMPIRE_DUP_1] = before_mayin_defeat_empire_dup1;
	after_buy_pd_map_[(uint32_t)MAYIN_DEFEAT_EMPIRE_DUP_1] = after_mayin_defeat_empire_dup1;
	before_buy_pd_map_[(uint32_t)MAYIN_DEFEAT_EMPIRE_DUP_2] = before_mayin_defeat_empire_dup2;
	after_buy_pd_map_[(uint32_t)MAYIN_DEFEAT_EMPIRE_DUP_2] = after_mayin_defeat_empire_dup2;
	before_buy_pd_map_[(uint32_t)MAYIN_DEFEAT_EMPIRE_DUP_3] = before_mayin_defeat_empire_dup3;
	after_buy_pd_map_[(uint32_t)MAYIN_DEFEAT_EMPIRE_DUP_3] = after_mayin_defeat_empire_dup3;
	after_buy_pd_map_[(uint32_t)GRATEFULL_GIFT] = after_buy_gratefull_gift;

	after_buy_pd_map_[(uint32_t)ERASE_MINE_COLD_TIME] = after_buy_erase_mine_cold_time;
	after_buy_pd_map_[(uint32_t)MINE_BUY_CHALLENGE_TIMES] = after_mine_buy_challenge_times;

	before_buy_pd_map_[(uint32_t)BUY_SUMMER_WEEKLY_SIGNED] = before_buy_summer_weekly_signed;

	
	

#if 0
    check_before_market_buy_map_[MARKET_TYPE_DAILY] = check_daily_market_before_buy;
    update_after_market_buy_map_[MARKET_TYPE_DAILY] = update_daily_market_after_buy;

    check_before_market_buy_map_[MARKET_TYPE_ELEM_DUP] = check_elem_dup_market_before_buy;
    update_after_market_buy_map_[MARKET_TYPE_ELEM_DUP] = update_elem_dup_market_after_buy;

    check_before_market_buy_map_[MARKET_TYPE_ARENA] = check_arena_market_before_buy;
    update_after_market_buy_map_[MARKET_TYPE_ARENA] = update_arena_market_after_buy;

    check_before_market_buy_map_[MARKET_TYPE_EXPED] = check_exped_market_before_buy;
    update_after_market_buy_map_[MARKET_TYPE_EXPED] = update_exped_market_after_buy;

    check_before_market_buy_map_[MARKET_TYPE_FAMILY] = check_family_market_before_buy;
    update_after_market_buy_map_[MARKET_TYPE_FAMILY] = update_family_market_after_buy;
#endif
}

//调用前需保证product_id是有效的商品id
uint32_t ShopRegisterFun::call_reg_func_after_buy_product(player_t* player, uint32_t product_id)
{
	if (after_buy_pd_map_.count(product_id)) {
		return (after_buy_pd_map_.find(product_id)->second) (player);
	}
	return 0;
}

//调用前需保证product_id是有效的商品id
uint32_t ShopRegisterFun::call_reg_func_before_buy_product(player_t* player, uint32_t product_id)
{
	if (before_buy_pd_map_.count(product_id)) {
		return (before_buy_pd_map_.find(product_id)->second) (player);
	}
	return 0;
}

uint32_t ShopRegisterFun::after_clear_escort_last_tm(player_t* player)
{
	g_escort_mgr.sync_player_info_other(
			player->userid, player->create_tm, 
			onlineproto::ES_SHIP_DISAPPEAR);

	g_escort_mgr.inform_player_escort_end(player->userid, player->create_tm);
	g_escort_mgr.del_player_from_escort(player->userid, player->create_tm);
	std::set<uint64_t> role_s;
	uint64_t role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	role_s.insert(role_key);
	EscortUtils::clear_other_attr(role_s);
	Utils::write_msglog_new(player->userid, "功能", "运宝", "成功清除运送剩余时间");
	return 0;
}

uint32_t ShopRegisterFun::before_clear_escort_last_tm(player_t *player) 
{
	uint64_t role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	escort_info_t* es_ptr = g_escort_mgr.get_user_escort_info(role_key);
	if (es_ptr == NULL) {
		return cli_err_buy_attr_to_fin_escort;
	}
	if (es_ptr->in_pk_flag) {
		ERROR_TLOG("be robed now;uid=[%u],create_tm=[%u]pk_flag=[%u]", 
				player->userid, player->create_tm, es_ptr->in_pk_flag);
		return cli_err_can_fin_escort_when_be_robbed;
	}
	return 0;
}

uint32_t ShopRegisterFun::after_buy_gold_vip_180_days(player_t* player)
{
	uint32_t last_gold_begin_time = GET_A(kAttrGoldVipFirstBuyTime); 
	uint32_t last_gold_end_time = GET_A(kAttrGoldVipEndTime);
	uint32_t last_silver_begin_time = GET_A(kAttrSilverVipFirstBuyTime);
	uint32_t last_silver_end_time = GET_A(kAttrSilverVipEndTime);
	deal_first_buy_time(player, BUY_GOLD_180_VIP);

	if (GET_A(kAttrIsYearlyVip) == 0) {
		SET_A(kAttrIsYearlyVip, 1);
		//还在本次活动中，将年费累计激活礼包设置为可以领取状态
		if (GET_A(kAttrVipGiftBagGetFlag) == commonproto::VIP_GIFT_CAN_NOT_GET) {
			SET_A(kAttrVipGiftBagGetFlag, commonproto::VIP_GIFT_CAN_GET);
		}
	}
	SET_A(kAttrThisActivityGoldVipStartTime, NOW());
	change_vip_end_time(player, CHANGE_GOLD_180_VIP);
	std::string title = "恭喜激活黄金勋章180天特权";
	std::string content("尊敬的高富帅，激活黄金勋章180天, "
			"快去看看吧\n<u><a href='event:vip,2'><font color='#000000'>点此查看详细</font></u></a>");
	PlayerUtils::generate_new_mail(player, title, content);	

	//落vip的数据
	set_buy_vip_trans(player, last_gold_begin_time,
			last_gold_end_time,
			last_silver_begin_time,
			last_silver_end_time, BUY_GOLD_180_VIP);
	return 0;
}

uint32_t ShopRegisterFun::before_buy_gold_vip_180_days(player_t* player)
{
	return 0;
}

uint32_t ShopRegisterFun::after_buy_gold_vip_30_days(player_t* player)
{
	uint32_t last_gold_begin_time = GET_A(kAttrGoldVipFirstBuyTime); 
	uint32_t last_gold_end_time = GET_A(kAttrGoldVipEndTime);
	uint32_t last_silver_begin_time = GET_A(kAttrSilverVipFirstBuyTime);
	uint32_t last_silver_end_time = GET_A(kAttrSilverVipEndTime);
	deal_first_buy_time(player, BUY_GOLD_VIP);
	
	ADD_A(kAttrGoldVipRechargeCnt, 1);
	//本次活动中，充值 黄金勋章 累计达到6次，则
	if (GET_A(kAttrGoldVipRechargeCnt) >= 6 && GET_A(kAttrIsYearlyVip) == 0) {
		SET_A(kAttrIsYearlyVip, 1);
		//还在本次活动中，将年费累计激活礼包设置为可以领取状态
		if (GET_A(kAttrVipGiftBagGetFlag) == commonproto::VIP_GIFT_CAN_NOT_GET) {
			SET_A(kAttrVipGiftBagGetFlag, commonproto::VIP_GIFT_CAN_GET);
		}
	}
	//若是本次活动中，第一次充值 黄金30天,则记录下充值开始的时间戳
	if (GET_A(kAttrGoldVipRechargeCnt) == 1) {
		SET_A(kAttrThisActivityGoldVipStartTime, NOW());
	}
	change_vip_end_time(player, CHANGE_GOLD_30_VIP);
	/*
	if (GET_A(kAttrGoldVipEndTime) > NOW()) {
		SET_A(kAttrGoldVipEndTime, GET_A(kAttrGoldVipEndTime) + 30 * DAY_SECS);
	} else {
		SET_A(kAttrGoldVipEndTime, NOW() + 30 * DAY_SECS);
	}
	//购买黄金勋章的同时，系统默认也一并购买了白银勋章
	if (GET_A(kAttrSilverVipEndTime) > NOW()) {
		SET_A(kAttrSilverVipEndTime, GET_A(kAttrSilverVipEndTime) + 30 * DAY_SECS);
	} else {
		SET_A(kAttrSilverVipEndTime, NOW() + 30 * DAY_SECS);
	}
	*/

	//发封通知邮件
	std::string title = "恭喜激活黄金勋章30天特权";
	std::string content("尊敬的高富帅，激活黄金勋章30天, "
			"快去看看吧\n<u><a href='event:vip,2'><font color='#000000'>点此查看详细</font></u></a>");
	PlayerUtils::generate_new_mail(player, title, content);	

	//落vip的数据
	set_buy_vip_trans(player, last_gold_begin_time,
			last_gold_end_time,
			last_silver_begin_time,
			last_silver_end_time, BUY_GOLD_VIP);

	return 0;
}

uint32_t ShopRegisterFun::before_buy_gold_vip_30_days(player_t* player)
{
	return 0;
}

uint32_t ShopRegisterFun::before_buy_gold_vip_30_days_0611(player_t* player)
{
    if (GET_A(kAttrGoldVipActivity150611Tasks) < 3
        || GET_A(kAttrGoldVipActivity150611Pets) < 3
        || GET_A(kAttrGoldVipActivity150611Equips) < 1) {
        return cli_err_shop_buy_cond_not_achieve;
    }
    if (GET_A(kAttrGoldVipActivity150611BuyLimit) >= GET_A_MAX(kAttrGoldVipActivity150611BuyLimit)) {
        return cli_err_shop_exceed_forever_limit;
    }
    return 0;
}

uint32_t ShopRegisterFun::after_buy_silver_vip_30_days(player_t* player)
{
	uint32_t last_gold_begin_time = GET_A(kAttrGoldVipFirstBuyTime); 
	uint32_t last_gold_end_time = GET_A(kAttrGoldVipEndTime);
	uint32_t last_silver_begin_time = GET_A(kAttrSilverVipFirstBuyTime);
	uint32_t last_silver_end_time = GET_A(kAttrSilverVipEndTime);
	deal_first_buy_time(player, BUY_SILVER_VIP);

	//已经是白银勋章 vip,且还在有效期内
	change_vip_end_time(player, CHANGE_SILIVER_VIP);

	//uint32_t last_silver_begin_time = 0, last_silver_end_time = 0;
	//set_buy_vip_trans(player, );
	/*
	if (GET_A(kAttrSilverVipEndTime) > NOW()) {
		SET_A(kAttrSilverVipEndTime, GET_A(kAttrSilverVipEndTime) + 30 * DAY_SECS);
	} else {
	//还不是白银勋章 vip, 或者勋章过期了
		if (GET_A(kAttrGoldVipEndTime) > NOW()) {  
			//已经是黄金勋章 vip,且还在有效期内
			SET_A(kAttrSilverVipEndTime, GET_A(kAttrGoldVipEndTime) + 30 * DAY_SECS);
		} else {
			SET_A(kAttrSilverVipEndTime, NOW() + 30 * DAY_SECS);
		}
	}
	*/
	//发封通知邮件
	std::string title = "恭喜激活白银勋章30天特权";
	std::string content("尊敬的高富帅，白银勋章30天, "
			"快去看看吧\n<u><a href='event:vip,2'><font color='#000000'>点此查看详细</font></u></a>");
	PlayerUtils::generate_new_mail(player, title, content);	

	//落vip的数据
	set_buy_vip_trans(player, last_gold_begin_time,
			last_gold_end_time,
			last_silver_begin_time,
			last_silver_end_time, BUY_SILVER_VIP);
	return 0;
}

uint32_t ShopRegisterFun::after_buy_luxury_suit(player_t* player)
{
	//同步map信息
    MapUtils::sync_map_player_info(player, commonproto::PLAYER_GET_LUXURY_SUIT);
	if (GET_A(kAttrBuyLuxurySuitTimeStamp) == 0) {
		SET_A(kAttrBuyLuxurySuitTimeStamp, NOW());
	}
	return 0;
}

uint32_t ShopRegisterFun::after_buy_month_card_30_days(player_t* player)
{
	int ret = 0;
	uint32_t prizeid = 2051;
	//已经是白银勋章 vip,且还在有效期内
	SET_A(kAttrMonthCardEndTime, NOW() + 30 * DAY_SECS);
	//返钻石券
	onlineproto::sc_0x0112_notify_get_prize noti_;
	noti_.Clear();

	ret = transaction_proc_prize(player, prizeid, noti_,
			commonproto::PRIZE_REASON_MONTH_CARD_REWARD,onlineproto::SYNC_REASON_PRIZE_ITEM,NO_ADDICT_DETEC);
	if (ret) {
		return ret;
	}

	//发系统消息
	static char msg[256];
	snprintf(msg, sizeof(msg), " [pi= + %u + | + %u + ] %s [/pi]购买了[cl=0x00DE00]月卡[/cl],获得了高达六倍的[cl=0xE500FF]巨额返利[/cl]!", 
			player->userid, player->create_tm, player->nick);

	std::string noti_msg; 
	noti_msg.assign(msg);
	onlineproto::sc_0x012A_system_notice msg_out_;
	msg_out_.set_type(0);
	msg_out_.set_content(msg);
	std::vector<uint32_t> svr_ids;
	svr_ids.push_back(g_server_id);
	Utils::switch_transmit_msg(
			switchproto::SWITCH_TRANSMIT_SERVERS,
			cli_cmd_cs_0x012A_system_notice, msg_out_,
			0, &svr_ids);

	send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_);
	return 0;
}

uint32_t ShopRegisterFun::before_buy_silver_vip_30_days(player_t* player)
{
	return 0;
}

uint32_t ShopRegisterFun::before_buy_month_card_30_days(player_t* player)
{
	//已经是有月卡，且还在有效期内
	if (GET_A(kAttrMonthCardEndTime) > NOW()) {
		return cli_err_repeat_get_month_card;
	} 
	return 0;
}

uint32_t ShopRegisterFun::before_buy_swim_chair(player_t* player)
{
	attr_type_t buy_chair_cnt = kDailySwimChairAmount;
	uint32_t count = GET_A(buy_chair_cnt);
	uint32_t total = 0;

	if(is_gold_vip(player)){
		total = 3;
	} else if(is_silver_vip(player)){
		total = 1;
	}

	if(count >= total){
		return cli_err_shop_exceed_daily_limit;
	}

	return 0;
}

uint32_t ShopRegisterFun::before_buy_open_hm_box_pos_3(player_t* player)
{
	//若寻宝位4，5中有且仅有一个位开启
	//此时需要白银vip才能开第3位
	if ((GET_A(kAttrFindBoxPos4IsOpen) && GET_A(kAttrFindBoxPos5IsOpen) == 0)
			|| (GET_A(kAttrFindBoxPos4IsOpen) == 0 && GET_A(kAttrFindBoxPos5IsOpen))) {
		if (is_vip(player) == false) {
			return cli_err_need_vip_open_exercise_pos;
		}
	} else if (GET_A(kAttrFindBoxPos4IsOpen) && GET_A(kAttrFindBoxPos5IsOpen)) {
		//若寻宝位4，5都开启	
		//此时需要黄金vip才能开第3位
		if (is_gold_vip(player) == false) {
			return cli_err_need_gold_vip_open_exercise_pos;
		}
	}
	//剩下的一种情况就是寻宝位4，5都没有开启，此时无需vip也能开启
	if (GET_A(kAttrFindBoxPos3IsOpen) == 1) {
		return cli_err_exericise_pos_has_open;
	}
	return 0;
}

uint32_t ShopRegisterFun::after_buy_open_hm_box_pos_3(player_t* player)
{
	SET_A(kAttrFindBoxPos3IsOpen, 1);
	return 0;
}

uint32_t ShopRegisterFun::before_buy_open_exercise_3(player_t* player)
{
	//若寻宝位4，5中有且仅有一个位开启
	//此时需要白银vip才能开第3位
	if ((GET_A(kAttrExercisePos4IsOpen) && GET_A(kAttrExercisePos5IsOpen) == 0)
			|| (GET_A(kAttrExercisePos4IsOpen) == 0 && GET_A(kAttrExercisePos5IsOpen))) {
		if (is_vip(player) == false) {
			return cli_err_need_vip_open_exercise_pos;
		}
	} else if (GET_A(kAttrExercisePos4IsOpen) && GET_A(kAttrExercisePos5IsOpen)) {
		//若寻宝位4，5都开启	
		//此时需要黄金vip才能开第3位
		if (is_gold_vip(player) == false) {
			return cli_err_need_gold_vip_open_exercise_pos;
		}
	}
	//剩下的一种情况就是寻宝位4，5都没有开启，此时无需vip也能开启
	if (GET_A(kAttrExercisePos3IsOpen) == 1) {
		return cli_err_exericise_pos_has_open;
	}
	return 0;
}

uint32_t ShopRegisterFun::after_buy_open_exercise_3(player_t* player)
{
	SET_A(kAttrExercisePos3IsOpen, 1);
	return 0;
}

uint32_t ShopRegisterFun::before_buy_open_hm_box_pos_4(player_t* player)
{
	//如果锻炼位3已经开启，且锻炼位5没有开启，或者锻炼位3已经未开启，锻炼位5已经开启
	//此时需要白银vip才能开第四位
	if ((GET_A(kAttrFindBoxPos3IsOpen) && GET_A(kAttrFindBoxPos5IsOpen) == 0)
			|| (GET_A(kAttrFindBoxPos3IsOpen) == 0 && GET_A(kAttrFindBoxPos5IsOpen))) {
		if (is_vip(player) == false) {
			return cli_err_need_vip_open_exercise_pos;
		}
	} else if (GET_A(kAttrFindBoxPos3IsOpen) && GET_A(kAttrFindBoxPos5IsOpen)) {
		//如果锻炼位3已经开启，且锻炼位5也已经开启	
		//此时需要黄金vip才能开第四位
		if (is_gold_vip(player) == false) {
			return cli_err_need_gold_vip_open_exercise_pos;
		}
	}
	//剩下的一种情况就是锻炼位3，5都没有开启，此时无需vip也能开启
	
	if (GET_A(kAttrFindBoxPos4IsOpen) == 1) {
		return cli_err_exericise_pos_has_open;
	}
	return 0;
}

uint32_t ShopRegisterFun::after_buy_open_hm_box_pos_4(player_t* player)
{
	SET_A(kAttrFindBoxPos4IsOpen, 1);
	return 0;
}

uint32_t ShopRegisterFun::before_buy_open_exercise_4(player_t* player)
{
	if ((GET_A(kAttrExercisePos3IsOpen) && GET_A(kAttrExercisePos5IsOpen) == 0)
			|| (GET_A(kAttrExercisePos3IsOpen) == 0 && GET_A(kAttrExercisePos5IsOpen))) {
		if (is_vip(player) == false) {
			return cli_err_need_vip_open_exercise_pos;
		}
	} else if (GET_A(kAttrExercisePos3IsOpen) && GET_A(kAttrExercisePos5IsOpen)) {
		if (is_gold_vip(player) == false) {
			return cli_err_need_gold_vip_open_exercise_pos;
		}
	}
	
	if (GET_A(kAttrExercisePos4IsOpen) == 1) {
		return cli_err_exericise_pos_has_open;
	}
	return 0;
}

uint32_t ShopRegisterFun::after_buy_open_exercise_4(player_t* player)
{
	SET_A(kAttrExercisePos4IsOpen, 1);
	return 0;
}

uint32_t ShopRegisterFun::before_buy_open_hm_box_pos_5(player_t* player)
{
	if ((GET_A(kAttrFindBoxPos3IsOpen) && GET_A(kAttrFindBoxPos4IsOpen) == 0)
			|| (GET_A(kAttrFindBoxPos3IsOpen) == 0 && GET_A(kAttrFindBoxPos4IsOpen))) {
		if (is_vip(player) == false) {
			return cli_err_need_vip_open_exercise_pos;
		}
	} else if (GET_A(kAttrFindBoxPos3IsOpen) && GET_A(kAttrFindBoxPos4IsOpen)) {
		if (is_gold_vip(player) == false) {
			return cli_err_need_gold_vip_open_exercise_pos;
		}
	}
	if (GET_A(kAttrFindBoxPos5IsOpen) == 1) {
		return cli_err_exericise_pos_has_open;
	}
	return 0;
}

uint32_t ShopRegisterFun::after_buy_open_hm_box_pos_5(player_t* player)
{
	SET_A(kAttrFindBoxPos5IsOpen, 1);
	return 0;
}

uint32_t ShopRegisterFun::before_buy_open_exercise_5(player_t* player)
{
	if ((GET_A(kAttrExercisePos3IsOpen) && GET_A(kAttrExercisePos4IsOpen) == 0)
			|| (GET_A(kAttrExercisePos3IsOpen) == 0 && GET_A(kAttrExercisePos4IsOpen))) {
		if (is_vip(player) == false) {
			return cli_err_need_vip_open_exercise_pos;
		}
	} else if (GET_A(kAttrExercisePos3IsOpen) && GET_A(kAttrExercisePos4IsOpen)) {
		if (is_gold_vip(player) == false) {
			return cli_err_need_gold_vip_open_exercise_pos;
		}
	}
	if (GET_A(kAttrExercisePos5IsOpen) == 1) {
		return cli_err_exericise_pos_has_open;
	}
	return 0;
}

uint32_t ShopRegisterFun::after_buy_open_exercise_5(player_t* player)
{
	SET_A(kAttrExercisePos5IsOpen, 1);
	return 0;
}

uint32_t ShopRegisterFun::after_buy_erase_mine_cold_time(player_t* player)
{
	SET_A(kAttrMineFightRefreshCdStartTm, 0);
	return 0;
}

uint32_t ShopRegisterFun::after_mine_buy_challenge_times(player_t* player)
{
	uint32_t left_times = GET_A(kDailyMineBuyChallengeTimes) + 
		commonproto::MINE_DAILY_OCCUPY_MINE_TIMES - GET_A(kDailyMineHasChallengeTimes);
	if (GET_A(kDailyMineHasChallengeTimes) >= 
			GET_A(kDailyMineBuyChallengeTimes) + commonproto::MINE_DAILY_OCCUPY_MINE_TIMES )
	{
		left_times = 0;
	}

	SET_A(kDailyMineLeftChallengeTimes, left_times);
	return 0;
}

uint32_t ShopRegisterFun::before_buy_summer_weekly_signed(player_t* player)
{
	uint32_t free_times = GET_A(kDailySummerWeeklySignedTimes);
	uint32_t buy_cnt = GET_A(kDailySummerWeeklyBuySignCnt);
	if (free_times) {
		return cli_err_summer_must_use_free_times;
	}
	if (buy_cnt) {
		return cli_err_summer_must_use_buy_cnt;
	}
	uint32_t has_signed_total_times= GET_A(kAttrSummerHasSignedTotalTimes);
	if (!(has_signed_total_times >= 0 && has_signed_total_times <= 6)) {
		return cli_err_summer_signed_has_fin_no_need_buy;
	}

	uint32_t key = TM_CONF_KEY_SUMMER_WEEKLY_SIGN;
	if (g_time_config.count(key) == 0) {
		return cli_err_sys_err;
	}
	TIME_CONFIG_LIMIT_T& time_config_map = g_time_config.find(key)->second;
	uint32_t index = 0;
	bool found_valid_time = false;
	for (uint32_t i = 1; i <= time_config_map.size(); ++i) {
		if (TimeUtils::is_time_valid(NOW(), key, i)) {
			index = i;
			found_valid_time = true;
			break;
		}
	}
	uint32_t has_open_days = 0;
	if (found_valid_time) {
		time_limit_t& time_info = time_config_map[index];
		//计算本活动已经开启的天数
		has_open_days = TimeUtils::get_days_between(time_info.start_time, NOW()) + 1;
		TRACE_TLOG("Buy cnt:Summer Weekly Signed,open_days=[%u],index=[%u],start_time=%u", has_open_days, index, time_info.start_time);
	}
	if (has_open_days <= has_signed_total_times) {
		return cli_err_summer_no_need_signed;
	}

	return 0;
}

uint32_t ShopRegisterFun::after_mayin_recharge_energy(player_t* player)
{
	DupUtils::set_mayin_train_gift_state(player);
	return 0;	
}

uint32_t ShopRegisterFun::before_mayin_defeat_empire_dup1(player_t* player)
{
	const duplicate_t* dup = g_duplicate_conf_mgr.find_duplicate(901);
	if (dup == NULL) {
		return cli_err_duplicate_id_not_found;
	}
	return 0;
}

uint32_t ShopRegisterFun::before_mayin_defeat_empire_dup2(player_t* player)
{
	const duplicate_t* dup = g_duplicate_conf_mgr.find_duplicate(902);
	if (dup == NULL) {
		return cli_err_duplicate_id_not_found;
	}
	return 0;
}

uint32_t ShopRegisterFun::before_mayin_defeat_empire_dup3(player_t* player)
{
	const duplicate_t* dup = g_duplicate_conf_mgr.find_duplicate(903);
	if (dup == NULL) {
		return cli_err_duplicate_id_not_found;
	}
	return 0;
}

uint32_t ShopRegisterFun::after_mayin_defeat_empire_dup1(player_t* player)
{
	DupUtils::use_diamond_buy_pass_dup(player, 901);
	return 0;
}

uint32_t ShopRegisterFun::after_mayin_defeat_empire_dup2(player_t* player)
{
	DupUtils::use_diamond_buy_pass_dup(player, 902);
	return 0;
}

uint32_t ShopRegisterFun::after_mayin_defeat_empire_dup3(player_t* player)
{
	DupUtils::use_diamond_buy_pass_dup(player, 903);
	return 0;
}

uint32_t ShopRegisterFun::after_buy_gratefull_gift(player_t* player)
{
	TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_GRATEFULL_HALL, 1);
	return 0;
}

uint32_t ShopRegisterFun::check_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();

    std::map<uint32_t, ol_market_item_t> *shop_items;
    switch (market)  {
    case MARKET_TYPE_DAILY:
        shop_items = player->daily_shop_items;
        break;
    case MARKET_TYPE_FAMILY:
        shop_items = player->family_shop_items;
        break;
    case MARKET_TYPE_ARENA:
        shop_items = player->arena_shop_items;
        break;
    case MARKET_TYPE_EXPED:
        shop_items = player->exped_shop_items;
        break;
    case MARKET_TYPE_NIGHT_RAID:
        shop_items = player->night_raid_shop_items;
        break;
    case MARKET_TYPE_ELEM_DUP:
        shop_items = player->elem_dup_shop_items;
        break;
	case MARKET_TYPE_SMELT_MONEY:
		shop_items = player->smelter_money_shop_items;
		break;
	case MARKET_TYPE_SMELT_GOLD:
		shop_items = player->smelter_gold_shop_items;
		break;
    default:
        return cli_err_invalid_market_type;
    }

    std::map<uint32_t, ol_market_item_t>::iterator iter = shop_items->find(pd_id);
    if (iter == shop_items->end()) {
        return cli_err_product_not_exist;
    }

    if (iter->second.count < buy_cnt) {
        return cli_err_shop_product_not_enough;
    }

    return 0;
}

uint32_t ShopRegisterFun::update_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();
    dbproto::user_raw_data_type_t db_data_type;

    std::map<uint32_t, ol_market_item_t> *shop_items;
    switch (market)  {
    case MARKET_TYPE_DAILY:
        shop_items = player->daily_shop_items;
        db_data_type = dbproto::DAILY_SHOP_PRODUCT;
        break;
    case MARKET_TYPE_FAMILY:
        shop_items = player->family_shop_items;
        db_data_type = dbproto::FAMILY_SHOP_PRODUCT;
        break;
    case MARKET_TYPE_ARENA:
        shop_items = player->arena_shop_items;
        db_data_type = dbproto::ARENA_SHOP_PRODUCT;
        break;
    case MARKET_TYPE_EXPED:
        shop_items = player->exped_shop_items;
        db_data_type = dbproto::EXPED_SHOP_PRODUCT;
        break;
    case MARKET_TYPE_NIGHT_RAID:
        shop_items = player->night_raid_shop_items;
        db_data_type = dbproto::NIGHT_RAID_SHOP_PRODUCT;
        break;
    case MARKET_TYPE_ELEM_DUP:
        shop_items = player->elem_dup_shop_items;
        db_data_type = dbproto::ELEM_DUP_SHOP_PRODUCT;
        break;
	case MARKET_TYPE_SMELT_MONEY:
		shop_items = player->smelter_money_shop_items;
		db_data_type = dbproto::SMELTER_MONEY;
		break;
	case MARKET_TYPE_SMELT_GOLD:
		shop_items = player->smelter_gold_shop_items;
		db_data_type = dbproto::SMELTER_GOLD;
		break;
    default:
        return cli_err_invalid_market_type;
    }

    std::map<uint32_t, ol_market_item_t>::iterator iter = shop_items->find(pd_id);
    if (iter != shop_items->end()) {
        uint32_t left_cnt = iter->second.count;

        if (left_cnt <= buy_cnt) {
            left_cnt = 0;
        } else {
            left_cnt = left_cnt - buy_cnt;
        }

        iter->second.count = left_cnt;

        onlineproto::sc_0x0131_shop_refresh noti;
        noti.set_market((onlineproto::market_type_t)market);
        commonproto::market_item_info_t *item_info = noti.mutable_item_info();
        FOREACH((*shop_items), iter) {
            commonproto::market_item_t *item = item_info->add_items();
            item->set_item_id(iter->second.item_id);
            item->set_count(iter->second.count);
        }
        // 同步到db
        PlayerUtils::update_user_raw_data(
                player->userid, player->create_tm, 
                db_data_type, noti.item_info(), "0");
    }

    return 0;
}

#if 0
uint32_t ShopRegisterFun::check_daily_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    //uint32_t buy_cnt = buy_in_.buy_cnt();
    attr_type_t pd_attr = (attr_type_t)(buy_in_.attr_id());
    attr_type_t cnt_attr = (attr_type_t)(pd_attr+1);

     if (market != MARKET_TYPE_DAILY) {
         return 0;
     }

     //属性值是否正确
     if (pd_attr < kAttrSpShopProduct1Id || pd_attr > kAttrSpShopProduct10Id
             || (pd_attr - kAttrSpShopProduct1Id) % 2 != 0) {
         return cli_err_invalid_market_attr;
     } 
     //属性值存的商品ID是不是要购买的商品ID
     uint32_t svr_pd_id = GET_A(pd_attr);
     uint32_t pd_cnt = GET_A(cnt_attr);
     if (pd_id != svr_pd_id || svr_pd_id == 0) {
         return cli_err_invalid_market_attr;
     }
     if (pd_cnt == 0) {
         return cli_err_product_sell_out;
     }

     //need_set_shop_attr = true;

    return 0;
}

uint32_t ShopRegisterFun::update_daily_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    if (market != MARKET_TYPE_DAILY) {
        return 0;
    }

    attr_type_t pd_attr = (attr_type_t)(buy_in_.attr_id());
    attr_type_t cnt_attr = (attr_type_t)(pd_attr+1);
    SET_A(cnt_attr, 0);
    return 0;
}

uint32_t ShopRegisterFun::check_elem_dup_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();

     if (market != MARKET_TYPE_ELEM_DUP) {
         return 0;
     }

    // 元素挑战商店
    std::map<uint32_t, ol_market_item_t>::iterator iter = 
        player->elem_dup_shop_items->find(pd_id);
    if (iter == player->elem_dup_shop_items->end()) {
        return cli_err_product_sell_out;
    }

    if (iter->second.count < buy_cnt) {
        return cli_err_shop_product_not_enough;
    }

    return 0;
}

uint32_t ShopRegisterFun::update_elem_dup_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();
     if (market != MARKET_TYPE_ELEM_DUP) {
         return 0;
     }

    // 元素挑战商店
    std::map<uint32_t, ol_market_item_t>::iterator iter = 
        player->elem_dup_shop_items->find(pd_id);
    if (iter != player->elem_dup_shop_items->end()) {
        uint32_t left_cnt = iter->second.count;

        if (left_cnt <= buy_cnt) {
            left_cnt = 0;
        } else {
            left_cnt = left_cnt - buy_cnt;
        }

        iter->second.count = left_cnt;

        onlineproto::sc_0x0131_shop_refresh noti;
        noti.set_market(onlineproto::MARKET_TYPE_ELEM_DUP);
        commonproto::market_item_info_t *item_info = noti.mutable_item_info();
        FOREACH(*(player->elem_dup_shop_items), iter) {
            commonproto::market_item_t *item = item_info->add_items();
            item->set_item_id(iter->second.item_id);
            item->set_count(iter->second.count);
        }
        // 同步到db
        PlayerUtils::update_user_raw_data(
                player->userid, player->create_tm, 
                dbproto::ELEM_DUP_SHOP_PRODUCT, noti.item_info(), "0");
    }

    return 0;
}

uint32_t ShopRegisterFun::check_arena_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();

     if (market != MARKET_TYPE_ARENA) {
         return 0;
     }

    std::map<uint32_t, ol_market_item_t>::iterator iter = 
        player->arena_shop_items->find(pd_id);
    if (iter == player->arena_shop_items->end()) {
        return cli_err_product_sell_out;
    }

    if (iter->second.count < buy_cnt) {
        return cli_err_shop_product_not_enough;
    }

    return 0;
}

uint32_t ShopRegisterFun::update_arena_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();
     if (market != MARKET_TYPE_ARENA) {
         return 0;
     }

    std::map<uint32_t, ol_market_item_t>::iterator iter = 
        player->arena_shop_items->find(pd_id);
    if (iter != player->arena_shop_items->end()) {
        uint32_t left_cnt = iter->second.count;

        if (left_cnt <= buy_cnt) {
            left_cnt = 0;
        } else {
            left_cnt = left_cnt - buy_cnt;
        }

        iter->second.count = left_cnt;

        onlineproto::sc_0x0131_shop_refresh noti;
        noti.set_market(onlineproto::MARKET_TYPE_ARENA);
        commonproto::market_item_info_t *item_info = noti.mutable_item_info();
        FOREACH(*(player->arena_shop_items), iter) {
            commonproto::market_item_t *item = item_info->add_items();
            item->set_item_id(iter->second.item_id);
            item->set_count(iter->second.count);
        }
        // 同步到db
        PlayerUtils::update_user_raw_data(
                player->userid, player->create_tm, 
                dbproto::ARENA_SHOP_PRODUCT, noti.item_info(), "0");
    }

    return 0;
}


uint32_t ShopRegisterFun::check_family_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();

     if (market != MARKET_TYPE_FAMILY) {
         return 0;
     }

    std::map<uint32_t, ol_market_item_t>::iterator iter = 
        player->family_shop_items->find(pd_id);
    if (iter == player->family_shop_items->end()) {
        return cli_err_product_sell_out;
    }

    if (iter->second.count < buy_cnt) {
        return cli_err_shop_product_not_enough;
    }

    return 0;
}

uint32_t ShopRegisterFun::update_family_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();
     if (market != MARKET_TYPE_FAMILY) {
         return 0;
     }

    std::map<uint32_t, ol_market_item_t>::iterator iter = 
        player->family_shop_items->find(pd_id);
    if (iter != player->family_shop_items->end()) {
        uint32_t left_cnt = iter->second.count;

        if (left_cnt <= buy_cnt) {
            left_cnt = 0;
        } else {
            left_cnt = left_cnt - buy_cnt;
        }

        iter->second.count = left_cnt;

        onlineproto::sc_0x0131_shop_refresh noti;
        noti.set_market(onlineproto::MARKET_TYPE_FAMILY);
        commonproto::market_item_info_t *item_info = noti.mutable_item_info();
        FOREACH(*(player->family_shop_items), iter) {
            commonproto::market_item_t *item = item_info->add_items();
            item->set_item_id(iter->second.item_id);
            item->set_count(iter->second.count);
        }
        // 同步到db
        PlayerUtils::update_user_raw_data(
                player->userid, player->create_tm, 
                dbproto::FAMILY_SHOP_PRODUCT, noti.item_info(), "0");
    }

    return 0;
}

uint32_t ShopRegisterFun::check_exped_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();

     if (market != MARKET_TYPE_EXPED) {
         return 0;
     }

    std::map<uint32_t, ol_market_item_t>::iterator iter = 
        player->exped_shop_items->find(pd_id);
    if (iter == player->exped_shop_items->end()) {
        return cli_err_product_sell_out;
    }

    if (iter->second.count < buy_cnt) {
        return cli_err_shop_product_not_enough;
    }

    return 0;
}

uint32_t ShopRegisterFun::check_night_raid_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();

     if (market != MARKET_TYPE_NIGHT_RAID) {
         return 0;
     }

    std::map<uint32_t, ol_market_item_t>::iterator iter = 
        player->night_raid_shop_items->find(pd_id);
    if (iter == player->night_raid_shop_items->end()) {
        return cli_err_product_sell_out;
    }

    if (iter->second.count < buy_cnt) {
        return cli_err_shop_product_not_enough;
    }

    return 0;
}
uint32_t ShopRegisterFun::update_night_raid_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();
     if (market != MARKET_TYPE_NIGHT_RAID) {
         return 0;
     }

    std::map<uint32_t, ol_market_item_t>::iterator iter = 
        player->night_raid_shop_items->find(pd_id);
    if (iter != player->night_raid_shop_items->end()) {
        uint32_t left_cnt = iter->second.count;

        if (left_cnt <= buy_cnt) {
            left_cnt = 0;
        } else {
            left_cnt = left_cnt - buy_cnt;
        }

        iter->second.count = left_cnt;

        onlineproto::sc_0x0131_shop_refresh noti;
        noti.set_market(onlineproto::MARKET_TYPE_NIGHT_RAID);
        commonproto::market_item_info_t *item_info = noti.mutable_item_info();
        FOREACH(*(player->night_raid_shop_items), iter) {
            commonproto::market_item_t *item = item_info->add_items();
            item->set_item_id(iter->second.item_id);
            item->set_count(iter->second.count);
        }
        // 同步到db
        PlayerUtils::update_user_raw_data(
                player->userid, player->create_tm, 
                dbproto::NIGHT_RAID_SHOP_PRODUCT, noti.item_info(), "0");
    }
    return 0;
}
uint32_t ShopRegisterFun::update_exped_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_)
{
    uint32_t pd_id = buy_in_.product_id();
    uint32_t buy_cnt = buy_in_.buy_cnt();
     if (market != MARKET_TYPE_EXPED) {
         return 0;
     }

    std::map<uint32_t, ol_market_item_t>::iterator iter = 
        player->exped_shop_items->find(pd_id);
    if (iter != player->exped_shop_items->end()) {
        uint32_t left_cnt = iter->second.count;

        if (left_cnt <= buy_cnt) {
            left_cnt = 0;
        } else {
            left_cnt = left_cnt - buy_cnt;
        }

        iter->second.count = left_cnt;

        onlineproto::sc_0x0131_shop_refresh noti;
        noti.set_market(onlineproto::MARKET_TYPE_EXPED);
        commonproto::market_item_info_t *item_info = noti.mutable_item_info();
        FOREACH(*(player->exped_shop_items), iter) {
            commonproto::market_item_t *item = item_info->add_items();
            item->set_item_id(iter->second.item_id);
            item->set_count(iter->second.count);
        }
        // 同步到db
        PlayerUtils::update_user_raw_data(
                player->userid, player->create_tm, 
                dbproto::EXPED_SHOP_PRODUCT, noti.item_info(), "0");
    }
    return 0;
}
#endif

void gen_db_transaction(player_t *player,
        const product_t *product, uint32_t buy_cnt, int32_t money_chg,
        dbproto::channel_type_t chn, dbproto::transaction_info_t *inf)
{
    inf->Clear();
    inf->set_transaction_time(NOW());
    inf->set_server_no(g_server_id);
    inf->set_account_id(player->userid);
    inf->set_s_create_tm(player->create_tm);
    inf->set_dest_account_id(player->userid);
    inf->set_d_create_tm(player->create_tm);
    inf->set_channel_id(chn);
    inf->set_pay_gate_trans_id(0);
    inf->set_product_id(product ?product->id :90007);
    //inf->set_product_type(dbproto::PRODUCT_TYPE_FOREVER); //商品的有效性根据内部的物品有效应决定
    inf->set_product_type(dbproto::product_type_t(product ?product->pd_type :1)); //商品的有效性根据内部的物品有效应决定
    inf->set_product_duration(0);//商品的时间根据内部的物品的时间决定
    inf->set_product_count(buy_cnt);
    inf->set_money_num(money_chg);
    inf->set_result(0);
}

void gen_db_diamond_transaction_new(player_t* player,
		const product_t *product, uint32_t buy_cnt, int32_t money_chg,
		dbproto::channel_type_t chn)
{
    if (!product) {
        return;
    }
    std::vector<product_element_t> all_elem_vec;
    FOREACH(product->item_elem, it) {
        all_elem_vec.push_back(it->second);
    }
    FOREACH(product->attr_elem, it) {
        all_elem_vec.push_back(it->second);
    }
    FOREACH(product->pet_elem, it) {
        all_elem_vec.push_back(it->second);
    }
    FOREACH(product->buff_elem, it) {
        all_elem_vec.push_back(it->second);
    }
    uint32_t total_elems = all_elem_vec.size();
    if (total_elems == 0) {
        return;
    }
    int32_t unit_money = money_chg / (int)total_elems;
    int32_t extra_money = money_chg % (int)total_elems;
    int32_t first_money = unit_money + extra_money;

    bool first = true;
    FOREACH(all_elem_vec, it) {
        const product_element_t &pd = *it;
		uint32_t trans_item_id = get_caiwu_id(pd.type, pd.id);
		uint32_t buy_elem_count = buy_cnt * pd.count;
        int32_t actual_money = 0;
        if (first) {
            actual_money = first_money;
            first = false;
        } else {
            actual_money = unit_money;
        }
        dbproto::cs_new_transaction trans_msg;
		dbproto::transaction_info_t* ptr = trans_msg.mutable_info();
		
		ptr->set_transaction_time(NOW());
		ptr->set_server_no(g_server_id);
		ptr->set_account_id(player->userid);
		ptr->set_s_create_tm(player->create_tm);
		ptr->set_dest_account_id(player->userid);
		ptr->set_d_create_tm(player->create_tm);
		ptr->set_channel_id(chn);
		ptr->set_pay_gate_trans_id(0);
		ptr->set_product_id(trans_item_id);
		ptr->set_product_type((dbproto::product_type_t)product->pd_type);
		ptr->set_product_duration(0);
		ptr->set_product_count(buy_elem_count);
		ptr->set_money_num(actual_money);
		ptr->set_result(0);
        g_dbproxy->send_msg(NULL, player->userid, 
				player->create_tm,
				db_cmd_new_transaction, trans_msg);
    }
}

/*
void gen_db_diamond_transaction(player_t* player,
		const product_t *product, uint32_t buy_cnt, int32_t money_chg,
		dbproto::channel_type_t chn)
{
	const std::map<uint32_t, product_element_t>& item_elem = product->item_elem;
	const std::map<uint32_t, product_element_t>& attr_elem = product->attr_elem;
	const std::map<uint32_t, product_element_t>& pet_elem = product->pet_elem;
	const std::map<uint32_t, product_element_t>& buff_elem = product->buff_elem;

	int32_t total_count = item_elem.size() + attr_elem.size() + pet_elem.size() + buff_elem.size();
	uint32_t count = 0;
	if (total_count == 1) {
		uint32_t item_id = 0;
		uint32_t item_count = 0;
		if (item_elem.size()) {
			item_id = get_caiwu_id(ELEMENT_TYPE_ITEM, item_elem.begin()->second.id);
			item_count = item_elem.begin()->second.count;
		} else if (attr_elem.size()) {
			item_id = get_caiwu_id(ELEMENT_TYPE_ATTR, attr_elem.begin()->second.id);
			item_count = attr_elem.begin()->second.count;
		} else if (pet_elem.size()) {
			item_id = get_caiwu_id(ELEMENT_TYPE_PET, pet_elem.begin()->second.id);
			item_count = pet_elem.begin()->second.count;
		} else if (buff_elem.size()) {
			item_id = get_caiwu_id(ELEMENT_TYPE_BUFF, buff_elem.begin()->second.id);
			item_count = buff_elem.begin()->second.count;
		}
        dbproto::cs_new_transaction trans_msg;
		dbproto::transaction_info_t* ptr = trans_msg.mutable_info();
		
		ptr->set_transaction_time(NOW());
		ptr->set_server_no(g_server_id);
		ptr->set_account_id(player->userid);
		ptr->set_s_create_tm(player->create_tm);
		ptr->set_dest_account_id(player->userid);
		ptr->set_d_create_tm(player->create_tm);
		ptr->set_channel_id(chn);
		ptr->set_pay_gate_trans_id(0);
		ptr->set_product_id(product ?item_id : 90007);
		ptr->set_product_type(dbproto::product_type_t(product ? product->pd_type : 1));
		ptr->set_product_duration(0);
		ptr->set_product_count(buy_cnt * item_count);
		ptr->set_money_num(money_chg);
		ptr->set_result(0);
        g_dbproxy->send_msg(NULL, player->userid, 
				player->create_tm,
				db_cmd_new_transaction, trans_msg);
	} else {
		std::map<uint32_t, product_element_t>::const_iterator it =
			item_elem.begin();
		int32_t unit_money_chg = money_chg / total_count;	
		for (; it != item_elem.end(); ++it) {
			uint32_t item_id = 0;
			item_id = get_caiwu_id(ELEMENT_TYPE_ITEM, it->second.id);
			dbproto::cs_new_transaction trans_msg;
			dbproto::transaction_info_t* ptr = trans_msg.mutable_info();
			ptr->set_transaction_time(NOW());
			ptr->set_server_no(g_server_id);
			ptr->set_account_id(player->userid);
			ptr->set_s_create_tm(player->create_tm);
			ptr->set_dest_account_id(player->userid);
			ptr->set_d_create_tm(player->create_tm);
			ptr->set_channel_id(chn);
			ptr->set_pay_gate_trans_id(0);
			ptr->set_product_id(product ?item_id : 90007);
			ptr->set_product_type(dbproto::product_type_t(product ? product->pd_type : 1));
			ptr->set_product_duration(0);
			ptr->set_product_count(buy_cnt * it->second.count);
			ptr->set_result(0);
			++count;
			if ((uint32_t)total_count == count) {
				ptr->set_money_num(money_chg - (count - 1) * unit_money_chg);
				g_dbproxy->send_msg(NULL, player->userid, 
						player->create_tm,
						db_cmd_new_transaction, trans_msg);
				break;
			} else {
				ptr->set_money_num(unit_money_chg);
			}
			g_dbproxy->send_msg(NULL, player->userid, 
					player->create_tm,
					db_cmd_new_transaction, trans_msg);
		}
		it = attr_elem.begin();
		for (; it != attr_elem.end(); ++it) {
			uint32_t attr_id = 0;
			attr_id = get_caiwu_id(ELEMENT_TYPE_ATTR, it->second.id);
			dbproto::cs_new_transaction trans_msg;
			dbproto::transaction_info_t* ptr = trans_msg.mutable_info();
			ptr->set_transaction_time(NOW());
			ptr->set_server_no(g_server_id);
			ptr->set_account_id(player->userid);
			ptr->set_s_create_tm(player->create_tm);
			ptr->set_dest_account_id(player->userid);
			ptr->set_d_create_tm(player->create_tm);
			ptr->set_channel_id(chn);
			ptr->set_pay_gate_trans_id(0);
			ptr->set_product_id(product ?attr_id: 90007);
			ptr->set_product_type(dbproto::product_type_t(product ? product->pd_type : 1));
			ptr->set_product_duration(0);
			ptr->set_product_count(buy_cnt * it->second.count);
			ptr->set_result(0);
			++count;
			if ((uint32_t)total_count == count) {
				ptr->set_money_num(money_chg - (count - 1) * unit_money_chg);
				g_dbproxy->send_msg(NULL, player->userid, 
						player->create_tm,
						db_cmd_new_transaction, trans_msg);
				break;
			} else {
				ptr->set_money_num(unit_money_chg);
			}
			g_dbproxy->send_msg(NULL, player->userid, 
					player->create_tm,
					db_cmd_new_transaction, trans_msg);
		}
		it = pet_elem.begin();
		for (; it != pet_elem.end(); ++it) {
			uint32_t pet_id = 0;
			pet_id = get_caiwu_id(ELEMENT_TYPE_PET, it->second.id);
			dbproto::cs_new_transaction trans_msg;
			dbproto::transaction_info_t* ptr = trans_msg.mutable_info();
			ptr->set_transaction_time(NOW());
			ptr->set_server_no(g_server_id);
			ptr->set_account_id(player->userid);
			ptr->set_s_create_tm(player->create_tm);
			ptr->set_dest_account_id(player->userid);
			ptr->set_d_create_tm(player->create_tm);
			ptr->set_channel_id(chn);
			ptr->set_pay_gate_trans_id(0);
			ptr->set_product_id(product ?pet_id: 90007);
			ptr->set_product_type(dbproto::product_type_t(product ? product->pd_type : 1));
			ptr->set_product_duration(0);
			ptr->set_product_count(buy_cnt * it->second.count);
			ptr->set_result(0);
			++count;
			if ((uint32_t)total_count == count) {
				ptr->set_money_num(money_chg - (count - 1) * unit_money_chg);
				g_dbproxy->send_msg(NULL, player->userid, 
						player->create_tm,
						db_cmd_new_transaction, trans_msg);
				break;
			} else {
				ptr->set_money_num(unit_money_chg);
			}
			g_dbproxy->send_msg(NULL, player->userid, 
					player->create_tm,
					db_cmd_new_transaction, trans_msg);
		}
		it = buff_elem.begin();
		for (; it != buff_elem.end(); ++it) {
			uint32_t buff_id = 0;
			buff_id = get_caiwu_id(ELEMENT_TYPE_BUFF, it->second.id);
			dbproto::cs_new_transaction trans_msg;
			dbproto::transaction_info_t* ptr = trans_msg.mutable_info();
			ptr->set_transaction_time(NOW());
			ptr->set_server_no(g_server_id);
			ptr->set_account_id(player->userid);
			ptr->set_s_create_tm(player->create_tm);
			ptr->set_dest_account_id(player->userid);
			ptr->set_d_create_tm(player->create_tm);
			ptr->set_channel_id(chn);
			ptr->set_pay_gate_trans_id(0);
			ptr->set_product_id(product ?buff_id: 90007);
			ptr->set_product_type(dbproto::product_type_t(product ? product->pd_type : 1));
			ptr->set_product_duration(0);
			ptr->set_product_count(buy_cnt * it->second.count);
			ptr->set_result(0);
			++count;
			if ((uint32_t)total_count == count) {
				ptr->set_money_num(money_chg - (count - 1) * unit_money_chg);
				g_dbproxy->send_msg(NULL, player->userid, 
						player->create_tm,
						db_cmd_new_transaction, trans_msg);
				break;
			} else {
				ptr->set_money_num(unit_money_chg);
			}
			g_dbproxy->send_msg(NULL, player->userid, 
					player->create_tm,
					db_cmd_new_transaction, trans_msg);
		}
	}
}
*/

uint32_t buy_product(player_t* player, uint32_t pd_id, uint32_t buy_cnt)
{
    //上一笔交易还未完成
    if (player->temp_info.can_use_diamond == false) {
		ERROR_TLOG("uid=[%u]:buy_product", player->userid);
        return cli_err_last_transaction_not_finished;
    }   


    if (buy_cnt == 0) {
        ERROR_TLOG("Player:%u Buy Product[%u] with 0 count",
                player->userid, pd_id);
        return cli_err_buy_product_cnt_zero;
    }

    int ret = 0;
    //商品存在
    const product_t *pd = g_product_mgr.find_product(pd_id);
    if (!pd) {
        return cli_err_product_not_exist;
    }

    //钻石够用?
    uint32_t need_cnt = pd->price;
    uint32_t have_cnt = 0;
    int err = 0;
    //抵用物品拥有的数量
    uint32_t sub_have_cnt = 0;
    //抵用物品可扣除的数量
    uint32_t sub_need_cnt = 0;
    bool diamond_pd = false;

	//黄金勋章用户享有8折优惠
    if (pd->discount_type == 1 && is_gold_vip(player)) {
        need_cnt = ceil(need_cnt * pd->discount_rate / 100.0); 
    } else if (pd->discount_type == 2) { //所有人打折
        need_cnt = ceil(need_cnt * pd->discount_rate / 100.0);
    }
    need_cnt *= buy_cnt;

    if (pd->sub_price_type == SUB_PRICE_TYPE_ATTR) {
        sub_have_cnt = GET_A((attr_type_t)(pd->sub_price_id));
    } else if (pd->sub_price_type == SUB_PRICE_TYPE_ITEM) {
        sub_have_cnt = player->package->get_total_usable_item_count(pd->sub_price_id);
    }
    //可抵用的数量(可用数量乘以兑换比)
    uint32_t sub_trans_cnt = sub_have_cnt * pd->sub_price_rate;
    //最多可以扣除的抵用数量(标称价*兑换比*购买数量)
    uint32_t usable_sub_trans_cnt = pd->sub_price * pd->sub_price_rate;
    if (pd->discount_type == 1 && is_gold_vip(player)) {
        usable_sub_trans_cnt = ceil(usable_sub_trans_cnt * pd->discount_rate / 100.0); 
    } else if (pd->discount_type == 2) { //所有人打折
        usable_sub_trans_cnt = ceil(usable_sub_trans_cnt * pd->discount_rate / 100.0);
    }
    usable_sub_trans_cnt *= buy_cnt;
   
    //抵用券可用至最大值
    if (sub_trans_cnt >= usable_sub_trans_cnt) {
        sub_need_cnt = (usable_sub_trans_cnt + pd->sub_price_rate - 1) / pd->sub_price_rate;
        need_cnt = (need_cnt > usable_sub_trans_cnt) ?(need_cnt - usable_sub_trans_cnt) :0;

    } else {//可抵用券用尽
        sub_need_cnt = sub_have_cnt;
        need_cnt -= sub_trans_cnt;
    }

    if (pd->price_type == PRODUCT_PRICE_TYPE_DIAMOND) {
        diamond_pd = true;
        have_cnt = player_get_diamond(player);
        if (need_cnt > have_cnt) {
            return cli_err_lack_diamond;
        }

    } else if (pd->price_type == PRODUCT_PRICE_TYPE_GOLD){
        if (!AttrUtils::is_player_gold_enough(player, need_cnt)) {
            return cli_err_gold_not_enough;
        }

    } else if (pd->is_attr_price_type) { //属性ID作为价格类型
        if (GET_A((attr_type_t)(pd->price_type)) < need_cnt) {
            return cli_err_not_enough_money;
        }

    } else {//道具ID作为价格类型
        uint32_t item_cnt = player->package->get_total_usable_item_count(pd->price_type);
        if (item_cnt < need_cnt) {
            return cli_err_no_enough_item_num;
        }
    }
    

    //限制条件满足
    if (pd->daily_key) {
        uint32_t max = AttrUtils::get_attr_max_limit(player, (attr_type_t)pd->daily_key);
        uint32_t val = AttrUtils::get_attr_value(player, (attr_type_t)pd->daily_key);
        if (val+buy_cnt > max) return cli_err_shop_exceed_daily_limit;
    }
    if (pd->weekly_key) {
        uint32_t max = AttrUtils::get_attr_max_limit(player, (attr_type_t)pd->weekly_key);
        uint32_t val = AttrUtils::get_attr_value(player, (attr_type_t)pd->weekly_key);
        if (val+buy_cnt > max) return cli_err_shop_exceed_weekly_limit;
    }
    if (pd->monthly_key) {
        uint32_t max = AttrUtils::get_attr_max_limit(player, (attr_type_t)pd->monthly_key);
        uint32_t val = AttrUtils::get_attr_value(player, (attr_type_t)pd->monthly_key);
        if (val+buy_cnt > max) return cli_err_shop_exceed_monthly_limit;
    }
    if (pd->forever_key) {
        uint32_t max = AttrUtils::get_attr_max_limit(player, (attr_type_t)pd->forever_key);
        uint32_t val = AttrUtils::get_attr_value(player, (attr_type_t)pd->forever_key);
        if (val+buy_cnt > max) return cli_err_shop_exceed_forever_limit;
    }

    std::vector<add_item_info_t> add_vec;
    FOREACH(pd->item_elem, it) {
        add_item_info_t add_item;
        const product_element_t &elem = it->second;
        add_item.item_id = elem.id;
        add_item.count = elem.count * buy_cnt;
        if (elem.duration) {
            add_item.expire_time = NOW() + (elem.duration * 86400);
        } else {
            add_item.expire_time = 0;
        }
        add_vec.push_back(add_item);
    }
    ret = check_swap_item_by_item_id(player, 0, &add_vec, false);
    if (ret) {
        return ret;
    }

    if (pd->pet_elem.size() * buy_cnt > PetUtils::can_add_pets_num(player)) {
        return cli_err_pets_full;
    }

    FOREACH(pd->attr_elem, it) {
        const product_element_t &elem = it->second;
        attr_type_t type = (attr_type_t)(elem.id);
        int64_t add = elem.count * (int)buy_cnt;
        uint32_t max = AttrUtils::get_attr_max_limit(player, type);
        uint32_t cur = AttrUtils::get_attr_value(player, type);
        uint32_t final;
        if (elem.method == 1) {
            final = cur | add;
        } else if (elem.method == 2) {
            final = add;
        } else if (elem.method == 3) {
            final = cur + buy_cnt * ranged_random(elem.range_min, elem.range_max);
        } else {
            final = cur + add;
        }
        if (add > 0 && max != kAttrMaxNoLimit && final > max) {
            return cli_err_attr_reach_max_limit;
        }
    }

	//可以买的情况下购买商品前的一些逻辑处理
	ret = g_shop_reg_fun.call_reg_func_before_buy_product(player, pd->id);
	if (ret) {
		return ret; 
	}

    //同步钻石信息 (内含交易记录)
    if (need_cnt || diamond_pd) {//如果是钻石商品 或者使用了钻石 需要落统计
        if (pd->price_type == PRODUCT_PRICE_TYPE_DIAMOND) {
            ret = player_chg_diamond_and_sync(player, -need_cnt, pd, buy_cnt,
                    dbproto::CHANNEL_TYPE_BUY_REDUCE, pd->name, true);
            if (ret) {
                return ret;
            }

        } else if (pd->price_type == PRODUCT_PRICE_TYPE_GOLD){ //金币同步
            err = AttrUtils::sub_player_gold(player, need_cnt, pd->stat_name.empty() 
                    ?pd->name :pd->stat_name);
            if (err) {
                return err; 
            }

        } else if (pd->is_attr_price_type) { //属性更改
            SUB_A((attr_type_t)pd->price_type, need_cnt);

        } else {//道具扣减
            std::vector<reduce_item_info_t> reduce_vec;
            reduce_item_info_t reduce;
            reduce.item_id = pd->price_type;
            reduce.count = need_cnt;
            reduce_vec.push_back(reduce);
            err = swap_item_by_item_id(player, &reduce_vec, 0);
            if (err) {
                return err;
            }
        }
    }
    //抵用物品扣减
    if (sub_need_cnt) {
        if (pd->sub_price_type == SUB_PRICE_TYPE_ATTR) {
            SUB_A((attr_type_t)(pd->sub_price_id), sub_need_cnt);
        } else if (pd->sub_price_type == SUB_PRICE_TYPE_ITEM) {
            reduce_single_item(player, pd->sub_price_id, sub_need_cnt);
        }
    }

    //到此 所以的元素都可以增加
    //增加物品
    swap_item_by_item_id(player, 0, &add_vec, false,
            onlineproto::SYNC_REASON_BUY_PRODUCT, NO_ADDICT_DETEC);

    //增加精灵
    FOREACH(pd->pet_elem, it) {
        const product_element_t &elem = it->second;
        uint32_t catch_time = 0;
        for (uint32_t i = 0; i < buy_cnt; i++) {
            PetUtils::create_pet(player, elem.id, elem.level, false, &catch_time);
        }
    }

    //增加属性
    std::map<uint32_t, uint32_t> attr_chg_map;
    FOREACH(pd->attr_elem, it) {
        const product_element_t &elem = it->second;
        
        if (attr_chg_map.count(elem.id) == 0) {
            attr_chg_map[elem.id] = GET_A((attr_type_t)elem.id);
        }
        uint32_t value = attr_chg_map[elem.id];
        int32_t add = elem.count * buy_cnt;

        //如果是商店产出金币
        if (elem.id == kAttrGold || elem.id == kAttrPaidGold) {
            AttrUtils::add_player_gold(player, add, elem.id==kAttrGold ?false :true, pd->stat_name);
            continue;
        }

        if (elem.method == 1) { //或运算
            value = (value | add);
        } else if (elem.method == 2 ) { //设置
            value = add;
        } else {//加
            if (elem.method == 3) {
                add = buy_cnt * ranged_random(elem.range_min, elem.range_max);
            }
            if (add < 0 && (uint32_t)(add*(-1)) >= value) {
                value = 0;
            } else if (add < 0 && (uint32_t)(add*(-1)) < value) {
                value = value - (uint32_t)(add*(-1));
            } else {
                value = value + add;
                uint32_t value_limit = AttrUtils::get_attr_max_limit(player, (attr_type_t)elem.id);
                if (value_limit != kAttrMaxNoLimit && value_limit < value) {
                    value = value_limit;
                }
            }
        }
        attr_chg_map[elem.id] = value;
        if (elem.target_attr) { //清理目标属性
            attr_chg_map[elem.target_attr] = 0;
        }
    }

    //更改buff
    bool has_buff = false;
    FOREACH(pd->buff_elem, it) {
        uint32_t buff_id = it->first;
        PlayerUtils::add_player_buff(player, buff_id);
        has_buff = true;
    }

    if (has_buff) {//同步buff
        PlayerUtils::sync_player_buff(player);
    }

    AttrUtils::set_attr_value(player, attr_chg_map);
	//购买商品后的一些逻辑处理
	g_shop_reg_fun.call_reg_func_after_buy_product(player, pd->id);

    if (pd->daily_key) {
        AttrUtils::add_attr_value(player, (attr_type_t)pd->daily_key, buy_cnt);
    }
    if (pd->weekly_key) {
        AttrUtils::add_attr_value(player, (attr_type_t)pd->weekly_key, buy_cnt);
    }
    if (pd->monthly_key) {
        AttrUtils::add_attr_value(player, (attr_type_t)pd->monthly_key, buy_cnt);
    }
    if (pd->forever_key) {
        AttrUtils::add_attr_value(player, (attr_type_t)pd->forever_key, buy_cnt);
    }

    //商品购买统计
    Utils::write_msglog_new(player->userid, "所有商店", "商店购买", 
            pd->stat_name.empty() ?pd->name :pd->stat_name);
    return 0;
} 

/**
 * @brief  buy_attr_and_use 
 *
 * @return 错误码
 */
uint32_t buy_attr_and_use(player_t* player, attr_type_t attr_type, 
        uint32_t product_id, uint32_t count)
{
    uint32_t attr_value = GET_A(attr_type);
    if (attr_value < count) {
        int need_count = count - attr_value;
        uint32_t err = buy_product(player, product_id, need_count);
        if (err) {
            return err;
        }
    }
    attr_value = GET_A(attr_type);
    if (attr_value < count) {
        ERROR_TLOG("%u buy attr failed attr = %u, product = %u,attr_value=[%u]",
                player->userid, (uint32_t)attr_type, product_id, attr_value);
        return cli_err_sys_err;
    }
    attr_value -= count;
    SET_A(attr_type, attr_value);
    return 0;
}

void random_select_product_from_product_vec(const market_t *market, 
        uint32_t type, uint32_t count, 
        std::vector<market_product_t> &result)
{
    const std::vector<market_product_t> *product_vec;
    if (market->count(type)) {
        product_vec = &((market->find(type))->second);
        if (product_vec->size() <= count) {
            result.insert(result.end(), product_vec->begin(), product_vec->end());
        } else {
            std::map<uint32_t, uint32_t> idx_weight_map;
            for (uint32_t i = 0; i < product_vec->size(); i++) {
                idx_weight_map[i] = (*product_vec)[i].weight;
            }
            std::set<uint32_t> idx_hit_set;
            Utils::rand_select_uniq_m_from_n_with_r(idx_weight_map, idx_hit_set, count);
            FOREACH(idx_hit_set, it) {
                result.push_back((*product_vec)[*it]);
            }
        }
    }
}

int random_select_product_from_market_default(
        uint32_t market,
        std::vector<market_product_t> &result)
{
    static uint32_t type1 = 2;
    static uint32_t type2 = 1;
    static uint32_t type3 = 5;
    static uint32_t type4 = 1;

    result.clear();
    const market_t *p_market = g_market_mgr.get_market(market);
    if (!p_market) {
        return -1;
    }
    random_select_product_from_product_vec(p_market, 1, type1, result);   
    random_select_product_from_product_vec(p_market, 2, type2, result);   
    random_select_product_from_product_vec(p_market, 3, type3, result);   
    random_select_product_from_product_vec(p_market, 4, type4, result);

    return 0;
}

int transaction_proc_exchange(player_t *player, uint32_t exchange_id, uint32_t count)
{
    const exchange_config_t* exchange_config = g_exchg_conf_mgr.get_exchg(exchange_id);

    if (exchange_config == NULL) {
        return cli_err_exchange_id_not_exist;
    }
    if (exchange_config->must_vip && !is_vip(player)) {
        return cli_err_not_vip;
    }

    const exchange_in_config_t* in_config = &exchange_config->in;
    const exchange_out_config_t* out_config = &exchange_config->out;

    std::vector<add_item_info_t> add_vec;
    std::vector<reduce_item_info_t> reduce_vec;

    // 检查永久上限
    if (exchange_config->forever_key) {
        uint32_t forever_times = GET_A((attr_type_t)exchange_config->forever_key);
        uint32_t forever_max = GET_A_MAX((attr_type_t)exchange_config->forever_key);
        if (forever_times + count > forever_max) {
            return cli_err_exchange_exceed_forever_limit;
        }
    }

    // 检查每日上限
    if (exchange_config->daily_key) {
        uint32_t daily_times = GET_A((attr_type_t)exchange_config->daily_key);
        uint32_t daily_max = GET_A_MAX((attr_type_t)exchange_config->daily_key);
        if (daily_times + count > daily_max) {
            return cli_err_exchange_exceed_daily_limit;
        }
    }

    // 检查每周上限
    if (exchange_config->weekly_key) {
        uint32_t weekly_times = GET_A((attr_type_t)exchange_config->weekly_key); 
        uint32_t weekly_max = GET_A_MAX((attr_type_t)exchange_config->weekly_key);
        if (weekly_times + count > weekly_max) {
            return cli_err_exchange_exceed_weekly_limit;
        }
    }

    // 检查每月上限
    if (exchange_config->monthly_key) {
        uint32_t monthly_times = GET_A((attr_type_t)exchange_config->monthly_key); 
        uint32_t monthly_max = GET_A_MAX((attr_type_t)exchange_config->monthly_key);
        if (monthly_times + count > monthly_max) {
            return cli_err_exchange_exceed_monthly_limit;
        }
    }
 
    //如果开启了随机掉落 则随机OUT项
    std::map<uint32_t, uint32_t> rate_list;
    rate_list.clear();
    std::set<uint32_t> hit_idx_set;
    if (exchange_config->rand_mode != 0) {
        //构建idx列表 idx的顺序为 物品->属性->精灵
        uint32_t idx = 0;
        FOREACH(exchange_config->out.item_list, it) {
            rate_list[idx] = it->rate;
            idx++;
        }
        FOREACH(exchange_config->out.attr_list, it) {
            rate_list[idx] = it->rate;
            idx++;
        }
        FOREACH(exchange_config->out.pet_list, it) {
            rate_list[idx] = it->rate;
            idx++;
        }
        Utils::rand_select_uniq_m_from_n_with_r(rate_list, 
                hit_idx_set, exchange_config->rand_mode);
        if (hit_idx_set.size() != exchange_config->rand_mode) {
            return cli_err_sys_err;
        }
    }

    //////////////////////////////////
    // 开始检查
    
    // 检查属性值是否可以扣除
    std::map<uint32_t, uint32_t> need_attr_map;
    uint32_t need_gold = 0;
    for (uint32_t i = 0; i < in_config->attr_list.size(); i++) {
        const exchange_attr_config_t* attr_config = &in_config->attr_list[i];   
        uint32_t value = 0;
        if ((attr_type_t)attr_config->id == kAttrGold) {
            need_gold += attr_config->count * count;
            if (!AttrUtils::is_player_gold_enough(player, need_gold)) {
                return cli_err_gold_not_enough;
            }
        } else {
            if (need_attr_map.count(attr_config->id) == 0) {
                need_attr_map[attr_config->id] = attr_config->count;
            } else {
                need_attr_map[attr_config->id] += attr_config->count * count;
            }
            value = AttrUtils::get_attr_value(player, (attr_type_t)attr_config->id);
            if (value < need_attr_map[attr_config->id]) {
                return cli_err_no_enough_item_num;
            }
        }
    }

    for (uint32_t i = 0; i < in_config->item_list.size(); i++) {
        const exchange_item_config_t* item_config = &in_config->item_list[i];
        reduce_item_info_t reduce_item;
        reduce_item.item_id = item_config->id;
        reduce_item.count = item_config->count * count;
        reduce_vec.push_back(reduce_item);
    }

    uint32_t item_idx = 0;
    for (uint32_t i = 0; i < out_config->item_list.size(); i++) {
        item_idx = i;
        if (exchange_config->rand_mode != 0 && hit_idx_set.count(item_idx) == 0) {
            continue;
        }
        const exchange_item_config_t* item_config = &out_config->item_list[i]; 
        add_item_info_t add_item;
        add_item.item_id = item_config->id;
        add_item.count = item_config->count * count;
        add_item.expire_time = item_config->expire_time;

        // 兑换系统防沉迷
        if (exchange_config->addiction) {
            if(check_player_addicted_threshold_none(player)){
                add_item.count = 0;
            } else if (check_player_addicted_threshold_half(player)){
                add_item.count /= 2;
            }
        }

        add_vec.push_back(add_item);
    }

    // 检查物品是否可以添加/扣除
    uint32_t err = check_swap_item_by_item_id(player, &reduce_vec, &add_vec, false);
    if (err) {
        return err;
    }

    if (out_config->pet_list.size() && count > 1) {
        return cli_err_exchange_exceed_forever_limit;
    }

    // 检查精灵是否可以添加
    uint32_t pet_idx = out_config->item_list.size() + out_config->attr_list.size();
    std::set<const exchange_pet_config_t*> add_pet_set;
    add_pet_set.clear();
    for (uint32_t i = 0; i < out_config->pet_list.size(); i++) {
        //没有随机到
        if (exchange_config->rand_mode != 0 && hit_idx_set.count(pet_idx+i) == 0) continue;

        const exchange_pet_config_t* pet_config = &out_config->pet_list[i];
        bool succ = PetUtils::check_can_create_pet(player, pet_config->id);
        if (!succ) {
            return cli_err_already_own_pet;
        }
        add_pet_set.insert(pet_config);
    }

    /////////////////////////////////////
    //开始加东西, 扣东西
    //计算概率 判断哪些属性、物品、精灵中标

    // 扣除属性
    if (need_gold) {
        AttrUtils::sub_player_gold(player, need_gold, 
                exchange_config->msg_sub_name.empty() ?exchange_config->desc :exchange_config->msg_sub_name);
    }
    std::map<uint32_t, uint32_t> attr_chg_map;
    FOREACH(need_attr_map, it) {
        uint32_t need_value = it->second;
        uint32_t type = it->first;
        uint32_t value = GET_A((attr_type_t)type);
        value = value - need_value;
        attr_chg_map[type] = value;
    }
    AttrUtils::set_attr_value(player, attr_chg_map);

    // 添加属性
    uint32_t attr_idx = out_config->item_list.size();
    attr_chg_map.clear();
    for (uint32_t i = 0; i < out_config->attr_list.size(); i++) {
        //未随机到
        if (exchange_config->rand_mode != 0 && hit_idx_set.count(attr_idx+i) == 0)  {
            continue;
        }
        const exchange_attr_config_t* attr_config = &out_config->attr_list[i]; 

         //如果是兑换产出金币
        if (attr_config->id == kAttrGold || attr_config->id == kAttrPaidGold) {
            AttrUtils::add_player_gold(player, attr_config->count *count, attr_config->id==kAttrGold ?false :true, 
                    exchange_config->msg_sub_name);
            continue;
        }
    
        if (attr_chg_map.count(attr_config->id) == 0) {
            attr_chg_map[attr_config->id] = GET_A((attr_type_t)attr_config->id);
        }

        uint32_t value = attr_chg_map[attr_config->id];
        uint32_t add_value = attr_config->count * count;

        // 兑换系统防沉迷
        if (exchange_config->addiction) {
            if (g_item_conf_mgr.is_addicted_attr_item(attr_config->id)) {
                if(check_player_addicted_threshold_none(player)){
                    add_value = 0;
                } else if (check_player_addicted_threshold_half(player)){
                    add_value /= 2;
                }
            }
        }

        value += add_value;
        uint32_t value_limit = AttrUtils::get_attr_max_limit(player, (attr_type_t)attr_config->id);
        if (value_limit != kAttrMaxNoLimit && value_limit < value) {
            value = value_limit;
        }
        attr_chg_map[attr_config->id] = value;
    }

    AttrUtils::set_attr_value(player, attr_chg_map);

    // 兑换设置唯一性
    if (exchange_config->forever_key) {
        uint32_t forever_times = GET_A((attr_type_t)exchange_config->forever_key);
        forever_times += count;
        SET_A((attr_type_t)exchange_config->forever_key, forever_times);
    }

    if (exchange_config->daily_key) {
        uint32_t daily_times = GET_A((attr_type_t)exchange_config->daily_key);    
        daily_times += count;
        SET_A((attr_type_t)exchange_config->daily_key, daily_times);
    }

    if (exchange_config->weekly_key) {
        uint32_t weekly_times = GET_A((attr_type_t)exchange_config->weekly_key); 
        weekly_times += count;
        SET_A((attr_type_t)exchange_config->weekly_key, weekly_times);
    }

    if (exchange_config->monthly_key) {
        uint32_t monthly_times = GET_A((attr_type_t)exchange_config->monthly_key); 
        monthly_times += count;
        SET_A((attr_type_t)exchange_config->monthly_key, monthly_times);
    }

    // 扣除/添加物品
    err = swap_item_by_item_id(player, &reduce_vec, &add_vec, false, 
            onlineproto::SYNC_REASON_EXCHANGE_ITEM, NO_ADDICT_DETEC);
 
    if (err) {
        return err;
    }

    FOREACH(add_pet_set, it) {
        const exchange_pet_config_t* pet_config = *it;
        uint32_t catch_time = 0;
        err = PetUtils::create_pet(player, pet_config->id, 
                pet_config->level, false, &catch_time);
        if (err) {
            return err;
        }
    }

    if (exchange_config->msg_dir.length() 
        && exchange_config->msg_name.length() 
        && exchange_config->msg_sub_name.length()) {
        Utils::write_msglog_new(player->userid, exchange_config->msg_dir, 
                exchange_config->msg_name,
                exchange_config->msg_sub_name); 
    }

    return 0;
}

uint32_t batch_buy_product(player_t* player, std::vector<buy_product_t> buy_vec, string batch_name, bool sync_cli)
{
    //上一笔交易还未完成
    if (player->temp_info.can_use_diamond == false) {
		ERROR_TLOG("uid=[%u]:buy_product", player->userid);
        return cli_err_last_transaction_not_finished;
    }   
    uint32_t need_diamond_cnt = 0; //钻石需求总量
    uint32_t need_gold_cnt = 0; //金币需求总量
    std::map<uint32_t, uint32_t> need_item_cnt_map; //道具需求总量
    std::map<uint32_t, int64_t> need_attr_cnt_map; //属性需求总量
    std::map<uint32_t, uint32_t> pd_buy_cnt_map; //商品购买总量
    std::map<uint32_t, uint32_t> pd_buy_diamond_map; //每种商品的钻石购买总价
    int ret = 0;
    bool diamond_pd = false;

    FOREACH(buy_vec, it) {
        uint32_t pd_id = it->pd_id;
        uint32_t buy_cnt = it->buy_cnt;
        pd_buy_cnt_map[pd_id] += buy_cnt;

        if (buy_cnt == 0) {
            ERROR_TLOG("Player:%u Buy Product[%u] with 0 count",
                    player->userid, pd_id);
            return cli_err_buy_product_cnt_zero;
        }

        //商品存在
        const product_t *pd = g_product_mgr.find_product(pd_id);
        if (!pd) {
            return cli_err_product_not_exist;
        }

        //钻石够用?
        uint32_t need_cnt = pd->price;
        uint32_t have_cnt = 0;
        //抵用物品拥有的数量
        uint32_t sub_have_cnt = 0;
        //抵用物品可扣除的数量
        uint32_t sub_need_cnt = 0;

        //黄金勋章用户享有8折优惠
        if (pd->discount_type == 1 && is_gold_vip(player)) {
            need_cnt = ceil(need_cnt * pd->discount_rate / 100.0); 
        } else if (pd->discount_type == 2) { //所有人打折
            need_cnt = ceil(need_cnt * pd->discount_rate / 100.0);
        }
        need_cnt *= buy_cnt;

        if (pd->sub_price_type == SUB_PRICE_TYPE_ATTR) {
            sub_have_cnt = GET_A((attr_type_t)(pd->sub_price_id));
        } else if (pd->sub_price_type == SUB_PRICE_TYPE_ITEM) {
            sub_have_cnt = player->package->get_total_usable_item_count(pd->sub_price_id);
        }
        //可抵用的数量(可用数量乘以兑换比) 减去已经要消耗的部分
        uint32_t sub_trans_cnt = sub_have_cnt * pd->sub_price_rate;
        if (pd->sub_price_type == SUB_PRICE_TYPE_ATTR) {
            if (sub_trans_cnt > need_attr_cnt_map[pd->sub_price_id]) {
                sub_trans_cnt -= need_attr_cnt_map[pd->sub_price_id];
            } else {
                sub_trans_cnt = 0;
            }
        } else if (pd->sub_price_type == SUB_PRICE_TYPE_ITEM) {
            if (sub_trans_cnt > need_item_cnt_map[pd->sub_price_id]) {
                sub_trans_cnt -= need_item_cnt_map[pd->sub_price_id];
            } else {
                sub_trans_cnt = 0;
            }
        }

        //最多可以扣除的抵用数量(标称价*兑换比*购买数量)
        uint32_t needed_sub_trans_cnt = pd->sub_price * pd->sub_price_rate;
        if (pd->discount_type == 1 && is_gold_vip(player)) {
            needed_sub_trans_cnt = ceil(needed_sub_trans_cnt * pd->discount_rate / 100.0); 
        } else if (pd->discount_type == 2) { //所有人打折
            needed_sub_trans_cnt = ceil(needed_sub_trans_cnt * pd->discount_rate / 100.0);
        }
        needed_sub_trans_cnt *= buy_cnt;

        //抵用券可用至最大值
        if (sub_trans_cnt >= needed_sub_trans_cnt) {
            sub_need_cnt = (needed_sub_trans_cnt + pd->sub_price_rate - 1) / pd->sub_price_rate;
            need_cnt = (need_cnt > needed_sub_trans_cnt) ?(need_cnt - needed_sub_trans_cnt) :0;

        } else {//可抵用券用尽
            sub_need_cnt = sub_have_cnt;
            need_cnt -= sub_trans_cnt;
        }
        
        if (pd->sub_price_type == SUB_PRICE_TYPE_ATTR) {
            need_attr_cnt_map[pd->sub_price_id] += sub_need_cnt;
        } else if (pd->sub_price_type == SUB_PRICE_TYPE_ITEM) {
            need_item_cnt_map[pd->sub_price_id] += sub_need_cnt;
        }

        if (pd->price_type == PRODUCT_PRICE_TYPE_DIAMOND) {
            have_cnt = player_get_diamond(player);
            need_diamond_cnt += need_cnt;
            if (need_diamond_cnt > have_cnt) {
                return cli_err_lack_diamond;
            }
            pd_buy_diamond_map[pd->id] += need_cnt;
            diamond_pd = true;

        } else if (pd->price_type == PRODUCT_PRICE_TYPE_GOLD){
            need_gold_cnt += need_cnt;
            if (!AttrUtils::is_player_gold_enough(player, need_gold_cnt)) {
                return cli_err_gold_not_enough;
            }

        } else if (pd->is_attr_price_type) { //属性ID作为价格类型
            need_attr_cnt_map[pd->price_type] += need_cnt;
            if (GET_A((attr_type_t)(pd->price_type)) < need_attr_cnt_map[pd->price_type]) {
                return cli_err_not_enough_money;
            }

        } else {//道具ID作为价格类型
            uint32_t item_cnt = player->package->get_total_usable_item_count(pd->price_type);
            need_item_cnt_map[pd->price_type] += need_cnt;
            if (item_cnt < need_item_cnt_map[pd->price_type]) {
                return cli_err_no_enough_item_num;
            }
        }


        //限制条件满足
        if (pd->daily_key) {
            uint32_t max = AttrUtils::get_attr_max_limit(player, (attr_type_t)pd->daily_key);
            uint32_t val = AttrUtils::get_attr_value(player, (attr_type_t)pd->daily_key);
            if (val+pd_buy_cnt_map[pd_id] > max) return cli_err_shop_exceed_daily_limit;
        }
        if (pd->weekly_key) {
            uint32_t max = AttrUtils::get_attr_max_limit(player, (attr_type_t)pd->weekly_key);
            uint32_t val = AttrUtils::get_attr_value(player, (attr_type_t)pd->weekly_key);
            if (val+pd_buy_cnt_map[pd_id] > max) return cli_err_shop_exceed_weekly_limit;
        }
        if (pd->monthly_key) {
            uint32_t max = AttrUtils::get_attr_max_limit(player, (attr_type_t)pd->monthly_key);
            uint32_t val = AttrUtils::get_attr_value(player, (attr_type_t)pd->monthly_key);
            if (val+pd_buy_cnt_map[pd_id] > max) return cli_err_shop_exceed_monthly_limit;
        }
        if (pd->forever_key) {
            uint32_t max = AttrUtils::get_attr_max_limit(player, (attr_type_t)pd->forever_key);
            uint32_t val = AttrUtils::get_attr_value(player, (attr_type_t)pd->forever_key);
            if (val+pd_buy_cnt_map[pd_id] > max) return cli_err_shop_exceed_forever_limit;
        }

    }

    //判断可否加物品
    std::vector<add_item_info_t> add_vec;
    FOREACH(buy_vec, it) {
        uint32_t pd_id = it->pd_id;
        uint32_t buy_cnt = it->buy_cnt;
        const product_t *pd = g_product_mgr.find_product(pd_id);
        if (!pd) {
            return cli_err_product_not_exist;
        }
        FOREACH(pd->item_elem, it2) {
            add_item_info_t add_item;
            const product_element_t &elem = it2->second;
            add_item.item_id = elem.id;
            add_item.count = elem.count * buy_cnt;
            if (elem.duration) {
                add_item.expire_time = NOW() + (elem.duration * 86400);
            } else {
                add_item.expire_time = 0;
            }
            add_vec.push_back(add_item);
        }
    }
    ret = check_swap_item_by_item_id(player, 0, &add_vec, false);
    if (ret) {
        return ret;
    }

    //判断可否加精灵
    uint32_t total_pets = 0;
    FOREACH(buy_vec, it) {
        uint32_t pd_id = it->pd_id;
        uint32_t buy_cnt = it->buy_cnt;
        const product_t *pd = g_product_mgr.find_product(pd_id);
        if (!pd) {
            return cli_err_product_not_exist;
        }
        total_pets += pd->pet_elem.size() * buy_cnt;
    }
    if (total_pets > PetUtils::can_add_pets_num(player)) {
        return cli_err_pets_full;
    }

    //判断可否改属性
    std::map<uint32_t, int64_t> attr_chg_map;
    FOREACH(buy_vec, it) {
        const product_t *pd = g_product_mgr.find_product(it->pd_id);
        if (!pd) {
            return cli_err_product_not_exist;
        }
        FOREACH(pd->attr_elem, it2) {
            const product_element_t &elem = it2->second;
            attr_type_t type = (attr_type_t)(elem.id);
            int64_t add = elem.count * (int)it->buy_cnt;
            uint32_t max = AttrUtils::get_attr_max_limit(player, type);
            uint32_t cur = AttrUtils::get_attr_value(player, type);
            if (attr_chg_map.count((uint32_t)type) == 0) {
                attr_chg_map[(uint32_t)type] = cur;
            }
            if (elem.method == 1) {
                attr_chg_map[(uint32_t)type] |= add;
            } else if (elem.method == 2) {
                attr_chg_map[(uint32_t)type] = add;
            } else if (elem.method == 3) {
                attr_chg_map[(uint32_t)type] += it->buy_cnt * ranged_random(elem.range_min, elem.range_max);
            } else {
                attr_chg_map[(uint32_t)type] += add;
            }
            if (add > 0 && max != kAttrMaxNoLimit && attr_chg_map[(uint32_t)type] > max) {
                return cli_err_attr_reach_max_limit;
            }
        }
    }

	//可以买的情况下购买商品前的一些逻辑处理
    FOREACH(buy_vec, it) {
        ret = g_shop_reg_fun.call_reg_func_before_buy_product(player, it->pd_id);
        if (ret) {
            return ret; 
        }
    }

    //扣钻石
    if (need_diamond_cnt || diamond_pd) {//扣钻石
        FOREACH(pd_buy_diamond_map, it) {
            uint32_t pd_id = it->first;
            uint32_t diamond = it->second;
            uint32_t cnt = pd_buy_cnt_map[it->first];
            const product_t *pd = g_product_mgr.find_product(pd_id);
            ret = player_chg_diamond_and_sync(player, -diamond, pd, cnt,
                    dbproto::CHANNEL_TYPE_BUY_REDUCE, batch_name, true);
            if (ret) {
                return ret;
            }
        }
    }
    //扣金币
    if (need_gold_cnt) {
        ret = AttrUtils::sub_player_gold(player, need_gold_cnt, batch_name);
        if (ret) {
            return ret; 
        }
    }
    //扣物品
    std::vector<reduce_item_info_t> reduce_vec;
    FOREACH(need_item_cnt_map, it) {
        reduce_item_info_t reduce;
        reduce.item_id = it->first;
        reduce.count = it->second;
        reduce_vec.push_back(reduce);
    }
    ret = swap_item_by_item_id(player, &reduce_vec, 0);
    if (ret) {
        return ret;
    }

    //到此都可以加了
    //合并属性 扣 need_attr_cnt_map 增 attr_chg_map;
    FOREACH(need_attr_cnt_map, it) {
        uint32_t cur_val = 0;
        if (attr_chg_map.count(it->first) == 0) {
            cur_val = GET_A((attr_type_t)it->first);
        } else {
            cur_val = attr_chg_map.find(it->first)->second;
        }
        if (it->second < cur_val) {
            attr_chg_map[it->first] = cur_val - it->second;
        } else {
            attr_chg_map[it->first] = 0;
        }
    }

    //增加物品
    if (sync_cli) {
        swap_item_by_item_id(player, 0, &add_vec, false,
                onlineproto::SYNC_REASON_BUY_PRODUCT, NO_ADDICT_DETEC);
    } else {
        swap_item_by_item_id(player, 0, &add_vec, false,
                onlineproto::SYNC_REASON_NOT_SHOW, NO_ADDICT_DETEC);
    }

    //增加精灵 清属性 及更改buff
    bool has_buff = false;
    FOREACH(buy_vec, it) {
        const product_t *pd = g_product_mgr.find_product(it->pd_id);
        if (!pd) continue;

        //加精灵
        FOREACH(pd->pet_elem, it2) {
            const product_element_t &elem = it2->second;
            uint32_t catch_time = 0;
            for (uint32_t i = 0; i < it->buy_cnt; i++) {
                PetUtils::create_pet(player, elem.id, elem.level, false, &catch_time);
            }
        }

        //清属性
        FOREACH(pd->attr_elem, it2) {
            const product_element_t &elem = it2->second;
            if (elem.target_attr) { //清理目标属性
                attr_chg_map[elem.target_attr] = 0;
            }
        }

        //更改buff
        FOREACH(pd->buff_elem, it) {
            uint32_t buff_id = it->first;
            PlayerUtils::add_player_buff(player, buff_id);
            has_buff = true;
        }
    }
    if (has_buff) {//同步buff
        PlayerUtils::sync_player_buff(player);
    }

    //同步属性
    std::map<uint32_t, uint32_t> final_attr_chg_map;
    FOREACH(attr_chg_map, it) {

        //如果是产出改变金币
        if (it->first == kAttrGold || it->first == kAttrPaidGold) {
            int32_t cur = GET_A((attr_type_t)it->first);
            if (cur > (int32_t)it->second) {//减少
                AttrUtils::sub_player_gold(player, cur - it->second, batch_name);
            } else {//增多
                AttrUtils::add_player_gold(player, it->second - cur, it->first == kAttrGold ?false :true, batch_name);
            }
            continue;
        }

        final_attr_chg_map[it->first] = it->second;
    }
    AttrUtils::set_attr_value(player, final_attr_chg_map);

    //购买商品后的一些逻辑处理
    FOREACH(buy_vec, it) {
        const product_t *pd = g_product_mgr.find_product(it->pd_id);
        if (!pd) continue;

        g_shop_reg_fun.call_reg_func_after_buy_product(player, pd->id);
        if (pd->daily_key) {
            AttrUtils::add_attr_value(player, (attr_type_t)pd->daily_key, it->buy_cnt);
        }
        if (pd->weekly_key) {
            AttrUtils::add_attr_value(player, (attr_type_t)pd->weekly_key, it->buy_cnt);
        }
        if (pd->monthly_key) {
            AttrUtils::add_attr_value(player, (attr_type_t)pd->monthly_key, it->buy_cnt);
        }
        if (pd->forever_key) {
            AttrUtils::add_attr_value(player, (attr_type_t)pd->forever_key, it->buy_cnt);
        }
    }

    //商品购买统计
    Utils::write_msglog_new(player->userid, "所有商店", "批量购买", batch_name);
    return 0;
}

uint32_t  change_vip_end_time(player_t* player, change_vip_time_flag_t flag)
{
	uint32_t count = 1;
	if (flag == CHANGE_GOLD_30_VIP || flag == CHANGE_GOLD_180_VIP) {
		if (flag == CHANGE_GOLD_180_VIP) {
			count = 6;
		}
		if (GET_A(kAttrGoldVipEndTime) > NOW()) {
			SET_A(kAttrGoldVipEndTime, GET_A(kAttrGoldVipEndTime) + count * 30 * DAY_SECS);
		} else {
			SET_A(kAttrGoldVipEndTime, NOW() + count * 30 * DAY_SECS);
		}
		//购买黄金勋章的同时，系统默认也一并购买了白银勋章
		if (GET_A(kAttrSilverVipEndTime) > NOW()) {
			SET_A(kAttrSilverVipEndTime, GET_A(kAttrSilverVipEndTime) + count * 30 * DAY_SECS);
		} else {
			SET_A(kAttrSilverVipEndTime, NOW() + count * 30 * DAY_SECS);
		}

        // 重置试用翅膀时间
        //EquipUtils::update_trial_cult_equip_expire_time(player, 0);

	} else if (flag == CHANGE_SILIVER_VIP) {
		//已经是白银勋章 vip,且还在有效期内
		if (GET_A(kAttrSilverVipEndTime) > NOW()) {
			SET_A(kAttrSilverVipEndTime, GET_A(kAttrSilverVipEndTime) + 30 * DAY_SECS);
		} else {
			//还不是白银勋章 vip, 或者勋章过期了
			if (GET_A(kAttrGoldVipEndTime) > NOW()) {  
				//已经是黄金勋章 vip,且还在有效期内
				SET_A(kAttrSilverVipEndTime, GET_A(kAttrGoldVipEndTime) + 30 * DAY_SECS);
			} else {
				SET_A(kAttrSilverVipEndTime, NOW() + 30 * DAY_SECS);
			}
		}
	}
	return 0;
}

uint32_t set_buy_vip_trans(player_t* player,
		uint32_t last_gold_begin_time,
		uint32_t last_gold_end_time,
		uint32_t last_silver_begin_time,
		uint32_t last_silver_end_time,
		buy_vip_type_t type)
{
	//落财务数据
	if (type == BUY_GOLD_VIP || type == BUY_GOLD_180_VIP) {
		pack_vip_info(player, GET_A(kAttrGoldVipFirstBuyTime),
				GET_A(kAttrGoldVipEndTime), commonproto::GOLD_VIP);
		pack_vip_info(player, GET_A(kAttrSilverVipFirstBuyTime),
			GET_A(kAttrSilverVipEndTime), commonproto::SILVER_VIP);

		bool is_gold_180_vip = false;
		if (type == BUY_GOLD_180_VIP) {
			is_gold_180_vip = true;
		}
		pack_buy_vip_trans(player, last_gold_begin_time,
				last_gold_end_time, is_gold_180_vip,
				commonproto::GOLD_VIP);
		pack_buy_vip_trans(player, last_silver_begin_time,
				last_silver_end_time, is_gold_180_vip,
				commonproto::SILVER_VIP);
		//返回
		return 0;
	}
	//到了这里，就是只充值白银的了
	pack_vip_info(player, GET_A(kAttrSilverVipFirstBuyTime),
			GET_A(kAttrSilverVipEndTime), commonproto::SILVER_VIP);
	pack_buy_vip_trans(player, last_silver_begin_time,
			last_silver_end_time, false, commonproto::SILVER_VIP);
	return 0;
}

uint32_t pack_vip_info(player_t* player,
		uint32_t begin_time, uint32_t end_time,
		commonproto::player_vip_type_t vip_type) 
{
	dbproto::cs_new_vip_user_info db_in;
	dbproto::vip_user_info_t* ptr = db_in.mutable_info();
	ptr->set_server_no(player->init_server_id);
	ptr->set_user_id(player->userid);
	ptr->set_u_create_tm(player->create_tm);
	ptr->set_begin_time(begin_time);
	ptr->set_end_time(end_time);
	ptr->set_time_flag(1);
	ptr->set_fee_flag(10000);
	ptr->set_curr_time(NOW());
	ptr->set_vip_type(1);
	ptr->set_ct_vip_type(vip_type);
	return g_dbproxy->send_msg(NULL, player->userid,
			player->create_tm, db_cmd_new_vip_user_info, db_in);
}

uint32_t pack_buy_vip_trans(player_t* player, uint32_t last_begin_time,
		uint32_t last_end_time, bool is_gold_180_vip,
		commonproto::player_vip_type_t vip_type)
{
	dbproto::cs_new_vip_op_trans db_in;
	dbproto::vip_op_trans_info_t* ptr = db_in.mutable_info();
	ptr->set_server_no(player->init_server_id);
	ptr->set_user_id(player->userid);
	ptr->set_u_create_tm(player->create_tm);
	ptr->set_cmd_id(18449);
	ptr->set_trade_id(0);
	ptr->set_apply_time(NOW());
	ptr->set_begin_time(last_begin_time);
	ptr->set_end_time(last_end_time);
	ptr->set_time_flag(1);
	ptr->set_fee_flag(10000);
	uint32_t action_type = 1;
	if (last_end_time > NOW()) {
		action_type = 2;
	}
	ptr->set_action_type(action_type);
	uint32_t time_length = 30;
	if (is_gold_180_vip) {
		time_length = 180;
	}
	ptr->set_time_length(time_length);
	ptr->set_vip_type(1);
	ptr->set_ct_vip_type(vip_type);
	return g_dbproxy->send_msg(NULL, player->userid,
			player->create_tm, db_cmd_new_vip_op_trans, db_in);
}

/*
uint32_t deal_first_buy_time(player_t* player,
		buy_vip_type_t vip_type)
{
	uint32_t last_gold_begin_time = GET_A(kAttrGoldVipFirstBuyTime); 
	uint32_t last_silver_begin_time = GET_A(kAttrSilverVipFirstBuyTime);
	//记录旧的时间
	//如果vip时间已经到期，则将first buy 的时间戳标志为现在的时间
	if (vip_type == BUY_SILVER_VIP) {
		if (NOW() > last_silver_begin_time) {
			SET_A(kAttrSilverVipFirstBuyTime, NOW());
		}
	} else {
		if (NOW() > last_gold_begin_time) {
			SET_A(kAttrGoldVipFirstBuyTime, NOW());
		}
		if (NOW() > last_silver_begin_time) {
			SET_A(kAttrSilverVipFirstBuyTime, NOW());
		}
	}
	return 0;
}
*/

uint32_t deal_first_buy_time(player_t* player,
		buy_vip_type_t vip_type)
{
	uint32_t gold_end_time = GET_A(kAttrGoldVipEndTime);
	uint32_t silver_end_time = GET_A(kAttrSilverVipEndTime);
	
	//记录旧的时间
	//如果vip时间已经到期，则将first buy 的时间戳标志为现在的时间
	if (vip_type == BUY_SILVER_VIP) {
		if (NOW() > silver_end_time) {
			SET_A(kAttrSilverVipFirstBuyTime, NOW());
		}
	} else {
		if (NOW() > gold_end_time) {
			SET_A(kAttrGoldVipFirstBuyTime, NOW());
		}
		if (NOW() > silver_end_time) {
			SET_A(kAttrSilverVipFirstBuyTime, NOW());
		}
	}
	return 0;
}

uint32_t before_exchange_first_check(uint32_t exchange_id)
{
	if (exchange_id == 10001|| exchange_id == 10002) {
		if (TimeUtils::check_month_odd(NOW())) {
			if (exchange_id == 10001) {
				return cli_err_can_not_exchange_in_this_month;
			}
		} else {
			if (exchange_id == 10002) {
				return cli_err_can_not_exchange_in_this_month;
			}
		}
	}
	return 0;
}

uint32_t get_caiwu_id(uint32_t product_elem_type, uint32_t id)
{
	switch (product_elem_type) {
    case ELEMENT_TYPE_BUFF: {
        std::bitset<32> bit(id);
        bit.set(28);
        return bit.to_ulong();
    }
    case ELEMENT_TYPE_ATTR: {
        std::bitset<32> bit(id);
        bit.set(29);
        return bit.to_ulong();
    }
    case ELEMENT_TYPE_ITEM: {
        std::bitset<32> bit(id);
        bit.set(30);
        return bit.to_ulong();
    }
    case ELEMENT_TYPE_PET: {
        std::bitset<32> bit(id);
        bit.set(31);
        return bit.to_ulong();
    }
    default :
        break;
    }
	return id;
}
