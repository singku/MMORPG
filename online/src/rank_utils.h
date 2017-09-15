#ifndef __RANK_UTILS_H__
#define __RANK_UTILS_H__
#include <stdint.h>
#include <vector>
#include "common.h"

struct player_t;

struct rank_keys_t {
	uint32_t key;
	uint32_t sub_key;
};

struct rank_key_order_t {
    rank_keys_t key;
    uint32_t order;
};

struct dump_rank_key_info_t {
    rank_keys_t orig_key;
    rank_keys_t new_key;
    uint32_t del_orig_key;
};

class RankUtils {
public:
	/**
	 *@brief 拉取排行榜信息
	 *@param player
	 *@param type 排名类型
	 *@param sub_type 
	 *@param order 排名升降序规则：（0，升序；1，降序）
	 *@return 成功：0； 失败：返回-1
	 */
	static int get_rank_list_info(
            player_t *player, uint32_t type, 
            uint32_t sub_type, uint32_t start, 
            uint32_t end, uint32_t order);
	/**
	 *@brief 交换rank服中的排名
	 *@param player
	 *@param type 排名类型
	 *@param sub_type 
	 *@param userid 自己
	 *@param u_create_tm 自己的create_tm
	 *@param ai_userid ai玩家
	 *@param ai_create_tm ai的create_tm
	 *@return 成功：0； 失败：返回错误码
	 */
	static int switch_user_rank(
			uint32_t type, uint32_t sub_type, 
			uint32_t userid, uint32_t u_create_tm,
			uint32_t ai_userid, uint32_t ai_create_tm);

	//static int get_player_arena_week_rank(player_t *player);

	/**
	 *@brief 设置竞技场玩家的玩家的周排名(竞技场专用)
	 *@param player
	 *@return 成功：0； 失败：返回-1
	 */
	//static int set_players_arena_week_rank(
	//		player_t *player, uint32_t type, uint32_t p_uid, uint32_t ai_uid);

    /** 
     * @brief 将玩家总榜的排名插入到周榜中
     * 
     * @param userid 玩家 userid
     * @param rank 玩家总榜的排名
     * 
     * @return 
     */
	//static uint32_t insert_player_to_arena_weekly_rank(uint32_t userid);	

	/**
	 *@brief 排行中，拉取玩家数据信息(需要处理rank服的回包)
	 *@param player
	 *@param type 排名类型
	 *@param sub_type 
	 *@param roles
	 *@param order 排名升降序规则：（0，升序；1，降序）
	 *@param flag false,拉取时插入排行记录 true,拉取时不插入排行记录
	 *@return 成功：0； 失败：返回-1
	 */
	static int get_user_rank_info(
		player_t *player, uint32_t type, 
		uint32_t sub_type, std::vector<role_info_t>& user_ids, 
		uint32_t order);

	static int get_user_rank_info_by_keys(
			player_t* player, 
			std::vector<rank_key_order_t>& rank_vec,
			uint32_t user_id, uint32_t u_create_tm);

	/**
	 *@brief 排行中,根据玩家排名获得玩家的米米号
	 *@param player
	 *@param type 排名类型
	 *@param sub_type(0) 
	 *@param rank_list 玩家的排名
	 *@return 成功：0； 失败：返回-1
	 */
	static int get_user_id_by_rank(player_t* player, uint32_t type, uint32_t sub_type, std::vector<uint32_t>& rank_list);

	static int select_challenge_rank_list(uint32_t rank, std::vector<uint32_t>& rank_list, uint32_t win_streak);

	static int inter_partition_select_challenge_rank_list(uint32_t rank, std::vector<uint32_t>& rank_list);

	/**
	 *@brief 通用排行中,存储一份战报
	 *@param player
	 *@param users_info 缓存中拉取出的玩家信息
	 *@param ati_tick 前端随机出的挑战方战斗动作种子
	 *@param def_tick 前端随机出的防守方战斗动作种子
	 *@param type 战报类型（commonproto::pvp_type_t类型）
	 *@param pk_result:战斗结果
	 *@param btl_type: 战斗类型
	 *@return 成功：0； 失败：返回-1
	 */
	static int save_arena_battle_report(
				player_t* player, 
				const commonproto::battle_player_data_t& def_btl_data,
				const commonproto::battle_player_data_t& atk_btl_data,
				uint32_t atk_tick, uint32_t def_tick, 
				commonproto::challenge_result_type_t pk_result = commonproto::QUIT);
	/**
	 *@brief 在排行榜中插入一行数据
	 *@param userid 玩家id
	 *@param type 排名类型
	 *@param sub_type 
	 *@param score 排名所用的分数
     *@param ttl 新插入排名的时候, 期望排名过期的时间(秒)
	 *@return 成功：0； 失败：返回-1
	 */
	static int rank_user_insert_score(
			uint32_t userid, uint32_t create_tm,
			uint32_t type, uint32_t sub_type, uint64_t score,
			uint32_t ttl = 30 * 86400);

