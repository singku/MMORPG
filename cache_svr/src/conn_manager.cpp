#include "conn_manager.h"
#include "proto.h"

static char send_buf[65536 * 32];

#define CONN_DETECT \
    do { \
        if (!conn->fdsess) { \
            ERROR_TLOG("SEND_TO_CONN[op_u:%u cmd:%u hex_cmd:0x%04X seq(wait_u):%u] BUT CONN CLOSED", \
                    conn->op_uid, conn->wait_cmd, conn->wait_cmd, conn->cur_seq); \
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
    header->uid = conn->op_uid;
    header->u_create_tm = conn->op_u_create_tm;
    header->cb_uid = conn->wait_uid;
    header->cb_u_create_tm = conn->wait_u_create_tm;

TRACE_TLOG("Send ERR TO Conn[cmd:%u, hex_cmd:0x%04X, op_uid:%u, seq(wait_uid):%u, ret:%d]", 
            conn->wait_cmd, conn->wait_cmd, conn->op_uid, conn->cur_seq, ret);

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
    header->uid = conn->op_uid;
    header->u_create_tm = conn->op_u_create_tm;
    header->cb_uid = conn->wait_uid;
    header->cb_u_create_tm = conn->wait_u_create_tm;

TRACE_TLOG("Send Msg To Conn: [cmd:%d, hex_cmd:0x%04X, op_uid:%u seq(wait_uid):%u]\nmsg:%s\n[%s]",
            conn->wait_cmd, conn->wait_cmd, conn->op_uid, conn->cur_seq, 
            msg.GetTypeName().c_str(), 
            msg.Utf8DebugString().c_str());

    send_pkg_to_client(conn->fdsess, send_buf, header->len);
    CONN_MGR.delete_conn(&conn);
    return 0;
}

int send_err_to_fdsess(const fdsession_t *fdsess, uint32_t uid, uint32_t u_create_tm, 
        uint32_t cb_uid, uint32_t cb_u_create_tm, uint32_t cmd, uint32_t seq, uint32_t ret)
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
