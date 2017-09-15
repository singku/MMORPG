#include <openssl/rc4.h>
#include "common.h"

#include "proto.h"
#include "player.h"
#include "global_data.h"
#include "service.h"
#include "attr_utils.h"
#include "attr.h"
#include "proto_queue.h"
#include "player_manager.h"
#include "utils.h"
#include "auth.h"
#include "statlogger/statlogger.h"
#include "timer_procs.h"
#include "user_action_log_utils.h"
#include "switch_proto.h"
#include "shop_conf.h"
#include "shop.h"
#include "map_utils.h"
#include "sys_ctrl.h"
#include "duplicate_utils.h"
#include "home_data.h"
#include "family_utils.h"
#include "duplicate_processor.h"
#include "rank_utils.h"
#include "swim.h"
#include "player_utils.h"
#include "escort_utils.h"
#include "mencrypt.h"
#include "task_utils.h"
#include "sys_ctrl.h"
#include "mail.h"
#include "mail_utils.h"
#include "pet_utils.h"
#include "time_utils.h"
#include "mine_utils.h"

#include <boost/lexical_cast.hpp>
#define CHECK_ONLINE(player) \
    do { \
        if (player && player->is_login == false \
            && player->cli_wait_cmd != cli_cmd_cs_0x0101_enter_svr) { \
            asynsvr_send_warning_msg("offline send to cli",  \
                    player->userid, cmd, 1, ""); \
            WARN_TLOG("offine player[%u] send to cli stack: '%s'", \
                    player->userid, stack_trace().c_str()); \
            return 0; \
        } \
    } while (0)

static char g_send_buf[65536 * 32];

int send_buff_to_fdsess(fdsession_t *fdsess, uint32_t uid, uint32_t cmd, 
        const char *body, uint32_t bodylen, uint32_t seq, uint32_t ret)
{
    cli_proto_header_t* header = (cli_proto_header_t *)(g_send_buf);
    onlineproto::proto_header_t pb_header;
    pb_header.Clear();
    pb_header.set_uid(uid);
    pb_header.set_seque(seq);
    pb_header.set_ret(ret);

    uint32_t head_len = pb_header.ByteSize();
    uint32_t total_len = sizeof(*header) + head_len;
    if (body && bodylen) {
        total_len += bodylen;
    }

    if (sizeof(g_send_buf) < total_len) {
        ERROR_TLOG("%u send buf size %lu not enough[need: %lu]",
                uid, sizeof(g_send_buf), total_len);
        return -1;
    }
 
    pb_header.SerializeToArray(g_send_buf + sizeof(*header), 
            sizeof(g_send_buf) - sizeof(*header));
    if (body && bodylen) {
        memcpy(g_send_buf + sizeof(*header) + head_len, body, bodylen);
    }

    header->total_len = total_len;
    header->head_len = head_len;
    header->cmd = cmd;

    if (cli_proto_encrypt) {//cli_proto_header_t之后的数据加密
        uint32_t encrypt_part_len = 0;
        uint32_t no_encrypt_len = sizeof(*header) - sizeof(uint32_t);
        char *encrypt_part = (char*)msg_encrypt(g_send_buf + no_encrypt_len, 
                head_len + bodylen + sizeof(uint32_t), &encrypt_part_len);
        total_len = no_encrypt_len + encrypt_part_len;
        if (total_len > sizeof(g_send_buf)) {
            ERROR_TLOG("U:%u too large body package %d", uid, total_len); 
            return -1;
        }
        memcpy(g_send_buf + no_encrypt_len, encrypt_part, encrypt_part_len);
        header->total_len = total_len;
    }

    int err = send_pkg_to_client(fdsess, g_send_buf, header->total_len);
    if (err) {
        return -1;
    }
    return header->total_len;
}

int send_buff_to_player(player_t *player, uint32_t cmd, const char *body, uint32_t bodylen,
        uint32_t seq, int ret)
{
    assert(player);
    LOCK_CLI_CHECK;
    CHECK_ONLINE(player);

    int len = send_buff_to_fdsess(player->fdsess, player->userid, cmd, body, bodylen,
            seq ?seq :player->seqno, ret);
    if (len == -1) {
        return -1;
    }

    temp_info_t* temp_info = &player->temp_info;
    struct timeval cli_diff_time = {0};
    if (cmd == player->cli_wait_cmd) {
        struct timeval cli_rsp_time = *get_now_tv();
        timersub(&cli_rsp_time, &(temp_info->cli_req_start), &cli_diff_time);
    }

TRACE_TLOG("Send Buff To Player Ok: [U:%u Len:%u Seq:%u Cmd:%d HexCmd:0x%04X Ret:%u cost: %d.%06d]",
            player->userid, len, seq ?seq :player->seqno, cmd, cmd, ret,
            cli_diff_time.tv_sec, cli_diff_time.tv_usec);

    return 0;
}

int send_msg_to_player(player_t* player, uint32_t cmd, const google::protobuf::Message& msg)
{
    assert(player);
    LOCK_CLI_CHECK;
    CHECK_ONLINE(player);

    static string tmp_str;
    msg.SerializeToString(&tmp_str);
    int len = send_buff_to_fdsess(player->fdsess, player->userid, cmd, tmp_str.c_str(), tmp_str.size(),
            player->seqno, 0);
    if (len == -1) {
        return -1;
    }

    temp_info_t* temp_info = &player->temp_info;
    struct timeval cli_diff_time = {0};
    if (cmd == player->cli_wait_cmd) {
        struct timeval cli_rsp_time = *get_now_tv();
        timersub(&cli_rsp_time, &(temp_info->cli_req_start), &cli_diff_time);
    }

    if (msg.GetTypeName() != "onlineproto.sc_0x0107_notify_player_change_state") {
TRACE_TLOG("Send Msg To Player Ok: [U:%u Len:%u Seq:%u Cmd:%d HexCmd:0X%04X msg_len:%u cost:%d.%06d]\nmsg:%s\n[%s]",
            player->userid, len, player->seqno, cmd, cmd, tmp_str.size(),
            cli_diff_time.tv_sec, cli_diff_time.tv_usec,
            msg.GetTypeName().c_str(), msg.Utf8DebugString().c_str());
    }
    return 0;
}

int send_err_to_player(player_t* player, uint32_t cmd, int ret, uint32_t seq)
{
    assert(player);
    LOCK_CLI_CHECK;
    CHECK_ONLINE(player);
    return send_buff_to_player(player, cmd, 0, 0, seq, ret);
}

