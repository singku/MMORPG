#ifndef LOGIN_PROCESSOR_H
#define LOGIN_PROCESSOR_H

#include "common.h"
#include "cmd_processor_interface.h"
#include "global_data.h"
#include "player.h"
#include "attr_utils.h"
#include "utils.h"
#include "service.h"
#include "proto.h"
#include "sys_ctrl.h"
#include "rank_utils.h"
#include "arena.h"

extern bool need_active_code;

class LoginCmdProcessor : public CmdProcessorInterface
{
public:

    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
    uint32_t proc_errno_from_serv(player_t* player, uint32_t ret);

private:

    int proc_pkg_from_serv_aft_check_session(
            player_t* player, const char* body, int bodylen);

    int proc_pkg_from_serv_aft_get_is_active(
            player_t* player, const char* body, int bodylen);

    int proc_pkg_from_serv_aft_get_login_info(
            player_t* player, const char* body, int bodylen);

    int proc_pkg_from_serv_aft_get_family_info(
            player_t* player, const char* body, int bodylen);
 
	//登录时领取竞技场每日排名的奖励
	int proc_pkg_from_serv_aft_get_arena_rank(
			player_t* player, const char* body, int bodylen);

    /*---------------------登录inline helper function-------------------------*/
    inline int login_steps_check_session_func(player_t *player)
    {
        act_check_session_req_t* req = (act_check_session_req_t *)(send_buf_);
        memset(req, 0, sizeof(*req));

        req->from_game = g_server_config.gameid;
        hex2bin(req->session, cli_in_.session().substr(0, 32).c_str(), 32);
        req->del_flag = 1;
        req->to_game = g_server_config.gameid;
        req->ip = player->fdsess->remote_ip;
        req->region = g_server_config.idc_zone;
        req->enter_game = 1;
        STRCPY_SAFE(req->tad, cli_in_.tad().c_str());
        login_session_t* session = (login_session_t *)player->session;
        memset(session, 0, sizeof(*session));
        STRCPY_SAFE(session->tad, cli_in_.tad().c_str());
        STRCPY_SAFE(session->browse, cli_in_.browse().c_str());
        STRCPY_SAFE(session->version, cli_in_.version().c_str());
        STRCPY_SAFE(session->device, cli_in_.device().c_str());
        STRCPY_SAFE(session->os, cli_in_.os().c_str());
        STRCPY_SAFE(session->resolution, cli_in_.resolution().c_str());
        STRCPY_SAFE(session->network, cli_in_.network().c_str());

        return g_dbproxy->send_to_act(player, player->userid, 
                act_cmd_check_session, 
                (const char*)req, sizeof(*req));
    }

    //目前是跳过检测激活
    inline int login_steps_get_active_func(player_t *player)
    {
        if (need_active_code) {
            act_is_active_req_t req;
            req.gameid = g_server_config.gameid;
            return g_dbproxy->send_to_act(player, player->userid, act_cmd_is_active, 
                    (const char *)&req, sizeof(req), 0);
        }
        set_login_step_finished(player, login_steps_get_active);
        return proc_login(player);
    }

    inline int login_steps_get_login_info_func(player_t *player)
    {
        return g_dbproxy->send_buf(player, player->userid, player->create_tm,
                db_cmd_get_login_info, NULL, 0);
    }

    inline int login_steps_get_family_info_func(player_t *player)
    {
        uint32_t family_id = GET_A(kAttrFamilyId);
        dbproto::cs_family_get_info     db_family_info_in_;
        db_family_info_in_.Clear();
        return g_dbproxy->send_msg(
                player, family_id, player->create_tm, 
                db_cmd_family_get_info, db_family_info_in_);
    }

	//策划将周排名，改为日排名，故废弃此函数
	inline int login_steps_get_arena_weekly_rank_func(player_t* player) 
	{
        uint32_t last_logout_tm = GET_A(kAttrLastLogoutTm);
        if (last_logout_tm == 0 //还没下过线(第一次登陆) 
            //上次登录时间大于上周五(竞技场结算点) 说明竞技场奖励在之前的登陆已经发放过了
            || GET_A(kAttrLastLoginTm) > TimeUtils::get_last_x_time(NOW(), 5)
            //不太可能的BUG
            || NOW() < last_logout_tm) {
            set_login_step_finished(player, login_steps_get_arena_weekly_rank);
            return proc_login(player);
        }
		std::vector<uint32_t> weekly_sub_keys;
		TimeUtils::get_n_x_date_between_timestamp(
				5, weekly_sub_keys, GET_A(kAttrLastLogoutTm), NOW(), 4);
		std::vector<rank_key_order_t> rank_vec;
		FOREACH(weekly_sub_keys, it) {
			struct rank_key_order_t tmp;
			tmp.key.key = commonproto::RANKING_ARENA;
			tmp.key.sub_key = *it;
            tmp.order = commonproto::RANKING_ORDER_ASC;
			rank_vec.push_back(tmp);
		}
		if (rank_vec.size()) {
			return RankUtils::get_user_rank_info_by_keys(
					player, rank_vec, 
					player->userid, player->create_tm);
		} else {
			set_login_step_finished(player, login_steps_get_arena_weekly_rank);
			return proc_login(player);
		}
	}

