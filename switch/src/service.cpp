#include "common.h"
#include "service.h"
#include "proto.h"
#include "server.h"
#include "timer_procs.h"

Service *g_dbproxy;

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

int Service::send_buf(server_t *svr, uint32_t userid, uint32_t u_create_tm, uint16_t cmd, 
        const char* body, int bodylen, bool wait_ret)
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
    header->seq = svr ?svr->fdsess()->fd :0; //seq存放等待的fd
    header->cmd = cmd;
    header->ret = 0;
    header->uid = userid;   //uid存放操作db的uid
    header->u_create_tm = u_create_tm;

    if (header->len > sizeof(send_buf_)) {
        ERROR_TLOG("too large pkg size %u cmd %u",
                header->len, cmd); 
        return -1;
    }

    if (bodylen) {
        memcpy(send_buf_ + sizeof(*header), body, bodylen);
    }

    int ret = net_send(fd_, send_buf_, header->len);

    if (ret != 0) {
        ERROR_TLOG("net_send to service '%s' failed", 
                service_name_.c_str());
        return -1; 
    }

    if (svr && wait_ret) {
        svr->set_waiting_serv_cmd(cmd);
		svr->set_serv_cmd(cmd);
        svr->svr_timer = ADD_TIMER_EVENT_EX(&g_waiting_rsp_timer, 
                kTimerTypeWaitingRsp,
                (void*)(svr->fdsess()->fd), 
                NOW() + kTimerIntervalWaitingRsp);
    }

    TRACE_TLOG("Send Buf To Service Ok: [u:%u cmd:0x%04X service:%s]",
            userid, cmd, service_name().c_str());

    return 0;
}

int Service::send_msg(server_t *svr, uint32_t userid, uint32_t u_create_tm, uint16_t cmd,
        const google::protobuf::Message& message, bool wait_ret)
{
    if (fd_ < 0) {
        ERROR_TLOG("service '%s' not available", service_name_.c_str());    
        return -1;
    }

    svr_proto_header_t* header = (svr_proto_header_t* )send_buf_;

    header->len = sizeof(*header) + message.ByteSize();
    header->seq = svr ?svr->fdsess()->fd :0;
    header->cmd = cmd;
    header->ret = 0;
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
    
    int ret = net_send(fd_, send_buf_, header->len);

    if (ret < 0) {
        ERROR_TLOG("net_send to service '%s' failed", 
                service_name_.c_str());
        return -1; 
    }
    if (svr && wait_ret) {
        svr->set_waiting_serv_cmd(cmd);
		svr->set_serv_cmd(cmd);
        svr->svr_timer = ADD_TIMER_EVENT_EX(&g_waiting_rsp_timer, 
                kTimerTypeWaitingRsp,
                (void*)(svr->fdsess()->fd), 
                NOW() + kTimerIntervalWaitingRsp);

    }
    TRACE_TLOG("Send Msg To Service Ok: [u:%u cmd:0X%04X service:%s]\nmsg:%s\n[%s]",
            userid, cmd, service_name().c_str(),
            message.GetTypeName().c_str(), message.Utf8DebugString().c_str());

    return 0;
}

int Service::close()
{
    fd_ = -1;

    return 0;
}

void Service::on_connect_callback(int fd, void* args)
{
    Service* service = (Service *)args;

    if (fd > 0) {
        DEBUG_TLOG("connect to service %s OK",
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