int send_err_to_fdsess(fdsession_t *fdsess, uint32_t cmd, uint32_t uid, int err, uint32_t seq)
{
    int len = send_buff_to_fdsess(fdsess, uid, cmd, 0, 0, seq, err);
    if (len == -1) {
        return -1;
    }

TRACE_TLOG("Send Err To fdsess Ok: [U:%u Len:%u Seq:%u Cmd:%d HexCmd:0X%04X Ret:%d]",
            uid, len, seq, cmd, cmd, err);
    return 0;
}

// 玩家离开服务器
int player_leave_server(player_t* player)
{
    assert(player);

    uint32_t level = GET_A(kAttrLv);

    REMOVE_TIMERS(player); // 定时器要删掉，他可能去三组请求验证session
    g_pending_proto_players.erase(player->userid); // 可能有很多包排着对
    if (!player->is_login) {
        // 还没有登录的用户 还没有初始化一些数据结构
        return 0; 
    }

    //日志记录用户离开
    UserActionLogUtils::write_db_log(player, dbproto::ActionTypeLogout, 0, 0, 0, 0, 0);
   
    //完成一些业务逻辑待完成的事件
    finish_module(player);

    // 通知switch
    online_report_player_onoff(player, (uint32_t)switchproto::PLAYER_LOGOUT);

    // 离开地图
    MapUtils::leave_map(player);

    //保存上次下线时间
    attr_data_info_t attrs[2] = {
        {kAttrLastLogoutTm, NOW()}, // 上次离线时间
        {kDailyLastLogoutTime, NOW()},  // 防沉迷的今日离线时间
    };
    AttrUtils::set_attr_value(player, array_elem_num(attrs), attrs);

    //累计当日在线的时间
    uint32_t daily_online_time = calc_daily_online_time(player);
    SET_A(kDailyOnlineTime, daily_online_time);

    //累计当日防沉迷在线的时间
    uint32_t daily_addict_online_time = calc_daily_addiction_online_time(player);
    SET_A(kDailyOnlineTimeCleanByDay, daily_addict_online_time);

    //累计距离上次领在线奖过去的时间
    uint32_t last_prize_tm = GET_A(kDailyOnlineRewardLastPrizeTm);
    int diff;
    if (last_prize_tm < GET_A(kDailyLastLoginTime)) {
        diff = NOW() - GET_A(kDailyLastLoginTime);
    } else {
        diff = NOW() - last_prize_tm;
    }
    if (diff > 0) {
        ADD_A(kDailyOnlineRewardPrizePastTm, diff);
    }

    // 通知平台用户退出
    static char buf[256];
    chnlhash32_t* hash = (chnlhash32_t *)buf;
    act_user_logout_req_t* req = 
        (act_user_logout_req_t *)(buf + sizeof(*hash));

    req->gameid = g_server_config.gameid;
    req->login_time = AttrUtils::get_attr_value(
            player, kAttrLastLoginTm);
    req->logout_time = AttrUtils::get_attr_value(
            player, kAttrLastLogoutTm);

    // TODO toby 合并到refresh
    // 删除家族操作锁
    if (player->family_lock_sets != NULL) {
        FOREACH(*(player->family_lock_sets), it) {
            RankUtils::lock_release(0, player->userid, player->create_tm, *it);
        }
    } 

    // 更新家族成员信息
    uint32_t family_id = GET_A(kAttrFamilyId);
    if (FamilyUtils::is_valid_family_id(family_id)) {
        dbproto::family_member_table_t up_info;
        up_info.Clear();
        up_info.set_family_id(family_id);
        up_info.set_userid(player->userid);
        up_info.set_u_create_tm(player->create_tm);
        up_info.set_battle_value(GET_A(kAttrCurBattleValue));
        up_info.set_last_logout_time(GET_A(kAttrLastLogoutTm));
        FamilyUtils::update_family_member_info(0, up_info, dbproto::DB_UPDATE_NO_INSERT);

        // 更新家族在线记录
        std::vector<std::string> vector;
        vector.push_back(
            boost::lexical_cast<std::string>(
                ROLE_KEY(ROLE(player->userid, player->create_tm))));
        RankUtils::set_del_member(
                NULL, player->userid, player->create_tm,
                rankproto::SET_FAMILY_ONLINE_USERIDS, 
                family_id,
                vector);
    }

    gen_chnlhash32(g_server_config.verifyid,
            g_server_config.security_code, 
            (const char *)req, sizeof(*req), hash);
    g_dbproxy->send_to_act(NULL, player->userid, 
            act_cmd_user_logout, buf, sizeof(*hash) + sizeof(*req));

    uint32_t oltime = 0;
    if (req->logout_time > req->login_time) {
        oltime = req->logout_time - req->login_time;
    }

    TRACE_TLOG("player leave:%u", player->userid);

    // 记录统计日志
    std::stringstream uid_str;
    uid_str << player->userid;
    g_stat_logger->logout(uid_str.str(), is_vip(player), level, oltime);

    player->is_login = false;
    return 0;
}

uint32_t player_get_diamond(player_t *player)
{
    return (GET_A(kAttrPaidDiamond) + GET_A(kAttrDiamond));
}

