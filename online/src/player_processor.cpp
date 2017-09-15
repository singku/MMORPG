extern "C" {
#include <libtaomee/tm_dirty/utf8_punc.h>
}
#include <boost/lexical_cast.hpp>
#include "player_processor.h"
#include "utils.h"
#include "player.h"
#include "attr_utils.h"
#include "item.h"
#include "pet.h"
#include "pet_utils.h"
#include "rune_utils.h"
#include "task_utils.h"
#include "player_utils.h"
#include "prize.h"
#include "tran_card.h"
#include "global_data.h"
#include "macro_utils.h"
#include "map_utils.h"
#include "service.h"
#include "auth.h"
#include "rank_utils.h"
#include "proto/db/dbproto.clisoft.pb.h"

int RequireServerTimeCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;

    uint64_t msec = get_now_tv()->tv_sec * 1000 + get_now_tv()->tv_usec / 1000;
    cli_out_.set_msec(msec);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int RTTTestCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;

    cli_out_.set_msec(cli_in_.msec());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int HeartBeatCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    return send_buff_to_player(player, player->cli_wait_cmd, 0, 0);
}

int CliSetAttrCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    if (!AttrUtils::is_valid_cli_attr(cli_in_.type())) {
        return send_err_to_player(player, player->cli_wait_cmd, 
                cli_err_invalid_cli_attr);
    }
    SET_A((attr_type_t)cli_in_.type(), cli_in_.value());
    return send_buff_to_player(player, player->cli_wait_cmd,
            0, 0);
}

int CliGetAttrListCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    db_in_.Clear();

    for (int i = 0; i < cli_in_.type_list_size();i++) {
        if (!AttrUtils::is_valid_cli_get_attr(cli_in_.type_list(i))) {
            return send_err_to_player(player, player->cli_wait_cmd, 
                    cli_err_invalid_cli_attr);
        }
        
        db_in_.add_type_list(cli_in_.type_list(i));  
    }
    return g_dbproxy->send_msg(
			player, cli_in_.user_id(), 
			cli_in_.create_tm(), 
			db_cmd_get_attr, db_in_);
}

int CliGetAttrListCmdProcessor::proc_pkg_from_serv(
        player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);
    cli_out_.mutable_attr_list()->CopyFrom(db_out_.attrs());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int GetOtherPlayerDataCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

	//缓存信息
	player->temp_info.other_userid = cli_in_.user_id();
	player->temp_info.other_create_tm = cli_in_.u_create_tm();
	player->temp_info.get_other_inf_reason = cli_in_.reason();

    if (!is_valid_uid(cli_in_.user_id())) {
        return send_err_to_player(player, 
                player->cli_wait_cmd, cli_err_userid_not_find);
    }
    cache_in_.Clear();
	commonproto::role_info_t* pb_ptr = cache_in_.add_roles();
	pb_ptr->set_userid(cli_in_.user_id());
	pb_ptr->set_u_create_tm(cli_in_.u_create_tm());
    return g_dbproxy->send_msg(player, cli_in_.user_id(), cli_in_.u_create_tm(),
            cache_cmd_ol_req_users_info, cache_in_);
}

int GetOtherPlayerDataCmdProcessor::proc_pkg_from_serv(
        player_t* player, const char* body, int bodylen)
{
	switch(player->serv_cmd) {
	case cache_cmd_ol_req_users_info:
		return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
	case db_cmd_family_get_info:
		return proc_pkg_from_serv_aft_get_family_info(player, body, bodylen);
	case ranking_cmd_get_user_multi_rank:
		return proc_pkg_from_serv_aft_get_arena_rank(player, body, bodylen);
	}
	return 0;
}
int GetOtherPlayerDataCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
        player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(cache_out_);
	uint32_t userid = player->temp_info.other_userid;
	uint32_t create_tm = player->temp_info.other_create_tm;

	if (cache_out_.user_infos_size() == 0) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_user_info_not_found);
	}
	cli_out_.mutable_info()->CopyFrom(cache_out_.user_infos(0));
	onlineproto::get_other_player_info_reason_t reason;
	reason = (onlineproto::get_other_player_info_reason_t)player->temp_info.get_other_inf_reason;
	cli_out_.set_reason(reason);
    CACHE_OUT_MSG(cli_out_);
	uint32_t family_id = cache_out_.user_infos(0).base_info().family_id();

	if(0 != family_id){
		//拉家族
		dbproto::cs_family_get_info db_family_info_in_;
		db_family_info_in_.Clear();
		return g_dbproxy->send_msg(
				player, family_id, player->create_tm, 
				db_cmd_family_get_info, db_family_info_in_);
	} else {
		//拉取排行榜
		//家族名字缓存写空
		char * family_name = player->session;
		uint32_t len = commonproto::FAMILY_NAME_MAX_LEN;
		memset(family_name,0, len); 

		std::vector<rank_key_order_t> rank_vec;
		struct rank_key_order_t tmp;
		tmp.key.key = commonproto::RANKING_ARENA;
		tmp.key.sub_key = 0;
        tmp.order = commonproto::RANKING_ORDER_ASC;
		rank_vec.push_back(tmp);

		return RankUtils::get_user_rank_info_by_keys(
				player, rank_vec, userid, create_tm);
	}
}

