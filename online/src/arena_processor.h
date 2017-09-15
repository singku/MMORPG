#ifndef __ARENA_PROCESSOR_H__
#define __ARENA_PROCESSOR_H__
#include "common.h"
#include "cmd_processor_interface.h"
#include "player.h"
#include "arena.h"

struct user_rank_info_t
{
	user_rank_info_t() : 
		user_id(0),
		create_tm(0),
		rank(0),
		score(0) {}
    uint32_t user_id;
	uint32_t create_tm;
    uint32_t rank;
    uint64_t score;
};

//获取竞技场对手列表
class GetArenaPlayerDataCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t*player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	
	int proc_pkg_from_serv_get_multi_rank(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_rank_users(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_rank_insert_last(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_users_info(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_btl_report_key(
			player_t* player, const char* body, int bodylen);
	onlineproto::cs_0x0217_get_arena_data	cli_in_;
	onlineproto::sc_0x0217_get_arena_data	cli_out_;
	//rankproto::sc_get_user_rank  user_rank_out_;
	rankproto::sc_get_rank_userid   rank_userid_out_;
	cacheproto::sc_batch_get_users_info	  cache_info_out_;
	rankproto::sc_get_battle_report_key rank_key_out_;

	rankproto::sc_get_user_multi_rank  multi_rank_out_;
	rankproto::sc_rank_insert_last  insert_last_out_;
};

//挑战
class ChallengeArenaPlayerCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t*player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	int proc_pkg_from_serv_aft_get_self_rank(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_challenge_player_userid(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_cache_info(
			player_t* player, const char* body, int bodylen);
	struct challenge_arena_player_session_t {
		uint32_t atk_tick;
		uint32_t def_tick;
	};
	onlineproto::cs_0x0218_challenge_arena_player cli_in_;
	onlineproto::sc_0x0218_challenge_arena_player cli_out_;
	rankproto::sc_get_user_rank user_rank_out_;
	rankproto::sc_get_rank_userid rank_userid_out_;
	dbproto::sc_get_cache_info db_out_;
	player_t challenge_player_;
	cacheproto::sc_batch_get_users_info	  cache_info_out_;
};

//获取竞技排行列表
class GetRankListCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t*player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	struct get_arena_ranking_session_t
	{
		uint32_t type;
		uint32_t sub_type;
		uint32_t total; //总排名数量
		uint32_t count; //拉取到的排名数量
		user_rank_info_t rank_info[kMaxRankPlayerNum];
        user_rank_info_t self_rank_data;
        uint32_t info_index; // 当前拉取详细信息的排名记录下标
		uint32_t order;
	};
	int proc_pkg_from_serv_aft_get_rank_list(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_cache_info(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_family_info(
			player_t* player, const char* body, int bodylen);

	onlineproto::cs_0x0219_get_ranking  cli_in_;
	onlineproto::sc_0x0219_get_ranking  cli_out_;
	rankproto::sc_get_rank_list   rank_list_out_;
	cacheproto::sc_batch_get_users_info	 cache_out_;
    dbproto::cs_family_get_info db_family_info_in_;
    dbproto::sc_family_get_info db_family_info_out_;
};

//竞技场战斗结果
class ArenaResultCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t*player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	struct arena_result_session_t 
	{
		user_rank_info_t rank_info[2];	//pk双方的排名信息
	};
	int proc_pkg_from_serv_aft_get_users_rank(player_t* player, const char *body, int bodylen);
	int proc_pkg_from_serv_aft_get_ai_attr(player_t* player, const char *body, int bodylen);
	onlineproto::cs_0x0224_arena_challenge_result	cli_in_;
	onlineproto::sc_0x0224_arena_challenge_result	cli_out_;
	cacheproto::sc_batch_get_users_info	 cache_out_;
	rankproto::sc_get_user_rank  user_rank_out_;
	dbproto::sc_get_attr db_out_;
};

//获取个人排行信息
class GetRankInfoCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t*player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	struct get_rank_info_session_t
	{
        uint32_t total_user_count;
		user_rank_info_t rank_info[MAX_GET_RANK_INFO_USER_NUM];
        uint32_t type;
        uint32_t sub_type;
	};
	int proc_pkg_from_serv_aft_get_rank_info(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_cache_info(
			player_t* player, const char* body, int bodylen);

	onlineproto::cs_0x0227_get_rank_info  cli_in_;
	onlineproto::sc_0x0227_get_rank_info  cli_out_;

    rankproto::sc_get_user_rank rank_info_out_;

	cacheproto::sc_batch_get_users_info	 cache_out_;
};

//查看战报
class ViewBtlReportCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0225_view_battle_report cli_in_;
	onlineproto::sc_0x0225_view_battle_report cli_out_;
	rankproto::sc_get_battle_report rank_out_;
};

class BuyChallengeTimesCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t*player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0228_buy_arena_challenge_times cli_in_;
	onlineproto::sc_0x0228_buy_arena_challenge_times cli_out_;
};

//排行榜点赞功能
class RankPraiseFunCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0231_rank_praise_function cli_in_;
	onlineproto::sc_0x0231_rank_praise_function cli_out_;
};

class PraiseCountExgCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0232_praise_count_exchange cli_in_;
	onlineproto::sc_0x0232_praise_count_exchange cli_out_;
};

class FakeCmdForArenaDailyRankCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0233_fake_cmd_for_arena_weeklyrank cli_in_;
	onlineproto::sc_0x0233_fake_cmd_for_arena_weeklyrank cli_out_;
	rankproto::sc_get_user_multi_rank rank_out_;
};

class FakeCmdForOpenSrvRankCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0254_fake_cmd_for_open_serv_rank cli_in_;
	onlineproto::sc_0x0254_fake_cmd_for_open_serv_rank cli_out_;
	rankproto::sc_get_user_rank  user_rank_out_;
};

/*
class FakeCmdForPowerWeeklyRankCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0255_fake_cmd_for_total_power_rank cli_in_;
	onlineproto::sc_0x0255_fake_cmd_for_total_power_rank cli_out_;
	rankproto::sc_get_user_rank rank_out_;
};

class FakeCmdForGetRankDumpTmCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0256_fake_cmd_for_get_dump_tm cli_in_;
	onlineproto::sc_0x0256_fake_cmd_for_get_dump_tm cli_out_;
	dbproto::sc_get_global_attr db_out_;
};
*/

#endif
