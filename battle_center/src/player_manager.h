#ifndef __PLAYER_MANAGER_H__
#define __PLAYER_MANAGER_H__

#include "common.h"
#include "singleton.h"

#define MATCH_TIME (30)

class server_t;
/**
 * @brief player_t 管理一个玩家的所有数据
 */
struct player_t {
    player_t () {
        uid = 0;
        score = 0;
        fdsess = 0;
        btl_name.clear();
        request_tm = 0;
        index = 0;
        btl = 0;
        btl_info.clear();
        dup_id = 0;
        map_id = 0;
        svr_id = 0;
        need_team_members = 1; //既然匹配了 肯定至少要1个
        seq = 0;
        btl_start = false;
    }
    int send_msg_to_player(uint32_t cmd, const google::protobuf::Message &message);
    int send_err_to_player(uint32_t cmd, int err);
    int relay_to_player(void *data, uint32_t cmd, uint32_t data_len);
    int relay_to_svr(void *data, uint32_t cmd, uint32_t data_len);

    uint32_t uid; //uid
    uint32_t score; //积分
    fdsession_t *fdsess; //online的fdsess.
    string btl_name; //btl的名字
    server_t *btl; //所连接到的btl服务
    uint32_t request_tm; //发起连接的起始时间
    uint32_t index; //进入服务器的序号
    uint32_t dup_id;
    uint32_t map_id;
    uint32_t svr_id; //所在服的ID
    uint32_t need_team_members; //至少需要匹配到的人数
    string btl_info;
    uint32_t seq;
    bool btl_start;
};

class player_manager_t {
public:
	typedef std::map<uint32_t, player_t*> id_to_player_map_t;
	typedef id_to_player_map_t::iterator id_to_player_map_iter_t;

    //dup_id, players
    typedef std::map<uint32_t, id_to_player_map_t> dupid_to_players_map_t;
    //svr_id, dup_id, players
    typedef std::map<uint32_t, dupid_to_players_map_t> svrid_dupid_players_map_t;

    player_manager_t() {
        index = 1;
        uid_to_player_map_.clear();
        rpvp_index_to_player_map_.clear();
        ppve_players_map_.clear();
    }

public:
    //根据player中的信息创建一个player并加入管理器
	player_t *create_new_player(player_t &player);
    //把玩家从管理器中删除
	void destroy_player(player_t *p);
    //当online_fd断线之后,按照fd批量删除玩家(要先发通知给btl)
    void batch_destroy_players(int fd);
    void add_player_to_rpvp_match(player_t *p);
    void add_player_to_ppve_match(player_t *p, uint32_t svr_id, uint32_t dup_id);

    //PVP匹配对手
    void do_rpvp_match();
    void do_ppve_match();

public: //inline funcs
	inline player_t *get_player_by_uid(uint32_t uid) {
        id_to_player_map_iter_t iter = uid_to_player_map_.find(uid);
        if (iter == uid_to_player_map_.end()) { return 0; }
        return iter->second;
    }

private:
	id_to_player_map_t uid_to_player_map_;
    id_to_player_map_t rpvp_index_to_player_map_;
    svrid_dupid_players_map_t ppve_players_map_;
    uint32_t index;
};

typedef singleton_default<player_manager_t> player_manager_singleton_t;
#define PLAYER_MGR (player_manager_singleton_t::instance())

int send_err_to_fdsess(fdsession_t *fdsess, uint32_t cmd, int err, uint32_t seq, uint32_t uid);

#endif // __PLAYER_MANAGER_H__
