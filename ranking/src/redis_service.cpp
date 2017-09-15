#include <sys/time.h>
#include <limits>
#include <libtaomee++/inet/pdumanip.hpp>
#include <libtaomee++/bitmanip/bitmanip.hpp>
extern "C" {
#include <libtaomee/conf_parser/config.h>
#include <libtaomee/log.h>
}
#include "redis_service.h"
#include "proto_processor.h"
#include "service.h"
#include "proto/client/common.pb.h"
#include "rank_utils.h"
#include "common.h"
#include "macro_utils.h"

using namespace std;
using namespace taomee;
using namespace redis;

char g_send_buf[1024 * 1024];

const int cli_proto_cmd_max = 100;


boost::shared_ptr<redis::client> redis_client;

void connect_redis(void)
{
	const char* c_host = config_get_strval("REDIS_IP");
	int port = config_get_intval("REDIS_PORT", 6379);
	string host = "127.0.0.1";
	if(c_host)
		host = c_host;
	redis_client = boost::shared_ptr<redis::client>(new redis::client(host, port));
	DEBUG_TLOG("redis:ip=[%s],port=[%u],use_count=[%u]", c_host, port, redis_client.use_count());

#if 0 //TEST(singku) added functions for redis dump and restore
    redis_client->select(1);
    redis::client::string_type serialized_val = redis_client->dump("rankings:12:0");
    DEBUG_TLOG("dump:[%s]", serialized_val.c_str());
    redis::client::string_type ret_str = redis_client->restore("rankings:12:111", 100000, serialized_val);
    DEBUG_TLOG("restore:[%s]", ret_str.c_str());
#endif
}

/** 
 * @brief 选择redis数据库,必须在连接redis完成后调用
 * 
 * @param db_idx  数据库编号
 * 
 * @return 
 */
int redis_select_db(long db_idx)
{
    // TODO toby 服号映射
    try {
        redis_client->select(db_idx);	
    } catch (redis::protocol_error&) {
		ERROR_TLOG("select db failed, db_idx:%u", db_idx);	
        return rank_err_sys_err;
    }

    return 0;
}

int init_cmd_procs()
{
	processor->register_cmd(ranking_cmd_get_users_rank, new GetUsersRankInfoCmdProcessor);
	processor->register_cmd(ranking_cmd_get_user_multi_rank, new GetUserMultiRankCmdProcessor);
	processor->register_cmd(ranking_cmd_rank_insert_last, new RankInsertLastCmdProcessor);
	processor->register_cmd(ranking_cmd_del_user_rank, new DelUserRankCmdProcessor);

	processor->register_cmd(ranking_cmd_get_ranking_users, new GetRankUserCmdProcessor);
	processor->register_cmd(ranking_cmd_get_ranking_list, new GetRankListCmdProcessor);

	processor->register_cmd(ranking_cmd_switch_arena_ranking_user, new SwitchArenaRankCmdProcessor);
	processor->register_cmd(ranking_cmd_save_battle_report, new SaveBtlReportCmdProcessor);
	processor->register_cmd(ranking_cmd_get_battle_report, new GetBtlReportCmdProcessor);
	processor->register_cmd(ranking_cmd_ranking_insert_score, new RankInsertScoreCmdProcessor);
	processor->register_cmd(ranking_cmd_get_battle_report_key, new GetBtlReportKeyCmdProcessor);
	processor->register_cmd(ranking_cmd_hset_insert_or_update, new HsetInsertOrUpdateCmdProcessor);
	processor->register_cmd(ranking_cmd_hset_get_info, new HsetGetInfoCmdProcessor);
	processor->register_cmd(ranking_cmd_hset_get_field_info, new HsetGetFieldInfoCmdProcessor);
    processor->register_cmd(ranking_cmd_set_insert_member, new SetInsertMemberCmdProcessor);
    processor->register_cmd(ranking_cmd_set_del_member, new SetDelMemberCmdProcessor);
    processor->register_cmd(ranking_cmd_set_get_all_member, new SetGetAllMemberCmdProcessor);
    processor->register_cmd(ranking_cmd_redis_del_key, new RedisDelKeyCmdProcessor);
    processor->register_cmd(ranking_cmd_redis_del_key_str, new RedisDelKeyStrCmdProcessor);
    processor->register_cmd(ranking_cmd_set_is_member, new SetIsMemberCmdProcessor);
    processor->register_cmd(ranking_cmd_string_insert, new StringInsertCmdProcessor);
    processor->register_cmd(ranking_cmd_clear_family_info, new ClearFamilyInfoCmdProcessor);
    processor->register_cmd(ranking_cmd_get_users_by_score, new GetUserbyScoRgeCmdProcessor);
    processor->register_cmd(ranking_cmd_list_lpush_member, new ListLpushMemberCmdProcessor);
    processor->register_cmd(ranking_cmd_list_get_range_member, new ListGetRangeMemberCmdProcessor);

    processor->register_cmd(ranking_cmd_dump_rank, new RankDumpRankCmdProcessor);

    processor->register_cmd(ranking_cmd_get_hash_len, new GetHashLenCmdProcessor);

    processor->register_cmd(ranking_cmd_save_common_btl_report, new SaveComBtlReportCmdProcessor);
    processor->register_cmd(ranking_cmd_get_common_btl_report, new GetComBtlReportCmdProcessor);

    processor->register_cmd(ranking_cmd_test_for_zjun, new TestForZjunCmdProcessor);
	return 0;
}


