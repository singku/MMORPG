#include <boost/lexical_cast.hpp>
#include "login_processor.h"
#include "player_manager.h"
#include "timer_procs.h"
#include "proto_processor.h"
#include "switch_proto.h"
#include "statlogger/statlogger.h"
#include "user_action_log_utils.h"
#include "data_proto_utils.h"
#include "item.h"
#include "map_utils.h"
#include "player_utils.h"
#include "pet_utils.h"
#include "tran_card.h"
#include "family_utils.h"
#include "escort_utils.h"
#include "rank_utils.h"
#include "home_data.h"
#include "task_utils.h"
#include "attr_utils.h"
#include "time_utils.h"
#include "prize.h"
#include "mail_utils.h"
#include "arena_conf.h"
#include "rune.h"
#include "duplicate_conf.h"
#include "duplicate_utils.h"
#include "arena.h"
#include "equip_utils.h"
#include "prize_processor.h"

const static int CStartMapId = 14;
const uint32_t base_exp_max = 300;

int LoginCmdProcessor::init_role(player_t *player) 
{
    uint32_t prof_idx = 0;
    prof_idx = GET_A(kAttrCurProf);
    if(prof_idx == PROF_NONE || prof_idx >= PROF_COUNT) {
        prof_idx = PROF_WARRIOR;
        SET_A(kAttrCurProf, prof_idx);
    }

    PetUtils::calc_pet_bag_size(player);

    if(!GET_A(kBaseInfoAttrStart)) {
        /*
        //设置年费VIP
        */
        if (db_out_.pet_list().pet_list_size() == 0) { //玩家还没有精灵
#if 0
            uint32_t create_tm;
            if (PetUtils::create_pet(player, 1001, 1, false, &create_tm)) {
                ERROR_TLOG("Login Create Pet failed!!");
            } else {
                if (player->pets->count(create_tm) == 0) {
                    ERROR_TLOG("Pet Create but not found when login");
                } else {
                    Pet *pet = &(player->pets->find(create_tm))->second;
                    pet->set_fight_pos(1);
                    player->fight_pet[pet->fight_pos() - 1] = pet;
                    PetUtils::save_pet(player, *pet, false, false);
                }
            }
            PetUtils::create_pet(player, 1101, 1, false, &create_tm);
            PetUtils::create_pet(player, 1013, 1, false, &create_tm);
            PetUtils::create_pet(player, 1015, 1, false, &create_tm);
            PetUtils::create_pet(player, 1017, 1, false, &create_tm);
            PetUtils::create_pet(player, 1019, 1, false, &create_tm);
            PetUtils::create_pet(player, 1041, 1, false, &create_tm);
            PetUtils::create_pet(player, 1027, 1, false, &create_tm);
            PetUtils::create_pet(player, 1035, 1, false, &create_tm);
            PetUtils::create_pet(player, 1001, 1, false, &create_tm);
#endif
        }
        SET_A(kAttrLv, 1);
        SET_A(kAttrExp, 0);

		//符文馆阵列默认第一级是开启的
		SET_A(kAttrCallLevelInfo, 1);
		//小屋房型默认是 15
		SET_A(kAttrHomeType, commonproto::HM_DEFAULT_TYPE);

        PlayerUtils::calc_player_battle_value(player);
#if 0
        player->m_tran_card->add_tranCard(player, 40001, 1);
        player->m_tran_card->add_tranCard(player, 40002, 1);

        add_item_info_t add_item;
        std::vector<add_item_info_t> add_vec;
        add_item.item_id = 10000; add_item.count = 1; add_item.level = 1; add_vec.push_back(add_item);
        add_item.item_id = 11000; add_item.count = 1; add_item.level = 1; add_vec.push_back(add_item);
        add_item.item_id = 12000; add_item.count = 1; add_item.level = 1; add_vec.push_back(add_item);
        add_item.item_id = 13000; add_item.count = 1; add_item.level = 1; add_vec.push_back(add_item);
        add_item.item_id = 14000; add_item.count = 1; add_item.level = 1; add_vec.push_back(add_item);

        add_item.item_id = 15101; add_item.count = 1; add_vec.push_back(add_item);
	
        swap_item_by_item_id(player, 0, &add_vec);
#endif
        SET_A(kBaseInfoAttrStart, 1);
    }

    return 0; 

}

int LoginCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;

    player->userid = cli_in_.uid();
    player->create_tm = cli_in_.u_create_tm();

    Utils::write_msglog_new(player->userid, "用户监控", "登陆", "收到进入游戏请求");
    //统计收到登陆请求
    g_stat_logger->new_trans(StatLogger::bSendOnlineSucc, Utils::to_string(player->userid));

    // 查找该米米号是否在服务器中，关闭连接
    player_t* exist_player = g_player_manager->get_player_by_userid(player->userid);

    if (exist_player) {
        send_err_to_player(exist_player, 276, cli_err_kick_off);
        close_client_conn(exist_player->fdsess->fd);
    }

    //如果换线登陆 且人数达到上限
    if (cli_in_.is_switch_online() && g_player_manager->total_players() >= MAX_PLAYERS_PER_ONLINE) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_too_many_players_online);
    }

    player_t* player_new = g_player_manager->create_player(
            player->fdsess, player->userid);
    if (!player_new) {
        KERROR_LOG(player->userid, "Alloc memory for player %u failed",
                player->userid);
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_sys_err);
    }
    player_new->seqno = player->seqno;
    player_new->temp_info.cache_string->assign(*(player->temp_info.cache_string));
    player_new->temp_info.cache_out_string->assign(*(player->temp_info.cache_out_string));
    player_new->cli_wait_cmd = player->cli_wait_cmd;
    player_new->create_tm = player->create_tm;
    player_new->temp_info.is_old_machine = cli_in_.is_old_machine();
    player_new->temp_info.is_switch_online = cli_in_.is_switch_online();

    g_player_manager->add_player_to_manager(player_new);

    //测试账号跳过session检测
    if (unlikely(g_test_for_robot == 1)) {
        if (player_new->userid >= 10000 && player_new->userid <= 19999) {
            set_login_step_finished(player_new, login_steps_check_session);
            return this->proc_login(player_new);
        }
    }

    return this->proc_login(player_new);
}

int LoginCmdProcessor::proc_pkg_from_serv(
        player_t* player, const char* body, int bodylen)
{
    switch (player->serv_cmd) {
        case act_cmd_check_session: 
            return proc_pkg_from_serv_aft_check_session(player, body, bodylen);

        case act_cmd_is_active:
            return proc_pkg_from_serv_aft_get_is_active(player, body, bodylen);

        case db_cmd_get_login_info:
            return proc_pkg_from_serv_aft_get_login_info(player, body, bodylen);

        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_get_family_info(player, body, bodylen);

		case ranking_cmd_get_user_multi_rank:
			return proc_pkg_from_serv_aft_get_arena_rank(player, body, bodylen);

        default:
            return 0;
    }

    return 0;
}

uint32_t LoginCmdProcessor::proc_errno_from_serv(
        player_t* player, uint32_t ret)
{
    if (ret == act_err_check_session_failed) {
        return cli_err_check_session_failed;
    }

    // cli_err_sys_err;
    return 0;
}

int LoginCmdProcessor::proc_pkg_from_serv_aft_check_session(
        player_t* player, const char* body, int bodylen)
{
    set_login_step_finished(player, login_steps_check_session);
    return this->proc_login(player);
}

int LoginCmdProcessor::proc_pkg_from_serv_aft_get_arena_rank(
		player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(rank_out_);
    ArenaUtils::proc_arena_rank_prize(player, rank_out_);
    set_login_step_finished(player, login_steps_get_arena_daily_rank);
	return this->proc_login(player);
}

