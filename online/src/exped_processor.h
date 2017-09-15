#ifndef __EXPED_PROCESSOR_H__
#define __EXPED_PROCESSOR_H__

#include "cmd_processor_interface.h"
#include "common.h"

class ExpedInfoSceneCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);	
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	int proc_pkg_from_serv_aft_get_infos_from_redis(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_infos_from_db(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_infos_from_cache(
			player_t* player, const char* body, int bodylen);
	onlineproto::cs_0x0240_expedition_into_scene cli_in_;
	onlineproto::sc_0x0240_expedition_into_scene cli_out_;
	rankproto::sc_get_users_by_score_range rank_out_;
	dbproto::sc_user_raw_data_get db_out_;
	cacheproto::sc_batch_get_users_info  cache_info_out_;
};

class ExpedStartCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	struct exped_start_tick_t {
		uint32_t atk_tick;
		uint32_t def_tick;
	};
private:
	onlineproto::cs_0x0243_expedition_start cli_in_;
	onlineproto::sc_0x0243_expedition_start cli_out_;
	dbproto::sc_user_raw_data_get db_out_;
};

class ExpedResultCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
	onlineproto::cs_0x0244_expedition_result cli_in_;
	onlineproto::sc_0x0244_expedition_result cli_out_;
};

class ExpedPickJoinedPetCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
	onlineproto::cs_0x0247_expedition_choose_pet cli_in_;
	onlineproto::sc_0x0247_expedition_choose_pet cli_out_;
};

class ExpedResetCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
	onlineproto::cs_0x0242_expedition_reset cli_in_;
	onlineproto::sc_0x0242_expedition_reset cli_out_;
};

class ExpedPrizeTotalProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
	onlineproto::cs_0x0245_expedition_prize_total cli_in_;
	onlineproto::sc_0x0245_expedition_prize_total cli_out_;
	dbproto::sc_user_raw_data_get db_out_;
};

#endif
