
#ifndef ATTR_UTILS_H
#define ATTR_UTILS_H

#include "common.h"
#include "attr.h"
#include "item_conf.h"
#include "pet_conf.h"

class AttrUtils
{
public:

    static uint32_t get_attr_max_limit(const player_t *player, enum attr_type_t type); 
    static uint32_t get_attr_initial_value(const player_t *player, enum attr_type_t type); 

    static uint32_t get_attr_value(const player_t* player, enum attr_type_t type);
    static bool is_valid_cli_attr(uint32_t type);
    static bool is_valid_cli_get_attr(uint32_t type);
    /**
     * @brief  set_attr_value 设置attr值，写入数据库，并同步客户端
     *
     * @param player 玩家结构
     * @param num_attr attrs数组个数
     * @param attrs attr数组
     * @param wait_ret 是否等待数据库返回
     * @return 0 成功 其他参见attr_op_err_t
     */
    static int set_attr_value(player_t* player, 
            uint32_t num_attr, attr_data_info_t *attrs,
            bool wait_ret = NO_WAIT_SVR,
            onlineproto::syn_attr_reason_t syn_attr_reason = onlineproto::ATTR_OTHER_REASON);

    static int set_attr_value(player_t* player, 
            std::vector<attr_data_info_t> &attr_vec,
            bool wait_ret = NO_WAIT_SVR,
            onlineproto::syn_attr_reason_t syn_attr_reason = onlineproto::ATTR_OTHER_REASON);

    static int set_attr_value(player_t* player, 
            std::map<uint32_t, uint32_t> &attr_map,
            bool wait_ret = NO_WAIT_SVR,
            onlineproto::syn_attr_reason_t syn_attr_reason = onlineproto::ATTR_OTHER_REASON);

    static int set_single_attr_value(player_t* player,
            enum attr_type_t type, uint32_t value, 
            bool wait_ret = NO_WAIT_SVR,
            onlineproto::syn_attr_reason_t syn_attr_reason = onlineproto::ATTR_OTHER_REASON);

    static int sub_attr_value(player_t* player, 
            enum attr_type_t type, uint32_t value, 
            bool wait_ret = NO_WAIT_SVR,
            onlineproto::syn_attr_reason_t syn_attr_reason = onlineproto::ATTR_OTHER_REASON);

    static int add_attr_value(player_t* player,
            enum attr_type_t type, uint32_t value, 
            bool wait_ret = NO_WAIT_SVR,
            onlineproto::syn_attr_reason_t syn_attr_reason = onlineproto::ATTR_OTHER_REASON);

	//缓存前一天daily attr
	static int cache_last_day_attr_value(player_t* player);
	//资源找回
	static int set_resource_retrieve_value(player_t *player);
    static int clear_special_attr_value(player_t *player, bool noti_client);
    static int reset_daily_attr_value(player_t* player, bool noti_client);
    static int reset_weekly_attr_value(player_t* player, bool noti_client);
    static int reset_monthly_attr_value(player_t* player, bool noti_client);
    static int ranged_clear(player_t* player, uint32_t low, uint32_t high, bool noti_client);
    static int ranged_reset(player_t* player, uint32_t low, uint32_t high, bool noti_client);

    static attr_type_t get_duplicate_daily_times_attr(uint32_t dup_id);
    static attr_type_t get_duplicate_daily_reset_times_attr(uint32_t dup_id);
    static attr_type_t get_duplicate_weekly_times_attr(uint32_t dup_id);
    static attr_type_t get_duplicate_pass_time_attr(uint32_t dup_id);
    static attr_type_t get_duplicate_best_time_attr(uint32_t dup_id);
    static attr_type_t get_duplicate_best_star_attr(uint32_t dup_id);
    static attr_type_t get_duplicate_last_play_time_attr(uint32_t dup_id);
	static attr_type_t get_tran_card_id_attr(uint32_t card_id);
	static attr_type_t get_tran_card_star_level_attr(uint32_t card_id);
	static attr_type_t get_tran_choose_flag(uint32_t card_id);

	//获得最近一次通过该副本所使用的时间属性id
	static attr_type_t get_dup_pass_tm_duration_attr(uint32_t dup_id);
	static uint32_t get_pet_pass_dup_tm_attr(uint32_t dup_id, uint32_t pet_id,
			uint32_t& type);

	static attr_type_t get_dup_revival_cnt_attr(uint32_t dup_id);
	
	static attr_type_t get_dup_surplus_hp_percent(uint32_t dup_id);
	
	static attr_type_t get_dup_power_record_attr(uint32_t dup_id);

    static const attr_config_t* get_attr_config(attr_type_t attr_type);

	static void register_stat_func();
	
    static bool is_player_gold_enough(player_t *player, uint32_t need_num);
    static uint32_t get_player_gold(player_t *player);
    static uint32_t add_player_gold(player_t *player, uint32_t add_num, 
            bool gold_from_diamond, string get_stat_name);
    static uint32_t sub_player_gold(player_t *player, uint32_t sub_num, string consume_stat_name);

    /**
     * @brief 装备或者精灵的属性转化给玩家的时候 找到对应的玩家的属性值id
    */
    //根据装备附加属性找到对应的玩家属性
    static attr_type_t get_player_attr_by_equip_attr(equip_add_attr_t equip_attr);
    //根据精灵普通属性找到对应的玩家属性
    static attr_type_t get_player_attr_by_pet_normal_attr(battle_value_normal_type_t pet_normal_attr);
    //根据精灵隐藏属性找到对应的玩家属性
    static attr_type_t get_player_attr_by_pet_hide_attr(battle_value_hide_type_t pet_hide_attr);
    //根据装备刻印属性找到对应的精灵属性
    static battle_value_normal_type_t get_pet_attr_by_equip_chisel_attr(equip_add_attr_t equip_attr);
    //根据装备位置找到对应的玩家主属性
    static attr_type_t get_player_attr_by_equip_pos(equip_body_pos_t pos);
    //根据装备的元素属性找到对应的玩家抗性属性
    static attr_type_t get_player_anti_attr_by_equip_elem(uint32_t elem_type);
    //根据装备的洗练1属性找到玩家的隐藏属性
    static attr_type_t get_player_attr_by_quench_type(uint32_t quench_type);


	static bool has_attr(const player_t* player, enum attr_type_t type);
    static int update_other_attr_value(
            uint32_t userid, uint32_t u_create_tm, std::vector<commonproto::attr_data_t>& attr_vec);

    static int update_element_dup_attr_type(player_t *player);

	static int change_other_attr_value_pub(
			uint32_t userid, uint32_t create_tm,
			uint32_t type, uint32_t change_value, bool is_minus);

	/**
	 * @brief 活动范围内的属性值增加
	 * @param key   time_config.xml中的id 
	 * @param sub_key time_config.xml中的tid
	 * @param type 属性id
	 * @param value 增加的值
	 */
	static int add_attr_in_special_time_range(player_t* player,
			uint32_t key, uint32_t sub_key,
			attr_type_t type, uint32_t value = 1);

private:
	static int change_other_attr_value(
			uint32_t userid, uint32_t create_tm,
			std::vector<attr_data_chg_info_t>& attr_vec);

};


#endif