int LoginCmdProcessor::proc_pkg_from_serv_aft_get_is_active(
        player_t* player, const char* body, int bodylen)
{
    act_is_active_ack_t* ack = (act_is_active_ack_t *)body;

    if (bodylen != sizeof(*ack)) {
        ERROR_TLOG("%u invalid is_active ack body len %d, expect %u",
                bodylen, sizeof(*ack));
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_sys_err); 
    }

    if (!ack->is_active) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_user_not_active); 
    }
    set_login_step_finished(player, login_steps_get_active);
    return this->proc_login(player);
}

int LoginCmdProcessor::proc_pkg_from_serv_aft_get_login_info(
        player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);

    // 解析db返回信息
    // 解析基本信息
    DataProtoUtils::unpack_base_info(player, db_out_.base_info());

    //判断是否封号
    if (db_out_.base_info().frozen_end_time() >= NOW()) {
        send_err_to_player(player, player->cli_wait_cmd, cli_err_account_frozen);
        close_client_conn(player->fdsess->fd);
        return 0;
    }
    // 解析属性信息
    DataProtoUtils::unpack_player_attrs(player, db_out_.attr_list());

    // 解析物品信息
    DataProtoUtils::unpack_player_item_info(player, db_out_.mutable_item_list());

    //符文信息
    DataProtoUtils::unpack_rune_data(player, db_out_.rune_list());

    // 解析精灵信息
    DataProtoUtils::unpack_player_pet_info(player, db_out_.pet_list());

	//任务信息
	DataProtoUtils::unpack_player_task(player, db_out_.task_list());

	//解析符文召唤阵列(在解析属性信息之后)
	player->rune_meseum->get_call_level_set_from_DB(player);

	//卡牌信息
	DataProtoUtils::unpack_tran_data(player);

	//好友信息
	DataProtoUtils::unpack_friend_data(player, db_out_.friend_list());

    //解析buff
    DataProtoUtils::unpack_player_buff(player, db_out_.user_buff());
	init_role(player);

    // 推送离线家族消息
    FamilyUtils::send_offline_family_msg_notice(player, db_out_);

    // 解析申请的家族记录
    DataProtoUtils::unpack_player_apply_family_ids(player, db_out_.family_apply_record());

    // 任务配表修改处理
    TaskUtils::task_update_to_config(player);

    // 解析元素商店刷新物品
    //DataProtoUtils::unpack_elem_dup_shop_items(player, db_out_.elem_dup_shop_info());
    // 解析竞技场商店刷新物品
    //DataProtoUtils::unpack_arena_shop_items(player, db_out_.arena_shop_info());
    // 解析家族商店刷新物品
    //DataProtoUtils::unpack_family_shop_items(player, db_out_.family_shop_info());
	// 解析远征商店刷新物品
    //DataProtoUtils::unpack_exped_shop_items(player, db_out_.exped_shop_info());

    DataProtoUtils::unpack_shop_items(player, MARKET_TYPE_DAILY, db_out_.daily_shop_info());
    DataProtoUtils::unpack_shop_items(player, MARKET_TYPE_FAMILY, db_out_.family_shop_info());
    DataProtoUtils::unpack_shop_items(player, MARKET_TYPE_ARENA, db_out_.arena_shop_info());
    DataProtoUtils::unpack_shop_items(player, MARKET_TYPE_ELEM_DUP, db_out_.elem_dup_shop_info());
    DataProtoUtils::unpack_shop_items(player, MARKET_TYPE_EXPED, db_out_.exped_shop_info());
    DataProtoUtils::unpack_shop_items(player, MARKET_TYPE_NIGHT_RAID, db_out_.night_raid_shop_info());

    DataProtoUtils::unpack_shop_items(player, MARKET_TYPE_SMELT_MONEY, db_out_.smelter_money_info());
    DataProtoUtils::unpack_shop_items(player, MARKET_TYPE_SMELT_GOLD, db_out_.smelter_gold_info());

	DataProtoUtils::unpack_all_titles_info(player, db_out_.title_list());

	DataProtoUtils::unpack_mine_ids(player, db_out_.mine_id_list());

	DataProtoUtils::unpack_mine_tmp_info(player, db_out_.mine_info_list());

	DataProtoUtils::unpack_mine_match_ids(player, db_out_.opponent_mine_ids());

	//DataProtoUtils::unpack_achieves_info(player, db_out_.achv_list());

    // 异常等级判定
    if (GET_A(kAttrGuideFinished)) {
        SET_A(kAttrLvBeforeStarter, 0);
    }
    if (GET_A(kAttrLvBeforeStarter)) {
        //有记录新手副本时的等级,说明新手副本没完服务器就挂了
        //恢复等级及战斗数值
        SET_A(kAttrLv, GET_A(kAttrLvBeforeStarter));
        PlayerUtils::calc_player_battle_value(player, onlineproto::SYNC_ATTR_STARTER);
    }
    set_login_step_finished(player, login_steps_get_login_info);
    return this->proc_login(player);
}

