#include <cmath>
#include "rank_utils.h"
#include "player.h"
#include "global_data.h"
#include "service.h"
#include "common.h"
#include "arena.h"
#include "utils.h"
#include "arena_conf.h"
#include "player_utils.h"
#include "item.h"
#include "prize.h"
#include "data_proto_utils.h"
#include <boost/lexical_cast.hpp>

/**
 *@brief 拉取排行榜信息
 *@param player
 *@param type 排名类型
 *@param sub_type 
 *@param order 排名升降序规则：（0，升序；1，降序）
 *@return 成功：0； 失败：返回-1
 */
int RankUtils::get_rank_list_info(
		player_t *player, uint32_t type, 
		uint32_t sub_type, uint32_t start, 
		uint32_t end, uint32_t order) 
{
	rankproto::cs_get_rank_list req_in_;	
	ostringstream key;
	key << type << ":" << sub_type;
	req_in_.set_rank_key(key.str());
	req_in_.set_start(start);
	req_in_.set_end(end);
	req_in_.set_order(order);
	req_in_.set_server_id(g_server_id);
	
	int ret = g_dbproxy->send_msg(
			player, player->userid, 
			player->create_tm,
			ranking_cmd_get_ranking_list, 
			req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}


/**
 *@brief 在排行榜中插入一行数据
 *@param player
 *@param type 排名类型
 *@param sub_type 
 *@param score 排名所用的分数
 *@return 成功：0； 失败：返回错误码
 */
int RankUtils::rank_user_insert_score(
	uint32_t userid, uint32_t create_tm,
	uint32_t type, uint32_t sub_type,
	uint64_t score, uint32_t ttl)
{
	std::ostringstream key;
	key << type << ":" << sub_type;
	rankproto::cs_rank_user_add req_in_;
	req_in_.set_rank_key(key.str());
	req_in_.set_score(score);
	req_in_.set_ttl(ttl);	
	req_in_.set_server_id(g_server_id);

	int ret = g_dbproxy->send_msg(
		NULL, userid, 
		create_tm,
		ranking_cmd_ranking_insert_score, 
		req_in_);

	if (ret) {
		return ret;
	}
	return 0;
}

/** 
 * @brief 删除排名记录
 * 
 * @param userid 
 * @param key 排名key,不包含前缀
 * 
 * @return 
 */
int RankUtils::rank_del_user(
        uint32_t userid, uint32_t u_create_tm, std::string key)
{
    rankproto::cs_del_user_rank req_in_;
    req_in_.set_key(key);
    req_in_.mutable_role()->set_userid(userid);
    req_in_.mutable_role()->set_u_create_tm(u_create_tm);
    req_in_.mutable_role()->set_server_id(g_server_id);

    int ret = g_dbproxy->send_msg(
		NULL, userid, u_create_tm,
		ranking_cmd_del_user_rank, 
		req_in_);
	if (ret) {
		return ret;
	}
    return 0;
}

/**
 *@brief 交换rank服中的排名
 *@param player
 *@param type 排名类型
 *@param sub_type 
 *@param first_userid 自己
 *@param sec_userid  ai玩家
 *@return 成功：0； 失败：返回-1
 */
int RankUtils::switch_user_rank(
		uint32_t type, uint32_t sub_type, 
		uint32_t userid, uint32_t u_create_tm,
		uint32_t ai_userid, uint32_t ai_create_tm)
{
	rankproto::cs_rank_switch_arena_user_rank req_in_;
	std::ostringstream key, key2;
	key << type << ":" << 0;
	req_in_.set_rank_key(key.str());
	key2 << type << ":" << sub_type;
	req_in_.set_daily_rank_key(key2.str());
	req_in_.mutable_atk_role()->set_userid(userid);
	req_in_.mutable_atk_role()->set_u_create_tm(u_create_tm);
	req_in_.mutable_def_role()->set_userid(ai_userid);
	req_in_.mutable_def_role()->set_u_create_tm(ai_create_tm);
	req_in_.set_server_id(g_server_id);

	int ret = g_dbproxy->send_msg(
			0, userid, 
			u_create_tm,
			ranking_cmd_switch_arena_ranking_user, 
			req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}

/**
 *@brief 排行中，拉取玩家数据信息(需要处理rank服的回包)
 *@param player
 *@param type 排名类型
 *@param sub_type 
 *@param userid
 *@param order 排名升降序规则：（0，升序；1，降序）
 *@param flag false,拉取时插入排行记录 true,拉取时不插入排行记录
 *@return 成功：0； 失败：返回错误码
 */
int RankUtils::get_user_rank_info(
		player_t *player, uint32_t type, uint32_t sub_type,
		std::vector<role_info_t>& user_ids,
		uint32_t order)
{
	rankproto::cs_get_user_rank req_in_;
	//req_in_.set_type(type);
	//req_in_.set_sub_type(sub_type);
	std::ostringstream key;
	key << type << ":" << sub_type;
	req_in_.set_rank_key(key.str());
	req_in_.set_order(order);
	FOREACH(user_ids, it) {
		commonproto::role_info_t* pb_ptr = req_in_.add_roles();
		pb_ptr->set_userid(it->userid);
		pb_ptr->set_u_create_tm(it->u_create_tm);
	}
	req_in_.set_server_id(g_server_id);
	int ret = g_dbproxy->send_msg(
			player, player->userid, 
			player->create_tm,
			ranking_cmd_get_users_rank, 
			req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}

int RankUtils::rank_insert_last(
		player_t* player, uint32_t userid, 
		uint32_t create_tm,
		uint32_t type,
		uint32_t sub_type, uint32_t order)
{
	rankproto::cs_rank_insert_last req_in_;
	ostringstream key;
	key << type << ":" << sub_type;
	req_in_.set_rank_key(key.str());
	req_in_.set_order(order);
	req_in_.set_server_id(g_server_id);

	int ret = g_dbproxy->send_msg(
			player, userid,
			create_tm,
			ranking_cmd_rank_insert_last,
			req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}

int RankUtils::get_user_rank_info_by_keys(
		player_t* player, 
		std::vector<rank_key_order_t>& rank_vec,
		uint32_t user_id, uint32_t u_create_tm)
	
{
	rankproto::cs_get_user_multi_rank req_in_;
	FOREACH (rank_vec, it) {
		std::ostringstream key;
		key << it->key.key << ":" << it->key.sub_key;
        rankproto::rank_key_order_t *inf = req_in_.add_rank_keys();
        inf->set_rank_key(key.str());
        inf->set_order(it->order);
	}
	req_in_.mutable_role()->set_userid(user_id);
	req_in_.mutable_role()->set_u_create_tm(u_create_tm);
	req_in_.set_server_id(g_server_id);
	int ret = g_dbproxy->send_msg(
		player, player->userid, 
		player->create_tm,
		ranking_cmd_get_user_multi_rank,
		req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}
	

//分区间选择挑战对手:(玩家排名 / 5) 区间，每个区间随机一个
int RankUtils::inter_partition_select_challenge_rank_list(
		uint32_t rank, std::vector<uint32_t>& rank_list) 
{
	uint32_t NUM = 5;
	uint32_t THRESHOLD = 10;
	uint32_t sum = 0;
	if (rank >= THRESHOLD) {
		for (uint32_t i = 0; i < NUM; ++i) {
			uint32_t INTER = rank / NUM;
			uint32_t tmp = sum + 1;
			sum = INTER + sum;
			uint32_t random_value = 0;
			//若能整除，注意最后一次循环可能会随机到自己,要避免此类情况
			if (rank % NUM == 0 && rank == sum) {
				random_value = taomee::ranged_random(tmp, sum - 1);
			} else { 
				random_value = taomee::ranged_random(tmp, sum);
			}
			rank_list.push_back(random_value);
		}
	} else if (rank > NUM) {
		Utils::rand_select_k(1, rank - 1, NUM, rank_list);
	} else {
		//若在前5名内的，要给玩家补齐竞技场内的空位
		for (uint32_t i = 1; i <= NUM; ++i) {
			if (rank == i) {
				continue;
			}
			rank_list.push_back(i);
		}
		//第五个站位 给玩家在6-10名内随机一个
		uint32_t tmp_value = taomee::ranged_random(6, 10); 
		rank_list.push_back(tmp_value);
	}
	/*
	} else if (rank > 1) {
		Utils::rand_select_k(1, rank - 1, rank - 1, rank_list);
	} else {
		rank_list.clear();
	}
	*/
	return 0;
}

int RankUtils::select_challenge_rank_list(
        uint32_t rank, std::vector<uint32_t>& rank_list, uint32_t win_streak)
{
	uint32_t NUM = 5;
	//int step = 0;
	if (rank <= 5) {
		//若在前5名内的，要给玩家补齐竞技场内的空位
		for (uint32_t i = 1; i <= NUM; ++i) {
			if (rank == i) {
				continue;
			}
			rank_list.push_back(i);
		}
		//第五个站位 给玩家在6-10名内随机一个
		uint32_t tmp_value = taomee::ranged_random(6, 10); 
		rank_list.push_back(tmp_value);
	} else {
		//与rank相乘的函数的值域是 [0.5, 0.9] 
		uint32_t low_rank = rank * (1 - std::min(sqrt(win_streak + 1), 5.0) * 0.1);
		if (low_rank + 5 >= rank) {
			/*
			uint32_t count = 0;
			uint32_t index = low_rank;
			while (count < 5) {
				if (index == rank) {
					++index;
					++count;
					continue;
				}
				TRACE_TLOG("zjun0413:index=[%u]", index);
				rank_list.push_back(index);
				++count;
				++index;
			}
			*/
			for (uint32_t i = 1; i <= 5; ++i) {
				rank_list.push_back(rank - i);
			}
			reverse(rank_list.begin(), rank_list.end());
		} else {
			Utils::rand_select_k(low_rank + 1, rank - 1, NUM - 1, rank_list);
			rank_list.push_back(low_rank);
			reverse(rank_list.begin(), rank_list.end());
		}
	}
	return 0;
}

/**
 *@brief 排行中,根据玩家排名获得玩家的米米号
 *@param player
 *@param type 排名类型
 *@param sub_type(0) 
 *@param rank_list 玩家的排名
 *@return 成功：0； 失败：返回错误码
 */
int RankUtils::get_user_id_by_rank(
		player_t* player, uint32_t type, 
		uint32_t sub_type, std::vector<uint32_t>& rank_list)
{
	rankproto::cs_get_rank_userid req_in_;
	std::ostringstream key;
	key << type << ":" << sub_type;
	req_in_.set_rank_key(key.str());
	FOREACH(rank_list, it) {
		req_in_.add_rank(*it);
	}
	req_in_.set_server_id(g_server_id);

	int ret = g_dbproxy->send_msg(
			player, player->userid, 
			player->create_tm,
			ranking_cmd_get_ranking_users, 
			req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}

/**
 *@brief 存储一份战报:暂时存放在player中temp_info里的btl_report字段
 *@param player
 *@param ati_tick 前端随机出的挑战方战斗动作种子
 *@param def_tick 前端随机出的防守方战斗动作种子
 *@param pk_result:战斗结果
 *@return 成功：0； 失败：返回-1
 */
int RankUtils::save_arena_battle_report(
		player_t* player, 
		const commonproto::battle_player_data_t& def_btl_data,
		const commonproto::battle_player_data_t& atk_btl_data,
		uint32_t atk_tick, uint32_t def_tick, 
		commonproto::challenge_result_type_t pk_result)
{
	uint32_t time = NOW();

	commonproto::battle_report_info_t report_info;
	report_info.mutable_def_battle_player_data()->CopyFrom(def_btl_data);
	report_info.mutable_atk_battle_player_data()->CopyFrom(atk_btl_data);

	report_info.set_def_tick(def_tick);
	report_info.set_atk_tick(atk_tick);
	report_info.set_time(time);
	report_info.set_result(pk_result);

	(*player->temp_info.tmp_btl_info).clear();
	report_info.SerializeToString(player->temp_info.tmp_btl_info);
	return 0;
}

int RankUtils::save_btl_report_to_redis(
		player_t* player, commonproto::pvp_type_t type, 
		uint64_t ai_role_key, commonproto::challenge_result_type_t result, 
		std::string& btl_key,
		uint32_t old_rank, uint32_t new_rank,
		uint32_t btl_key_ttl, 
		uint32_t btl_key_list_ttl)
{
	commonproto::battle_report_info_t report_info;
	if (player->temp_info.tmp_btl_info) {
		report_info.ParseFromString(*(player->temp_info.tmp_btl_info));
	}
	(*player->temp_info.tmp_btl_info).clear();
	
	report_info.set_result(result);
	report_info.set_quit_flag(0);

	uint32_t time_stamp = 0;
	if (type == commonproto::ESCORT) {
		time_stamp = player->temp_info.escort_rob_tm;
	} else if (type == commonproto::ARENA) {
		time_stamp = NOW();
		report_info.set_old_rank(old_rank);
		report_info.set_new_rank(new_rank);

		report_info.set_def_old_rank(new_rank);
		report_info.set_def_new_rank(old_rank);
	} else {
		time_stamp = NOW();
	}

	rankproto::cs_save_battle_report  save_in_;
	std::string btl_info_pkg;
	report_info.SerializeToString(&btl_info_pkg);
	save_in_.set_pkg(btl_info_pkg);	
	save_in_.set_type((uint32_t)type);
	role_info_t role_info = KEY_ROLE(ai_role_key);
	save_in_.mutable_role()->set_userid(role_info.userid);
	save_in_.mutable_role()->set_u_create_tm(role_info.u_create_tm);
	save_in_.set_timestamp(time_stamp);
	save_in_.set_btl_key_ttl(btl_key_ttl);
	save_in_.set_btl_key_list_ttl(btl_key_list_ttl);
	save_in_.set_server_id(g_server_id);
	int ret = g_dbproxy->send_msg(
			0, player->userid, 
			player->create_tm,
			ranking_cmd_save_battle_report, save_in_);
	if (ret) {
		return ret;
	}
	uint64_t p_role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	std::string prefix("btlkey");
	btl_key = prefix + boost::lexical_cast<std::string>(time_stamp) 
		+ boost::lexical_cast<std::string>(p_role_key) 
		+ boost::lexical_cast<std::string>(ai_role_key);
	return 0;
}

/**
 *@brief 挑战胜利, 发奖励
 *@param player
 *@param new_rank 新排名
 *@return 成功：0； 失败：返回-1
 */
/*
int RankUtils::get_reward_after_challenge(
		player_t* player, uint32_t new_rank)
{
	rank_reward_t* reward_ptr = g_arena_rank_reward_conf_mgr.get_arena_rank_reward(new_rank);
	if (reward_ptr == NULL) {
		ERROR_TLOG("new rank not found;value=[%u]", new_rank);
		return cli_err_rank_not_fount_in_table;
	}	
	if (player == NULL) {
		ERROR_TLOG("player not exsit");
		return cli_err_data_error;
	}
	onlineproto::sc_0x0112_notify_get_prize noti_prize_msg;
	transaction_proc_prize(
			player, reward_ptr->single_reward, 
			noti_prize_msg, commonproto::PRIZE_REASON_ARENA_GIFT);
	return 0;
}
*/

//竞技场连胜推送
/*
int RankUtils::arena_push_killing_spree(
		player_t* player, uint32_t win_streak_count)
{
	onlineproto::sc_0x0220_arena_push_killing_spree msg_out_;
	msg_out_.set_userid(player->userid);
	std::string nick(player->nick);
	msg_out_.set_nick(nick);
	msg_out_.set_stream_kill(win_streak_count);
	switchproto::cs_sw_transmit_only sw_in_;
	sw_in_.set_transmit_type(switchproto::SWITCH_TRANSMIT_WORLD);
	uint16_t cmd = cli_cmd_cs_0x0220_arena_push_killing_spree;
	sw_in_.set_cmd(cmd);
	std::string pkg;
	msg_out_.SerializeToString(&pkg);
	sw_in_.set_pkg(pkg);
	int ret = g_switch->send_msg(0, g_online_id, sw_cmd_sw_transmit_only, sw_in_);
	if (ret) {
		return ret;
	}
	return 0;
}
*/

/*
int RankUtils::win_streak_get_reward(
		player_t* player, uint32_t win_streak_count)
{
	streak_info_t* streak_ptr = g_arena_streak_reward_conf_mgr.get_streak_conf_info(win_streak_count);
	if (streak_ptr == NULL) {
		ERROR_TLOG("win streak count err:value=[%u]", win_streak_count);
		return 0;
	}
	if (player == NULL) {
		ERROR_TLOG("player not exsit");
		return cli_err_data_error;
	}
	onlineproto::sc_0x0112_notify_get_prize noti_prize_msg;
	transaction_proc_prize(player, streak_ptr->prize_id, 
			noti_prize_msg, commonproto::PRIZE_REASON_ARENA_GIFT);
	return 0;
}
*/

int RankUtils::get_ranking_info_type(uint32_t type, uint32_t sub_type)
{
    int rank_info_type = 0;

    if (type == commonproto::RANKING_TYPE_FAMILY) {
        rank_info_type = commonproto::RANKING_INFO_FAMILY;
    } else {
        rank_info_type = commonproto::RANKING_INFO_USER;
    }

    return rank_info_type;
}

int RankUtils::hset_insert_or_update(
        player_t *player,  uint32_t userid, uint32_t u_create_tm,
        uint32_t server_id,
        uint32_t key, uint32_t sub_key, uint32_t type, 
        const vector<rankproto::hset_field_t> *fields)
{
	rankproto::cs_hset_insert_or_update req_in_;
    std::ostringstream redis_key;
    redis_key << key << ":" << sub_key;

    req_in_.set_oper_type(type);
	req_in_.set_key(redis_key.str());
    req_in_.set_server_id(server_id);

    FOREACH(*fields, it) {
        rankproto::hset_field_t *field = req_in_.add_fields(); 
        field->CopyFrom(*it);
    }
    int ret = g_dbproxy->send_msg(
            player, userid, u_create_tm, 
			ranking_cmd_hset_insert_or_update, req_in_);
    if (ret) {
        return ret;
    }

    return 0;
}

int RankUtils::hset_insert_or_update_str_version(
        player_t *player,  uint32_t userid, uint32_t u_create_tm,
        uint32_t server_id,
        std::string type, std::string sub_type, uint32_t ope_type, 
        const vector<rankproto::hset_field_t> *fields, uint32_t ttl)
{
	rankproto::cs_hset_insert_or_update req_in_;
	std::ostringstream redis_key;
	redis_key << type << ":" << sub_type;

	req_in_.set_oper_type(ope_type);
	req_in_.set_key(redis_key.str());
	req_in_.set_server_id(server_id);
	req_in_.set_key_ttl(ttl);

	FOREACH(*fields, it) {
		rankproto::hset_field_t *field = req_in_.add_fields(); 
		field->CopyFrom(*it);
	}
	int ret = g_dbproxy->send_msg(
			player, userid, u_create_tm, 
			ranking_cmd_hset_insert_or_update, req_in_);
	if (ret) {
		return ret;
	}

	return 0;
}

int RankUtils::lock_acquire(
        player_t *player, const std::string key,
        const std::string &value, uint32_t expire_time)
{
	rankproto::cs_string_insert req_in_;
	req_in_.set_key(key);
	req_in_.set_value(value);
	req_in_.set_expire(expire_time);
    req_in_.set_server_id(g_server_id);
    return g_dbproxy->send_msg(
            player, player->userid, player->create_tm, 
			ranking_cmd_string_insert, req_in_);
}

int RankUtils::lock_release(
        player_t *player, uint32_t userid, 
        uint32_t u_create_tm, const std::string key)
{
    rankproto::cs_redis_del_key_str req_in_;
	req_in_.set_key(key);
    req_in_.set_type(rankproto::REDIS_KEY_TYPE_STRING);
    req_in_.set_server_id(g_server_id);
    return g_dbproxy->send_msg(
            player, userid, u_create_tm, 
			ranking_cmd_redis_del_key_str, req_in_);
}

/** 
 * @brief 向Set中增加集合成员
 * 
 * @param player 
 * @param key 
 * @param sub_key 
 * @param value 
 * 
 * @return 
 */

int RankUtils::set_insert_member(
        player_t *player,  uint32_t userid, uint32_t u_create_tm,
        uint32_t server_id,
        uint32_t key, uint32_t sub_key, 
        const std::vector<std::string> &value)
{
	rankproto::cs_set_insert_member req_in_;

    std::ostringstream redis_key;
    redis_key << key << ":" << sub_key;
	req_in_.set_key(redis_key.str());
    FOREACH(value, it) {
        req_in_.add_values(*it);
    }
    req_in_.set_server_id(server_id);

    int ret = g_dbproxy->send_msg(
            player, userid, u_create_tm, 
			ranking_cmd_set_insert_member, req_in_);
    if (ret) {
        return ret;
    }

    return 0;
}

/** 
 * @brief 删除Set中的成员
 * 
 * @param player 
 * @param key 
 * @param sub_key 
 * @param value 
 * 
 * @return 
 */
int RankUtils::set_del_member(
        player_t *player, uint32_t userid, uint32_t u_create_tm,
        uint32_t key, uint32_t sub_key, 
        const std::vector<std::string> &value)
{
	rankproto::cs_set_del_member req_in_;

    std::ostringstream redis_key;
    redis_key << key << ":" << sub_key;
	req_in_.set_key(redis_key.str());
    FOREACH(value, it) {
        req_in_.add_values(*it);
    }
    req_in_.set_server_id(g_server_id);

    int ret = g_dbproxy->send_msg(
            player, userid, u_create_tm, 
			ranking_cmd_set_del_member, req_in_);
    if (ret) {
        return ret;
    }

    return 0;
}

/** 
 * @brief 拉取Set中的所有成员
 * 
 * @param player 
 * @param key 
 * @param sub_key 
 * @param members 
 * 
 * @return 
 */
int RankUtils::set_get_all_member(
        player_t *player,  uint32_t key, uint32_t sub_key)
{
	rankproto::cs_set_get_all_member req_in_;
    std::ostringstream redis_key;
    redis_key << key << ":" << sub_key;
	req_in_.set_key(redis_key.str());
    req_in_.set_server_id(g_server_id);

    int ret = g_dbproxy->send_msg(
            player, player->userid, player->create_tm, 
			ranking_cmd_set_get_all_member, req_in_);
    if (ret) {
        return ret;
    }

    return 0;
}

/** 
 * @brief 删除redis key
 * 
 * @param player 
 * @param key 
 * 
 * @return 
 */
int RankUtils::redis_del_key_str(
        player_t *player,  uint32_t userid, uint32_t u_create_tm,
        const std::string key, uint32_t type)
{
	rankproto::cs_redis_del_key_str req_in_;
	req_in_.set_key(key);
    req_in_.set_type(type);
    req_in_.set_server_id(g_server_id);

    int ret = g_dbproxy->send_msg(player, userid, u_create_tm, 
			ranking_cmd_redis_del_key_str, req_in_);
    if (ret) {
        return ret;
    }

    return 0;
}

/** 
 * @brief 判断是否Set成员
 * 
 * @param player 
 * @param key 
 * @param sub_key 
 * @param value 
 * 
 * @return 
 */

int RankUtils::set_is_member(
        player_t *player,  uint32_t userid, 
        uint32_t u_create_tm, uint32_t server_id,
        uint32_t key, uint32_t sub_key,  
        const std::vector<std::string> &value)
{
	rankproto::cs_set_is_member req_in_;

    std::ostringstream redis_key;
    redis_key << key << ":" << sub_key;
	req_in_.set_key(redis_key.str());
    req_in_.set_server_id(server_id);
    FOREACH(value, it) {
        req_in_.add_values(*it);
    }
    int ret = g_dbproxy->send_msg(
            player, userid, u_create_tm, 
			ranking_cmd_set_is_member, req_in_);
    if (ret) {
        return ret;
    }

    return 0;
}

int RankUtils::get_users_by_score_range(player_t* player, 
		uint32_t type, uint32_t sub_type,
		uint64_t low_score, uint64_t high_score,
		std::set<uint64_t>& role_keys, uint64_t unit_power)
{
	rankproto::cs_get_users_by_score_range req_in_;
    std::ostringstream redis_key;
    redis_key << type << ":" << sub_type;
	req_in_.set_rank_key(redis_key.str());
	req_in_.set_low_score(low_score);
	req_in_.set_high_score(high_score);
	req_in_.set_unit_power(unit_power);
	req_in_.set_server_id(g_server_id);
	FOREACH(role_keys, it) {
		role_info_t role_info = KEY_ROLE(*it);
		commonproto::role_info_t *role_ptr = req_in_.add_role_list();
		role_ptr->set_userid(role_info.userid);
		role_ptr->set_u_create_tm(role_info.u_create_tm);
	}
	int ret = g_dbproxy->send_msg(
			player, player->userid, 
			player->create_tm,
			ranking_cmd_get_users_by_score, 
			req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}

/**
 *@brief 拉取奖励榜信息
 *@param player
 *@param type 类型
 *@return 成功：0； 失败：返回-1
 */
int RankUtils::get_bulletin_list_info(
		player_t *player, uint32_t type, 
		uint32_t sub_type, int32_t start, int32_t end) 
{
	rankproto::cs_list_get_range_member req_in_;	
	ostringstream key;
	key << type << ":" << sub_type;
	req_in_.set_key(key.str());
	req_in_.set_start(start);
	req_in_.set_end(end);
	req_in_.set_server_id(g_server_id);
	
	int ret = g_dbproxy->send_msg(
			player, player->userid, 
			player->create_tm,
			ranking_cmd_list_get_range_member, 
			req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}

/**
 *@brief 向奖励榜插入信息
 *@param player
 *@param type 类型
 *@return 成功：0； 失败：返回-1
 */
int RankUtils::lpush_bulletin_list(
		player_t *player, uint32_t type, uint32_t sub_type, 
		bulletin_elems &values, uint32_t max) 
{
	rankproto::cs_list_lpush_member req_in_;	
	ostringstream key;
	key << type << ":" << sub_type;
	req_in_.set_key(key.str());
	req_in_.set_server_id(g_server_id);
	req_in_.set_max(max);

	std::string pkg;
	bulletin_elems::iterator it = values.begin();
	for(;it != values.end(); it++){
		pkg.clear();
		(*it).SerializeToString(&pkg);
		req_in_.add_value(pkg);
	}
	
	int ret = g_dbproxy->send_msg(
			player, player->userid, 
			player->create_tm,
			ranking_cmd_list_lpush_member, 
			req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}

uint32_t RankUtils::dump_rank(player_t* player, std::vector<dump_rank_key_info_t>& key_vec)
{
	rankproto::cs_dump_rank_info  req_in;
	FOREACH(key_vec, it) {
		rankproto::dump_rank_keys_info_t* ptr = req_in.add_keys_info();
        commonproto::rank_keys_info_t *orig_key = ptr->mutable_orig_key();
        commonproto::rank_keys_info_t *new_key = ptr->mutable_new_key();

        ptr->set_del_orig_key(it->del_orig_key);
        orig_key->set_key(it->orig_key.key);
        orig_key->set_sub_key(it->orig_key.sub_key);
        new_key->set_key(it->new_key.key);
        new_key->set_sub_key(it->new_key.sub_key);

	}
	req_in.set_server_id(g_server_id);
		
	int ret = g_dbproxy->send_msg(NULL,
			player->userid, player->create_tm,
			ranking_cmd_dump_rank,
			req_in);
	if (ret) {
		ERROR_TLOG("Dump Rank Err");
		return ret;
	}
	return 0;
}

uint32_t RankUtils::hash_get_field_info(player_t* player,
		const uint32_t type, const std::string& sub_type,
		std::vector<std::string>& field_name)
{
	std::ostringstream key;
	key << type << ":" << sub_type;
	rankproto::cs_hset_get_field_info req_in;
	req_in.set_key(key.str());
	FOREACH(field_name, it) {
		req_in.add_field_names(*it);
	}
	req_in.set_server_id(g_server_id);
	return g_dbproxy->send_msg(player, 
			player->userid, player->create_tm,
			ranking_cmd_hset_get_field_info, req_in);
}

//更新明星招募信息
int RankUtils::update_star_lottery_info_to_redis(player_t *player,
		const commonproto::star_lottery_elem_list &elem_list)
{
    // 更新明星招募id映射V
	uint32_t key = rankproto::HASHSET_STAR_LOTTERY_INFO_MAP;
	// uint32_t sub_key = TimeUtils::day_align_low(NOW());
	uint32_t type = rankproto::REDIS_INSERT_OR_UPDATE;

	rankproto::cs_hset_insert_or_update req_in_;
    std::ostringstream redis_key;
    redis_key << key << ":" << "0";

    req_in_.set_oper_type(type);
	req_in_.set_key(redis_key.str());
    req_in_.set_server_id(g_server_id);
    // req_in_.set_key_ttl(ttl_time);

	rankproto::hset_field_t *field = req_in_.add_fields(); 
	std::string pkg;
	pkg.clear();

	elem_list.SerializeToString(&pkg);
	uint64_t field_name = ROLE_KEY( 
			ROLE(player->userid, player->create_tm));

    field->set_name(boost::lexical_cast<string>(field_name));
    field->set_value(pkg);

    int ret = g_dbproxy->send_msg(
            NULL, player->userid, player->create_tm, 
			ranking_cmd_hset_insert_or_update, req_in_);

    if (ret) {
        return ret;
    }

    return 0;
}

int RankUtils::get_star_lottery_info_from_redis(player_t *player)
{
	uint64_t field_name = ROLE_KEY( 
			ROLE(player->userid, player->create_tm));
	//拉取明星招募信息
	std::ostringstream redis_key;
	redis_key << rankproto::HASHSET_STAR_LOTTERY_INFO_MAP<<":"<< "0";

	rankproto::cs_hset_get_field_info     rank_get_field_info_in_;
	rank_get_field_info_in_.Clear();
	rank_get_field_info_in_.set_key(redis_key.str());
	rank_get_field_info_in_.add_field_names(boost::lexical_cast<string>(field_name));
	rank_get_field_info_in_.set_server_id(g_server_id);

	return g_dbproxy->send_msg(
			player, player->userid, player->create_tm, 
			ranking_cmd_hset_get_field_info, rank_get_field_info_in_);
}

int RankUtils::save_battle_simple_report(player_t* player,
		const std::vector<role_info_t>& role_vec,
		commonproto::pvp_type_t type,
		const std::string pkg, int ttl)
{
	rankproto::cs_save_btl_report req_in;
	FOREACH (role_vec, it) {
		commonproto::btl_report_pkg_t* pb_btl = req_in.add_btl_report();
		pb_btl->set_userid(it->userid);
		pb_btl->set_create_tm(it->u_create_tm);
		pb_btl->set_pkg(pkg);
		pb_btl->set_generate_time(NOW());
		pb_btl->set_type(type);
		pb_btl->set_ttl(DAY_SECS * 7);
	}
	req_in.set_server_id(g_server_id);
	return g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
			ranking_cmd_save_common_btl_report, req_in);
}