int GetOtherPlayerDataCmdProcessor::proc_pkg_from_serv_aft_get_family_info(
        player_t* player, const char* body, int bodylen)
{
    dbproto::sc_family_get_info     db_family_info_out_;
    PARSE_SVR_MSG(db_family_info_out_);

	uint32_t userid = player->temp_info.other_userid;
	uint32_t create_tm = player->temp_info.other_create_tm;
    // 家族不存在
    if (!db_family_info_out_.mutable_family_info()->has_family_id()) {
		//影响用户体验 不返错误码
		ERROR_TLOG("can not get family id! userid = %u , create_tm = %u",userid, create_tm);
    } else {
		char * family_name = player->session;
		uint32_t len = commonproto::FAMILY_NAME_MAX_LEN;
		memset(family_name,0, len); 
		strncpy(family_name, db_family_info_out_.family_info().family_name().c_str(), len);
	}
	//拉取排行榜
	std::vector<rank_key_order_t> rank_vec;
	struct rank_key_order_t tmp;
	tmp.key.key = commonproto::RANKING_ARENA;
	tmp.key.sub_key = 0;
    tmp.order = commonproto::RANKING_ORDER_ASC;
	rank_vec.push_back(tmp);
	
	return RankUtils::get_user_rank_info_by_keys(
			player, rank_vec, userid, create_tm);
}

int GetOtherPlayerDataCmdProcessor::proc_pkg_from_serv_aft_get_arena_rank(
        player_t* player, const char* body, int bodylen)
{
	rankproto::sc_get_user_multi_rank  multi_rank_out_;
	PARSE_SVR_MSG(multi_rank_out_);
	uint32_t userid = player->temp_info.other_userid;
	uint32_t create_tm = player->temp_info.other_create_tm;
	uint32_t rank = 0;

	if(0 == multi_rank_out_.rank_info_size()){
		//影响用户体验 不返错误码
		ERROR_TLOG("can not get arena rank! userid = %u , create_tm = %u",userid, create_tm);
	} else {
		const commonproto::rank_player_info_t& inf = multi_rank_out_.rank_info(0);
		rank = inf.rank();
	}
    onlineproto::sc_0x013A_get_other_player_info cli_out_;
	PARSE_OUT_MSG(cli_out_);
	commonproto::battle_player_data_t * inf = cli_out_.mutable_info();
	inf->set_arena_rank(rank);
	uint32_t len = commonproto::FAMILY_NAME_MAX_LEN;
	char *family_name = player->session;
	char *name = new char[len];
	strncpy(name, family_name, len); 
	inf->mutable_base_info()->set_family_name(name);
	delete[] name;
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int GetOtherPlayerHeadCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    cache_in_.Clear();
	if (0 == cli_in_.roles_size()){
		cli_out_.Clear();
		return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	}
    for (int i = 0; i < cli_in_.roles_size(); i++) {
        if (!is_valid_uid(cli_in_.roles(i).userid())) {
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_uid_invalid);
        }
        cache_in_.add_keys(PlayerUtils::make_head_cache_key(cli_in_.roles(i).userid(), 
                    cli_in_.roles(i).u_create_tm()));
    }
    if (cache_in_.keys_size() == 0) {
        cli_out_.Clear();
        return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    }
    return g_dbproxy->send_msg(
			player, player->userid,
			player->create_tm, 
			cache_cmd_get_cache, cache_in_);
}

int GetOtherPlayerHeadCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(cache_out_);
    cli_out_.Clear();
    for (int i = 0; i < cache_out_.key_values_size(); i++) {
        const cacheproto::cache_key_value_t &info = cache_out_.key_values(i);
        onlineproto::uid_head_item_t *new_info = cli_out_.add_infos();
        new_info->mutable_role()->set_userid(PlayerUtils::get_cache_uid_from_key(info.key()));
        new_info->mutable_role()->set_u_create_tm(PlayerUtils::get_cache_ucrtm_from_key(info.key()));
        new_info->set_head_item_id(atoi_safe(info.value().c_str()));
    }
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int BuyVpCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    
    uint32_t buy_cnt = GET_A(kDailyVpDayBuyCount);
    if (buy_cnt >= AttrUtils::get_attr_max_limit(player, kDailyVpDayBuyCount)) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_buy_vp_cnt_exceed);
    }

    //vip/svip首次购买免费
    int32_t need_diamond = 0;
    if (is_vip(player) && buy_cnt == 0) {
        need_diamond = 0;
    } else {
        // need_diamond = 20 + buy_cnt / 2 * 20;
        need_diamond = ceil((buy_cnt + 1)/ 2.0) * 10;
    }
    
    if (need_diamond) {//
        // const product_t *pd = g_product_mgr.find_product(40007);
        // int ret = player_chg_diamond_and_sync(player, -need_diamond, pd, 1, 
                // dbproto::CHANNEL_TYPE_BUY_REDUCE, "购买体力");
        attr_type_t attr_type = kServiceBuyVp;	
        const uint32_t product_id = 40007;	
        int ret = buy_attr_and_use(player, attr_type, product_id, need_diamond);
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd, ret);
        }
    }
    //加体力
    ADD_A(kAttrCurVp, 40);
    //加购买次数
    ADD_A(kDailyVpDayBuyCount, 1);

	//黑瞳送点心活动
	if(TimeUtils::is_current_time_valid(TM_CONF_KEY_KUROME_SEND_DESSERT, 1)){
		const uint32_t item_id  = 70506;
		const uint32_t item_cnt = 10;
		add_single_item(player, item_id, item_cnt);
	}

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int AlchemyCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    static int alchemy[9][2] = {
        {0,  800},
        {5,  1850},
        {10, 3550},
        {20, 6800},
        {30, 9750},
        {40, 12400},
        {50, 14750},
        {60, 16800},
        {70, 18550}
    };

    uint32_t cnt = GET_A(kDailyAlchemyTimes);
    if (cnt >= GET_A_MAX(kDailyAlchemyTimes)) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_alchemy_cnt_exceed);
    }
    int32_t need_diamond = alchemy[cnt][0];
    int32_t produce_gold = alchemy[cnt][1];

    if (need_diamond) {
        // const product_t *pd = g_product_mgr.find_product(90036);
        // int ret = player_chg_diamond_and_sync(player, 
                // -need_diamond, pd, 1, dbproto::CHANNEL_TYPE_BUY_REDUCE, "炼金");
        attr_type_t attr_type = kServiceBuyAlchemy;	
        const uint32_t product_id = 90036;	
        int ret = buy_attr_and_use(player, attr_type, product_id, need_diamond);
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd, ret);
        }
    }
    AttrUtils::add_player_gold(player, produce_gold, true, "金币宝藏");
    ADD_A(kDailyAlchemyTimes, 1);
	//黑瞳送点心活动
	if(TimeUtils::is_current_time_valid(TM_CONF_KEY_KUROME_SEND_DESSERT, 1)){
		const uint32_t item_id  = 70506;
		const uint32_t item_cnt = 10;
		add_single_item(player, item_id, item_cnt);
	}
    // 更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_GOLD_TREASURE, 1);

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FrontStatCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen) 
{
    PARSE_MSG;

    if (cli_in_.dir() == "") {
        cli_in_.set_dir("未命名");
    }
    
    if (unlikely(cli_in_.dir() == "客户端性能")) {
        dbproto::cs_set_client_soft db_msg;
        if (cli_in_.name() == "CapabilitiesVersion") {
            db_msg.set_soft1_v(cli_in_.sub_name());
        } else if (cli_in_.name() == "DriverInfo") {
            db_msg.set_soft2_v(cli_in_.sub_name());
        }
        g_dbproxy->send_msg(
				0, player->userid, 
				player->create_tm,
				db_cmd_set_clisoftv, db_msg);
    }

    Utils::write_msglog_new(player->userid, cli_in_.dir(), cli_in_.name(), cli_in_.sub_name());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int RandomNickCmdProcessor::proc_pkg_from_client(
        player_t *player, const char* body, int bodylen)
{
    PARSE_MSG;

    std::string nick = get_random_nick(GET_A(kAttrSex));
    if (nick.empty()) {
        ERROR_TLOG("P:%u rand nick list is empty", player->userid);
        RET_ERR(cli_err_sys_err);
    }

    nick_session_t* session = (nick_session_t *)player->session;
    memset(session, 0, sizeof(*session));
    strncpy(session->nick, nick.c_str(), sizeof(session->nick) - 1);
    db_in_.Clear();
    db_in_.set_nick(nick);

    return g_dbproxy->send_msg(
			player, player->userid,
			player->create_tm,
            db_cmd_get_userid_by_nick, db_in_);
}

int RandomNickCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);

    nick_session_t* session = (nick_session_t *)player->session;
    if (db_out_.userid() == 0) {
        cli_out_.Clear();
        cli_out_.set_nick(session->nick);
        RET_MSG;
    } else {
        std::string nick = get_random_nick(GET_A(kAttrSex)); 
        strncpy(session->nick, nick.c_str(), sizeof(session->nick));
        db_in_.Clear();
        db_in_.set_nick(nick);
        return g_dbproxy->send_msg(
				player, player->userid,
				player->create_tm,
                db_cmd_get_userid_by_nick, db_in_);
    }

    return 0; 
}

