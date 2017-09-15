#ifndef __MINE_PROCESSOR_H__
#define __MINE_PROCESSOR_H__

#include "cmd_processor_interface.h"
#include "common.h"
#include "mine_utils.h"

class SearchMineCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	struct search_mine_session_t
	{
		//待占领的矿
		mine_info_t mine[5];	
		search_step_t search_step;
	};
	int proc_pkg_from_serv_aft_search_mine_info(player_t* player,
				const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_mine_pets_info(player_t* player,
				const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_users_info(player_t* player,
				const char* body, int bodylen);
	int proc_pkg_from_serv_aft_refresh_pet_info(player_t* player,
				const char* body, int bodylen);
	int proc_pkg_from_serv_aft_open_mine_panel(player_t* player,
				const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_team_defeate_state(player_t* player,
				const char* body, int bodylen);

	onlineproto::cs_0x082E_search_mine cli_in_;
	onlineproto::sc_0x082E_search_mine cli_out_;
	dbproto::sc_get_mine_info db_out_;
	rankproto::sc_hset_get_info rank_out_;
	dbproto::sc_get_player_mine_info db_player_mine_out_;
	dbproto::sc_user_raw_data_get db_raw_date_out_;
};

class ExploitNewMineCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	struct mine_id_t
	{
		uint64_t tmp_mine_id;
	};
private:
	onlineproto::cs_0x0835_exploit_new_mine cli_in_;
	onlineproto::sc_0x0835_exploit_new_mine cli_out_;
	dbproto::sc_save_mine_info db_out_;
};

class GetMyMineInfoCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	int proc_pkg_from_serv_aft_get_defend_pets_info(
			player_t *player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_mine_info(
			player_t *player, const char* body, int bodylen);
	onlineproto::cs_0x0834_get_my_mine_info cli_in_;
	onlineproto::sc_0x0834_get_my_mine_info cli_out_;
	rankproto::sc_hset_get_info rank_out_;
	dbproto::sc_get_player_mine_info db_out_;
};

class GiveUpMineCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	struct mine_id_t
	{
		uint64_t mine_id;
	};
	
private:
	int prog_pkg_from_serv_aft_get_attack_flag(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_teams_info(
			player_t* player, const char* body, int bodylen);
	onlineproto::cs_0x0836_give_up_this_mine cli_in_;
	onlineproto::sc_0x0836_give_up_this_mine cli_out_;
	rankproto::sc_hset_get_info rank_out_;
	dbproto::sc_get_player_mine_info db_out_;
};

class AcceptDefendMineCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	int proc_pkg_from_serv_aft_incr_def_cnt(
			player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv_aft_get_ply_mine_info(
			player_t *player, const char *body, int bodylen);
	onlineproto::cs_0x0833_accept_join_defend_mine cli_in_;
	onlineproto::sc_0x0833_accept_join_defend_mine cli_out_;
	dbproto::sc_get_player_mine_info db_out_;
	dbproto::sc_increment_defender_cnt db_incr_out_;
};

class StartOccupyMineCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	struct start_occupy_step_session_t
	{
		start_occupy_step_t step;	
		std::string tem_btl_pet_info;
		uint32_t ack_tick;
		uint32_t def_tick;
		uint64_t mine_id;
	};
	
private:
	int proc_pkg_from_serv_aft_get_mine_attack_info(player_t* player,
			const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_all_info(player_t* player,
			const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_mine_fight_info(player_t* player,
			const char* body, int bodylen);
	onlineproto::cs_0x082F_start_occupy_mine cli_in_;
	onlineproto::sc_0x082F_start_occupy_mine cli_out_;
	rankproto::sc_hset_get_field_info  rank_get_field_out_;
	rankproto::sc_hset_get_info rank_get_info_;
	dbproto::sc_user_raw_data_get db_out_;
	dbproto::sc_get_player_mine_info db_get_attack_out_;
};

class OccupyMineRetCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0830_occupy_mine_result cli_in_;
	onlineproto::sc_0x0830_occupy_mine_result cli_out_;
	dbproto::sc_save_mine_info db_out_;
};

class GetMineBtlReportCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0838_get_mine_fight_btl_report cli_in_;
	onlineproto::sc_0x0838_get_mine_fight_btl_report cli_out_;
	rankproto::sc_get_btl_report rank_out_;
};

class GetMineInfoByMineIdCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	int proc_pkg_from_serv_aft_get_mine_info(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_pet_info(
			player_t* player, const char* body, int bodylen);
	onlineproto::cs_0x0839_get_mine_info_by_mine_id cli_in_;
	onlineproto::sc_0x0839_get_mine_info_by_mine_id cli_out_;
	dbproto::sc_get_player_mine_info db_out_;
	rankproto::sc_hset_get_info rank_out_;
};

class AnewSearchMineCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	struct refresh_mine_session_t
	{
		mine_info_t mine_info[5];	
	};
	int proc_pkg_from_serv_aft_anew_search_mine_info(
			player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv_aft_anew_get_mine_pets_info(
			player_t *player, const char *body, int bodylen);
	onlineproto::cs_0x0837_refresh_mine cli_in_;
	onlineproto::sc_0x0837_refresh_mine cli_out_;
	dbproto::sc_get_mine_info db_out_;
	rankproto::sc_hset_get_info rank_out_;
	dbproto::sc_user_raw_data_get db_raw_date_out_;
};

class MineSendkMsgCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x083A_mine_send_hyperlink_msg cli_in_;
	onlineproto::sc_0x083A_mine_send_hyperlink_msg cli_out_;
};

class CheckMyMineExpireCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	int proc_pkg_from_serv_aft_get_mine_info(player_t* player,
			const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_defend_pets_info(player_t* player,
			const char* body, int bodylen);
	onlineproto::cs_0x083C_check_my_mine_expire cli_in_;
	onlineproto::sc_0x083C_check_my_mine_expire cli_out_;
	rankproto::sc_hset_get_info rank_out_;
	dbproto::sc_get_player_mine_info db_out_;
};

#endif
