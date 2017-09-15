#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include "common.h"

#define MAX_PLAYERS_PER_ONLINE  (1500)

class PlayerManager
{
public:

    static int init_player(player_t *player, fdsession_t *fdsess, uint32_t userid);

    static int uninit_player(player_t *player);

    void send_msg_to_all_player(uint32_t cmd, const google::protobuf::Message& message);

    void send_err_to_all_player(int err);

    void send_msg_to_player_in_special_map(uint32_t cmd, uint32_t map_id, google::protobuf::Message& message);

    void on_battle_close();

    void on_battle_center_close();

    bool is_player_has_old_machine(uint32_t userid);

public: //inline functions
    inline uint32_t total_players() {
        return fd_users_.size();
    }

    inline player_t* get_player_by_fd(int fd);

    inline player_t* get_player_by_userid(uint32_t userid);

    static inline player_t* create_player(fdsession_t *fdsess, uint32_t userid);

    inline int add_player_to_manager(player_t *player);

    inline int del_player_from_manager(player_t *player);

    static inline void destroy_player(player_t* player);

    inline void get_userid_list(std::vector<uint32_t>& userid_list);

    inline void get_player_list(std::vector<player_t*>& player_list);

    inline player_t* get_random_player();

    inline void add_player_to_fd_users(player_t *player);
   
    inline void erase_player_from_fd_users(player_t *player);

    inline void add_player_to_uid_users(player_t *player);

    inline void erase_player_from_uid_users(player_t *player);

private:

    typedef std::map<uint32_t, player_t *>::iterator id_users_ptr_t;

    std::map<int, player_t *> fd_users_; // fd映射player
    std::map<uint32_t, player_t *> id_users_; // 米米号映射player
};

inline player_t* PlayerManager::get_player_by_fd(int fd)
{
    std::map<int, player_t*>::iterator it;
    it = fd_users_.find(fd);
    if (it == fd_users_.end()) {
        return NULL; 
    } else {
        return it->second; 
    }
}

inline player_t* PlayerManager::get_player_by_userid(uint32_t userid)
{
    std::map<uint32_t, player_t*>::iterator it;
    it = id_users_.find(userid);
    if (it == id_users_.end()) {
        return NULL; 
    } else {
        return it->second;   
    }
}

inline player_t* PlayerManager::create_player(fdsession_t *fdsess, uint32_t userid)
{
   player_t* player = (player_t *)calloc(1, sizeof(player_t));
    if (player == NULL) {
        ERROR_TLOG("out of memory");
        return NULL; 
    }
    init_player(player, fdsess, userid);
    return player;
}

inline int PlayerManager::add_player_to_manager(player_t *player)
{
    add_player_to_fd_users(player);
    add_player_to_uid_users(player);
    return 0;
}

inline int PlayerManager::del_player_from_manager(player_t *player)
{
    erase_player_from_fd_users(player);
    erase_player_from_uid_users(player);
    return 0;
}

inline void PlayerManager::destroy_player(player_t* player)
{
    uninit_player(player);
    delete player;
}

inline void PlayerManager::get_userid_list(std::vector<uint32_t>& userid_list)
{
    id_users_ptr_t ptr;
    userid_list.clear();
    for (ptr = id_users_.begin(); ptr != id_users_.end(); ptr++) {
        userid_list.push_back(ptr->first);
    }
}

inline void PlayerManager::get_player_list(std::vector<player_t*>& player_list)
{
    id_users_ptr_t ptr;
    player_list.clear();
    for (ptr = id_users_.begin(); ptr != id_users_.end(); ptr++) {
        if (ptr->second->is_login) {
            player_list.push_back(ptr->second);
        }
    }
}

inline player_t* PlayerManager::get_random_player()
{
    size_t size = id_users_.size();
    player_t *p = 0;
    int offset = ranged_random(0, size - 1);
    id_users_ptr_t p_iter = id_users_.begin();
    for (int i = 0; i < offset; i++) {
        p_iter++;
    }
    p = p_iter->second;
    return p;
} 

inline void PlayerManager::add_player_to_fd_users(player_t *player)
{
    fd_users_[player->fdsess->fd] = player;
}

inline void PlayerManager::erase_player_from_fd_users(player_t *player)
{
    fd_users_.erase(player->fdsess->fd);
}

inline void PlayerManager::add_player_to_uid_users(player_t *player)
{
    id_users_[player->userid] = player;
}

inline void PlayerManager::erase_player_from_uid_users(player_t *player)
{
    id_users_.erase(player->userid);
}

#endif