std::string RandomNickCmdProcessor::get_random_nick(uint32_t sex)
{
    if (g_rand_nick_pos1[0].size() == 0 || g_rand_nick_pos1[1].size() == 0
            || g_rand_nick_pos2.size() == 0 || g_rand_nick_pos3.size() == 0) {
        return "";
    }

    uint32_t cnt = 0;
    std::string nick;
    while (cnt < 5) {
        nick.clear();
        uint32_t index1 = rand() % g_rand_nick_pos2.size();
        uint32_t index2 = rand() % g_rand_nick_pos3.size(); 

        nick += g_rand_nick_pos3[index2];

        if (index1 % 2) {
            nick += g_rand_nick_pos2[index1];
        }

        if (sex == 0) {
            uint32_t index = rand() % g_rand_nick_pos1[0].size();
            nick += g_rand_nick_pos1[0][index];
        } else {
            uint32_t index = rand() % g_rand_nick_pos1[1].size(); 
            nick += g_rand_nick_pos1[1][index];
        }

        int ret = tm_dirty_check(0, (char*)nick.c_str());
        if (ret == 0) {
            break;
        } else {
            char tmp_nick[128] = {0};
            memcpy(tmp_nick, nick.c_str(), strlen(nick.c_str()));
            tm_dirty_replace(tmp_nick);
            ERROR_TLOG("Find Dirty Nick:%s[%s]", nick.c_str(), tmp_nick);
            cnt ++;
        }
    }

    return nick;
}

const uint32_t name_card_id = 38001;
int SetNickCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    if (GET_A(kAttrNameSet) && player->package->get_total_usable_item_count(name_card_id) == 0) {
        RET_ERR(cli_err_no_way_change_nick);
    }

    if (likely(g_test_for_robot == 0)) {
        int ret = Utils::check_dirty_name(cli_in_.nick());
        if (ret) {
            RET_ERR(ret);
        }
    }

    nick_session_t* session = (nick_session_t *)player->session;
    memset(session, 0, sizeof(*session));
    strncpy(session->nick, cli_in_.nick().c_str(), sizeof(session->nick) - 1);
    db_insert_in_.Clear();
    db_insert_in_.set_nick(cli_in_.nick());
    return g_dbproxy->send_msg(
			player, player->userid,
			player->create_tm,
			db_cmd_insert_nick_and_userid, db_insert_in_);
}

int SetNickCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_insert_out_);

    nick_session_t* session = (nick_session_t *)player->session;
    //nick插入成功 则删除老的数据
    db_delete_in_.set_nick(player->nick);
    g_dbproxy->send_msg(
			0, player->userid,
			player->create_tm,
			db_cmd_delete_nick_and_userid, db_delete_in_);
    db_change_in_.set_nick(session->nick);
    g_dbproxy->send_msg(
			0, player->userid,
			player->create_tm,
			db_cmd_change_nick, db_change_in_);

    if (GET_A(kAttrNameSet) == 0) {
        SET_A(kAttrNameSet, 1);
    } else {//扣改名卡
        reduce_single_item(player, name_card_id, 1);
    }
    STRCPY_SAFE(player->nick, session->nick);
    MapUtils::sync_map_player_info(player, commonproto::PLAYER_INFO_CHANGE);
    RET_MSG;
}

int GetSessionCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    if (!player->is_login) {
        RET_ERR(cli_err_player_not_login);
    }

    // TODO toby session check
    //temp_info_t* temp_info = &player->temp_info;
    //if (!Utils::is_cool_down(temp_info->last_get_session_time, 200)) {
        //RET_ERR(cli_err_get_session_too_fast);
    //}
    //temp_info->last_get_session_time = get_now_tv()->tv_sec;

    char send_buf[1024];
    chnlhash32_t* hash = (chnlhash32_t *)send_buf;
    act_add_session_req_t* req = (act_add_session_req_t *)(send_buf + sizeof(*hash));

    req->gameid = g_server_config.gameid;
    req->ip = player->fdsess->remote_ip;

    gen_chnlhash32(g_server_config.verifyid, g_server_config.security_code, 
            (const char*)req, sizeof(*req), hash);

    return g_dbproxy->send_to_act(player, player->userid,
            act_cmd_add_session, send_buf, sizeof(*req) + sizeof(*hash));
}

int GetSessionCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    act_add_session_ack_t* ack = (act_add_session_ack_t*)body;

    if (bodylen != sizeof(*ack)) {
        ERROR_TLOG("%u invalid add session body len %d, expect %u",
                player->userid, bodylen, sizeof(*ack)); 
        RET_ERR(cli_err_sys_err);
    }

    char session_hex[33] = {0};
    bin2hex_frm(session_hex, ack->session,
            sizeof(ack->session), 0);

    cli_out_.Clear();
    cli_out_.set_session(session_hex);

    RET_MSG;
}

int GetOnlineListCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    sw_in_.Clear();
    sw_in_.set_server_type(commonproto::SERVER_TYPE_ONLINE);
    sw_in_.set_idc_zone(g_server_config.idc_zone);
    sw_in_.set_svr_recommend(cli_in_.svr_recommend());
    sw_in_.set_my_server_id(g_server_id);
    return g_switch->send_msg(player, g_online_id, 0, sw_cmd_get_server_list, sw_in_);
}

int GetOnlineListCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(sw_out_);
    cli_out_.Clear();
    cli_out_.mutable_server_list()->CopyFrom(sw_out_.server_list());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int GenInviteCodeCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
#if 0
    //singku测试内网充值
    struct platform_recharge_diamond_req_t {
        uint32_t dest_user; //充值目标用户米米号
        uint32_t dest_server_id; //区服号
        uint32_t type;   //消费类型
        uint32_t diamond_cnt;  //充值增加的钻石数量
        uint32_t trans_id; //充值交易号
        uint32_t dest_create_tm; // 用户的create_tm 
        uint32_t add_data_2; // 附加信息2 
    } __attribute__((packed));

    struct platform_recharge_diamond_atk_t {
        uint32_t trans_id; //充值交易号(原样返回)
    };

    for (int i = 0; i < 10000; i ++) {
        uint32_t users[] = {
            6031662, 50720, 6034610/*,100363,100365, 100387, 100390*/
        };
        uint32_t idx = i % (sizeof(users) / sizeof(uint32_t));
        platform_recharge_diamond_req_t charge_body;
        charge_body.dest_user = users[idx];
        charge_body.dest_server_id = 1;
        charge_body.type = 0;
        charge_body.diamond_cnt = 100;
        charge_body.trans_id = NOW() + i;
        charge_body.dest_create_tm = 0;
        charge_body.add_data_2 = 0;
        g_dbproxy->send_to_act(0, charge_body.dest_user, sw_cmd_sw_recharge_diamond, (char*)(&charge_body), sizeof(charge_body));
    }
#endif

    PARSE_MSG;
    uint64_t n = player->userid;
    n = n << 32;
    n |= player->create_tm;
    if (GET_A(kAttrInviteCodePart1)) {
        cli_out_.set_code(Utils::to_string(n));
        RET_MSG;
    }
    SET_A(kAttrInviteCodePart1, player->userid);
    SET_A(kAttrInviteCodePart2, player->create_tm);
    cli_out_.set_code(Utils::to_string(n));
    RET_MSG;
}

int SignInviteCodeCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    if (GET_A(kAttrSignInviteCode)) {
        RET_ERR(cli_err_already_sign_invite_code);
    }
    uint64_t code = strtoul(cli_in_.code().c_str(), 0, 10);
    uint32_t create_tm = code;
    uint32_t uid = code >> 32;
    if (!is_valid_uid(uid)) {
        RET_ERR(cli_err_invalid_invite_code);
    }
    if (uid == player->userid) {
        RET_ERR(cli_err_can_not_be_self);
    }
    dbproto::cs_check_user_exist db_in;
    db_in.set_server_id(0);
    db_in.set_is_init_server(false);
    return g_dbproxy->send_msg(player, uid, create_tm, db_cmd_check_user_exist, db_in);
}

int SignInviteCodeCmdProcessor::proc_pkg_from_serv(player_t *player, const char *body, int bodylen)
{
    switch(player->serv_cmd) {
    case db_cmd_check_user_exist:
        return proc_pkg_from_serv_aft_check_user_exist(player, body, bodylen);
    case db_cmd_get_attr:
        return proc_pkg_from_serv_aft_get_invite_code_info(player, body, bodylen);
    default:
        RET_MSG;
    }
}

int SignInviteCodeCmdProcessor::proc_pkg_from_serv_aft_check_user_exist(
        player_t *player, const char *body, int bodylen)
{
    dbproto::sc_check_user_exist db_out;
    PARSE_SVR_MSG(db_out);
    if (db_out.exist() == false) {
        RET_ERR(cli_err_invalid_invite_code);
    }

    dbproto::cs_get_attr db_in;
    db_in.add_type_list(kAttrInviteCodePart1);
    db_in.add_type_list(kAttrInviteCodePart2);
    uint64_t code = strtoul(cli_in_.code().c_str(), 0, 10);
    uint32_t create_tm = code;
    uint32_t uid = code >> 32;
    return g_dbproxy->send_msg(player, uid, create_tm, db_cmd_get_attr, db_in);
}

