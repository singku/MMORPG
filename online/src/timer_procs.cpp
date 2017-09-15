#include "timer_procs.h"
#include "service.h"
#include "statlogger/statlogger.h"
#include "global_data.h"
#include "player.h"
#include "player_manager.h"
#include "switch_proto.h"
#include "attr_utils.h"
#include "time_utils.h"
#include "sys_ctrl.h"
#include "mcast_utils.h"
#include "global_data.h"
#include "item.h"
#include "swim.h"
#include "escort_utils.h"
#include "task_utils.h"
#include "rank_utils.h"
#include "pet_utils.h"
#include "pet.h"
#include "duplicate_world_boss.h"
#include "duplicate_utils.h"
#include "player.h"
#include "mail_utils.h"
#include "arena.h"
#include "player_utils.h"

#define REGISTER_TIMER_TYPE(nbr_, cb_) \
    do { \
        if (register_timer_callback(nbr_, cb_) == -1) { \
            ERROR_LOG("register timer type error\t[%u]", nbr_); \
            return -1; \
        }\
    } while(0)

#define CHECK_TIMER(type) \
    player_t *player = 0; \
    do { \
        uint64_t tmp = (uint64_t)data; \
        uint32_t uid = (uint32_t)tmp; \
        player = g_player_manager->get_player_by_userid(uid); \
        if (!player) { \
            WARN_TLOG("Timer[type:%d] Callback But Player[%u] deleted", type, uid); \
            return 0; \
        } \
    } while(0)

int reconnect_service_timely(void* owner, void* data)
{
    Service* service = (Service *)data;
    if (service->connect() != 0) {
        INFO_LOG("connect to server:%s", service->service_name().c_str());
        ADD_TIMER_EVENT_EX(&g_reconnect_timer,
                kTimerTypeReconnectServiceTimely,
                service,
                get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely);  
    }
    return 0;
}

int sync_client_time_timely(void* owner, void* data)
{
    CHECK_TIMER(kTimerTypeSyncClientTimeTimely);
    //onlineproto::sc_keep_live sc_keep_live;
    //sc_keep_live.set_server_time(get_now_tv()->tv_sec);
    //send_msg_to_player(player, cli_cmd_cs_keep_live, sc_keep_live);
    player->sync_client_time_timer = ADD_TIMER_EVENT_EX(
            player,
            kTimerTypeSyncClientTimeTimely,
            (void*)(player->userid),
            get_now_tv()->tv_sec + kTimerIntervalSyncClientTimeTimely);  
    return 0;
}

//全局定时器清理每日跳水排行
//添加一个功能：周五零点后dump下排行榜
int daily_reset_dive(void *owner, void *data) 
{
    g_dive_rank.clear_dive_rank();
    ADD_TIMER_EVENT_EX(&g_reset_dive_timer, 
            kTimerTypeDailyResetDive, 
            0, TimeUtils::second_at_day_start(1));

	weekly_00_dump_rank_utils();

    return 0;
}

//全局定时器T人下线
int daily_kick_off(void *owner, void *data) 
{
    if (g_svr_close_before_zero) {//不为0表示会关服 需要T人下线
        std::vector<player_t*> player_list;
        g_player_manager->get_player_list(player_list);
        for (uint32_t i = 0; i < player_list.size(); i++) {
            player_t* player = player_list[i]; 
            send_err_to_player(player, player->cli_wait_cmd, cli_err_server_closed);
            close_client_conn(player->fdsess->fd);
        }
    }

    uint32_t day_offset = 1;
    if (g_svr_close_before_zero) {
        //不为0则定时器是在明天的0点前的某时刻触发 
        //这时下一个定时器要在后天0点前的某时刻触发 day_offset是2
        day_offset = 2;
    } 
	ADD_TIMER_EVENT_EX(&g_kick_off_timer, 
			kTimerTypeDailyKickOff, 
			0, TimeUtils::second_at_day_start(day_offset) - g_svr_close_before_zero);
    return 0;
}

//玩家的每日操作定时器
int daily_operation(void *owner, void *data)
{
    CHECK_TIMER(kTimerTypeDailyOperation);
    //到了下一天的整天做一些操作
    try_init_player_daily_data(player);
    //重新定时
    player->daily_op_timer = ADD_TIMER_EVENT_EX(
            player,
            kTimerTypeDailyOperation,
            (void*)(player->userid),
            TimeUtils::second_at_day_start(1));

    return 0;
}

