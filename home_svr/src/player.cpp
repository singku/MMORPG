#include "player.h"
#include "proto.h"
#include "service.h"
#include "switch.h"
#include "proto/client/pb0x01.pb.h"

static char send_buf[65536 * 32];
static char hex_buf[sizeof(send_buf) * 3 + 1]; 

#define ONLINE_DETECT \
    do { \
        if (!player->fdsess()) { \
            ERROR_TLOG("SEND_TO_PLAYER[%d] BUT PLAYER NOT ONLINE", player->uid()); \
            return -1; \
        } \
    } while (0)

int player_t::home_broadcast(uint16_t cmd, const google::protobuf::Message &msg, player_t *skip)
{
	switchproto::cs_sw_transmit_only out;
    FOREACH(home_data_->at_home_player_map_, it) {
        player_t *dest = it->second;
        if (dest == skip || !dest->fdsess()) continue;

		switchproto::sw_player_basic_info_t* player_ptr = out.add_receivers();
		player_ptr->set_userid(dest->uid());
		player_ptr->set_create_tm(dest->u_create_tm());
    }

    if (out.receivers_size() == 0) {
        return 0;
    }

	out.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
	out.set_cmd(cmd);

	std::string pkg;
	msg.SerializeToString(&pkg);
	out.set_pkg(pkg);

	return g_switch->send_msg(0, this->uid(), this->u_create_tm(), sw_cmd_sw_transmit_only, out);
}

int player_t::home_broadcast_leave(player_t *p)
{
	onlineproto::sc_0x0105_notify_leave_map noti_msg;
	noti_msg.add_userid_list(p->uid());	
    home_broadcast(cli_cmd_cs_0x0105_notify_leave_map, noti_msg, p);
    return 0;
}

int player_t::home_broadcast_enter(player_t *p)
{
	onlineproto::sc_0x0103_notify_enter_map noti_msg;
    noti_msg.mutable_player()->CopyFrom(p->get_map_player_info());
	noti_msg.set_x_pos(p->x_);
	noti_msg.set_y_pos(p->y_);
    home_broadcast(cli_cmd_cs_0x0103_notify_enter_map, noti_msg, p);
    
    return 0;
}

int send_msg_to_player(player_t *player, uint32_t cmd, const google::protobuf::Message& message, bool finished)
{
    ONLINE_DETECT;

    svr_proto_header_t *header = (svr_proto_header_t *)(send_buf);

    if (sizeof(send_buf) < sizeof(*header) + message.ByteSize()) {
        KERROR_LOG(player->uid(), "send buf size %lu not enough proto %lu",
                sizeof(send_buf), sizeof(*header) + message.ByteSize());
        return -1;
    }

    message.SerializeToArray(send_buf + sizeof(*header), 
            sizeof(send_buf) - sizeof(*header));

    header->len = sizeof(*header) + message.ByteSize();
    header->seq = player ?player->cur_seq() :0;
    header->cmd = cmd;
    header->ret = 0;
    header->uid = player ?player->header_uid() :0;
    header->u_create_tm = player ?player->header_u_create_tm() : 0;
    header->cb_uid = player ?player->uid() :0;
    header->cb_u_create_tm = player ?player->u_create_tm() :0;

TRACE_TLOG("Send Msg To Player: [u:%u cmd:%d hex_cmd:0x%04X]\nmsg:%s\n[%s]",
            player ?player->uid() :0, cmd, cmd,
            message.GetTypeName().c_str(), 
            message.Utf8DebugString().c_str());

    if (finished) {
        player->wait_cmd_ = 0;
    }

    return send_pkg_to_client(player->fdsess(), send_buf, header->len);
}

int send_noti_to_player(player_t *player, uint32_t cmd, const google::protobuf::Message &message)
{
TRACE_TLOG("Send Noti Msg To Player: [u:%u cmd:%d hex_cmd:0x%04X]\nmsg:%s\n[%s]",
            player ?player->uid() :0, cmd, cmd, 
            message.GetTypeName().c_str(), 
            message.Utf8DebugString().c_str());

    return sw_send_msg_to_player(player->uid(), player->u_create_tm(), cmd, &message);
}

int send_err_to_player(player_t* player, uint32_t cmd, uint32_t ret)
{
    ONLINE_DETECT;

    svr_proto_header_t* header = (svr_proto_header_t *)send_buf;

    header->len = sizeof(*header);
    header->seq = player ?player->cur_seq() :0;
    header->cmd = cmd;
    header->ret = ret;
    header->uid = player ?player->header_uid() :0;
    header->u_create_tm = player ?player->header_u_create_tm() :0;
    header->cb_uid = player ?player->uid() :0;
    header->cb_u_create_tm = player ?player->u_create_tm() :0;

TRACE_TLOG("Send ERR TO Player[cmd:%u, hex_cmd:0x%04X, uid:%u, ret:%d]", 
            cmd, cmd, player->uid(), ret);

    player->wait_cmd_ = 0;
    return send_pkg_to_client(player->fdsess(), send_buf, header->len);
}

int send_err_to_fdsess(const fdsession_t *fdsess, uint32_t uid, uint32_t u_create_tm,
        uint32_t cb_uid, uint32_t cb_u_create_tm,
        uint32_t cmd, uint32_t seq, uint32_t ret)
{
    svr_proto_header_t* header = (svr_proto_header_t *)send_buf;
    header->len = sizeof(*header);
    header->seq = seq;
    header->cmd = cmd;
    header->ret = ret;
    header->uid = uid;
    header->u_create_tm = u_create_tm;
    header->cb_uid = cb_uid;
    header->cb_u_create_tm = cb_u_create_tm;

TRACE_TLOG("Send Err To Fd: [fd:%d cmd:%u, hex_cmd:0x%04X err:%d]",
            fdsess->fd, cmd, cmd, ret);

    return send_pkg_to_client(fdsess, send_buf, header->len);
}
