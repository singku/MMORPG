#include "switch.h"
#include "proto.h"
#include "service.h"
#include "player.h"
#include "player_manager.h"

//玩家离线处理
void proc_player_offline(uint32_t uid, uint32_t u_create_tm)
{
    player_t *p = PLAYER_MGR.get_player_by_role(ROLE(uid, u_create_tm));
    if (p) {
        player_t *host = PLAYER_MGR.get_player_by_role(p->at_whos_home());
        if (host) {
            host->del_visitor(p);
            p->set_at_host(ROLE(0,0));
        }
        PLAYER_MGR.try_release_player_when_offline(p);
    }
    return;
}

/* 
 * @brief 处理跨服通知回包
 */
int sw_send_msg_to_player(uint32_t uid, uint32_t create_tm, 
        uint16_t cmd, const google::protobuf::Message *msg)
{
	switchproto::cs_sw_transmit_only out;
	switchproto::sw_player_basic_info_t* player_ptr = out.add_receivers();
	player_ptr->set_userid(uid);
	player_ptr->set_create_tm(create_tm);
	out.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
	out.set_cmd(cmd);
	
	std::string pkg;
	msg->SerializeToString(&pkg);
	out.set_pkg(pkg);

	return g_switch->send_msg(0, uid, create_tm, sw_cmd_sw_transmit_only, out);
}

void handle_switch_notify(void* data, uint32_t len) 
{
	svr_proto_header_t *header = (svr_proto_header_t *)data;

    TRACE_TLOG("RECV SWITCH RET PKG[cmd:%d hex_cmd:0x%04x, uid:%u, ret:%u]", 
            header->cmd, header->cmd, header->uid, header->ret);

    if (header->ret) { //返回错误码
        switch (header->ret) {
        case sw_err_player_not_exist:
            proc_player_offline(header->uid, header->u_create_tm);
            break;
        default :
            break;
        }
        return;
    }

	return;
}
