#ifndef __PLAYER_CONF_H__
#define __PLAYER_CONF_H__

#include "common.h"

#define PLAYER_MAX_LEVEL    (60)
#define PLAYER_MAX_HP       (50000)

//玩家职业类型
enum prof_type_t {
    PROF_NONE           = 0, //无职业
    PROF_WARRIOR        = commonproto::PROF_WARRIOR, //战士
    PROF_WIZARD         = commonproto::PROF_WIZARD, //法师
    PROF_ARCHER         = commonproto::PROF_ARCHER, //弓箭手
    PROF_COUNT
};

//vip类型
enum player_vip_type_t {
	NO_VIP = 0, 	//非vip
	SILVER_VIP = 1, //白银vip
	GOLD_VIP = 2,	//黄金vip
};

struct player_conf_t {
    uint32_t player_id; // id
    prof_type_t prof; //职业

    uint32_t basic_normal_battle_values[kMaxBattleValueTypeNum]; // 基础战斗属性
    uint32_t basic_normal_battle_values_grow[kMaxBattleValueTypeNum]; //基础战斗属性的等级成长率
    uint32_t basic_hide_battle_values[kMaxBattleValueHideTypeNum]; //基础的隐藏战斗属性值
    uint32_t basic_hide_battle_values_grow[kMaxBattleValueHideTypeNum]; //基础的隐藏战斗属性按等级的成长率
    uint32_t basic_hide_battle_values_coeff[kMaxBattleValueHideTypeNum]; //隐藏战斗属性成长系数
};

class player_conf_mgr_t {
public:
    player_conf_mgr_t() {
        clear();
    }
    ~player_conf_mgr_t() {
        clear();
    }
    inline void clear() {
        player_conf_map_.clear();
    }
    
    inline const std::map<uint32_t, player_conf_t> &const_player_conf_map() const {
        return player_conf_map_;
    }
    
    inline void copy_from(const player_conf_mgr_t &m) {
        player_conf_map_ = m.const_player_conf_map();
    }
    
    inline bool is_player_conf_exist(uint32_t player_id) {
        if (player_conf_map_.count(player_id) > 0) return true;
        return false;
    }
    
    inline bool add_player_conf(const player_conf_t &item) {
        if (is_player_conf_exist(item.player_id)) return false;
        player_conf_map_[item.player_id] = item; return true;
    }
    
    inline const player_conf_t *find_player_conf(uint32_t player_id) {
        if (!is_player_conf_exist(player_id)) return 0;
        return &((player_conf_map_.find(player_id))->second);
    }

    static inline bool is_valid_prof(uint32_t prof) {
        return (prof && prof <= (uint32_t)(PROF_ARCHER));
    }
private:
    std::map<uint32_t, player_conf_t> player_conf_map_;
};

struct player_power_rank_conf_t
{
	uint32_t userid;
	uint32_t power_rank;
	uint32_t power_value;
	uint32_t test_stage;
	uint32_t prize_id;
	uint32_t extra_prize_id;
};

class player_power_rank_conf_mgr_t
{
private:
	std::vector<player_power_rank_conf_t> power_rank_1st_vec_;
	std::vector<player_power_rank_conf_t> power_rank_2nd_vec_;
public:
	player_power_rank_conf_mgr_t() {
		clear();
	}
	~player_power_rank_conf_mgr_t() {
		clear();
	}
	inline void clear() {
		power_rank_1st_vec_.clear();
		power_rank_2nd_vec_.clear();
	}
	inline const std::vector<player_power_rank_conf_t> &const_power_1st_conf_vec() const {
		return power_rank_1st_vec_;
	}
	inline const std::vector<player_power_rank_conf_t> &const_power_2nd_conf_vec() const {
		return power_rank_2nd_vec_;
	}

	inline void copy_from(const player_power_rank_conf_mgr_t &m) {
		power_rank_1st_vec_ = m.const_power_1st_conf_vec();
		power_rank_2nd_vec_ = m.const_power_2nd_conf_vec();
	}
	
