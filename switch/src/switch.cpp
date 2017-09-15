#include "player.h"
#include "server.h"
#include "server_manager.h"
#include "player_manager.h"
#include "switch_proto.h"
#include "switch.h"
#include "proto.h"

static char g_send_buf[10000000];

//---------------------------------------------------------------------- 
// helper functions
//---------------------------------------------------------------------- 
//

void prepare_player_basic(const switchproto::sw_player_basic_info_t &msg, 
        player_basic_t &basic) 
{
    basic.uid_ = msg.userid();
    basic.create_tm_ = msg.create_tm();
    basic.is_vip_ = msg.is_vip();
}

void prepare_server_basic(const switchproto::cs_register_server &msg, 
        fdsession_t *fdsess, server_basic_info_t &basic) 
{
    basic.server_id_ = msg.server_id();
    basic.online_id_ = msg.online_id();
    basic.server_type_ = msg.server_type();
    basic.listen_port_ = msg.listen_port();
    basic.host_ip_ = msg.host_ip();
    basic.idc_zone_ = msg.idc_zone();
    basic.fdsess_ = fdsess;
    basic.name_ = msg.svr_name();
}

server_t *prepare_dup_reg(server_t *svr, fdsession_t *fdsess, 
        switchproto::cs_register_server &msg, int &mode)
{
    mode = -1;
    if (svr->fdsess() != fdsess) {
        //不同fdsess的重复注册
        if (msg.reg_mode() == switchproto::SERVER_REG_MODE_FORCE) {
            //抢占模式
            int fd = svr->fdsess()->fd;
            SERVER_MGR.del_server(svr);
            close_client_conn(fd);
            mode = switchproto::SERVER_REG_MODE_FORCE;
            return NULL;
        } else {//等待模式
            mode = switchproto::SERVER_REG_MODE_WAIT;
            return svr;
        }
    } else {
        mode = switchproto::SERVER_REG_MODE_FAKE_REPEAT;
        return svr;
    }
}
//---------------------------------------------------------------------- 
// functions
//---------------------------------------------------------------------- 

/**
 * @brief send_err_to_fdsess 给 fdsess 对端发送一个 err 错误码
 * @return -1: 失败, 0: 成功;
 */
int send_err_to_fdsess(fdsession_t *fdsess, uint32_t cmd, 
        int32_t err, uint32_t seq, uint32_t svr_id, 
        uint32_t uid, uint32_t u_create_tm)
{
    assert(fdsess);

	svr_proto_header_t* header = (svr_proto_header_t*)g_send_buf;
    header->len = sizeof(*header);
    header->seq = seq;
    header->cmd = cmd;
    header->ret = err;
    header->uid = uid;
    header->u_create_tm = u_create_tm;
    int ret = send_pkg_to_client(fdsess, g_send_buf, header->len);
    if (ret != 0) {
        ERROR_TLOG("send_pkg_to_client err(%d) to fdsess [cmd:%u hex_cmd:0x%04x] failed", 
                err, cmd, cmd);
        return -1; 
    }

    TRACE_TLOG("Send Err(%d) To fdsess(svr_id=%u) Ok: [cmd:%u hex_cmd:0x%04x]", 
            err, svr_id, cmd, cmd);

    return 0;
}

int send_err_to_act(fdsession_t *fdsess, uint32_t cmd,
		int32_t err, uint32_t seq, uint32_t svr_id, uint32_t uid)
{
	assert(fdsess);
	
	act_proto_header_t* header = (act_proto_header_t*)g_send_buf;
	header->len = sizeof(*header);
	header->seq = seq;
	header->cmd = cmd;
	header->ret = err;
	header->uid = uid;
	int ret = send_pkg_to_client(fdsess, g_send_buf, header->len);
	if (ret != 0) {
		ERROR_TLOG("send_pkg_to_client err(%d) to fdsess [cmd:%u hex_cmd:0x%04x] failed", 
				err, cmd, cmd);
		return -1; 
	}

    TRACE_TLOG("Send Err(%d) To fdsess(svr_id=%u) Ok: [cmd:%u hex_cmd:0x%04x]", 
            err, svr_id, cmd, cmd);
	return 0;
}

/**
 * @brief send_err_to_server 给对端发送一个 err 错误码
 * @return -1: 失败, 0: 成功;
 */
