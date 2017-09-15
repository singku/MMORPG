#include "rank_utils.h"

#include <string>

#include "redis_service.h"
#include "common.h"
#include <sstream>
#include <libtaomee/timer.h>
#include <libtaomee/utils.h>
#include "time.h"
#include "proto/ranking/rank.pb.h"

using namespace std;

uint32_t get_user_multi_rank_info(
		uint32_t server_id,
		const std::vector<rank_key_order_t>& keys_vec,
		uint64_t role_key,
		std::vector<user_key_rank_info_t>& rank_info_vec)
{
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
	rank_info_vec.clear();
	string str_role_key = number_to_string(role_key);
	std::string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);
	FOREACH (keys_vec, it) {
		struct user_key_rank_info_t rank_info;
		rank_info.rank = 0;
		rank_info.score = 0;
		bool rank_found_flag = true;
		rank_info.rank_key = (*it).key;
        uint32_t order = (*it).order;
		std::string redis_key = prefix + rank_info.rank_key;
		uint32_t rank = 0;
		try {
			if (order) {
				rank = redis_client->zrevrank(redis_key, str_role_key);
			} else {
				rank = redis_client->zrank(redis_key, str_role_key);
			}
		} catch (redis::protocol_error&) {
			rank_info.rank = 0;
			rank_info.score = 0;
			rank_found_flag = false;
		} catch (boost::bad_lexical_cast&) {
			rank_info.rank = 0;
			rank_info.score = 0;
			rank_found_flag = false;
		}
		if (rank_found_flag) {
			try {
				rank_info.score = redis_client->zscore(redis_key, str_role_key);
			} catch (boost::bad_lexical_cast&) {
				rank_info.score = 0;
			}
			if (rank_info.score == 0) {
				rank_info.rank = 0;
			} else {
				rank_info.rank = rank + 1;
			}
		}
		rank_info_vec.push_back(rank_info);
	}
	return 0;
}

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
		const std::vector<role_info_t>& user_vec,
		std::vector<user_rank_info_t>& rank_infos_vec)
{
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
    std::string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);
	std::string redis_key = prefix + rank_key;
	rank_infos_vec.clear();
	uint32_t user_vec_size = user_vec.size();
	for (uint32_t i = 0; i < user_vec_size; ++i) {
		uint64_t role_key = ROLE_KEY(ROLE(user_vec[i].userid, user_vec[i].u_create_tm));
		string str_role_key = number_to_string(role_key);
		struct user_rank_info_t rank_info = {0};
		rank_info.user_id = user_vec[i].userid;
		rank_info.create_tm = user_vec[i].u_create_tm;
		try {
			if (order) {
				rank_info.rank = redis_client->zrevrank(redis_key, str_role_key);
			} else {
				rank_info.rank = redis_client->zrank(redis_key, str_role_key);
			}
		} catch (exception &) {
			ERROR_TLOG("rank_err,uid=[%u],u_create_tm=[%u]", 
					user_vec[i].userid, user_vec[i].u_create_tm);
            continue;//这个人的排名查找错误
		}

		//若找到了排名，再找分数
        try {
            rank_info.score = redis_client->zscore(redis_key, str_role_key);
        } catch (exception &) {
			ERROR_TLOG("score_err,rank=[%u]", rank_info.rank);
            continue;//这个人的分数查找错误
        }
        //redis的排名从0开始的
        rank_info.rank += 1;
		rank_infos_vec.push_back(rank_info);
	}
    //提供了用户,却一个都没查到
    if (user_vec.size() && !rank_infos_vec.size()) {
        return rank_err_get_rank_err;
    }
	return 0;
}

