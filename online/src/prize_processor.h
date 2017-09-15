#ifndef __PRIZE_PROCESSOR_H__
#define __PRIZE_PROCESSOR_H__

#include "common.h"
#include "cmd_processor_interface.h"
#include "proto/db/dbproto.gift_code.pb.h"

class RequirePrizeCmdProcessor : public CmdProcessorInterface
{
public:
    RequirePrizeCmdProcessor() {
        register_require_reason();
    }
public:
    typedef int (*prize_proc_func_t)(player_t*, uint32_t, 
            onlineproto::sc_0x0112_notify_get_prize&);
	typedef int (*prize_proc_func_need_get_info_from_serv)(player_t*);

    void register_require_reason();
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	int get_player_arena_week_rank(player_t* player, const char *body, int bodylen);
	int proc_diamond_ten_times_lottery_prize_form_svr(player_t* player, const char *body, int bodylen);
	int proc_raw_data_from_svr(player_t* player, const char* body, int bodylen);
	int proc_expedition_prize(player_t* player, const char* body, int bodylen);
	int proc_night_raid_prize(player_t* player, const dbproto::sc_user_raw_data_get &msg);

    bool check_lottery(
            player_t *player, onlineproto::cs_0x0113_require_prize cli_in);

	struct required_prize_session_t {
		uint32_t prize_id;
		uint32_t reason;
	};

private:
    onlineproto::cs_0x0113_require_prize cli_in_;
    onlineproto::sc_0x0113_require_prize cli_out_;
    onlineproto::sc_0x0112_notify_get_prize noti_;
	dbproto::sc_user_raw_data_get db_raw_data_;

    std::map<uint32_t, prize_proc_func_t> prize_func_map_;

	std::map<uint32_t, prize_proc_func_need_get_info_from_serv> prize_srv_map_;
};

class UseGiftCodeCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x014B_exchange_gift_code cli_in_;
    onlineproto::sc_0x014B_exchange_gift_code cli_out_;
    dbproto::cs_use_gift_code db_in_;
    dbproto::sc_use_gift_code db_out_;
};

class GetPrizeBulletinCmdProcessor : public CmdProcessorInterface
{
	public:
		int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
		int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	private:
		int proc_get_members_from_ranking(player_t* player, const char* body, int bodylen);
		onlineproto::cs_0x0160_inform_prize_bulletin cli_in_;
		onlineproto::sc_0x0160_inform_prize_bulletin cli_out_;
};
//资源找回
class ResourceRetrieveCmdProcessor : public CmdProcessorInterface
{
	public:
		ResourceRetrieveCmdProcessor(){
			 register_resource_type();
		}
	public:
		int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
		int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

		typedef int (*retrieve_proc_func_t)(player_t*, uint32_t, uint32_t,
			   	onlineproto::sc_0x0112_notify_get_prize&);
		void register_resource_type();
		void get_user_retrieve_value_map(player_t *player);
		void clear_user_retrieve_value(player_t *player, commonproto::resource_type_t type);
	private:
		onlineproto::cs_0x014F_resource_retrieve cli_in_;
		onlineproto::sc_0x014F_resource_retrieve cli_out_;
		onlineproto::sc_0x0112_notify_get_prize noti_;
		std::map<uint32_t, retrieve_proc_func_t> retrieve_func_map_;
		//value为0 不可找回
		std::map<uint32_t, uint32_t> value_map_;
};
int proc_swim_resource_retrieve(player_t *player, uint32_t count, 
		uint32_t type, onlineproto::sc_0x0112_notify_get_prize &noti);
int proc_escort_resource_retrieve(player_t *player, uint32_t count,
	   	uint32_t type, onlineproto::sc_0x0112_notify_get_prize &noti);
int proc_mon_crisis_resource_retrieve(player_t *player, uint32_t count,
	   	uint32_t type, onlineproto::sc_0x0112_notify_get_prize &noti);

class UseMagicWordCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
    uint32_t proc_errno_from_serv(player_t *player, uint32_t ret) {
        switch (ret) {
        case 1:
            return cli_err_magic_word_arg_err;
        case 2:
            return cli_err_magic_word_not_exist;
        case 3:
            return cli_err_magic_word_not_active;
        case 4:
            return cli_err_magic_word_not_effect_date;
        case 5:
            return cli_err_magic_word_frozen;
        case 6:
            return cli_err_magic_word_gotten;
        case 7:
            return cli_err_magic_word_internal_err;
        case 11:
            return cli_err_magic_word_got_same;
        default:
            return cli_err_sys_err;
        }
    }

private:
    onlineproto::cs_0x014B_exchange_gift_code cli_in_;
    onlineproto::sc_0x014B_exchange_gift_code cli_out_;
};


class StarLotteryCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0625_star_lottery                   cli_in_;
	onlineproto::sc_0x0625_star_lottery                  cli_out_;
	rankproto::sc_hset_get_field_info    rank_get_field_info_out_;
};

class GetStarLotteryInfoCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0626_get_star_lottery_info         cli_in_;
	onlineproto::sc_0x0626_get_star_lottery_info         cli_out_;
	rankproto::sc_hset_get_field_info    rank_get_field_info_out_;
};

int proc_test_prize(player_t *player, uint32_t prize_id, 
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_arena_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_arena_prize_from_svr(player_t* player);

int proc_exped_prize_from_svr(player_t* player);

int proc_night_raid_prize_from_svr(player_t* player);

int proc_online_reward_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_alchemy_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_buy_vp_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_sign_in_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_month_sign_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_seven_days_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_wei_xin_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_month_card_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_normal_one_time_lottery_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_normal_ten_times_lottery_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_diamond_one_time_lottery_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

// int proc_diamond_ten_times_lottery_prize(player_t *player, uint32_t prize_id,
		// onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_diamond_ten_times_lottery_prize_form_cli(player_t* player);

int proc_exped_prize(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_change_clothes_plan(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_favorite_ct_reward(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int can_get_change_clothes_plan_reward(
        player_t *player, uint32_t prize_id, uint32_t &reward_record);

int update_change_clothes_plan_start_time(player_t *player);
int update_change_clothes_plan_record(player_t *player, commonproto::pet_info_t *pet_info);

int proc_get_year_vip_reward(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_battle_value_gift(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_marly_shot_gift(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_marly_question_gift(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_sign_invite_code_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_invited_players_reach_n_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_share_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_get_first_recharge_gift(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_consume_diamond_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_evil_knife_legend(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_mayin_bucket_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_vip_get_yaya_fragment(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_survey_prize(player_t *player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_charge_diamond_get_gift_prize(player_t *player, uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &noti);

int proc_summer_weekly_signed(player_t* player, uint32_t prize_id,
		onlineproto::sc_0x0112_notify_get_prize &noti);

#endif