int sys_offline_noti(void *owner, void *data)
{
    //发送广播通知
    static uint32_t cnt = 1;
    static int next_sec = 0;

    static char msg[256];
    snprintf(msg, sizeof(msg), "各位亲爱的玩家,赤瞳之刃还有%d分钟就要关服了,请大家注意游戏进度,及时下线",
            11-cnt);

    if (cnt <= 9) {
        next_sec = TimeUtils::second_at_day_start(1) - g_svr_close_before_zero - 600 + 60 *cnt;
        cnt ++;
    } else {
        cnt = 1;
        next_sec = TimeUtils::second_at_day_start(2) - g_svr_close_before_zero - 600;
    }

    std::string noti_msg;
    noti_msg.assign(msg);
    McastUtils::notify(noti_msg);
    //定时通知用户下线
    ADD_TIMER_EVENT_EX(&g_sys_noti_timer,
            kTimerTypeSysNotiOffline, 0,
            next_sec);

    return 0;
}

int check_svr_timeout(void* owner, void* data)
{
    CHECK_TIMER(kTimerTypeCheckSvrTimeout);
    player->svr_request_timer = NULL;
	ERROR_LOG("P:%u check_svr_timeout(cli_cmd:0x%04X wait_svr_cmd:0x%04X)", 
            player->userid, player->cli_wait_cmd, player->wait_svr_cmd);

    player->wait_svr_cmd = 0;
    send_err_to_player(player, player->cli_wait_cmd,
            cli_err_sys_busy);
    return 0;
}

int clean_expired_items_timely(void* onwer, void* data)
{
    CHECK_TIMER(kTimerTypeCleanExpiredItemsTimely);
    clean_expired_items(player, true);
    player->clean_expired_items_timer = ADD_TIMER_EVENT_EX(
            player,
            kTimerTypeCleanExpiredItemsTimely,
            (void*)(player->userid),
            get_now_tv()->tv_sec + kTimerIntervalCleanExpiredItems);
    return 0;
}

int check_db_diamond_return_timely(void *owner, void *data)
{
    CHECK_TIMER(kTimerTypeCheckDbDiamondTimely);
    player->check_money_return = NULL;
    StatInfo stat_info;
    stat_info.add_info("proc_diamond_op_time_out", "online");
    stat_info.add_op(StatInfo::op_sum, "proc_diamond_op_time_out");
    g_stat_logger->log("后台监控", "钻石操作超时", "", "", stat_info);

    send_err_to_player(player, player->cli_wait_cmd, cli_err_sys_busy);
    return 0;
}

int flush_shop_timely(void *owner, void *data)
{
    CHECK_TIMER(kTimerTypeShopFlush);
    //刷新商店
    try_flush_all_shops(player, DO_IT_NOW);
    /*
    try_flush_shop(player, true, onlineproto::MARKET_TYPE_DAILY);
    try_flush_shop(player, true, onlineproto::MARKET_TYPE_ELEM_DUP);
    try_flush_shop(player, true, onlineproto::MARKET_TYPE_ARENA);
    try_flush_shop(player, true, onlineproto::MARKET_TYPE_FAMILY);
    try_flush_shop(player, true, onlineproto::MARKET_TYPE_EXPED);
    try_flush_shop(player, true, onlineproto::MARKET_TYPE_NIGHT_RAID);
    */
    //定时刷新商店
    try_setup_shop_timer(player);
    return 0;
}

int check_escort_state(void *owner, void *data)
{
	std::set<uint64_t> uids;
	g_escort_mgr.check_finish_escort_players(uids);
	FOREACH(uids, it) {
		role_info_t role_info = KEY_ROLE(*it);
		g_escort_mgr.sync_player_info_other(
				role_info.userid, role_info.u_create_tm, 
				onlineproto::ES_SHIP_DISAPPEAR);

		g_escort_mgr.inform_player_escort_end(
				role_info.userid, role_info.u_create_tm);

		g_escort_mgr.del_player_from_escort(role_info.userid, role_info.u_create_tm);
		Utils::write_msglog_new(*it, "功能", "运宝", "运宝完成");
	}
	EscortUtils::clear_other_attr(uids);
	//
	/*
	uint32_t total_num = 0;
	g_escort_mgr.get_escort_num(total_num);
	if (0 == total_num) {
		return 0;
	}
	*/

	ADD_TIMER_EVENT_EX(
			&g_escort_timer, 
			kTimerTypeTestEscortFinish, 
			0, 
			NOW() + kTimerIntervalTestEscort);
	return 0;
}

