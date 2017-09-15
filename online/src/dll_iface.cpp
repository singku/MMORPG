#include <execinfo.h>
#include "common.h"
#include "proto.h"
#include "proto_processor.h"
#include "dll_iface.h"
#include "global_data.h"
#include "player.h"
#include "service.h"
#include "timer_procs.h"
#include "xml_configs.h"
#include "proto_queue.h"
#include "player_manager.h"
#include "switch_proto.h"
#include "login_processor.h"
#include "player_processor.h"
#include "duplicate_processor.h"
#include "map_processor.h"
#include "item_func_processor.h"
#include "item_processor.h"
#include "equip_processor.h"
#include "task_processor.h"
#include "pet_processor.h"
#include "mcast_utils.h"
#include "pet_conf.h"
#include "prize_conf.h"
#include "statlogger/statlogger.h"
#include "rune_processor.h"
#include "task_info.h"
#include "tran_card_processor.h"
#include "mail_processor.h"
#include "home_processor.h"
#include "shop_processor.h"
#include "prize_processor.h"
#include "friend_processor.h"
#include "arena_processor.h"
#include "chat_processor.h"
#include "swim_processor.h"
#include "escort_processor.h"
#include "family_processor.h"
#include "trans_prof_processor.h"
#include "exped_processor.h"
#include "buff_conf.h"
#include "buff.h"
#include "builder_conf.h"
#include "exped_conf.h"
#include "duplicate_conf.h"
#include "task_utils.h"
#include "achieve.h"
#include "question_processor.h"
#include "mine_processor.h"
#include "title.h"
#include "bless_pet_processor.h"
#include "arena_conf.h"

// #include "test_prob.h"

#ifdef TEST_CACHE
#include "test_cache.h"
#endif

