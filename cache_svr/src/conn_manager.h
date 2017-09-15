#ifndef __CONN_MANAGER_H__
#define __CONN_MANAGER_H__

#include "common.h"
#include "timer_procs.h"

struct conn_info_t {
    conn_info_t() {
        wait_uid = 0;
        wait_u_create_tm = 0;
        wait_cmd = 0;
        cur_seq = 0;
        op_uid = 0;
        op_u_create_tm = 0;
        fdsess = 0;
    }
    //实际发起请求的uid 包头中的seq
    uint32_t wait_uid; 
    uint32_t wait_u_create_tm;
    //发起请求的命令
    uint16_t wait_cmd;
    //当前处理的seq == wait_uid
    uint32_t cur_seq;
    //操作数据的uid
    uint32_t op_uid;
    //操作数据的op_u_create_tm;
    uint32_t op_u_create_tm;
    //该连接对应的fdsession
    fdsession_t *fdsess;
};

class conn_manager_t {
public:
    conn_manager_t() { 
        wait_uid_map_.clear();
        wait_set_.clear();
        fd_conn_map_.clear();
    }
    ~conn_manager_t() {
        FOREACH(wait_set_, it) {
            conn_info_t *conn = *it;
            wait_set_.erase(conn);
            delete_conn(&conn);
        }
        wait_set_.clear();
        wait_uid_map_.clear();
        fd_conn_map_.clear();
    }

public:
    //等待uid 的cmd的 conn们
    //<uid, <cmd, <conn>>
    typedef std::set<conn_info_t*> w_set_t;
    typedef std::set<conn_info_t*>::iterator w_set_iter_t;

    typedef std::map<uint32_t, w_set_t > cmd_map_t;
    typedef cmd_map_t::iterator cmd_map_iter_t;

    typedef std::map<uint32_t, w_set_t > fd_conn_map_t;
    typedef fd_conn_map_t::iterator fd_conn_map_iter_t;

    typedef std::map<uint64_t, cmd_map_t > wait_map_t;
    typedef wait_map_t::iterator wait_map_iter_t;

    inline conn_info_t *create_new_conn(fdsession_t *fdsess, uint32_t wait_uid, uint32_t wait_u_create_tm,
            uint32_t cmd, uint32_t seq, uint32_t op_uid, uint32_t op_u_create_tm) {
        conn_info_t *conn = new conn_info_t();
        if (!conn) {
            return 0;
        }
        conn->fdsess = fdsess;
        conn->wait_uid = wait_uid;
        conn->wait_u_create_tm = wait_u_create_tm;
        conn->wait_cmd = cmd;
        conn->cur_seq = seq;
        conn->op_uid = op_uid;
        conn->op_u_create_tm = op_u_create_tm;
        fd_conn_map_iter_t it = fd_conn_map_.find(fdsess->fd);
        if (it == fd_conn_map_.end()) {
            w_set_t conn_set;
            conn_set.clear();
            conn_set.insert(conn);
            fd_conn_map_[fdsess->fd] = conn_set;
        } else {
            w_set_t &conn_set = it->second;
            conn_set.insert(conn);
        }
        return conn;
    }

    inline void delete_conn(conn_info_t **conn) {
        fd_conn_map_iter_t it = fd_conn_map_.find((*conn)->fdsess->fd);
        if (it != fd_conn_map_.end()) {
            w_set_t &conn_set = it->second;
            conn_set.erase(*conn);
        }
        if (wait_set_.count(*conn) != 0) {
            wait_set_.erase(*conn);
        }
        uint64_t wait_key = comp_u64((*conn)->op_uid, (*conn)->op_u_create_tm);
        wait_map_iter_t it2 = wait_uid_map_.find(wait_key);
        if (it2 != wait_uid_map_.end()) {
            cmd_map_t &cmd_map = it2->second;
            cmd_map_iter_t it3 = cmd_map.find((*conn)->wait_cmd);
            if (it3 != cmd_map.end()) {
                w_set_t &wait_set = it3->second;
                wait_set.erase(*conn);
                if (wait_set.empty()) cmd_map.erase(it3);
                if (cmd_map.empty()) wait_uid_map_.erase(it2);
            }
        }
        delete *conn;
        *conn = 0;
    }

    //cache svr不能立即完成的命令(异步访问DB)
    //会加入到等待的map中去 当cache_svr收到db的回包后
    //会依次轮询 wait_uid_map如果有匹配的
    //等待命令 则将这个数据返回
    inline void add_to_wait_map(conn_info_t &conn) {
        uint64_t wait_key = comp_u64(conn.op_uid, conn.op_u_create_tm);
        wait_map_iter_t it = wait_uid_map_.find(wait_key); 
        if (it == wait_uid_map_.end()) { 
            cmd_map_t cmd_map;
            cmd_map.clear();
            w_set_t w_set; 
            w_set.clear(); 
            w_set.insert(&conn); 
            cmd_map[conn.wait_cmd] = w_set;
            wait_uid_map_[wait_key] = cmd_map; 
        } else { 
            cmd_map_t &cmd_map = it->second; 
            cmd_map_iter_t it2 = cmd_map.find(conn.wait_cmd);
            if (it2 == cmd_map.end()) {
                w_set_t w_set;
                w_set.clear();
                w_set.insert(&conn);
                cmd_map[conn.wait_cmd] = w_set;
            } else {
                w_set_t &w_set = it2->second;
                w_set.insert(&conn);
            }
        }

        //加入到等待集合
        wait_set_.insert(&conn);

        //添加超时定时器
        ADD_TIMER_EVENT_EX(&g_waiting_rsp_timer,
                kTimerTypeWaitingRsp,
                &conn,
                NOW() + kTimerIntervalWaitingRsp);
    }