	inline int login_steps_get_arena_daily_rank_func(player_t* player)
	{
		//获取上一次领竞技场奖励的时间
		uint32_t last_get_reward_tm = GET_A(kAttrArenaLastGetRewardTm);

        if (last_get_reward_tm == 0) {//第一次登陆时必定为0,说明还没有参加过竞技场 无须获奖
            last_get_reward_tm = NOW();//初始化
            SET_A(kAttrArenaLastGetRewardTm, NOW());
			set_login_step_finished(player, login_steps_get_arena_daily_rank);
			return proc_login(player);
        }

        std::set<uint32_t> daily_keys;

        //如果上次领奖是当天(不一定是领取当日奖励,有可能是当天领取以前未领的奖励)
        if (TimeUtils::is_same_day(last_get_reward_tm, NOW())) {
            //如果当天奖励已领(说明以前的也必定领过了)
            if(TimeUtils::test_gived_time_exceed_tm_point(last_get_reward_tm, ARENA_DAILY_REWARD_TM_PONIT)) {
                set_login_step_finished(player, login_steps_get_arena_daily_rank);
                return proc_login(player);

            //如果当天的没领(以前的必定已领), 但此刻已经过了定时发奖时间,则登陆主动领取今日奖励
            } else if (TimeUtils::test_gived_time_exceed_tm_point(NOW(), ARENA_DAILY_REWARD_TM_PONIT)) {
                daily_keys.insert(TimeUtils::time_to_date(NOW()));
            }

        //上次领奖不是当天
        } else {
            //如果上次领奖时间没有超过当天的21点,则需要领取当天的
            if (!TimeUtils::test_gived_time_exceed_tm_point(last_get_reward_tm, ARENA_DAILY_REWARD_TM_PONIT)) {
                daily_keys.insert(TimeUtils::time_to_date(last_get_reward_tm));

            //如果超过了21点 则当天的肯定领取了,但有可能持续在线,且打了第二天的竞技场,需要尝试领取下一天的
            } else if (!TimeUtils::is_same_day(last_get_reward_tm + DAY_SECS, NOW())) {
                //如果第二天不是今天
                daily_keys.insert(TimeUtils::time_to_date(last_get_reward_tm + DAY_SECS));

            //若第二天就是今天且现在时间超过了21点 则领取今天的奖励
            } else if (TimeUtils::test_gived_time_exceed_tm_point(NOW(), ARENA_DAILY_REWARD_TM_PONIT)) {
                daily_keys.insert(TimeUtils::time_to_date(NOW()));
            }
        }

		std::vector<rank_key_order_t> rank_vec;
		FOREACH(daily_keys, it) {
			struct rank_key_order_t tmp;
			tmp.key.key = commonproto::RANKING_ARENA;
			tmp.key.sub_key = *it;
            tmp.order = commonproto::RANKING_ORDER_ASC;
			rank_vec.push_back(tmp);
            tmp.key.key = commonproto::RANKING_RPVP;
            tmp.key.sub_key = *it;
            tmp.order = commonproto::RANKING_ORDER_DESC;
            rank_vec.push_back(tmp);
		}
		if (rank_vec.size()) {
			return RankUtils::get_user_rank_info_by_keys(
					player, rank_vec,
					player->userid, player->create_tm);
		} else {
			set_login_step_finished(player, login_steps_get_arena_daily_rank);
			return proc_login(player);
		}
	}

	inline int init_role(player_t *player);	


    int send_login_response(player_t* player);

    int proc_login(player_t *player);
   
    onlineproto::cs_0x0101_enter_svr cli_in_; 
    onlineproto::sc_0x0101_enter_svr cli_out_;
    dbproto::sc_get_login_info db_out_;
	rankproto::sc_get_user_multi_rank rank_out_;
	rankproto::sc_get_user_rank  user_rank_out_;

    struct login_session_t
    {
        char tad[128];
        char browse[128];
        char version[128];
        char device[128];
        char os[64];
        char resolution[64];
        char network[64];
    };

    char send_buf_[4096];
};

class LoginCompleteCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
};

#endif