uint32_t rank_insert_last(
		uint32_t server_id,
		std::string rank_key,
		uint32_t order, uint64_t role_key,
		struct user_key_rank_info_t& rank_info)
{
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
    std::string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);
	std::string redis_key = prefix + rank_key;
	std::vector<string> result;
	string str_role_key = number_to_string(role_key);
	uint64_t score = 0;
	uint32_t rank = 0;
    bool key_exist = true;
    try {
        if (order) {
            redis_client->zrange(redis_key, 0, 0, result);
        } else {
            redis_client->zrevrange(redis_key, 0, 0, result);
        }
    } catch (redis::key_error&)  {//先尝试捕获key不存在的异常
        key_exist = false;

    } catch (exception &) {
        return rank_err_insert_rank_last;
    }

	TRACE_TLOG("zjun0311,key_exist=[%u],size=[%u]", key_exist, result.size());
	if (!key_exist || result.size() == 0) {//空集 则最后一名就是第一名
        score = 1;
        rank = 1;
        try {
            redis_client->zadd(redis_key, score, str_role_key);
        } catch (exception &) {
            ERROR_TLOG("insert first rank, but err");
            return rank_err_insert_rank_last;
        }
        rank_info.rank = rank;
        rank_info.score = score;
		rank_info.rank_key = rank_key;
        return 0;
    }

	uint64_t last_role_key = string_to_number<uint64_t>(result[0]);
	role_info_t role_info = KEY_ROLE(last_role_key);
	
	std::vector<role_info_t> user_ids;
	user_ids.push_back(role_info);
	std::vector<user_rank_info_t> rank_infos_vec;
	get_users_rank_info_from_redis(server_id, rank_key, order, user_ids, rank_infos_vec);
	if (rank_infos_vec.size()) {
		score = rank_infos_vec[0].score;
		rank = rank_infos_vec[0].rank;

	} else {//当前最后一名的信息没有查到
        return rank_err_insert_rank_last;
    }

    //调整最新最后一名的积分与排名
	if (order) {
		if (score > 0) {
			--score;
		}
	} else {
		++score;
	}
	try {
		redis_client->zadd(redis_key, score, str_role_key);
	} catch (redis::protocol_error&) {
		return rank_err_insert_rank_last;
	}
	//当前最后一名的下一名赋给玩家
	rank_info.rank = rank+1;
	rank_info.score = score;
	rank_info.rank_key = rank_key;
	return 0;
}

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
		uint32_t& count, double start, 
		double end)
{
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
    std::string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);
	std::string redis_key = prefix + rank_key;
    int range_modification = 0;
    try {
        count = redis_client->zcount(redis_key, start, end, range_modification);
    } catch (redis::protocol_error&) {
        //key not found
		return rank_err_get_rank_count_err;
    } catch (boost::bad_lexical_cast&) {
		return rank_err_get_rank_count_err;
	}
    return 0;
}

uint32_t get_rank_list_info_from_redis(
		uint32_t server_id, std::string rank_key, 
		uint32_t start, uint32_t end, 
        uint32_t order, std::vector<user_rank_info_t>& vec_info)
{
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
    std::string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);
	std::string redis_key = prefix + rank_key;
    redis::client::string_score_vector result;
	try {
		if (order) {
			redis_client->zrevrange(redis_key, start, end, result);
		} else {
			redis_client->zrange(redis_key, start, end, result);
		}
	} catch (redis::protocol_error&) {
		return rank_err_redis_key_not_found;
	} catch (redis::key_error&) {
		return rank_err_redis_key_not_found;
	} catch (boost::bad_lexical_cast&) {
		return rank_err_lexical_cast_err;
	}
    
	vec_info.clear();
    uint32_t i = 0;
    FOREACH(result, it) {
        redis::client::string_score_pair &vpair = *it;
        uint64_t role_key = string_to_number<uint64_t>(vpair.first);
		role_info_t role_info = KEY_ROLE(role_key);
        user_rank_info_t tmp;
        tmp.user_id = role_info.userid;
		tmp.create_tm = role_info.u_create_tm;
        tmp.rank = start+i+1;
        tmp.score = vpair.second;
        vec_info.push_back(tmp);
        i++;
    }
	return 0;
}

