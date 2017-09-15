#include "service.h"
#include "player.h"
#include "proto.h"
#include "timer_procs.h"

Service *g_dbproxy;
Service *g_switch;

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

int Service::send_buf(player_t* player, uint32_t userid, uint32_t u_create_tm,
        uint16_t cmd, const char* body, int bodylen)
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
    header->seq = player ?player->uid() :0; //seq存放等待的uid
    header->cmd = cmd;
    header->ret = 0;
    header->uid = userid;   //uid存放操作db的uid
    header->u_create_tm = u_create_tm;
    header->cb_uid = player ?player->uid() :0;
    header->cb_u_create_tm = player ?player->u_create_tm() : 0;

    if (bodylen) {
        memcpy(send_buf_ + sizeof(*header), body, bodylen);
    }

    int ret = net_send(fd_, send_buf_, header->len);

    if (ret != 0) {
        ERROR_TLOG("net_send to service '%s' failed", 
                service_name_.c_str());
        return -1; 
    }

    TRACE_TLOG("Send Buf To Service Ok: [u:%u cmd:0x%04X service:%s]",
            player ?player->uid() :0, cmd, service_name().c_str());

    if (player) {
        player->wait_serv_cmd_ = cmd;
        player->serv_cmd_ = cmd;
    }

    return 0;
}

int Service::send_msg(player_t* player, uint32_t userid, uint32_t u_create_tm,
        uint16_t cmd, const google::protobuf::Message& message)
{
    if (fd_ < 0) {
        ERROR_TLOG("service '%s' not available", service_name_.c_str());    
        return -1;
    }

    svr_proto_header_t* header = (svr_proto_header_t* )send_buf_;

    header->len = sizeof(*header) + message.ByteSize();
    header->seq = player ?player->uid() :0;
    header->cmd = cmd;
    header->ret = 0;
    header->uid = userid;
    header->u_create_tm = u_create_tm;
    header->cb_uid = player ?player->uid() :0;
    header->cb_u_create_tm = player ?player->u_create_tm() : 0;

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

    TRACE_TLOG("Send Msg To Service Ok: [u:%u cmd:0X%04X service:%s]\nmsg:%s\n[%s]",
            player ?player->uid() :0, cmd, service_name().c_str(),
            message.GetTypeName().c_str(), message.Utf8DebugString().c_str());

    if (player) {
        player->serv_cmd_ = cmd;
        player->wait_serv_cmd_ = cmd;
    }

    return 0;
}

int Service::send_cmsg(player_t* player, uint32_t userid, uint32_t u_create_tm,
        uint16_t cmd, Cmessage *msg)
{
    if (fd_ < 0) {
        KERROR_LOG(0, "service '%s' not available", service_name_.c_str());    
        return -1;
    }

	svr_proto_header_t header;

	header.len = sizeof(header);
	header.seq = player ?player->uid() :0;
	header.cmd = cmd;
	header.ret = 0;
	header.uid = userid;
    header.u_create_tm = u_create_tm;
    header.cb_uid = player ?player->uid() :0;
    header.cb_u_create_tm = player ?player->u_create_tm() : 0;

    if (player) {
        player->serv_cmd_ = cmd;
        player->wait_serv_cmd_ = cmd;
    }

TRACE_TLOG("Send CMsg To Service Ok: [u:%u cmd:0X%04X service:%s]",
            player ?player->uid() :0, cmd, service_name().c_str());

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
