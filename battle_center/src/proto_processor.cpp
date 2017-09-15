#include "proto_processor.h"
#include "proto.h"
#include "server_manager.h"
#include "player_manager.h"

ProtoProcessor* g_proto_processor;
uint32_t max_incoming_packets_len = 8192;

ProtoProcessor::ProtoProcessor() { }

ProtoProcessor::~ProtoProcessor() { }

int ProtoProcessor::register_command(
        uint32_t cmd,
        CmdProcessorInterface* processor)
{
    cmd_processors_[cmd] = processor; 
    return 0;
}

int ProtoProcessor::get_pkg_len(int fd, const void* avail_data, 
       int avail_len, int from)
{
    if (avail_len < (int)sizeof(svr_proto_header_t)) {
        return 0; // continue to recv        
    }
    svr_proto_header_t* header = (svr_proto_header_t *)avail_data;
    if (header->len < (int)sizeof(svr_proto_header_t)) {
        ERROR_TLOG("too small pkg %u from client", header->len); 
        return -1;
    }
    if (header->len > max_incoming_packets_len) {
        ERROR_TLOG("too large pkg %u from client[max:%u]", header->len, max_incoming_packets_len); 
        return -1;
    }
    return header->len;
}

int ProtoProcessor::proc_pkg_from_client(void* data, int len,
        fdsession_t* fdsess)
{
    svr_proto_header_t* header = static_cast<svr_proto_header_t *>(data);
    const char* body = static_cast<char *>(data) + sizeof(svr_proto_header_t);
    int bodylen = header->len - sizeof(svr_proto_header_t);

    player_t *player = 0;
    //根据fd找player
    player = PLAYER_MGR.get_player_by_uid(header->uid);
    if (!player) {
        if (header->cmd != btl_cmd_enter_duplicate) {//乱入的数据不处理
            return send_err_to_fdsess(fdsess, header->cmd, 0, header->seq, header->uid);
        }
        player_t tmp_player;
        tmp_player.fdsess = fdsess;
        tmp_player.uid = header->uid;
        player = PLAYER_MGR.create_new_player(tmp_player);
    } else { //已经有player了
        if (header->cmd == btl_cmd_enter_duplicate) {//重复进入
            return send_err_to_fdsess(fdsess, header->cmd, cli_err_already_in_duplicate_map, 
                    header->seq, header->uid);
        }
    }
	// 至此, player必须有值了, 没有就是有问题
	if (!player) {
		ERROR_LOG("Error: nofound player: cmd:%u, uid_inpkg=%u",
			    header->cmd, header->uid);
        return send_err_to_fdsess(fdsess, header->cmd, cli_err_sys_err,
                header->seq, header->uid);
	}
    player->seq = header->seq;
    std::map<uint32_t, CmdProcessorInterface *>::iterator it;
    it = cmd_processors_.find(header->cmd);
    if (it == cmd_processors_.end()) {
        //没有找到,直接发给btl
        return player->relay_to_svr(data, header->cmd, len);
        //return player->send_err_to_player(header->cmd, sw_err_cmd_not_found);
    }
    CmdProcessorInterface* processor = it->second;
    int ret = processor->proc_pkg_from_client(player, body, bodylen);

    //如果放弃匹配 则处理函数中会删除player的对象
    if (header->cmd == btl_cmd_give_up_match) {
        return 0;
    }
    if (ret != 0) {
        ERROR_TLOG("PROC PKF FROM CLIENT ERR: %d [u:%d cmd:0x%04X]", 
                ret, header->uid, header->cmd); 
        return player->send_err_to_player(header->cmd, ret);
    }
    return 0;
}

void ProtoProcessor::proc_pkg_from_serv(int fd, void* data, int len)
{
    svr_proto_header_t* header = static_cast<svr_proto_header_t *>(data);
    player_t *player = PLAYER_MGR.get_player_by_uid(header->uid);
    if (!player) {
        return;
    }
    header->seq = player->seq;

    if (header->cmd == btl_cmd_enter_duplicate) {
        header->cmd = btl_cmd_notify_match_result;
    }
    //直接发给player
    player->relay_to_player(data, header->cmd, len);
    bool end_pkg = false;
    if (header->cmd == btl_cmd_notify_end) {
        end_pkg = true;
    } else if (header->cmd == btl_cmd_msg_relay) {
        battleproto::sc_battle_relay relay_msg;
        parse_message((const char*)data + sizeof(*header), len - sizeof(*header), &relay_msg);
        if (relay_msg.cmd() == cli_cmd_cs_0x0208_duplicate_exit) {
            end_pkg = true;
        }
    }
    if (end_pkg) {
        player->btl_start = false;
        PLAYER_MGR.destroy_player(player);       
    }
}