//先扣购买的钻石 再扣赠送的钻石(属性值表示)
//增加时 增加购买的钻石 赠送钻石不走这个逻辑
int player_chg_diamond_and_sync(player_t *player, 
        int32_t diff, 
        const product_t *pd, 
        uint32_t pd_cnt,
        dbproto::channel_type_t chn, 
        string default_pd_name,
        bool sync_db,
        bool add_paid_diamond)
{
    if (player->temp_info.can_use_diamond == false) {
        return cli_err_last_trans_not_finished;
    }   

    uint32_t buy_diamond = GET_A(kAttrPaidDiamond);
    uint32_t given_diamond = GET_A(kAttrDiamond);
    uint32_t total_diamond = buy_diamond + given_diamond;

    uint32_t sub_given_diamond = 0;
    int32_t buy_diamond_diff = 0;

    if (diff <= 0) {
        uint32_t f_diff = diff * (-1);
        if (total_diamond < f_diff) {
			ERROR_TLOG("uid=[%u]:total diamond not enough, total_diamond=[%u],f_diff=[%u]", 
                    player->userid, total_diamond, f_diff);
            return cli_err_lack_diamond;
        }

        if (buy_diamond < f_diff) {
            sub_given_diamond = f_diff - buy_diamond;
            SUB_A(kAttrDiamond, sub_given_diamond);
            SET_A(kAttrPaidDiamond, 0);
            buy_diamond_diff = -(buy_diamond);
        } else {
            SUB_A(kAttrPaidDiamond, f_diff);
            buy_diamond_diff = diff;
        }

        if (TimeUtils::is_current_time_valid(TM_CONF_KEY_CONSUME_DIAMOND, 0)) {//活动时间内
            if (!TimeUtils::is_time_valid(GET_A(kAttrConsumeDiamondLastJoinTm), TM_CONF_KEY_CONSUME_DIAMOND, 0)) {
                SET_A(kAttrConsumeDiamondLastJoinTm, NOW());
                SET_A(kAttrConsumeDiamondCnt, 0);
                SET_A(kAttrConsumeDiamondGiftGetFlag, 0);
            }
            ADD_A(kAttrConsumeDiamondCnt, f_diff);
        }

        //付费钻石消耗统计
        if (buy_diamond_diff) {
            StatInfo stat_info;
            stat_info.add_info("item", pd ?pd->name :default_pd_name);
            stat_info.add_info("数量", pd_cnt);
            stat_info.add_info("付费钻石", abs(buy_diamond_diff));
            stat_info.add_op(StatInfo::op_item, "item");
            stat_info.add_op(StatInfo::op_item_sum, "item", "数量");
            stat_info.add_op(StatInfo::op_item_sum, "item", "付费钻石");
            g_stat_logger->log("钻石系统", "付费钻石购买商品", 
                    Utils::to_string(player->userid), "", stat_info);

            g_stat_logger->buy_item(Utils::to_string(player->userid), 
                    is_vip(player), GET_A(kAttrLv), abs(buy_diamond_diff), pd ?pd->name :default_pd_name, pd_cnt);
        }

        //免费钻石消耗统计
        StatInfo stat_info;
        stat_info.add_info("item", pd ?pd->name :default_pd_name);
        stat_info.add_info("数量", pd_cnt);
        stat_info.add_info("免费钻石", sub_given_diamond);
        stat_info.add_op(StatInfo::op_item, "item");
        stat_info.add_op(StatInfo::op_item_sum, "item", "数量");
        stat_info.add_op(StatInfo::op_item_sum, "item", "免费钻石");
        g_stat_logger->log("钻石系统", "免费钻石购买商品", 
                Utils::to_string(player->userid), "", stat_info);

        //总计消耗统计
        StatInfo stat_info_all;
        stat_info_all.add_info("付费钻石消耗总量", abs(buy_diamond_diff));
        stat_info_all.add_info("免费钻石消耗总量", sub_given_diamond);
        stat_info_all.add_op(StatInfo::op_sum, "付费钻石消耗总量");
        stat_info_all.add_op(StatInfo::op_sum, "免费钻石消耗总量");
        g_stat_logger->log("钻石系统", "支出", 
                Utils::to_string(player->userid), "", stat_info);

    //钻石增加
    } else {
        if (add_paid_diamond) {
            ADD_A(kAttrPaidDiamond, diff);
        } else {
            ADD_A(kAttrDiamond, diff);
        }
        buy_diamond_diff = diff;
        StatInfo stat_info;
        stat_info.add_info("item", pd ?pd->name :default_pd_name);
        stat_info.add_info("数量", pd_cnt);
        stat_info.add_info("付费钻石", add_paid_diamond ?diff :0);
        stat_info.add_info("免费钻石", add_paid_diamond ?0 :diff);
        stat_info.add_op(StatInfo::op_item, "item");
        stat_info.add_op(StatInfo::op_item_sum, "item", "数量");
        stat_info.add_op(StatInfo::op_item_sum, "item", "付费钻石");
        stat_info.add_op(StatInfo::op_item_sum, "item", "免费钻石");
        g_stat_logger->log("钻石系统", "收入", 
                Utils::to_string(player->userid), "", stat_info);
    }

    //交易记录同步DB
    if (sync_db) {
        //交易记录
		gen_db_diamond_transaction_new(player, pd, pd_cnt,
				buy_diamond_diff * 100, chn);
        //dbproto::cs_new_transaction trans_msg;
        //gen_db_transaction(player, pd, pd_cnt, 
		//       buy_diamond_diff * 100, chn, trans_msg.mutable_info());
        //g_dbproxy->send_msg(NULL, player->userid, 
		//		player->create_tm,
		//		db_cmd_new_diamond_transaction, trans_msg);

        player->temp_info.can_use_diamond = false;
        player->check_money_return = ADD_TIMER_EVENT_EX(player, 
                kTimerTypeCheckDbDiamondTimely,
                (void*)(player->userid),
                NOW() + kTimerIntervalDbDiamondTimely);

    }
    return 0;
}

//计算玩家每日在线的时间
uint32_t calc_daily_online_time(player_t* player)
{
    uint32_t last_login_time = AttrUtils::get_attr_value(
            player, kDailyLastLoginTime);
    uint32_t now = get_now_tv()->tv_sec;
    uint32_t online_time = AttrUtils::get_attr_value(
            player, kDailyOnlineTime);

    // == 0 表示在跨天的时候因玩家在线被清理掉了
    if (last_login_time == 0) {
        // 则设置成当天的起始时间
        last_login_time = TimeUtils::day_align_low(now) + 1;
        attr_data_info_t attr = {
            kDailyLastLoginTime, last_login_time
        };
        AttrUtils::set_attr_value(player, 1, &attr);
    }

    if (now > last_login_time) {
        return online_time + (now - last_login_time); 
    } else {
        return online_time;
    }
    return 0;
}

//计算玩家每日防沉迷在线的时间,离线时间达到规定时间，会清零
uint32_t calc_daily_addiction_online_time(player_t* player)
{
    uint32_t last_login_time = AttrUtils::get_attr_value(
            player, kDailyLastLoginTime);
    uint32_t now = get_now_tv()->tv_sec;
    uint32_t online_time = AttrUtils::get_attr_value(
            player, kDailyOnlineTimeCleanByDay);

    // == 0 表示在跨天的时候因玩家在线被清理掉了
    if (last_login_time == 0) {
        // 则设置成当天的起始时间
        last_login_time = TimeUtils::day_align_low(now) + 1;
        attr_data_info_t attr = {
            kDailyLastLoginTime, last_login_time
        };
        AttrUtils::set_attr_value(player, 1, &attr);
    }

    if (now > last_login_time) {
        return online_time + (now - last_login_time); 
    } else {
        return online_time;
    }
    return 0;
}
uint32_t calc_daily_offline_time(player_t* player)
{
    uint32_t last_logout_time = AttrUtils::get_attr_value(
            player, kDailyLastLogoutTime);
    uint32_t offline_time = AttrUtils::get_attr_value(
            player, kDailyOfflineTime);

    if (last_logout_time == 0) {
        return 0;  
    } else {
        if (NOW() > last_logout_time) {
            return offline_time + (NOW() - last_logout_time); 
        } else {
            return offline_time; 
        }
    }

    return 0;
}

