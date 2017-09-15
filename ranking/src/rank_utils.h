#ifndef RANK_UTILS_H_
#define RANK_UTILS_H_

#include <stdint.h>
#include <vector>
#include <set>
#include "common.h"

struct user_rank_info_t
{
	uint32_t user_id;
	uint32_t create_tm;
	uint32_t rank;
	uint64_t score;
};

struct keys_info_t {
	uint32_t key;
	uint32_t sub_key;
};

struct dump_key_info_t {
    keys_info_t orig_key;
    keys_info_t new_key;
    uint32_t del_orig_key;
};

//单玩家sub_key对应的排名与分数信息
//struct user_each_key_rank_info_t
struct user_key_rank_info_t
{
	std::string rank_key;
	uint32_t rank;
	uint64_t score;
};

struct user_power_info_t
{
	uint32_t userid;
	uint32_t power;
};

const int BTL_REP_KEY_LIST_MAX_LEN = 10;

//插入到最后一名,前一名的得分+-1作为本人的积分
uint32_t rank_insert_last(
		uint32_t server_id,
		std::string redis_key,
		uint32_t order, uint64_t role_key,
		struct user_key_rank_info_t& rank_info);

struct rank_key_order_t {
    std::string key;
    uint32_t order;
};

uint32_t get_user_multi_rank_info(
		uint32_t server_id,
		const std::vector<rank_key_order_t>& keys_vec,
		uint64_t role_key,
		std::vector<user_key_rank_info_t>& rank_info_vec);

/**
 *@brief 根据key-sub_key获取指定玩家的排名
		(若找不到排名，则返回rank,score都为0)
 *@param key 
 *@param sub_key 
 *@param order
 *@param user_ids:玩家的id数组，用于获取的玩家排名信息
 *@param rank_infos_vec:返回的指定的玩家排名信息（id,rank,score）的数组
 *@return 成功：0
 */
uint32_t get_users_rank_info_from_redis(
		uint32_t server_id, std::string rank_key, 
		uint32_t order, 
		const std::vector<role_info_t>& user_ids,
		std::vector<user_rank_info_t>& rank_infos_vec);

/**
 *@brief 根据key-sub_key获取排名数量
 *@param key 
 *@param sub_key 
 *@param count 返回排名数量
 *@param start 
 *@param end
 *@return 成功：0
 */
uint32_t get_rank_count_from_redis(
		uint32_t server_id,
		std::string rank_key,
		uint32_t& count, double start = 0, 
        /*double end = 0x7FFFFFFFFFFFFFFF);*/
		double end = 1.7976931348623158e+308);

/**
 *@brief 根据key-sub_key获取排名数量
 *@param key 
 *@param sub_key 
 *@param start
 *@param end
 *@param vec_info: 返回的排行榜玩家信息
 *@return 成功：0
 */
uint32_t get_rank_list_info_from_redis(
		uint32_t server_id,
		std::string redis_key, 
        uint32_t start, uint32_t end, 
        uint32_t order, std::vector<user_rank_info_t>& vec_info); 


/**
 *@brief 根据排名计算出对应玩家的id号和分数信息
 *@param rank_list: 玩家们的排名
 *@param user_score_vec: 返回的玩家们的id号和分数信息
 *@param key 
 *@param sub_key 
 *@param end
 *@return 成功：0
 */
uint32_t get_rank_users_score(
	uint32_t server_id,
	const std::vector<uint32_t>& rank_list,
	std::vector<user_rank_info_t>& user_score_vec,
	std::string rank_key);

/**
 *@brief 根据key-sub_key交换排名
 *@param key 
 *@param sub_key 
 *@param start
 *@param end
 *@param vec_info: 返回的排行榜玩家信息
 *@return 成功：0
 */
uint32_t exchange_arena_rank(
		uint32_t server_id,
		std::string redis_key,
		std::string redis_key01,
		uint64_t first_userid, uint64_t second_userid);