int SignInviteCodeCmdProcessor::proc_pkg_from_serv_aft_get_invite_code_info(
        player_t *player, const char *body, int bodylen)
{
    dbproto::sc_get_attr db_out;
    PARSE_SVR_MSG(db_out);

    if (db_out.attrs_size() != 2) {
        RET_ERR(cli_err_sys_err);
    }
    const commonproto::attr_data_t &attr1 = db_out.attrs(0);
    const commonproto::attr_data_t &attr2 = db_out.attrs(1);
    uint32_t part1 = attr1.value();
    
    if (part1 == 0) {//还没有产生邀请码
        RET_ERR(cli_err_invalid_invite_code);
    }

    uint32_t part2 = attr2.value();
    uint64_t n = part1;
    n = n << 32;
    n |= part2;
    if (n != strtoul(cli_in_.code().c_str(), 0, 10)) {
        RET_ERR(cli_err_invalid_invite_code);
    }

    //邀请码对了 设置已填写邀请码的时间
    SET_A(kAttrSignInviteCode, NOW());
    SET_A(kAttrInviterUid, part1);
    SET_A(kAttrInviterCreateTm, part2);
    //给对方加1
    if (GET_A(kAttrLv) >= 20) {
        AttrUtils::change_other_attr_value_pub(GET_A(kAttrInviterUid), 
                GET_A(kAttrInviterCreateTm), kAttrInvitedPlayers, 1, false);
        SET_A(kAttrInviterAddTm, NOW());
    }
    RET_MSG;
}

int GetChargeDiamondDrawCardsCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    std::vector<cache_prize_elem_t> result;
    cli_out_.Clear();

    //为空 尝试去DB拉取
    if (player->daily_charge_diamond_draw_cards_info->empty()) {
        return PlayerUtils::get_user_raw_data(player, 
                dbproto::CHARGE_DIAMOND_DRAW_PRIZE_ACTIVITY);
    }

    cache_prize_to_proto_prize(*(player->daily_charge_diamond_draw_cards_info),
            *(cli_out_.mutable_cards()));
    RET_MSG;
}

int GetChargeDiamondDrawCardsCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);
    cli_out_.Clear();
    commonproto::prize_elem_list_t pb_msg;
    if (db_out_.raw_data().size() == 0 || !pb_msg.ParseFromString(db_out_.raw_data())) {
        std::vector<cache_prize_elem_t> result;
        refresh_player_charge_diamond_draw_prize_info(player, result);

    } else {
        for (int i = 0; i < pb_msg.prize_list_size(); i++) {
            const commonproto::prize_elem_t &inf = pb_msg.prize_list(i);
            cache_prize_elem_t tmp;
            tmp.type = inf.type();
            tmp.id = inf.id();
            tmp.count = inf.count();
            tmp.level = inf.level();
            tmp.talent_level = inf.talent_level();
            player->daily_charge_diamond_draw_cards_info->push_back(tmp);
        }
    }
    cache_prize_to_proto_prize(*(player->daily_charge_diamond_draw_cards_info),
            *(cli_out_.mutable_cards()));
    RET_MSG;
}

int RefreshChargeDiamondDrawCardsCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    cli_out_.Clear();
    int ret = buy_attr_and_use(player, kServiceBuyChargeDiamondDrawCardsRefresh,
            90044, 1);
    if (ret) {
        RET_ERR(ret);
    }
    std::vector<cache_prize_elem_t> result;
    refresh_player_charge_diamond_draw_prize_info(player, result);
    cache_prize_to_proto_prize(*(player->daily_charge_diamond_draw_cards_info),
            *(cli_out_.mutable_cards()));
    RET_MSG;
}

int DrawChargeDiamondCardCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    //判断每日免费次数
    bool has_free_cnt = false;
    if (GET_A(kDailyChargeDiamondDrawDailyCnt) > 0) {
        has_free_cnt = true;
    //非免费次数
    } else {
        uint32_t charge_cnt = GET_A(kAttrChargeDiamondDrawPrizeChargeCnt) / 100;
        uint32_t draw_cnt = GET_A(kAttrChargeDiamondDrawPrizeDrawCnt);
        if (draw_cnt >= charge_cnt) {
            RET_ERR(cli_err_can_not_get_prize_yet);
        }
    }
    std::map<uint32_t, uint32_t> nr;
    uint32_t idx = 0;
    FOREACH((*(player->daily_charge_diamond_draw_cards_info)), it) {
        const cache_prize_elem_t &elem = *it;
        if (elem.count != 0) {
            nr[idx] = elem.pow;
        }
        idx++;
    }
    if (nr.empty()) {
        RET_ERR(cli_err_all_prize_get);
    }
    std::set<uint32_t> hit_idx;
    Utils::rand_select_uniq_m_from_n_with_r(nr, hit_idx, 1);
    idx = *(hit_idx.begin());

    cache_prize_elem_t *elem = &((*player->daily_charge_diamond_draw_cards_info)[idx]);
    if (elem->count == 0) {
        RET_ERR(cli_err_sys_err);
    }
    std::vector<cache_prize_elem_t> award_elems;
    award_elems.push_back(*elem);
    onlineproto::sc_0x0112_notify_get_prize noti_msg;
    int ret = transaction_proc_packed_prize(player, award_elems, &noti_msg, 
            commonproto::PRIZE_REASON_CHARGE_DIAMOND_DRAW_CARD, "充钻抽卡奖励");
    if (ret) {
        RET_ERR(ret);
    }

    //更新DB
    elem->count = 0;
    commonproto::prize_elem_list_t pb_msg;
    cache_prize_to_proto_prize((*player->daily_charge_diamond_draw_cards_info), 
            *(pb_msg.mutable_prize_list()));
    PlayerUtils::update_user_raw_data(player->userid, player->create_tm, 
            dbproto::CHARGE_DIAMOND_DRAW_PRIZE_ACTIVITY, pb_msg, "0");

    //扣次数
    if (has_free_cnt) {
        SUB_A(kDailyChargeDiamondDrawDailyCnt, 1);
        Utils::write_msglog_new(player->userid, "收费", "全民乐翻天", "使用免费抽奖");
    } else {
        ADD_A(kAttrChargeDiamondDrawPrizeDrawCnt, 1);
        string tmp = "使用付费抽奖" + Utils::to_string(GET_A(kAttrChargeDiamondDrawPrizeDrawCnt)) + "次";
        Utils::write_msglog_new(player->userid, "收费", "全民乐翻天", "使用付费抽奖");
        Utils::write_msglog_new(player->userid, "收费", "全民乐翻天", tmp);

    }
    pb_msg.Clear();
    cache_prize_to_proto_prize(award_elems, *(pb_msg.mutable_prize_list()));
    cli_out_.mutable_card()->CopyFrom(pb_msg.prize_list(0));
    send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_msg);
    RET_MSG;   
}

int MayinSendFlowerCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
	const uint32_t TARGET_NUM = 520;
	uint32_t count = 0;
	uint32_t product_id = 0;
	if (cli_in_.type() == onlineproto::SEND_1_FLOWERS) {
		uint32_t item_id = 70501;
		count = player->package->get_total_item_count(item_id);
		uint32_t ret = reduce_single_item(player, item_id, count);
		if (ret) {
			RET_ERR(cli_err_lack_usable_item);
		}
	} else if (cli_in_.type() == onlineproto::SEND_9_FLOWERS) {
		product_id = 90050;
		count = 9;
		int ret = buy_attr_and_use(player, kServiceBuyMayinFlower, product_id, 1);
		if (ret) {
			RET_ERR(ret);
		}
	} else if (cli_in_.type() == onlineproto::SEND_99_FLOWERS) {
		product_id = 90061;
		count = 99;
		int ret = buy_attr_and_use(player, kServiceBuyMayinFlower, product_id, 1);
		if (ret) {
			RET_ERR(ret);
		}
	}
	uint32_t old_count = GET_A(kAttrSendMayinFlowerCount);
	ADD_A(kAttrSendMayinFlowerCount, count);
	uint32_t new_count = GET_A(kAttrSendMayinFlowerCount);
	if (old_count < TARGET_NUM && new_count >= TARGET_NUM) {
		static char msg[256];
		snprintf(msg, sizeof(msg), "恭喜[pi=%u|%u]%s[/pi]送出520朵鲜花，获得[cl=0xe36c0a]真爱大礼包[/cl]![op=%u|%u]我也要参加[/op]",
				player->userid, player->create_tm, player->nick, player->userid,
				116);
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
	}
	RankUtils::rank_user_insert_score(
		player->userid, player->create_tm,
		commonproto::RANKING_MAYIN_FLOWER, 0,
		GET_A(kAttrSendMayinFlowerCount));
	//获取自己的名次
	std::vector<role_info_t> user_ids;
	role_info_t role_info;
	role_info.userid = player->userid;
	role_info.u_create_tm = player->create_tm;
	user_ids.push_back(role_info);
	RankUtils::get_user_rank_info(player, commonproto::RANKING_MAYIN_FLOWER, 
			0, user_ids, commonproto::RANKING_ORDER_DESC);
	return 0;
}

int MayinSendFlowerCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(rank_out_);
	if (rank_out_.rank_info_size()) {
		const commonproto::rank_player_info_t &inf = rank_out_.rank_info(0);
		//如果玩家到了第一名，则系统广播
		if (inf.rank() == 1) {
			static char msg[256];
			snprintf(msg, sizeof(msg), "恭喜[pi=%u|%u]%s[/pi]成为鲜花排行榜[cl=0xe36c0a]第一名[/cl]![op=%u|%u]去瞧瞧热闹[/op]",
				player->userid, player->create_tm, player->nick, player->userid,
				116);
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
		}
	}
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int SurveyGetCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    string buff_id = Utils::to_string(cli_in_.type());
    return PlayerUtils::get_user_raw_data(player, dbproto::SURVEY_DATA, buff_id);
}

int SurveyGetCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);
    commonproto::survey_info_list_t survey_msg;
    survey_msg.ParseFromString(db_out_.raw_data());
    cli_out_.Clear();
    cli_out_.mutable_answer()->CopyFrom(survey_msg.survey_list());
    RET_MSG;
}

int SurveySubmitCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    string buff_id = Utils::to_string(cli_in_.type());
    return PlayerUtils::get_user_raw_data(player, dbproto::SURVEY_DATA, buff_id);
}

int SurveySubmitCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);
    commonproto::survey_info_list_t survey_msg;

    //没有答题 或者答题数据不合法 则允许重新答题
    if (db_out_.raw_data().size() == 0/* ||
        !survey_msg.ParseFromString(db_out_.raw_data())*/) {
        survey_msg.mutable_survey_list()->CopyFrom(cli_in_.answer());
        PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
                dbproto::SURVEY_DATA, survey_msg, Utils::to_string(cli_in_.type()));

    } else {
        RET_ERR(cli_err_answer_question_number_repeat);
    }

    RET_MSG;
}

//黑瞳送点心
int WeeklyRankingActivityCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
	const uint32_t activity_type = cli_in_.type();

	if(activity_type == commonproto::ACTIVITY_KUROME_SEND_DESSERT){
		if(!TimeUtils::is_current_time_valid(TM_CONF_KEY_KUROME_SEND_DESSERT, 1)){
			RET_ERR(cli_err_activity_time_invalid);
		}

		attr_type_t key[2] = {
			kDailyKuromeSendDessertTimes,
			kWeeklyKuromeSendDessertTimes};
		const uint32_t count    = cli_in_.item_cnt();
		const uint32_t item_id  = cli_in_.item_id();

		uint32_t ret = reduce_single_item(player, item_id, count);
		if (ret) {
			RET_ERR(cli_err_lack_usable_item);
		}
		ADD_A(key[0], count);
		ADD_A(key[1], count);

		//插入排行榜
		uint32_t sub_key = TimeUtils::get_start_time(
				TM_CONF_KEY_KUROME_SEND_DESSERT, 1);
		RankUtils::rank_user_insert_score(
				player->userid, player->create_tm,
				commonproto::RANKING_KUROME_SEND_DESSERT, sub_key,
				GET_A(key[1]));
	}

    RET_MSG;
}

int WeeklyRankingActivityCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
	return 0;
}

int FakeCmdForWeeklyActivityRankCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(user_rank_out_);

	//领奖标志key  1：黑瞳送点心活动 2:塔兹米送粽子 3:龙之试炼 4:须佐 5:红离
	attr_type_t flag = kAttrWeeklyActivityRankFlag;                    
	// const uint32_t activity_time_key[] = {TM_CONF_KEY_KUROME_SEND_DESSERT};
	uint32_t reward_flag = GET_A(flag);
	//发过奖励不处理
	if(taomee::test_bit_on(flag, 5)){
		return 0;
	}

	const commonproto::rank_player_info_t &player_info = user_rank_out_.rank_info(0);
	//解析ranking::key
	std::string str_rank_key = player_info.rank_key();
	std::vector<std::string> keys_vec = split(str_rank_key, ':');
	uint32_t key = 0, sub_key = 0;
	if (keys_vec.size() >= 2) {
		key = boost::lexical_cast<uint32_t>(keys_vec[0]);
		sub_key = boost::lexical_cast<uint32_t>(keys_vec[1]);
	}

	if (player->userid == player_info.userid() &&
			player->create_tm == player_info.u_create_tm()) {
		uint32_t rank = player_info.rank();
		uint32_t prize_id = 0;
		if(key == commonproto::RANKING_KUROME_SEND_DESSERT){
			if(rank == 1){
				prize_id = 11555;
			} else if (rank >= 2 && rank <= 3){
				prize_id = 11556;
			} else if (rank >= 4 && rank <= 6){
				prize_id = 11557;
			} else if (rank >= 7 && rank <= 10){
				prize_id = 11558;
			}

			if(prize_id != 0 ){
				std::string content = "恭喜您在红离の热情火焰活动中获得了第" + boost::lexical_cast<std::string>(rank) +
					"名，请领取奖励！";
				PlayerUtils::generate_new_mail(player, "获得红离の热情火焰活动奖励", content,
						prize_id);

				// reward_flag = taomee::set_bit_on(reward_flag,
						   // commonproto::ACTIVITY_LONG_SEND_SCALE);
				reward_flag = taomee::set_bit_on(reward_flag,
					   5);
				SET_A(flag, reward_flag);
			}
		} 
		return 0;
	} else {
		ERROR_TLOG("get weekly activity rank from serv err!"
				"uid=[%u] create_tm=[%u]", 
				player->userid, player->create_tm);
	}
	return 0;
}