int remove_from_dive_list(void *owner, void *data)
{
    CHECK_TIMER(kTimerTypeClearDive);
    g_dive.remove_diver_by_userid(player->userid);
    player->dive_timer = NULL;
	SET_A(kDailyDiveIsStart, 0);
    return 0;
}

int add_player_vp(void *owner, void *data)
{
    CHECK_TIMER(kTimerTypeVpAdd);
    //12分钟恢复体力1点
    //uint32_t max_vp = is_this_year_vip(player) ?SVIP_VP :is_vip(player) ?VIP_VP :NORMAL_VP;
	uint32_t max_vp = is_gold_vip(player) ?SVIP_VP : is_silver_vip(player) ?VIP_VP : NORMAL_VP;
    if (GET_A(kAttrCurVp) < max_vp) {
        if (GET_A(kAttrCurVp) + VP_ADD_VAL > max_vp) {
            SET_A(kAttrCurVp, max_vp);
        } else {
            ADD_A(kAttrCurVp, VP_ADD_VAL);
        }
    }
    SET_A(kAttrLastAddVpTime, NOW());
    player->vp_add_timer = ADD_TIMER_EVENT_EX(
            player,
            kTimerTypeVpAdd,
            (void*)(player->userid),
            NOW() + VP_ADD_GAP);
    return 0;
}

//暂时不用，该为日排名奖励
int send_arena_weekly_reward(void* owner, void* data)
{
	CHECK_TIMER(kTimerTypeSendArenaWeeklyReward);
	if (player->wait_svr_cmd) {
        player->weekly_arena_reward_timer = ADD_TIMER_EVENT_EX(
                player,
                kTimerTypeSendArenaWeeklyReward,
                (void*)(player->userid),
                NOW() + 3);

		return 0;
	}

    //重新定时
	time_t next_friday_start_time = TimeUtils::get_next_x_time(NOW(), 5);
	uint32_t time_inval = taomee::ranged_random(0, 300);
	uint32_t send_arana_reward_time = time_inval + next_friday_start_time;	
	player->weekly_arena_reward_timer = ADD_TIMER_EVENT_EX(
			player,
			kTimerTypeSendArenaWeeklyReward,
			(void*)(player->userid),
			send_arana_reward_time);

	player->cli_wait_cmd = cli_cmd_cs_0x0233_fake_cmd_for_arena_weeklyrank;
	//去redis中查询玩家的周排名
	//RankUtils::get_player_arena_week_rank(player);
	uint32_t sub_key = TimeUtils::get_prev_friday_date();
	std::vector<role_info_t> roles;
    role_info_t role;
    role.userid = player->userid;
    role.u_create_tm = player->create_tm;
	roles.push_back(role);
	RankUtils::get_user_rank_info(
			player, commonproto::RANKING_ARENA,
			sub_key, roles,
			commonproto::RANKING_ORDER_ASC);
	return 0;
}

int send_arena_daily_reward(void* owner, void* data)
{
	CHECK_TIMER(kTimerTypeSendArenaDailyReward);
	if (player->wait_svr_cmd) {
		player->daily_arena_reward_timer = ADD_TIMER_EVENT_EX(
				player,
				kTimerTypeSendArenaDailyReward,
				(void*)(player->userid),
				NOW() + 3);
		return 0;
	}
	//time_t  next_day_start_tm = TimeUtils::second_at_day_start(1);
	time_t next_start_tm =  TimeUtils::cal_time_based_on_hms(ARENA_DAILY_REWARD_TM_PONIT);
	uint32_t time_inval = taomee::ranged_random(10, 30);
	uint32_t send_reward_tm = time_inval + next_start_tm + DAY_SECS;
	player->daily_arena_reward_timer = ADD_TIMER_EVENT_EX(
			player,
			kTimerTypeSendArenaDailyReward,
			(void*)(player->userid),
			send_reward_tm);

	player->cli_wait_cmd = cli_cmd_cs_0x0233_fake_cmd_for_arena_weeklyrank;
	uint32_t sub_key = TimeUtils::time_to_date(NOW());
    std::vector<rank_key_order_t> rank_vec;
    struct rank_key_order_t tmp;
    tmp.key.key = commonproto::RANKING_ARENA;
    tmp.key.sub_key = sub_key;
    tmp.order = commonproto::RANKING_ORDER_ASC;
    rank_vec.push_back(tmp);
    tmp.key.key = commonproto::RANKING_RPVP;
    tmp.key.sub_key = sub_key;
    tmp.order = commonproto::RANKING_ORDER_DESC;
    rank_vec.push_back(tmp);
	RankUtils::get_user_rank_info_by_keys(
			player, rank_vec,
            player->userid, player->create_tm);
	return 0;
}