    //从等待列表中删除等待uid和cmd的连接 并以set的形式返回这个等待的列表
    inline void del_from_wait_map(uint32_t uid, uint32_t u_create_tm, uint32_t cmd, 
            std::set<conn_info_t*> &result) {
        result.clear();
        uint64_t wait_key = comp_u64(uid, u_create_tm);
        wait_map_iter_t it = wait_uid_map_.find(wait_key);
        if (it == wait_uid_map_.end()) return;
        cmd_map_t &cmd_map = it->second;
        cmd_map_iter_t it2 = cmd_map.find(cmd);
        if (it2 == cmd_map.end()) return;

        result = it2->second;
        cmd_map.erase(it2);
        if (cmd_map.empty()) {
            wait_uid_map_.erase(it);
        }
        
        //从等待集合中删除
        FOREACH(result, it3) {
            wait_set_.erase(*it3);
        }
    }

    //判断某个uid cmd的信息是否在被等待中
    inline bool is_uid_cmd_in_waiting(uint32_t uid, uint32_t u_create_tm, uint32_t cmd) {
        uint64_t wait_key = comp_u64(uid, u_create_tm);
        wait_map_iter_t it1 = wait_uid_map_.find(wait_key);
        if (it1 == wait_uid_map_.end()) return false;

        cmd_map_t &cmd_map = it1->second;
        cmd_map_iter_t it2 = cmd_map.find(cmd);
        if (it2 == cmd_map.end()) return false;

        return true;
    }

    //判断conn是否在wait_map中
    inline bool is_conn_in_waiting(conn_info_t *conn) {
        return (wait_set_.count(conn) > 0);
    }

    //当fd断开后 连接不可用 则需删除wait_map中的conn
    inline void del_conn_when_fd_closed(int fd) {
        fd_conn_map_iter_t it = fd_conn_map_.find(fd);
        if (it == fd_conn_map_.end()) {
            return;
        }
        w_set_t &conn_set = it->second;
        FOREACH(conn_set, it) {
            conn_info_t *conn = *it;
            delete_conn(&conn);
        }
        /*
        FOREACH(wait_uid_map_, it) {
            cmd_map_t &cmd_map = it->second;
            FOREACH(cmd_map, it2) {
                w_set_t &w_set = it2->second;
                FOREACH(w_set, it3) {
                    conn_info_t *conn = *it3;
                    if (conn->fdsess && conn->fdsess->fd != fd) continue;
                    w_set.erase(it3);
                    wait_set_.erase(conn); //从辅助集合中删除
                    delete_conn(&conn);
                    if (!w_set.empty()) continue;
                    cmd_map.erase(it2); break;
                }
                if (!cmd_map.empty()) continue;
                wait_uid_map_.erase(it); break;
            }
        }
        */
    }

    //清理等待超时的conn
    inline void clean_expire_conn(conn_info_t *conn) {
        if (!is_conn_in_waiting(conn)) return;
        wait_set_.erase(conn); //从辅助集合中删除

        uint64_t wait_key = comp_u64(conn->op_uid, conn->op_u_create_tm);
        wait_map_iter_t it1 = wait_uid_map_.find(wait_key);
        if (it1 == wait_uid_map_.end()) return;

        cmd_map_t &cmd_map = it1->second;
        cmd_map_iter_t it2 = cmd_map.find(conn->wait_cmd);
        if (it2 == cmd_map.end()) return;

        w_set_t &w_set = it2->second;
        if (w_set.count(conn) == 0 ) return;

        w_set.erase(conn);

        if (!w_set.empty()) return;

        cmd_map.erase(it2);
        if (!cmd_map.empty()) return;

        wait_uid_map_.erase(it1);
    }

private:
    //<op_uid, conn>  //等待op_uid信息的所有连接
    //当redis没有命中的时候 cache_svr异步去db请求数据
    //此时可能会有多个请求op_uid的请求过来 将这些请求
    //全部缓存起来 等待db回包后一次性返回
    wait_map_t wait_uid_map_;
    w_set_t wait_set_;
    fd_conn_map_t fd_conn_map_;
};

typedef singleton_default<conn_manager_t> conn_manager_singleton_t;
#define CONN_MGR (conn_manager_singleton_t::instance())

int send_err_to_conn(conn_info_t *conn, uint32_t ret);
int send_msg_to_conn(conn_info_t *conn, const google::protobuf::Message& msg);
int send_err_to_fdsess(const fdsession_t *fdsess, uint32_t uid, uint32_t u_create_tm, 
        uint32_t cb_uid, uint32_t cb_u_create_tm,
        uint32_t cmd, uint32_t seq, uint32_t ret);

#endif