uint32_t get_rank_users_score(
	uint32_t server_id,
	const std::vector<uint32_t>& rank_list,
	std::vector<user_rank_info_t>& user_score_vec,
	std::string rank_key)
{
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
    std::string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);
	std::string redis_key = prefix + rank_key;
	user_score_vec.clear();
	for (uint32_t i = 0; i < rank_list.size(); ++i) {
		std::vector<string> result;
		try {
			redis_client->zrange(
					redis_key, rank_list[i] - 1, 
					rank_list[i] - 1, result);
		} catch (exception&) {
			ERROR_TLOG("not get user_score info by zrange;rank=[%u]", 
					rank_list[i] - 1);
			continue;
		}
		if (0 == result.size()) {
			ERROR_TLOG("not get user_score info by zrange02;rank=[%u]", 
					rank_list[i] - 1);
			continue;
		}
		uint64_t role_key = string_to_number<uint64_t>(result[0]);
		role_info_t role_info = KEY_ROLE(role_key);
		
		uint64_t score = 0;
		try {
			score = redis_client->zscore(redis_key, result[0]);
		} catch (boost::bad_lexical_cast&) {
			ERROR_TLOG("get_score throw bad lexical cast;uid=[%u],create_tm=[%u]", 
					role_info.userid, role_info.u_create_tm);
			continue;
		}
		user_rank_info_t tmp;
		tmp.user_id = role_info.userid;
		tmp.create_tm = role_info.u_create_tm;
		tmp.rank = rank_list[i];
		tmp.score = score;
		user_score_vec.push_back(tmp);
	}
	return 0;
}

/**
 *@brief 根据key-sub_key交换排名
 *@param server_id 
 *@param redis_key 日榜的key
 *@param redis_key01 总榜的key
 *@param first_role_key 自己的role_key
 *@param second_role_key 对方的role_key
 *@return 成功：0
 */
uint32_t exchange_arena_rank(
		uint32_t server_id,
		std::string redis_key,
		std::string redis_key01,
		uint64_t first_role_key, uint64_t second_role_key)
{
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
	string str_role_key = number_to_string(first_role_key);
	string str_ai_role_key = number_to_string(second_role_key);
	//应该从总榜中取出分数 , 总榜 redis_key01
	uint64_t user_score = 0, ai_score = 0, user_rank = 0, ai_rank = 0;
	try {
		//uint64_t user_score = redis_client->zscore(redis_key.str(), str_role_key);
		//uint64_t ai_score = redis_client->zscore(redis_key.str(), str_ai_role_key);
		//uint64_t user_score = 0, ai_score = 0;
		user_score = redis_client->zscore(redis_key01, str_role_key);
		ai_score = redis_client->zscore(redis_key01, str_ai_role_key);
		redis_client->zrem(redis_key01, str_role_key);
		redis_client->zadd(redis_key01, ai_score, str_role_key);
		redis_client->zrem(redis_key01, str_ai_role_key);
		redis_client->zadd(redis_key01, user_score, str_ai_role_key);


		//从新的总榜中取出 玩家自己的名次，作为天榜的分数
		user_rank = redis_client->zrank(redis_key01, str_role_key);
		user_rank++;
		redis_client->zrem(redis_key, str_role_key);
		redis_client->zadd(redis_key, user_rank, str_role_key);
	} catch (redis::protocol_error&) {
		return rank_err_redis_not_available;
	} catch (boost::bad_lexical_cast&) {
		return rank_err_get_redis_socre_err;
	}
	//判断被揍的人是否已经在天榜中有名; 如果有名，则更新他的名次；若无名，则不更新
	//found_rank_flag :表示被揍的人在天榜中是否有排名 
	bool found_rank_flag = false;
	//
	try {
		//先判断被揍的人 是否在天榜中有名
		redis_client->zrank(redis_key, str_ai_role_key);
		found_rank_flag = true;
	} catch (redis::protocol_error&) {
		found_rank_flag = false;
	} catch (boost::bad_lexical_cast&) {
		found_rank_flag = false;
	}
	if (found_rank_flag) {
		try {
			//从新的总榜中取出 被揍对手的名次，作为天榜中的分数
			ai_rank = redis_client->zrank(redis_key01, str_ai_role_key);
			ai_rank++;
			redis_client->zrem(redis_key, str_ai_role_key);
			redis_client->zadd(redis_key, ai_rank, str_ai_role_key);
		} catch (exception &) {
			ERROR_TLOG("Add AiUser Rank Failed");
		}
	}
	return 0;
}

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
	uint64_t role_key, uint64_t score, 
	uint32_t ttl)
{
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
	uint32_t flag = 0;
	string str_role_key = number_to_string(role_key);
	try {
		redis_client->zadd(redis_key, score, str_role_key);	
		flag = 1;
	} catch (redis::protocol_error&) {
		return rank_err_redis_not_available;
	}
	if (1 == flag) {
		try {
			redis_client->expire(redis_key, ttl);
		} catch (redis::protocol_error&) {
			return rank_err_redis_not_available;
		}
	}
	return 0;
}

