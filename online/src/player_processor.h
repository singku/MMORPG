#ifndef __PLAYER_PROCESSOR_H__
#define __PLAYER_PROCESSOR_H__

#include "common.h"
#include "cmd_processor_interface.h"

class RequireServerTimeCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
    onlineproto::cs_0x0114_require_server_time cli_in_;
    onlineproto::sc_0x0114_require_server_time cli_out_;
};

class RTTTestCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
    onlineproto::cs_0x0115_rtt_test cli_in_;
    onlineproto::sc_0x0115_rtt_test cli_out_;
};

class HeartBeatCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
};

class CliSetAttrCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x012B_cli_set_attr cli_in_;
};

class CliGetAttrListCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
    onlineproto::cs_0x0135_cli_get_attr_list cli_in_;
    onlineproto::sc_0x0135_cli_get_attr_list cli_out_;
    dbproto::cs_get_attr db_in_;
    dbproto::sc_get_attr db_out_;
};

class GetOtherPlayerDataCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
    int proc_pkg_from_serv_aft_get_cache_info(
            player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_family_info(
            player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_arena_rank(
			player_t* player, const char* body, int bodylen);
    onlineproto::cs_0x013A_get_other_player_info cli_in_;
    onlineproto::sc_0x013A_get_other_player_info cli_out_;
    cacheproto::cs_batch_get_users_info cache_in_;
    cacheproto::sc_batch_get_users_info cache_out_;
};

class GetOtherPlayerHeadCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x013D_get_others_head cli_in_;
    onlineproto::sc_0x013D_get_others_head cli_out_;
    cacheproto::cs_get_cache cache_in_;
    cacheproto::sc_get_cache cache_out_;
};

class BuyVpCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0137_buy_vp cli_in_;
    onlineproto::sc_0x0137_buy_vp cli_out_;
};

class AlchemyCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x013E_alchemy cli_in_;
    onlineproto::sc_0x013E_alchemy cli_out_;
};

class FrontStatCmdProcessor: public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0140_front_stat_log cli_in_;
    onlineproto::sc_0x0140_front_stat_log cli_out_;
};

/*随机昵称*/
class RandomNickCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char* body, int bodylen);
    static std::string get_random_nick(uint32_t sex);

private:
    struct nick_session_t{
        char nick[64];
    };

    onlineproto::cs_0x0002_require_random_nick cli_in_;
    onlineproto::sc_0x0002_require_random_nick cli_out_;
    dbproto::cs_get_user_by_nick db_in_;
    dbproto::sc_get_user_by_nick db_out_;;
};

/*设置昵称*/
class SetNickCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    struct nick_session_t{
        char nick[64];
    };

    onlineproto::cs_0x0142_set_nick cli_in_;
    onlineproto::sc_0x0142_set_nick cli_out_;
    dbproto::cs_insert_nick_and_user db_insert_in_;
    dbproto::sc_insert_nick_and_user db_insert_out_;
    dbproto::cs_delete_nick_and_user db_delete_in_;
    dbproto::cs_change_nick db_change_in_;
};

//获取session
class GetSessionCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x000A_cli_get_session cli_in_;
    onlineproto::sc_0x000A_cli_get_session cli_out_;
};

class GetOnlineListCmdProcessor: public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    onlineproto::cs_0x0148_get_online_list cli_in_;
    onlineproto::sc_0x0148_get_online_list cli_out_;
    switchproto::cs_get_server_list sw_in_;
    switchproto::sc_get_server_list sw_out_;
};

class GenInviteCodeCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

private:
    onlineproto::cs_0x014D_gen_invite_code cli_in_;
    onlineproto::sc_0x014D_gen_invite_code cli_out_;
};

class SignInviteCodeCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_check_user_exist(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_get_invite_code_info(player_t *player, const char *body, int bodylen);

private:
    onlineproto::cs_0x014E_sign_invite_code cli_in_;
    onlineproto::sc_0x014E_sign_invite_code cli_out_;
};

class GetChargeDiamondDrawCardsCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    onlineproto::cs_0x015A_get_charge_diamond_draw_cards cli_in_;
    onlineproto::sc_0x015A_get_charge_diamond_draw_cards cli_out_;
    dbproto::cs_user_raw_data_get db_in_;
    dbproto::sc_user_raw_data_get db_out_;
};

class RefreshChargeDiamondDrawCardsCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x015B_refresh_charge_diamond_draw_cards cli_in_;
    onlineproto::sc_0x015B_refresh_charge_diamond_draw_cards cli_out_;
};

class DrawChargeDiamondCardCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

private:
    onlineproto::cs_0x015C_draw_charge_diamond_card cli_in_;
    onlineproto::sc_0x015C_draw_charge_diamond_card cli_out_;
};

class MayinSendFlowerCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0614_mayin_send_flower cli_in_;
	onlineproto::sc_0x0614_mayin_send_flower cli_out_;
	rankproto::sc_get_user_rank rank_out_;
};

class WeeklyRankingActivityCmdProcessor: public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0622_weekly_ranking_activity cli_in_;
	onlineproto::sc_0x0622_weekly_ranking_activity cli_out_;
};

class SurveyGetCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x016A_get_survey_answer cli_in_;
    onlineproto::sc_0x016A_get_survey_answer cli_out_;
    dbproto::sc_user_raw_data_get db_out_;
};

class SurveySubmitCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x016B_submit_survey_answer cli_in_;
    onlineproto::sc_0x016B_submit_survey_answer cli_out_;
    dbproto::sc_user_raw_data_get db_out_;
};

class FakeCmdForWeeklyActivityRankCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0255_fake_cmd_for_weekly_activity_rank cli_in_;
	onlineproto::sc_0x0255_fake_cmd_for_weekly_activity_rank cli_out_;
	rankproto::sc_get_user_rank  user_rank_out_;
};

#endif
