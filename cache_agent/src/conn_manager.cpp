#include "conn_manager.h"
#include "proto.h"

static char send_buf[65536 * 32];

#define CONN_DETECT \
    do { \
        if (!conn->fdsess) { \
            ERROR_TLOG("SEND_TO_CONN[u:%u cmd:%u hex_cmd:0x%04X seq:%u] BUT CONN CLOSED", \
                    conn->uid, conn->wait_cmd, conn->wait_cmd, conn->cur_seq); \
            return -1; \
        } \
    } while (0)

int send_err_to_conn(conn_info_t *conn, uint32_t ret)
{
    assert(conn);
    CONN_DETECT;

    svr_proto_header_t* header = (svr_proto_header_t *)send_buf;

    header->len = sizeof(*header);
    header->seq = conn->cur_seq;
    header->cmd = conn->wait_cmd;
    header->ret = ret;
    header->uid = conn->uid;
    header->u_create_tm = conn->u_create_tm;
    header->cb_uid = conn->cb_uid;
    header->cb_u_create_tm = conn->cb_u_create_tm;


TRACE_TLOG("Send ERR TO Conn[cmd:%u, hex_cmd:0x%04X, uid:%u, seq:%u, ret:%d]", 
            conn->wait_cmd, conn->wait_cmd, conn->uid, conn->cur_seq, ret);

    send_pkg_to_client(conn->fdsess, send_buf, header->len);
    CONN_MGR.delete_conn(&conn);
    return 0;
}

int send_msg_to_conn(conn_info_t *conn, const google::protobuf::Message& msg)
{
    assert(conn);
    CONN_DETECT;

    svr_proto_header_t *header = (svr_proto_header_t *)(send_buf);

    if (sizeof(send_buf) < sizeof(*header) + msg.ByteSize()) {
        ERROR_TLOG("send buf size %lu not enough proto %lu",
                sizeof(send_buf), sizeof(*header) + msg.ByteSize());
        return -1;
    }

    msg.SerializeToArray(send_buf + sizeof(*header), sizeof(send_buf) - sizeof(*header));

    header->len = sizeof(*header) + msg.ByteSize();
    header->seq = conn->cur_seq;
    header->cmd = conn->wait_cmd;
    header->ret = 0;
    header->uid = conn->uid;
    header->u_create_tm = conn->u_create_tm;
    header->cb_uid = conn->cb_uid;
    header->cb_u_create_tm = conn->cb_u_create_tm;

TRACE_TLOG("Send Msg To Conn: [cmd:%d, hex_cmd:0x%04X, uid:%u seq:%u]\nmsg:%s\n[%s]",
            conn->wait_cmd, conn->wait_cmd, conn->uid, conn->cur_seq, 
            msg.GetTypeName().c_str(), 
            msg.Utf8DebugString().c_str());

    send_pkg_to_client(conn->fdsess, send_buf, header->len);
    CONN_MGR.delete_conn(&conn);
    return 0;
}

int send_err_to_fdsess(const fdsession_t *fdsess, uint32_t uid, uint32_t u_create_tm, 
        uint32_t cb_uid, uint32_t cb_u_create_tm,
        uint32_t cmd, uint32_t seq, uint32_t ret)
{
    svr_proto_header_t* header = (svr_proto_header_t *)send_buf;
    header->len = sizeof(*header);
    header->seq = seq;
    header->cmd = cmd;
    header->ret = ret;
    header->uid = uid;
    header->u_create_tm = u_create_tm;
    header->cb_uid = cb_uid;
    header->cb_u_create_tm = cb_u_create_tm;

TRACE_TLOG("Send Err To Fd: [fd:%d, u:%u, cmd:%u, hex_cmd:0x%04X err:%d]",
            fdsess->fd, uid, cmd, cmd, ret);

    return send_pkg_to_client(fdsess, send_buf, header->len);
}

int finish_conn_msg(conn_info_t *conn)
{
    assert(conn);
    assert(conn->fdsess);

    cacheproto::sc_batch_get_users_info out;
    out.Clear();

    //从等待列表中删除
    CONN_MGR.del_from_wait_map(conn->uid, conn->u_create_tm);

    FOREACH(conn->ok_info_vec, it) {
        commonproto::battle_player_data_t &inf = *it;
        commonproto::battle_player_data_t *dst = out.add_user_infos();
        dst->CopyFrom(inf);
    }
    FOREACH(conn->err_info_vec, it) {
        cacheproto::uid_errcode_t &inf = *it;
        cacheproto::uid_errcode_t *dst = out.add_errs();
        dst->CopyFrom(inf);
    }
    FOREACH(conn->wait_uid_set, it) {
        cacheproto::uid_errcode_t *errinf = out.add_errs();
        errinf->set_errcode(cache_err_req_time_up);
        uint32_t high, low;
        decomp_u64(*it, high, low);
        errinf->mutable_role()->set_userid(high);
        errinf->mutable_role()->set_u_create_tm(low);
    }
    conn->ok_info_vec.clear();
    conn->err_info_vec.clear();
    return send_msg_to_conn(conn, out);
}