int send_weekly_activity_rank_reward(void* owner, void* data)
{
	CHECK_TIMER(kTimerTypeWeeklyActivityRankReward);
	if (player->wait_svr_cmd) {
		player->weekly_activity_rank_reward_timer= ADD_TIMER_EVENT_EX(
				player,
				kTimerTypeWeeklyActivityRankReward,
				(void*)(player->userid),
				NOW() + 3);
		return 0;
	}

	uint32_t reward_flag = GET_A(kAttrWeeklyActivityRankFlag);
	if (!taomee::test_bit_on(reward_flag, 4)) {

		player->cli_wait_cmd = cli_cmd_cs_0x0255_fake_cmd_for_weekly_activity_rank;

		uint32_t sub_key = TimeUtils::get_start_time(
				TM_CONF_KEY_KUROME_SEND_DESSERT, 1);

		std::vector<role_info_t> role_vec;
		role_info_t role_info;
		role_info.userid= player->userid;
		role_info.u_create_tm = player->create_tm;
		role_vec.push_back(role_info);

		RankUtils::get_user_rank_info(
				player, commonproto::RANKING_KUROME_SEND_DESSERT, 
				sub_key, role_vec, commonproto::RANKING_ORDER_DESC);
	}
			
	return 0;
}

int send_open_srv_gold_consume_reward(void* owner, void* data)
{
	CHECK_TIMER(kTimerTypeSendGoldConsumeReward);
	if (player->wait_svr_cmd) {
		player->open_srv_gold_consume_reward_timer = ADD_TIMER_EVENT_EX(
				player,
				kTimerTypeSendGoldConsumeReward,
				(void*)(player->userid),
				NOW() + 3);
		return 0;
	}

	uint32_t reward_flag = GET_A(kAttrOpenServRankingFlag);
	if (!taomee::test_bit_on(reward_flag, 3)) {

		player->cli_wait_cmd = cli_cmd_cs_0x0254_fake_cmd_for_open_serv_rank;
		uint32_t start_day = g_srv_time_mgr.get_start_time(TM_SUBKEY_GOLD_CONSUME );

		std::vector<role_info_t> role_vec;
		role_info_t role_info;
		role_info.userid= player->userid;
		role_info.u_create_tm = player->create_tm;
		role_vec.push_back(role_info);

		RankUtils::get_user_rank_info(
				player, commonproto::RANKING_TL_GOLD_CONSUME, 
				start_day, role_vec, commonproto::RANKING_ORDER_DESC);
	}
			
	return 0;
}

int send_open_srv_power_reward(void* owner, void* data)
{
	CHECK_TIMER(kTimerTypeSendOpenSrvPowerReward);
	if (player->wait_svr_cmd) {
		player->open_srv_power_reward_timer = ADD_TIMER_EVENT_EX(
				player,
				kTimerTypeSendOpenSrvPowerReward,
				(void*)(player->userid),
				NOW() + 3);
		return 0;
	}

	uint32_t reward_flag = GET_A(kAttrOpenServRankingFlag);
	if (!taomee::test_bit_on(reward_flag, 2)) {

		player->cli_wait_cmd = cli_cmd_cs_0x0254_fake_cmd_for_open_serv_rank;
		//活动时间内
		uint32_t start_day = g_srv_time_mgr.get_start_time(TM_SUBKEY_POWER);

		std::vector<role_info_t> role_vec;
		role_info_t role_info;
		role_info.userid= player->userid;
		role_info.u_create_tm = player->create_tm;
		role_vec.push_back(role_info);

		RankUtils::get_user_rank_info(
				player, commonproto::RANKING_TL_TOTAL_POWER, 
				start_day, role_vec, commonproto::RANKING_ORDER_DESC);
	}
			
	return 0;
}
int send_diamond_recharge_reward(void* owner, void* data)
{
	CHECK_TIMER(kTimerTypeSendDiamondRechargeReward);
	if (player->wait_svr_cmd) {
		player->diamond_recharge_reward_timer = ADD_TIMER_EVENT_EX(
				player,
				kTimerTypeSendDiamondRechargeReward,
				(void*)(player->userid),
				NOW() + 3);
		return 0;
	}

	uint32_t reward_flag = GET_A(kAttrOpenServRankingFlag);
	// if (!taomee::test_bit_on(reward_flag, 1) && 
			// TimeUtils::is_current_time_valid(TM_CONF_KEY_RANKING_GIFT_TIME_LIMIT, 1)) {
	if (!taomee::test_bit_on(reward_flag, 1)) {

		player->cli_wait_cmd =cli_cmd_cs_0x0254_fake_cmd_for_open_serv_rank;
		//活动时间内
		// uint32_t start_day = TimeUtils::get_start_time(TM_CONF_KEY_RANKING_TIME_LIMIT, 1);
		uint32_t start_day = g_srv_time_mgr.get_start_time(TM_SUBKEY_DIAMOND_RECHARGE);

		std::vector<role_info_t> role_vec;
		role_info_t role_info;
		role_info.userid= player->userid;
		role_info.u_create_tm = player->create_tm;
		role_vec.push_back(role_info);

		RankUtils::get_user_rank_info(
				player, commonproto::RANKING_TL_DIAMOND_RECHARGE, 
				start_day, role_vec, commonproto::RANKING_ORDER_DESC);
	}
			
	return 0;
}

