#ifndef  __BLESS_PET_PROCESSOR_H__
#define  __BLESS_PET_PROCESSOR_H__
#include "common.h"
#include "player.h"
#include "proto/client/pb0x06.pb.h"
#include "cmd_processor_interface.h"
#include "rank_utils.h"

//创建伙伴祈福队伍
class CreateBlessTeamCmdProcessor : public CmdProcessorInterface{
	public:
		int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
		int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	private:
		onlineproto::cs_0x061B_create_bless_team cli_in_;
		onlineproto::sc_0x061B_create_bless_team cli_out_;
};

class GetBlessTeamInfoCmdProcessor : public CmdProcessorInterface{
	public:
		int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
		int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	private:
		onlineproto::cs_0x061E_get_bless_team_info cli_in_;
		onlineproto::sc_0x061E_get_bless_team_info cli_out_;
		rankproto::cs_hset_get_field_info     rank_get_field_info_in_;
		rankproto::sc_hset_get_field_info     rank_get_field_info_out_;
};

// class GetBlessTeamListCmdProcessor : public CmdProcessorInterface{
	// public:
		// int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
		// int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	// private:
		// int proc_pkg_from_serv_aft_get_team_list_from_redis(player_t *player, const char *body, int bodylen);
		// int proc_pkg_from_serv_aft_get_team_info_from_redis(player_t *player, const char *body, int bodylen);
		// onlineproto::cs_0x061F_get_bless_team_list cli_in_;
		// onlineproto::sc_0x061F_get_bless_team_list cli_out_;
		// rankproto::cs_hset_get_info     rank_get_info_in_;
		// rankproto::sc_hset_get_info     rank_get_info_out_;
		// rankproto::cs_hset_get_field_info     rank_get_field_info_in_;
		// rankproto::sc_hset_get_field_info     rank_get_field_info_out_;
// };

class DismissBlessTeamCmdProcessor : public CmdProcessorInterface{
	public:
		int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
		int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	private:
		onlineproto::cs_0x061D_dismiss_bless_team cli_in_;
		onlineproto::sc_0x061D_dismiss_bless_team cli_out_;
};

class RequireBlessTeamCmdProcessor : public CmdProcessorInterface{
	public:
		int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
		int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
	private:
		int proc_pkg_from_serv_aft_get_infos_from_redis(player_t *player, const char *body, int bodylen);
		int proc_pkg_from_serv_aft_get_pet_info(player_t *player, const char *body, int bodylen);
		onlineproto::cs_0x061C_require_bless_team cli_in_;
		onlineproto::sc_0x061C_require_bless_team cli_out_;
		rankproto::cs_hset_get_field_info     rank_get_field_info_in_;
		rankproto::sc_hset_get_field_info     rank_get_field_info_out_;
		// cacheproto::cs_batch_get_users_info cache_in_;
		// cacheproto::sc_batch_get_users_info cache_out_;
		dbproto::cs_pet_list_get_info db_in_;
		dbproto::sc_pet_list_get_info db_out_;
};

//伙伴祈福获得队友battle信息,enter dup之前需要调用此协议
class CacheBlessTeamMemberInfoCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
	int proc_pkg_from_serv_aft_get_infos_from_redis(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv_aft_get_infos_from_cache(player_t *player, const char *body, int bodylen);
	onlineproto::cs_0x0621_syn_bless_team_member_btl_info cli_in_;
	onlineproto::sc_0x0621_syn_bless_team_member_btl_info cli_out_;
	rankproto::cs_hset_get_field_info     rank_get_field_info_in_;
	rankproto::sc_hset_get_field_info     rank_get_field_info_out_;
	cacheproto::sc_batch_get_users_info   cache_info_out_;
};
#endif