/**
 *@brief 排行删除记录
 *@param key 
 */
uint32_t rank_del_record(uint32_t server_id, const string &key, const string &str_uid)
{
	try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
		redis_client->zrem(key, str_uid);
	} catch (redis::protocol_error&) {
		return rank_err_redis_not_available;
	}

	return 0;
}

/** 
 * @brief 根据 key覆盖更新对应hash表的域值,如果不存在对应hash表则先创建
 *        再更新
 * 
 * @param key 
 * @param field_name        hash表域名
 * @param field_value       hash表域值
 * 
 * @return 
 */
int hset_insert_or_update(uint32_t server_id, const string &key, 
        const string &field_name, const string &field_value,
		const uint32_t ttl)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_HASHSET);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        redis_client->hset(redis_key.str(), field_name, field_value);	
		if(ttl){
			redis_client->expire(redis_key.str(), ttl);
		}
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }
	return 0;
}

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
bool hset_insert(uint32_t server_id, const string &key, 
        const string &field_name, const string &field_value,
		const uint32_t ttl)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_HASHSET);
	redis_key << prefix << key;

    bool flag = false;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        flag = redis_client->hsetnx(redis_key.str(), field_name, field_value);	
		if(flag && ttl){
			redis_client->expire(redis_key.str(), ttl);
		}
    } catch (redis::protocol_error&) {
        flag = false;
        return flag;
    }

	return flag;
}

/** 
 * @brief 根据 key删除hash表对应域值
 * 
 * @param key 
 * @param field_name        hash表域名
 * 
 * @return 
 */
int hset_del_field(
        uint32_t server_id, const string &key, const string &field_name)
{
    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
        redis_client->hdel(key, field_name);	
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }
	return 0;
}

/** 
 * @brief 根据 key, field_name查询对应的value记录
 * 
 * @param key 
 * @param sub_key  
 * @param field_name        hash表域名
 * @param field_value       hash表域值
 * 
 * @return 
 */
int hset_get_field_value(
        uint32_t server_id, const string &key, const string &field_name, 
        string &field_value)
{
    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
		if (!redis_client->hexists(key, field_name)) {
			return rank_err_hash_field_not_exist;
		} 

        field_value = redis_client->hget(key, field_name);	
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }
	return 0;
}


/** 
 * @brief 根据 key查询对应hashset的域数量
 * 
 * @param key 
 * @param field_name 
 * @param field_value 
 * 
 * @return 
 */
int hset_get_field_num(uint32_t server_id, const string &key, uint32_t &num)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_HASHSET);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }

        num = redis_client->hlen(redis_key.str());	
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }
	return num;
}


/* 
 * @brief 根据 key sub_key 新增对应string值和超时时间,如果存在则不更新
 *        
 * 
 * @param key 
 * @param sub_key  
 * @param value 
 * @param expire
 * 
 * @return 
 */
