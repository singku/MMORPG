#include "proto_processor.h"
#include "proto.h"
#include "player_manager.h"


ProtoProcessor* g_proto_processor;

ProtoProcessor::ProtoProcessor()
{
    //memset(&default_player_, 0, sizeof(default_player_));
}

ProtoProcessor::~ProtoProcessor()
{
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

            if (header->len > 128*1024) {
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
            if (header->len > 128*1024) {
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
        fdsession_t* fdsess)
{
    //小屋服务器包头的uid是房主uid 不是操作人的uid
    svr_proto_header_t* header = static_cast<svr_proto_header_t *>(data);
    const char* body = static_cast<char *>(data) + sizeof(svr_proto_header_t);
    int bodylen = header->len - sizeof(svr_proto_header_t);

    player_t *p = 0;
    bool need_player = true;
	switch (header->cmd) {
        case home_cmd_family_hall_sync_map_player_info:
        case home_cmd_family_hall_state_change:
        case home_cmd_leave_family_hall:
        case home_cmd_enter_family_hall:
        case home_cmd_change_state:
		case home_cmd_gen_visit_log:
		case home_cmd_sync_map_player_info:
		case home_cmd_notify_pet_exe_state_change:
			need_player = false;
               break;
		case home_cmd_enter_home:
			need_player = true;
            break;
		case home_cmd_exit_home:
			if (header->cb_uid == 0) {
				need_player = false;
			}
			break;
		default :
			need_player = true;
			break;
	}

    if (need_player) {
        role_info_t role;
        role.userid = header->cb_uid;
        role.u_create_tm = header->cb_u_create_tm;
        p = PLAYER_MGR.get_player_by_role(role);
        if (!p) {
            p = PLAYER_MGR.create_new_player(role, fdsess, header->seq);
        }
        if (!p) {
            ERROR_TLOG("FAILED TO CREATE NEW PLAYER uid: %d", header->ret);
            return send_err_to_fdsess(fdsess, header->uid, header->u_create_tm, 
                    header->cb_uid, header->cb_u_create_tm,
                    header->cmd, header->seq, home_err_sys_err);
        }

        //至此 P一定是个有效值
        if (p->wait_serv_cmd_) {//还在处理服务器消息
            return send_err_to_fdsess(fdsess, header->uid, header->u_create_tm,
                    header->cb_uid, header->cb_u_create_tm,
                    header->cmd, header->seq, home_err_cmd_not_finished);
        }
        p->set_cur_seq(header->seq);
        p->set_header_uid(header->uid);
        p->set_header_u_create_tm(header->u_create_tm);
        
        if (p->fdsess() != fdsess) {
            WARN_TLOG("PLAYER [%u]FDSESS NOT MATCH WILL RESET", p->uid());
            p->set_fdsess(fdsess);
        }
    } else {
        //使用default player的命令 如果没有新建player 不能请求svr调用 只能立即返回
        p = &default_player_;
        if (p->wait_serv_cmd_) {
            return send_err_to_fdsess(fdsess, header->uid, header->u_create_tm,
                    header->cb_uid, header->cb_u_create_tm,
                    header->cmd, header->seq, home_err_cmd_not_finished);
        }

        p->set_userid(header->uid);
        p->set_u_create_tm(header->u_create_tm);
        p->set_fdsess(fdsess);
        p->set_cur_seq(header->seq);
        p->set_header_uid(header->uid);
        p->set_header_u_create_tm(header->u_create_tm);
    }

    p->wait_cmd_ = header->cmd;

    std::map<uint32_t, CmdProcessorInterface *>::iterator it;
    it = cmd_processors_.find(header->cmd);
    if (it == cmd_processors_.end()) {
		ERROR_TLOG("cmd_not_find,[cmd:0x%04X]", header->cmd);
        return send_err_to_player(p, header->cmd, home_err_cmd_not_found);
    }
    CmdProcessorInterface* processor = it->second;
    int ret = processor->proc_pkg_from_client(p, body, bodylen);
    TRACE_TLOG("PROC PKG FROM CLIENT : %d [u:%d cmd:0x%04X fd:%d]", ret, header->uid, header->cmd, fdsess->fd); 
    if (ret != 0) {
        ERROR_TLOG("PROC PKF FROM CLIENT ERR: %d [u:%d cmd:0x%04X]", ret, header->uid, header->cmd); 
        return send_err_to_player(p, header->cmd, ret);
    }

    return 0;
}

void ProtoProcessor::proc_pkg_from_serv(int fd, void* data, int len)
{
    svr_proto_header_t* header = static_cast<svr_proto_header_t *>(data);
    const char* body = (char *)data + sizeof(*header);
    int bodylen = len - sizeof(*header);

    role_info_t role;
    role.userid = header->cb_uid;
    role.u_create_tm = header->cb_u_create_tm;
    player_t *player = PLAYER_MGR.get_player_by_role(role);

    if (!player) {
        WARN_TLOG("RECV NO WAIT RET_PKG FROM SERVER[u:%d cmd:0X%04X]", 
                header->seq, header->cmd);
        return;  
    }

    if (header->cmd != player->wait_serv_cmd_) {
        ERROR_TLOG("RECV PKG NOT MATCH WAIT_CMD[u:%d cmd:0x%04X, wait_cmd:0x%04X, ret:%d", 
                header->cmd, player->wait_serv_cmd_, header->ret);
        return ; 
    }

    
    std::map<uint32_t, CmdProcessorInterface *>::iterator it;
    DEBUG_TLOG("serv_cmd=[%u][0X%04X],id=[%u]", player->serv_cmd_, player->uid());
    DEBUG_TLOG("wait_cmd=[%u][0X%04X],id=[%u]", player->wait_cmd_, player->uid());
    it = cmd_processors_.find(player->wait_cmd_);
    if (it == cmd_processors_.end()) {
        KERROR_LOG(player->uid(), "cmd %u processor not find", player->serv_cmd_);  
        send_err_to_player(player, player->wait_cmd_, home_err_sys_err);
        player->wait_serv_cmd_ = 0;
        return ;
    }

    CmdProcessorInterface* processor = it->second;
    int ret = header->ret;
    if (ret == 0) {
        ret = processor->proc_pkg_from_serv(player, body, bodylen);
    }
    // 清除等待的命令号
    player->wait_serv_cmd_ = 0;

    if (ret) {
        KERROR_LOG(player->uid(), "SERVER CMD PROC ERR[cmd:0x%04X err:%d]", 
                header->cmd, ret);
        if (header->cmd == db_cmd_get_home_info) {//有可能其他人也在等待这个消息
            role_info_t host_role;
            host_role.userid = header->uid;
            host_role.u_create_tm = header->u_create_tm;
            std::set<uint64_t> wait_set;
            PLAYER_MGR.delete_from_home_waiting_list(host_role, wait_set);
            FOREACH(wait_set, it) {
                player_t *wait_p = PLAYER_MGR.get_player_by_role(KEY_ROLE(*it));
                if (!wait_p) continue;
                wait_p->set_wait_host(ROLE(0, 0));
                send_err_to_player(wait_p, wait_p->wait_cmd_, ret);
            }
        } else {
            send_err_to_player(player, player->wait_cmd_, ret);
        }
    }
}
