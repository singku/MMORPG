#include "common.h"

#include "service.h"
#include "player.h"
#include "proto.h"
#include "timer_procs.h"
#include "global_data.h"
#include "switch_proto.h"
#include "statlogger/statlogger.h"
#include "dll_iface.h"

#define CHECK_ONLINE(header) \
    do { \
        if (player && player->is_login == false \
            && player->cli_wait_cmd != cli_cmd_cs_0x0101_enter_svr) { \
            asynsvr_send_warning_msg("offline send to svr",  \
                    header->uid, header->cmd, 1, ""); \
            WARN_TLOG("offine player[%u] send to svr stack: '%s'", \
                    player->userid, stack_trace().c_str()); \
        } \
    } while (0)






Service::Service(const std::string& service_name)
{
    service_name_ = service_name;
    fd_ = -1;
    this->is_connect_by_name_ = true;
}

Service::Service(const std::string& service_name, const std::string& ipaddr, in_addr_t port)
{
    this->service_name_ = service_name;
    this->ipaddr_ = ipaddr;
    this->port_ = port;
    fd_ = -1;
    this->is_connect_by_name_ = false;
}

Service::~Service()
{
    if (fd_ >= 0) {
        close_svr(fd_);
    }
}

int Service::send_to_act(player_t* player, userid_t userid, uint16_t cmd, 
        const char* body, int bodylen, uint32_t ret)
{
    LOCK_SVR_CHECK;
    if (fd_ < 0) {
        ERROR_TLOG("service '%s' not available", service_name_.c_str());    
        return -1;
    }

    act_proto_header_t* header = (act_proto_header_t* )send_buf_;

    if (!body || bodylen < 0) {
        bodylen = 0; 
    }

    header->len = sizeof(*header) + bodylen;
    header->seq = player ? player->fdsess->fd : 0;
    header->cmd = cmd;
    header->ret = ret;
    header->uid = userid;

    if (bodylen) {
        if (sizeof(send_buf_) - sizeof(*header) < (uint32_t)bodylen) {
            asynsvr_send_warning_msg("ServiceSendBuf OutBound", 
                    header->uid, header->cmd, 1, "");
            return -1;
        }

        memcpy(send_buf_ + sizeof(*header), body, bodylen);
    }

    int err = net_send(fd_, send_buf_, header->len);

    if (err != 0) {
        ERROR_TLOG("net_send to service '%s' failed", 
                service_name_.c_str());
        return -1; 
    }
 
TRACE_TLOG("SEND BUF TO SERVICE [u:%u, cmd:%u, hex_cmd:0x%04x, service:%s, len(%u+%u)=%u]",
        player ?player->userid :userid, cmd, cmd, service_name().c_str(),
        sizeof(*header), bodylen, header->len);

    CHECK_ONLINE(header);
    if (player) {
        player->serv_cmd = cmd;
        player->wait_svr_cmd = cmd;

        if (player->svr_request_timer) {
            WARN_TLOG("Player[%u] Add DB_Request_Timer[ReqCmd:%u] While Another Timer Already Added",
                    player->userid, cmd);
        } else {
            uint32_t tm = player->temp_info.my_svr_time_out;
            tm = tm? tm :kTimerIntervalCheckSvrTimeout;
            player->svr_request_timer = ADD_TIMER_EVENT_EX(player, 
                    kTimerTypeCheckSvrTimeout,
                    (void*)(player->userid), 
                    get_now_tv()->tv_sec + tm);
            player->temp_info.svr_req_start = *get_now_tv();
        }
    }

    return 0;
}

int Service::send_buf(player_t* player, userid_t userid, uint32_t u_create_tm, uint16_t cmd, 
        const char* body, int bodylen, uint32_t ret)
{
    LOCK_SVR_CHECK;
    if (fd_ < 0) {
        ERROR_TLOG("service '%s' not available", service_name_.c_str());    
        return -1;
    }

    svr_proto_header_t* header = (svr_proto_header_t* )send_buf_;

    if (!body || bodylen < 0) {
        bodylen = 0; 
    }

    header->len = sizeof(*header) + bodylen;
    header->seq = player ? player->fdsess->fd : 0;
    header->cmd = cmd;
    header->ret = ret;
    header->uid = userid;
    header->u_create_tm = u_create_tm;
    header->cb_uid = player ?player->userid :0;
    header->cb_u_create_tm = player ?player->create_tm :0;

    if (bodylen) {
        if (sizeof(send_buf_) - sizeof(*header) < (uint32_t)bodylen) {
            asynsvr_send_warning_msg("ServiceSendBuf OutBound", 
                    header->uid, header->cmd, 1, "");
            return -1;
        }

        memcpy(send_buf_ + sizeof(*header), body, bodylen);
    }

    int err = net_send(fd_, send_buf_, header->len);

    if (err != 0) {
        ERROR_TLOG("net_send to service '%s' failed", 
                service_name_.c_str());
        return -1; 
    }
 
TRACE_TLOG("SEND BUF TO SERVICE [u:%u, cmd:%u, hex_cmd:0x%04x, service:%s, len(%u+%u)=%u]",
        player ?player->userid :userid, cmd, cmd, service_name().c_str(),
        sizeof(*header), bodylen, header->len);

    CHECK_ONLINE(header);
    if (player) {
        player->serv_cmd = cmd;
        player->wait_svr_cmd = cmd;

        if (player->svr_request_timer) {
            WARN_TLOG("Player[%u] Add DB_Request_Timer[ReqCmd:%u] While Another Timer Already Added",
                    player->userid, cmd);
        } else {
            uint32_t tm = player->temp_info.my_svr_time_out;
            tm = tm? tm :kTimerIntervalCheckSvrTimeout;
            player->svr_request_timer = ADD_TIMER_EVENT_EX(player, 
                    kTimerTypeCheckSvrTimeout,
                    (void*)(player->userid), 
                    get_now_tv()->tv_sec + tm);
            player->temp_info.svr_req_start = *get_now_tv();
        }
    }

    return 0;
}