bool is_vip(const player_t* player)
{
	/*
    uint32_t vip_begin_time = AttrUtils::has_attr(player, kAttrVipBeginTime) 
        ?GET_A(kAttrVipBeginTime) :0;
    uint32_t vip_end_time = AttrUtils::has_attr(player, kAttrVipEndTime) 
        ?GET_A(kAttrVipEndTime) :0;

    uint32_t now = get_now_tv()->tv_sec;

    if (vip_begin_time == 0 || vip_end_time == 0) {
        return false; 
    } else {
        // 截止到这天的结束
        vip_end_time = TimeUtils::day_align_high(vip_end_time);
        if (vip_end_time > vip_begin_time && vip_end_time >= now) {
            return true; 
        } else {
            return false; 
        }
    }
	*/
	return is_gold_vip(player) || is_silver_vip(player);
}

bool is_silver_vip(const player_t* player)
{
	uint32_t silver_vip_end_time = AttrUtils::has_attr(player, kAttrSilverVipEndTime)
		?GET_A(kAttrSilverVipEndTime) : 0;
	if (silver_vip_end_time > NOW()) {
		return true;
	} else {
		return false;
	}
}

bool is_gold_vip(const player_t* player)
{
	uint32_t gold_vip_end_time = AttrUtils::has_attr(player, kAttrGoldVipEndTime) 
		?GET_A(kAttrGoldVipEndTime) : 0;
	if (gold_vip_end_time > NOW()) {
		return true;
	} else {
		return false;
	}
}

player_vip_type_t get_player_vip_flag(const player_t* player)
{
	if (is_gold_vip(player)) {
		return GOLD_VIP;
	} else if (is_silver_vip(player)) {
		return SILVER_VIP;
	}	
	return NO_VIP;
}

player_vip_type_t get_player_base_info_vip_flag(
        const commonproto::player_base_info_t &info)
{
    uint32_t gold_vip_end_time = info.gold_vip_end_time();
    uint32_t silver_vip_end_time = info.silver_vip_end_time();
    if (gold_vip_end_time > NOW()) {
        return GOLD_VIP;
    }

    if (silver_vip_end_time > NOW()) {
        return SILVER_VIP;
    }

    return NO_VIP;
}

uint32_t calc_seqno(uint32_t pkg_len, uint32_t seqno, uint16_t cmd)
{           
    return seqno - seqno / 7 + 147 + pkg_len % 21 + cmd % 13;
}   

#if 0
bool check_player_addicted(player_t* player)
{
    uint32_t online_time = calc_daily_online_time(player);
    
    if (online_time >= kOnlineTimeThresholdNone) {
        return true; 
    } else {
        return false; 
    }
}
#endif

void finish_module(player_t *player)
{
    assert(player);

    //如果在小屋 通知其他人离开小屋
	PlayerUtils::leave_current_home(player);

    //如果玩家还在打副本
    if (player->temp_info.dup_state != PLAYER_DUP_NONE) {
        DupUtils::tell_btl_exit(player, NO_WAIT_SVR);

        //统计离线时副本状态
        Utils::write_msglog_new(player->userid, "副本", "离线时所处阶段", 
                DupUtils::get_duplicate_name(player->temp_info.dup_id) + " " 
                + Utils::to_string(player->temp_info.dup_phase));

        Utils::write_msglog_new(player->userid, "副本", "离线时在副本的时长", 
                DupUtils::get_duplicate_name(player->temp_info.dup_id) + " " 
                + Utils::to_string((uint32_t)(NOW() - player->temp_info.dup_enter_tm)));
        
    }

    //如果玩家在排队跳水
    if (GET_A(kDailyDiveIsStart)) {
        SET_A(kDailyDiveIsStart, 0);
        g_dive.remove_diver_by_userid(player->userid);
    }

	//Kevin:如果玩家还在运宝的打劫过程中:设置使得被打劫者正在打劫他的人数减1
	//这样被打劫者运宝时间到了后，他的信息就能顺利从运宝管理器中删除
	if (player->temp_info.escort_def_id) {
		EscortUtils::deal_when_rob_result_come_out(player, commonproto::QUIT);
	}

	MineUtils::set_mine_attack_state(player, MINE_NOT_BE_ATTACKED);
	if (player->mine_info->mine_id_ && player->mine_info->def_uid_key_) {
		std::string pkg;
		std::vector<rob_resource_info_t> tmp_vec;
		MineUtils::generate_mine_btl_report(player,
				player->mine_info->mine_id_, pkg,
				commonproto::WIN, tmp_vec);
		std::vector<role_info_t> role_vec;
		role_info_t roles = KEY_ROLE(player->mine_info->def_uid_key_);
		role_vec.push_back(roles);
		RankUtils::save_battle_simple_report(player, role_vec,
				commonproto::MINE_FIGHT, pkg, DAY_SECS * 7);
	}
}

bool is_login_step_finished(player_t *player, login_steps_t step)
{
    return player->temp_info.login_steps->test(step);
}

void set_login_step_finished(player_t *player, login_steps_t step)
{
    player->temp_info.login_steps->set(step);
}

void try_flush_all_shops(player_t *player, bool immediatly)
{
    try_flush_shop(player, immediatly, onlineproto::MARKET_TYPE_DAILY);
    try_flush_shop(player, immediatly, onlineproto::MARKET_TYPE_ELEM_DUP);
    try_flush_shop(player, immediatly, onlineproto::MARKET_TYPE_ARENA);
    try_flush_shop(player, immediatly, onlineproto::MARKET_TYPE_EXPED);
    try_flush_shop(player, immediatly, onlineproto::MARKET_TYPE_FAMILY);
    try_flush_shop(player, immediatly, onlineproto::MARKET_TYPE_NIGHT_RAID);
    try_flush_shop(player, immediatly, onlineproto::MARKET_TYPE_SMELT_MONEY);
    try_flush_shop(player, immediatly, onlineproto::MARKET_TYPE_SMELT_GOLD);
}