int LoginCmdProcessor::proc_pkg_from_serv_aft_get_family_info(
            player_t* player, const char* body, int bodylen)
{
    dbproto::sc_family_get_info     db_family_info_out_;
    PARSE_SVR_MSG(db_family_info_out_);

    // 家族不存在
    if (!db_family_info_out_.mutable_family_info()->has_family_id()) {
        FamilyUtils::clear_self_family_info(player);
    } else {
        SET_A(kAttrFamilyLevel, db_family_info_out_.mutable_family_info()->level());

        uint32_t family_id = GET_A(kAttrFamilyId);
        dbproto::cs_family_update_info   db_update_family_info_in_;
        dbproto::family_info_table_t *update_data = 
            db_update_family_info_in_.mutable_family_info();
        update_data->set_family_id(family_id);
        update_data->set_last_member_login_time(NOW());
        FamilyUtils::update_family_info(NULL, *update_data, dbproto::DB_UPDATE_NO_INSERT);
    }

    strncpy(player->family_name, 
            db_family_info_out_.family_info().family_name().c_str(), 
            sizeof(player->family_name) );

    set_login_step_finished(player, login_steps_get_family_info);
    return this->proc_login(player);
}

int LoginCmdProcessor::send_login_response(player_t* player)
{
    // 客户端返回包
    cli_out_.Clear();

    if (player->temp_info.is_switch_online) {
        cli_out_.set_is_switch_online(true);
    } else {
        cli_out_.set_is_switch_online(false);
    }

    const map_conf_t *mc = g_map_conf_mgr.find_map_conf(CStartMapId);
    if (!mc) {
        ERROR_LOG("find_map_conf fialed mapid: %u ",  CStartMapId);
        return -1;
    }
    player->cur_map_id = CStartMapId;
    int npos = mc->init_pos.size();
    int pos_index = ranged_random(0, npos-1);
    player->map_x = mc->init_pos[pos_index].x;
    player->map_y = mc->init_pos[pos_index].y;

    player->temp_info.can_use_diamond = true;
    uint32_t ini_seq = NOW() % ranged_random(1, 1000000);

    // 打包回复信息
    
    //非换线登陆需要打包所有信息 换线登陆只需要打包位置和初始序列号
    if (!player->temp_info.is_switch_online) {
        commonproto::player_base_info_t* base_info = cli_out_.mutable_user_info();
        DataProtoUtils::pack_player_base_info(player, base_info);
        DataProtoUtils::pack_player_attrs(player, cli_out_.mutable_attr_list());
        DataProtoUtils::pack_player_pet_list(player, cli_out_.mutable_bag_pet_list(), PET_LOC_BAG);
        DataProtoUtils::pack_player_item_info(player, cli_out_.mutable_item_list());
        DataProtoUtils::pack_player_task(player, cli_out_.mutable_task_list());
        DataProtoUtils::pack_rune_data(player, cli_out_.mutable_rune_list());
        DataProtoUtils::pack_tran_card_data(player, cli_out_);
        DataProtoUtils::pack_friend_data(player, cli_out_.mutable_friend_list());
        DataProtoUtils::pack_player_buff(player, cli_out_.mutable_user_buff());
        DataProtoUtils::pack_player_suit_buff_info(player, cli_out_.mutable_suit_buff_list());
		//DataProtoUtils::pack_all_achieves_info(player, cli_out_.mutable_achv_list());
    }

    cli_out_.set_xpos(player->map_x);
    cli_out_.set_ypos(player->map_y);
    cli_out_.set_ini_seq(ini_seq);

    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);

    player->seqno = ini_seq;
    return 0;
}