uint32_t GetUsersRankInfoCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, 
		std::string& ack_body)
{
	PARSE_RANK_MSG;
	uint32_t order = cli_in_.order();
	uint32_t roles_size = cli_in_.roles_size();
	std::vector<role_info_t> user_vec;
	for (uint32_t i = 0; i < roles_size; ++i) {
		role_info_t role_info;
		role_info.userid = cli_in_.roles(i).userid();
		role_info.u_create_tm = cli_in_.roles(i).u_create_tm();
		user_vec.push_back(role_info);
	}

	std::vector<user_rank_info_t> rank_infos_vec;
	uint32_t ret = get_users_rank_info_from_redis(
			cli_in_.server_id(),
			cli_in_.rank_key(), order,
			user_vec, rank_infos_vec);

	if (ret) {
		FOREACH(user_vec, it) {
			user_rank_info_t tmp;
			tmp.user_id = it->userid;
			tmp.create_tm = it->u_create_tm;
			tmp.rank = 0;
			tmp.score = 0;
			rank_infos_vec.push_back(tmp);
		}
	}

	uint32_t zcount = 0;
	get_rank_count_from_redis(cli_in_.server_id(), cli_in_.rank_key(), zcount);

	cli_out_.Clear();
	FOREACH(rank_infos_vec, it) {
		commonproto::rank_player_info_t *rank_info = cli_out_.add_rank_info();
		rank_info->set_userid(it->user_id);
		rank_info->set_u_create_tm(it->create_tm);
        rank_info->set_rank(it->rank);
        rank_info->set_score(it->score);
		rank_info->set_rank_key(cli_in_.rank_key());
	}
	cli_out_.set_total_users(zcount);
	RETURN_RANK_MSG;
}

uint32_t DelUserRankCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, 
        const std::string& req_body, std::string& ack_body)
{
	PARSE_RANK_MSG;
    std::string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);
    std::string redis_key = prefix + cli_in_.key();

    uint64_t role_key = ROLE_KEY_PROTO(cli_in_.role());
    rank_del_record(cli_in_.role().server_id(), redis_key, number_to_string(role_key));
	RETURN_RANK_MSG;
}