#define CONFIG_READ_INTVAL(data, name) \
    do { \
        int ret = -1; \
        ret = config_get_intval(#name, ret); \
        if (ret == -1) { \
            ERROR_LOG("not find config '%s'", #name); \
            return -1; \
        } \
        data.name = ret; \
    } while (0);

#define CONFIG_READ_STRVAL(data, name) \
    do { \
        const char *conf_str = NULL; \
        conf_str = config_get_strval(#name); \
        if (conf_str == NULL) { \
            ERROR_LOG("not find config '%s'", #name); \
            return -1; \
        } \
        STRCPY_SAFE(data.name, conf_str); \
    } while (0);

static int init_processors();
static bool load_configs();
static int init_connections();
static int start_function_timers();
static void proc_cached_proto();
static int init_world_boss();

void init_assert()
{
    assert((int)kMaxBattleValueTypeNum == (int)kMaxEffortNum);
}

std::string stack_trace()
{
    // 打印堆栈信息
    void* buffs[100];
    int num_ptrs;
    num_ptrs = backtrace(buffs, array_elem_num(buffs));

    char** strings;
    strings = backtrace_symbols(buffs, num_ptrs);

    std::string stack;
    if (strings) {
        for (int i = 1; i < num_ptrs; i++) {
            stack += std::string(strings[i]) + "\n"; 
        }
        free(strings);
    }

    return stack;
}

void pb_log_handler(google::protobuf::LogLevel level,
        const char *filename, int line, const std::string &message)
{
    static const char *level_names[] = {"INFO", "WARNING", "ERROR", "FATAL" };

    std::string stack = stack_trace();

    ERROR_TLOG("[%s %s:%d] %s\n stack: '%s'",
            level_names[level], filename, line, message.c_str(),
            stack.c_str());
    DEBUG_TLOG("[%s %s:%d] %s\n stack: '%s'",
            level_names[level], filename, line, message.c_str(),
            stack.c_str());
}

extern "C" int  init_service(int isparent)
{
    g_proto_processor = new ProtoProcessor();

	if (!isparent) {
#ifdef ENABLE_TRACE_LOG
#ifdef USE_TLOG
        SET_LOG_LEVEL((tlog_lvl_t)/*tlog_lvl_trace*/config_get_intval("log_level", 6));
        SET_TIME_SLICE_SECS(86400);
#endif
#endif       
        srand(NOW());
        srandom(NOW());

        SetLogHandler(pb_log_handler);

        g_player_manager = new PlayerManager();
        g_pending_proto_players.clear();
        
        INIT_LIST_HEAD(&g_reconnect_timer.timer_list);
        INIT_LIST_HEAD(&g_sys_noti_timer.timer_list);
        INIT_LIST_HEAD(&g_switch_reg_timer.timer_list);
        INIT_LIST_HEAD(&g_escort_timer.timer_list);
        INIT_LIST_HEAD(&g_world_boss_timer.timer_list);
        INIT_LIST_HEAD(&g_kick_off_timer.timer_list);
        INIT_LIST_HEAD(&g_reset_dive_timer.timer_list);
        INIT_LIST_HEAD(&g_dump_rank_timer.timer_list);

        init_assert();
        setup_timer();
        init_processors();
        register_timers();
        init_world_boss();
		//注册购买属性清零后的逻辑处理函数
		g_shop_reg_fun.register_buy_product_proc_func();
		g_achieve_reg_func.register_listen_achieve_func();
        memset(&g_server_config, 0, sizeof(g_server_config));

        CONFIG_READ_STRVAL(g_server_config, dbproxy_name);
        CONFIG_READ_STRVAL(g_server_config, switch_name);
        CONFIG_READ_STRVAL(g_server_config, battle_name);
        CONFIG_READ_STRVAL(g_server_config, battle_center_name);

        CONFIG_READ_INTVAL(g_server_config, verifyid);
        CONFIG_READ_STRVAL(g_server_config, security_code);
        CONFIG_READ_INTVAL(g_server_config, idc_zone);
        CONFIG_READ_INTVAL(g_server_config, gameid);
        CONFIG_READ_STRVAL(g_server_config, conf_path);
        CONFIG_READ_STRVAL(g_server_config, statistic_file);
        g_server_config.use_gm = config_get_intval("use_gm", 0);
        g_server_config.shut_addiction = config_get_intval("shut_addiction", 1);
        g_server_config.version_department = config_get_intval("version_department", 0);
        g_server_id = config_get_intval("server_id", 1);
        g_online_id = get_server_id();//async_server的调用

        if (g_svr_close_before_zero >= 86400) {
            g_svr_close_before_zero = 0;
        }
        if (g_svr_open_after_zero >= 86400) {
            g_svr_open_after_zero = 0;
        }
        // 初始化网络连接
        if (init_connections() != 0) {
            KERROR_LOG(0, "init server connections failed"); 
            return -1;
        }
        if (!load_configs()) {
            return -1; 
        }
        // 启动业务逻辑定时器 
        if (start_function_timers() != 0) {
            KERROR_LOG(0, "start function timers failed"); 
            return -1;
        }

        char str[64] = "操你大爷 fuck";
        DEBUG_TLOG("DIRTY bef [%s]", str);
        int ret = tm_dirty_check(0, str);
        DEBUG_TLOG("DIRTY check ret=%d [%s]", ret, str);
        ret = tm_dirty_replace(str);
        DEBUG_TLOG("DIRTY replace ret=%d [%s]", ret, str);
        char str2[64] = "见到你很高兴";
        DEBUG_TLOG("DIRTY bef [%s]", str2);
        ret = tm_dirty_check(0, str2);
        DEBUG_TLOG("DIRTY check ret=%d [%s]", ret, str2);
        ret = tm_dirty_replace(str2);
        DEBUG_TLOG("DIRTY replace ret=%d [%s]", ret, str2);

        char str3[64] = "123456789";
        DEBUG_TLOG("DIRTY bef [%s]", str3);
        ret = tm_dirty_check(0, str3);
        DEBUG_TLOG("DIRTY check ret=%d [%s]", ret, str3);
        ret = tm_dirty_replace(str3);
        DEBUG_TLOG("DIRTY replace ret=%d [%s]", ret, str3);

        g_stat_logger = new StatLogger(g_server_config.gameid, -1, g_server_id);
		// shot1();
		// shot2();
        //test_prob();
		// test_one_time_normal_lottery_rand();
		// test_ten_times_normal_rand();
		// test_one_time_diamond_lottery_rand();
		// TODO
		// test_ten_times_diamond_rand();
		// open_box_test_rand();
#if 0
        for (uint32_t i = 0; i < 100000; i++) {
            RandomNickCmdProcessor::get_random_nick(0);
            RandomNickCmdProcessor::get_random_nick(1);
        }
#endif
	}

	return 0;
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" int  fini_service(int isparent)
{
    delete g_proto_processor;

	if (!isparent) {
	}

	return 0;
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void proc_events()
{
    handle_timer();
    proc_cached_proto();
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" int get_pkg_len(int fd, const void* avail_data, int avail_len, int isparent)
{
    if (isparent) {
        return g_proto_processor->get_pkg_len(fd, avail_data, avail_len, PROTO_FROM_CLIENT); 
    } else {
        return g_proto_processor->get_pkg_len(fd, avail_data, avail_len, PROTO_FROM_SERV); 
    }
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" int  proc_pkg_from_client(void* data, int len, fdsession_t* fdsess)
{
    g_proto_processor->proc_pkg_from_client(data, len, fdsess, false);

    return 0;
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void proc_pkg_from_serv(int fd, void* data, int len)
{
    g_proto_processor->proc_pkg_from_serv(fd, data, len);
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void on_client_conn_closed(int fd)
{
    player_t* player = g_player_manager->get_player_by_fd(fd);
    if (player) {
        player_leave_server(player);
        g_player_manager->del_player_from_manager(player);
        g_player_manager->destroy_player(player); 
    }
}

extern "C" int before_reload(int isparent)
{
    google::protobuf::ShutdownProtobufLibrary();
    delete g_proto_processor;
    g_proto_processor = 0;
    unregister_timers_callback();

    return 0;
}

extern "C" int reload_global_data()
{
    unregister_timers_callback();
    g_proto_processor = new ProtoProcessor();
    init_processors();
    register_timers();
    SetLogHandler(pb_log_handler);
    init_task_cond_fun();

    refresh_timers_callback();
    AttrUtils::register_stat_func();
    g_shop_reg_fun.register_buy_product_proc_func();
	g_achieve_reg_func.register_listen_achieve_func();

    return 0;
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void proc_mcast_pkg(const void* data, int len)
{
    const comm_proto_header_t* header = static_cast<const comm_proto_header_t *>(data);
    const char* body = static_cast<const char *>(data) + sizeof(comm_proto_header_t);
    int bodylen = header->len - sizeof(comm_proto_header_t);

    if (header->cmd == 0) {
        svrcommproto::mcast_reload_conf mcast_reload_conf;
        if (parse_message(body, bodylen, &mcast_reload_conf) != 0) {
            ERROR_TLOG("decode mcast_proto  failed");
            return ;
        }
        std::string conf_name = mcast_reload_conf.conf_name();
        uint32_t server_id = mcast_reload_conf.serverid();
        if (server_id != 0 && server_id != g_online_id) {
            return;
        }

        INFO_TLOG("reload conf %s", conf_name.c_str());
        INFO_TLOG("reload server_id %d", server_id);

        if (conf_name.compare("all") == 0) {
            bool flag = load_configs();
            if (!flag) {
                ERROR_TLOG("reload conf %s failed", conf_name.c_str());
            } else {
                INFO_TLOG("reload conf %s success!", conf_name.c_str());
            }

            // 重载配表时，重置在线玩家任务数据
            std::vector<player_t*> player_list;
            g_player_manager->get_player_list(player_list);
            FOREACH(player_list, iter) {
                TaskUtils::task_update_to_config(*iter);
            }
            return;
        }

        int ret = McastUtils::reload_configs(conf_name);
        if (ret != 0) {
            ERROR_TLOG("reload conf %s failed", conf_name.c_str());
        } else {
            INFO_TLOG("reload conf %s success!", conf_name.c_str());
        }

    } else if (header->cmd == 1) {

        svrcommproto::mcast_notify mcast_notify; 

        if (parse_message(body, bodylen, &mcast_notify) != 0) {
            KERROR_LOG(0, "decode mcast_notify failed"); 
            return ;
        }

        McastUtils::notify(mcast_notify.msg());
    }
}

/**
* @brief AsyncServer框架要求实现的接口之一。 
*
*/
extern "C" void on_fd_closed(int fd)
{
    if (fd == g_dbproxy->fd()) {
        DEBUG_TLOG("dbproxy closed");
        g_dbproxy->close(); 

        ADD_TIMER_EVENT_EX(&g_reconnect_timer,
                kTimerTypeReconnectServiceTimely,
                g_dbproxy,
                get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely);  

        asynsvr_send_warning_msg("ConnErr", g_online_id, 0, 0, g_dbproxy->service_name().c_str());
    } else if (fd == g_switch->fd()) {
        DEBUG_TLOG("switch closed"); 
        g_switch->close();
        g_svr_loginable = false;
        ADD_TIMER_EVENT_EX(&g_reconnect_timer,
                kTimerTypeReconnectServiceTimely,
                g_switch,
                get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely);  

        asynsvr_send_warning_msg("ConnErr", g_online_id, 0, 0, g_switch->service_name().c_str());

    } else if (fd == g_battle->fd()) {
        DEBUG_TLOG("battle closed"); 
        g_battle->close();
        g_player_manager->on_battle_close();

        ADD_TIMER_EVENT_EX(&g_reconnect_timer,
                kTimerTypeReconnectServiceTimely,
                g_battle,
                get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely);  
        asynsvr_send_warning_msg("ConnErr", g_online_id, 0, 0, g_battle->service_name().c_str());
#if 1
    } else if (fd == g_battle_center->fd()) {
        DEBUG_TLOG("battle center closed");
        g_battle_center->close();
        g_player_manager->on_battle_center_close();

        ADD_TIMER_EVENT_EX(&g_reconnect_timer,
                kTimerTypeReconnectServiceTimely,
                g_battle_center,
                get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely);  
        asynsvr_send_warning_msg("ConnErr", g_online_id, 0, 0, g_battle_center->service_name().c_str());
#endif
    }
}

// 处理客户端缓存的协议
void proc_cached_proto()
{
    std::map<userid_t, player_t*>::iterator ptr = 
        g_pending_proto_players.begin();

    for (; ptr != g_pending_proto_players.end(); ) {
        player_t* player = ptr->second;
        ptr++;
        player_t* expect_player = g_player_manager->get_player_by_userid(player->userid);
        if (expect_player == NULL) {
            WARN_TLOG("cached proto not find player %u", player->userid); 
            continue;
        }

        if (expect_player->userid != player->userid) {
            WARN_TLOG("cached proto not find player %u", player->userid); 
            continue;
        }

        ProtoQueue* proto_queue = player->proto_queue;

        if (proto_queue->empty()) {
            g_pending_proto_players.erase(player->userid);
            continue;
        }

        if (player->wait_svr_cmd) {
            continue ; 
        }

        ProtoQueue::proto_t proto = {0};
        int ret = proto_queue->pop_proto(proto);

        if (ret != 0) {
            KERROR_LOG(player->userid, "pop proto from queue failed");
            continue; 
        }

        g_proto_processor->proc_pkg_from_client(
                proto.data, proto.len, proto.fdsession, true);

        free(proto.data);

        if (proto_queue->empty()) {
            g_pending_proto_players.erase(player->userid); 
        }
    }
}

int init_processors()
{
    g_proto_processor->register_command(sw_cmd_register_server, new SwitchRegCmdProcessor());
    //g_proto_processor->register_command(sw_cmd_get_server_list, new SwitchGetSvrListCmdProcessor());
    g_proto_processor->register_command(sw_cmd_sw_transmit_only, new SwitchTransmitCmdProcessor());

    g_proto_processor->register_command(sw_cmd_sw_notify_kick_player_off, new SwitchNotifyKickPlayerCmdProcessor());
    g_proto_processor->register_command(sw_cmd_sw_notify_erase_player_escort_info, new SwitchNotifyEraseEscortInfoCmdProcessor());
    g_proto_processor->register_command(sw_cmd_sw_notify_attr_changed_by_other, new SwitchNotifyAttrChangedCmdProcessor());
    g_proto_processor->register_command(sw_cmd_sw_notify_new_mail, new SwitchNotifyNewMailCmdProcessor());
    g_proto_processor->register_command(sw_cmd_sw_notify_player_frozen_account, new SwNtfFrozenAccountCmdProcessor());
    g_proto_processor->register_command(sw_cmd_sw_notify_new_mail_to_svr, new SwNotifyNewMailToSvrCmdProcessor());
    g_proto_processor->register_command(sw_cmd_sw_only_notify_player_attr_changed, new SwOnlyNotifyAttrChangedCmdProcessor());

    g_proto_processor->register_command(cli_cmd_cs_0x000A_cli_get_session, new GetSessionCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0114_require_server_time, new RequireServerTimeCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0115_rtt_test, new RTTTestCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0116_heart_beat, new HeartBeatCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0148_get_online_list, new GetOnlineListCmdProcessor());

    g_proto_processor->register_command(cli_cmd_cs_0x012B_cli_set_attr, new CliSetAttrCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0135_cli_get_attr_list, new CliGetAttrListCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x013A_get_other_player_info, new GetOtherPlayerDataCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x013D_get_others_head, new GetOtherPlayerHeadCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0142_set_nick, new SetNickCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0002_require_random_nick, new RandomNickCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x022C_buy_duplicate_cnt, new DupBuyDailyCntCmdProcessor());

    g_proto_processor->register_command(cli_cmd_cs_0x0101_enter_svr, new LoginCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0102_enter_map, new EnterMapCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0104_leave_map, new LeaveMapCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0106_player_change_state, new PlayerChangeStateCmdProcessor());

    ItemUseCmdProcessor* use_item_processor = new ItemUseCmdProcessor();
    use_item_processor->init();
    g_proto_processor->register_command(cli_cmd_cs_0x010C_use_item, use_item_processor);
    g_proto_processor->register_command(cli_cmd_cs_0x010D_one_key_equip, new PlayerOneKeyEquipArmItemFuncProcessor());

    g_proto_processor->register_command(cli_cmd_cs_0x0110_login_complete, new LoginCompleteCmdProcessor());

	//装备相关
	g_proto_processor->register_command(cli_cmd_cs_0x0117_equip_level_up, new EquipLevelUpCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x013F_hide_fashion, new HideFashionCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0146_cultivate_equip, new CultivateEquipCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0147_equip_quench, new EquipQuenchCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0149_set_equip_show_status, new SetEquipShowStatusCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x014C_get_mount_battle_value, new GetMountBattleValueCmdProcessor());
	//物品熔炼
	g_proto_processor->register_command(cli_cmd_cs_0x016C_item_smelter, new ItemSmelterCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x016D_item_reinit_equip_attr, new ItemReInitEquipAttrCmdProcessor());

	//进阶转职
	g_proto_processor->register_command(cli_cmd_cs_0x0139_trans_prof, new TransProfCmdProcessor());

	//任务相关
	g_proto_processor->register_command(cli_cmd_cs_0x0401_accept_task, new AcceptTaskCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0402_abandon_task, new AbandonTaskCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0403_complete_task, new TaskCompleteCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0405_complete_evil_knife_legend, new CompleteEvilKnifeLegendCmdProcessor());
		

    //副本相关
    g_proto_processor->register_command(cli_cmd_cs_0x0201_duplicate_enter_map, new EnterDuplicateCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0203_duplicate_leave_map, new LeaveDuplicateCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0205_duplicate_battle_ready, new ReadyDuplicateCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0208_duplicate_exit, new ExitDuplicateCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x020A_duplicate_hit_character, new HitDuplicateObjCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x020F_duplicate_stat_info, new StatDuplicateCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0211_duplicate_to_next_phase, new ToDuplicateNextPhaseCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0212_duplicate_revival, new RevivalDuplicateCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0215_duplicate_pick_up_dead_pet_prize, new DuplicatePickUpCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0256_buy_dup_clean_cd, new BuyDupCleanCDCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0257_buy_pass_dup, new BuyPassDupCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0258_pet_buy_pass_dup, new PetBuyPassDupCmdProcessor());

    g_proto_processor->register_command(btl_cmd_notify_kill_character, new DuplicateBtlNotifyKillCharacterCmdProcessor());
    g_proto_processor->register_command(btl_cmd_notify_end, new DuplicateBtlNotifyEndCmdProcessor());
    g_proto_processor->register_command(btl_cmd_notify_msg_relay, new DuplicateBtlNotifyRelayCmdProcessor());
    g_proto_processor->register_command(btl_cmd_notify_match_result, new DuplicateMatchResultCmdProcessor());

    g_proto_processor->register_command(cli_cmd_cs_0x0226_one_key_pass_duplicate, new OneKeyPassDuplicateCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0227_get_rank_info, new GetRankInfoCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0225_view_battle_report, new ViewBtlReportCmdProcessor());
    //g_proto_processor->register_command(cli_cmd_cs_0x0228_buy_arena_challenge_times, new BuyChallengeTimesCmdProcessor());
    //g_proto_processor->register_command(cli_cmd_cs_0x0229_duplicate_switch_fight_pet, new DupSwitchFightPetCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x022B_duplicate_mon_flush_request, new DupFrontMonFlushReqCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x022D_skill_affect, new SkillEffectCmdProcessor());

    //g_proto_processor->register_command(cli_cmd_cs_0x0235_match_opponent, new RPVPMatchCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0248_pvep_match_opponent, new PVEPMatchCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0249_pvep_revival, new PVEPRevivalCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0250_pvep_reset, new PVEPResetCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0251_night_raid_prize_total, new PVEPPrizeTotalCmdProcessor());

    g_proto_processor->register_command(cli_cmd_cs_0x0238_drop_match_opponent, new GiveUpMatchCmdProcessor());
    //g_proto_processor->register_command(btl_cmd_notify_rpvp_match_result, new RPVPMatchNotifyResultCmdProcessor());
    g_proto_processor->register_command(btl_cmd_notify_battle_down, new BattleNotifyDownCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x023F_get_world_boss_dup_info, new GetWorldBossDupInfoCmdProcessor());

    //精灵相关
    g_proto_processor->register_command(cli_cmd_cs_0x0307_change_pet_location, new PetSwitchLocCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0309_set_fight_pet_status, new PetSetFightCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x030C_conditional_pet_evolution, new ConditionalPetEvolutionCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x030D_conditional_pet_level_up, new ConditionalPetLevelUpCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x030E_conditional_pet_add_exp, new ConditionalPetAddExpCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x030F_conditional_pet_set_talent, new ConditionalPetSetTalentCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0310_conditional_pet_add_effort, new ConditionalPetAddEffortCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x031A_get_pets, new GetPetCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0323_chisel_on, new PetChiselCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0328_call_pet_born, new PetCallBornCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x032A_update_skill, new UpdateSkillCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x032B_pet_awaken, new ConditionalPetAwakenCmdProcessor());


    //奖励相关
    g_proto_processor->register_command(cli_cmd_cs_0x0113_require_prize, new RequirePrizeCmdProcessor());
    //g_proto_processor->register_command(cli_cmd_cs_0x014B_exchange_gift_code, new UseGiftCodeCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x014B_exchange_gift_code, new UseMagicWordCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x014D_gen_invite_code, new GenInviteCodeCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x014E_sign_invite_code, new SignInviteCodeCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x014F_resource_retrieve, new ResourceRetrieveCmdProcessor());


    //商城相关
    g_proto_processor->register_command(cli_cmd_cs_0x012E_buy_product, new BuyProductCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x012F_sell_items, new SellProductCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0131_shop_refresh, new ShopRefreshCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0136_exchange, new ExchangeCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0137_buy_vp, new BuyVpCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x013E_alchemy, new AlchemyCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0140_front_stat_log, new FrontStatCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0144_shop_get_product_info, new ShopGetProductInfoCmdProcessor());


	//符文相关
    g_proto_processor->register_command(cli_cmd_cs_0x0320_rune_call, new RuneCallCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0321_get_rune_from_runepack_to_tranpack, new GetRuneFromRpToTpCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0322_sell_gray_rune, new SellGrayRuneCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x031B_uprune_by_exp, new UpRuneByExpCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x031C_swallow_rune, new SwallowRuneCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x031D_equip_rune, new EquipRuneCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x031E_chgrune_pack, new ChgRunePackCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x031F_rune_to_bottleexp, new RunetoExpBottleCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0324_rune_one_key_to_bottleexp, new OneKeyToBottleCmdProcessor());

	//解锁
    g_proto_processor->register_command(cli_cmd_cs_0x0327_unlock_pet_rune_pos, new UnlockPetRunePosCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0329_unlock_rune_collect_bag, new UnlockRuneCollectBagCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0337_buy_rune_trans_pack_page, new BuyRuneTransPackPageCmdProcessor());
	//卡牌相关
    g_proto_processor->register_command(cli_cmd_cs_0x0119_update_star_level, new UptarLvCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0120_choose_role, new ChooseRoleCmdProcessor());

    //充钻抽卡
    g_proto_processor->register_command(cli_cmd_cs_0x015A_get_charge_diamond_draw_cards, new GetChargeDiamondDrawCardsCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x015B_refresh_charge_diamond_draw_cards, new RefreshChargeDiamondDrawCardsCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x015C_draw_charge_diamond_card, new DrawChargeDiamondCardCmdProcessor());
	//prize榜单
    g_proto_processor->register_command(cli_cmd_cs_0x0160_inform_prize_bulletin, new GetPrizeBulletinCmdProcessor());


	// mail related
	
    g_proto_processor->register_command(cli_cmd_cs_0x0501_get_mail_list, new GetMailListCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0502_del_mail_list, new DelMailListCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0503_get_mail_attachment, new GetMailAttachmentCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0504_read_mail, new ReadMailCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0506_add_mail, new AddMailCmdProcessor());
	//小屋相关
	g_proto_processor->register_command(cli_cmd_cs_0x0122_access_home, new AccessHomeCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0133_add_visit_log, new HmAddVisitLogCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0132_home_get_visit_log_list, new HmGetVisitLogCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x013B_hm_ask_for_gift, new HmAskForGiftCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x032D_pet_op_begin, new PetOpBeginCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x032F_pet_op_end, new PetRecallCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0331_open_exercise_box, new OpenExerciseBoxCmdProcessor());
	//g_proto_processor->register_command(cli_cmd_cs_0x032D_pet_exercise, new PetExerciseCmdProcessor());
	//g_proto_processor->register_command(cli_cmd_cs_0x032F_pet_recall, new PetRecallCmdProcessor());
	//g_proto_processor->register_command(cli_cmd_cs_0x0330_open_exercise_pos, new OpenExercisePosCmdProcessor());
	//g_proto_processor->register_command(cli_cmd_cs_0x0331_open_exercise_box, new OpenExerciseBoxCmdProcessor());

	
	//好友系统
	g_proto_processor->register_command(cli_cmd_cs_0x0601_refresh_friend_data, new RefreshFriendDataCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0603_add_friend, new AddFriendCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0604_send_personal_msg, new SendPersonalMsgCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0607_add_blacklist, new AddBlacklistCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0608_remove_friend, new RemoveFriendCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0609_search_friend, new SearchFriendCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x060F_recommendation, new RecommendationCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0610_get_friend_info, new GetFriendInfoCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0612_get_question_info, new GetQuestionInfoCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0613_submit_user_answer, new SubmitUserAnswerCmdProcessor());

	//竞技场相关
	g_proto_processor->register_command(cli_cmd_cs_0x0218_challenge_arena_player, new ChallengeArenaPlayerCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0217_get_arena_data, new GetArenaPlayerDataCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0219_get_ranking, new GetRankListCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0224_arena_challenge_result, new ArenaResultCmdProcessor());

	//聊天相关
	g_proto_processor->register_command(cli_cmd_cs_0x060A_say, new SayCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x060B_show_item, new ShowItemCmdProcessor());
	//g_proto_processor->register_command(cli_cmd_cs_0x060C_emotion, new EmotionCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x060D_show_pet, new ShowPetCmdProcessor());

	g_proto_processor->register_command(cli_cmd_cs_0x060E_broadcast, new BroadcastCmdProcessor());
	//运宝相关
	g_proto_processor->register_command(cli_cmd_cs_0x0801_escort_refresh_airship, new EscortRefreshShipCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x081A_buy_ship, new EscortBuyShipCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0802_escort_get_other_airship, new EscortGetOtherShipCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0803_escort_start, new EscortStartCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0805_escort_robbery, new EscortRobberyCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0808_escort_get_reward, new EscortGetRewardCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0810_robbery_result, new EscortRobberyResultCmdProcessor());

	g_proto_processor->register_command(cli_cmd_cs_0x0821_start_swim, new StartSwimCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0822_get_exp, new SwimGetExpCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0823_swim_pause, new SwimPauseCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0827_use_chair, new UseChairCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0828_withdraw_chair, new WithdrawChairCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x082A_apply_dive, new ApplyDiveCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x082B_prepare_dive, new PrepareDiveCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x082C_start_dive, new StartDiveCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x082D_inform_dive_rank, new InformDiveRankCmdProcessor());
    // 家族相关
	g_proto_processor->register_command(cli_cmd_cs_0x0701_family_create, new FamilyCreateCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0702_family_dismiss, new FamilyDismissCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0703_family_apply, new FamilyApplyCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0704_family_quit, new FamilyQuitCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0705_family_leader_reassign, new FamilyLeaderReassignCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0706_family_set_member_title, new FamilySetMemberTitleCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0707_family_get_info, new FamilyGetInfoCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0708_family_get_member_info, new FamilyGetMemberInfoCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0709_family_contribute, new FamilyConstructCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0710_family_get_member_list, new FamilyGetMemberListCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0711_family_update_msg_info, new FamilyUpdateMsgInfoCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0712_family_get_apply_list, new FamilyGetApplyListCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0713_family_deal_invite, new FamilyDealInviteCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0714_require_random_name, new RequireRandomNameCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0715_family_invite, new FamilyInviteCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0716_family_get_event_list, new FamilyGetEventListCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0717_family_event_deal, new FamilyEventDealCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x071E_family_get_log_list, new FamilyGetLogListCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x071F_family_get_recommend_list, new FamilyGetRecommendListCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0721_family_msg_del, new FamilyMsgDelCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0722_family_config, new FamilyConfigCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0723_family_leader_reassign_request, new FamilyLeaderReassignRequestCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0724_family_cancel_apply, new FamilyCancelApplyCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0725_family_tech_up, new FamilyTechUpCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0726_change_family_name, new FamilyNameChangeCmdProcessor());

	//排行榜点赞
	g_proto_processor->register_command(cli_cmd_cs_0x0231_rank_praise_function, new RankPraiseFunCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0232_praise_count_exchange, new PraiseCountExgCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0143_hm_gift_exchange, new HmGiftExchgCmdProcessor());

	//模拟客户端协议，用于周五零点之后，去排名服获取玩家周排名之用
	g_proto_processor->register_command(cli_cmd_cs_0x0233_fake_cmd_for_arena_weeklyrank, new FakeCmdForArenaDailyRankCmdProcessor());

	g_proto_processor->register_command(cli_cmd_cs_0x0254_fake_cmd_for_open_serv_rank , new FakeCmdForOpenSrvRankCmdProcessor());

	g_proto_processor->register_command(cli_cmd_cs_0x0234_reset_dup, new ResetDupCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0255_fake_cmd_for_weekly_activity_rank, new FakeCmdForWeeklyActivityRankCmdProcessor());

	g_proto_processor->register_command(cli_cmd_cs_0x0234_reset_dup, new ResetDupCmdProcessor());
	//远征
	g_proto_processor->register_command(cli_cmd_cs_0x0240_expedition_into_scene, new ExpedInfoSceneCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0243_expedition_start, new ExpedStartCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0244_expedition_result, new ExpedResultCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0247_expedition_choose_pet, new ExpedPickJoinedPetCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0242_expedition_reset, new ExpedResetCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0245_expedition_prize_total, new ExpedPrizeTotalProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0614_mayin_send_flower, new MayinSendFlowerCmdProcessor());

	g_proto_processor->register_command(cli_cmd_cs_0x0615_get_titles, new GetTitleCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0617_equip_title, new EquipTitleCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0619_unload_title, new UnloadTitleCmdProcessor());

	g_proto_processor->register_command(cli_cmd_cs_0x061A_require_daily_question_reward, new RequireDailyQuestionRewardCmdProcessor());
	//伙伴祈福
	g_proto_processor->register_command(cli_cmd_cs_0x061B_create_bless_team, new CreateBlessTeamCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x061C_require_bless_team, new RequireBlessTeamCmdProcessor ());
	g_proto_processor->register_command(cli_cmd_cs_0x061D_dismiss_bless_team , new DismissBlessTeamCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x061E_get_bless_team_info, new GetBlessTeamInfoCmdProcessor());
	// g_proto_processor->register_command(cli_cmd_cs_0x061F_get_bless_team_list, new GetBlessTeamListCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0621_syn_bless_team_member_btl_info, new CacheBlessTeamMemberInfoCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0622_weekly_ranking_activity, new WeeklyRankingActivityCmdProcessor());

	//明星招募
	g_proto_processor->register_command(cli_cmd_cs_0x0625_star_lottery, new StarLotteryCmdProcessor());
	g_proto_processor->register_command(cli_cmd_cs_0x0626_get_star_lottery_info, new GetStarLotteryInfoCmdProcessor());
    //答题相关
    g_proto_processor->register_command(cli_cmd_cs_0x016A_get_survey_answer, new SurveyGetCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x016B_submit_survey_answer, new SurveySubmitCmdProcessor());
	//挖矿相关
    g_proto_processor->register_command(cli_cmd_cs_0x082E_search_mine, new SearchMineCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0835_exploit_new_mine, new ExploitNewMineCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0834_get_my_mine_info, new GetMyMineInfoCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0836_give_up_this_mine, new GiveUpMineCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0833_accept_join_defend_mine, new AcceptDefendMineCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x082F_start_occupy_mine, new StartOccupyMineCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0830_occupy_mine_result, new OccupyMineRetCmdProcessor());

    g_proto_processor->register_command(cli_cmd_cs_0x0838_get_mine_fight_btl_report, new GetMineBtlReportCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0623_send_hyperlink_msg, new SendHyperLinkMsgCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0839_get_mine_info_by_mine_id, new GetMineInfoByMineIdCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0837_refresh_mine, new AnewSearchMineCmdProcessor());

    g_proto_processor->register_command(cli_cmd_cs_0x083A_mine_send_hyperlink_msg, new MineSendkMsgCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x083C_check_my_mine_expire, new CheckMyMineExpireCmdProcessor());
	//帝具相关
    g_proto_processor->register_command(cli_cmd_cs_0x0335_diju_light_lamp, new DijuLightLampCmdProcessor());
    //g_proto_processor->register_command(cli_cmd_cs_0x0336_diju_up_stage, new DijuUpStageCmdProcessor());
    g_proto_processor->register_command(cli_cmd_cs_0x0259_pet_diju_awake, new PetDijuAwakeCmdProcessor());

    g_buff_handler.init();
#ifdef TEST_CACHE
    g_proto_processor->register_command(cache_cmd_ol_req_users_info, new GetCacheInfoCmdProcessor());
#endif
    return 0;
}

int init_world_boss()
{
    g_world_boss_mgr.init(commonproto::WORLD_BOSS_DUP_ID_1);
    return 0;
}

const char *gen_full_path(const char *base_path, const char *file_name)
{
    static char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, file_name);
    return full_path;
}

bool load_configs()
{
    //NOTI(singku) 注意配置表的加载顺序
    //奖励、商品表 要在物品、精灵、属性表加载之后加载


    bool succ = (1
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "attribute.xml"), load_attr_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "time_config.xml"), load_time_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "buff.xml"), load_client_buff_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "server_buff.xml"), load_server_buff_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "equip_buff.xml"), load_equip_buff_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "item.xml"), load_item_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "skill_parent.xml"), load_skill_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "pet.xml"), load_pet_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "rune.xml"), load_rune_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "rune_exp.xml"), load_rune_exp_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "rune_rate.xml"), load_rune_rate_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "map.xml"), load_map_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "duplicate.xml"), load_duplicate_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "title_info.xml"), load_title_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "prize.xml"), load_prize_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "player.xml"), load_player_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "tasks.xml"), load_task_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "reward_task.xml"), load_reward_task_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "sys_ctrl.xml"), load_sys_ctrl_config) == 0
			//&& load_xmlconf(gen_full_path(g_server_config.conf_path, "transform_card.xml"), load_tran_card_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "shop.xml"), load_shop_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "market.xml"), load_market_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "builder.xml"), load_builder_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "exchanges.xml"), load_exchange_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "arena_str_reward.xml"), load_arena_streak_reward_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "arenas.xml"), load_arena_rank_reward_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "trans_profession.xml"), load_trans_profession_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "home_gift.xml"), load_hm_gift_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "name_pool.xml"), load_name_pool_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "achieve.xml"), load_achieve_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "nick.xml"), load_nick_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "pet_quality.xml"), load_pet_quality_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "family_templates.xml"), load_family_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "global_attr.xml"), load_global_attr_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "pet_group.xml"), load_pet_group_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "expedition.xml"), load_exped_info) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "duplicate_area_prize.xml"), load_dup_area_prize_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "cultivate_equip.xml"), load_cultivate_equip_config) == 0
            && load_xmlconf(gen_full_path(g_server_config.conf_path, "topest_power_users.xml"), load_topest_power_player_info_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "test1_joined_user.xml"), load_first_test_uid_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "test2_joined_user.xml"), load_second_test_uid_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "question.xml"), load_question_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "bless_pet.xml"), load_bless_pet_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "rpvp_reward.xml"), load_rpvp_reward_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "pet_pass_dup.xml"), load_pet_pass_dup_config) == 0
			&& load_xmlconf(gen_full_path(g_server_config.conf_path, "suit.xml"), load_suit_config) == 0
            );
            
    if (succ == false) {
        return succ;
    }
	/*
	if (g_test_for_robot) {
		load_xmlconf(gen_full_path(g_server_config.conf_path, "test_topest_power.xml"), load_topest_power_player_info_config);
		load_xmlconf(gen_full_path(g_server_config.conf_path, "test1_joined_user.xml"), load_first_test_uid_config);
		load_xmlconf(gen_full_path(g_server_config.conf_path, "test2_joined_user.xml"), load_second_test_uid_config);
	}
	*/

    //校验精灵中的奖励是否正确
    FOREACH((g_pet_conf_mgr.const_pet_conf_map()), it) {
        const pet_conf_t &pet_conf = it->second;
        FOREACH(pet_conf.prize_id_list, it2) {
            uint32_t prize_id = *it2;
            if (!g_prize_conf_mgr.is_prize_exist(prize_id)) {
                ERROR_TLOG("Prize %u for pet %u not eixst", prize_id, pet_conf.id);
                return false;
            }
        }
        FOREACH(pet_conf.task_prize_list, it2) {
            uint32_t task_id = it2->first;
            if (! g_task_conf_mgr.is_task_conf_exist(task_id)) {
                ERROR_TLOG("Task %u for pet %u not exist", task_id, pet_conf.id);
                return false;
            }
            const std::vector<uint32_t> &prize_vec = it2->second;
            FOREACH(prize_vec, it3) {
                uint32_t prize_id = *it3;
                if (!g_prize_conf_mgr.is_prize_exist(prize_id)) {
                    ERROR_TLOG("Task %u prize %u for pet %u not eixst", prize_id, pet_conf.id);
                    return false;
                }
            }
        }
        if (pet_conf.hit_prize_id && !g_prize_conf_mgr.is_prize_exist(pet_conf.hit_prize_id)) {
            ERROR_TLOG("Prize %u for pet %u hit_prize not exist", pet_conf.hit_prize_id, pet_conf.id);
            return false;
        }
    }
    //检验builder中的奖励是否正确
    FOREACH((g_builder_conf_mgr.const_builder_conf_map()), it) {
        const builder_conf_t &builder_conf = it->second;
        FOREACH(builder_conf.prize_id_list, it2) {
            uint32_t prize_id = *it2;
            if (!g_prize_conf_mgr.is_prize_exist(prize_id)) {
                ERROR_TLOG("Prize %u for builder %u not eixst", prize_id, builder_conf.id);
                return false;
            }
        }
        if (builder_conf.hit_prize_id && g_prize_conf_mgr.is_prize_exist(builder_conf.hit_prize_id)) {
            ERROR_TLOG("Prize %u for builder %u hit_prize not exist", builder_conf.hit_prize_id, builder_conf.id);
            return false;
        }
    }

    //校验副本中的奖励是否合法
    FOREACH((g_duplicate_conf_mgr.const_dup_map()), it) {
        const duplicate_t &dup = it->second;
        FOREACH((dup.prize_vec), it2) {
            if (!g_prize_conf_mgr.is_prize_exist(*it2)) {
                ERROR_TLOG("Duplicate :%u prize id :%u not exist",
                        it->first, *it2);
                return false;
            }
        }
        FOREACH((dup.vip_prize_vec), it2) {
            if (!g_prize_conf_mgr.is_prize_exist(*it2)) {
                ERROR_TLOG("Duplicate :%u vip prize id :%u not exist",
                        it->first, *it2);
                return false;
            }
        }
        FOREACH((dup.svip_prize_vec), it2) {
            if (!g_prize_conf_mgr.is_prize_exist(*it2)) {
                ERROR_TLOG("Duplicate :%u svip prize id :%u not exist",
                        it->first, *it2);
                return false;
            }
        }
    }

    //校验装备升品后的物品是否合法
    FOREACH((g_item_conf_mgr.const_item_conf_map()), it) {
        const item_conf_t &item_conf = it->second;
        uint32_t next_id = item_conf.next_quality_item;
        if (next_id && !g_item_conf_mgr.is_equip(next_id)) {
            ERROR_TLOG("Item equip %u next_quality_item:%u is not equip",
                    item_conf.item_id, next_id);
            return false;
        }
        FOREACH(item_conf.material, it) {
            if (!g_item_conf_mgr.is_item_conf_exist(it->first)) {
                ERROR_TLOG("Item %u upgrade material :%u not exist",
                        item_conf.item_id, it->first);
                return false;
            }
        }
    }
    g_all_conf_loaded ++;

    return succ;
}

