#ifndef REDIS_SERVICE_H_
#define REDIS_SERVICE_H_

//#include <redisclient/redisclient.h>
#include "../redisclient/redisclient.h"
#include "service.h"
#include "cmd_processor_interface.h"
#include "proto/ranking/rank.pb.h"


extern boost::shared_ptr<redis::client> redis_client;

extern const int cli_proto_cmd_max;

void connect_redis(void);
int redis_select_db(long db_idx);

int init_cmd_procs();

//根据id号获得该玩家的排名信息
class GetUsersRankInfoCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_get_user_rank cli_in_;
	rankproto::sc_get_user_rank cli_out_;
};

class GetUserMultiRankCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, 
			std::string& ack_body);

private:
	rankproto::cs_get_user_multi_rank cli_in_;
	rankproto::sc_get_user_multi_rank cli_out_;
};

class RankInsertLastCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm,
			const std::string& req_body,
			std::string& ack_body);
private:
	rankproto::cs_rank_insert_last cli_in_;
	rankproto::sc_rank_insert_last cli_out_;
};

//获得rank排名对应的玩家id号
class GetRankUserCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_get_rank_userid cli_in_;
	rankproto::sc_get_rank_userid cli_out_;
};

//拉取排行榜
class GetRankListCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_get_rank_list  cli_in_;
	rankproto::sc_get_rank_list  cli_out_;
};

//交换竞技场两个玩家的排名 （包括总榜和日榜）
class SwitchArenaRankCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_rank_switch_arena_user_rank  cli_in_;
	rankproto::sc_rank_switch_arena_user_rank  cli_out_;
};

//保存战报
class SaveBtlReportCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_save_battle_report 	cli_in_;
	rankproto::sc_save_battle_report	cli_out_;
};

//拉取战报
class GetBtlReportCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_get_battle_report cli_in_;
	rankproto::sc_get_battle_report cli_out_;
};

//插入一个数据
class RankInsertScoreCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_rank_user_add cli_in_;
	rankproto::sc_rank_user_add cli_out_;
};

class GetBtlReportKeyCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_get_battle_report_key cli_in_;
	rankproto::sc_get_battle_report_key cli_out_;
};

//新增hash表或更新hash表域值
class HsetInsertOrUpdateCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_hset_insert_or_update cli_in_;
	rankproto::sc_hset_insert_or_update cli_out_;
};

// 拉取hash表所有域值
class HsetGetInfoCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_hset_get_info cli_in_;
	rankproto::sc_hset_get_info cli_out_;
};

// 拉取hash表指定域值
class HsetGetFieldInfoCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_hset_get_field_info cli_in_;
	rankproto::sc_hset_get_field_info cli_out_;
};

// 向Set中插入新成员
class SetInsertMemberCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_set_insert_member cli_in_;
	rankproto::sc_set_insert_member cli_out_;
};

// 从Set中删除新成员
class SetDelMemberCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_set_del_member cli_in_;
	rankproto::sc_set_del_member cli_out_;
};

// 获取所有set成员
class SetGetAllMemberCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_set_get_all_member    cli_in_;
	rankproto::sc_set_get_all_member  cli_out_;
};

// 判断是否set成员
class SetIsMemberCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_set_is_member    cli_in_;
	rankproto::sc_set_is_member  cli_out_;
};

// 删除redis指定key集合
class RedisDelKeyCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_redis_del_key cli_in_;
	rankproto::sc_redis_del_key cli_out_;
};

class RedisDelKeyStrCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_redis_del_key_str cli_in_;
	rankproto::sc_redis_del_key_str cli_out_;
};



// 插入string成员，key已存在时不插入
class StringInsertCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_string_insert cli_in_;
	rankproto::sc_string_insert cli_out_;
};

/** 
 * @brief 删除家族信息
 */
class ClearFamilyInfoCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_clear_family_info cli_in_;
	rankproto::sc_clear_family_info cli_out_;
};

class DelUserRankCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_del_user_rank cli_in_;
	rankproto::sc_del_user_rank cli_out_;
};

/** 
 * @brief 根据分数范围查询玩家id
 */
class GetUserbyScoRgeCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_get_users_by_score_range cli_in_;
	rankproto::sc_get_users_by_score_range cli_out_;
};
/** 
 * @brief list插入数据
 */
class ListLpushMemberCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_list_lpush_member cli_in_;
	rankproto::sc_list_lpush_member cli_out_;
};

/** 
 * @brief 获取list数据
 */
class ListGetRangeMemberCmdProcessor : public CmdProcessorInterface
{
	public:
		uint32_t process(
				userid_t userid,  uint32_t u_create_tm, 
				const std::string& req_body, std::string& ack_body);
	private:
		rankproto::cs_list_get_range_member cli_in_;
		rankproto::sc_list_get_range_member cli_out_;
};
//Confirm kevin测试用 非业务协议处理函数
class TestForZjunCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_test_for_zjun cli_in_;
	rankproto::sc_test_for_zjun cli_out_;
};

class RankDumpRankCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_dump_rank_info cli_in_;
	rankproto::sc_dump_rank_info cli_out_;
};

class GetHashLenCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_dump_rank_info cli_in_;
	rankproto::sc_dump_rank_info cli_out_;
};

class SaveComBtlReportCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_save_btl_report cli_in_;
	rankproto::sc_save_btl_report cli_out_;
};

class GetComBtlReportCmdProcessor : public CmdProcessorInterface
{
public:
	uint32_t process(
			userid_t userid,  uint32_t u_create_tm, 
			const std::string& req_body, std::string& ack_body);
private:
	rankproto::cs_get_btl_report cli_in_;
	rankproto::sc_get_btl_report cli_out_;
};

#endif