bool string_insert(uint32_t server_id, const string &key, 
        const string &value, uint32_t expire)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_STRING);
	redis_key << prefix << key;

    bool flag = false;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }

        flag = redis_client->setnx(redis_key.str(), value);	
        if (flag == true) {
            redis_client->expire(redis_key.str(), expire);
        }
    } catch (redis::protocol_error&) {
        flag = false;
        return flag;
    }

	return flag;
}

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
int hset_change_int_value(uint32_t server_id, const string &key,
        const string &field_name, const int64_t field_value, const uint32_t ttl)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_HASHSET);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }

        redis_client->hincrby(redis_key.str(), field_name, field_value);	
		if(ttl){
			redis_client->expire(redis_key.str(), ttl);
		}
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }

	return 0;
}

/** 
 * @brief 根据key sub_key查询指定hash表的全部域信息
 * 
 * @param key 
 * @param out       返回的域信息
 * 
 * @return 
 */
int hset_get_one_set_all_info(uint32_t server_id, const string &key,
        std::vector< std::pair<string, string> > &out)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_HASHSET);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        redis_client->hgetall(redis_key.str(), out);	
    } catch (std::exception& e) {
		ERROR_TLOG("exception:%s",e.what());
        return rank_err_sys_err;
    }

    return 0;
}

/** 
 * @brief 向Set中插入新元素
 * 
 * @param key 
 * @param sub_key 
 * @param value 
 * 
 * @return 
 */
int set_insert_member(uint32_t server_id, const string &key,
        const string &value)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_SET);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        redis_client->sadd(redis_key.str(), value);	
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }

    return 0;
}

/** 
 * @brief 从Set中删除元素
 * 
 * @param key 
 * @param value 
 * 
 * @return 
 */
int set_del_member(uint32_t server_id, const string &key,
        const string &value)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_SET);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        redis_client->srem(redis_key.str(), value);	
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }

    return 0;
}

/** 
 * @brief 判断是否set成员
 * 
 * @param key 
 * @param sub_key 
 * @param value 
 * 
 * @return 
 */
int set_is_member(uint32_t server_id, const string &key,
        const string &value, bool &flag)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_SET);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        flag = redis_client->sismember(redis_key.str(), value);	
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }

    return 0;
}


/** 
 * @brief 拉取一个Set中的所有元素集合
 * 
 * @param key 
 * @param std::pair<string 
 * @param out 
 * 
 * @return 
 */
int set_get_all_member(uint32_t server_id, const string &key,
        std::set<string> &out)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_SET);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        redis_client->smembers(redis_key.str(), out);	
    } catch (std::exception& e) {
		ERROR_TLOG("exception:%s",e.what());
        return rank_err_sys_err;
    }

    return 0;
}

/** 
 * @brief 删除redis中的指定key集合
 * 
 * @param key 
 * @param sub_key 
 * @param value 
 * 
 * @return 
 */
int redis_del_key(const string &prefix, const uint32_t key, 
        const uint32_t sub_key)
{
	ostringstream redis_key;
	redis_key << prefix << ":" << key << ":" << sub_key;

    try {
        redis_client->del(redis_key.str());	
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }

    return 0;
}

/** 
 * @brief 删除redis中的指定key集合 
 * 
 * @param key 
 * 
 * @return 
 */
int redis_del_key_str(uint32_t server_id, const string key)
{
    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        redis_client->del(key);	
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }

    return 0;
}

/**
 *@brief 根据对战的两个玩家的id,生成一份战报,保存在set中；并保存战报key到玩家的链表中
 * 战报key 为 battle_key ,生成规则：时间戳 + atk_id + def_id 
 *@param atk_id : 玩家米米号
 *@param def_id : ai玩家米米号
 *@param type: 战报类型
 *@param pkg : 打包好的战报数据
 *@param btl_key_ttl: 战报key 的ttl
 *@param btl_key_list_ttl: 保存战报key的链表的ttl
 *@return 成功：0
 */
