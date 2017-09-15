#include "bless_pet_processor.h"
#include <boost/lexical_cast.hpp>
#include "utils.h"
#include "common.h"
#include "macro_utils.h"
#include "service.h"
#include "proto_processor.h"
#include "data_proto_utils.h"
#include "global_data.h"
#include "bless_pet_conf.h"
#include "player_utils.h"
#include "pet_utils.h"
#include "bless_pet_utils.h"

const static int max_team_num            = 4;
const static uint32_t max_bless_times    = 3;
// const static uint32_t need_gold_cnt      = 2000;
// const static uint32_t mail_prize_id      = 2502;

int RequireBlessTeamCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t m_userid    = cli_in_.member().userid();
	uint32_t m_create_tm = cli_in_.member().u_create_tm();
	uint32_t l_userid    = cli_in_.team_leader().userid();
	uint32_t l_create_tm = cli_in_.team_leader().u_create_tm();
	uint32_t bless_id  = cli_in_.pet_bless_id();
	uint32_t type = cli_in_.type();

	if(m_userid == l_create_tm && m_create_tm == l_create_tm){
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_can_not_join_bless_team);
	}
	//被动加入 只能拉好友加入自己的队伍
	if(type == commonproto::JOIN_TEAM_PASSIVE){
		if(player->userid != l_userid || player->create_tm != l_create_tm){
			return send_err_to_player(player, player->cli_wait_cmd,
					cli_err_can_not_join_bless_team);
		}

	} else if (type == commonproto::JOIN_TEAM_ACTIVE){

		if(GET_A(kDailyJoinBlessPetTeamTimes) >= max_bless_times){
			return send_err_to_player(player, player->cli_wait_cmd,
					cli_err_assist_bless_times_over_limit);
		}
		//主动加入
		if(player->userid != m_userid || player->create_tm != m_create_tm){
			return send_err_to_player(player, player->cli_wait_cmd,
					cli_err_can_not_join_bless_team);
		}

		if(!BlessPetUtils::check_has_bless_pet(player, bless_id)){
			return send_err_to_player(player, player->cli_wait_cmd,
					cli_err_have_no_bless_pet);
		}
	}

	//拉取队伍信息
	uint64_t leader_key = ROLE_KEY(ROLE(l_userid, l_create_tm));
	//一天的起点做subkey
	uint32_t day = TimeUtils::day_align_low(NOW());
	// 根据leader_key信息
	std::ostringstream redis_key;
	redis_key << rankproto::HASHSET_BLESS_PET_TEAM_MAP<<":"<< day;
	rank_get_field_info_in_.Clear();
	rank_get_field_info_in_.set_key(redis_key.str());
	rank_get_field_info_in_.add_field_names(boost::lexical_cast<string>(leader_key));
	rank_get_field_info_in_.set_server_id(g_server_id);

	return g_dbproxy->send_msg(
			player, player->userid, player->create_tm, 
			ranking_cmd_hset_get_field_info, rank_get_field_info_in_);
}

int RequireBlessTeamCmdProcessor::proc_pkg_from_serv_aft_get_pet_info(
		player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	uint32_t bless_id  = cli_in_.pet_bless_id();
	bool has_pet = false;

	const commonproto::pet_list_t& pet_inf = db_out_.pet_list();
	if (pet_inf.pet_list_size() == 0) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_have_no_bless_pet);
	}

	has_pet = BlessPetUtils::check_other_has_bless_pet(pet_inf, bless_id);
	if(!has_pet){
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_have_no_bless_pet);
	}

	int ret = 0;
	//解析缓存信息
	commonproto::bless_pet_team_info_t team_info;
	ret = BlessPetUtils::parse_team_info_from_string(team_info,
		   	*player->temp_info.bless_team_info); 
	if(ret){
		return send_err_to_player(
				player, player->cli_wait_cmd, ret);
	}

	// ret = AttrUtils::sub_player_gold(player, need_gold_cnt, "拉好友伙伴祈福");
	// if(ret){
		// return send_err_to_player(
				// player, player->cli_wait_cmd, ret);
	// }
	//给好友发金币
	// std::string content = "您的好友" + boost::lexical_cast<std::string>(player->nick) +
		// "在伙伴祈福队伍中拉取您为队友，支付给您金币";
	// PlayerUtils::generate_new_mail(player, "获得一定的奖励", content,
			// mail_prize_id);

	//加入队伍
	commonproto::role_info_t *member = team_info.add_members();
	member->set_userid(cli_in_.member().userid());
	member->set_u_create_tm(cli_in_.member().u_create_tm());
    // 更新祈福队伍
	BlessPetUtils::update_bless_team_info_to_redis(player, team_info);

	cli_out_.Clear();
	cli_out_.mutable_team()->CopyFrom(team_info);
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