//登录时尝试刷新商店/定时器到刷新商店/玩家主动刷新商店
//登录时尝试刷新需判断上次下线时间
//定时器到为立即刷新
void try_flush_shop(player_t *player, bool immediatly, onlineproto::market_type_t market_type)
{
    attr_type_t last_fresh_attr;
    std::map<uint32_t, ol_market_item_t> *shop_item_map = 0;
    dbproto::user_raw_data_type_t db_data_type;

    switch (market_type) {
    case onlineproto::MARKET_TYPE_DAILY:
        last_fresh_attr = kAttrSpShopLastRefreshTm;
        shop_item_map = player->daily_shop_items;
        db_data_type = dbproto::DAILY_SHOP_PRODUCT;
        break;

    case onlineproto::MARKET_TYPE_ELEM_DUP:
        last_fresh_attr = kAttrElemDupShopLastRefreshTm;
        shop_item_map = player->elem_dup_shop_items;
        db_data_type = dbproto::ELEM_DUP_SHOP_PRODUCT;
        break;

    case onlineproto::MARKET_TYPE_ARENA:
        last_fresh_attr = kAttrArenaShopLastRefreshTm;
        shop_item_map = player->arena_shop_items;
        db_data_type = dbproto::ARENA_SHOP_PRODUCT;
        break;

    case onlineproto::MARKET_TYPE_EXPED:
        last_fresh_attr = kAttrExpedShopLastRefreshTm;
        shop_item_map = player->exped_shop_items;
        db_data_type = dbproto::EXPED_SHOP_PRODUCT;
        break;
    case onlineproto::MARKET_TYPE_NIGHT_RAID:
        last_fresh_attr = kAttrNightRaidShopLastRefreshTm;
        shop_item_map = player->night_raid_shop_items;
        db_data_type = dbproto::NIGHT_RAID_SHOP_PRODUCT;
        break;

    case onlineproto::MARKET_TYPE_FAMILY:
        last_fresh_attr = kAttrFamilyShopLastRefreshTm;
        shop_item_map = player->family_shop_items;
        db_data_type = dbproto::FAMILY_SHOP_PRODUCT;
        break;
	case onlineproto::MARKET_TYPE_SMELT_MONEY:
		last_fresh_attr = kAttrSmeltMoneyLastRefreshTm;
		shop_item_map = player->smelter_money_shop_items;
		db_data_type = dbproto::SMELTER_MONEY;
		break;
	case onlineproto::MARKET_TYPE_SMELT_GOLD:
		last_fresh_attr = kAttrSmeltGoldLastRefreshTm;
		shop_item_map = player->smelter_gold_shop_items;
		db_data_type = dbproto::SMELTER_GOLD;
		break;
    default:
        return;
    }

    bool flush = false;
    if (immediatly) {//立即刷新
        flush = true;
    } else {
        uint32_t time1 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time1", 1200);
        uint32_t time2 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time2", 1800);

        uint32_t now_hm = TimeUtils::second_trans_hm(NOW());
        uint32_t last_refresh_tm = GET_A(last_fresh_attr);
        uint32_t last_refresh_hm = TimeUtils::second_trans_hm(last_refresh_tm);
        if (last_refresh_tm == 0) {
            flush = true;
        } else if (!TimeUtils::is_same_day(last_refresh_tm, NOW())) {
            if (now_hm >= time1) {
                flush = true;
            }
        } else {//当天
            if (last_refresh_hm < time1 && now_hm >= time1) {
                flush = true;
            } else if (last_refresh_hm >= time1 && last_refresh_hm < time2 && now_hm >= time2) {
                flush = true;
            }
        }
    }
    if (!flush) {
        return;
    }

    std::vector<market_product_t> result;
    random_select_product_from_market_default((uint32_t)market_type, result);
    shop_item_map->clear();

    FOREACH(result, iter) {
        uint32_t product_id = (*iter).shop_id;
        std::map<uint32_t, ol_market_item_t>::iterator iter2 = 
           shop_item_map->find(product_id);
        if(iter2 != shop_item_map->end()) {
            ol_market_item_t *item = &(iter2->second);
            item->count = item->count + 1;
        } else {
            ol_market_item_t item;
            item.item_id = product_id;
            item.count = 1;
            shop_item_map->insert(
                    std::pair<uint32_t, ol_market_item_t>(product_id, item));
        }
    }

    onlineproto::sc_0x0131_shop_refresh noti;
    noti.set_market(market_type);
    commonproto::market_item_info_t *item_info = noti.mutable_item_info();
    FOREACH((*shop_item_map), iter) {
        commonproto::market_item_t *item = item_info->add_items();
        item->set_item_id(iter->second.item_id);
        item->set_count(iter->second.count);
    }
    send_msg_to_player(player, cli_cmd_cs_0x0131_shop_refresh, noti);

    // 同步到db
    PlayerUtils::update_user_raw_data(
            player->userid, player->create_tm, 
            db_data_type, noti.item_info(), "0");
    
    SET_A(last_fresh_attr, NOW());
    return;

    /*
    if (market_type == onlineproto::MARKET_TYPE_DAILY) {
        try_flush_daily_shop(player, immediatly);
    } else if (market_type == onlineproto::MARKET_TYPE_ELEM_DUP) {
        try_flush_elem_dup_shop(player, immediatly);
    } else if (market_type == onlineproto::MARKET_TYPE_ARENA) {
        try_flush_arena_shop(player, immediatly);
    } else if (market_type == onlineproto::MARKET_TYPE_EXPED) {
		try_flush_exped_shop(player, immediatly);
	} else if (market_type == onlineproto::MARKET_TYPE_FAMILY) {
        try_flush_family_shop(player, immediatly);
    } else if (market_type == onlineproto::MARKET_TYPE_NIGHT_RAID) {
		try_flush_night_raid_shop(player, immediatly);
		}
    */
    return;
}