	inline bool is_power_rank_exist_in_1st(uint32_t userid) {
		FOREACH(power_rank_1st_vec_, it) {
			if (it->userid == userid) {
				return true;
			}
		}
		return false;
	}
	inline bool is_power_rank_exist_in_2nd(uint32_t userid) {
		FOREACH(power_rank_2nd_vec_, it) {
			if (it->userid == userid) {
				return true;
			}
		}
		return false;
	}
	inline const player_power_rank_conf_t* find_player_info_in_1st(uint32_t userid) {
		FOREACH(power_rank_1st_vec_, it) {
			if (it->userid == userid) {
				return &(*it);
			}
		}
		return NULL;
	}
	inline const player_power_rank_conf_t* find_player_info_in_2nd(uint32_t userid) {
		FOREACH(power_rank_2nd_vec_, it) {
			if (it->userid == userid) {
				return &(*it);
			}
		}
		return NULL;
	}
	inline bool add_power_rank_conf(const player_power_rank_conf_t& item) {
		if (item.test_stage == 1 && !is_power_rank_exist_in_1st(item.userid)) {
			power_rank_1st_vec_.push_back(item);
			return true;
		} else if (item.test_stage == 2 && !is_power_rank_exist_in_2nd(item.userid)) {
			power_rank_2nd_vec_.push_back(item);
			return true;
		}
		return false;
	}
	inline void print_power_rank_info() {
		FOREACH(power_rank_1st_vec_, it) {
			uint32_t userid = it->userid;
			uint32_t power_rank = it->power_rank;
			uint32_t power_value = it->power_value;
			uint32_t test_stage = it->test_stage;
			uint32_t prize_id = it->prize_id;
			TRACE_TLOG("load config 1player power rank,userid=[%u],"
					"power_rank=[%u],value=[%u],test_stage=[%u],prize_id=[%u]",
					userid, power_rank, power_value, test_stage, prize_id);
		}
		FOREACH(power_rank_2nd_vec_, it) {
			uint32_t userid = it->userid;
			uint32_t power_rank = it->power_rank;
			uint32_t power_value = it->power_value;
			uint32_t test_stage = it->test_stage;
			uint32_t prize_id = it->prize_id;
			TRACE_TLOG("load config 2player power rank,userid=[%u],"
					"power_rank=[%u],value=[%u],test_stage=[%u],prize_id=[%u]",
					userid, power_rank, power_value, test_stage, prize_id);
		}
	}
};

class joined_test_userid_conf_mgr_t
{
private:
	std::vector<uint32_t> first_test_uids;
	std::vector<uint32_t> second_test_uids;
public:
	joined_test_userid_conf_mgr_t() {
		clear_first_uids();
		clear_second_uids();
	}
	~joined_test_userid_conf_mgr_t() {
		clear_first_uids();
		clear_second_uids();
	}
	inline void clear_first_uids() {
		first_test_uids.clear();
	}
	inline void clear_second_uids() {
		second_test_uids.clear();
	}
	inline bool add_first_test_uid(uint32_t uid) {
		std::vector<uint32_t>::iterator it;
		it = std::find(first_test_uids.begin(), first_test_uids.end(), uid);
		if (it != first_test_uids.end()) {
			return false;
		}
		first_test_uids.push_back(uid);
		return true;
	}
	inline bool add_second_test_uid(uint32_t uid) {
		std::vector<uint32_t>::iterator it;
		it = std::find(second_test_uids.begin(), second_test_uids.end(), uid);
		if (it != second_test_uids.end()) {
			return false;
		}
		second_test_uids.push_back(uid);
		return true;
	}

	inline const std::vector<uint32_t>& const_first_test_uid() const {
		return first_test_uids;
	}

	inline void copy_first_from(const joined_test_userid_conf_mgr_t &m) {
		first_test_uids = m.const_first_test_uid();	
	}

	inline const std::vector<uint32_t>& const_second_test_uid() const {
		return second_test_uids;
	}

	inline void copy_second_from(const joined_test_userid_conf_mgr_t& m) {
		second_test_uids = m.const_second_test_uid();
	}

	inline bool is_uid_exist_in_1st(uint32_t uid) {
		std::vector<uint32_t>::iterator it;
		it = std::find(first_test_uids.begin(), first_test_uids.end(), uid);
		if (it == first_test_uids.end()) {
			return false;
		}
		return true;
	}
	inline bool is_uid_exist_in_2nd(uint32_t uid) {
		std::vector<uint32_t>::iterator it;
		it = std::find(second_test_uids.begin(), second_test_uids.end(), uid);
		if (it == second_test_uids.end()) {
			return false;
		}
		return true;
	}
	void print_1st_test_uid_info() {
		FOREACH(first_test_uids, it) {
			TRACE_TLOG("joined_1st,uid=%u", *it);
		}
	}
	void print_2nd_test_uid_info() {
		FOREACH(second_test_uids, it) {
			TRACE_TLOG("joined_2nd,uid=%u", *it);
		}
	}
};

extern player_conf_mgr_t g_player_conf_mgr;
extern player_power_rank_conf_mgr_t g_ply_power_rank_conf_mgr;
extern joined_test_userid_conf_mgr_t g_joined_test_uid_conf_mgr;

#endif