int init_connections()
{
    // 初始化dbproxy
    g_dbproxy = new Service(std::string(g_server_config.dbproxy_name));
    ADD_TIMER_EVENT_EX(&g_reconnect_timer, 
            kTimerTypeReconnectServiceTimely, 
            g_dbproxy,
            get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely); 

    // 初始化switch服务器
    g_switch = new Service(std::string(g_server_config.switch_name));
    ADD_TIMER_EVENT_EX(&g_reconnect_timer, 
            kTimerTypeReconnectServiceTimely, 
            g_switch,
            get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely); 

    // 初始化battle服务器
    char real_battle_name[64];
    sprintf(real_battle_name, "%s_%u", g_server_config.battle_name, g_online_id);
    g_battle = new Service(std::string(real_battle_name));
    ADD_TIMER_EVENT_EX(&g_reconnect_timer, 
            kTimerTypeReconnectServiceTimely, 
            g_battle,
            get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely); 

#if 1
    // 初始化battle_center服务器
    g_battle_center = new Service(g_server_config.battle_center_name);
    ADD_TIMER_EVENT_EX(&g_reconnect_timer, 
            kTimerTypeReconnectServiceTimely, 
            g_battle_center,
            get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely); 
#endif

    return 0;
}

int start_function_timers()
{
	// 定时T人定时器
	ADD_TIMER_EVENT_EX(&g_kick_off_timer, 
			kTimerTypeDailyKickOff, 
			0, TimeUtils::second_at_day_start(1) - g_svr_close_before_zero);

    //T人前定时通知用户下线(晚上X点下线, 提前10分钟广播)
    ADD_TIMER_EVENT_EX(&g_sys_noti_timer,
            kTimerTypeSysNotiOffline, 0,
            TimeUtils::second_at_day_start(1) - g_svr_close_before_zero - 600);

    // 世界boss
    ADD_TIMER_EVENT_EX(&g_world_boss_timer, 
            kTimerTypeWorldBossCheck, 
            0, NOW() + kTimerIntervalWorldBoss);

	// 运宝定时器
	ADD_TIMER_EVENT_EX(&g_escort_timer, 
			kTimerTypeTestEscortFinish, 
			0, kTimerIntervalTestEscort);

	// 0点清理跳水排行
	ADD_TIMER_EVENT_EX(&g_reset_dive_timer, 
			kTimerTypeDailyResetDive, 
			0, TimeUtils::second_at_day_start(1));

	ADD_TIMER_EVENT_EX(&g_dump_rank_timer, 
			kTimerTypeDaily21DumpRank, 
			0, NOW() + TimeUtils::second_to_hm(2100) - 5);

    return 0;
}