#if 0
// 刷新每日商店
void try_flush_daily_shop(player_t *player, bool immediatly)
{
    bool flush = false;
    if (immediatly) {//立即刷新
        flush = true;
    } else {
        uint32_t time1 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time1", 1200);
        uint32_t time2 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time2", 1800);

        uint32_t now_hm = TimeUtils::second_trans_hm(NOW());
        uint32_t last_refresh_tm = GET_A(kAttrSpShopLastRefreshTm);
        uint32_t last_refresh_hm = TimeUtils::second_trans_hm(last_refresh_tm);
        if (last_refresh_tm == 0) {
            flush = true;
        } else if (!TimeUtils::is_same_day(last_refresh_tm, NOW())) {
            if (now_hm >= time1) {
                flush = true;
            }
        } else {//当天
            if (last_refresh_hm < time1 && now_hm >= time1) {
                flush = true;
            } else if (last_refresh_hm > time1 && last_refresh_hm < time2 && now_hm >= time2) {
                flush = true;
            }
        }
    }
    if (!flush) {
        return;
    }

    std::vector<market_product_t> result;
    random_select_product_from_market_default(MARKET_TYPE_DAILY, result);
    AttrUtils::ranged_clear(player, 
            kAttrSpShopProduct1Id, kAttrSpShopProduct10Cnt, NO_NOTI_CLI);
    uint32_t i = 0;
    std::vector<attr_data_info_t> attr_vec;
    for (i = 0; i < DAILY_MARKET_COLUMN && i < result.size(); i++) {
        attr_type_t id_attr = (attr_type_t)(kAttrSpShopProduct1Id + 2*i);
        attr_type_t cnt_attr = (attr_type_t)(id_attr + 1);
        attr_data_info_t attr;

        attr.type = id_attr;
        attr.value = result[i].shop_id;
        attr_vec.push_back(attr);

        attr.type = cnt_attr;
        attr.value = 1;
        attr_vec.push_back(attr);
    }
    for (; i < DAILY_MARKET_COLUMN; i++) {
        attr_type_t id_attr = (attr_type_t)(kAttrSpShopProduct1Id + 2*i);
        attr_type_t cnt_attr = (attr_type_t)(id_attr + 1);
        attr_data_info_t attr;

        attr.type = id_attr;
        attr.value = 0;
        attr_vec.push_back(attr);

        attr.type = cnt_attr;
        attr.value = 0;
        attr_vec.push_back(attr);
    }
    AttrUtils::set_attr_value(player, attr_vec);
    onlineproto::sc_0x0131_shop_refresh noti;
    noti.set_market(onlineproto::MARKET_TYPE_DAILY);
    send_msg_to_player(player, cli_cmd_cs_0x0131_shop_refresh, noti);
    SET_A(kAttrSpShopLastRefreshTm, NOW());
}

// 刷新元素挑战商店
void try_flush_elem_dup_shop(player_t *player, bool immediatly)
{
    bool flush = false;
    if (immediatly) {//立即刷新
        flush = true;
    } else {
        uint32_t time1 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time1", 1200);
        uint32_t time2 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time2", 1800);

        uint32_t now_hm = TimeUtils::second_trans_hm(NOW());
        uint32_t last_refresh_tm = GET_A(kAttrElemDupShopLastRefreshTm);
        uint32_t last_refresh_hm = TimeUtils::second_trans_hm(last_refresh_tm);
        if (last_refresh_tm == 0) {
            flush = true;
        } else if (!TimeUtils::is_same_day(last_refresh_tm, NOW())) {
            if (now_hm >= time1) {
                flush = true;
            }
        } else {//当天
            if (last_refresh_hm < time1 && now_hm >= time1) {
                flush = true;
            } else if (last_refresh_hm > time1 && last_refresh_hm < time2 && now_hm >= time2) {
                flush = true;
            }
        }
    }
    if (!flush) {
        return;
    }

    std::vector<market_product_t> result;
    random_select_product_from_market_default(MARKET_TYPE_ELEM_DUP, result);

    player->elem_dup_shop_items->clear();
    FOREACH(result, iter) {
        uint32_t product_id = (*iter).shop_id;
        std::map<uint32_t, ol_market_item_t>::iterator iter2 = 
           player->elem_dup_shop_items->find(product_id);
        if(iter2 != player->elem_dup_shop_items->end()) {
            ol_market_item_t *item = &(iter2->second);
            item->count = item->count + 1;
        } else {
            ol_market_item_t item;
            item.item_id = product_id;
            item.count = 1;
            player->elem_dup_shop_items->insert(
                    std::pair<uint32_t, ol_market_item_t>(product_id, item));
        }
    }

    onlineproto::sc_0x0131_shop_refresh noti;
    noti.set_market(onlineproto::MARKET_TYPE_ELEM_DUP);
    commonproto::market_item_info_t *item_info = noti.mutable_item_info();
    FOREACH(*(player->elem_dup_shop_items), iter) {
        commonproto::market_item_t *item = item_info->add_items();
        item->set_item_id(iter->second.item_id);
        item->set_count(iter->second.count);
    }
    send_msg_to_player(player, cli_cmd_cs_0x0131_shop_refresh, noti);

    // 同步到db
    PlayerUtils::update_user_raw_data(
            player->userid, player->create_tm, 
            dbproto::ELEM_DUP_SHOP_PRODUCT, noti.item_info(), "0");
    
    SET_A(kAttrElemDupShopLastRefreshTm, NOW());
    return;
}

// 刷新竞技场商店
void try_flush_arena_shop(player_t *player, bool immediatly)
{
    bool flush = false;
    if (immediatly) {//立即刷新
        flush = true;
    } else {
        uint32_t time1 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time1", 1200);
        uint32_t time2 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time2", 1800);

        uint32_t now_hm = TimeUtils::second_trans_hm(NOW());
        uint32_t last_refresh_tm = GET_A(kAttrArenaShopLastRefreshTm);
        uint32_t last_refresh_hm = TimeUtils::second_trans_hm(last_refresh_tm);
        if (last_refresh_tm == 0) {
            flush = true;
        } else if (!TimeUtils::is_same_day(last_refresh_tm, NOW())) {
            if (now_hm >= time1) {
                flush = true;
            }
        } else {//当天
            if (last_refresh_hm < time1 && now_hm >= time1) {
                flush = true;
            } else if (last_refresh_hm > time1 && last_refresh_hm < time2 && now_hm >= time2) {
                flush = true;
            }
        }
    }
    if (!flush) {
        return;
    }

    std::vector<market_product_t> result;
    random_select_product_from_market_default(MARKET_TYPE_ARENA, result);

    player->arena_shop_items->clear();
    FOREACH(result, iter) {
        uint32_t product_id = (*iter).shop_id;
        std::map<uint32_t, ol_market_item_t>::iterator iter2 = 
           player->arena_shop_items->find(product_id);
        if(iter2 != player->arena_shop_items->end()) {
            ol_market_item_t *item = &(iter2->second);
            item->count = item->count + 1;
        } else {
            ol_market_item_t item;
            item.item_id = product_id;
            item.count = 1;
            player->arena_shop_items->insert(
                    std::pair<uint32_t, ol_market_item_t>(product_id, item));
        }
    }

    onlineproto::sc_0x0131_shop_refresh noti;
    noti.set_market(onlineproto::MARKET_TYPE_ARENA);
    commonproto::market_item_info_t *item_info = noti.mutable_item_info();
    FOREACH(*(player->arena_shop_items), iter) {
        commonproto::market_item_t *item = item_info->add_items();
        item->set_item_id(iter->second.item_id);
        item->set_count(iter->second.count);
    }
    send_msg_to_player(player, cli_cmd_cs_0x0131_shop_refresh, noti);

    // 同步到db
    PlayerUtils::update_user_raw_data(
            player->userid, player->create_tm, 
            dbproto::ARENA_SHOP_PRODUCT, noti.item_info(), "0");
    
    SET_A(kAttrArenaShopLastRefreshTm, NOW());
    return;
}

