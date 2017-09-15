#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libtaomee++/inet/byteswap.hpp>
#include <libtaomee++/utils/strings.hpp>
extern "C" {
//#include <libtaomee/log.h>
#include <libtaomee/project/types.h>
}

#include "proto_processor.h"
#include "proto/common/svr_proto_header.h"
#include "proto/db/db_errno.h"
#include "server.h"
//#include "mytimer.h"

ProtoProcessor::ProtoProcessor()
{

}

ProtoProcessor::~ProtoProcessor()
{
    //std::map<uint32_t, CmdProcessorInterface*>::iterator it;

    //it = m_cmd_processors.begin();

    //while (it != m_cmd_processors.end()) {
    
        //CmdProcessorInterface* processor = it->second;
        //delete processor;
    //}
}

int ProtoProcessor::register_cmd(
        uint32_t cmd,
        CmdProcessorInterface* processor)
{
    m_cmd_processors[cmd] = processor; 

    return 0;
}

int ProtoProcessor::get_pkg_len(
       const void* avail_data, 
       int avail_len)
{
    if (avail_len < (int)sizeof(svr_proto_header_t)) {
        return 0; // continue to recv        
    } else {
        svr_proto_header_t* header = (svr_proto_header_t *)avail_data;

        if (header->len < (int)sizeof(svr_proto_header_t)) {
            ERROR_LOG("too small pkg %u from client", header->len); 
            return -1;
        }

        if (header->len > 65536) {
            ERROR_LOG("too large pkg %u from client", header->len); 
            return -1;
        }

        return header->len;
    }
}

int ProtoProcessor::process(
        const char* recv_buf,
        int recv_len,
        char** send_buf,
        int* send_len)
{
    svr_proto_header_t* req_header = (svr_proto_header_t *)recv_buf;

    if (recv_len != (int)req_header->len) {
        //ERROR_LOG("recvlen %d not equal to pkglen %u",
                //recv_len, req_header->len); 
        return -1;
    }

    DEBUG_TLOG("req: len = %u, seq = %u, cmd = %04x, uid = %u, u_create_tm= %u",
            req_header->len, req_header->seq, req_header->cmd, req_header->uid, req_header->u_create_tm);

    const char* req_body = (char *)recv_buf + sizeof(*req_header);
    int req_bodylen = req_header->len - sizeof(*req_header);

    *send_buf = m_send_buf;

    svr_proto_header_t* ack_header = (svr_proto_header_t *)m_send_buf;

    ack_header->len = sizeof(*ack_header);
    ack_header->seq = req_header->seq;
    ack_header->cmd = req_header->cmd;
    ack_header->ret = 0;
    ack_header->uid = req_header->uid;
    ack_header->u_create_tm = req_header->u_create_tm;
    ack_header->cb_uid = req_header->cb_uid;
    ack_header->cb_u_create_tm = req_header->cb_u_create_tm;
    uint32_t u_create_tm = req_header->u_create_tm;

    std::map<uint32_t, CmdProcessorInterface *>::iterator it;

    it = m_cmd_processors.find(req_header->cmd);

    if (it == m_cmd_processors.end()) {
        ack_header->ret = db_err_cmd_not_find;
        *send_len = ack_header->len;
        return 0;
    }

    CmdProcessorInterface* processor = it->second;

    m_req_body.clear();
    m_ack_body.clear();

    m_req_body.assign(req_body, req_bodylen);

    uint32_t ret = 0;
    userid_t userid = req_header->uid;
    ret = processor->process(userid, u_create_tm, m_req_body, m_ack_body);

    ack_header->ret = ret;

    if (ret == 0) {

        ack_header->len = sizeof(*ack_header) + m_ack_body.size();

        if (ack_header->len > sizeof(m_send_buf)) {
            return -1;
        }

        memcpy(m_send_buf + sizeof(*ack_header),
                m_ack_body.c_str(), m_ack_body.size());

        mysql_commit(&g_db->handle);
    } else {
        mysql_rollback(&g_db->handle);

        ERROR_LOG("proc cmd 0x%04x uid %u err %u, rollback", 
                req_header->cmd, req_header->uid,
                ack_header->ret);
    }

    DEBUG_TLOG("ack: len = %u, seq = %u, cmd = %04x, uid = %u, u_create_tm= %u",
            ack_header->len, ack_header->seq, ack_header->cmd, ack_header->uid, ack_header->u_create_tm);

    *send_len = ack_header->len;

    return 0;
}