uint32_t save_btl_report(
		uint32_t server_id,
		uint64_t atk_role_key, uint64_t def_role_key, 
		uint32_t type, uint32_t time_stamp, 
		const string& pkg,
		uint32_t btl_key_ttl, uint32_t btl_key_list_ttl)
{
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
	ostringstream redis_key01, redis_key02, redis_key03;
	string str_atk_role_key = number_to_string(atk_role_key);
	string str_def_role_key = number_to_string(def_role_key);
	string str_time = number_to_string(time_stamp);
	string prefix("btlkey");
	string battle_key = prefix + str_time + str_atk_role_key + str_def_role_key;
	redis_key01 << battle_key;
	try {
		redis_client->set(redis_key01.str(), pkg);
		redis_client->expire(redis_key01.str(), btl_key_ttl);
	} catch (redis::protocol_error&) {
		//ERROR_TLOG("save btl report:set,expire,protocol_error:"
		//		"atk_id=[%u],def_id=[%u]", atk_id, def_id);
		return rank_err_redis_not_available;
	} catch (boost::bad_lexical_cast&) {
		//ERROR_TLOG("save btl report:set,expire,bad lexical cast"
		//		"atk_id=[%u],def_id=[%u]", atk_id, def_id);
		return rank_err_save_btl_pkg_err;
	}
	redis_key02 << "btlreport:" << type << atk_role_key;
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
		
		redis_client->rpush(redis_key02.str(), battle_key);
		redis_client->expire(redis_key02.str(), btl_key_list_ttl);
	} catch (redis::protocol_error&) {
		//ERROR_TLOG("save btl report:rpush,expire,protocol_error"
		//		"atk_id=[%u],def_id=[%u]", atk_id, def_id);
		return rank_err_redis_not_available;
	} catch (boost::bad_lexical_cast&) {
		//ERROR_TLOG("save btl report:rpush,expire,bad lexical cast"
		//		"atk_id=[%u],def_id=[%u]", atk_id, def_id);
		return rank_err_redis_not_available;
	}
	redis_key03 << "btlreport:" << type << def_role_key;
	try {
		int len = redis_client->llen(redis_key03.str());
		while (len >= BTL_REP_KEY_LIST_MAX_LEN) {
			try {
				string rm_btl_key = redis_client->lpop(redis_key03.str());
				redis_client->del(rm_btl_key);		
			} catch (exception&) {
				ERROR_TLOG("pop btl_key02,err;[%s]", redis_key03.str().c_str());
			}
			--len;
		}
		redis_client->rpush(redis_key03.str(), battle_key);
		redis_client->expire(redis_key03.str(), btl_key_list_ttl);
	} catch (redis::protocol_error&) {
		//ERROR_TLOG("save btl report:rpush,expire,protocol_error02"
		//		"atk_id=[%u],def_id=[%u]", atk_id, def_id);
		return rank_err_redis_not_available;
	} catch (boost::bad_lexical_cast&) {
		//ERROR_TLOG("save btl report:rpush,expire,bad lexical cast02"
		//		"atk_id=[%u],def_id=[%u]", atk_id, def_id);
		return rank_err_redis_not_available;
	}
	return 0;
}

uint32_t get_btl_report_key(
		uint32_t server_id,
		uint64_t role_key, uint32_t type, 
		rankproto::sc_get_battle_report_key& cli_out_, 
		uint32_t flag, uint32_t count)
{
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
	cli_out_.Clear();
	ostringstream redis_key;
	redis_key << "btlreport:" << type << role_key;
	try {
		int len = redis_client->llen(redis_key.str());
		for (uint32_t i = len; i > 0 && count > 0; --i) {
			string key = redis_client->lindex(redis_key.str(), i - 1);
			DEBUG_TLOG("role_key=[%u],get btl report key;[%s]", role_key, key.c_str());
			if (flag) {
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
			} else {
				rankproto::btl_report_info* btl_ptr = cli_out_.add_btl_inf();
				btl_ptr->set_key(key);
			}
			--count;
		}
	} catch (redis::protocol_error&) {
		return rank_err_redis_not_available;
	} catch (boost::bad_lexical_cast&) {
		return rank_err_get_btl_key_err;
	}
	return 0;
}