void try_flush_night_raid_shop(player_t *player, bool immediatly)
{
    bool flush = false;
    if (immediatly) {//立即刷新
        flush = true;
    } else {
        uint32_t time1 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time1", 1200);
        uint32_t time2 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time2", 1800);

        uint32_t now_hm = TimeUtils::second_trans_hm(NOW());
        uint32_t last_refresh_tm = GET_A(kAttrNightRaidShopLastRefreshTm);
        uint32_t last_refresh_hm = TimeUtils::second_trans_hm(last_refresh_tm);
        if (last_refresh_tm == 0) {
            flush = true;
        } else if (!TimeUtils::is_same_day(last_refresh_tm, NOW())) {
            if (now_hm >= time1) {
                flush = true;
            }
        } else {//当天
            if (last_refresh_hm < time1 && now_hm >= time1) {
                flush = true;
            } else if (last_refresh_hm > time1 && last_refresh_hm < time2 && now_hm >= time2) {
                flush = true;
            }
        }
    }
    if (!flush) {
        return;
    }

    std::vector<market_product_t> result;
    random_select_product_from_market_default(MARKET_TYPE_NIGHT_RAID, result);

    player->night_raid_shop_items->clear();
    FOREACH(result, iter) {
        uint32_t product_id = (*iter).shop_id;
        std::map<uint32_t, ol_market_item_t>::iterator iter2 = 
           player->night_raid_shop_items->find(product_id);
        if(iter2 != player->night_raid_shop_items->end()) {
            ol_market_item_t *item = &(iter2->second);
            item->count = item->count + 1;
        } else {
            ol_market_item_t item;
            item.item_id = product_id;
            item.count = 1;
            player->night_raid_shop_items->insert(
                    std::pair<uint32_t, ol_market_item_t>(product_id, item));
        }
    }

    onlineproto::sc_0x0131_shop_refresh noti;
    noti.set_market(onlineproto::MARKET_TYPE_NIGHT_RAID);
    commonproto::market_item_info_t *item_info = noti.mutable_item_info();
    FOREACH(*(player->night_raid_shop_items), iter) {
        commonproto::market_item_t *item = item_info->add_items();
        item->set_item_id(iter->second.item_id);
        item->set_count(iter->second.count);
    }
    send_msg_to_player(player, cli_cmd_cs_0x0131_shop_refresh, noti);

    // 同步到db
    PlayerUtils::update_user_raw_data(
            player->userid, player->create_tm, 
            dbproto::NIGHT_RAID_SHOP_PRODUCT, noti.item_info(), "0");
    
    SET_A(kAttrNightRaidShopLastRefreshTm, NOW());
    return;
}

void try_flush_exped_shop(player_t *player, bool immediatly)
{
    bool flush = false;
    if (immediatly) {//立即刷新
        flush = true;
    } else {
        uint32_t time1 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time1", 1200);
        uint32_t time2 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time2", 1800);

        uint32_t now_hm = TimeUtils::second_trans_hm(NOW());
        uint32_t last_refresh_tm = GET_A(kAttrExpedShopLastRefreshTm);
        uint32_t last_refresh_hm = TimeUtils::second_trans_hm(last_refresh_tm);
        if (last_refresh_tm == 0) {
            flush = true;
        } else if (!TimeUtils::is_same_day(last_refresh_tm, NOW())) {
            if (now_hm >= time1) {
                flush = true;
            }
        } else {//当天
            if (last_refresh_hm < time1 && now_hm >= time1) {
                flush = true;
            } else if (last_refresh_hm > time1 && last_refresh_hm < time2 && now_hm >= time2) {
                flush = true;
            }
        }
    }
    if (!flush) {
        return;
    }

    std::vector<market_product_t> result;
    random_select_product_from_market_default(MARKET_TYPE_EXPED, result);

    player->exped_shop_items->clear();
    FOREACH(result, iter) {
        uint32_t product_id = (*iter).shop_id;
        std::map<uint32_t, ol_market_item_t>::iterator iter2 = 
           player->exped_shop_items->find(product_id);
        if(iter2 != player->exped_shop_items->end()) {
            ol_market_item_t *item = &(iter2->second);
            item->count = item->count + 1;
        } else {
            ol_market_item_t item;
            item.item_id = product_id;
            item.count = 1;
            player->exped_shop_items->insert(
                    std::pair<uint32_t, ol_market_item_t>(product_id, item));
        }
    }

    onlineproto::sc_0x0131_shop_refresh noti;
    noti.set_market(onlineproto::MARKET_TYPE_EXPED);
    commonproto::market_item_info_t *item_info = noti.mutable_item_info();
    FOREACH(*(player->exped_shop_items), iter) {
        commonproto::market_item_t *item = item_info->add_items();
        item->set_item_id(iter->second.item_id);
        item->set_count(iter->second.count);
    }
    send_msg_to_player(player, cli_cmd_cs_0x0131_shop_refresh, noti);

    // 同步到db
    PlayerUtils::update_user_raw_data(
            player->userid, player->create_tm, 
            dbproto::EXPED_SHOP_PRODUCT, noti.item_info(), "0");
    
    SET_A(kAttrExpedShopLastRefreshTm, NOW());
    return;
}

