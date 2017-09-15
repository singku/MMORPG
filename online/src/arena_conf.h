#ifndef __ARENA_CONF_H__
#define __ARENA_CONF_H__
#include "arena.h"

//竞技场连胜奖励
class arena_streak_reward_conf_mgr_t
{
public:
	typedef std::map<uint32_t, streak_info_t> ArenaStreakConfMgr;
	arena_streak_reward_conf_mgr_t() {
		clear();
	}
	~arena_streak_reward_conf_mgr_t() {
		clear();
	}
	inline void clear() {
		arena_streak_conf_.clear();
	}
	inline const ArenaStreakConfMgr& const_streak_conf_map() const {
		return arena_streak_conf_;
	}
	inline void copy_from(const arena_streak_reward_conf_mgr_t &m) {
		arena_streak_conf_ = m.const_streak_conf_map();
	}
	inline bool is_streak_conf_exist(uint32_t count) {
		return arena_streak_conf_.count(count) > 0 ? true : false;
	}
	inline streak_info_t* get_streak_conf_info(uint32_t count) {
		ArenaStreakConfMgr::iterator it = arena_streak_conf_.find(count);
		if (it != arena_streak_conf_.end()) {
			return &it->second;
		}
		return NULL;
	}
	inline bool add_streak_conf(const streak_info_t &streak_info) {
		if (is_streak_conf_exist(streak_info.count)) {
			return false;
		}
		arena_streak_conf_.insert(ArenaStreakConfMgr::value_type(streak_info.count, streak_info));
		return true;
	}
	inline void print_streak_info() {
		FOREACH(arena_streak_conf_, it) {
			uint32_t count = it->second.count;
			uint32_t prize_id = it->second.prize_id;
			/*
			uint32_t item_id = it->second.item_id;
			uint32_t item_count = it->second.item_count;
			uint32_t add_exp = it->second.add_exp;
			uint32_t coin = it->second.coin;
			uint32_t arenacoin = it->second.arenacoin;
			*/
			/*
			TRACE_TLOG("load arena streak:count=[%u],item_id=[%u],item_count=[%u],add_exp=[%u]"
					"coin=[%u],arenacoin=[%u]", count, item_id, item_count, add_exp, coin, arenacoin);
			*/
			TRACE_TLOG("load arena streak:count=[%u],prize_id=[%u]",
					count, prize_id);
		}
	}
private:
	ArenaStreakConfMgr arena_streak_conf_;
};

//竞技场排名奖励
class arena_rank_reward_mgr_t
{
public:
	typedef std::map<uint32_t, rank_reward_t> RankRewardConfMgr;
	inline rank_reward_t* get_arena_rank_reward(uint32_t rank) {
		FOREACH(rank_reward_conf_, it) {
			if (it->second.start_rank <= rank && it->second.end_rank >= rank) {
				return &it->second;
			}
		}
		return NULL;
	}
	arena_rank_reward_mgr_t() {
		clear();
	}
	~arena_rank_reward_mgr_t() {
		clear();
	}
	inline void clear() {
		rank_reward_conf_.clear();
	}
	inline const RankRewardConfMgr& const_rank_reward_conf_map() const {
		return rank_reward_conf_;
	}
	inline void copy_from(const arena_rank_reward_mgr_t &m) {
		rank_reward_conf_ = m.const_rank_reward_conf_map();
	}
	inline bool is_rank_reward_conf_exist(uint32_t id) {
		RankRewardConfMgr::iterator it = rank_reward_conf_.find(id);
		if (it != rank_reward_conf_.end()) {
			return &it->second;
			return true;
		}
		return false;
	}
	inline bool add_rank_reward_conf(const rank_reward_t& reward) {
		if (is_rank_reward_conf_exist(reward.id)) {
			return false;
		}
		rank_reward_conf_.insert(RankRewardConfMgr::value_type(reward.id, reward));
		return true;
	}
	inline void print_rank_reward_info() {
		FOREACH(rank_reward_conf_, it) {
			uint32_t id = it->second.id;
			uint32_t start_rank = it->second.start_rank;
			uint32_t end_rank = it->second.end_rank;
			uint32_t bonus_id = it->second.bonus_id;
			uint32_t single_reward = it->second.single_reward;
			/*
			uint32_t item_id = it->second.item_id;
			uint32_t item_count = it->second.item_count;
			uint32_t add_exp = it->second.add_exp;
			uint32_t coin = it->second.coin;
			uint32_t arenacoin = it->second.arenacoin;
			*/
			/*
			TRACE_TLOG("print_rank_reward_info:id=[%u],start=[%u],end=[%u],bonus_id=[%u],item_id=[%u]"
					"item_count=[%u],add_exp=[%u],coin=[%u],arenacoin=[%u]", id, start_rank, end_rank,
					bonus_id, item_id, item_count, add_exp, coin, arenacoin);
			*/
			TRACE_TLOG("print_rank_reward_info:id=[%u],start=[%u],end=[%u],bonus_id=[%u],reward=[%u]", 
				id, start_rank, end_rank, bonus_id, single_reward);
		}
		
	}
private: 
	RankRewardConfMgr rank_reward_conf_;
};

