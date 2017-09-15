#ifndef __MAP_USER_MGR_H__
#define __MAP_USER_MGR_H__

//地图用户管理器
#include "common.h"
#include "map_conf.h"
#include "player.h"

#include "libtaomee/random/random.h"

#define MAX_LINE_PER_MAP    (500)
#define MAX_PLAYERS_PER_LINE    (50)

class map_user_manager_t {
public:
    map_user_manager_t() {
        all_map_players_.clear();
    }
    ~map_user_manager_t() {
        all_map_players_.clear();
    }
    
    //地图切线 1个地图分为N个线 一个线容纳M人
    //<uid, player>
    typedef std::map<uint32_t, player_t*> player_map_t;
    typedef std::map<uint32_t, player_t*>::iterator player_map_iter_t;

    //<line_id, player_map>
    typedef std::map<uint32_t, player_map_t> line_map_t;
    typedef std::map<uint32_t, player_map_t>::iterator line_map_iter_t;

    //<map_id, line_map_t>
    typedef std::map<uint32_t, line_map_t> map_line_map_t;
    typedef std::map<uint32_t, line_map_t>::iterator map_line_map_iter_t;

public: //inline functions
    inline uint32_t get_available_line_id(uint32_t map_id) {
        if (!g_map_conf_mgr.is_map_conf_exist(map_id)) {
            return 0;
        }
        if (all_map_players_.count(map_id) == 0) {
            //第一个进来的人
            return 1; //Line_ID从1开始
        }
        map_line_map_iter_t it = all_map_players_.find(map_id);
        line_map_t &line_map = it->second;
        uint32_t max_line_id = 1;
        FOREACH(line_map, it2) {
            if (it2->second.size() < MAX_PLAYERS_PER_LINE) {
                return it2->first;
            }
            max_line_id = it2->first;
        }
        if (max_line_id == MAX_LINE_PER_MAP) {
            return 0;
        }
        return ++max_line_id;
    }
public:
    inline void get_players_in_the_same_map_line(player_t *p, 
                std::set<const player_t*> &ret_set) {
        assert(p);
        ret_set.clear();
        uint32_t map_id = p->cur_map_id;
        uint32_t line_id = p->cur_map_line_id;
        player_map_t *player_map = get_player_map(map_id, line_id);
        if (!player_map) return;
        FOREACH(*player_map, it) {
            ret_set.insert(it->second);
        }
    }
private:
    inline line_map_t *get_line_map(uint32_t map_id) {
        if (all_map_players_.count(map_id) == 0) {
            return 0;
        }
        return &(all_map_players_.find(map_id)->second);
    }
    inline player_map_t *get_player_map(uint32_t map_id, uint32_t line_id) {
        line_map_t *line_map = get_line_map(map_id);
        if (line_map == 0) return 0;
        if (line_map->count(line_id) == 0) return 0;
        return &(line_map->find(line_id)->second);
    }
    inline player_map_t *new_player_map(uint32_t map_id, uint32_t line_id) {
        line_map_t *line_map = 0;
        if (all_map_players_.count(map_id) == 0) {
            line_map_t tmp;
            tmp.clear();
            all_map_players_[map_id] = tmp;
        }
        line_map = &(all_map_players_.find(map_id)->second);
        player_map_t *player_map = 0;
        if (line_map->count(line_id) == 0) {
            player_map_t tmp;
            tmp.clear();
            line_map->insert(make_pair(line_id, tmp));
        }
        player_map = &(line_map->find(line_id)->second);
        return player_map;
    }

public:
	bool add_player(player_t *p, uint32_t map_id) {
		assert(p);
		const map_conf_t *mc = g_map_conf_mgr.find_map_conf(map_id);
		if (!mc) return false;

		uint32_t line_id = get_available_line_id(map_id);
		if (line_id == 0) return false;
		player_map_t *player_map = get_player_map(map_id, line_id);
		if (player_map == 0) {
			player_map = new_player_map(map_id, line_id);
		}
		(*player_map)[p->userid] = p;
		p->cur_map_line_id = line_id;
		p->cur_map_id = map_id;

		int npos = mc->init_pos.size();

		int pos_index = ranged_random(0, npos-1);

		p->map_x = mc->init_pos[pos_index].x;
		p->map_y = mc->init_pos[pos_index].y;

		return true;
	}
    void del_player(player_t *p) {
        if (!p) return;
        uint32_t map_id = p->cur_map_id;
        uint32_t line_id = p->cur_map_line_id;
        player_map_t *player_map = get_player_map(map_id, line_id);
        if (!player_map) return;
        player_map->erase(p->userid);
        p->cur_map_id = 0;
        p->cur_map_line_id = 0;
        p->map_x = 0;
        p->map_y = 0;
    }

private:
    map_line_map_t all_map_players_;
};

extern map_user_manager_t g_map_user_mgr;

#endif