string get_key_prefix_str(uint32_t type)
{
    string prefix[] = {
        "rankings:",
        "hashset:",
        "set:",
        "string:",
		"list:",
		"dump:"
    };
    if (type >= array_elem_num(prefix)) {
        string ret;
        return ret;
    }
    return prefix[type];
}

uint32_t get_btl_report_pkg(
		string key, rankproto::sc_get_battle_report& cli_out_)
{
	try {
		string pkg = redis_client->get(key);
		uint32_t flag = 1;
		if (!pkg.compare(redis::client::missing_value())) {
			//战报不存在
			//TODO kevin 前端无法传 type:所以不删除key
			flag = 0;
		}
		cli_out_.set_pkg(pkg);
		cli_out_.set_flag(flag);
	} catch (redis::protocol_error&) {
		return rank_err_redis_not_available;
	} catch (boost::bad_lexical_cast&) {
		return rank_err_btl_rep_get_err;
	}
	return 0;
}

uint32_t get_users_info_by_score_range(
		uint32_t server_id,
		std::string redis_key,
		uint64_t& low_score, uint64_t& high_score,
		std::vector<user_rank_info_t>& power_infos_vec,
		const std::set<uint64_t>& uid_set,
		const uint64_t unit_power)
{
	//TODO kevin : 记得添加异常处理
	int ret = redis_select_db(server_id);
	if (ret) {
		return ret;
	}
	power_infos_vec.clear();
	redis::client::string_vector result;
	double tmp_low, tmp_high;
	memcpy(&tmp_low, &low_score, sizeof(double));
	memcpy(&tmp_high, &high_score, sizeof(double));
	try {
		redis_client->zrangebyscore(
				redis_key, tmp_low, 
				tmp_high, result);
	} catch (exception &) {
		result.clear();
	}
	uint32_t found = true;
	uint32_t count = 0;
	while (result.empty()) {
		++count;
		if (count > 30) {
			found = false;
			break;
		}
		if (low_score >= unit_power) {
			low_score -= unit_power;
		} else {
			low_score = 0;
		}
		high_score += unit_power;
		double tmp_low2, tmp_high2;
		memcpy(&tmp_low2, &low_score, sizeof(double));
		memcpy(&tmp_high2, &high_score, sizeof(double));
		try {
			redis_client->zrangebyscore(
					redis_key, 
					tmp_low2, tmp_high2, 
					result);
		} catch (exception &) {
			result.clear();
		}
	}
	if (found) {
		FOREACH(result, it) {
			uint64_t role_key = string_to_number<uint64_t>(*it);
			role_info_t role_info = KEY_ROLE(role_key);
			//排除用户角色
			uint64_t user_key = ROLE_KEY(ROLE(role_info.userid, 0));
			if (uid_set.count(role_key) || uid_set.count(user_key)) {
				continue;
			}

			user_rank_info_t tmp;
			tmp.user_id = role_info.userid;
			tmp.create_tm = role_info.u_create_tm;

			if(tmp.user_id == 0 || 0 == tmp.create_tm){
				continue;
			}
			//Confirm kevin: 这里只要求匹配到对手的id号，无需计算出rank,score
			tmp.rank = 0;
			tmp.score = 0;
			power_infos_vec.push_back(tmp);
		}
	}
	return 0;
}
/** 
 * @brief 向List头部插入新元素
 * 
 * @param key 
 * @param value 
 * 
 * @return 
 */
uint32_t list_lpush_member(uint32_t server_id, const std::string &key, const std::string &value)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_LIST);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        redis_client->lpush(redis_key.str(), value);	
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }

    return 0;
}
/** 
 * @brief 从List头部删除元素
 * 
 * @param key 
 * @param value 
 * 
 * @return 
 */
