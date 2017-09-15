
#include <string>
#include <async_serv/net_if.h>

extern "C" {
#include <libtaomee/log.h>
#include <libtaomee/timer.h>
}
#include <libtaomee++/proto/proto_base.h>
#include <libtaomee++/proto/proto_util.h>

#include "dll_iface.h"
#include "proto.h"
#include "timer_procs.h"
#include "service.h"
#include "client.h"

Service::Service(const std::string& service_name)
{
    service_name_ = service_name;
    fd_ = -1;
}

Service::~Service()
{
    if (fd_ >= 0) {
        close_svr(fd_);
    }
}

int Service::send_to_act(client_info_t* client, userid_t userid, uint16_t cmd, 
        const char* body, int bodylen, uint32_t ret)
{
    if (fd_ < 0) {
        ERROR_TLOG("service '%s' not available", service_name_.c_str());    
        return -1;
    }

    act_proto_header_t* header = (act_proto_header_t* )send_buf_;

    if (!body || bodylen < 0) {
        bodylen = 0; 
    }

    header->len = sizeof(*header) + bodylen;
    header->seq = client ? client->fd : 0;
    header->cmd = cmd;
    header->ret = ret;
    header->uid = userid;

    if (bodylen) {
        memcpy(send_buf_ + sizeof(*header), body, bodylen);
    }

    if (client) {
        if (client->db_request_timer) {//上一个等待返回的服务还没有返回
            WARN_TLOG("SVR Request Again before Last SVR_cmd Respond"
                    " [u:%u, cmd:0x%04x, wait_serv_cmd:0x%04x, cli_cmd:0x%04x]",
                    client->userid, cmd, client->wait_serv_cmd, client->cli_cmd);
            return 0;
        }
        client->wait_serv_cmd = cmd;
        client->db_request_timer = ADD_TIMER_EVENT_EX(client, 
                kTimerTypeCheckDbTimeout,
                client, 
                get_now_tv()->tv_sec + kTimerIntervalCheckDbTimeout);
    }

    int err = net_send(fd_, send_buf_, header->len);
    if (err != 0) {
        ERROR_TLOG("net_send to service '%s' failed", 
                service_name_.c_str());
        return -1; 
    }
 
TRACE_TLOG("SEND BUF TO SERVICE [u:%u, cmd:%u, hex_cmd:0x%04x, service:%s, len(%u+%u)=%u]",
        client ?client->userid :0, cmd, cmd, service_name().c_str(),
        sizeof(*header), bodylen, header->len);

    return 0;
}

int Service::send_buf(client_info_t* client, userid_t userid, uint32_t u_create_tm, uint16_t cmd, 
        const char* body, int bodylen, uint32_t ret)
{
    if (fd_ < 0) {
        ERROR_TLOG("service '%s' not available", service_name_.c_str());    
        return -1;
    }

    svr_proto_header_t* header = (svr_proto_header_t* )send_buf_;

    if (!body || bodylen < 0) {
        bodylen = 0; 
    }

    header->len = sizeof(*header) + bodylen;
    header->seq = client ? client->fd : 0;
    header->cmd = cmd;
    header->ret = ret;
    header->uid = userid;
    header->u_create_tm = u_create_tm;
    if (bodylen) {
        memcpy(send_buf_ + sizeof(*header), body, bodylen);
    }

    if (client) {
        if (client->db_request_timer) {//上一个等待返回的服务还没有返回
            WARN_TLOG("SVR Request Again before Last SVR_cmd Respond"
                    " [u:%u, cmd:0x%04x, wait_serv_cmd:0x%04x, cli_cmd:0x%04x]",
                    client->userid, cmd, client->wait_serv_cmd, client->cli_cmd);
            return 0;
        }
        client->wait_serv_cmd = cmd;
        client->db_request_timer = ADD_TIMER_EVENT_EX(client, 
                kTimerTypeCheckDbTimeout,
                client, 
                get_now_tv()->tv_sec + kTimerIntervalCheckDbTimeout);
    }

    int err = net_send(fd_, send_buf_, header->len);
    if (err != 0) {
        ERROR_TLOG("net_send to service '%s' failed", 
                service_name_.c_str());
        return -1; 
    }
 
TRACE_TLOG("SEND BUF TO SERVICE [u:%u, cmd:%u, hex_cmd:0x%04x, service:%s, len(%u+%u)=%u]",
        client ?client->userid :0, cmd, cmd, service_name().c_str(),
        sizeof(*header), bodylen, header->len);

    return 0;
}

