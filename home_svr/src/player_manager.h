#ifndef __PLAYER_MANAGER_H__
#define __PLAYER_MANAGER_H__

#include "common.h"
#include "player.h"

struct player_t;

class PlayerManager
{
public:
    PlayerManager() { 
        timer_map.clear();
    }
    ~PlayerManager() { }

    //根据角色信息获得房主的player
    player_t *get_player_by_role(role_info_t role);

    /**
     * @说明 玩家在线 但不一定在小屋服务器有小屋数据 只有当他人
     * 访问玩家的小屋信息时 才会建立起玩家的小屋数据 在小屋服务器拥有
     * 小屋数据的玩家 保存在player_map里。online_player_ids保存哪些玩家在线
     * 以便决定是否发送通知包到这些玩家.当玩家的小屋数据被其他人访问的时候
     * 小屋服务器会去DB拉取数据过来 并建立这个数据结构 该数据结构在一定的规则内
     * 有效(比如最后一个人离开后1分钟内都没有人再进来)  当他人对玩家的小屋数据做出
     * 操作的时候 比如留言 这时会根据 online_player_ids的数据判定是否需要通知到
     * 对应的玩家 所以online上的玩家上线和下线都要报告给小屋服务器  同时 当小屋服务器
     * 发现玩家的小屋信息被访问同时玩家在线时 会通知到online online的玩家在修改online的
     * 小屋数据时 也会一并同步到小屋.
     */
    //创建具有小屋数据的玩家(如果最后一个参数不给 则暂时不放置小屋数据)
    player_t *create_new_player(role_info_t role, fdsession_t *fdsess, uint32_t cur_seq, uint32_t home_type = 0);
    //删除具有小屋数据的玩家
    void delete_player(role_info_t role);
    //新增一个在线的玩家
    //void add_online_player(role_info_t role, int fd);
    //删除一个在线的玩家
    //void delete_online_player(role_info_t role);
    //删除一批同一个fd的玩家
    //void delete_online_players_with_fd(int fd);

public:
    /*
    inline bool is_player_online(role_info_t role) {
        return (online_player_map_.count(ROLE_KEY(role)) > 0);
    }
    */
    inline bool home_info_is_waiting(role_info_t role) {
        return (waiting_home_map_.count(ROLE_KEY(role)) > 0);
    }

    inline void add_to_home_waiting_list(role_info_t wait, role_info_t host) {
        std::map<uint64_t, std::set<uint64_t> >::iterator it;
        it = waiting_home_map_.find(ROLE_KEY(host));
        if (it == waiting_home_map_.end()) {
            std::set<uint64_t> w_set;
            w_set.clear();
            w_set.insert(ROLE_KEY(wait));
            waiting_home_map_[ROLE_KEY(host)] = w_set;
        } else {
            std::set<uint64_t> &w_set = it->second;
            w_set.insert(ROLE_KEY(wait));
        }
    }

    inline void delete_from_home_waiting_list(role_info_t host, std::set<uint64_t> &result) {
        result.clear();
        std::map<uint64_t, std::set<uint64_t> >::iterator it;
        it = waiting_home_map_.find(ROLE_KEY(host));
        if (it == waiting_home_map_.end()) return;
        std::set<uint64_t> &w_set = it->second;
        result = w_set;
        waiting_home_map_.erase(ROLE_KEY(host));
    }
    
    inline void player_quit_wait(role_info_t host, role_info_t wait) {
        std::map<uint64_t, std::set<uint64_t> >::iterator it;
        it = waiting_home_map_.find(ROLE_KEY(host));
        if (it == waiting_home_map_.end()) return;
        std::set<uint64_t> &wait_set = it->second;
        wait_set.erase(ROLE_KEY(host));
    }

    //如果玩家没有访问别人 且没有访客 则销毁
    inline void try_release_player(player_t *p) {
        if (!p->at_whos_home().userid && !p->has_visitor()) {
            if (p->wait_host().userid) {
                player_quit_wait(p->wait_host(), p->role());
            }
            delete_player(p->role());
        }
    }

    //当玩家断线后 且没有访客的时候 销毁
    inline void try_release_player_when_offline(player_t *p) {
        if (!p->has_visitor()) {
            if (p->wait_host().userid) {
                player_quit_wait(p->wait_host(), p->role());
            }
            delete_player(p->role());
        }
    }

public:
    //<host, timer_struct_t>
    std::map<uint64_t, timer_struct_t*> timer_map; //所有的等待定时器地址
private:
    std::map<uint64_t, player_t *> player_map_; //玩家信息表
    //<uid, fd>
    //std::map<uint64_t, int> online_player_map_; //在线的玩家米米号集合
    //<uid, set>等待玩家uid房间信息的玩家集合
    std::map<uint64_t, std::set<uint64_t> > waiting_home_map_;
};

extern PlayerManager PLAYER_MGR;

#endif