int RequireBlessTeamCmdProcessor::proc_pkg_from_serv_aft_get_infos_from_redis(
		player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_get_field_info_out_);
	uint32_t m_userid    = cli_in_.member().userid();
	uint32_t m_create_tm = cli_in_.member().u_create_tm();
	// uint32_t bless_id  = cli_in_.pet_bless_id();

    if (rank_get_field_info_out_.fields_size() == 0) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

	commonproto::bless_pet_team_info_t team_info;
	string pkg = rank_get_field_info_out_.fields(0).value();
	//缓存信息
	(*player->temp_info.bless_team_info) = pkg;


	int ret = 0;
	ret = BlessPetUtils::parse_team_info_from_string(team_info, pkg); 

	if(ret){
		return send_err_to_player(
				player, player->cli_wait_cmd, ret);
	}

	//是否达上限
	if(team_info.members_size() >= max_team_num){
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_bless_team_members_over_limit);
	}

	//是否已经加入过
	for(int i = 0; i < team_info.members_size(); i++){
		if(m_userid == team_info.members(i).userid()
			 && m_create_tm == team_info.members(i).u_create_tm()){
			return send_err_to_player(
					player, player->cli_wait_cmd,
					cli_err_can_not_join_bless_team );
		}
	}	

#if 0
	uint32_t type = cli_in_.type();
	//被动加入 扣金币给好友加金币
	if(type == commonproto::JOIN_TEAM_PASSIVE){
		//拉取好友pet信息
		db_in_.Clear();
		return g_dbproxy->send_msg(
				player, m_userid, m_create_tm, 
				db_cmd_pet_list_get, db_in_);

	} else if (type == commonproto::JOIN_TEAM_ACTIVE){
		//主动加入
		commonproto::role_info_t *member = team_info.add_members();
		member->set_userid(cli_in_.member().userid());
		member->set_u_create_tm(cli_in_.member().u_create_tm());
		//发奖励
		const bless_pet_conf_t *conf_inf =
			g_bless_pet_conf_mgr.find_bless_pet_conf(bless_id);
		uint32_t pet_id = conf_inf->pet_id;
		const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
		const uint32_t item_id  = pet_conf->talent_item;
		const uint32_t item_cnt = 1;
		add_single_item(player, item_id, item_cnt);
	}
#endif

	commonproto::role_info_t *member = team_info.add_members();
	member->set_userid(cli_in_.member().userid());
	member->set_u_create_tm(cli_in_.member().u_create_tm());
    // 更新祈福队伍
	BlessPetUtils::update_bless_team_info_to_redis(player, team_info);
	ADD_A(kDailyJoinBlessPetTeamTimes, 1);

	cli_out_.Clear();
	cli_out_.mutable_team()->CopyFrom(team_info);
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);

	return 0;
}

int RequireBlessTeamCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	switch(player->serv_cmd) {
		case ranking_cmd_hset_get_field_info:
			return proc_pkg_from_serv_aft_get_infos_from_redis(
					player, body, bodylen);
		case db_cmd_pet_list_get:
			return proc_pkg_from_serv_aft_get_pet_info(player, body, bodylen);
	}

	return 0;
}

int CreateBlessTeamCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t userid    = cli_in_.team_leader().userid();
	uint32_t create_tm = cli_in_.team_leader().u_create_tm();
	uint32_t bless_id  = cli_in_.pet_bless_id();

	//创建次数
	if(GET_A(kDailyCreateBlessPetTeamTimes)){
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_can_not_create_bless_team);
	}

	//是否有伙伴
	if(!g_bless_pet_conf_mgr.bless_pet_conf_exist(bless_id)){
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_bless_pet_id_invalid);
	}

	if(!BlessPetUtils::check_has_bless_pet(player, bless_id)){
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_have_no_bless_pet);
	}

	if(player->userid != userid || player->create_tm != create_tm){
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_can_not_create_bless_team);
	}

	commonproto::bless_pet_team_info_t  my_team;
	my_team.mutable_team_leader()->set_userid(userid);
	my_team.mutable_team_leader()->set_u_create_tm(create_tm);
	my_team.set_team_name(cli_in_.team_name());
	my_team.set_pet_bless_id(bless_id);

    // 更新祈福队伍
	int ret = BlessPetUtils::update_bless_team_info_to_redis(player, my_team);
	if(ret){
		return ret;
	}

	// //加入拉取队伍列表
	// ret = BlessPetUtils::push_bless_team_list(player);
	// if(ret){
		// return ret;
	// }

	ADD_A(kDailyCreateBlessPetTeamTimes, 1);

	cli_out_.Clear();
	cli_out_.mutable_team()->CopyFrom(my_team);
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

int CreateBlessTeamCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	return 0;
}

int DismissBlessTeamCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	//还没有队伍
	if(!GET_A(kDailyCreateBlessPetTeamTimes)){
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_no_bless_team_info);
	}
	//祈福过不能再创建
	if(GET_A(kDailyBlessPetTimes)){
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_can_not_dismiss_bless_team);
	}

	BlessPetUtils::delete_bless_team_info_to_redis(player);
	SET_A(kDailyCreateBlessPetTeamTimes, 0);
	cli_out_.Clear();
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

int DismissBlessTeamCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	return 0;
}

int GetBlessTeamInfoCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t l_userid    = cli_in_.team_leader().userid();
	uint32_t l_create_tm = cli_in_.team_leader().u_create_tm();

	//还没有队伍
	if(!GET_A(kDailyCreateBlessPetTeamTimes)){
		// return send_err_to_player(player, player->cli_wait_cmd,
				// cli_err_no_bless_team_info);
		return 0;
	}

	if(player->userid != l_userid || player->create_tm != l_create_tm){
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_can_not_get_bless_team_info);
	}

	//拉取队伍信息
	uint64_t leader_key = ROLE_KEY(ROLE(l_userid, l_create_tm));
	uint32_t day = TimeUtils::day_align_low(NOW());

	// 根据leader_key信息
	std::ostringstream redis_key;
	redis_key << rankproto::HASHSET_BLESS_PET_TEAM_MAP<<":"<< day;
	rank_get_field_info_in_.Clear();
	rank_get_field_info_in_.set_key(redis_key.str());
	rank_get_field_info_in_.add_field_names(boost::lexical_cast<string>(leader_key));
	rank_get_field_info_in_.set_server_id(g_server_id);

	 g_dbproxy->send_msg(
			player, player->userid, player->create_tm, 
			ranking_cmd_hset_get_field_info, rank_get_field_info_in_);
	return 0;
}

int GetBlessTeamInfoCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_get_field_info_out_);

    if (rank_get_field_info_out_.fields_size() == 0) {
        return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_no_bless_team_info);
    }

	commonproto::bless_pet_team_info_t team_info;
	string pkg = rank_get_field_info_out_.fields(0).value();

	int ret = 0;
	ret = BlessPetUtils::parse_team_info_from_string(team_info, pkg); 

	if(ret){
		return send_err_to_player(
				player, player->cli_wait_cmd, ret);
	}

	cli_out_.Clear();
	cli_out_.mutable_team()->CopyFrom(team_info);
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

#if 0
int GetBlessTeamListCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;

	//拉取队伍列表
	int ret = BlessPetUtils::get_bless_team_list(player);
	if(ret){
		return ret;
	}

	return 0;
}

int GetBlessTeamListCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	switch(player->serv_cmd) {
		case ranking_cmd_list_get_range_member:
			return proc_pkg_from_serv_aft_get_team_list_from_redis(
					player, body, bodylen);
		case ranking_cmd_hset_get_info:
			return proc_pkg_from_serv_aft_get_team_info_from_redis(
					player, body, bodylen);
	}

	return 0;
}

int GetBlessTeamListCmdProcessor:: proc_pkg_from_serv_aft_get_team_list_from_redis(
		player_t *player, const char *body, int bodylen)
{
	rankproto::sc_list_get_range_member rank_out_;	
	rank_out_.Clear();
	PARSE_SVR_MSG(rank_out_);
	if(rank_out_.value_size() == 0){
		//TODO
    cli_out_.Clear();
	send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	}
	//拉队伍信息
	uint32_t key = rankproto::HASHSET_BLESS_PET_TEAM_MAP;
	uint32_t sub_key = TimeUtils::day_align_low(NOW());

	std::ostringstream redis_key;
	redis_key << key <<":"<< sub_key;

	rank_get_field_info_in_.Clear();
	rank_get_field_info_in_.set_key(redis_key.str());
	rank_get_field_info_in_.set_server_id(g_server_id);

	for(int32_t i =0 ; i < rank_out_.value_size(); i++){
		rank_get_field_info_in_.add_field_names(rank_out_.value(i));
	}

	g_dbproxy->send_msg(
			player, player->userid, player->create_tm, 
			ranking_cmd_hset_get_field_info, rank_get_field_info_in_);
	return 0;
}

