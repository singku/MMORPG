#include "common.h"
#include "proto_processor.h"
#include "proto.h"
#include "global_data.h"
#include "player_manager.h"
#include "utils.h"
#include "timer_procs.h"

ProtoProcessor::ProtoProcessor()
{
}

ProtoProcessor::~ProtoProcessor()
{
    FOREACH(cmd_processors_, it) {
        delete it->second;
    }
    cmd_processors_.clear();
}

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
    if (from == PROTO_FROM_CLIENT) {
        if (avail_len < (int)sizeof(svr_proto_header_t)) {
            return 0; // continue to recv        
        } else {
            svr_proto_header_t* header = (svr_proto_header_t *)avail_data;

            if (header->len < (int)sizeof(svr_proto_header_t)) {
                ERROR_TLOG("too small pkg %u from client", header->len); 
                return -1;
            }

            if (header->len > g_svr_pkg_max_size) {
                ERROR_TLOG("too large pkg %u from client", header->len); 
                return -1;
            }

            return header->len;
        }
    } else if (from == PROTO_FROM_SERV) {
        if (avail_len < (int)sizeof(svr_proto_header_t)) {
            return 0; // continue to recv 
        } else {
            svr_proto_header_t* header = (svr_proto_header_t *)avail_data;
            if (header->len < (int)sizeof(svr_proto_header_t)) {
                ERROR_TLOG("too small pkg %u from server", header->len); 
                return -1;
            }
            if (header->len > g_svr_pkg_max_size) {
                ERROR_TLOG("too large pkg %u from server", header->len); 
                return -1;
            }

            return header->len;
        }
    } else {

        ERROR_TLOG("get pkg len invalid from %d", from); 
        return -1;
    }
}

int ProtoProcessor::proc_pkg_from_client(void* data, int len,
        fdsession_t* fdsess, bool from_queue)
{
    svr_proto_header_t* header =reinterpret_cast<svr_proto_header_t *>(data);
    char *body = static_cast<char *>(data) + sizeof(svr_proto_header_t);

    int bodylen = header->len - sizeof(svr_proto_header_t);

TRACE_LOG("Get Cli Pkg len = %u, seq = %u, cmd = %u hex_cmd = 0x%04X ret= %u uid=%u", 
            header->len, header->seq, header->cmd, header->cmd, header->ret, header->uid);

    player_t* player = g_player_manager->get_player_by_uid(header->uid, false);
    if (!player) {
        if (header->cmd == btl_cmd_enter_duplicate) {
            player = g_player_manager->create_new_player(header->uid, fdsess);
        }
    }

    player_t tmp_player;
    if (header->cmd == btl_cmd_duplicate_trig) {
        tmp_player.uid = header->uid;
        tmp_player.fdsess = fdsess;
        player = &tmp_player;
    }

    if (!player) {
        ERROR_TLOG("Player %u try to op battle[cmd:%u hexcmd:0x%04X] without enter_dup first",
                header->uid, header->cmd, header->cmd);
        return send_err_to_fdsess(fdsess, header->cmd, header->uid, header->seq, cli_err_duplicate_ended);
    }

    player->wait_cmd = header->cmd;
    player->cur_seq = header->seq;
    std::map<uint32_t, CmdProcessorInterface *>::iterator it;

    it = cmd_processors_.find(header->cmd);

    if (it == cmd_processors_.end()) {
        return send_err_to_player(player, player->wait_cmd, cli_err_cmd_not_find);
    }

    CmdProcessorInterface* processor = it->second;
    processor->proc_pkg_from_client(player, body, bodylen);
    //NOTI(singku)
    //处理函数中可能结束副本 销毁player 所以player可能变得不可用
    //此处之后不再加和player相关的代码
    return 0;
}

void ProtoProcessor::proc_pkg_from_serv(int fd, void* data, int len)
{
    return;
}

CmdProcessorInterface* ProtoProcessor::get_processor(uint32_t cmd)
{
    std::map<uint32_t, CmdProcessorInterface *>::iterator it;

    it = cmd_processors_.find(cmd);

    if (it == cmd_processors_.end()) {
        return NULL;
    } else {
        return it->second; 
    }
}
