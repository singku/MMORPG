#include "cache_processor.h"
#include "conn_manager.h"
#include "service.h"
#include "hiredis_manager.h"

int CliGetPlayerCacheInfoCmdProcessor::proc_pkg_from_client(
        conn_info_t *conn, const char* body, int bodylen)
{
    in_.Clear();
    if (parse_message(body, bodylen, &in_)) {
        send_err_to_conn(conn, cache_err_msg_format_err); 
        return 0;
    }

    uint32_t op_uid = (in_.roles_size() == 0) ? conn->op_uid :in_.roles(0).userid();
    uint32_t op_u_create_tm = (in_.roles_size() == 0) ? conn->op_u_create_tm :in_.roles(0).u_create_tm();

    conn->op_uid = op_uid;
    conn->op_u_create_tm = op_u_create_tm;

    cacheproto::sc_batch_get_users_info ret_msg;
    ret_msg.Clear();
    //先调用redis_mgr 获取数据
    bool ret = HIREDIS_MGR.get_player_info(op_uid, op_u_create_tm, ret_msg.add_user_infos());
    if (ret == true) { //得到数据 则直接返回
        send_msg_to_conn(conn, ret_msg);
        return 0;
    }
    
    //如果获取失败 则向db发起请求 同时将自己加入等待列表
    if (CONN_MGR.is_uid_cmd_in_waiting(op_uid, op_u_create_tm, conn->wait_cmd)) {
        CONN_MGR.add_to_wait_map(*conn);
        return 0; //已经在等在中了 只要把自己加入等待列表即可
    }

    //没有等待列表 把自己加入等待列表 发出请求
    CONN_MGR.add_to_wait_map(*conn);
    dbproto::cs_get_base_info db_msg;
    return g_dbproxy->send_msg(conn, op_uid, op_u_create_tm, db_cmd_get_cache_info, conn->wait_cmd, db_msg);
}

int DbGetPlayerCacheInfoCmdProcessor::proc_pkg_from_serv(
        conn_info_t *conn, const char* body, int bodylen)
{
    out_.Clear();
    if (parse_message(body, bodylen, &out_)) {
        return cache_err_sys_err; 
    }
    
    const commonproto::battle_player_data_t &src = out_.cache_info();
    cacheproto::sc_batch_get_users_info ret_msg;
    commonproto::battle_player_data_t *dst = ret_msg.add_user_infos();
    dst->CopyFrom(src);

    string str;
    str.clear();
    dst->SerializeToString(&str);
    HIREDIS_MGR.set_player_info(conn->op_uid, conn->op_u_create_tm, str);
 
    std::set<conn_info_t *> w_set;
    CONN_MGR.del_from_wait_map(conn->op_uid, conn->op_u_create_tm, conn->wait_cmd, w_set);
    FOREACH(w_set, it) {
        conn_info_t *w_conn = *it;
        send_msg_to_conn(w_conn, ret_msg);
    }
    return 0;
}

int SetCacheCmdProcessor::proc_pkg_from_client(
        conn_info_t *conn, const char *body, int bodylen)
{
    in_.Clear();
    if (parse_message(body, bodylen, &in_)) {
        send_err_to_conn(conn, cache_err_msg_format_err); 
        return 0;
    }
    HIREDIS_MGR.set_cache(in_.key(), in_.value(), in_.ttl());
    return send_msg_to_conn(conn, out_);
}

int GetCacheCmdProcessor::proc_pkg_from_client(
        conn_info_t *conn, const char *body, int bodylen)
{
    in_.Clear();
    if (parse_message(body, bodylen, &in_)) {
        send_err_to_conn(conn, cache_err_msg_format_err); 
        return 0;
    }
    string value;
    out_.Clear();
    for (int i = 0; i < in_.keys_size(); i++) {
        cacheproto::cache_key_value_t *cache = out_.add_key_values();
        bool rt = HIREDIS_MGR.get_cache(in_.keys(i), value);
        if (rt == false) {
            value.assign("<<Err:key not found>>");
        }
        cache->set_key(in_.keys(i));
        cache->set_value(value);
    }

    return send_msg_to_conn(conn, out_);
}

int SetUserInfoOutDateCmdProcessor::proc_pkg_from_client(
        conn_info_t *conn, const char *body, int bodylen)
{
    HIREDIS_MGR.del_player_info(conn->op_uid, conn->op_u_create_tm);

    return send_msg_to_conn(conn, out_);
}
