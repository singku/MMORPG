#ifndef __PLAYER_MANAGER_H__
#define __PLAYER_MANAGER_H__

#include "common.h"
#include "singleton.h"
#include "player.h"

class player_manager_t {
public:
	typedef std::map<uint32_t, player_t*> uid_to_player_map_t;
	typedef uid_to_player_map_t::iterator uid_to_player_map_iter_t;

public:
	player_t *create_new_player(uint32_t uid, fdsession_t *fdsess);
    player_t *create_artifacial_player(uint32_t uid, fdsession_t *fdsess);
    void batch_del_players(int fd);
    void del_player(player_t *p);

public: //inline funcs
	inline player_t *get_player_by_uid(uint32_t uid, bool is_artifacial) {
        uid_to_player_map_iter_t iter;
        if (is_artifacial) {
            iter = uid_to_artifacial_player_map_.find(uid);
            if (iter == uid_to_artifacial_player_map_.end()) { return 0; }

        } else {
            iter = uid_to_player_map_.find(uid);
            if (iter == uid_to_player_map_.end()) { return 0; }
        }
        return iter->second;
    }
	inline player_t *get_artifacial_player_by_uid(uint32_t uid) {
        uid_to_player_map_iter_t iter = uid_to_artifacial_player_map_.find(uid);
        if (iter == uid_to_artifacial_player_map_.end()) { return 0; }
        return iter->second;
    }
    inline void inc_player_num() {
        total_player_num_++;
    }
    inline void dec_player_num() {
        if (unlikely(total_player_num_ == 0)) return;
        total_player_num_--;
    }
    inline uint32_t get_total_player_num() {
        return total_player_num_;
    }

private:
	void add_player(player_t *p);

private:
	uid_to_player_map_t uid_to_player_map_;
    uid_to_player_map_t uid_to_artifacial_player_map_; //临时玩家
	int32_t total_player_num_; // 总人数
};

typedef singleton_default<player_manager_t> player_manager_singleton_t;
#define PLAYER_MGR (player_manager_singleton_t::instance())

#endif // __PLAYER_MANAGER_H__
