#include "player_manager.h"

PlayerManager PLAYER_MGR;

player_t* PlayerManager::get_player_by_role(role_info_t role)
{
    std::map<uint64_t, player_t*>::iterator it;
    it = player_map_.find(ROLE_KEY(role));
    if (it == player_map_.end()) {
        return NULL; 
    } else {
        return it->second;   
    }
}

player_t* PlayerManager::create_new_player(role_info_t role,
        fdsession_t *fdsess, uint32_t cur_seq, uint32_t home_type)
{
    player_t *player = new player_t();

    if (player == NULL) {
        KERROR_LOG(0, "out of memory");
        return NULL; 
    }

    player->set_userid(role.userid);
    player->set_fdsess(fdsess);
    player->set_cur_seq(cur_seq);
    player->set_u_create_tm(role.u_create_tm);
	if (home_type == 0) {
		home_type = DEFAULT_HOME_TYPE;
	}  
    if (home_type) {
        player->init_home_info();
        player->set_home_info_loaded();
		player->set_home_type(home_type);
	}
    player_map_[ROLE_KEY(player->role())] = player;
    return player;
}

void PlayerManager::delete_player(role_info_t role)
{
    std::map<uint64_t, player_t*>::iterator it;
    it = player_map_.find(ROLE_KEY(role));
    if (it == player_map_.end()) return;

    delete it->second;
    player_map_.erase(it);
}