int exerc_pet_add_exp(void* owner, void* data)
{
	const uint32_t ADD_EXP_COEF = commonproto::HM_ADD_EXP_COEF;
	const uint32_t TIME_INTER = commonproto::HM_TIME_INTER;
	const uint32_t PET_EXERCISE_DURATION = commonproto::HM_PET_EXERCISE_DURATION;
	CHECK_TIMER(kTimerTypeExercisePetAddExp);
	//给正在锻炼中的精灵加经验
	std::vector<Pet*> pets_vec;
	pets_vec.clear();
	PetUtils::get_in_exercise_pets(player, pets_vec);
	FOREACH(pets_vec, it) {
		Pet* pet = NULL;	
		pet = *it;
		if (pet == NULL) {
			continue;
		}
		uint32_t last_add_exp_tm = pet->last_add_exp_tm();
		//先判断精灵锻炼时间是否已经结束
		if (PetUtils::check_pet_exercise_over(pet, PET_EXERCISE_DURATION)) {
			pet->set_exercise_tm(0);
			pet->set_is_excercise(false);
			pet->set_last_add_exp_tm(0);
		}
		if (PetUtils::check_is_pet_level_max(pet)) {
			continue;
		}
		//计算上次加经验以来经历过多少个30分
		uint32_t count = (NOW() - last_add_exp_tm) / TIME_INTER;
		uint32_t add_exp = ADD_EXP_COEF * pet->level() * count;
		if (add_exp == 0) {
			continue;
		}
		uint32_t real_add_exp;
		PetUtils::add_pet_exp(player, pet, add_exp, 
				real_add_exp, ADDICT_DETEC,
				onlineproto::EXP_FROM_EXERCISE);
	}
	
	//重新获得当前仍处于锻炼时间内的精灵集合
	//若集合不为空，则设定下一阶段的定时器
	pets_vec.clear();
	PetUtils::get_in_exercise_pets(player, pets_vec);
	if (pets_vec.empty()) {
		player->exercise_pets_add_exp = NULL;
	} else {
		player->exercise_pets_add_exp = ADD_TIMER_EVENT_EX(
			player,
			kTimerTypeExercisePetAddExp,
			(void*)(player->userid),
			get_now_tv()->tv_sec + kTimerIntervalExercPetAddExp);
	}
	return 0;
}

