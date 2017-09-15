#ifndef __PLAYER_MANAGER_H__
#define __PLAYER_MANAGER_H__

#include "common.h"
#include "singleton.h"
#include "player.h"
#include <vector>
class player_manager_t {
public:
	typedef std::map<uint32_t, player_t*> uid_to_player_map_t;
	typedef uid_to_player_map_t::iterator uid_to_player_map_iter_t;

	player_manager_t(void){
        total_player_num_ = 0;
        vip_player_num_ = 0;
    }

public:
	player_t *create_new_player(player_basic_t *basic);
	void add_player(player_t *p, uint32_t in_server_id, uint32_t in_online_id);
	void del_player(player_t *p);
    void batch_del_players(uint32_t online_id);
    void get_player_list(std::vector<role_info_t> &players, uint32_t server_id);

public: //inline funcs
	inline player_t *get_player_by_uid(uint32_t uid) {
        uid_to_player_map_iter_t iter = uid_to_player_map_.find(uid);
        if (iter == uid_to_player_map_.end()) { return 0; }
        return iter->second;
    }
    inline void inc_player_num(bool is_vip, uint32_t svr_id, uint32_t net = TELCOM) {
        total_player_num_++;
        total_player_num_of_svrs_[svr_id]++;

        if (is_vip) {
            vip_player_num_++;
            total_vip_player_num_of_svrs_[svr_id]++;
        }
        if (net == TELCOM) {
            total_player_on_tel_++;
            total_tel_player_num_of_svrs_[svr_id]++;
        } else {
            total_player_on_net_++;
            total_net_player_num_of_svrs_[svr_id]++;
        }
    }
    inline void dec_player_num(bool is_vip, uint32_t svr_id, uint32_t net = TELCOM) {
        if (net == TELCOM) {
            if (likely(total_player_on_tel_ != 0)) {
                total_player_on_tel_ --;
            }
            if (likely(total_tel_player_num_of_svrs_[svr_id] != 0)) {
                total_tel_player_num_of_svrs_[svr_id] --;
            }
        } else if (net == NETCOM) {
            if (likely(total_player_on_net_ != 0)) {
                total_player_on_net_ --;
            }
            if (likely(total_net_player_num_of_svrs_[svr_id] != 0)) {
                total_net_player_num_of_svrs_[svr_id] --;
            }
        }
        if (likely(total_player_num_ != 0)) {
            total_player_num_--;
        }
        if (likely(total_player_num_of_svrs_[svr_id] != 0)) {
            total_player_num_of_svrs_[svr_id] --;
        }
        if (is_vip && likely(vip_player_num_ != 0)) {
            vip_player_num_ --;
        }
        if (is_vip && likely(total_vip_player_num_of_svrs_[svr_id] != 0)) {
            total_vip_player_num_of_svrs_[svr_id] --;
        }
    }
    inline uint32_t get_total_player_num(uint32_t svr_id) {
        if (svr_id == 0) return total_player_num_;
        return total_player_num_of_svrs_[svr_id];
    }
    inline uint32_t get_total_vip_player_num(uint32_t svr_id) {
        if (svr_id == 0) return vip_player_num_;
        return total_vip_player_num_of_svrs_[svr_id];

    }
    inline uint32_t get_total_net_player_num(uint32_t svr_id) {
        if (svr_id == 0) return total_player_on_net_;
        return total_net_player_num_of_svrs_[svr_id];
    }
    inline uint32_t get_total_tel_player_num(uint32_t svr_id) {
        if (svr_id == 0) return total_player_on_tel_;
        return total_tel_player_num_of_svrs_[svr_id];
    }

private:
	uid_to_player_map_t uid_to_player_map_;
	int32_t total_player_num_; // 总人数
    std::map<uint32_t, uint32_t> total_player_num_of_svrs_; //<svr_id, players_cnt>

    int32_t vip_player_num_; //VIP总人数
    std::map<uint32_t, uint32_t> total_vip_player_num_of_svrs_; //<svr_id, players_cnt> 

    int32_t total_player_on_net_; //网通总人数
    std::map<uint32_t, uint32_t> total_net_player_num_of_svrs_; //<svr_id, players_cnt>

    int32_t total_player_on_tel_; //电信总人数
    std::map<uint32_t, uint32_t> total_tel_player_num_of_svrs_; //<svr_id, players_cnt>
};

typedef singleton_default<player_manager_t> player_manager_singleton_t;
#define PLAYER_MGR (player_manager_singleton_t::instance())

#endif // __PLAYER_MANAGER_H__