uint32_t list_lpop_member(uint32_t server_id, const std::string &key)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_LIST);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        redis_client->lpop(redis_key.str());	
    } catch (redis::protocol_error&) {
        return rank_err_sys_err;
    }

    return 0;
}
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
	   	const int start, const int end, std::vector<std::string> &out)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_LIST);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        redis_client->lrange(redis_key.str(), start, end, out);	
    } catch (std::exception& e) {
		ERROR_TLOG("exception:%s",e.what());
        return rank_err_sys_err;
    }

    return 0;
}
/** 
 * @brief 剪切list元素range：start end
 * 
 * @param key 
 * @param start 
 * @param end 
 * 
 * @return 
 */
uint32_t list_trim_member(uint32_t server_id, const string &key,
	   	const int start, const int end)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_LIST);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        redis_client->ltrim(redis_key.str(), start, end);	
    } catch (std::exception& e) {
		ERROR_TLOG("exception:%s",e.what());
        return rank_err_sys_err;
    }

    return 0;
}
/** 
 * @brief 获取list元素个数
 * 
 * @param key 
 * 
 * @return 
 */
uint32_t list_get_length(uint32_t server_id, const std::string &key, uint32_t & len)
{
	ostringstream redis_key;
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_LIST);
	redis_key << prefix << key;

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
 
        len = redis_client->llen(redis_key.str());	
    } catch (std::exception& e) {
		ERROR_TLOG("exception:%s",e.what());
        return rank_err_sys_err;
    }

    return 0;
}

uint32_t dump_rank(std::vector<dump_key_info_t>& key_vec, uint32_t server_id)
{
    string prefix = get_key_prefix_str(rankproto::REDIS_KEY_TYPE_ZSET);

    try {
        int ret = redis_select_db(server_id);
        if (ret) {
            return ret;
        }
    } catch (std::exception& e) {
		ERROR_TLOG("exception:%s",e.what());
        return rank_err_sys_err;
    }
	FOREACH(key_vec, it) {
		std::string old_redis_key = prefix + 
            number_to_string(it->orig_key.key) + ":" 
            + number_to_string(it->orig_key.sub_key);

		std::string new_redis_key = prefix + 
            number_to_string(it->new_key.key) + ":" 
            + number_to_string(it->new_key.sub_key);
		try {
			if (!(redis_client->exists(old_redis_key))) {
				ERROR_TLOG("Not Exist:redis_key:%s", old_redis_key.c_str());
				continue;
			}
		} catch (std::exception& e) {
			ERROR_TLOG("exception:%s", e.what());
			continue;
		}
		try {
			if (redis_client->exists(new_redis_key)) {
				ERROR_TLOG("Exist:new_redis_key:%s", new_redis_key.c_str());
				continue;
			}
		} catch (std::exception& e) {
			ERROR_TLOG("exception:%s", e.what());
			continue;
		}
		redis::client::string_type serialized_val;
		redis::client::string_type ret_str;
		try {
			serialized_val = redis_client->dump(old_redis_key);
		} catch (std::exception& e) {
			ERROR_TLOG("Dump Err,old_redis_key=[%s],new_redis_key=[%s]", 
                    old_redis_key.c_str(), new_redis_key.c_str());
			ERROR_TLOG("exception:%s", e.what());
			continue;
		}
		try {
			ret_str = redis_client->restore(new_redis_key, 86400 * 7 * 1000, serialized_val);
		} catch (std::exception& e) {
			ERROR_TLOG("Restore Err,old_redis_key=[%s],new_redis_key=[%s]", 
                    old_redis_key.c_str(), new_redis_key.c_str());
			ERROR_TLOG("exception:%s", e.what());
			continue;
		}

        if (it->del_orig_key) {
            try {
                redis_client->del(old_redis_key);	
            } catch (std::exception &e) {
                ERROR_TLOG("DEL KEY=[%s] exception:%s", old_redis_key.c_str(), e.what());
                continue;
            }
        }
    }
	return 0;
}