int check_world_boss_timely(void *owner, void *data)
{
    ADD_TIMER_EVENT_EX(&g_world_boss_timer,
            kTimerTypeWorldBossCheck,
            0,
            NOW() + kTimerIntervalWorldBoss);

    world_boss_dup_info_t *dup_info = g_world_boss_mgr.get_world_boss_dup_info(
            commonproto::WORLD_BOSS_DUP_ID_1);
    if (dup_info == NULL) {
        g_world_boss_mgr.init(commonproto::WORLD_BOSS_DUP_ID_1);
        dup_info = g_world_boss_mgr.get_world_boss_dup_info(
            commonproto::WORLD_BOSS_DUP_ID_1);
    }

    uint32_t nowtime = NOW();
    bool ready_flag = TimeUtils::is_time_valid(nowtime, 3, 1);
    bool fight_flag = TimeUtils::is_time_valid(nowtime, 3, 2);

    uint32_t dup_id = commonproto::WORLD_BOSS_DUP_ID_1;

   // 实时触发
    onlineproto::sc_0x023E_noti_world_boss_dup_status noti_msg;
    if ((dup_info->status == commonproto::WORLD_BOSS_DUP_CLOSED ||
            dup_info->status == commonproto::WORLD_BOSS_DUP_FIGHT) && 
            ready_flag == true) {
        // 通知世界boss进入准备阶段
        dup_info->status = commonproto::WORLD_BOSS_DUP_READY;
        noti_msg.set_status((commonproto::world_boss_dup_status_t)dup_info->status);
        g_player_manager->send_msg_to_all_player(
                cli_cmd_cs_0x023E_noti_world_boss_dup_status, noti_msg);

        std::vector<uint32_t> paras;
        g_world_boss_mgr.world_boss_sys_notify(
                commonproto::WORLD_BOSS_STATUS_NOTI_READY, dup_id, paras);

        g_world_boss_mgr.clear_dup_record(dup_id);
        dup_info->status = commonproto::WORLD_BOSS_DUP_READY;
        dup_info->start_time = g_world_boss_mgr.compute_start_end_time(
                commonproto::WORLD_BOSS_DUP_ID_1, nowtime, 0);

        // 从战斗阶段跳到准备阶段，只有改系统时间或者配表错误才会出现的情况
        // 关闭挑战，清除记录，不发奖励
        if (dup_info->status == commonproto::WORLD_BOSS_DUP_FIGHT) {
            // 触发世界boss副本事件结束
            const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
            if (dup == NULL) {
                ERROR_TLOG("world boss end ,dup id not exist,dup_id=[%u]", dup_id);
                return 0;
            } 

            player_t tmp_player;
            tmp_player.userid = 0;
            battleproto::cs_battle_duplicate_trig btl_in_;
            btl_in_.set_dup_id(dup_id);
            btl_in_.set_map_id(dup->map_id);
            btl_in_.set_trig_type(commonproto::WORLD_BOSS_TRIG_SHUTDOWN);
            DupUtils::send_to_battle(&tmp_player, btl_cmd_duplicate_trig, btl_in_, NO_WAIT_SVR);

            // 清除战斗记录
            g_world_boss_mgr.clear_dup_record(dup_id);
            std::string debug_msg = "世界boss自动触发结束了";
            PlayerUtils::noti_gm_debug_msg(0, debug_msg);
        }
    } else if ((dup_info->status == commonproto::WORLD_BOSS_DUP_READY || 
                dup_info->status == commonproto::WORLD_BOSS_DUP_CLOSED) &&
            fight_flag == true) {
        // 通知世界boss挑战开始
        g_world_boss_mgr.clear_dup_record(dup_id);
        dup_info->status = commonproto::WORLD_BOSS_DUP_FIGHT;
        dup_info->start_time = g_world_boss_mgr.compute_start_end_time(
                commonproto::WORLD_BOSS_DUP_ID_1, nowtime, 0);
        noti_msg.set_status((commonproto::world_boss_dup_status_t)dup_info->status);
        g_player_manager->send_msg_to_all_player(
                cli_cmd_cs_0x023E_noti_world_boss_dup_status, noti_msg);

        // 触发世界boss副本事件开始
        player_t tmp_player;
        tmp_player.userid = 0;
        battleproto::cs_battle_duplicate_trig btl_in_;
        btl_in_.set_dup_id(dup_id);

        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
        if (dup == NULL) {
            ERROR_TLOG("world boss start ,dup id not exist,dup_id=[%u]", dup_id);
            return cli_err_duplicate_id_not_found;
        } 

        btl_in_.set_dup_id(dup_id);
        btl_in_.set_map_id(dup->map_id);
        btl_in_.set_trig_type(commonproto::WORLD_BOSS_TRIG_OPEN);
        DupUtils::send_to_battle(&tmp_player, btl_cmd_duplicate_trig, btl_in_, NO_WAIT_SVR);

        std::string debug_msg = "世界boss自动触发开始了";
        PlayerUtils::noti_gm_debug_msg(0, debug_msg);

        std::vector<uint32_t> paras;
        g_world_boss_mgr.world_boss_sys_notify(
                commonproto::WORLD_BOSS_STATUS_NOTI_COUNTDOWN, dup_id, paras);
        g_world_boss_mgr.world_boss_sys_notify(
                commonproto::WORLD_BOSS_STATUS_NOTI_FIGHT, dup_id, paras);
    } else if (dup_info->status == commonproto::WORLD_BOSS_DUP_FIGHT && 
            fight_flag == false) {
        //通知世界boss结束
        dup_info->status = commonproto::WORLD_BOSS_DUP_CLOSED;
        noti_msg.set_status((commonproto::world_boss_dup_status_t)dup_info->status);
        g_player_manager->send_msg_to_all_player(
                cli_cmd_cs_0x023E_noti_world_boss_dup_status, noti_msg);

        // 触发奖励
        g_world_boss_mgr.give_world_boss_reward(commonproto::WORLD_BOSS_DUP_ID_1);

        // 触发世界boss副本事件结束
        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
        if (dup == NULL) {
            ERROR_TLOG("world boss end ,dup id not exist,dup_id=[%u]", dup_id);
            return cli_err_duplicate_id_not_found;
        } 

        player_t tmp_player;
        tmp_player.userid = 0;
        battleproto::cs_battle_duplicate_trig btl_in_;
        btl_in_.set_dup_id(dup_id);
        btl_in_.set_map_id(dup->map_id);
        btl_in_.set_trig_type(commonproto::WORLD_BOSS_TRIG_SHUTDOWN);
        DupUtils::send_to_battle(&tmp_player, btl_cmd_duplicate_trig, btl_in_, NO_WAIT_SVR);

        // 清除战斗记录
        g_world_boss_mgr.clear_dup_record(dup_id);
        dup_info->end_time = g_world_boss_mgr.compute_start_end_time(
                commonproto::WORLD_BOSS_DUP_ID_1, nowtime, 1);

        std::string debug_msg = "世界boss自动触发结束了";
        PlayerUtils::noti_gm_debug_msg(0, debug_msg);

        Utils::write_msglog_new(10001, "奖励", "世界boss", "副本结束");
    } else {
        // 其他状态，不处理
    }

    DEBUG_TLOG("world boss timer, ready_flag:%u, fight_flag:%u,dup_status:%u,dup_start_tm:%u", 
            ready_flag, fight_flag, dup_info->status, dup_info->start_time);
    return 0;
}

