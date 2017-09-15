#include "proto_processor.h"
#include "proto.h"
#include "switch.h"
#include "server.h"
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
    if (avail_len < (int)sizeof(act_proto_header_t)) {
        return 0; // continue to recv        
    }
    act_proto_header_t* header = (act_proto_header_t *)avail_data;
    if (header->len < (int)sizeof(act_proto_header_t)) {
        ERROR_TLOG("too small pkg %u from client", header->len); 
        return -1;
    }
    if (header->len > max_incoming_packets_len) {
        ERROR_TLOG("too large pkg %u from client", header->len); 
        return -1;
    }
    return header->len;
}

int ProtoProcessor::proc_pkg_from_client(void* data, int len,
        fdsession_t* fdsess, bool from_queue)
{
    svr_proto_header_t* header = static_cast<svr_proto_header_t *>(data);
    const char* body = static_cast<char *>(data) + sizeof(svr_proto_header_t);
    int bodylen = header->len - sizeof(svr_proto_header_t);
	if (header->cmd >= 0x8050) {//act cmd 包头比svr包头少
        body -= (sizeof(svr_proto_header_t) - sizeof(act_proto_header_t));
        bodylen += (sizeof(svr_proto_header_t) - sizeof(act_proto_header_t));
    }
    TRACE_TLOG("GET PKG FROM CLIENT U:%u, FD:%u CMD:0x%04x", header->uid, fdsess->fd, header->cmd);


    //服务器和switch通讯时默认是需要注册的 
    //但有一些拉取服务器信息的协议
    //比如login服务不需要注册
    //通过switch传递消息,如果是Online则需要注册,如果是第三方(比如客服给玩家加东西),第三方可以不注册
    server_t *svr = 0;
    //根据fd找svr
    svr = SERVER_MGR.get_server_by_fd(fdsess->fd);

    //如果是服务器请求注册协议(特殊处理)
    if (header->cmd == sw_cmd_register_server) {
        switchproto::cs_register_server msg;
        if (parse_message(body, bodylen, &msg)) {
            return send_err_to_fdsess(fdsess, 
                    sw_cmd_register_server, sw_err_proto_format_err, 
                    header->seq, header->uid, header->uid, header->u_create_tm);
        }
        int reg_mode = 0;
        if (svr) {
            //同一个fdess重复发注册命令，忽略
            WARN_TLOG("DUP_REG_CMD: from svr_id:%u", svr->online_id());
            return 0;
        }
        server_t *src_svr = SERVER_MGR.get_server_by_olid(header->uid);
        if (src_svr) {//同一个id的svr已经注册了
            DEBUG_TLOG("SVR_DUP_REG[%u]", header->uid);
            //src_svr可能被强制T掉
            svr = prepare_dup_reg(src_svr, fdsess, msg, reg_mode);
        }
        if (svr) { //如果svr继续存在
            if (reg_mode == switchproto::SERVER_REG_MODE_WAIT) {
                //svr存在且是不同的fdsess等待模式注册,则断开该请求
                return send_err_to_fdsess(fdsess, header->cmd, 
                        sw_err_server_reg_already_exist, header->seq, 
                        header->uid, header->uid, header->u_create_tm);
            } else if (reg_mode == switchproto::SERVER_REG_MODE_FAKE_REPEAT) {
                //同一个fdess重复发注册命令，忽略
                WARN_TLOG("DUP_REG_CMD: from svr_id:%u", svr->server_id());
                return 0;
            }
        }
        //抢占模式下 svr已经被delete 需要重新注册
        server_basic_info_t basic;
        prepare_server_basic(msg, fdsess, basic);
        svr = SERVER_MGR.create_new_server(&basic);
        if (!svr) {
            ERROR_LOG("Error: failed add_server: uid_inpkg=%u", header->uid);
            return send_err_to_fdsess(fdsess, header->cmd, 
                    sw_err_sys_err, header->seq, header->uid,
                    header->uid, header->u_create_tm);
        }
        SERVER_MGR.add_server(svr);
		SERVER_MGR.add_svrid_to_svridset(svr->server_id());

    } else { //对于非注册协议
        //如果是switch单服就能处理不等待其他服回包的cmd 就使用默认的svr
        bool must_register = false;
		switch(header->cmd) {
			case sw_cmd_get_server_list :
			case sw_cmd_sw_transmit_only :
			case sw_cmd_sw_is_player_online:      
				svr = &default_svr_;
				svr->set_online_id(header->uid);
				svr->set_server_id(99999999);
				svr->set_fdsess(fdsess);
				svr->clear_waiting_cmd();
				svr->clear_waiting_serv_cmd();
				break;
			case sw_cmd_online_sync_player_info:
			case sw_cmd_online_report_player_onoff:
				must_register = true;
				break;
			default:
				break;
		}
        if (must_register) {
            if (!svr) {
                ERROR_TLOG("Error: need_register");
                return send_err_to_fdsess(fdsess, header->cmd, 
                        sw_err_need_register, header->seq, 
                        header->uid, header->uid, header->u_create_tm);
            }
        } else {
            if (!svr) {
                svr = SERVER_MGR.create_temp_server(fdsess);
                if (!svr) {
                    ERROR_TLOG("Error: failed create_tmp_server");
					if (header->cmd >= 0x8050) {//act cmd 包头比svr包头少
						return send_err_to_act(fdsess, header->cmd,
								sw_err_sys_err, header->seq, header->uid,
								header->uid);
					} else {
						return send_err_to_fdsess(fdsess, header->cmd, 
								sw_err_sys_err, header->seq, header->uid,
								header->uid, header->u_create_tm);
					}
                }
                SERVER_MGR.add_server(svr);
            }
        }
  
        if (svr->fdsess() != fdsess){//如果已经存在 
			if (header->cmd >= 0x8050) {//act cmd 包头比svr包头少
				return send_err_to_act(fdsess, header->cmd,
						sw_err_server_conflict, header->seq, header->uid,
						header->uid);
			} else {
				return send_err_to_fdsess(fdsess, header->cmd, sw_err_server_conflict,
						header->seq, header->uid, header->uid, header->u_create_tm);
			}
        }
    }

	// 至此, svr必须有值了, 没有就是有问题
	if (!svr) {
		ERROR_LOG("Error: nofound server: cmd:%u, uid_inpkg=%u",
			    header->cmd, header->uid);
		if (header->cmd >= 0x8050) {//act cmd 包头比svr包头少
			return send_err_to_act(fdsess, header->cmd,
					sw_err_server_not_exist, header->seq, header->uid,
					header->uid);
		} else {
			return send_err_to_fdsess(fdsess, header->cmd, sw_err_server_not_exist,
					header->seq, header->uid, header->uid, header->u_create_tm);
		}
	}
    if (svr->waiting_cmd() || (!from_queue && svr->proto_queue && !svr->proto_queue->empty())) {
        ProtoQueue* queue = svr->proto_queue;

        if (queue->full()) {
            ERROR_TLOG("svr fd:%d is waiting server 0x%04x, "
                    "discard this message 0x%04x", svr->fdsess()->fd, 
                    svr->waiting_serv_cmd(), header->cmd);
            if (header->cmd >= 0x8050) {//act cmd 包头比svr包头少
                return send_err_to_act(fdsess, header->cmd,
                        sw_err_sys_busy, header->seq, header->uid,
                        header->uid);
            } else {
                return send_err_to_fdsess(fdsess, header->cmd, sw_err_sys_busy,
                        header->seq, header->uid, header->uid, header->u_create_tm);
            }
        }
        queue->push_proto((const char*)data, len, fdsess);
        TRACE_TLOG("svr fd:%d is waiting server 0x%04x, add proto %u to queue, queue_size = %u",
                svr->fdsess()->fd, svr->waiting_serv_cmd(), header->cmd, queue->size());

        if (g_pending_proto_svrs.find(svr->fdsess()->fd) 
                == g_pending_proto_svrs.end()) {
            g_pending_proto_svrs[svr->fdsess()->fd] = svr;
        }

        return 0;
    }

	if (svr->server_type() == TEMP_SVR_TYPE) {
		svr->set_online_id(header->uid);
	}
    svr->process_uid = header->uid;
    svr->set_waiting_cmd(header->cmd);
    svr->process_seq = header->seq;

    std::map<uint32_t, CmdProcessorInterface *>::iterator it;
    it = cmd_processors_.find(header->cmd);
    if (it == cmd_processors_.end()) {
        return send_err_to_server(svr, header->cmd, sw_err_cmd_not_found, header->uid, header->u_create_tm);
    }
    CmdProcessorInterface* processor = it->second;
    int ret = processor->proc_pkg_from_client(svr, body, bodylen);
    if (ret != 0) {
        ERROR_TLOG("PROC PKG FROM CLIENT ERR: %d [u:%d cmd:0x%04X]", 
                ret, header->uid, header->cmd); 
        return send_err_to_server(svr, header->cmd, ret, header->uid, header->u_create_tm);
    }

    return 0;
}