uint32_t GetUserMultiRankCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm,
		const std::string& req_body,
		std::string& ack_body)
{
	PARSE_RANK_MSG;
	std::vector<rank_key_order_t> keys_vec;
	uint32_t keys_size = cli_in_.rank_keys_size();
	for (uint32_t i = 0; i < keys_size; ++i) {
        struct rank_key_order_t inf;
        inf.key = cli_in_.rank_keys(i).rank_key();
        inf.order = cli_in_.rank_keys(i).order();
		keys_vec.push_back(inf);
	}
	uint32_t uid = cli_in_.role().userid();
	uint32_t create_tm = cli_in_.role().u_create_tm();
	std::vector<user_key_rank_info_t> rank_info_vec;
	uint64_t role_key = ROLE_KEY(ROLE(uid, create_tm));
	uint32_t ret = get_user_multi_rank_info(
			cli_in_.server_id(),
			keys_vec,
			role_key, rank_info_vec);
	if (ret) {
		ERROR_TLOG("Get User Multi Rank Err,ret=[%u]", ret);
		return ret;
	}
	cli_out_.Clear();
	FOREACH (rank_info_vec, it) {
		commonproto::rank_player_info_t* ptr = cli_out_.add_rank_info();
		ptr->set_userid(uid);
		ptr->set_u_create_tm(create_tm);
		ptr->set_rank(it->rank);
		ptr->set_rank_key(it->rank_key);
		ptr->set_score(it->score);
	}
	RETURN_RANK_MSG;
}

uint32_t RankInsertLastCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, 
		std::string& ack_body)
{
	PARSE_RANK_MSG;
	std::string rank_key = cli_in_.rank_key();

	uint32_t order = cli_in_.order();
	struct user_key_rank_info_t rank_info;
	uint64_t role_key = ROLE_KEY(ROLE(userid, u_create_tm));
	uint32_t ret = rank_insert_last(
			cli_in_.server_id(), rank_key,
			order, role_key, rank_info);
	if (ret) {
		return ret;
	}
	cli_out_.Clear();
	commonproto::rank_player_info_t* ptr = cli_out_.mutable_rank_info();	
	ptr->set_userid(userid);
	ptr->set_u_create_tm(u_create_tm);
	ptr->set_rank(rank_info.rank);
	ptr->set_score(rank_info.score);
	ptr->set_rank_key(rank_info.rank_key);
	RETURN_RANK_MSG;
}

//根据排名值获得对应的玩家信息(id号)
uint32_t GetRankUserCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, 
		std::string& ack_body)
{
	PARSE_RANK_MSG;
	std::string rank_key = cli_in_.rank_key();
	std::vector<uint32_t> rank_list;
	for (int i = 0; i < cli_in_.rank_size(); ++i) {
		rank_list.push_back(cli_in_.rank(i));
	}
	std::vector<user_rank_info_t> user_score_vec;
	get_rank_users_score(cli_in_.server_id(), rank_list, user_score_vec, rank_key);
	cli_out_.Clear();
	FOREACH(user_score_vec, it) {
		commonproto::rank_player_info_t *player_info = cli_out_.add_player_info();
		player_info->set_userid(it->user_id);
		player_info->set_u_create_tm(it->create_tm);
		//已经加1的rank值，无需在online上再加1
		player_info->set_rank(it->rank);
		player_info->set_score(it->score);
	}
	RETURN_RANK_MSG;
}

uint32_t GetRankListCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, 
        const std::string& req_body, std::string& ack_body)
{
	cli_in_.Clear();
	if (!cli_in_.ParseFromString(req_body)) {
		ERROR_TLOG("parse err");	
		return rank_err_parse_err;
	}
	uint32_t start = cli_in_.start();
	uint32_t end = cli_in_.end();
	uint32_t order = cli_in_.order();
	std::vector<user_rank_info_t> vec_info;
	
	get_rank_list_info_from_redis(
			cli_in_.server_id(), cli_in_.rank_key(), 
			start, end, order, vec_info);
	
	cli_out_.Clear();

    bool has_self = false;
	for (uint32_t i = 0; i < vec_info.size(); ++i) {
		commonproto::rank_player_info_t *rank_ptr = cli_out_.add_user_info();
		rank_ptr->set_userid(vec_info[i].user_id);
		rank_ptr->set_u_create_tm(vec_info[i].create_tm);
		rank_ptr->set_rank(vec_info[i].rank);
		rank_ptr->set_score(vec_info[i].score);
		if (vec_info[i].user_id == userid &&
				vec_info[i].create_tm == u_create_tm) {
			has_self = true;
		}
	}

	//获取玩家自己的排名
    if (!has_self) {
        std::vector<role_info_t> role_key_vec;
		role_info_t role_info;
		role_info.userid = userid;
		role_info.u_create_tm = u_create_tm;
        role_key_vec.push_back(role_info);
        std::vector<user_rank_info_t> rank_infos_vec;
		get_users_rank_info_from_redis(cli_in_.server_id(), cli_in_.rank_key(), 
				order, role_key_vec, rank_infos_vec);
		
        if (rank_infos_vec.size() != 0) { //玩家有排名
            commonproto::rank_player_info_t *rank_info = cli_out_.mutable_self_info();
            rank_info->set_userid(rank_infos_vec[0].user_id);
			rank_info->set_u_create_tm(rank_infos_vec[0].create_tm);
            rank_info->set_rank(rank_infos_vec[0].rank);        
			rank_info->set_score(rank_infos_vec[0].score);
			rank_info->set_rank_key(cli_in_.rank_key());
        }
    }

	uint32_t total = 0;
	//获取排行榜总人数
	get_rank_count_from_redis(cli_in_.server_id(), cli_in_.rank_key(), total);
	
	cli_out_.set_count(vec_info.size());	
	cli_out_.set_total(total);
	cli_out_.SerializeToString(&ack_body);
	return 0;
}