int Service::send_msg(player_t* player, userid_t userid, uint32_t u_create_tm, uint16_t cmd,
        const google::protobuf::Message& message, uint32_t ret)
{
    LOCK_SVR_CHECK;
    if (fd_ < 0) {
        ERROR_TLOG("service '%s' not available", service_name_.c_str());    
        return -1;
    }

    svr_proto_header_t* header = (svr_proto_header_t* )send_buf_;

    header->len = sizeof(*header) + message.ByteSize();
    header->seq = player ? player->fdsess->fd : 0;
    header->cmd = cmd;
    header->ret = ret;
    header->uid = userid;
    header->u_create_tm = u_create_tm;
    header->cb_uid = player ?player->userid :0;
    header->cb_u_create_tm = player ?player->create_tm :0;

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

    int err = net_send(fd_, send_buf_, header->len);

    if (err < 0) {
        ERROR_TLOG("net_send to service '%s' failed", 
                service_name_.c_str());
        return -1; 
    }

TRACE_TLOG("Send Msg To Service Ok: [u:%u cmd:%u hex_cmd:0X%04X service:%s len(%u+%u)=%u]\nmsg:%s\n[%s]",
            player ?player->userid :userid, cmd, cmd, service_name().c_str(),
            sizeof(*header), message.ByteSize(), header->len,
            message.GetTypeName().c_str(), message.Utf8DebugString().c_str());
 
    CHECK_ONLINE(header);
    if (player) {
        player->serv_cmd = cmd;
        player->wait_svr_cmd = cmd;

        if (player->svr_request_timer) {
            WARN_TLOG("Player[%u] Add DB_Request_Timer[ReqCmd:%u] While Another Timer Already Added",
                    player->userid, cmd);
        } else {
            uint32_t tm = player->temp_info.my_svr_time_out;
            tm = tm? tm :kTimerIntervalCheckSvrTimeout;
            player->svr_request_timer = ADD_TIMER_EVENT_EX(player, 
                    kTimerTypeCheckSvrTimeout,
                    (void*)(player->userid), 
                    get_now_tv()->tv_sec + tm);
            player->temp_info.svr_req_start = *get_now_tv();
        }
    }

    return 0;
}

int Service::send_cmsg(player_t* player, userid_t userid, uint32_t u_create_tm,
        uint16_t cmd, Cmessage *msg, uint32_t ret)
{
    if (fd_ < 0) {
        ERROR_TLOG("service '%s' not available", service_name_.c_str());    
        return -1;
    }

	svr_proto_header_t header;

	header.len = sizeof(header);
	header.seq = player ? player->fdsess->fd : 0;
	header.cmd = cmd;
	header.ret = ret;
	header.uid = userid;
    header.u_create_tm = u_create_tm;
    header.cb_uid = player ?player->userid :0;
    header.cb_u_create_tm = player ?player->create_tm :0;

    CHECK_ONLINE((&header));
    if (player) {
        player->serv_cmd = cmd;
        player->wait_svr_cmd = cmd;

        if (player->svr_request_timer) {
            WARN_TLOG("Player[%u] Add DB_Request_Timer[ReqCmd:%u] While Another Timer Already Added",
                    player->userid, cmd);
        } else {
            uint32_t tm = player->temp_info.my_svr_time_out;
            tm = tm? tm :kTimerIntervalCheckSvrTimeout;
            player->svr_request_timer = ADD_TIMER_EVENT_EX(player, 
                    kTimerTypeCheckSvrTimeout,
                    (void*)(player->userid), 
                    get_now_tv()->tv_sec + tm);
            player->temp_info.svr_req_start = *get_now_tv();
        }
    }

TRACE_TLOG("Send CMsg To Service Ok: [u:%u cmd:%u, cmd:0X%04X service:%s len(%u+?)=?]",
            player ?player->userid :userid, cmd, cmd, service_name().c_str(), header.len);

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
        DEBUG_TLOG("connect to service %s OK[fd=%d]",
                service->service_name().c_str(), fd);
        service->set_fd(fd);

        // 如果是switch服务 注册online到switch
        if (service == g_switch) {
            reg_to_switch_req(0, (void*)1);
            ADD_TIMER_EVENT_EX(&g_switch_reg_timer,
                    kTimerTypeSwRegTimely,
                    0, //0表示超时调用
                    NOW() + kTimerIntervalSwRegTimely);
        }
    } else {
        ERROR_TLOG("connect to service %s Failed", 
                service->service_name().c_str());

        ADD_TIMER_EVENT_EX(&g_reconnect_timer, 
                kTimerTypeReconnectServiceTimely, 
                service, 
                NOW() + kTimerIntervalReconnectServiceTimely);

        asynsvr_send_warning_msg("ConnErr", g_online_id, 0, 0, service->service_name().c_str());
    }
}

int Service::connect()
{
    if (is_connect_by_name_) {
        return asyn_connect_to_service(service_name_.c_str(), 0,
                65536, Service::on_connect_callback, this);    
    } else {
        return asyn_connect_to_svr(ipaddr_.c_str(), port_, 
                65536, Service::on_connect_callback, this);
    }
}