int daily_00_dump_rank_utils()
{
	//只有每个服的online_id值最小的那个子进程去请求排名服
	if (g_online_id != g_server_id * 100 + 1) {
		return 0;
	}
	return 0;
}

int weekly_00_dump_rank_utils()
{
	//只有每个服的online_id值最小的那个子进程去请求排名服
	if (g_online_id != g_server_id * 100 + 1) {
		return 0;
	}
	if (!TimeUtils::is_current_time_valid(TM_CONF_KEY_ACTIVITY_OPEN_TIME, 2)) {
		return 0;
	}
	uint32_t friday_date = TimeUtils::get_prev_friday_date();
	uint32_t time_stamp = TimeUtils::date_to_time(friday_date);
	std::vector<dump_rank_key_info_t> key_vec;
	dump_rank_key_info_t tmp;

	tmp.orig_key.key = commonproto::RANKING_ARENA;
    tmp.orig_key.sub_key = 0;
    tmp.new_key.key = commonproto::RANKING_ARENA;
	tmp.new_key.sub_key = time_stamp;
    tmp.del_orig_key = 0;
	key_vec.push_back(tmp);

	tmp.orig_key.key = commonproto::RANKING_MOUNT;
    tmp.orig_key.sub_key = 0;
    tmp.new_key.key = commonproto::RANKING_MOUNT;
	tmp.new_key.sub_key = friday_date;
    tmp.del_orig_key = 0;
	key_vec.push_back(tmp);

	tmp.orig_key.key = commonproto::RANKING_WING;
    tmp.orig_key.sub_key = 0;
    tmp.new_key.key = commonproto::RANKING_WING;
    tmp.new_key.sub_key = friday_date;
    tmp.del_orig_key = 0;
	key_vec.push_back(tmp);

	tmp.orig_key.key = commonproto::RANKING_ACHIEVEMENT;
    tmp.orig_key.sub_key = 0;
	tmp.new_key.key = commonproto::RANKING_ACHIEVEMENT;
    tmp.new_key.sub_key = friday_date;
    tmp.del_orig_key = 0;
	key_vec.push_back(tmp);

    tmp.orig_key.key = commonproto::RANKING_TOTAL_POWER;
    tmp.orig_key.sub_key = 0;
	tmp.new_key.key = commonproto::RANKING_TOTAL_POWER;
	tmp.new_key.sub_key = friday_date;
    tmp.del_orig_key = 0;
	key_vec.push_back(tmp);

	tmp.orig_key.key = commonproto::RANKING_SPIRIT_TOTAL_POWER;
    tmp.orig_key.sub_key = 0;
    tmp.new_key.key = commonproto::RANKING_SPIRIT_TOTAL_POWER;
	tmp.new_key.sub_key = friday_date;
    tmp.del_orig_key = 0;
	key_vec.push_back(tmp);

	tmp.orig_key.key = commonproto::RANKING_RPVP;
	tmp.orig_key.sub_key = 0;
	tmp.new_key.key = commonproto::RANKING_RPVP;
	tmp.new_key.sub_key = time_stamp;
	tmp.del_orig_key = 0;
	key_vec.push_back(tmp);

	player_t tmp_player;
	tmp_player.userid = 0;
	RankUtils::dump_rank(&tmp_player, key_vec);
	return 0;
}

