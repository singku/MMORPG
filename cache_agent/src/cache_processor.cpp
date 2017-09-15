#include "cache_processor.h"
#include "conn_manager.h"
#include "service.h"
#include "timer_procs.h"

int GetPlayerBaseInfoCmdProcessor::proc_pkg_from_client(
        conn_info_t *conn, const char* body, int bodylen)
{
    in_.Clear();
    if (parse_message(body, bodylen, &in_)) {
        return send_err_to_conn(conn, cache_err_msg_format_err); 
    }

    if (in_.roles_size() == 0) {
        cacheproto::sc_batch_get_users_info ol_out;
        ol_out.Clear();
        return send_msg_to_conn(conn, ol_out);
    }

    conn->ok_info_vec.clear();
    conn->err_info_vec.clear();

    cacheproto::cs_batch_get_users_info cache_msg;
    conn->wait_uid_set.clear();
    for (int i = 0; i < in_.roles_size();  i++) {
        if (in_.roles(i).userid() == 0) continue;
        uint64_t key = comp_u64(in_.roles(i).userid(), in_.roles(i).u_create_tm());
        conn->wait_uid_set.insert(key); //添加一个等待的uid
        //发出每一个请求
        cache_msg.Clear();
        commonproto::role_info_t *inf = cache_msg.add_roles();
        inf->set_userid(in_.roles(i).userid());
        inf->set_u_create_tm(in_.roles(i).u_create_tm());
        g_dbproxy->send_msg(conn, in_.roles(i).userid(), in_.roles(i).u_create_tm(),
                cache_cmd_ag_req_users_info, conn->uid, cache_msg);
    }

    //万一反过来的全是0
    if (conn->wait_uid_set.empty()) {
        return finish_conn_msg(conn);
    }

    //将conn加入等待队列
    CONN_MGR.add_to_wait_map(*conn);
    //超时设置
    ADD_TIMER_EVENT_EX(&g_waiting_rsp_timer,
            kTimerTypeWaitingRsp,
            (void*)(comp_u64(conn->uid, conn->u_create_tm)), 
            NOW() + kTimerIntervalWaitingRsp);

    return 0;
}

int GetPlayerBaseCacheCmdProcessor::proc_pkg_from_serv(
        conn_info_t *conn, const char* body, int bodylen, svr_proto_header_t *header)
{
    if (header->ret) { //返回错误码
        cacheproto::uid_errcode_t errinf;
        errinf.set_errcode(header->ret);
        errinf.mutable_role()->set_userid(header->uid);
        errinf.mutable_role()->set_u_create_tm(header->u_create_tm);
        conn->err_info_vec.push_back(errinf);
        conn->wait_uid_set.erase(comp_u64(header->uid, header->u_create_tm));

        if (conn->wait_uid_set.empty()) {
            return finish_conn_msg(conn);
        }
        return 0;
    }

    svr_out_.Clear();
    if (parse_message(body, bodylen, &svr_out_) 
        || svr_out_.user_infos_size() == 0) { //解析出错
        cacheproto::uid_errcode_t errinf;
        errinf.set_errcode(cache_err_sys_err);
        errinf.mutable_role()->set_userid(header->uid);
        errinf.mutable_role()->set_u_create_tm(header->u_create_tm);
        conn->err_info_vec.push_back(errinf);
        conn->wait_uid_set.erase(comp_u64(header->uid, header->u_create_tm));
        if (conn->wait_uid_set.empty()) {
            return finish_conn_msg(conn);
        }
        return 0;
    }
    
    commonproto::battle_player_data_t info;
    info.CopyFrom(svr_out_.user_infos(0));
    conn->ok_info_vec.push_back(info);
    conn->wait_uid_set.erase(comp_u64(info.base_info().user_id(), info.base_info().create_tm()));
    if (conn->wait_uid_set.empty()) {
        return finish_conn_msg(conn);
    }

    return 0;
}
