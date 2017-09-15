#include "common.h"
#include "proto_processor.h"
#include "proto.h"
#include "global_data.h"
#include "player_manager.h"
#include "service.h"
#include "proto_queue.h"
#include "utils.h"
#include "statlogger/statlogger.h"
#include "timer_procs.h"
#include "rank_utils.h"
#include "mencrypt.h"

ProtoProcessor::ProtoProcessor()
{
    memset(&default_player_, 0, sizeof(default_player_));
    PlayerManager::init_player(&default_player_, NULL, 0);
}

ProtoProcessor::~ProtoProcessor()
{
    FOREACH(cmd_processors_, it) {
        delete it->second;
    }
    cmd_processors_.clear();
    PlayerManager::uninit_player(&default_player_);
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

        static char request[]  = "<policy-file-request/>";
        static char response[] = "<?xml version=\"1.0\"?>"
            "<cross-domain-policy>"
            "<allow-access-from domain=\"*\" to-ports=\"*\" />"
            "</cross-domain-policy>";

        if ((avail_len == sizeof(request)) && !memcmp(avail_data, request, sizeof(request))) {
            net_send(fd, response, sizeof(response));
            TRACE_TLOG("Policy Req [%s] Received, Rsp [%s] Sent", request, response);
            return 0;
        }

        if (avail_len < (int)sizeof(cli_proto_header_t)) {
            return 0; // continue to recv        
        } else {
            cli_proto_header_t* header = (cli_proto_header_t *)avail_data;

            if (header->total_len < (int)sizeof(cli_proto_header_t)) {
                ERROR_TLOG("too small pkg %u from client", header->total_len); 
                return -1;
            }

            if (header->total_len > g_cli_pkg_max_size) {
                ERROR_TLOG("too large pkg %u from client", header->total_len); 
                return -1;
            }

            return header->total_len;
        }
    } else if (from == PROTO_FROM_SERV) {
        if (avail_len < (int)sizeof(act_proto_header_t)) {
            //账号平台的包头比D计划后台包头少4个字节的create_tm 是公司通用包头
            return 0; // continue to recv 
        } else {
            act_proto_header_t* header = (act_proto_header_t *)avail_data;
            if (header->len < (int)sizeof(act_proto_header_t)) {
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
    cli_proto_header_t* header =reinterpret_cast<cli_proto_header_t *>(data);
    void *plain_data = 0;
    uint32_t plain_data_len = 0;

    if (!cli_proto_encrypt) {
        //未加密或者测试账号不进入解密流程) 
        plain_data = (char*)data + sizeof(cli_proto_header_t);
        plain_data_len = len - sizeof(cli_proto_header_t);
    
    } else if (cli_proto_encrypt) {//解包
        uint32_t uncrypt_len = sizeof(cli_proto_header_t) - sizeof(uint32_t); //checksum参与加密
        uint32_t crypt_len = len -  uncrypt_len;
        //char *pt = bin2hex(0, (char*)data+uncrypt_len, crypt_len);
        //DEBUG_TLOG("In Bef Decode:%u[%s]", crypt_len, pt);
        plain_data = msg_decrypt((char*)data + uncrypt_len, crypt_len, &plain_data_len);
        if (likely(g_test_for_robot == 0)) {
            if (!MCheckCode_x86_32((const u8 *)plain_data, plain_data_len, 0)) {
                ERROR_TLOG("check sum failed cmd:0x%x", header->cmd);
                return send_err_to_fdsess(fdsess, header->cmd, 0, 
                        cli_err_proto_format_err, 0);
            }
        }
        //pt = bin2hex(0, (char*)plain_data, plain_data_len);
        //DEBUG_TLOG("In Aft Decode:%u[%s]", plain_data_len, pt);

        plain_data = (char *)plain_data + sizeof(uint32_t); //跳过校验和
        plain_data_len = plain_data_len - sizeof(uint32_t); //跳过校验和
    }

    char *pb_head = (char*)plain_data;
    if (plain_data_len < header->head_len) {
        ERROR_TLOG("P:(fd)%d Cli_Send_Invalid_Package, Cmd:0x%04X TotalLen:%u, HeadLen:%u PlainDataLen:%u",
                fdsess->fd, header->cmd, header->total_len, header->head_len, plain_data_len);
        return 0;
    }
    onlineproto::proto_header_t pb_header;
    if (!pb_header.ParseFromArray(pb_head, header->head_len)) {
        std::string errstr = pb_header.InitializationErrorString();
        ERROR_TLOG("Parse ProtoHead failed err= '%s', head_len:%u", errstr.c_str(), header->head_len);
        return send_err_to_fdsess(fdsess, header->cmd, 0, 
                cli_err_proto_format_err, pb_header.seque());
    }

    //判定是否服务器允许进入的时间段
    if (likely(g_svr_close_before_zero != 0)) {//服务器是会关服的
        uint32_t today_zero = TimeUtils::second_at_day_start(0);
        uint32_t this_open = today_zero + g_svr_open_after_zero;
        uint32_t next_close = today_zero + DAY_SECS - g_svr_close_before_zero;
        if (next_close <= this_open) {
            next_close += DAY_SECS;
        }
        uint32_t cur = NOW();
        if (cur < this_open || cur >= next_close) {//服务器关闭中
            send_err_to_fdsess(fdsess, header->cmd, pb_header.uid(), 
                    cli_err_server_closed, pb_header.seque());
            close_client_conn(fdsess->fd);
            return 0;
        }
    }


    char *body = pb_head + header->head_len;
    int bodylen = plain_data_len - header->head_len;

    player_t* player = g_player_manager->get_player_by_fd(fdsess->fd);

TRACE_TLOG("Get Cli Pkg U:%u Len:%u Seq:%u Cmd:%u HexCmd:0x%04X Ret:%u", 
            player ?player->userid :pb_header.uid(), header->total_len, pb_header.seque(), header->cmd, header->cmd, pb_header.ret());

    if (header->cmd != cli_cmd_cs_0x0101_enter_svr) {
        if (!player) {
            ERROR_TLOG("MUST LOGIN FIRST");
            return send_err_to_fdsess(fdsess, header->cmd, pb_header.uid(), 
                    cli_err_need_login_first, pb_header.seque());
        }
    } else {
        if (player) {
            ERROR_TLOG("P:%u DOUBLE LOGIN", player->userid); 
            return send_err_to_player(player, header->cmd, cli_err_dup_login, pb_header.seque());
        }

        player = &default_player_;
        player->userid = pb_header.uid();
        player->fdsess = fdsess;
        player->wait_svr_cmd = 0;
        player->pkg_idx = 0;
    }

    // 如果正在处理其他协议，缓存该协议，等待下次处理
    // 或者有等待协议--保证用户命令顺序执行
    if (player->wait_svr_cmd != 0
        || (!from_queue && player->proto_queue && !player->proto_queue->empty())) {

        ProtoQueue* queue = player->proto_queue;

        if (queue->full()) {
            KERROR_LOG(player->userid, "player is waiting server 0x%04x, "
                    "discard this message %u", player->serv_cmd, header->cmd);
            return send_err_to_player(player, header->cmd, cli_err_sys_busy, pb_header.seque()); 
        }

        queue->push_proto((const char*)data, len, fdsess);

        TRACE_TLOG("player %u is waiting server 0x%04x, add proto %u to queue, len = %u",
                player->userid, player->wait_svr_cmd, header->cmd, queue->size());

        if (g_pending_proto_players.find(player->userid) 
                == g_pending_proto_players.end()) {
            g_pending_proto_players[player->userid] = player;
        }

        return 0;
    }

    if (player->pkg_idx != 0) {//不是第一个包 需要检查seq
        if (unlikely(g_test_for_robot == 1) && (player->userid >= 10000 && player->userid <= 19999)) {
            //跳过seq校验
            //} else if (pb_header.seque() != player->seqno + 1) {
        } else if (pb_header.seque() <= player->seqno) {
            WARN_TLOG("U:%u Seq:%u <= Expected_Seq:%u pkg_idx:%u", player->userid,
                    pb_header.seque(), player->seqno, player->pkg_idx);
            return send_err_to_player(player, header->cmd, cli_err_seq_not_match, pb_header.seque());
        }
    }
    player->seqno = pb_header.seque();
    player->pkg_idx++;

    temp_info_t* temp_info = &player->temp_info;
    temp_info->cli_req_start = *get_now_tv();

#if 0
    // 是否登录次数过多需要先回答问题
    if (temp_info->login_too_much 
        && header->cmd != cli_cmd_cs_answer_question) {
        return send_err_to_player(player, player->wait_cmd, 
                cli_err_must_answer_question_first); 
    }
#endif

    player->cli_wait_cmd = header->cmd;
    std::map<uint32_t, CmdProcessorInterface *>::iterator it;

    it = cmd_processors_.find(header->cmd);

    if (it == cmd_processors_.end()) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    CmdProcessorInterface* processor = it->second;

    int ret = processor->proc_pkg_from_client(player, body, bodylen);

    if (ret != 0) {
        KERROR_LOG(player->userid, "PROC CLIENT PKG cmd:0x%x ERR: %d", header->cmd, ret); 
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    return 0;
}

void ProtoProcessor::proc_pkg_from_serv(int fd, void* data, int len)
{
    svr_proto_header_t* header = static_cast<svr_proto_header_t *>(data);
    const char* body = (char *)data + sizeof(*header);
    int bodylen = len - sizeof(*header);
    if (header->cmd >= 0xA000 
        || header->cmd== 2502) {//act cmd
        body -= (sizeof(svr_proto_header_t) - sizeof(act_proto_header_t));
        bodylen += (sizeof(svr_proto_header_t) - sizeof(act_proto_header_t));
    }

    bool need_match_wait_cmd = true;
    bool use_header_cmd = false;
	switch (header->cmd) {
		case sw_cmd_online_report_player_onoff:
			return;
		case btl_cmd_notify_kill_character:
		case btl_cmd_notify_end:
		case btl_cmd_notify_msg_relay:
        case btl_cmd_notify_battle_down:
        case btl_cmd_notify_match_result:
		case db_cmd_new_transaction:
		case sw_cmd_sw_notify_new_mail:
		case sw_cmd_sw_notify_player_frozen_account:
			need_match_wait_cmd = false;
			use_header_cmd = true;
			break;
		default :
			break;
	}

    bool handle_switch_ret = false;
    if (fd == g_switch->fd()) {
        use_header_cmd = true;
        handle_switch_ret = true;
        //由于switch的大部分命令号是通知命令,所以大部分是处理包头的cmd
        //但是online也会到switch查找在线人员的列表
        switch (header->cmd) {
        case sw_cmd_sw_is_player_online:
        case sw_cmd_sw_get_userid_list:
        case sw_cmd_get_server_list:
			use_header_cmd = false;
			need_match_wait_cmd = true;
            break;
        default:
            break;
        }
    }

    player_t* player = 0;
    CmdProcessorInterface* processor;
    std::map<uint32_t, CmdProcessorInterface *>::iterator it;

    if (use_header_cmd) {
        player = g_player_manager->get_player_by_userid(header->uid);
        it = cmd_processors_.find(header->cmd);
        if (handle_switch_ret) {
            if (it == cmd_processors_.end()) {
                ERROR_TLOG("Switch return unknown pkg: cmd:0x%04X", header->cmd);
                return;
			//case sw_cmd_sw_is_player_online:
			//	return;
            }
            processor = it->second;
            //WARING(singku)(这里的player大部分情况下是为0的 具体的处理函数要谨慎操作)
            processor->proc_pkg_from_serv(player, body, bodylen);
            return;
        }
    } else {
        player = g_player_manager->get_player_by_fd(header->seq);
        if (player) {
            it = cmd_processors_.find(player->cli_wait_cmd);
        }
    }

    if (!player) {
        return;
    }
    if (header->ret) {
        player->temp_info.ready_enter_dup_id = 0;
    }

    bool no_wait_money_ret = false;
    if (header->cmd == db_cmd_new_transaction) {
        no_wait_money_ret = true;
    }

    if (no_wait_money_ret == true) {
        //NOTI(singku) 如果DB返回错误码 则不能清理定时并置可以使用钻石 玩家只能重新登录
        //删除定时器 可以使用钻石
        if (header->ret == 0) {
            if (player->check_money_return) {
                REMOVE_TIMER(player->check_money_return);
            }
            player->check_money_return = NULL;
            player->temp_info.can_use_diamond = true;
            return;
        } else {
            ERROR_TLOG("P:%u db reduce money err:%u ", player->userid, header->ret);
            send_err_to_player(player, player->cli_wait_cmd, cli_err_sys_err);
            return;
        }
    }

    if (need_match_wait_cmd && header->cmd != player->wait_svr_cmd) {
        ERROR_TLOG("RECV NO MATCH PKG FROM SERVER [u:%u cmd:0x%04X wait_cmd:0x%04x ret:%d]", 
                player->userid, header->cmd, player->wait_svr_cmd, header->ret);
        return ; 
    }

    // 如果需要等待服务器cmd 且cmd相等 清除等待的命令号
    if (need_match_wait_cmd) { //可以将等待的定时器删除
        player->wait_svr_cmd = 0;
        if (player->svr_request_timer) {
            REMOVE_TIMER(player->svr_request_timer); 
            player->svr_request_timer = NULL;
        }
    }

    if (it == cmd_processors_.end()) {
        ERROR_TLOG("SERVER RET CMD NOT FOUND[u:%u cmd:0x%04X ret:%d]", 
                player->userid, header->cmd, header->ret);  
        send_err_to_player(player, player->cli_wait_cmd, cli_err_sys_err);
        return ;
    }

    processor = it->second;

    temp_info_t* temp_info = &player->temp_info;
    struct timeval db_response_time = *get_now_tv();
    struct timeval db_diff_time = {0};
    timersub(&db_response_time, &(temp_info->svr_req_start), &db_diff_time);

TRACE_TLOG("GET PKG FROM SERVER [U:%u Len:%u Seq:%u HexCmd:0x%04X Ret:%d CliCmd:%d(0x%04X) cost: %d.%06d]", 
            player->userid, header->len, header->seq, header->cmd, header->ret, player->cli_wait_cmd, player->cli_wait_cmd,
            db_diff_time.tv_sec, db_diff_time.tv_usec);

    if (header->ret == 0) {
        int ret = processor->proc_pkg_from_serv(player, body, bodylen);

        // proc_serv出错或者协议回调序列结束时 删除家族操作锁
        if (player->wait_svr_cmd == 0 && player && player->family_lock_sets != NULL) {
            FOREACH(*(player->family_lock_sets), it) {
                RankUtils::lock_release(0, player->userid, player->create_tm, *it);
            }
        }

        if (ret != 0) {
            ERROR_TLOG("%u PROC SERVER CMD ERR [cmd:0x%04x ret:%d]", 
                    player->userid, header->cmd, ret); 
            send_err_to_player(player, player->cli_wait_cmd, ret);
        }

	} else {
        // 其他服直接返回错误码时 删除家族操作锁
        if (player->wait_svr_cmd == 0 && player && player->family_lock_sets != NULL) {
            FOREACH(*(player->family_lock_sets), it) {
                RankUtils::lock_release(0, player->userid, player->create_tm,  *it);
            }
        }

        ERROR_TLOG("%u SERVER RET CMD ERR[cmd:0x%04X ret:%d]", 
                player->userid, header->cmd, header->ret);

        int cli_errno = processor->proc_errno_from_serv(player, header->ret);
        if (cli_errno && cli_errno != cli_err_sys_err) {
            SEND_ERR(cli_errno);
            return;
        } 
        if (header->ret == db_err_sys_err
            || header->ret == 2002 /* sql有误*/ 
            || header->ret == 1017 /* 系统超时*/ ) {
            asynsvr_send_warning_msg("SysErr", 
                    header->uid, header->cmd, 1, "");
        }
        if (header->ret == db_err_user_not_find) {
            SEND_ERR(cli_err_userid_not_find);

        } else if (header->ret == db_err_nick_already_exist){
            SEND_ERR(cli_err_name_already_exist);

        } else if (header->ret <= 10000){
            int parse_err = 0;
            parse_errno(header->cmd, header->ret, parse_err);
            if (parse_err == 0) {
                parse_err = cli_errno;
            }
            SEND_ERR(parse_err);

        } else {
            SEND_ERR(cli_err_sys_err);
        }
    }
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

int ProtoProcessor::parse_errno(int cmd, int serv_errno, int &cli_errno)
{
    cli_errno = serv_errno;

    if (cmd == 0xA039 && serv_errno == 1103) {
        cli_errno = cli_err_passwd_err;
    } 

    return 0;
}


