#include "server_manager.h"
#include "player_manager.h"
#include "proto.h"

int server_t::connect()
{
    int fd = connect_to_service(server_name_.c_str(), 0, 65536, 2);
    if (fd < 0) {
        ERROR_TLOG("Connect to svr:%s failed:%s", 
                server_name_.c_str(), strerror(errno));
        return -1;
    }
    this->set_fd(fd);
    SERVER_MGR.add_svr_to_fd_svr_map(this);   
    return 0;
}

int server_t::close()
{
    if (is_connected()) {
        close_svr(fd_);
    }
    fd_ = -1;
    return 0;
}

server_t *server_manager_t::create_new_server(string server_name)
{
    server_t *svr = new server_t(server_name);
    if (!svr) {
        ERROR_TLOG("Error: failed calloc server_t, svrnm=%s", 
                server_name.c_str());
        return 0;
    }
    svrnm_to_server_map_[server_name] = svr;
    svr->connect();
     
	DEBUG_TLOG("create_new_server, svrnm=%s", server_name.c_str());
	return svr;
}

void server_manager_t::del_server(server_t *svr)
{
    battleproto::sc_battle_notify_battle_down noti_msg;
    std::set<player_t *> &player_set = svr->all_players_on_me();
    FOREACH(player_set, it) {
        player_t *player = *it;
        player->btl = 0;
        player->send_msg_to_player(btl_cmd_notify_battle_down, noti_msg);
        PLAYER_MGR.destroy_player(player);
    }
    this->svrnm_to_server_map_.erase(svr->server_name());
    this->fd_to_server_map_.erase(svr->fd());
	DEBUG_TLOG("delete_server, svrnm=%s", svr->server_name().c_str());
    delete svr;
}

int server_t::send_msg_to_svr(uint32_t userid, uint16_t cmd,
        const google::protobuf::Message& message)
{
    if (!this->is_connected()) {
        int ret = this->connect();
        if (ret) {
            ERROR_TLOG("U:%u send(%u) to server but Service:%s not connected",
                    userid, cmd);
            return -1;
        }
    }

    svr_proto_header_t* header = (svr_proto_header_t* )g_send_buf;

    header->len = sizeof(*header) + message.ByteSize();
    header->seq = this->fd();
    header->cmd = cmd;
    header->ret = 0;
    header->uid = userid;

    if (header->len > sizeof(g_send_buf)) {
        ERROR_TLOG("too large pkg size %u cmd %u send to '%s'",
                header->len, cmd, this->server_name().c_str()); 
        return -1;
    }

    if (!message.SerializePartialToArray(
            g_send_buf + sizeof(*header), 
            sizeof(g_send_buf) - sizeof(*header))) {
        ERROR_TLOG("serialize message 0x'%04x' failed", cmd);
        return -1;
    }
    
    int ret = net_send(this->fd(), g_send_buf, header->len);

    if (ret < 0) {
        ERROR_TLOG("net_send to server '%s' failed", 
                this->server_name().c_str());
        return -1; 
    }

    TRACE_TLOG("Send Msg To server_t Ok: [u:%u cmd:0X%04X server:%s]\nmsg:%s\n[%s]",
            userid, cmd, server_name().c_str(),
            message.GetTypeName().c_str(), message.Utf8DebugString().c_str());

    return 0;
}

int server_t::send_buf_to_svr(uint32_t userid, uint16_t cmd, void *data, uint32_t data_len)
{
    if (!this->is_connected()) {
        int ret = this->connect();
        if (ret) {
            ERROR_TLOG("U:%u send(%u) to server but Service:%s not connected",
                    userid, cmd);
            return -1;
        }
    }

    if (data_len > sizeof(g_send_buf)) {
        ERROR_TLOG("too large pkg size %u cmd %u send to '%s'",
                data_len, cmd, this->server_name().c_str()); 
        return -1;
    }

    int ret = net_send(this->fd(), data, data_len);

    if (ret < 0) {
        ERROR_TLOG("net_send to server '%s' failed", 
                this->server_name().c_str());
        return -1; 
    }

    TRACE_TLOG("Send Buf To server_t Ok: [fd:%d u:%u cmd:0X%04X len:%u server:%s]",
            this->fd(), userid, cmd, data_len, server_name().c_str());

    return 0;
}