uint32_t SwitchArenaRankCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, 
        const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;
	std::string rank_key = cli_in_.rank_key();
	std::string daily_rank_key = cli_in_.daily_rank_key();
    std::string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);
	std::string redis_key = prefix + rank_key;
	std::string daily_redis_key = prefix + daily_rank_key;

	uint64_t first_role_key = ROLE_KEY(ROLE(cli_in_.atk_role().userid(), cli_in_.atk_role().u_create_tm()));
	uint64_t sec_role_key = ROLE_KEY(ROLE(cli_in_.def_role().userid(), cli_in_.def_role().u_create_tm()));
	exchange_arena_rank(cli_in_.server_id(), daily_redis_key, redis_key, first_role_key, sec_role_key);
	return 0;
}

uint32_t SaveBtlReportCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, 
        const std::string& req_body, std::string& ack_body)
{
	PARSE_RANK_MSG;
	uint64_t def_role_key = ROLE_KEY(ROLE(cli_in_.role().userid(), cli_in_.role().u_create_tm()));
	uint64_t atk_role_key = ROLE_KEY(ROLE(userid, u_create_tm));
	uint32_t type = cli_in_.type();
	uint32_t time_stamp = cli_in_.timestamp();
	uint32_t btl_key_ttl = cli_in_.btl_key_ttl();
	uint32_t btl_key_list_ttl = cli_in_.btl_key_list_ttl();
	save_btl_report(cli_in_.server_id(), atk_role_key, def_role_key, type, time_stamp, cli_in_.pkg(), btl_key_ttl, btl_key_list_ttl);
	RETURN_RANK_MSG;
	return 0;
}

uint32_t GetBtlReportCmdProcessor :: process(
		userid_t userid, uint32_t u_create_tm, 
        const std::string& req_body, std::string& ack_body)
{
	cli_in_.Clear();
	if (!cli_in_.ParseFromString(req_body)) {
		ERROR_TLOG("parse err");	
		return rank_err_parse_err;
	}
	
	cli_out_.Clear();
	uint32_t ret = get_btl_report_pkg(cli_in_.key(), cli_out_);
	if (ret) {
		return ret;
	}
	cli_out_.SerializeToString(&ack_body);
	return 0;
}

uint32_t RankInsertScoreCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
        const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;
    std::string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);
	std::string rank_key = cli_in_.rank_key();
	std::string redis_key = prefix + rank_key;
	uint64_t score = cli_in_.score();
	uint32_t ttl = cli_in_.ttl();
	uint64_t role_key = ROLE_KEY(ROLE(userid, u_create_tm));
	uint32_t ret = rank_user_insert_score(
			cli_in_.server_id(), redis_key,
			role_key, score, ttl);
	if (ret) {
		return ret;
	}
	cli_out_.Clear();
	cli_out_.SerializeToString(&ack_body);
	return 0;
}

uint32_t HsetInsertOrUpdateCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

	uint32_t key_ttl = cli_in_.key_ttl();
    uint32_t type = cli_in_.oper_type();
    if ( type == rankproto::REDIS_CHANGE_INT_VALUE) {
        for (int i = 0; i < cli_in_.fields_size();i++) {
            const rankproto::hset_field_t &field = cli_in_.fields(i);
            int64_t value = string_to_number<int64_t>(field.value());
            hset_change_int_value(
                    cli_in_.server_id(), cli_in_.key(), field.name(), value,
					key_ttl);
        }
    } else if (type == rankproto::REDIS_INSERT_OR_UPDATE) {
        for (int i = 0; i < cli_in_.fields_size();i++) {
            const rankproto::hset_field_t &field = cli_in_.fields(i);
            hset_insert_or_update(
                    cli_in_.server_id(), cli_in_.key(), field.name(), field.value(), key_ttl);
        }
    } else if (type == rankproto::REDIS_INSERT) {
        for (int i = 0; i < cli_in_.fields_size();i++) {
            const rankproto::hset_field_t &field = cli_in_.fields(i);
            hset_insert(
                    cli_in_.server_id(), cli_in_.key(), field.name(), field.value(), key_ttl);
        }
    } else if (type == rankproto::REDIS_DELETE) {
		for (int i = 0; i < cli_in_.fields_size();i++) {
			const rankproto::hset_field_t &field = cli_in_.fields(i);

			string key = cli_in_.key();
			ostringstream redis_key;
			string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_HASHSET);
			redis_key << prefix << key;

			hset_del_field(cli_in_.server_id(), redis_key.str(), field.name());
		}
	}

	cli_out_.Clear();

    RETURN_RANK_MSG;
}

uint32_t StringInsertCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

    bool flag = false;
    flag = string_insert(
            cli_in_.server_id(), cli_in_.key(),
            cli_in_.value(), cli_in_.expire());
    cli_out_.Clear();
    cli_out_.set_flag(flag);

    RETURN_RANK_MSG;
}

uint32_t HsetGetInfoCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

    cli_out_.Clear();
    for (int i = 0; i < cli_in_.keys_size();i++) {
        const string &key = cli_in_.keys(i);
        std::vector< std::pair<string, string> > hset_info;
        hset_get_one_set_all_info(cli_in_.server_id(), key, hset_info);

        rankproto::hset_info_t *info = cli_out_.add_hset_infos();
        info->set_key(key);

        FOREACH(hset_info, it) {
            rankproto::hset_field_t *field = info->add_fields();
            field->set_name(it->first);
            field->set_value(it->second);
        }
    }

    RETURN_RANK_MSG;
}

uint32_t HsetGetFieldInfoCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_HASHSET);
    std::string redis_key = prefix + cli_in_.key();
    cli_out_.Clear();
    for (int i = 0;  i < cli_in_.field_names_size();i++) {
        std::string field_value;
        int ret = hset_get_field_value(
                cli_in_.server_id(), redis_key, cli_in_.field_names(i), field_value);
        if (ret == rank_err_sys_err) {
            return ret;
        } else if (ret == rank_err_hash_field_not_exist) {
			continue;
		}
        rankproto::hset_field_t *field = cli_out_.add_fields();
        field->set_name(cli_in_.field_names(i));
        field->set_value(field_value);
    }

    RETURN_RANK_MSG;
}

uint32_t SetInsertMemberCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

    cli_out_.Clear();

    for (int i = 0; i < cli_in_.values_size();i++) {
        const std::string &value = cli_in_.values(i);
        set_insert_member(cli_in_.server_id(), cli_in_.key(), value);
    }

    RETURN_RANK_MSG;
}

uint32_t SetDelMemberCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

    cli_out_.Clear();

    for (int i = 0; i < cli_in_.values_size();i++) {
        const std::string &value = cli_in_.values(i);
        set_del_member(cli_in_.server_id(), cli_in_.key(), value);
    }

    RETURN_RANK_MSG;
}

