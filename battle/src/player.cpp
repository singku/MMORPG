#include "common.h"

#include "proto.h"
#include "player.h"
#include "global_data.h"

static char g_send_buf[65536 * 32];

#define CHECK_ARTIFACIAL \
    do { \
        if (player->is_artifacial) {\
            return 0;\
        }\
    } while (0)

int relay_notify_msg_to_player(player_t* player, uint32_t cmd, const google::protobuf::Message& msg)
{
    assert(player);
    CHECK_ARTIFACIAL;

    svr_proto_header_t* header = (svr_proto_header_t *)(g_send_buf);


    battleproto::sc_battle_relay relay_msg;
    relay_msg.set_uid(player->uid);
    relay_msg.set_create_tm(player->create_tm);
    relay_msg.set_cmd(cmd);
    string pkg;
    msg.SerializeToString(&pkg);
    relay_msg.set_pkg(pkg);

    uint32_t body_len = relay_msg.ByteSize();
    uint32_t total_len = sizeof(*header) + body_len;
    if (sizeof(g_send_buf) < total_len) {
        ERROR_TLOG("%u send buf size %lu not enough[need: %lu]",
                player->uid, sizeof(g_send_buf), total_len);
        return -1;
    }
 
    relay_msg.SerializeToArray(g_send_buf + sizeof(*header), 
            sizeof(g_send_buf) - sizeof(*header));

    header->len = total_len;
    header->seq = player->cur_seq;
    header->cmd = btl_cmd_notify_msg_relay;
    header->ret = 0;
    header->uid = player->uid;

TRACE_TLOG("Send Relay Msg To Player Ok: [u:%u cmd:%d, hex_cmd:0X%04X seq:%u, len(%u+%u)=%u]\nmsg:%s\n[%s]",
            player->uid, cmd, cmd, header->seq,
            sizeof(*header), body_len, total_len,
            msg.GetTypeName().c_str(), msg.Utf8DebugString().c_str());

    return send_pkg_to_client(player->fdsess, g_send_buf, header->len);
}

int relay_msg_to_player(player_t* player, uint32_t cmd, const google::protobuf::Message& msg)
{
    assert(player);
    CHECK_ARTIFACIAL;

    svr_proto_header_t* header = (svr_proto_header_t *)(g_send_buf);

    battleproto::sc_battle_relay relay_msg;
    relay_msg.set_uid(player->uid);
    relay_msg.set_create_tm(player->create_tm);
    relay_msg.set_cmd(cmd);
    string pkg;
    msg.SerializeToString(&pkg);
    relay_msg.set_pkg(pkg);

    uint32_t body_len = relay_msg.ByteSize();
    uint32_t total_len = sizeof(*header) + body_len;
    if (sizeof(g_send_buf) < total_len) {
        ERROR_TLOG("%u send buf size %lu not enough[need: %lu]",
                player->uid, sizeof(g_send_buf), total_len);
        return -1;
    }
 
    relay_msg.SerializeToArray(g_send_buf + sizeof(*header), 
            sizeof(g_send_buf) - sizeof(*header));

    header->len = total_len;
    header->seq = player->cur_seq;
    header->cmd = btl_cmd_msg_relay;
    header->ret = 0;
    header->uid = player->uid;

TRACE_TLOG("Send Relay Msg To Player Ok: [u:%u cmd:%d, hex_cmd:0X%04X seq:%u, len(%u+%u)=%u]\nmsg:%s\n[%s]",
            player->uid, cmd, cmd, header->seq,
            sizeof(*header), body_len, total_len,
            msg.GetTypeName().c_str(), msg.Utf8DebugString().c_str());

    return send_pkg_to_client(player->fdsess, g_send_buf, header->len);
}

int send_msg_to_player(player_t* player, uint32_t cmd, const google::protobuf::Message& msg)
{
    assert(player);
    CHECK_ARTIFACIAL;

    svr_proto_header_t* header = (svr_proto_header_t *)(g_send_buf);

    uint32_t body_len = msg.ByteSize();
    uint32_t total_len = sizeof(*header) + body_len;
    if (sizeof(g_send_buf) < total_len) {
        ERROR_TLOG("%u send buf size %lu not enough[need: %lu]",
                player->uid, sizeof(g_send_buf), total_len);
        return -1;
    }
 
    msg.SerializeToArray(g_send_buf + sizeof(*header), 
            sizeof(g_send_buf) - sizeof(*header));

    header->len = total_len;
    header->seq = player->cur_seq;
    header->cmd = cmd;
    header->ret = 0;
    header->uid = player->uid;

TRACE_TLOG("Send Msg To Player Ok: [u:%u cmd:%d, hex_cmd:0X%04X seq:%u, len(%u+%u)=%u]\nmsg:%s\n[%s]",
            player->uid, cmd, cmd, header->seq,
            sizeof(*header), body_len, total_len,
            msg.GetTypeName().c_str(), msg.Utf8DebugString().c_str());

    return send_pkg_to_client(player->fdsess, g_send_buf, header->len);
}

int send_err_to_player(player_t* player, uint32_t cmd, int err)
{
    assert(player);
    CHECK_ARTIFACIAL;

    svr_proto_header_t* header = (svr_proto_header_t *)(g_send_buf);

    uint32_t total_len = sizeof(*header);
    if (sizeof(g_send_buf) < total_len) {
        ERROR_TLOG("%u send buf size %lu not enough[need: %lu]",
                player->uid, sizeof(g_send_buf), total_len);
        return -1;
    }
 
    header->len = total_len;
    header->seq = player->cur_seq;
    header->cmd = cmd;
    header->ret = err;
    header->uid = player->uid;

TRACE_TLOG("Send Err To Player Ok: [u:%u cmd:%d, hex_cmd:0X%04X seq:%u, err:%u, len=%u]",
            player->uid, cmd, cmd, header->seq, err, total_len);

    return send_pkg_to_client(player->fdsess, g_send_buf, header->len);
}

int send_err_to_fdsess(fdsession_t *fdsess, uint32_t cmd, uint32_t uid, uint32_t seq, int err)
{
    assert(fdsess);
    svr_proto_header_t* header = (svr_proto_header_t *)(g_send_buf);

    uint32_t total_len = sizeof(*header);
    if (sizeof(g_send_buf) < total_len) {
        ERROR_TLOG("send buf size %lu not enough[need: %lu]",
                sizeof(g_send_buf), total_len);
        return -1;
    }
 
    header->len = total_len;
    header->seq = seq;
    header->cmd = cmd;
    header->ret = err;
    header->uid = uid;

TRACE_TLOG("Send Err To Fdsess Ok: [u:%u cmd:%d, hex_cmd:0X%04X seq:%u, len=%u, err:%u]", 
        uid, cmd, cmd, seq, total_len, err);

    return send_pkg_to_client(fdsess, g_send_buf, header->len);
}