/**
 *@brief 根据score 范围内的相关信息
 */
uint32_t get_users_info_by_score_range(
		uint32_t server_id,
		std::string redis_key,
		uint64_t& low_score, uint64_t& high_score,
		std::vector<user_rank_info_t>& power_infos_vec,
		const std::set<uint64_t>& uids,
		const uint64_t uint_power);

/**
 *@brief 根据key-sub_key, score 插入一行排名
 *@param key 
 *@param sub_key 
 *@param score
 *@return 成功：0
 */
uint32_t rank_user_insert_score(
		uint32_t server_id,
		std::string redis_key, 
        uint64_t role_key, uint64_t score, uint32_t ttl);

/**
 *@brief 根据对战的两个玩家的id,生成一份战报,保存在hash中；并保存战报key到玩家的链表中
 * 战报key 为 battle_key ,生成规则：时间戳 + atk_id + def_id 
 *@param atk_id : 玩家米米号
 *@param def_id : ai玩家米米号
 *@param type: 战报类型
 *@param pkg : 打包好的战报数据
 *@param btl_key_ttl: 战报key 的ttl
 *@param btl_key_list_ttl: 保存战报key的链表的ttl
 *@return 成功：0
 */
uint32_t save_btl_report(uint32_t server_id, uint64_t atk_role_key, 
		uint64_t def_role_key, uint32_t type, 
        uint32_t time_stamp, const std::string& pkg,
		uint32_t btl_key_ttl, uint32_t btl_key_list_ttl); 

/**
 *@brief 获取战报key的值
 *@param uid : 玩家米米号
 *@param type: 战报类型
 *@param flag : 是否同时拉取出战报：0，不拉；1；拉取
 *@param count : 需要获取的key的数量 ,默认拉取10份
 *@return 成功：0
 */
uint32_t get_btl_report_key(uint32_t server_id, 
		uint64_t role_key, uint32_t type, 
        rankproto::sc_get_battle_report_key& cli_out_, 
        uint32_t flag = 0, uint32_t count = 10);

/** 
 * @brief 根据 key sub_key 覆盖更新对应hash表的域值,如果不存在对应hash表则先创建
 *        再更新
 * 
 * @param key 
 * @param sub_key  
 * @param field_name        hash表域名
 * @param field_value       hash表域值
 * 
 * @return 
 */
int hset_insert_or_update(uint32_t server_id, const std::string &key, 
        const std::string &field_name, const std::string &field_value,
		const uint32_t ttl);

bool hset_insert(uint32_t server_id, const std::string &key, 
        const std::string &field_name, const std::string &field_value,
		const uint32_t ttl);

/** 
 * @brief 根据 key sub_key 新增对应hash表域值,如果存在则不更新
 *        
 * 
 * @param key 
 * @param sub_key  
 * @param field_name        hash表域名
 * @param field_value       hash表域值
 * 
 * @return 
 */
bool string_insert(uint32_t server_id, const std::string &key, 
        const std::string &value, uint32_t expire);

/** 
 * @brief 根据 key sub_key 增量更新对应hash表的域值,如果不存在对应hash表则先创建
 *        再更新
 * 
 * @param key 
 * @param sub_key 
 * @param field_name 
 * @param field_value 
 * 
 * @return 
 */
int hset_change_int_value(uint32_t server_id, const std::string &key,
        const std::string &field_name, const int64_t field_value, const uint32_t ttl);

/** 
 * @brief 根据key sub_key查询指定hash表的全部域信息
 * 
 * @param key 
 * @param out       返回的域信息
 * 
 * @return 
 */
int hset_get_one_set_all_info(uint32_t server_id, const std::string &key,
        std::vector< std::pair<std::string, std::string> > &out);

/** 
 * @brief 根据 key, field_name查询对应的value记录
 * 
 * @param key 
 * @param field_name 
 * @param field_value 
 * 
 * @return 
 */