	/**
	 *@brief 保存一份战报到redis中
	 *@param player
	 *@return 成功：0； 失败：返回-1
	 */
	static int save_btl_report_to_redis(
			player_t* player,
			commonproto::pvp_type_t type,
			uint64_t ai_role_key, 
			commonproto::challenge_result_type_t result, 
			std::string& btl_key, 
			uint32_t old_rank = 0, uint32_t new_rank = 0,	
			uint32_t btl_key_ttl = 30 * 86400, 
			uint32_t btl_key_list_ttl = 30 * 86400);
	/**
	 *@brief 挑战胜利, 发奖励
	 *@param player
	 *@param new_rank 新排名
	 *@return 成功：0； 失败：返回-1
	 */
	//static int get_reward_after_challenge(player_t* player, uint32_t new_rank);
	//static int arena_push_killing_spree(player_t* player, uint32_t win_streak_count);
	//static int win_streak_get_reward(player_t* player, uint32_t win_streak_count);

    /** 
     * @brief 判断拉取排名返回的信息类型
     * 
     * @param type 排名类型
     * @param sub_type 排名子类型
     * @return 
     */
    static int get_ranking_info_type(uint32_t type, uint32_t sub_type);

    /** 
     * @brief 插入或更新一个redis hash表记录
     * 
     * @param key 
     * @param sub_key 
     * @param type  操作类型 rankproto::redis_field_operation_t
     * @param fields  域值
     * 
     * @return 
     */
    static int hset_insert_or_update(
        player_t *player,  uint32_t userid, uint32_t u_create_tm, 
        uint32_t server_id,
        uint32_t key, uint32_t sub_key, uint32_t type, 
        const vector<rankproto::hset_field_t> *fields);

    static int lock_acquire(
        player_t *player, const std::string key,
        const std::string &value, uint32_t expire_time);

    static int lock_release(
        player_t *player, uint32_t userid, uint32_t u_create_tm, const std::string key);

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
    static int set_insert_member(
        player_t *player,  uint32_t userid, uint32_t u_create_tm,
        uint32_t server_id,
        uint32_t key, uint32_t sub_key, 
        const std::vector<std::string> &value);

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
    static int set_del_member(
        player_t *player, uint32_t userid, uint32_t u_create_tm,
        uint32_t key, uint32_t sub_key, 
        const std::vector<std::string> &value);

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
    static int set_get_all_member(
            player_t *player,  uint32_t key, uint32_t sub_key);

    /** 
     * @brief 删除redis key
     * 
     * @param player 
     * @param key 
     * @param value 
     * @param operate_type: redis_key_prefix_type_t
     * @return 
     */
    static int redis_del_key_str(
            player_t *player,  uint32_t userid, uint32_t u_create_tm,
            const std::string key, uint32_t operate_type);

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
    static int set_is_member(
            player_t *player,  uint32_t userid,  
            uint32_t u_create_tm, uint32_t server_id,
            uint32_t key, uint32_t sub_key, 
            const std::vector<std::string> &value);
	
	static int rank_insert_last(
		player_t* player, uint32_t userid,
		uint32_t create_tm, uint32_t type,
		uint32_t sub_type, uint32_t order);
//	static uint32_t get_last_n_weekly_rank(
//		player_t* player, uint32_t type,
//		std::vector<uint32_t>&weekly_sub_keys);

    static int rank_del_user(uint32_t userid, uint32_t u_create_tm, std::string key);

	static int get_users_by_score_range(player_t* player,
			uint32_t type, uint32_t sub_type,
			uint64_t low_score, uint64_t high_score,
			std::set<uint64_t>& role_keys, uint64_t unit_power);

	typedef google::protobuf::RepeatedPtrField<commonproto::prize_bulletin_info_t> bulletin_elems; 

	static int lpush_bulletin_list(player_t *player,
		   	uint32_t type, uint32_t sub_type,
			bulletin_elems &values, uint32_t max);

	static int get_bulletin_list_info(player_t *player,
			uint32_t type, uint32_t sub_type,
		   	int32_t start, int32_t end);

	static uint32_t dump_rank(player_t* player, std::vector<dump_rank_key_info_t>& key_vec);

	static uint32_t hash_get_field_info(player_t* player,
			const uint32_t type, const std::string& sub_type,
			std::vector<string>& field_name);

	static int hset_insert_or_update_str_version(
			player_t *player,  uint32_t userid, uint32_t u_create_tm,
			uint32_t server_id,
			std::string type, std::string sub_type, uint32_t ope_type, 
			const vector<rankproto::hset_field_t> *fields, uint32_t ttl);

	static int update_star_lottery_info_to_redis(player_t *player, const commonproto::star_lottery_elem_list &elem_list);

	static int get_star_lottery_info_from_redis(player_t *player);

	static int save_battle_simple_report(player_t* player,
			const std::vector<role_info_t>& roles,
			commonproto::pvp_type_t type,
			const std::string pkg, int ttl);
};

#endif