// 刷新家族商店
void try_flush_family_shop(player_t *player, bool immediatly)
{
    bool flush = false;
    if (immediatly) {//立即刷新
        flush = true;
    } else {
        uint32_t time1 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time1", 1200);
        uint32_t time2 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
                "time2", 1800);

        uint32_t now_hm = TimeUtils::second_trans_hm(NOW());
        uint32_t last_refresh_tm = GET_A(kAttrFamilyShopLastRefreshTm);
        uint32_t last_refresh_hm = TimeUtils::second_trans_hm(last_refresh_tm);
        if (last_refresh_tm == 0) {
            flush = true;
        } else if (!TimeUtils::is_same_day(last_refresh_tm, NOW())) {
            if (now_hm >= time1) {
                flush = true;
            }
        } else {//当天
            if (last_refresh_hm < time1 && now_hm >= time1) {
                flush = true;
            } else if (last_refresh_hm > time1 && last_refresh_hm < time2 && now_hm >= time2) {
                flush = true;
            }
        }
    }
    if (!flush) {
        return;
    }

    std::vector<market_product_t> result;
    random_select_product_from_market_default(MARKET_TYPE_FAMILY, result);

    player->family_shop_items->clear();
    FOREACH(result, iter) {
        uint32_t product_id = (*iter).shop_id;
        std::map<uint32_t, ol_market_item_t>::iterator iter2 = 
           player->family_shop_items->find(product_id);
        if(iter2 != player->family_shop_items->end()) {
            ol_market_item_t *item = &(iter2->second);
            item->count = item->count + 1;
        } else {
            ol_market_item_t item;
            item.item_id = product_id;
            item.count = 1;
            player->family_shop_items->insert(
                    std::pair<uint32_t, ol_market_item_t>(product_id, item));
        }
    }

    onlineproto::sc_0x0131_shop_refresh noti;
    noti.set_market(onlineproto::MARKET_TYPE_FAMILY);
    commonproto::market_item_info_t *item_info = noti.mutable_item_info();
    FOREACH(*(player->family_shop_items), iter) {
        commonproto::market_item_t *item = item_info->add_items();
        item->set_item_id(iter->second.item_id);
        item->set_count(iter->second.count);
    }
    send_msg_to_player(player, cli_cmd_cs_0x0131_shop_refresh, noti);

    // 同步到db
    PlayerUtils::update_user_raw_data(
            player->userid, player->create_tm, 
            dbproto::FAMILY_SHOP_PRODUCT, noti.item_info(), "0");
    
    SET_A(kAttrFamilyShopLastRefreshTm, NOW());
    return;
}
#endif

//尝试设置商店定时器 (每天12点和18点)
void try_setup_shop_timer(player_t *player)
{
    uint32_t now_hm = TimeUtils::second_trans_hm(NOW());
    uint32_t to_hm = 0;
    uint32_t time1 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
            "time1", 1200);
    uint32_t time2 = g_module_mgr.get_module_conf_uint32_def(module_type_daily_shop,
            "time2", 1800);

    if (now_hm >= time1 && now_hm < time2) {
        to_hm = time2;
    } else {
        to_hm = time1;
    }
    player->shop_flush_timer = ADD_TIMER_EVENT_EX(
            player,
            kTimerTypeShopFlush,
            (void*)(player->userid),
            NOW() + TimeUtils::second_to_hm(to_hm));
}

//为玩家初始化每日数据(登录时调用/在线跨天时调用)
void try_init_player_daily_data(player_t *player)
{
    // 清理每日属性
    AttrUtils::reset_daily_attr_value(player, true);
    // 清理每周属性
    AttrUtils::reset_weekly_attr_value(player, true);
    // 清理每月属性
    AttrUtils::reset_monthly_attr_value(player, true);
	//
	DupUtils::clean_mayin_defeat_empire_dup(player);
    // 清理每日悬赏任务
    TaskUtils::reset_daily_reward_task(player);
    // 清理有时间限制任务
    TaskUtils::reset_time_limit_task(player);

    // 重新设置每日最后一次登录时间
    SET_A(kDailyLastLoginTime, NOW());
    // 更新元素挑战副本当前属性类型
    AttrUtils::update_element_dup_attr_type(player);
    // 清理小屋碎片
	home_data_t::clean_hm_fragment_info(player);
	//处理霸者领域（属性试练）的每日数据(赠送入场券)
	DupUtils::deal_with_trail_dup_after_login(player);
	
	MineUtils::daily_reset_mine_system_related_info(player);

	//检查上次签到是否在本轮活动范围内
	clean_summer_signed_total_times(player);

	DupUtils::clean_daily_activity_dup(player);

}

// 是否是本次活动的年费vip
bool is_this_activity_year_vip(const player_t* player)
{
	if (Utils::is_activity_open(module_type_gold_vip_reset, 
			GET_A(kAttrThisActivityGoldVipStartTime))) {
		return true;
	}
	return false;
}

uint32_t deal_with_year_vip_state_when_login(player_t* player)
{
	if (GET_A(kAttrIsYearlyVip)) {
		if (!is_this_activity_year_vip(player)) {
			SET_A(kAttrIsYearlyVip, 0);
			SET_A(kAttrGoldVipRechargeCnt, 0);
			SET_A(kAttrThisActivityGoldVipStartTime, 0);
			if (GET_A(kAttrVipGiftBagGetFlag) == commonproto::VIP_GIFT_CAN_GET) {
				//TODO kevin年费礼包重置，以邮件形式将礼包发给玩家
				const uint32_t prize_id = 2200;
				std::vector<cache_prize_elem_t> prize_vec;
				prize_vec.clear();
				transaction_pack_prize(player, prize_id, prize_vec, NO_ADDICT_DETEC);
				if (prize_vec.size()) {
					new_mail_t new_mail;
					new_mail.sender.assign("系统邮件");
					new_mail.title.assign("累计礼包");
					new_mail.content.assign("黄金勋章180天活动充值，领取累计礼包");
					std::string attachment;
					MailUtils::serialize_prize_to_attach_string(prize_vec, attachment);
					new_mail.attachment = attachment;
					MailUtils::add_player_new_mail(player, new_mail);
				}
			}
			SET_A(kAttrVipGiftBagGetFlag, commonproto::VIP_GIFT_CAN_NOT_GET);
		}
	}
	return 0;
}

uint32_t clean_summer_signed_total_times(player_t* player)
{
	/*
    uint32_t last_date = TimeUtils::time_to_date(GET_A(kAttrLastLoginTm));
    if (last_date == TimeUtils::get_today_date()) {
		return 0;
	}
	*/
	uint32_t last_sign_tm = GET_A(kAttrSummerLastSignTimestamp);
	//检查上次签到的时间，与本次登录的时间是否属于同一轮签到时间范围内
	uint32_t key = TM_CONF_KEY_SUMMER_WEEKLY_SIGN;
	if (g_time_config.count(key) == 0) {
		return cli_err_sys_err;
	}
	TIME_CONFIG_LIMIT_T& time_config_map = g_time_config.find(key)->second;
	bool is_the_same_round = false;
	for (uint32_t i = 1; i <= time_config_map.size(); ++i) {
		if (TimeUtils::is_time_valid(last_sign_tm, key, i) &&
				TimeUtils::is_time_valid(NOW(), key, i)) {
			is_the_same_round = true;
			break;
		}
	}
	if (is_the_same_round == false) {
		//若为false,则说明上次签到时间与现在时间不在同一活动时间范围内
		//清零一轮签到的总次数
		TRACE_TLOG("Clear Summer Signed Total Times:last_tm=[%u],NOW=[%u]", last_sign_tm, NOW());
		SET_A(kAttrSummerHasSignedTotalTimes, 0);
	}
	return 0;
}