int hset_get_field_value(
        uint32_t server_id, const std::string &key, const std::string &field_name, 
        std::string &field_value);


/** 
 * @brief 根据 key查询对应hashset的域数量
 * 
 * @param key 
 * @param num
 * 
 * @return 
 */
int hset_get_field_num(uint32_t server_id, const std::string &key, uint32_t &num);

/** 
 * @brief 根据 key删除hash表对应域值
 * 
 * @param key 
 * @param field_name        hash表域名
 * 
 * @return 
 */
int hset_del_field(
        uint32_t server_id, const std::string &key, const std::string &field_name);

/** 
 * @brief 向Set中插入新元素
 * 
 * @param key 
 * @param value 
 * 
 * @return 
 */
int set_insert_member(uint32_t server_id, const std::string &key,
        const std::string &value);

/** 
 * @brief 从Set中删除元素
 * 
 * @param key 
 * @param sub_key 
 * @param value 
 * 
 * @return 
 */
int set_del_member(uint32_t server_id, const std::string &key,
        const std::string &value);

/** 
 * @brief 拉取一个Set中的所有元素集合
 * 
 * @param key 
 * @param std::pair<std::string 
 * @param out 
 * 
 * @return 
 */
int set_get_all_member(uint32_t server_id, const std::string &key,
        std::set<std::string> &out);

/** 
 * @brief 判断是否set成员
 * 
 * @param key 
 * @param value 
 * 
 * @return 
 */
int set_is_member(uint32_t server_id, const std::string &key,
        const std::string &value, bool &flag);

/** 
 * @brief 删除redis中的指定key集合
 * 
 * @param key 
 * @param sub_key 
 * @param value 
 * 
 * @return 
 */
/*int redis_del_key(const std::string &prefix, const uint32_t key, */
/*const uint32_t sub_key);*/

/** 
 * @brief 删除redis中的指定key集合 
 * 
 * @param key 
 * 
 * @return 
 */
int redis_del_key_str(uint32_t server_id, const std::string key);

/*bool string_insert(const uint32_t key, const uint32_t sub_key, */
/*const std::string &value, uint32_t expire);*/

bool string_insert(uint32_t server_id, const std::string &key, 
        const std::string &value, uint32_t expire);

/** 
 * @brief 取redis key 前缀
 * 
 * @param type 
 * 
 * @return 
 */
std::string get_key_prefix_str(uint32_t type);

/**
 *@brief 排行删除记录
 *@param key 
 */
uint32_t rank_del_record(
        uint32_t server_id, const std::string &key, const std::string &str_uid);

uint32_t get_btl_report_pkg(
	std::string key, rankproto::sc_get_battle_report& cli_out_);

/** 
 * @brief 向List头部插入新元素
 * 
 * @param key 
 * @param value 
 * 
 * @return 
 */
uint32_t list_lpush_member(uint32_t server_id, const std::string &key, const std::string &value);
/** 
 * @brief 从List头部删除元素
 * 
 * @param key 
 * @param value 
 * 
 * @return 
 */
uint32_t list_lpop_member(uint32_t server_id, const std::string &key);
/** 
 * @brief 获取list元素range：start end
 * 
 * @param key 
 * @param start 
 * @param end 
 * @param string_vector 
 * 
 * @return 
 */
int list_get_range_member(uint32_t server_id, const std::string &key,
	   	const int start, const int end, std::vector<std::string> &out);
/** 
 * @brief 剪切list元素range：start end
 * 
 * @param key 
 * @param start 
 * @param end 
 * 
 * @return 
 */
uint32_t list_trim_member(uint32_t server_id, const std::string &key,
	   	const int start, const int end);
/** 
 * @brief 获取list元素个数
 * 
 * @param key 
 * @param len 
 * 
 * @return 
 */
uint32_t list_get_length(uint32_t server_id, const std::string &key, uint32_t & len);

uint32_t dump_rank(std::vector<dump_key_info_t>& key_vec, uint32_t server_id);
#endif
