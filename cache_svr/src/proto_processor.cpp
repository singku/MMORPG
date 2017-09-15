#include "proto_processor.h"
#include "proto.h"
#include "conn_manager.h"

ProtoProcessor* g_proto_processor;

ProtoProcessor::ProtoProcessor()
{
}

ProtoProcessor::~ProtoProcessor()
{
}

int ProtoProcessor::register_command(
        uint32_t cmd,
        CmdProcessorInterface* processor)
{
    cmd_processors_[cmd] = processor; 

    return 0;
}

int ProtoProcessor::get_pkg_len(int fd, const void* avail_data, 
       int avail_len, int from)
{
    if (from == PROTO_FROM_CLIENT) {
        if (avail_len < (int)sizeof(svr_proto_header_t)) {
            return 0; // continue to recv        
        }
        svr_proto_header_t* header = (svr_proto_header_t *)avail_data;
        if (header->len < (int)sizeof(svr_proto_header_t)) {
            ERROR_TLOG("too small pkg %u from client", header->len); 
            return -1;
        }
        if (header->len > 128*1024) {
            ERROR_TLOG("too large pkg %u from client", header->len); 
            return -1;
        }
        return header->len;

    } else if (from == PROTO_FROM_SERV) {
        if (avail_len < (int)sizeof(svr_proto_header_t)) {
            return 0; // continue to recv 
        } else {
            svr_proto_header_t* header = (svr_proto_header_t *)avail_data;
            if (header->len < (int)sizeof(svr_proto_header_t)) {
                ERROR_TLOG("too small pkg %u from server", header->len); 
                return -1;
            }
            if (header->len > 128*1024) {
                ERROR_TLOG("too large pkg %u from server", header->len); 
                return -1;
            }
            return header->len;
        }
    } else {
        ERROR_TLOG("get pkg len invalid from %d", from); 
        return -1;
    }
}

int ProtoProcessor::proc_pkg_from_client(void* data, int len,
        fdsession_t* fdsess)
{
    svr_proto_header_t* header = static_cast<svr_proto_header_t *>(data);
    const char* body = static_cast<char *>(data) + sizeof(svr_proto_header_t);
    int bodylen = header->len - sizeof(svr_proto_header_t);

TRACE_TLOG("GET PKG FROM CLIENT [op_u:%u cmd:%u hex_cmd:0x%04X seq(wait_u):%u fd:%d]",
            header->uid, header->cmd, header->cmd, header->seq, fdsess->fd);

    conn_info_t *conn = CONN_MGR.create_new_conn(fdsess, header->cb_uid, header->cb_u_create_tm,
            header->cmd, header->seq, header->uid, header->u_create_tm);
    if (!conn) {
        ERROR_TLOG("Cache svr: create conn failed! memory maybe not enough");
        return send_err_to_fdsess(fdsess, header->uid, header->u_create_tm,
                header->cb_uid, header->cb_u_create_tm,
                header->cmd, header->seq, cache_err_sys_err);
    }

    std::map<uint32_t, CmdProcessorInterface *>::iterator it;
    it = cmd_processors_.find(header->cmd);
    if (it == cmd_processors_.end()) {
        return send_err_to_conn(conn, cache_err_cmd_not_found);
    }
    CmdProcessorInterface *processor = it->second;
    int ret = processor->proc_pkg_from_client(conn, body, bodylen);
    if (ret != 0) {
        ERROR_TLOG("PROC PKG FROM CONN ERR: %d [op_u:%u seq(wait_u):%u cmd:%u hex_cmd:0x%04X]", 
                ret, conn->op_uid, conn->wait_uid, conn->wait_cmd, conn->wait_cmd); 
        return send_err_to_conn(conn, ret);
    }
    return 0;
}

void ProtoProcessor::proc_pkg_from_serv(int fd, void* data, int len)
{
    svr_proto_header_t* header = static_cast<svr_proto_header_t *>(data);
    const char* body = (char *)data + sizeof(*header);
    int bodylen = len - sizeof(*header);

TRACE_TLOG("GET PKG FROM SERVER [u:%u cmd:%u hex_cmd:0x%04X seq:%u ret:%d fd:%d]",
            header->uid, header->cmd, header->cmd, header->seq, header->ret, fd);

    conn_info_t conn;
    conn.op_uid = header->uid;
    conn.op_u_create_tm = header->u_create_tm;
    //向db发数据出去的时候 是以wait_cmd作为seq发的
    conn.wait_cmd = header->seq;
    conn.wait_uid = header->cb_uid;
    conn.wait_u_create_tm = header->cb_u_create_tm;

    uint32_t wait_cmd = header->seq;

    std::map<uint32_t, CmdProcessorInterface *>::iterator it;
    it = cmd_processors_.find(header->cmd);
    if (it == cmd_processors_.end()) {
        ERROR_TLOG("Proc Svr Ret Pkg: cmd not find[op_u:%u cmd:%u hex_cmd:%u seq:%u ret:%d]", 
                header->uid, header->cmd, header->cmd, header->seq, header->ret);  
        return;
    }

    CmdProcessorInterface* processor = it->second;
    int ret = header->ret;
    //包头有错误码
    if (ret) {
        std::set<conn_info_t *> w_set;
        CONN_MGR.del_from_wait_map(header->uid, header->u_create_tm, wait_cmd, w_set);
        FOREACH(w_set, it) {
            conn_info_t *w_conn = *it;
            send_err_to_conn(w_conn, cache_err_user_not_found);
        }
        return;
    }
    //返回错误码 照样处理
    ret = processor->proc_pkg_from_serv(&conn, body, bodylen);
    if (ret) {
        std::set<conn_info_t *> w_set;
        CONN_MGR.del_from_wait_map(header->uid, header->u_create_tm, wait_cmd, w_set);
        FOREACH(w_set, it) {
            conn_info_t *w_conn = *it;
            send_err_to_conn(w_conn, ret);
        }
    }
    return;
}