uint32_t SetIsMemberCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;
    cli_out_.Clear();

    for (int i = 0; i < cli_in_.values_size();i++) {
        const std::string &val = cli_in_.values(i);
        bool flag = false;
        set_is_member(cli_in_.server_id(), cli_in_.key(), val, flag);
        cli_out_.add_flags(flag);
    }

    RETURN_RANK_MSG;
}

uint32_t RedisDelKeyCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

    cli_out_.Clear();

    //std::string prefix = cli_in_.prefix();
    //const std::string &key = cli_in_.key();
    //redis_del_key(prefix, key, sub_key);

    RETURN_RANK_MSG;
}

uint32_t RedisDelKeyStrCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

    cli_out_.Clear();

    std::string prefix = get_key_prefix_str(cli_in_.type());
    std::string key = prefix + cli_in_.key();
    redis_del_key_str(cli_in_.server_id(), key);

    RETURN_RANK_MSG;
}

uint32_t SetGetAllMemberCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

    cli_out_.Clear();

    std::set<std::string> members;
    set_get_all_member(cli_in_.server_id(), cli_in_.key(),members);
    FOREACH(members, it) {
        cli_out_.add_members(*it); 
    }

    RETURN_RANK_MSG;
}

uint32_t GetBtlReportKeyCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
	cli_in_.Clear();
	if (!cli_in_.ParseFromString(req_body)) {
		ERROR_TLOG("parse err");	
		return -1;
	}
	uint32_t type = cli_in_.type();
	uint32_t count = cli_in_.count();
	uint64_t role_key = ROLE_KEY(ROLE(cli_in_.role().userid(), cli_in_.role().u_create_tm()));
	uint32_t flag = cli_in_.flag();
	cli_out_.Clear();
	uint32_t ret = get_btl_report_key(
			cli_in_.server_id(), role_key, 
			type, cli_out_, flag, count);
	if (ret) {
		return ret;
	}
	cli_out_.SerializeToString(&ack_body);
	return 0;
}

uint32_t ClearFamilyInfoCmdProcessor :: process (
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;
    for(int i = 0; i < cli_in_.family_ids_size();i++) {
        uint32_t family_id = cli_in_.family_ids(i);
        ostringstream redis_key;    
        // 取家族名字
        string family_name;
        string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_HASHSET);
        redis_key << prefix << rankproto::HASHSET_FAMILY_ID_NAME_MAP <<":0";
        hset_get_field_value(
            cli_in_.server_id(), redis_key.str(), 
            number_to_string(family_id), family_name);

        // 删除家族成员id缓存
        redis_key.str("");
        prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_SET);
        redis_key << prefix << rankproto::SET_FAMILY_ONLINE_USERIDS << 
            ":" << family_id;
        redis_del_key_str(cli_in_.server_id(), redis_key.str());

        redis_key.str("");
        redis_key << prefix << rankproto::SET_FAMILY_ALL_USERIDS << 
            ":" << family_id;
        redis_del_key_str(cli_in_.server_id(), redis_key.str());

        // 删除家族名字记录
        redis_key.str("");
        redis_key << prefix << rankproto::SET_FAMILY_NAME << ":0";
        set_del_member(cli_in_.server_id(), redis_key.str(), family_name);

        // 删除家族ID记录
        redis_key.str("");
        redis_key << prefix << rankproto::SET_FAMILY_ID << ":0";
        set_del_member(cli_in_.server_id(), redis_key.str(), family_name);

        // 删除家族-id名字记录
        redis_key.str("");
        redis_key << prefix << rankproto::HASHSET_FAMILY_ID_NAME_MAP << ":0";
        hset_del_field(
                cli_in_.server_id(), redis_key.str(), number_to_string(family_id));

        // 删除家族名字-id记录
        redis_key.str("");
        redis_key << prefix << rankproto::HASHSET_FAMILY_NAME_ID_MAP << ":0";
        hset_del_field(cli_in_.server_id(), redis_key.str(), family_name);
    }
    
    RETURN_RANK_MSG;
}