struct rpvp_score_reward_conf_t {
    rpvp_score_reward_conf_t() {
        id = 0;
        min = 0;
        max = 0;
        prize_id = 0;
    }
    uint32_t id;
    uint32_t min;
    uint32_t max;
    uint32_t prize_id;
};

struct rpvp_rank_reward_conf_t {
    rpvp_rank_reward_conf_t() {
        id = 0;
        min = 0;
        max = 0;
        prize_id = 0;
    }
    uint32_t id;
    uint32_t min;
    uint32_t max;
    uint32_t prize_id;
};

//手动竞技场奖励
class rpvp_reward_mgr_t {
public:
	rpvp_reward_mgr_t() {
		clear();
	}
	~rpvp_reward_mgr_t() {
		clear();
	}

	inline void clear() {
		rpvp_score_reward_conf_map_.clear();
        rpvp_rank_reward_conf_map_.clear();
	}

    uint32_t find_rpvp_prize_id_by_score(uint32_t score) {
        FOREACH(rpvp_score_reward_conf_map_, it) {
            const rpvp_score_reward_conf_t &conf = it->second;
            if (score >= conf.min && score < conf.max) {
                return conf.prize_id;
            }
        }
        return 0;
    }

    uint32_t find_rpvp_prize_id_by_rank(uint32_t rank) {
        FOREACH(rpvp_rank_reward_conf_map_, it) {
            const rpvp_rank_reward_conf_t &conf = it->second;
            if (rank >= conf.min && rank <= conf.max) {
                return conf.prize_id;
            }
        }
        return 0;
    }

	inline const std::map<uint32_t, rpvp_score_reward_conf_t> &const_rpvp_score_reward_conf_map() const {
		return rpvp_score_reward_conf_map_;
	}

	inline const std::map<uint32_t, rpvp_rank_reward_conf_t> &const_rpvp_rank_reward_conf_map() const {
		return rpvp_rank_reward_conf_map_;
	}

	inline void copy_from(const rpvp_reward_mgr_t &m) {
		rpvp_score_reward_conf_map_ = m.const_rpvp_score_reward_conf_map();
        rpvp_rank_reward_conf_map_ = m.const_rpvp_rank_reward_conf_map();

	}

	inline bool is_rpvp_score_reward_conf_exist(uint32_t id) {
        if (rpvp_score_reward_conf_map_.count(id) == 0) {
            return false;
        }
        return true;
	}

	inline bool is_rpvp_rank_reward_conf_exist(uint32_t id) {
        if (rpvp_rank_reward_conf_map_.count(id) == 0) {
            return false;
        }
        return true;
	}

	inline bool add_rpvp_score_reward_conf(const rpvp_score_reward_conf_t &reward) {
		if (is_rpvp_score_reward_conf_exist(reward.id)) {
			return false;
		}
		rpvp_score_reward_conf_map_[reward.id] = reward;
		return true;
	}

	inline bool add_rpvp_rank_reward_conf(const rpvp_rank_reward_conf_t &reward) {
		if (is_rpvp_rank_reward_conf_exist(reward.id)) {
			return false;
		}
		rpvp_rank_reward_conf_map_[reward.id] = reward;
		return true;
	}
private: 
    std::map<uint32_t, rpvp_score_reward_conf_t> rpvp_score_reward_conf_map_;
    std::map<uint32_t, rpvp_rank_reward_conf_t> rpvp_rank_reward_conf_map_;
};

#endif