//所有的登录逻辑在这里处理
int LoginCmdProcessor::proc_login(player_t *player)
{
#define CHECK_AND_RUN_STEP(step) \
    do { \
        if (!is_login_step_finished(player, (step))) { \
            return step##_func(player); \
        } \
    } while (0)

    //check_session必须是第一步 此时需要使用cli_in_中的数据。   
    CHECK_AND_RUN_STEP(login_steps_check_session);
    CHECK_AND_RUN_STEP(login_steps_get_active);
    CHECK_AND_RUN_STEP(login_steps_get_login_info);
    CHECK_AND_RUN_STEP(login_steps_get_family_info);
	//CHECK_AND_RUN_STEP(login_steps_get_arena_weekly_rank);
    CHECK_AND_RUN_STEP(login_steps_get_arena_daily_rank);

    //以下是所有登录流程走完之后的操作
    Utils::write_msglog_new(player->userid, "用户监控", "登陆", "进入游戏成功");

	//如果牙牙活动过期，清理活动属性
	uint32_t clear_time = GET_A(kAttrActivClearTime);
	//在活动范围内而且没有清理过
	if (TimeUtils::is_current_time_valid(TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3)
		   	&& !TimeUtils::is_time_valid(clear_time, 
				TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3)) {
		AttrUtils::ranged_clear(player,
				(uint32_t)kAttrActivZhaoMu,
				(uint32_t)kAttrActivPetAdvance, true);
		SET_A(kAttrVipGetYayaFragmentFlag, 0);
		SET_A(kAttrActivClearTime, NOW());
	}

    //尝试清理每日数据
    try_init_player_daily_data(player);
    //如果在线的话 会在下一个日整点触发每日操作
    player->daily_op_timer = ADD_TIMER_EVENT_EX(
            player,
            kTimerTypeDailyOperation,
            (void*)(player->userid),
            TimeUtils::second_at_day_start(1));

    //登录清理过期道具(不通知客户端)
    clean_expired_items(player, false);
    //定期清理过期道具
    player->clean_expired_items_timer = ADD_TIMER_EVENT_EX(
            player, 
            kTimerTypeCleanExpiredItemsTimely,
            (void*)(player->userid),
            get_now_tv()->tv_sec + kTimerIntervalCleanExpiredItems);

	//连续登录天数累加(这段逻辑放在kAttrLastLoginTm更新之前)
	if (TimeUtils::is_same_day(GET_A(kAttrLastLoginTm), NOW()) == false) {
		if (TimeUtils::is_same_day(GET_A(kAttrLastLoginTm), NOW() - DAY_SECS)) {
			ADD_A(kAttrTotalLoginDays, 1);
		} else {
			SET_A(kAttrTotalLoginDays, 0);
		}
	}
	

    //更新上次登录时间
    SET_A(kAttrLastLoginTm, NOW());

    // 计算离线时间
    uint32_t offline_time = calc_daily_offline_time(player);
    SET_A(kDailyOfflineTime, offline_time);

    // 记录防沉迷今日登陆时间
    SET_A(kDailyLastLoginTime, NOW());

    // 记录当前登陆服务器id
    SET_A(kAttrLastOnlineId, g_online_id);

    //记录登陆日志
    UserActionLogUtils::write_db_log(player, dbproto::ActionTypeLogin, g_online_id, 0, 0, 0, 0);

    // 刷新家族成员信息,必须在玩家登陆时间更新完成后
    FamilyUtils::refresh_family_member_info(player);

    // 判断并补发世界boss奖励
    uint32_t reward_mask = GET_A(kAttrWorldBossRewardRecord);
    g_world_boss_mgr.give_world_boss_single_user_reward(
            (uint32_t)commonproto::WORLD_BOSS_DUP_ID_1, 
            player->userid, player->create_tm,
            reward_mask);

    //vip登录记录
    if (is_vip(player)) {
        Utils::write_msglog_new(player->userid, "vip", "日登陆数据", "VIP登陆");
    }
    if (is_gold_vip(player)) {
        Utils::write_msglog_new(player->userid, "vip", "日登陆数据", "黄金VIP登陆");
    } else if (is_silver_vip(player)) {
        Utils::write_msglog_new(player->userid, "vip", "日登陆数据", "白银VIP登陆");
    }
    //登录分布记录
    login_session_t *session = (login_session_t*)player->session;
    std::stringstream uid_str;
    uid_str << player->userid;
    g_stat_logger->login_online(uid_str.str(), "", "", 
            is_vip(player), GET_A(kAttrLv), 
            player->fdsess->remote_ip, 
            session->tad, "", session->browse, session->device, 
            session->version, session->resolution, session->network, 
            g_server_config.idc_zone == 0 ? "电信" : "网通");

    // 竞技场周排名奖励在线领取定时器(现在改为日排名)
	/*
	time_t next_friday_start_time = TimeUtils::get_next_x_time(NOW(), 5);
	uint32_t time_inval = taomee::ranged_random(0, 300);
	uint32_t send_arana_reward_time = time_inval + next_friday_start_time;	
	player->weekly_arena_reward_timer = ADD_TIMER_EVENT_EX(
			player,
			kTimerTypeSendArenaWeeklyReward,
			(void*)(player->userid),
			send_arana_reward_time);
	*/
	// 竞技场日排名奖励在线领取定时器
	/*
	time_t next_day_start_tm = TimeUtils::second_at_day_start(1);
	uint32_t time_inval = taomee::ranged_random(0, 300);
	uint32_t send_reward_tm = next_day_start_tm + time_inval;
	*/
	time_t next_start_tm =  TimeUtils::cal_time_based_on_hms(ARENA_DAILY_REWARD_TM_PONIT);
	//如果已经超过今天21点
	if (TimeUtils::test_gived_time_exceed_tm_point(NOW(), ARENA_DAILY_REWARD_TM_PONIT)) {
		next_start_tm = next_start_tm + DAY_SECS;
	}
	uint32_t time_inval = taomee::ranged_random(10, 30);
	uint32_t send_reward_tm = next_start_tm + time_inval;
	player->daily_arena_reward_timer = ADD_TIMER_EVENT_EX(
			player,
			kTimerTypeSendArenaDailyReward,
			(void*)(player->userid),
			send_reward_tm);

	//创建总战力榜称号奖励定时器
	

	//防沉迷
	if (check_player_clear_addicted(player)){
		//离线满足时间要求，防沉迷在线时间清零
		SET_A(kDailyOnlineTimeCleanByDay, 0);
		//离线时间抵消沉迷在线时间
		SET_A(kDailyOfflineTime, 0);
	}
    //在线防沉迷check定时器
    // player->check_addiction_noti = ADD_TIMER_EVENT_EX(
            // player,
            // kTimerTypeDailyAddictionNoti,
            // (void*)(player->userid),
            // NOW() + kTimerIntervaladdiction);//当天的23点59分58秒操作

    //上线判定体力
    if (GET_A(kAttrLastAddVpTime) < NOW()) {
        uint32_t gap = NOW() - GET_A(kAttrLastAddVpTime);
        if (gap >= VP_ADD_GAP) { //12分钟2点
            //uint32_t max_vp = is_this_year_vip(player) ?SVIP_VP :is_vip(player) ?VIP_VP :NORMAL_VP;
			uint32_t max_vp = is_gold_vip(player) ?SVIP_VP :is_silver_vip(player) ?VIP_VP :NORMAL_VP;
            if (GET_A(kAttrCurVp) < max_vp) {
                if (GET_A(kAttrCurVp) + gap / VP_ADD_GAP * VP_ADD_VAL > max_vp) {
                    SET_A(kAttrCurVp, max_vp);
                } else {
                    ADD_A(kAttrCurVp, gap / VP_ADD_GAP * VP_ADD_VAL);
                }
            }
            SET_A(kAttrLastAddVpTime, NOW());
        }
        uint32_t left = gap % VP_ADD_GAP;
        player->vp_add_timer = ADD_TIMER_EVENT_EX(
                player,
                kTimerTypeVpAdd,
                (void*)(player->userid),
                NOW() + left);
    }

    //刷新商店
    try_flush_all_shops(player, DO_IT_DEPENDS);
    try_setup_shop_timer(player);

    // 上报到switch
    online_report_player_onoff(player, (uint32_t)switchproto::PLAYER_LOGIN);
    player->is_login = true;

	EscortUtils::deal_with_escort_relate_when_login(player);

	//判断黄金勋章180天
	deal_with_year_vip_state_when_login(player);

	//判断小屋里的精灵寻宝是否已经找到宝箱
	//PlayerUtils::send_mail_notify_home_pets_found_item(player);

	//判断是否领取过封测战力前十名的奖励(4月8日的公测不开放)
	PlayerUtils::recv_system_mail(player, GET_TEST_PRIZE);
	//判断是否领取首创角色奖励
	PlayerUtils::recv_system_mail(player, GET_CREATE_ROLE_PRIZE);
	//领取玛音试练奖励
	PlayerUtils::recv_system_mail(player, GET_MAYIN_BUCKET_PRIZE);
	//领取玛音送花排行榜的奖励
	PlayerUtils::recv_system_mail(player, GET_MAYIN_FLOWER_PRIZE);
	//领取合服奖励
	PlayerUtils::recv_system_mail(player, GET_MERGE_SVR_PRIZE);

    // 更新天选者计划活动开始时间
    update_change_clothes_plan_start_time(player);

    // 坐骑翅膀升阶奖励,中间增加的规则,需要补偿老玩家
    EquipUtils::add_cult_equip_level_reward(player);

	if (PlayerUtils::test_vip_state(player)) {
		std::string title = "勋章已到期，高富帅来续费吧";
			std::string content("尊敬的高富帅，勋章特权已到期，快来续费吧! "
					"快去看看吧\n<u><a href='event:vip,2'><font color='#000000'>点此查看明细</font></u></a>");
		PlayerUtils::generate_new_mail(player, title, content);
	}

	//处理排位赛初始为0 的问题
	if (GET_A(kAttrArenaHistoryMaxRank) == 0) {
		SET_A(kAttrArenaHistoryMaxRank, 0x7FFFFFFF);
	}
	//判断是否更新充值排行榜
    if (g_srv_time_mgr.is_now_time_valid(TM_SUBKEY_DIAMOND_RECHARGE)) {
		uint32_t clear_time = GET_A(kAttrClearOpenServFlagTime);
		if(!g_srv_time_mgr.is_time_valid(TM_SUBKEY_DIAMOND_RECHARGE, clear_time)){
			//清除相关数据
			SET_A(kAttrOpenServRankingFlag,         0);
			SET_A(kAttrDiamondRechargeRankingCount, 0);
			SET_A(kAttrGoldConsumeRankingCount,     0);

			SET_A(kAttrClearOpenServFlagTime, NOW());
		}
		uint32_t count =  GET_A(kAttrDiamondRechargeRankingCount);
		if(0 != count){
			//活动时间内
		uint32_t start_day = g_srv_time_mgr.get_start_time(TM_SUBKEY_DIAMOND_RECHARGE);
			//更新充值排名
			RankUtils::rank_user_insert_score(
					player->userid, player->create_tm,
					commonproto::RANKING_TL_DIAMOND_RECHARGE, start_day,
					count);
		}
    }
	//更新开服战力排行榜
    if (g_srv_time_mgr.is_now_time_valid(TM_SUBKEY_POWER)) {
		uint32_t power = GET_A(kAttrBattleValueRecord);
		uint32_t start_day = g_srv_time_mgr.get_start_time(TM_SUBKEY_POWER);
		//更新战力排名
		RankUtils::rank_user_insert_score(
				player->userid, player->create_tm,
				(uint32_t)commonproto::RANKING_TL_TOTAL_POWER, 
				start_day, power);
	}

	//邮件发送充值排行奖励
	uint32_t time = 0;
	time = g_srv_time_mgr.get_send_time(TM_SUBKEY_DIAMOND_RECHARGE, TM_SUBKEY_DIAMOND_RECHARGE_REWARD);
	uint32_t reward_flag = GET_A(kAttrOpenServRankingFlag);
	if(time && !taomee::test_bit_on(reward_flag, 1)){
		player->diamond_recharge_reward_timer = ADD_TIMER_EVENT_EX(
				player,
				kTimerTypeSendDiamondRechargeReward,
				(void*)(player->userid),
				time);
	}

	//邮件发送开服战力排行奖励
	time = 0;
	time = g_srv_time_mgr.get_send_time(TM_SUBKEY_POWER, TM_SUBKEY_POWER_REWARD);
	reward_flag = GET_A(kAttrOpenServRankingFlag);
	if(time && !taomee::test_bit_on(reward_flag, 2)){
		player->open_srv_power_reward_timer = ADD_TIMER_EVENT_EX(
				player,
				kTimerTypeSendOpenSrvPowerReward,
				(void*)(player->userid),
				time);
	}

	//邮件发送开服金币消耗排行奖励
	time = 0;
	time = g_srv_time_mgr.get_send_time(TM_SUBKEY_GOLD_CONSUME, TM_SUBKEY_GOLD_CONSUME_REWARD);
	reward_flag = GET_A(kAttrOpenServRankingFlag);
	if(time && !taomee::test_bit_on(reward_flag, 3)){
		player->open_srv_gold_consume_reward_timer = ADD_TIMER_EVENT_EX(
				player,
				kTimerTypeSendGoldConsumeReward,
				(void*)(player->userid),
				time);
	}
	//周活动rank排行奖励
	time = 0;
	time = TimeUtils::get_activity_rank_reward_time(
			TM_CONF_KEY_KUROME_SEND_DESSERT, 1,
			TM_CONF_KEY_KUROME_SEND_DESSERT, 2);

	//4:须佐 , 5:红离
	reward_flag = GET_A(kAttrWeeklyActivityRankFlag);
	if(time && !taomee::test_bit_on(reward_flag, 5)){
		player->weekly_activity_rank_reward_timer = ADD_TIMER_EVENT_EX(
				player,
				kTimerTypeWeeklyActivityRankReward,
				(void*)(player->userid),
				time);
	}

	PlayerUtils::deal_equiped_title(player);

    //黄金vip极限放送活动
    //成就 前后端规定的成就ID段为任务的90000-100000
    uint32_t total_achieve = TaskUtils::total_fini_task(player, 90000, 100000);
    SET_A(kAttrGoldVipActivity150611Tasks, total_achieve);
    //伙伴
    SET_A(kAttrGoldVipActivity150611Pets, player->pets->size());
	//合服处理
	PlayerUtils::deal_after_merger_server(player);
	//加时装buf
	PlayerUtils::calc_player_suit_buff_info(player);

    return send_login_response(player);
}


int LoginCompleteCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    //TODO(singku)处理物品奖励那些东东
    return 0;
}

int LoginCompleteCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    return 0; 
}