int GetBlessTeamListCmdProcessor:: proc_pkg_from_serv_aft_get_team_info_from_redis(
		player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_get_field_info_out_);

    // if (rank_get_field_info_out_.fields_size() == 0) {
        // // return send_err_to_player(
				// // player, player->cli_wait_cmd,
				// // cli_err_no_bless_team_info);
    // }
    cli_out_.Clear();
	std::string pkg;
	for(int i = 0; i < rank_get_field_info_out_.fields_size(); i++){
		pkg.clear();
		pkg = rank_get_field_info_out_.fields(i).value();
		commonproto::bless_pet_team_info_t *team_info = cli_out_.add_team();
		int ret = 0;
		ret = BlessPetUtils::parse_team_info_from_string(*team_info, pkg); 
		if(ret){
			return send_err_to_player(
					player, player->cli_wait_cmd, ret);
		}
	}

	send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}
#endif

int CacheBlessTeamMemberInfoCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t l_userid    = player->userid;
	uint32_t l_create_tm = player->create_tm;

	//还没有队伍
	if(!GET_A(kDailyCreateBlessPetTeamTimes)){
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_no_bless_team_info);
	}

	if(GET_A(kDailyBlessPetTimes) >= max_bless_times){
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_bless_pet_times_over_limit);
	}
	//拉取队伍信息
	uint64_t leader_key = ROLE_KEY(ROLE(l_userid, l_create_tm));
	uint32_t day = TimeUtils::day_align_low(NOW());

	// 根据leader_key信息
	std::ostringstream redis_key;
	redis_key << rankproto::HASHSET_BLESS_PET_TEAM_MAP<<":"<< day;
	rank_get_field_info_in_.Clear();
	rank_get_field_info_in_.set_key(redis_key.str());
	rank_get_field_info_in_.add_field_names(boost::lexical_cast<string>(leader_key));
	rank_get_field_info_in_.set_server_id(g_server_id);

	return g_dbproxy->send_msg(
			player, player->userid, player->create_tm, 
			ranking_cmd_hset_get_field_info, rank_get_field_info_in_);
	return 0;
}

int CacheBlessTeamMemberInfoCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	switch(player->serv_cmd) {
		case ranking_cmd_hset_get_field_info:
			return proc_pkg_from_serv_aft_get_infos_from_redis(
					player, body, bodylen);
		case cache_cmd_ol_req_users_info:
			return proc_pkg_from_serv_aft_get_infos_from_cache(player, body, bodylen);
	}

	return 0;
}

int CacheBlessTeamMemberInfoCmdProcessor:: proc_pkg_from_serv_aft_get_infos_from_redis(
		player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_get_field_info_out_);

    if (rank_get_field_info_out_.fields_size() == 0) {
        return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_no_bless_team_info);
    }

	commonproto::bless_pet_team_info_t team_info;
	string pkg = rank_get_field_info_out_.fields(0).value();

	int ret = 0;
	ret = BlessPetUtils::parse_team_info_from_string(team_info, pkg); 

	if(ret){
		return send_err_to_player(
				player, player->cli_wait_cmd, ret);
	}
	//没有队友
	if(team_info.members_size() == 0){
		cli_out_.Clear();
		send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
		return 0;
	}

	const bless_pet_conf_t *conf_inf =
		g_bless_pet_conf_mgr.find_bless_pet_conf(team_info.pet_bless_id());
	player->temp_info.bless_pet_id  = conf_inf->pet_id;
	//去缓存中拉取队友信息
	cacheproto::cs_batch_get_users_info cache_info_in_;
	for(int i = 0; i < team_info.members_size(); i++){
		commonproto::role_info_t* pb_ptr = cache_info_in_.add_roles();
		pb_ptr->set_userid(team_info.members(i).userid());
		pb_ptr->set_u_create_tm(team_info.members(i).u_create_tm());
	}

	return g_dbproxy->send_msg(player, player->userid, player->create_tm,
			cache_cmd_ol_req_users_info, cache_info_in_);
}

int CacheBlessTeamMemberInfoCmdProcessor:: proc_pkg_from_serv_aft_get_infos_from_cache(
		player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(cache_info_out_);

	if (cache_info_out_.user_infos_size() == 0 ||
			cache_info_out_.user_infos_size() > max_team_num) {
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_get_bless_member_btl_inf_err);
	}

	std::string pkg;
	std::vector<std::string> &info_vec = 
		*(player->temp_info.bless_team_member_info);
	//清理缓存
	info_vec.clear();
	for(int i = 0; i < cache_info_out_.user_infos_size(); i++){
		//设置team 为友军
		cache_info_out_.mutable_user_infos(i)->set_team(1);
		pkg.clear();
		cache_info_out_.user_infos(i).SerializeToString(&pkg);
		info_vec .push_back(pkg);
	}

	cli_out_.Clear();
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}
