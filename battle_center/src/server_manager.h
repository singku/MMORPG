#ifndef __SERVER_MANAGER_H__
#define __SERVER_MANAGER_H__

#include "common.h"
#include "singleton.h"

#include "player_manager.h"
class player_t;

class server_t
{
public:  
    server_t(const std::string& server_name) {
        server_name_ = server_name;
        fd_ = -1;
    }
    ~server_t() {
        if (fd_ >= 0) {
            close_svr(fd_);
        }
    }

    int connect();
    int close();
    int send_msg_to_svr(uint32_t uid, uint16_t cmd, const google::protobuf::Message &message);

    int send_buf_to_svr(uint32_t userid, uint16_t cmd, void *data, uint32_t data_len);

public:
    inline bool is_connected() {
        return (fd_ >= 0);
    }

    inline const std::string& server_name() const {
        return server_name_; 
    }
    inline int fd() const {
        return fd_;
    }
    inline void set_fd(int fd) {
        fd_ = fd; 
    }
    inline void clear_players_on_me() {
        all_players_on_me_.clear();
    }
    inline std::set<player_t *> &all_players_on_me() {
        return all_players_on_me_;
    }
    inline void add_player(player_t *player) {
        //DEBUG_TLOG("P:%u my btl:%p, bef_add size:%u", player->uid, this, all_players_on_me_.size());
        all_players_on_me_.insert(player);
        //DEBUG_TLOG("P:%u my btl:%p, aft_add size:%u", player->uid, this, all_players_on_me_.size());
    }
    inline void del_player(player_t *player) {
        //DEBUG_TLOG("P:%u my btl:%p, bef_del size:%u", player->uid, this, all_players_on_me_.size());
        all_players_on_me_.erase(player);
        //DEBUG_TLOG("P:%u my btl:%p, aft_del size:%u", player->uid, this, all_players_on_me_.size());
    }
private:
    int fd_;
    std::string server_name_;
    std::set<player_t*> all_players_on_me_;
};

class server_manager_t {
public:
	typedef std::map<string, server_t*> svrnm_to_server_map_t;
	typedef svrnm_to_server_map_t::iterator svrnm_to_server_map_iter_t;
    typedef svrnm_to_server_map_t::const_iterator svrnm_to_server_map_const_iter_t;

    typedef std::map<int, server_t*> fd_to_server_map_t;
    typedef std::map<int, server_t*>::iterator fd_to_server_map_iter_t;

	server_t *create_new_server(string server_name);
	void del_server(server_t *svr);

public: //inline funcs
    inline server_t *get_server_by_name(string name) {
        svrnm_to_server_map_iter_t iter = svrnm_to_server_map_.find(name);
        if (iter == svrnm_to_server_map_.end()) { return 0; }
        return iter->second;
    }
    inline server_t *get_server_by_fd(int fd) {
        fd_to_server_map_iter_t iter = fd_to_server_map_.find(fd);
        if (iter == fd_to_server_map_.end()) { return 0; }
        return iter->second;
    }
    inline void add_svr_to_fd_svr_map(server_t *svr) {
        fd_to_server_map_[svr->fd()] = svr;
    }

public:
    const svrnm_to_server_map_t &all_server_map() {return svrnm_to_server_map_;}
private:
	svrnm_to_server_map_t svrnm_to_server_map_;
    fd_to_server_map_t fd_to_server_map_;
};

typedef singleton_default<server_manager_t> server_manager_singleton_t;
#define SERVER_MGR (server_manager_singleton_t::instance())



#endif // __SERVER_MANAGER_H__