int send_err_to_server(server_t *svr, uint32_t cmd, int32_t err, 
        uint32_t uid, uint32_t u_create_tm)
{
	fdsession_t *fdsess = svr->fdsess();
	assert(fdsess);
	svr->clear_waiting_cmd();
	if (cmd >= 0x8050) {
		return send_err_to_act(fdsess, cmd, err,
				svr->process_seq, svr->online_id(), svr->online_id());
	} else {
		return send_err_to_fdsess(fdsess, cmd, err, 
				svr->process_seq, svr->online_id(), uid, u_create_tm);
	}
}

int send_msg_to_server(server_t *svr, uint32_t cmd, Message &msg, bool clear_waiting_cmd)
{
	assert(svr);
	assert(svr->fdsess());

	fdsession_t *fdsess = svr->fdsess();
    svr_proto_header_t* header = (svr_proto_header_t* )g_send_buf;
    header->len = sizeof(*header) + msg.ByteSize();
    header->seq = svr->process_seq;
    header->cmd = cmd;
    header->ret = 0;
    header->uid = svr->online_id();
    if (header->len > sizeof(g_send_buf)) {
        ERROR_TLOG("sending packets too long(%u) [cmd:%u, hex_cmd:0x%04x] svr_id:%u",
                header->len, cmd, cmd, header->uid);
        return -1;
    }

    if (clear_waiting_cmd) {
        svr->clear_waiting_cmd();
    }

    if (!msg.SerializePartialToArray(
                g_send_buf + sizeof(*header), 
                sizeof(g_send_buf) - sizeof(*header))) {
        ERROR_TLOG("serialize message 0x'%04x' failed", cmd);
        return -1;
    }

    int ret = send_pkg_to_client(fdsess, g_send_buf, header->len);
    if (ret < 0) {
        ERROR_TLOG("send_pkg_to_client to svr_id[%u] failed[cmd:%u hexcmd:0x%04x]", 
                svr->online_id(), cmd, cmd);
        return -1; 
    }

    TRACE_TLOG("Send Msg To SVR Ok: [svr_id:%u(fd:%u) seq:%u cmd:%u hexcmd:0x%04x]\nmsg:%s\n[%s]",
            svr->online_id(), svr->fdsess()->fd, header->seq, cmd, cmd,
            msg.GetTypeName().c_str(), msg.Utf8DebugString().c_str());

    return 0;
}

int send_msg_to_act(server_t *svr, uint32_t cmd, const char* body, int bodylen, int ret)
{
	assert(svr);
	assert(svr->fdsess());
	
	fdsession_t *fdsess = svr->fdsess();
	act_proto_header_t* header = (act_proto_header_t*)g_send_buf;
	header->len = sizeof(*header) + bodylen;
	header->seq = svr->process_seq;
	header->cmd = cmd;
	header->ret = ret;
	header->uid = svr->online_id();

    //if (cmd == sw_cmd_sw_get_role_info) {
        //const platform_get_role_info_ack_t  *ack_body = (const platform_get_role_info_ack_t *)body;
        //if (ack_body->level == 0) {
            //header->ret = 1;
        //}
    //}

	if (header->len > sizeof(g_send_buf)) {
		ERROR_TLOG("sending packets too long(%u) [cmd:%u, hex_cmd:0x%04x] svr_id:%u",
			header->len, cmd, cmd, header->uid);
		return -1;
	}
	svr->clear_waiting_cmd();

	if (bodylen) {
		memcpy(g_send_buf + sizeof(*header), body, bodylen);
	}

	int msg_ret = send_pkg_to_client(fdsess, g_send_buf, header->len);	
	if (msg_ret < 0) {
		ERROR_TLOG("send_pkg_to_client to svr_id[%u] failed[cmd:%u hexcmd:0x%04x]",
				svr->online_id(), cmd, cmd);
		return -1;
	}
    TRACE_TLOG("Send Msg To SVR Ok: [svr_id:%u(fd:%u) seq:%u cmd:%u hexcmd:0x%04x] bodylen:%u\n",
            svr->online_id(), svr->fdsess()->fd, header->seq, cmd, cmd, bodylen);
	
    //char hex_buf[5000] = {};
    //bin2hex(hex_buf, g_send_buf, bodylen + sizeof(*header), sizeof(g_send_buf));
    //TRACE_TLOG("Send Msg To SVR buf:%s", hex_buf);
    
	return 0;
}