//每日21点需要dump的排名加在这里, 相应的每日21点获取排名的应该在dump完成后再去获取
//一般延迟10秒左右获取
int daily_21_dump_rank(void *owner, void *data)
{
    //只有每个服的online_id值最小的那个子进程去请求排名服
    if (g_online_id != g_server_id * 100 + 1) {
        return 0;
    }

	std::vector<dump_rank_key_info_t> key_vec;
	dump_rank_key_info_t tmp;

	tmp.orig_key.key = commonproto::RANKING_RPVP;
    tmp.orig_key.sub_key = 0;
    tmp.new_key.key = commonproto::RANKING_RPVP;
    tmp.new_key.sub_key = TimeUtils::time_to_date(NOW());

    //今天是周日 dump之后清0 RPVP
    uint32_t last_sunday_time = TimeUtils::get_last_x_time(NOW(), 0);
    if (TimeUtils::time_to_date(last_sunday_time) == TimeUtils::time_to_date(NOW())) {
        tmp.del_orig_key = 1;
    } else {
        tmp.del_orig_key = 0;
    }
	key_vec.push_back(tmp);

	player_t tmp_player;
	tmp_player.userid = 0;
	RankUtils::dump_rank(&tmp_player, key_vec);

	ADD_TIMER_EVENT_EX(&g_dump_rank_timer, 
			kTimerTypeDaily21DumpRank, 
			0, NOW() + TimeUtils::second_to_hm(2100) - 5);
    return 0;
}

int register_timers()
{
    REGISTER_TIMER_TYPE(kTimerTypeReconnectServiceTimely, reconnect_service_timely);
    REGISTER_TIMER_TYPE(kTimerTypeSyncClientTimeTimely, sync_client_time_timely);
    REGISTER_TIMER_TYPE(kTimerTypeDailyOperation, daily_operation);
    REGISTER_TIMER_TYPE(kTimerTypeCheckSvrTimeout, check_svr_timeout);
    REGISTER_TIMER_TYPE(kTimerTypeCleanExpiredItemsTimely, clean_expired_items_timely);
    REGISTER_TIMER_TYPE(kTimerTypeSysNotiOffline, sys_offline_noti);
    REGISTER_TIMER_TYPE(kTimerTypeCheckDbDiamondTimely, check_db_diamond_return_timely);
    REGISTER_TIMER_TYPE(kTimerTypeSwRegTimely, reg_to_switch_req);
    REGISTER_TIMER_TYPE(kTimerTypeShopFlush, flush_shop_timely);
    REGISTER_TIMER_TYPE(kTimerTypeTestEscortFinish, check_escort_state);
    REGISTER_TIMER_TYPE(kTimerTypeClearDive, remove_from_dive_list);
    REGISTER_TIMER_TYPE(kTimerTypeVpAdd, add_player_vp);
    //REGISTER_TIMER_TYPE(kTimerTypeSendArenaWeeklyReward, send_arena_weekly_reward);
    //REGISTER_TIMER_TYPE(kTimerTypeExercisePetAddExp, exerc_pet_add_exp);
    REGISTER_TIMER_TYPE(kTimerTypeWorldBossCheck, check_world_boss_timely);
    REGISTER_TIMER_TYPE(kTimerTypeDailyKickOff, daily_kick_off);
    REGISTER_TIMER_TYPE(kTimerTypeDailyResetDive, daily_reset_dive);
    REGISTER_TIMER_TYPE(kTimerTypeSendArenaDailyReward, send_arena_daily_reward);
    REGISTER_TIMER_TYPE(kTimerTypeSendDiamondRechargeReward, send_diamond_recharge_reward);
    REGISTER_TIMER_TYPE(kTimerTypeSendOpenSrvPowerReward, send_open_srv_power_reward);
    REGISTER_TIMER_TYPE(kTimerTypeSendGoldConsumeReward, send_open_srv_gold_consume_reward);
    REGISTER_TIMER_TYPE(kTimerTypeWeeklyActivityRankReward, send_weekly_activity_rank_reward);
    REGISTER_TIMER_TYPE(kTimerTypeDaily21DumpRank, daily_21_dump_rank);
    return 0;
}


