#ifndef __CONN_MANAGER_H__
#define __CONN_MANAGER_H__

#include "common.h"

struct conn_info_t {
    conn_info_t() {
        uid = 0;
        u_create_tm = 0;
        wait_cmd = 0;
        cur_seq = 0;
        fdsess = 0;
        wait_uid_set.clear();
        cb_uid = 0;
        cb_u_create_tm = 0;
        ok_info_vec.clear();
        err_info_vec.clear();
    }
    //实际发起请求的uid 
    uint32_t uid; 
    uint32_t u_create_tm;
    //发起请求的命令
    uint16_t wait_cmd;
    //当前处理的seq == wait_uid
    uint32_t cur_seq;
    //该连接对应的fdsession
    fdsession_t *fdsess;
    uint32_t cb_uid;
    uint32_t cb_u_create_tm;

    std::set<uint64_t> wait_uid_set;
    
    //回包信息
    std::vector<commonproto::battle_player_data_t> ok_info_vec;
    std::vector<cacheproto::uid_errcode_t> err_info_vec;
};

class conn_manager_t {
public:
    conn_manager_t() { 
        wait_uid_map_.clear();
    }
    ~conn_manager_t() {
        FOREACH(wait_uid_map_, it) {
            conn_info_t *conn = it->second;
            delete_conn(&conn);
        }
        wait_uid_map_.clear();
    }

public:
    typedef std::map<uint64_t, conn_info_t*> wait_map_t;
    typedef std::map<uint64_t, conn_info_t*>::iterator wait_map_iter_t;

    inline conn_info_t *create_new_conn(fdsession_t *fdsess, 
            uint32_t uid, uint32_t u_create_tm, 
            uint32_t cb_uid, uint32_t cb_u_create_tm,
            uint32_t cmd, uint32_t seq) {
        conn_info_t *conn = new conn_info_t();
        if (!conn) {
            return 0;
        }
        conn->fdsess = fdsess;
        conn->uid = uid;
        conn->u_create_tm = u_create_tm;
        conn->cb_uid = cb_uid;
        conn->cb_u_create_tm = cb_u_create_tm;
        conn->wait_cmd = cmd;
        conn->cur_seq = seq;
        return conn;
    }

    inline void delete_conn(conn_info_t **conn) {
        delete *conn;
        *conn = 0;
    }

    //每当一个连接发出一批请求的时候 这个连接被加入到等待列表
    //每当agent收到回包 会清理这个等待列表 如果等待列表里的连接
    //超时 会自动被拿出 然后给连接回超时包
    inline void add_to_wait_map(conn_info_t &conn) {
        uint64_t key = comp_u64(conn.uid, conn.u_create_tm);
        wait_uid_map_.insert(make_pair(key, &conn));
    }

    //从等待列表中删除
    inline conn_info_t *del_from_wait_map(uint32_t uid, uint32_t u_create_tm) {
        uint64_t key = comp_u64(uid, u_create_tm);
        wait_map_iter_t it = wait_uid_map_.find(key);
        if (it == wait_uid_map_.end()) return NULL;
        conn_info_t *conn = it->second;
        wait_uid_map_.erase(it);
        return conn;
    }

    //当fd断开后 连接不可用 则需删除wait_map中的conn
    inline void del_conn_when_fd_closed(int fd) {
        FOREACH(wait_uid_map_, it) {
            conn_info_t *conn = it->second;
            if (conn->fdsess && conn->fdsess->fd != fd) continue;
            delete_conn(&conn);
            wait_uid_map_.erase(it);
        }
    }

    //conn是否在等待
    inline bool is_uid_waiting(uint32_t uid, uint32_t u_create_tm) {
        uint64_t key = comp_u64(uid, u_create_tm);
        return (wait_uid_map_.count(key) > 0);
    }

    inline conn_info_t *get_conn_by_uid(uint32_t uid, uint32_t u_create_tm) {
        uint64_t key = comp_u64(uid, u_create_tm);
        wait_map_iter_t it = wait_uid_map_.find(key);
        if (it == wait_uid_map_.end()) return 0;
        return it->second; 
    }

private:
    //<op_uid, conn>  //等待op_uid信息的所有连接
    //当redis没有命中的时候 cache_svr异步去db请求数据
    //此时可能会有多个请求op_uid的请求过来 将这些请求
    //全部缓存起来 等待db回包后一次性返回
    wait_map_t wait_uid_map_;
};

typedef singleton_default<conn_manager_t> conn_manager_singleton_t;
#define CONN_MGR (conn_manager_singleton_t::instance())

int send_err_to_conn(conn_info_t *conn, uint32_t ret);
int send_msg_to_conn(conn_info_t *conn, const google::protobuf::Message& msg);
int send_err_to_fdsess(const fdsession_t *fdsess, uint32_t uid, uint32_t u_create_tm,
        uint32_t cb_uid, uint32_t cb_u_create_tm,
        uint32_t cmd, uint32_t seq, uint32_t ret);

int finish_conn_msg(conn_info_t *conn);

#endif
