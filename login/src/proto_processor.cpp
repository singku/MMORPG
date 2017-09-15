#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libtaomee++/inet/byteswap.hpp>
#include <libtaomee++/utils/strings.hpp>
extern "C" {
#include <libtaomee/log.h>
}
#include <async_serv/net_if.h>
#include "proto/client/header.pb.h"
#include "proto_processor.h"
#include "proto.h"
#include "login_cmd_processor.h"
#include "service.h"
#include "client.h"
#include "dll_iface.h"
#include "mencrypt.h"

std::map<int, client_info_t*> g_pending_proto_clients; 

ProtoProcessor::ProtoProcessor() {}
ProtoProcessor::~ProtoProcessor() {}

int ProtoProcessor::register_cmd(
        uint32_t cmd,
        CmdProcessorInterface* processor)
{
    m_cmd_processors[cmd] = processor; 
    return 0;
}

int ProtoProcessor::get_pkg_len(int fd, const void* avail_data, int avail_len, int from)
{
    if (from == PROTO_FROM_CLIENT) {
        static char request[]  = "<policy-file-request/>";
        static char response[] = "<?xml version=\"1.0\"?>"                             
            "<cross-domain-policy>"
            "<allow-access-from domain=\"*\" to-ports=\"*\" />"
            "</cross-domain-policy>";

        if ((avail_len == sizeof(request)) && !memcmp(avail_data, request, sizeof(request))) {
            net_send(fd, response, sizeof(response));
            return 0;
        }

        if (avail_len < (int)sizeof(cli_proto_header_t)) {
            return 0; // continue to recv        
        } else {
            cli_proto_header_t* header = (cli_proto_header_t *)avail_data;
            if (header->len < (int)sizeof(cli_proto_header_t)) {
                ERROR_TLOG("too small pkg %u from client", header->len); 
                return -1;
            }
            if (header->len > incoming_packet_max_size) {
                ERROR_TLOG("too large pkg %u from client", header->len); 
                return -1;
            }

            return header->len;
        }
    } else if (from == PROTO_FROM_SERV) {
        if (avail_len < (int)sizeof(comm_proto_header_t)) {
            return 0; // continue to recv 
        } else {
            comm_proto_header_t* header = (comm_proto_header_t *)avail_data;
            if (header->len < (int)sizeof(comm_proto_header_t)) {
                ERROR_TLOG("too small pkg %u from server", header->len); 
                return -1;
            }
            if (header->len > incoming_packet_max_size) {
                ERROR_TLOG("too large pkg cmd: %04x, len: %u, uid: %u from server", 
                        header->cmd, header->len, header->uid); 
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
        fdsession_t* fdsess, bool from_queue)
{
    cli_proto_header_t* header = (cli_proto_header_t *)data;
    uint32_t pb_head_len = header->head_len;
    uint32_t cmd = header->cmd;

    void *plain_data = 0;
    uint32_t plain_data_len = 0;
    if (cli_proto_encrypt) {//解包
        uint32_t uncrypt_len = sizeof(cli_proto_header_t) - sizeof(uint32_t);
        uint32_t crypt_len = len -  uncrypt_len;
        //char *pt = bin2hex(0, (char*)data+uncrypt_len, crypt_len);
        //DEBUG_TLOG("In Bef Decode:%u[%s]", crypt_len, pt);
        plain_data = msg_decrypt((char*)data + uncrypt_len, crypt_len, &plain_data_len);

        // 检查校验和
        if (!MCheckCode_x86_32((const u8 *)plain_data, plain_data_len, 0)) {
            ERROR_TLOG("check sum failed cmd:0x%x", cmd);
            return send_to_fdsess(fdsess, cmd, NULL, 0, 0, 0, cli_err_proto_format_err);
        }
        //pt = bin2hex(0, (char*)plain_data, plain_data_len);
        //DEBUG_TLOG("In Aft Decode:%u[%s]", plain_data_len, pt);
        plain_data = (char *)plain_data + sizeof(uint32_t);
        plain_data_len = plain_data_len - sizeof(uint32_t);

    } else {
        plain_data = (char*)data + sizeof(cli_proto_header_t);
        plain_data_len = len - sizeof(cli_proto_header_t);
    }

    onlineproto::proto_header_t pb_header;
    if (!pb_header.ParseFromArray((char*)plain_data, pb_head_len)) {
        std::string errstr = pb_header.InitializationErrorString();
        ERROR_TLOG("Parse ProtoHead failed err= '%s'", errstr.c_str());
        return send_to_fdsess(fdsess, cmd, NULL, 0, 0, pb_header.seque(), cli_err_proto_format_err);
    }

    const char* body = (char *)plain_data + pb_head_len;
    int bodylen = plain_data_len - pb_head_len;
    client_info_t *client = get_client(fdsess->fd);

    if (!client) {//client不存在 则为第一个包
        if (cmd != cli_cmd_cs_0x0001_login && cmd != cli_cmd_cs_0x0006_session_login) {
            //第一个包不是登陆请求包
            return send_err_to_fdsess(fdsess, cmd, cli_err_need_login_first);
        }       
    }
    if (!client) {
        client = get_or_alloc_client(fdsess->fd);
    }
    if (!client) {
        ERROR_TLOG("out of memory when alloc client"); 
        return -1;
    }
    client->fdsession = fdsess;
    if (client->wait_serv_cmd != 0 
        || (!from_queue && client->proto_queue && !client->proto_queue->empty())) {
        ProtoQueue* queue = client->proto_queue;
        if (queue->full()) {
            ERROR_TLOG("player[%u] is waiting server 0x%04x, discard this message %u", 
                    client->userid, client->wait_serv_cmd, cmd);
            return send_err_to_client(client, cli_err_sys_busy, pb_header.seque()); 
        }   
        queue->push_proto((const char*)data, len);
TRACE_TLOG("player %u is waiting server 0x%04x, add proto %u to queue, len = %u", 
        client->userid, client->wait_serv_cmd, cmd, queue->size());

        if (g_pending_proto_clients.find(client->fd) 
                == g_pending_proto_clients.end()) {
            g_pending_proto_clients[client->fd] = client;
        }   

        WARN_TLOG("cached_proto:%u %u", client->userid, cmd);
        return 0;   
    }

TRACE_LOG("GET PKG FROM CLIENT [cmd:%u hex_cmd:0x%04X len:%u uid:%u seq:%u]", 
            cmd, cmd, header->len, client->userid, pb_header.seque());  

    if (client->pkg_idx != 0) {
        //不是第一个包 需要检查seq
        if (pb_header.seque() != client->cli_seq + 1) {
            return send_to_client(client, cmd, NULL, 0, cli_err_seq_not_match, pb_header.seque());
        }
    }
    client->cli_seq = pb_header.seque();
    client->pkg_idx++;

    std::map<uint32_t, CmdProcessorInterface *>::iterator it;

    it = m_cmd_processors.find(cmd);
    if (it == m_cmd_processors.end()) {
        return send_to_client(client, cmd, NULL, 0, cli_err_cmd_not_found);
    }
    CmdProcessorInterface* processor = it->second;

    client->cli_cmd = cmd;
    client->wait_serv_cmd = 0;
    return processor->proc_pkg_from_client(client, body, bodylen);
}

void ProtoProcessor::proc_pkg_from_serv(int fd, void* data, int len)
{
    svr_proto_header_t* header = (svr_proto_header_t *)data;
    const char* body = (char *)data + sizeof(*header);
    int bodylen = len - sizeof(*header);
    if (header->cmd >= 0xA000) {//act cmd 包头比svr包头少
        body -= (sizeof(svr_proto_header_t) - sizeof(act_proto_header_t));
        bodylen += (sizeof(svr_proto_header_t) - sizeof(act_proto_header_t));
    }
    int client_fd = header->seq;
    client_info_t* client = get_client(client_fd);

    if (!client) {
        ERROR_TLOG("client fd %d already closed", client_fd);
        return;  
    }

    if (header->cmd != client->wait_serv_cmd) {
        ERROR_TLOG("client:%u recv unexpect pkg, cmd = 0x%04x, expect = 0x%04x, cli_cmd = 0x%04x",
                client->userid, header->cmd, client->wait_serv_cmd, client->cli_cmd);
        return;
    }
    client->wait_serv_cmd = 0;
    client->cur_serv_cmd = header->cmd;
    std::map<uint32_t, CmdProcessorInterface *>::iterator it;
    it = m_cmd_processors.find(client->cli_cmd);
    if (it == m_cmd_processors.end()) {
        ERROR_TLOG("cmd %u processor not find", client->cli_cmd);
	    return ;
    }

TRACE_LOG("GET PKG FROM SERVER [cli_cmd:%u cmd:0x%04X ret:%d len:%u uid:%u]", 
            client->cli_cmd, header->cmd, header->ret, header->len, client->userid);

    CmdProcessorInterface* processor = it->second;
    //删除定时器
    if (client->db_request_timer) {
        REMOVE_TIMER(client->db_request_timer);
        client->db_request_timer = NULL;
    }
    if (header->ret == 0) {
        processor->proc_pkg_from_serv(client, header->uid, body, bodylen);
    } else {
        ERROR_TLOG("%u Recv Err From Svr[cli_cmd:0x%04X wait_serv_cmd:0x%04X ret:%u]", 
                client->userid, client->cli_cmd, header->cmd, header->ret);
        processor->proc_errno_from_serv(client, header->ret); 
    }
}
