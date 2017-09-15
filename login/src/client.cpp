#include <stdio.h>
#include <map>

#include <google/protobuf/message.h>
#include <async_serv/net_if.h>
#include <libtaomee/project/stat_agent/msglog.h>
extern "C" {
#include <libtaomee/log.h>
}

#include "dll_iface.h"
#include "client.h"
#include "proto.h"
#include "proto/client/cli_cmd.h"
#include "proto/client/header.pb.h"
#include "proto_queue.h"
#include "proto_processor.h"
#include "statlogger/statlogger.h"
#include "mencrypt.h"

static std::map<int, client_info_t*> clients;
static char send_buf[65536];
static uint32_t kMaxProtoQueueNum = 30;

client_info_t* get_client(int fd)
{
    std::map<int, client_info_t*>::iterator it;

    it = clients.find(fd);

    if (it == clients.end()) {
        return NULL; 
    } else {
        return it->second; 
    }
}

client_info_t* get_or_alloc_client(int fd)
{
    client_info_t* client = NULL;
    client = get_client(fd);
    if (client == NULL) {
        client = (client_info_t *)calloc(1, sizeof(client_info_t));
        if (client == NULL) {
            ERROR_TLOG("out of memory"); 
            return NULL;
        }
        clients[fd] = client;
        client->fd = fd;
        INIT_LIST_HEAD(&client->timer_list);
        client->proto_queue = new ProtoQueue(kMaxProtoQueueNum);
        client->cli_seq = 0;
        client->pkg_idx = 0;
        client->svr_create_tm_map = new std::map<uint32_t, uint32_t> ();
        client->cache_msg = new string();
    }

    return client;
}

void destroy_client(int fd)
{
    std::map<int, client_info_t*>::iterator it;
    it = clients.find(fd);
    if (it != clients.end()) {
        //从命令缓存队列里面清除
        g_pending_proto_clients.erase(it->first);
        delete(it->second->proto_queue);
        it->second->proto_queue = NULL;
        delete(it->second->svr_create_tm_map);
        delete(it->second->cache_msg);
        free(it->second);
        clients.erase(it);
    }
}

int send_to_fdsess(fdsession_t *fdsess, uint32_t cmd, 
        const char *body, int bodylen, uint32_t uid, 
        uint32_t seq, int32_t ret)
{
    assert(fdsess);

    cli_proto_header_t *header = (cli_proto_header_t *)send_buf;
    onlineproto::proto_header_t pb_header;
    pb_header.set_seque(seq);
    pb_header.set_uid(uid);
    pb_header.set_ret(ret);

    if (!body || bodylen < 0) {
        bodylen = 0; 
    }

    uint32_t pb_head_len = pb_header.ByteSize();
    uint32_t total_len = sizeof(*header) + pb_head_len + bodylen;
    if (total_len > sizeof(send_buf)) {
        ERROR_TLOG("u:%u too large package %d", uid, total_len); 
        return -1;
    }

    header->len = total_len;
    header->head_len = pb_head_len;
    header->cmd = cmd; 

    pb_header.SerializeToArray(send_buf + sizeof(*header), 
            sizeof(send_buf) - sizeof(*header));

    if (body && bodylen) {
        memcpy(send_buf + sizeof(*header) + pb_head_len, body, bodylen);  
    }

    if (cli_proto_encrypt) {//cli_proto_header_t之后的数据加密
        uint32_t encrypt_part_len = 0;
        uint32_t no_encrypt_len = sizeof(*header) - sizeof(uint32_t);
        char *encrypt_part = (char*)msg_encrypt(send_buf + no_encrypt_len, 
                pb_head_len + bodylen + sizeof(uint32_t), &encrypt_part_len);
        total_len = no_encrypt_len + encrypt_part_len;
        if (total_len > sizeof(send_buf)) {
            ERROR_TLOG("U:%u too large body package %d", uid, total_len); 
            return -1;
        }
        memcpy(send_buf + no_encrypt_len, encrypt_part, encrypt_part_len);
        header->len = total_len;
    }


TRACE_TLOG("Send Buf To Client: len = %u, seq = %u, cmd = %u, ret = %u", 
            header->len, seq, cmd, ret);

    return send_pkg_to_client(fdsess, send_buf, total_len);
}