void ProtoProcessor::proc_pkg_from_serv(int fd, void* data, int len)
{
    svr_proto_header_t* header = static_cast<svr_proto_header_t *>(data);
    const char* body = (char *)data + sizeof(*header);
    int bodylen = len - sizeof(*header);

    server_t *svr = SERVER_MGR.get_server_by_fd(header->seq);
    if (!svr) {
        WARN_TLOG("RECV NO WAIT RET_PKG FROM SERVER[svr:%d cmd:%u hex_cmd:0X%04X ret:%d]", 
                header->seq, header->cmd, header->cmd, header->ret);
        return;  
    }

    if (header->cmd != svr->waiting_serv_cmd()) {
        ERROR_TLOG("RECV PKG NOT MATCH WAIT_CMD[svr:%d cmd:0x%04X, wait_cmd:0x%04X, ret:%d", 
                header->uid, header->cmd, svr->waiting_cmd(), header->ret);
        return ; 
    }

    // 清除等待的命令号
    REMOVE_TIMER(svr->svr_timer);
    svr->process_uid = header->uid;
    svr->process_u_create_tm = header->u_create_tm;
    svr->clear_waiting_serv_cmd();
    std::map<uint32_t, CmdProcessorInterface *>::iterator it;
    it = cmd_processors_.find(svr->waiting_cmd());
    if (it == cmd_processors_.end()) {
        ERROR_TLOG("cmd %u processor not find", header->cmd);  
        send_err_to_server(svr, header->cmd, sw_err_cmd_not_found, header->uid, header->u_create_tm);
        return ;
    }

    CmdProcessorInterface* processor = it->second;

    int ret = header->ret;
    if (ret == 0) {
        ret = processor->proc_pkg_from_serv(svr, body, bodylen);
    }

    if (ret) {
        ERROR_TLOG("SERVER CMD PROC ERR[cmd:0x%04X err:%d]", 
                header->cmd, ret);
        send_err_to_server(svr, header->cmd, ret, header->uid, header->u_create_tm);
    }
}