int Service::send_msg(client_info_t* client, userid_t userid, uint32_t u_create_tm, uint16_t cmd,
        const google::protobuf::Message& message, uint32_t ret)
{
    if (fd_ < 0) {
        ERROR_TLOG("service '%s' not available", service_name_.c_str());    
        return -1;
    }
    svr_proto_header_t* header = (svr_proto_header_t* )send_buf_;
    header->len = sizeof(*header) + message.ByteSize();
    header->seq = client ? client->fd : 0;
    header->cmd = cmd;
    header->ret = ret;
    header->uid = userid;
    header->u_create_tm = u_create_tm;

    if (header->len > sizeof(send_buf_)) {
        ERROR_TLOG("too large pkg size %u cmd %u send to '%s'",
                header->len, cmd, service_name_.c_str()); 
        return -1;
    }

    if (!message.SerializePartialToArray(
            send_buf_ + sizeof(*header), 
            sizeof(send_buf_) - sizeof(*header))) {
        ERROR_TLOG("serialize message 0x'%04x' failed", cmd);
        return -1;
    }

    if (client) {
        if (client->db_request_timer) {//上一个等待返回的服务还没有返回
            WARN_TLOG("SVR Request Again before Last SVR_cmd Respond"
                    " [u:%u, cmd:0x%04x, wait_serv_cmd:0x%04x, cli_cmd:0x%04x]",
                    client->userid, cmd, client->wait_serv_cmd, client->cli_cmd);
            return 0;
        }
        client->wait_serv_cmd = cmd;
        client->db_request_timer = ADD_TIMER_EVENT_EX(client, 
                kTimerTypeCheckDbTimeout,
                client, 
                get_now_tv()->tv_sec + kTimerIntervalCheckDbTimeout);
    }

    int err = net_send(fd_, send_buf_, header->len);

    if (err < 0) {
        ERROR_TLOG("net_send to service '%s' failed", 
                service_name_.c_str());
        return -1; 
    }

TRACE_TLOG("Send Msg To Service Ok: [u:%u cmd:%u hex_cmd:0X%04X service:%s len(%u+%u)=%u]\nmsg:%s\n[%s]",
            client ?client->userid :0, cmd, cmd, service_name().c_str(),
            sizeof(*header), message.ByteSize(), header->len,
            message.GetTypeName().c_str(), message.Utf8DebugString().c_str());

    return 0;
}

int Service::send_cmsg(client_info_t* client, userid_t userid, uint32_t u_create_tm,
        uint16_t cmd, Cmessage *msg, uint32_t ret)
{
    if (fd_ < 0) {
        ERROR_TLOG("service '%s' not available", service_name_.c_str());    
        return -1;
    }

	svr_proto_header_t header;

	header.len = sizeof(header);
	header.seq = client ? client->fd : 0;
	header.cmd = cmd;
	header.ret = ret;
	header.uid = userid;
    header.u_create_tm = u_create_tm;
    if (client) {
        if (client->db_request_timer) {//上一个等待返回的服务还没有返回
            WARN_TLOG("SVR Request Again before Last SVR_cmd Respond"
                    " [u:%u, cmd:0x%04x, wait_serv_cmd:0x%04x, cli_cmd:0x%04x]",
                    client->userid, cmd, client->wait_serv_cmd, client->cli_cmd);
            return 0;
        }
        client->wait_serv_cmd = cmd;
        client->db_request_timer = ADD_TIMER_EVENT_EX(client, 
                kTimerTypeCheckDbTimeout,
                client, 
                get_now_tv()->tv_sec + kTimerIntervalCheckDbTimeout);
    }

TRACE_TLOG("Send CMsg To Service Ok: [u:%u cmd:%u, cmd:0X%04X service:%s len(%u+?)=?]",
            client ?client->userid :0, cmd, cmd, service_name().c_str(), header.len);

	return net_send_msg(fd_, (char *)&header, msg);
}

int Service::close()
{
    fd_ = -1;

    return 0;
}

void Service::on_connect_callback(int fd, void* args)
{
    Service* service = (Service *)args;

    DEBUG_LOG("on connect callback");

    if (fd > 0) {
        DEBUG_LOG("connect to service %s OK",
                service->service_name().c_str());
        service->set_fd(fd);

    } else {
        ERROR_TLOG("connect to service %s Failed", 
                service->service_name().c_str());

        ADD_TIMER_EVENT_EX(&g_reconnect_timer, 
                kTimerTypeReconnectServiceTimely, 
                service, 
                get_now_tv()->tv_sec + kTimerIntervalReconnectServiceTimely);
    }
}

int Service::connect()
{
    return asyn_connect_to_service(service_name_.c_str(), 0,
            65536, Service::on_connect_callback, this);    
}