int send_to_client(client_info_t* client, uint32_t cmd, 
        const char* body, int bodylen, int32_t ret, uint32_t seq)
{
    assert(client);

    return send_to_fdsess(client->fdsession, cmd, body, bodylen, 
            client->userid, seq ?seq :client->cli_seq, ret);
}

int send_msg_to_client(client_info_t* client, uint32_t cmd, 
        const google::protobuf::Message& msg)
{
    assert(client);

    cli_proto_header_t* header = (cli_proto_header_t *)(send_buf);
    onlineproto::proto_header_t pb_header;
    pb_header.set_uid(client->userid);
    pb_header.set_seque(client->cli_seq);
    pb_header.set_ret(0);

    uint32_t pb_head_len = pb_header.ByteSize();
    uint32_t body_len = msg.ByteSize();
    uint32_t total_len = sizeof(*header) + pb_head_len + body_len;

    if (sizeof(send_buf) < total_len) {
        ERROR_TLOG("%u send buf size %lu not enough proto %lu",
                client->userid, sizeof(send_buf), total_len);
        return -1;
    }

    header->len = total_len;
    header->head_len = pb_head_len;
    header->cmd = cmd ?cmd :client->cli_cmd;
    pb_header.SerializeToArray(send_buf + sizeof(*header),
            sizeof(send_buf) - sizeof(*header));

    msg.SerializeToArray(send_buf + sizeof(*header) + pb_head_len, 
            sizeof(send_buf) - sizeof(*header) - pb_head_len);

    if (cli_proto_encrypt) {//cli_proto_header_t之后的数据加密
        uint32_t encrypt_part_len = 0;
        uint32_t no_encrypt_len = sizeof(*header) - sizeof(uint32_t);
        char *encrypt_part = (char*)msg_encrypt(send_buf + no_encrypt_len,  
                pb_head_len + body_len + sizeof(uint32_t), &encrypt_part_len);
        total_len = no_encrypt_len + encrypt_part_len;
        if (total_len > sizeof(send_buf)) {
            ERROR_TLOG("U:%u too large body package %d", client->userid, total_len); 
            return -1;
        }
        memcpy(send_buf + no_encrypt_len, encrypt_part, encrypt_part_len);
        header->len = total_len;
    }


TRACE_TLOG("Send Msg To Client Ok: [u:%u seq:%u, cmd:%d, hex_cmd:0X%04X len(%u+%u)=%u]\nmsg:%s\n[%s]",
            client->userid, client->cli_seq, client->cli_cmd, client->cli_cmd,
            sizeof(*header), msg.ByteSize(), header->len,
            msg.GetTypeName().c_str(), msg.Utf8DebugString().c_str());

    return send_pkg_to_client(client->fdsession, send_buf, header->len);
}

int send_err_to_client(client_info_t *client, int32_t err, uint32_t seq)
{
    return send_to_client(client, client->cli_cmd, NULL, 0, err, seq);
}

int send_err_to_fdsess(fdsession_t *fdsess, uint32_t cmd, int32_t err, uint32_t seq)
{
    return send_to_fdsess(fdsess, cmd, NULL, 0, cmd, seq, err);
}

void write_msglog_new(userid_t userid, uint32_t svr_id,
        const std::string& dir, const std::string& name, const std::string& subname, uint32_t count)
{
    StatInfo stat;
    stat.add_info("item", subname);
    stat.add_op(StatInfo::op_item, "item");
    get_stat_logger(svr_id)->log(dir, name, to_string(userid), "", stat); 
}

void write_msglog_new(const std::string &user, uint32_t svr_id,
        const std::string& dir, const std::string& name, const std::string& subname, uint32_t count)
{
    StatInfo stat;
    stat.add_info("item", subname);
    stat.add_op(StatInfo::op_item, "item");
    get_stat_logger(svr_id)->log(dir, name, user, "", stat); 
}

void write_msglog_new(const char *user, uint32_t svr_id,
        const std::string& dir, const std::string& name, const std::string& subname, uint32_t count)
{
    StatInfo stat;
    stat.add_info("item", subname);
    stat.add_op(StatInfo::op_item, "item");
    get_stat_logger(svr_id)->log(dir, name, std::string(user), "", stat); 
}

std::string to_string(uint32_t n)
{
    char hex_buf[128] = {0};

    snprintf(hex_buf, sizeof(hex_buf), "%u", n);

    return std::string(hex_buf);
}