uint32_t GetUserbyScoRgeCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;
	std::string rank_key = cli_in_.rank_key();
    std::string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);
	std::string redis_key = prefix + rank_key;
	uint64_t low_score = cli_in_.low_score();
	uint64_t high_score = cli_in_.high_score();
	uint32_t role_size = cli_in_.role_list_size();
	uint64_t unit_power = cli_in_.unit_power();
	std::set<uint64_t> uid_set;
	for (uint32_t i = 0; i < role_size; ++i) {
		uint64_t role_key = ROLE_KEY(ROLE(cli_in_.role_list(i).userid(), cli_in_.role_list(i).u_create_tm()));
		uid_set.insert(role_key);
	}
	//排除同一userid玩家
	uint64_t role_key = ROLE_KEY(ROLE(userid, 0));
	uid_set.insert(role_key);

	std::vector<user_rank_info_t> power_infos;
	uint32_t count = 0;
	do {
		++count;
		if (count > 2000) {
			ERROR_TLOG("Match Get Limit,LowScore=[%llu],HighScore=[%llu],unit_power=[%llu]"
					",Cnt=[%u],uid=[%u],u_ctm=[%u]", 
					low_score, high_score, unit_power, count, userid, u_create_tm);
			break;
		}
		if (count > 1) {
			if (low_score > unit_power) {
				low_score -= unit_power;
			} else {
				low_score = 0;
			}
			high_score += unit_power;
		}
		get_users_info_by_score_range(
				cli_in_.server_id(),
				redis_key,
				low_score, high_score, 
				power_infos, uid_set, unit_power);

	} while (power_infos.empty());
	if (power_infos.empty()) {
		return rank_err_exped_match_failed;
	}
	cli_out_.Clear();
	uint32_t index = rand() % power_infos.size();
	cli_out_.mutable_player_info()->set_userid(power_infos[index].user_id);
	cli_out_.mutable_player_info()->set_u_create_tm(power_infos[index].create_tm);
	cli_out_.mutable_player_info()->set_score(power_infos[index].score);
	cli_out_.mutable_player_info()->set_rank(power_infos[index].rank);
	cli_out_.mutable_player_info()->set_rank_key(redis_key);
    RETURN_RANK_MSG;
}

//Confirm kevin测试用 非业务协议处理函数
uint32_t TestForZjunCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, 
		const std::string& req_body, std::string& ack_body)
{
	PARSE_RANK_MSG;
	uint32_t key = cli_in_.key();
	uint32_t sub_key = cli_in_.sub_key();

	ostringstream redis_key;
	redis_key << "rankings:" << key << ":" << sub_key;
	std::string str_userid = number_to_string(userid);
	uint32_t u_rank = 0; u_rank = 0;
	std::vector<string> result;
	try {
		u_rank = redis_client->zrank(redis_key.str(), str_userid);
	} catch (redis::protocol_error&) {
	} catch (boost::bad_lexical_cast&) {
	}
	cli_out_.Clear();
    RETURN_RANK_MSG;
}

uint32_t ListLpushMemberCmdProcessor::process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

    cli_out_.Clear();
	int ret = 0;

	std::string rank_key = cli_in_.key();
	uint32_t max_len = cli_in_.max();
	for(int i = 0; i < cli_in_.value_size(); i++){
		ret = list_lpush_member(
				cli_in_.server_id(), rank_key,
				cli_in_.value(i));
		if (ret) {
			return ret;
		}
	}

	if(max_len > 1){
		ret = list_trim_member(
				cli_in_.server_id(), rank_key,
				0, max_len - 1);
		if (ret) {
			return ret;
		}
	}

    RETURN_RANK_MSG;
}

uint32_t ListGetRangeMemberCmdProcessor::process(
		 userid_t userid,  uint32_t u_create_tm,
		 const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;

    cli_out_.Clear();
	int ret = 0;

	std::string rank_key = cli_in_.key();
	int32_t start = cli_in_.start();
	int32_t end = cli_in_.end();
	std::vector<std::string> out;
	out.clear();
	ret = list_get_range_member(
			 cli_in_.server_id(), rank_key,
			 start, end, out);
	if (ret) {
		return ret;
	}
	cli_out_.set_key(rank_key);

	FOREACH(out, it){
		cli_out_.add_value(*it);
	}

    RETURN_RANK_MSG;
}

uint32_t RankDumpRankCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm,
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;
	std::vector<dump_key_info_t> key_vec;
	for (int i = 0; i < cli_in_.keys_info_size(); ++i) {
		const rankproto::dump_rank_keys_info_t& inf = cli_in_.keys_info(i);
		dump_key_info_t tmp;
        tmp.orig_key.key = inf.orig_key().key();
        tmp.orig_key.sub_key = inf.orig_key().sub_key();
        tmp.new_key.key = inf.new_key().key();
        tmp.new_key.sub_key = inf.new_key().sub_key();
        tmp.del_orig_key = inf.del_orig_key();

		key_vec.push_back(tmp);
	}
	uint32_t ret = dump_rank(key_vec, cli_in_.server_id());
	if (ret) {
		ERROR_TLOG("Dump Rank Err");
		return ret;
	}
    RETURN_RANK_MSG;
}

uint32_t GetHashLenCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm,
		const std::string& req_body, std::string& ack_body)
{
	return 0;
}

uint32_t SaveComBtlReportCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm,
		const std::string& req_body, std::string& ack_body)
{
	PARSE_RANK_MSG;
	for (int i = 0; i < cli_in_.btl_report_size(); ++i) {
		ostringstream redis_key01, redis_key02;
		const commonproto::btl_report_pkg_t& pb_inf = cli_in_.btl_report(i);
		uint32_t userid = pb_inf.userid();
		uint32_t create_tm = pb_inf.create_tm();
		uint32_t type = pb_inf.type();
		uint64_t role_key = ROLE_KEY(ROLE(userid, create_tm));
		uint32_t generate_time = pb_inf.generate_time();
		uint32_t ttl = pb_inf.ttl();
		const std::string& pkg = pb_inf.pkg();
		redis_key01 << "combtlkey:" << generate_time << ":" << role_key;
		redis_key02 << "btlreport:" << type << ":" << role_key;
		try {
			redis_client->set(redis_key01.str(), pkg);
			redis_client->expire(redis_key01.str(), ttl);
		} catch (std::exception&) {
			return rank_err_redis_not_available;
		}

		try {
			int len = redis_client->llen(redis_key02.str());
			while (len >= BTL_REP_KEY_LIST_MAX_LEN) {
				try {
					string rm_btl_key = redis_client->lpop(redis_key02.str());
					redis_client->del(rm_btl_key);		
				} catch (exception&) {
					ERROR_TLOG("pop btl_key,err;[%s]", redis_key02.str().c_str());
				}
				--len;
			}

			redis_client->rpush(redis_key02.str(), redis_key01.str());
			redis_client->expire(redis_key02.str(), ttl);
		} catch (std::exception&) {
			return rank_err_redis_not_available;
		}
	}
	return 0;
}

uint32_t GetComBtlReportCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm,
		const std::string& req_body, std::string& ack_body)
{
    PARSE_RANK_MSG;
	cli_out_.Clear();
	uint64_t role_key = ROLE_KEY(ROLE(cli_in_.userid(), cli_in_.create_tm()));
	uint32_t type = cli_in_.type();
	uint32_t count = cli_in_.count();
	int ret = redis_select_db(cli_in_.server_id());
	if (ret) {
		return ret;
	}
	ostringstream redis_key;
	redis_key << "btlreport:" << type << ":" << role_key;

	try {
		int len = redis_client->llen(redis_key.str());
		for (uint32_t i = len; i > 0 && count > 0; --i) {
			string key = redis_client->lindex(redis_key.str(), i - 1);
			string pkg = redis_client->get(key);
			if (pkg.compare(redis::client::missing_value())) {
				rankproto::btl_report_info* btl_ptr = cli_out_.add_btl_inf();
				btl_ptr->set_key(key);
				btl_ptr->set_pkg(pkg);
			} else {
				//战报可能因为过期而不存在
				redis_client->lrem(redis_key.str(), -1, key);
				continue;
			}
			--count;
		}
	} catch (redis::protocol_error&) {
		return rank_err_redis_not_available;
	} catch (boost::bad_lexical_cast&) {
		return rank_err_get_btl_key_err;
	}
	RETURN_RANK_MSG;
}
