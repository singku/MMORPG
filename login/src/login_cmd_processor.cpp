#include <libtaomee++/utils/strings.hpp>
#include <libtaomee++/random/random.hpp>
extern "C" {
#include <libtaomee/log.h>
#include <libtaomee/tm_dirty/tm_dirty.h>
#include <libtaomee/random/random.h>
#include <async_serv/net_if.h>
#include <async_serv/dll.h>
}

#include "login_cmd_processor.h"
#include "dll_iface.h"
#include "client.h"
#include "service.h"
#include "statlogger/statlogger.h"
#include "auth.h"
#include "proto/db/dbproto.attr.pb.h"
#include "proto/client/attr_type.h"

int LoginCmdProcessor::proc_pkg_from_client(client_info_t* client,
        const char* body, int bodylen)
{
    //进行是否可登陆的判断
    cli_in_.Clear();
    if (parse_message(body, bodylen, &cli_in_)) {
        return send_err_to_client(client, cli_err_proto_format_err);
    }

    if (!cli_in_.has_uid() && !cli_in_.has_email() && !cli_in_.has_nick()) {
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "未填写米米号或者邮箱");
        return send_err_to_client(client, cli_err_lack_login_info);
    }

    if (cli_in_.server_id() && g_servers_map.count(cli_in_.server_id()) == 0) {
        return send_err_to_client(client, cli_err_server_not_exist);
    }

    client->on_server_id = cli_in_.server_id();

    if (cli_in_.has_uid()) {
        client->userid = cli_in_.uid();
        client->has_uid = true;
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "收到米米号登陆请求");
        get_stat_logger(client->on_server_id)->new_trans(StatLogger::bGetLoginReq, to_string(cli_in_.uid()));

    } else if (cli_in_.has_email()) {
        client->has_email = true;
        strncpy(client->email, cli_in_.email().c_str(), sizeof(client->email));
        write_msglog_new(cli_in_.email(), client->on_server_id, "用户监控", "登陆", "收到昵称登陆请求");
        get_stat_logger(client->on_server_id)->new_trans(StatLogger::bGetLoginReq, cli_in_.email());

    } else if (cli_in_.has_nick()) {
        client->has_nick = true;
        strncpy(client->nick, cli_in_.nick().c_str(), sizeof(client->nick));
        write_msglog_new(cli_in_.nick(), client->on_server_id, "用户监控", "登陆", "收到邮箱登陆请求");
        get_stat_logger(client->on_server_id)->new_trans(StatLogger::bGetLoginReq, cli_in_.nick());
    }

    //统计收到登陆请求

    login_session_check_pwd_t *session = (login_session_check_pwd_t*)client->session;
    memset(session, 0, sizeof(*session));
    act_login_with_verify_code_req_t *req_body = &(session->req_body); 

    if (cli_in_.has_email()) {
        strncpy(req_body->email, cli_in_.email().c_str(), sizeof(req_body->email));
    }
    hex2bin(req_body->passwd_md5_two, cli_in_.passwd().substr(0, 32).c_str(), SESSION_LEN); 
    req_body->verifyid = g_server_config.verifyid; 
    req_body->region = g_server_config.idc_zone; 
    req_body->gameid = g_server_config.gameid; 
    req_body->ip = client->fdsession->remote_ip;
    hex2bin(req_body->verify_session, 
            cli_in_.verify_image_session().substr(0, 32).c_str(), SESSION_LEN);
    strncpy(req_body->verify_code, cli_in_.verify_code().c_str(), 
            sizeof(req_body->verify_code));
    req_body->login_channel = 0; 
    strncpy(req_body->login_promot_tag, cli_in_.tad().c_str(), 
            sizeof(req_body->login_promot_tag));

    strncpy(session->tad, cli_in_.tad().c_str(),
            sizeof(session->tad));

    if (cli_in_.has_uid()) {
        DEBUG_TLOG("Login request, uid:%u, ip:%u, server_id:%u", 
            cli_in_.uid(), ntohl(client->fdsession->remote_ip), cli_in_.server_id());
    } else if (cli_in_.has_email()) {
        DEBUG_TLOG("Login request, email:%s, ip:%u, server_id:%u", 
            cli_in_.email().c_str(), ntohl(client->fdsession->remote_ip), cli_in_.server_id());
    }

    if (cli_in_.has_uid()) {
        // 请求account验证密码
        get_stat_logger(client->on_server_id)->new_trans(StatLogger::bSendLoginReq, to_string(cli_in_.uid()));
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "发送验证密码请求");
        return g_dbproxy->send_to_act(client, cli_in_.uid(), 
                act_cmd_login_with_verify_code, (const char *)req_body, sizeof(*req_body));

    } else if (cli_in_.has_email()) {
        write_msglog_new(cli_in_.email(), client->on_server_id, "用户监控", "登陆", "发送验证密码请求");
        get_stat_logger(client->on_server_id)->new_trans(StatLogger::bSendLoginReq, cli_in_.email());
        return g_dbproxy->send_to_act(client, 0, 
                act_cmd_login_with_verify_code, (const char *)req_body, sizeof(*req_body));

    } else {
        //没有uid和email则只能根据昵称唯一性去找uid
        db_gubn_in_.Clear();
        db_gubn_in_.set_nick(cli_in_.nick());
        return g_dbproxy->send_msg(client, 0, 0, db_cmd_get_userid_by_nick, db_gubn_in_);
    }
}

int LoginCmdProcessor::proc_pkg_from_serv(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    switch (client->cur_serv_cmd) {
    case act_cmd_login_with_verify_code:    
        return proc_pkg_from_serv_aft_check_passwd(
				client, ack_uid, body, bodylen);
		// 激活检测
	case act_cmd_is_active:
		return proc_pkg_from_serv_aft_get_is_active(
				client, ack_uid, body, bodylen);
    case db_cmd_get_base_info:
        return proc_pkg_from_serv_aft_get_base_info(
                client, ack_uid, body, bodylen);
    case db_cmd_get_userid_by_nick:
        return proc_pkg_from_serv_aft_get_userid(
                client, body, bodylen);
    default:
        return 0;
    }

    return 0;
}

int LoginCmdProcessor::proc_pkg_from_serv_aft_get_userid(client_info_t* client,
        const char* body, int bodylen)
{
    db_gubn_out_.Clear();
    
    if (parse_message(body, bodylen, &db_gubn_out_)) {
        write_msglog_new(client->nick, client->on_server_id, "用户监控", "登陆", "获取昵称系统错误");
        return send_err_to_client(client, cli_err_sys_err); 
    }

    if (db_gubn_out_.userid() == 0) {
        write_msglog_new(client->nick, client->on_server_id, "用户监控", "登陆", "昵称不存在");
        return send_err_to_client(client, cli_err_login_nick_not_exist); 
    }

    login_session_check_pwd_t* session = (login_session_check_pwd_t*)client->session;
    act_login_with_verify_code_req_t* req_body = &(session->req_body);
    client->userid = db_gubn_out_.userid();
    //统计
    write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "发送验证密码请求");
    get_stat_logger(client->on_server_id)->new_trans(StatLogger::bSendLoginReq, to_string(client->userid));
    return g_dbproxy->send_to_act(client, client->userid, 
                act_cmd_login_with_verify_code, (const char *)req_body, sizeof(*req_body));
}

int LoginCmdProcessor::proc_pkg_from_serv_aft_check_passwd(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    act_login_with_verify_code_ack_t* ack_body = 
            (act_login_with_verify_code_ack_t *)body;

    client->userid = ack_uid;
    if (bodylen < (int)sizeof(ack_body)) {
        ERROR_TLOG("too small ack body %d, expect %lu", 
                bodylen, sizeof(ack_body));
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "act返回协议有误");
        return send_err_to_client(client, cli_err_sys_err);
    }

    if (ack_body->flag == ACT_LOGIN_ACK_FLAG_SUCC
        || ack_body->flag == ACT_LOGIN_ACK_FLAG_LOGIN_IN_DIFF_CITY
        || ack_body->flag == ACT_LOGIN_ACK_FLAG_LOGIN_IN_DIFF_CITY_TOO_MUCH) {
        login_session_check_pwd_t* session = 
            (login_session_check_pwd_t *)client->session;
        std::stringstream uid_str;
        uid_str << ack_uid;
        get_stat_logger(client->on_server_id)->verify_passwd(uid_str.str(), 
                client->fdsession->remote_ip, 
                session->tad, "", "", "", "", "",
                g_server_config.idc_zone == 0 ? "电信" : "网通"); 
    }

    login_session_t* session = (login_session_t *)client->session;
    if (sizeof(*session) > sizeof(client->session)) {
        ERROR_TLOG("client session buf is not enough");
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "系统错误");
        return send_err_to_client(client, cli_err_sys_err); 
    }

    if (bodylen > (int)sizeof(session->login_buf)) {
        ERROR_TLOG("too large login result");
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "系统错误");
        return send_err_to_client(client, cli_err_sys_err); 
    }
    write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "密码验证成功");
    //统计密码验证成功
    get_stat_logger(client->on_server_id)->new_trans(StatLogger::bGetLoginSucc, to_string(ack_uid));


    client->userid = ack_uid;
    session->userid = ack_uid;
    session->login_buflen = bodylen;
    memcpy(session->login_buf, body, bodylen);

    char *ext_body = session->login_buf + sizeof(*ack_body);
    char session_hex[SESSION_LEN * 2 + 1] = {0}; 
    bin2hex_frm(session_hex, ext_body, SESSION_LEN, 0); 
    memcpy(client->login_session, session_hex, sizeof(session_hex));

    if (g_need_active_code) {
        //发送查询用户激活
        act_is_active_req_t req;
        req.gameid = g_server_config.gameid;
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "发送账号激活验证请求");
        return g_dbproxy->send_to_act(client, ack_uid, 
                act_cmd_is_active, 
                (const char*)&req, sizeof(req));
    } else {
        db_gbi_in_.Clear();
        db_gbi_in_.set_server_id(client->on_server_id);
        return g_dbproxy->send_msg(client, session->userid, 0, 
                db_cmd_get_base_info, db_gbi_in_);
    }
}

//--------------------------------------------------
// 查询用户激活
//-------------------------------------------------- 
int LoginCmdProcessor::proc_pkg_from_serv_aft_get_is_active( client_info_t* client,
		userid_t ack_uid, const char* body, int bodylen)
{
	 act_is_active_ack_t* ack = (act_is_active_ack_t*)body;

	if (bodylen != sizeof(*ack)) {
		ERROR_TLOG("invalid ack body len %d, expect %u",
				bodylen, sizeof(*ack));
		return send_to_client(client, cli_err_sys_err, NULL, 0);
	}
	//账号未激活，发错误码
	if (false == ack->is_active) {
        ERROR_TLOG("user not active %d, active status %d", 
                ack_uid, ack->is_active);
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "账号未激活");
        cli_out_.Clear();
        cli_out_.set_status(onlineproto::LOGIN_STATUS_NOT_ACTIVE);
        cli_out_.set_session(client->login_session);
        return send_msg_to_client(client, client->cli_cmd, cli_out_);
        //账号未激活回包
        //return send_err_to_client(client, cli_err_user_not_active);
	}

	login_session_t* session = (login_session_t *)client->session;
	session->is_active = ack->is_active;
    write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "账号激活验证成功");
	db_gbi_in_.Clear();
    db_gbi_in_.set_server_id(client->on_server_id);
	return g_dbproxy->send_msg(client, session->userid, 0,
			db_cmd_get_base_info, db_gbi_in_);
}

int LoginCmdProcessor::proc_pkg_from_serv_aft_get_base_info(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    db_gbi_out_.Clear();
    if (parse_message(body, bodylen, &db_gbi_out_)) {
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "db协议有误");
        return send_err_to_client(client, cli_err_sys_err); 
    }

    bool has_usable_account = false;
    bool has_account = false;
    for (int i = 0; i < db_gbi_out_.base_info_size(); i++) {
        has_account = true;
        const commonproto::player_base_info_t &base_info = db_gbi_out_.base_info(i);
        // 检查用户是否被冻结
        uint32_t now = get_now_tv()->tv_sec;
        if (base_info.frozen_end_time() != 0 && base_info.frozen_end_time() >= now) {
            write_msglog_new(client->userid, client->on_server_id, "用户监控", "米米号登录", "角色被冻结");
            continue;
        }
        has_usable_account = true;
        break;
    }
    if (has_account && !has_usable_account) {
        return send_err_to_client(client, cli_err_all_role_frozen);
    }
    if (!has_account) {
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "创建角色", "新用户验证密码成功");
    }
    return response_to_client(
            client, db_gbi_out_.mutable_base_info());
}

int LoginCmdProcessor::proc_errno_from_serv(
        client_info_t* client, int ret)
{
    uint32_t err = 0;
    if (client->cur_serv_cmd == db_cmd_get_base_info) {
        if (ret == db_err_user_not_find) {
            return response_to_client(client, NULL); 
        }
    }

    switch (ret) {
    case 0:
        err = 0;
        break;
    case 1105: //db没有找到米米号
        err = cli_err_userid_not_find;
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "米米号不存在");
        break;
    case 1107:
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "米米号被冻结");
        err = cli_err_account_frozen;
        break;
    case 1103:
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "密码错误");
        err = cli_err_passwd_err;
        break;
    case db_err_nick_not_exist:
        err = cli_err_login_nick_not_exist;
        break;
    default:
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "db错误(" + to_string(ret) + ")");
        err = cli_err_sys_err;
        break;
    }

    if (err) {
        return send_err_to_client(client, err); 
    }

    return 0;
}

int LoginCmdProcessor::response_to_client(client_info_t* client, 
        google::protobuf::RepeatedPtrField<commonproto::player_base_info_t> *base_info)
{
    cli_out_.Clear();
    if (base_info) {
        for (int i = 0; i < base_info->size(); i++) {
            onlineproto::login_base_info_t *inf = cli_out_.add_base_infos();
            inf->mutable_base_info()->CopyFrom(base_info->Get(i));
            client->svr_create_tm_map->insert(make_pair(inf->base_info().server_id(), 
                        inf->base_info().create_tm()));
        }
    }

    login_session_t* session = (login_session_t *)client->session;
    act_login_with_verify_code_ack_t* ack_body = 
            (act_login_with_verify_code_ack_t *)session->login_buf;

    const char *ext_body = session->login_buf + sizeof(*ack_body);
    int ext_bodylen = session->login_buflen - sizeof(*ack_body);

    if (ack_body->flag == ACT_LOGIN_ACK_FLAG_SUCC) {
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "登陆成功");
        act_login_succ_ack_t *ext_ack_body = 
                (act_login_succ_ack_t *)(ext_body);
        if (ext_bodylen != sizeof(*ext_ack_body)) {
            ERROR_TLOG("invalid login succ ack bodylen %u, expect %lu",
                    ext_bodylen, sizeof(*ext_ack_body));
            write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "系统错误返回包出错");
            return -1; 
        }

        char session_hex[SESSION_LEN * 2 + 1] = {0}; 
        bin2hex(session_hex, ext_ack_body->session, 
                sizeof(ext_ack_body->session), sizeof(session_hex));
        bin2hex_frm(session_hex, ext_ack_body->session, 
                sizeof(ext_ack_body->session), 0);

        cli_out_.set_status(onlineproto::LOGIN_STATUS_LOGIN_OK);
        cli_out_.set_session(std::string(session_hex)); 
        cli_out_.set_is_active(true);

    } else if (ack_body->flag == ACT_LOGIN_ACK_FLAG_WRONG_PASSWD_TOO_MUCH
            || ack_body->flag == ACT_LOGIN_ACK_FLAG_ACCOUNT_UNSAFE
            || ack_body->flag == ACT_LOGIN_ACK_FLAG_WRONG_VERIFY_CODE) {
    
        act_login_need_verify_image_ack_t* ext_ack_body = 
                (act_login_need_verify_image_ack_t *)(ext_body);

        if (ext_bodylen < (int)sizeof(*ext_ack_body)) {
            ERROR_TLOG("invalid login wrong passwd too much "
                    "ack bodylen %u, expect %lu", ext_bodylen, sizeof(*ext_ack_body)); 
            write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "系统错误返回包出错");
            return -1;
        }

        if (ext_bodylen != (int)(sizeof(*ext_ack_body) + ext_ack_body->image_size)) {
            ERROR_TLOG("invalid login wrong passwd too much "
                    "ack bodylen %u, expect %lu", 
                    ext_bodylen, sizeof(*ext_ack_body) + ext_ack_body->image_size); 
            write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "系统错误返回包出错");
            return -1; 
        }

        if (ack_body->flag == ACT_LOGIN_ACK_FLAG_WRONG_PASSWD_TOO_MUCH) {
            cli_out_.set_status(onlineproto::LOGIN_STATUS_WRONG_PASSWD_TOO_MUCH); 
            write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "密码错误次数过多");
        } else if (ack_body->flag == ACT_LOGIN_ACK_FLAG_ACCOUNT_UNSAFE) {
            cli_out_.set_status(onlineproto::LOGIN_STATUS_ACCOUNT_UNSAFE); 
            write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "账号异常");
        } else {
            cli_out_.set_status(onlineproto::LOGIN_STATUS_WRONG_VERIFY_CODE); 
            write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "验证码错误");
        }

        char session_hex[SESSION_LEN * 2 + 1] = {0}; 
        bin2hex_frm(session_hex, ext_ack_body->verify_image_session, 
                sizeof(ext_ack_body->verify_image_session), 0);

        cli_out_.set_verify_image_session(std::string(session_hex));
        cli_out_.set_verify_image(ext_ack_body->image, ext_ack_body->image_size);

    } else if (ack_body->flag == ACT_LOGIN_ACK_FLAG_LOGIN_IN_DIFF_CITY 
            || ack_body->flag == ACT_LOGIN_ACK_FLAG_LOGIN_IN_DIFF_CITY_TOO_MUCH) {
        act_login_in_diff_city_ack_t* ext_ack_body = 
                (act_login_in_diff_city_ack_t *)(ext_body);
        
        if (ext_bodylen != sizeof(*ext_ack_body)) {
            ERROR_TLOG("invalid login in diff city ack bodylen %u, expect %lu",
                    ext_bodylen, sizeof(*ext_ack_body));
            write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "系统错误返回包出错");
            return -1;
        }

        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "异地登陆");

        if (ack_body->flag == ACT_LOGIN_ACK_FLAG_LOGIN_IN_DIFF_CITY) {
            cli_out_.set_status(onlineproto::LOGIN_STATUS_LOGIN_IN_DIFF_CITY); 
        } else {
            cli_out_.set_status(onlineproto::LOGIN_STATUS_LOGIN_IN_DIFF_CITY_TOO_MUCH); 
        }

        char session_hex[SESSION_LEN * 2 + 1] = {0}; 
        bin2hex_frm(session_hex, ext_ack_body->session, 
                sizeof(ext_ack_body->session), 0);

        cli_out_.set_session(std::string(session_hex)); 
        cli_out_.set_last_login_ip(ext_ack_body->last_login_ip);
        cli_out_.set_last_login_time(ext_ack_body->last_login_time);

        char last_login_city[sizeof(ext_ack_body->last_login_city) + 1] = {0};
        char current_login_city[sizeof(ext_ack_body->current_login_city) + 1] = {0};
        strncpy(last_login_city, ext_ack_body->last_login_city, 
                sizeof(ext_ack_body->last_login_city));
        strncpy(current_login_city, ext_ack_body->current_login_city,
                sizeof(ext_ack_body->current_login_city));

        cli_out_.set_last_login_city(std::string(last_login_city));
        cli_out_.set_current_login_city(std::string(current_login_city));
        cli_out_.set_is_active(true);
    } else {
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "系统错误未知flag");
        ERROR_TLOG("recv invalid login flag %u", ack_body->flag);    
        return -1;
    }
    //设置初始序列号
    uint32_t ini_seq = get_now_tv()->tv_sec % ranged_random(1, 1000000);
    cli_out_.set_ini_seq(ini_seq);
    cli_out_.set_server_id(client->on_server_id);
    send_msg_to_client(client, client->cli_cmd, cli_out_);
    client->cli_seq = ini_seq;
    return 0;
}

int StatCmdProcessor::proc_pkg_from_client(client_info_t* client,
        const char* body, int bodylen)
{
    cli_in_.Clear();
    if (parse_message(body, bodylen, &cli_in_)) {
        return send_err_to_client(client, cli_err_proto_format_err);
    }
    write_msglog_new(client->userid, client->on_server_id, cli_in_.stid(), cli_in_.sstid(), cli_in_.dir());
    return send_err_to_client(client, 0);
}

int CreateRoleCmdProcessor::proc_pkg_from_client(client_info_t* client,
        const char* body, int bodylen)
{
    cli_in_.Clear();
    if (parse_message(body, bodylen, &cli_in_)) {
        return send_err_to_client(client, cli_err_proto_format_err);
    }
    write_msglog_new(client->userid, client->on_server_id, "用户监控", "创建角色", "收到建角色请求");
    //统计收到建角色申请
    get_stat_logger(client->on_server_id)->new_trans(StatLogger::bGetNewroleReq, to_string(client->userid));

    if (!client->on_server_id) {//服务器没定
        return send_err_to_client(client, cli_err_server_not_exist);
    }
    if (g_servers_map.count(client->on_server_id) == 0) {
        return send_err_to_client(client, cli_err_server_not_exist);
    }

    create_role_session_t* session = (create_role_session_t *)client->session;
    memset(session, 0, sizeof(session));
#if 0
    char nick_buf[64] = {0};
    snprintf(nick_buf, sizeof(nick_buf) - 1, "%s", cli_in_.nick().c_str());
    trim_blank(nick_buf);
    string nick(nick_buf);
    if (nick.find('<') != string::npos || nick.find('>') != string::npos || nick.find(' ') != string::npos) {
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "创建角色", "包含空格等非法字符");
        return send_err_to_client(client, cli_err_name_invalid_char);
    }

    if (nick == "" || nick.size() == 0) {
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "创建角色", "昵称为空");
        return send_err_to_client(client, cli_err_name_empty); 
    }

    if (nick.size() >= sizeof(session->nick)) {
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "创建角色", "昵称太长");
        return send_err_to_client(client, cli_err_sys_err);  
    }
    strncpy(session->nick, nick.c_str(), sizeof(session->nick) - 1);
#endif
    strncpy(session->nick, to_string(client->userid).c_str(), sizeof(session->nick) - 1);
    strncpy(session->tad, cli_in_.tad().c_str(), sizeof(session->tad) - 1);
    session->sex = cli_in_.sex();
    session->prof = cli_in_.prof();
    session->server_id = client->on_server_id;

    act_check_session_req_t* req_body = (act_check_session_req_t *)m_send_buf;

#if 0
    if (tm_dirty_check(0, session->nick)) {
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "创建角色", "昵称带敏感词");
        char nick_tmp[256];
        memcpy(nick_tmp, session->nick, strlen(session->nick) + 1);
        tm_dirty_replace(nick_tmp);
        WARN_TLOG("DIRTY_NICK:%s", nick_tmp);
        return send_err_to_client(client, cli_err_name_dirty);
    }
#endif

    req_body->from_game = g_server_config.gameid;
    hex2bin(req_body->session, cli_in_.session().substr(0, 32).c_str(), SESSION_LEN); 
    req_body->del_flag = 0;
    req_body->to_game = g_server_config.gameid;
    req_body->ip = client->fdsession->remote_ip;
    req_body->enter_game = 0;
    req_body->region = g_server_config.idc_zone;
    req_body->tad[0] = '\0';
    return g_dbproxy->send_to_act(client, client->userid,
            act_cmd_check_session, (const char*)req_body, sizeof(*req_body));
}

int CreateRoleCmdProcessor::proc_pkg_from_serv(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    switch (client->cur_serv_cmd) {
    // 验证session
    case act_cmd_check_session:
        return proc_pkg_from_serv_aft_check_session(
                client, ack_uid, body, bodylen);
    // 插入角色
    case db_cmd_create_role:
        return proc_pkg_from_serv_aft_create_role(
                client, ack_uid, body, bodylen);
    // 向account注册gameflag
    case act_cmd_add_gameflag:
        return proc_pkg_from_serv_aft_add_gameflag(
                client, ack_uid, body, bodylen);
    // 插入昵称
    case db_cmd_insert_nick_and_userid:
        return proc_pkg_from_serv_aft_insert_nick(
                client, ack_uid, body, bodylen);
    // 检查用户是否存在
    case db_cmd_check_user_exist:
        return proc_pkg_from_serv_aft_check_exist(
                client, ack_uid, body, bodylen);
    default:
        return 0;
    }

    return 0;
}

int CreateRoleCmdProcessor::proc_errno_from_serv(
        client_info_t* client,
        int ret)
{
    if (ret == db_err_role_already_exist) {
        // 如果存在角色，向account系统注册gameflag
        return proc_pkg_from_serv_aft_create_role(
                client, client->userid, NULL, 0);    
    } else if (ret == db_err_nick_already_exist) {
        //如果昵称重复，返回错误信息
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "创建角色", "昵称重复");
        return send_err_to_client(client, cli_err_name_already_exist);
    } else {
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "创建角色", "db错误(" + to_string(ret) + ")");
        return send_err_to_client(client, ret);
    }
}

int CreateRoleCmdProcessor::proc_pkg_from_serv_aft_check_session(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    db_cue_in_.Clear();
    create_role_session_t* session = (create_role_session_t *)client->session;
    db_cue_in_.set_server_id(session->server_id);
    db_cue_in_.set_is_init_server(true);
    return g_dbproxy->send_msg(client, client->userid, 0,
            db_cmd_check_user_exist, db_cue_in_);
}

int CreateRoleCmdProcessor::proc_pkg_from_serv_aft_check_exist(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    db_cue_out_.Clear();
    if (parse_message(body, bodylen, &db_cue_out_)) {                              
        return send_err_to_client(client, cli_err_sys_err);
    }
    
    if (db_cue_out_.exist()) {
        return send_err_to_client(client, cli_err_user_already_exist);
    }

#if 1 //现在改成了登陆时先不创建名字

    //统计发送建角色申请
    get_stat_logger(client->on_server_id)->new_trans(StatLogger::bSendNewroleSucc, to_string(client->userid));

    create_role_session_t* session = (create_role_session_t *)client->session;
    db_cr_in_.set_nick("");
    db_cr_in_.set_sex(session->sex);
    db_cr_in_.set_cur_prof(session->prof);
    if (g_servers_map.count(session->server_id) == 0) {
        return send_err_to_client(client, cli_err_server_not_exist);
    }
    const csvr_info_t &inf = (g_servers_map.find(session->server_id))->second;
    db_cr_in_.set_cur_server_id(inf.cur_svr_id);
    db_cr_in_.set_init_server_id(inf.init_svr_id);

    //游戏自定义统计男/女
    if (session->sex == 0) {
        write_msglog_new(client->userid, client->on_server_id, "基本", "创建角色", "选择男性");
    } else {
        write_msglog_new(client->userid, client->on_server_id, "基本", "创建角色", "选择女性");
    }

    return g_dbproxy->send_msg(client, client->userid, 0, 
            db_cmd_create_role, db_cr_in_);
#endif

#if 0
    db_inau_in_.Clear();
    create_role_session_t* session = (create_role_session_t *)client->session;
    db_inau_in_.set_nick(session->nick, strlen(session->nick));
    return g_dbproxy->send_msg(client,client->userid, 0, 
            db_cmd_insert_nick_and_userid,
            db_inau_in_);
#endif
}

int CreateRoleCmdProcessor::proc_pkg_from_serv_aft_insert_nick(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    create_role_session_t* session = (create_role_session_t *)client->session;
    db_cr_in_.set_nick(session->nick, strlen(session->nick));
    db_cr_in_.set_sex(session->sex);
    db_cr_in_.set_cur_prof(session->prof);
    if (g_servers_map.count(session->server_id) == 0) {
        return send_err_to_client(client, cli_err_server_not_exist);
    }
    const csvr_info_t &inf = (g_servers_map.find(session->server_id))->second;
    db_cr_in_.set_cur_server_id(inf.cur_svr_id);
    db_cr_in_.set_init_server_id(inf.init_svr_id);

    return g_dbproxy->send_msg(client, client->userid, 0,
            db_cmd_create_role, db_cr_in_);
}

int CreateRoleCmdProcessor::proc_pkg_from_serv_aft_create_role(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    db_cr_out_.Clear();
    if (parse_message(body, bodylen, &db_cr_out_)) {
        return send_err_to_client(client, cli_err_sys_err);
    }
    db_cr_out_.SerializeToString(client->cache_msg);

    chnlhash32_t *hash = (chnlhash32_t *)(m_send_buf);
    act_add_gameflag_req_t *req = (act_add_gameflag_req_t *)(m_send_buf + sizeof(*hash));
    create_role_session_t *session = (create_role_session_t *)client->session;
    memset(req, 0, sizeof(*req));

    req->idc_zone = g_server_config.idc_zone;
    req->gameid = g_server_config.gameid;
    req->channel = 0;
    strncpy(req->tad, session->tad, sizeof(req->tad));

    gen_chnlhash32(g_server_config.verifyid, g_server_config.security_code,
            (const char*)req, sizeof(*req), hash);

    std::stringstream uid_str;
    uid_str << client->userid;
    get_stat_logger(client->on_server_id)->reg_role(uid_str.str(), "", "", 
            client->fdsession->remote_ip, 
            session->tad, "", "", "", "", "",
            g_server_config.idc_zone == 0 ? "电信" : "网通");

    write_msglog_new(client->userid, client->on_server_id, "用户监控", "创建角色", "创建成功");

    return g_dbproxy->send_to_act(client, client->userid, act_cmd_add_gameflag,
            m_send_buf, sizeof(*hash) + sizeof(*req));
}

int CreateRoleCmdProcessor::proc_pkg_from_serv_aft_add_gameflag(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    db_cr_out_.ParseFromString(*(client->cache_msg));
    cli_out_.Clear();
    cli_out_.mutable_base_info()->CopyFrom(db_cr_out_.base_info());
    return send_msg_to_client(client, client->cli_cmd, cli_out_);
}

int RandomNickCmdProcessor::proc_pkg_from_client(client_info_t* client,
        const char* body, int bodylen)
{
    if (parse_message(body, bodylen, &cli_in_)) {
        return send_err_to_client(client, cli_err_proto_format_err); 
    }
    std::string nick = get_random_nick(cli_in_.sex());
    if (nick.empty()) {
        ERROR_TLOG("rand nick list is empty");
        return send_err_to_client(client, cli_err_sys_err); 
    }

    random_nick_session_t* session = (random_nick_session_t *)client->session;
    memset(session, 0, sizeof(*session));
    session->sex = cli_in_.sex();
    strncpy(session->nick, nick.c_str(), sizeof(session->nick) - 1);
    db_in_.Clear();
    db_in_.set_nick(nick);

    return g_dbproxy->send_msg(client, client->userid, 0,
            db_cmd_get_userid_by_nick, db_in_);
}

int RandomNickCmdProcessor::proc_pkg_from_serv(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    if (parse_message(body, bodylen, &db_out_)) {
        return send_err_to_client(client, cli_err_sys_err);
    }
    random_nick_session_t* session = (random_nick_session_t *)client->session;
    if (db_out_.userid() == 0) {
        cli_out_.Clear();
        cli_out_.set_nick(session->nick);
        return send_msg_to_client(client, client->cli_cmd, cli_out_);
    } else {
        std::string nick = get_random_nick(session->sex); 
        strncpy(session->nick, nick.c_str(), sizeof(session->nick));

        db_in_.Clear();
        db_in_.set_nick(nick);
        return g_dbproxy->send_msg(client, client->userid, 0, 
                db_cmd_get_userid_by_nick, db_in_);
    }

    return 0; 
}

std::string RandomNickCmdProcessor::get_random_nick(uint32_t sex)
{
    if (rand_nick_pos1[0].size() == 0 || rand_nick_pos1[1].size() == 0
        || rand_nick_pos2.size() == 0 || rand_nick_pos3.size() == 0) {
        return "";
    }
    std::string nick;
    uint32_t index1 = rand() % rand_nick_pos2.size();
    uint32_t index2 = rand() % rand_nick_pos3.size(); 

    nick += rand_nick_pos3[index2];

    if (index1 % 2) {
        nick += rand_nick_pos2[index1];
    }

    if (sex == 0) {
        uint32_t index = rand() % rand_nick_pos1[0].size();
        nick += rand_nick_pos1[0][index];
    } else {
        uint32_t index = rand() % rand_nick_pos1[1].size(); 
        nick += rand_nick_pos1[1][index];
    }

    return nick;
}

int GetSvrListCmdProcessor::proc_pkg_from_client(
        client_info_t* client, const char* body, int bodylen)
{
    cli_in_.Clear();
    cli_out_.Clear();

    //统计拉服务器列表
    get_stat_logger(client->on_server_id)->new_trans(StatLogger::bStartGetSrvlist, to_string(client->userid));

    if (parse_message(body, bodylen, &cli_in_)) {
        return send_err_to_client(client, cli_err_proto_format_err);
    }

    act_check_session_req_t* req_body = (act_check_session_req_t *)m_send_buf;
    req_body->from_game = g_server_config.gameid;
    hex2bin(req_body->session, cli_in_.session().substr(0, 32).c_str(), 32); 
    req_body->del_flag = 0;
    req_body->to_game = g_server_config.gameid;
    req_body->ip = client->fdsession->remote_ip;
    req_body->enter_game = 0;
    req_body->region = g_server_config.idc_zone;
    req_body->tad[0] = '\0';

    uint32_t *recommend = (uint32_t*)client->session;
    *recommend = (uint32_t)(cli_in_.svr_recommend());
    return g_dbproxy->send_to_act(client, client->userid,
            act_cmd_check_session, (const char *)req_body, sizeof(*req_body));
}

int GetSvrListCmdProcessor::proc_pkg_from_serv(client_info_t* client, 
        userid_t ack_uid, const char* body, int bodylen)
{
    switch (client->cur_serv_cmd) {
    case act_cmd_check_session:
        return proc_pkg_from_serv_aft_check_session(
                client, ack_uid, body, bodylen);
    case db_cmd_get_attr:
        return proc_pkg_from_serv_aft_get_pre_online(
                client, ack_uid, body, bodylen);
    case sw_cmd_get_server_list:
        return proc_pkg_from_serv_aft_get_server_list(
                client, ack_uid, body, bodylen);
    default:
        return 0;
    }

    return 0;
}

int GetSvrListCmdProcessor::proc_errno_from_serv(
        client_info_t* client, int ret)
{
    switch(ret) {
    case 1107:
        ret = cli_err_account_frozen;
        break;
    case 1103:
        ret = cli_err_passwd_err;
        break;
    }

    return send_err_to_client(client, ret);
}

int GetSvrListCmdProcessor::proc_pkg_from_serv_aft_check_session(client_info_t* client, 
        userid_t ack_uid, const char* body, int bodylen)
{
    uint32_t *recommend = (uint32_t*)client->session;
    if (recommend) {
        db_in_.Clear();
        db_in_.add_type_list(kAttrLastOnlineId);
        return g_dbproxy->send_msg(client, client->userid, client->on_server_create_tm,
            db_cmd_get_attr, db_in_);
    }
    *recommend = 0;
    switchproto::cs_get_server_list sw_in;
    sw_in.set_server_type(commonproto::SERVER_TYPE_ONLINE);
    sw_in.set_idc_zone(g_server_config.idc_zone);
    sw_in.set_svr_recommend(false);

    if (g_servers_map.count(client->on_server_id) == 0) {
        return send_err_to_client(client, cli_err_server_not_exist);
    }
    const csvr_info_t &inf = (g_servers_map.find(client->on_server_id))->second;
    sw_in.set_my_server_id(inf.cur_svr_id);

    return g_dbproxy->send_msg(client, get_server_id(), 0,
            sw_cmd_get_server_list, sw_in, 0);
}

int GetSvrListCmdProcessor::proc_pkg_from_serv_aft_get_pre_online(
        client_info_t* client, userid_t ack_uid, const char* body, int bodylen)
{
    db_out_.Clear();
    if (parse_message(body, bodylen, &db_out_)) {
        return send_err_to_client(client, cli_err_sys_err);
    }

    uint32_t *pre_online_id = (uint32_t*)client->session;
    *pre_online_id = db_out_.attrs(0).value();
    switchproto::cs_get_server_list sw_in;
    sw_in.set_server_type(commonproto::SERVER_TYPE_ONLINE);
    sw_in.set_idc_zone(g_server_config.idc_zone);
    sw_in.set_svr_recommend(true);
    if (g_servers_map.count(client->on_server_id) == 0) {
        return send_err_to_client(client, cli_err_server_not_exist);
    }
    const csvr_info_t &inf = (g_servers_map.find(client->on_server_id))->second;
    sw_in.set_my_server_id(inf.cur_svr_id);

    return g_dbproxy->send_msg(client, get_server_id(), 0,
            sw_cmd_get_server_list, sw_in, 0);
}

int GetSvrListCmdProcessor::proc_pkg_from_serv_aft_get_server_list(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    switchproto::sc_get_server_list sw_out;
    if (parse_message(body, bodylen, &sw_out)) {
        return send_err_to_client(client, cli_err_sys_err); 
    }

    //统计拉取服务器列表成功
    get_stat_logger(client->on_server_id)->new_trans(StatLogger::bGetSrvlistSucc, to_string(client->userid));

    uint32_t *pre_online_id = (uint32_t*)client->session;
    cli_out_.Clear();
    cli_out_.mutable_online_svrs()->CopyFrom(sw_out.server_list());
    cli_out_.set_pre_online_id(*pre_online_id);

    return send_msg_to_client(client, client->cli_cmd, cli_out_);
}

int ActiveUserCmdProcessor::proc_pkg_from_client(client_info_t* client,
        const char* body, int bodylen)
{
    cli_in_.Clear();
    if (parse_message(body, bodylen, &cli_in_)) {
        return send_err_to_client(client, cli_err_proto_format_err); 
    }

    chnlhash32_t* hash = (chnlhash32_t *)(send_buf_);
    act_active_user_req_t* req = 
            (act_active_user_req_t *)(send_buf_ + sizeof(*hash));

    req->gameid = g_server_config.gameid;
    strncpy(req->active_code, cli_in_.active_code().c_str(), 
            sizeof(req->active_code));
    req->ip = client->fdsession->remote_ip;
    hex2bin(req->verify_session, 
            cli_in_.verify_image_session().substr(0, 32).c_str(), SESSION_LEN);
    strncpy(req->verify_code, cli_in_.verify_code().c_str(), 
            sizeof(req->verify_code));

    gen_chnlhash32(g_server_config.verifyid, g_server_config.security_code,
            (const char*)req, sizeof(*req), hash);

    return g_dbproxy->send_to_act(client, client->userid,
            act_cmd_active_user, send_buf_, sizeof(*req) + sizeof(*hash));
}

int ActiveUserCmdProcessor::proc_pkg_from_serv(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    act_active_user_ack_t* ack = (act_active_user_ack_t *)body;
    if (bodylen < (int)sizeof(*ack)) {
        KERROR_LOG(client->userid, "invalid active user ack body %d, expect %u",
                bodylen, sizeof(*ack));
        return send_err_to_client(client, cli_err_sys_err);
    }

    const char* ext_body = body + sizeof(*ack);
    int ext_bodylen = bodylen - sizeof(*ack);

    cli_out_.Clear();
    cli_out_.set_status((onlineproto::active_status_t)ack->flag);

    if (ack->flag != ACTIVE_ACK_FLAG_OK) {
        act_active_need_verify_image_ack_t* ext_ack = 
            (act_active_need_verify_image_ack_t *)ext_body;
        if (ext_bodylen < (int)sizeof(*ext_ack)) {
            ERROR_TLOG("u:%u invalid active verify image bodylen %d, expect %u",
                    client->userid, ext_bodylen, sizeof(*ext_ack));
            return send_err_to_client(client, cli_err_sys_err);
        }

        if (ext_bodylen != (int)(sizeof(*ext_ack) + ext_ack->image_size)) {
            ERROR_TLOG("u:%u invalid active verify image bodylen %d, expect %u",
                    client->userid, ext_bodylen, sizeof(*ext_ack) + ext_ack->image_size); 
            return send_err_to_client(client, cli_err_sys_err);
        }

        char session_hex[SESSION_LEN * 2 + 1] = {0}; 
        bin2hex_frm(session_hex, ext_ack->verify_image_session, 
                sizeof(ext_ack->verify_image_session), 0);

        cli_out_.set_verify_image_session(std::string(session_hex));
        cli_out_.set_verify_image(ext_ack->image, ext_ack->image_size);
    }

    return send_msg_to_client(client, client->cli_cmd, cli_out_);
}

int ActiveUserCmdProcessor::proc_errno_from_serv(
        client_info_t* client,
        int ret)
{
    if (ret == act_err_active_code_err) {
        return send_err_to_client(client, cli_err_wrong_active_code);
    } else {
        return send_err_to_client(client, ret); 
    }
}

int GetVerifyImageCmdProcessor::proc_pkg_from_client(client_info_t* client,
        const char* body, int bodylen)
{
    act_get_verify_image_req_t req;
    memset(&req, 0, sizeof(req));

    req.verify_id = g_server_config.verifyid;
    req.ip = client->fdsession->remote_ip;

    return g_dbproxy->send_to_act(client, client->userid,
            act_cmd_get_verify_image,
            (const char *)&req, sizeof(req));
}

int GetVerifyImageCmdProcessor::proc_pkg_from_serv(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    act_get_verify_image_ack_t* ack = 
        (act_get_verify_image_ack_t *)body;

    if (bodylen < (int)sizeof(*ack)) {
        ERROR_TLOG("%u recv invalid get verify image body len %d, expect %u",
                client->userid, bodylen, sizeof(*ack));
        return send_err_to_client(client, cli_err_sys_err);
    }

    if (ack->flag == ACT_LOGIN_ACK_FLAG_SUCC) {
        ERROR_TLOG("%u no image", client->userid);
        return send_err_to_client(client, cli_err_sys_err); 
    }

    if (bodylen != (int)(sizeof(*ack) + ack->image_size)) {
        ERROR_TLOG("%u recv invalid get verify image body len %d, expect %u",
                client->userid, bodylen, sizeof(*ack) + ack->image_size);
        return send_err_to_client(client, cli_err_sys_err);
    }

    cli_out_.Clear();

    char session_hex[SESSION_LEN * 2 + 1] = {0}; 
    bin2hex_frm(session_hex, ack->verify_image_session, 
            sizeof(ack->verify_image_session), 0);

    cli_out_.set_verify_image_session(std::string(session_hex));
    cli_out_.set_verify_image(ack->image, ack->image_size);

    return send_msg_to_client(client, client->cli_cmd, cli_out_);
}

int GetVerifyImageCmdProcessor::proc_errno_from_serv(
        client_info_t* client,
        int ret)
{
    return send_err_to_client(client, cli_err_sys_err);
}

int SessionLoginCmdProcessor::proc_pkg_from_client(client_info_t* client,
        const char* body, int bodylen)
{
    if (parse_message(body, bodylen, &cli_in_)) {
        return send_err_to_client(client, cli_err_proto_format_err); 
    }

    client->userid = cli_in_.userid();
    act_check_session_req_t req = {0};
    req.from_game = cli_in_.from_gameid();
    hex2bin(req.session, cli_in_.session().substr(0, 32).c_str(), SESSION_LEN); 
    req.del_flag = 1;
    req.to_game = g_server_config.gameid;
    req.ip = client->fdsession->remote_ip;
    req.enter_game = 0;
    req.region = g_server_config.idc_zone;
    req.tad[0] = '\0';

    write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "收到session登陆请求");
    client->on_server_id = cli_in_.server_id();

    session_login_session_t* session = 
        (session_login_session_t *)client->session;
    session->from_gameid = cli_in_.from_gameid();
    return g_dbproxy->send_to_act(client, client->userid,
            act_cmd_check_session, (const char*)&req, sizeof(req));
}

int SessionLoginCmdProcessor::proc_pkg_from_serv(client_info_t* client,
        userid_t ack_uid, const char* body, int bodylen)
{
    switch (client->cur_serv_cmd) {
    case act_cmd_check_session: 
        return proc_pkg_from_serv_aft_check_session(
                client, ack_uid, body, bodylen);
    case act_cmd_add_session:
        return proc_pkg_from_serv_aft_add_session(
                client, ack_uid, body, bodylen);
    case db_cmd_get_base_info:
        return proc_pkg_from_serv_aft_get_base_info(
                client, ack_uid, body, bodylen);
    }

    return 0;
}

int SessionLoginCmdProcessor::proc_pkg_from_serv_aft_check_session(
        client_info_t* client, userid_t ack_uid, const char* body, int bodylen)
{
    chnlhash32_t* hash = (chnlhash32_t *)send_buf_;
    act_add_session_req_t* req = (act_add_session_req_t *)(send_buf_ + sizeof(*hash));
    req->gameid = g_server_config.gameid;
    req->ip = client->fdsession->remote_ip;
    gen_chnlhash32(g_server_config.verifyid, g_server_config.security_code, 
            (const char*)req, sizeof(*req), hash);

    write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "session验证成功");
    client->userid = ack_uid;
    return g_dbproxy->send_to_act(client, client->userid, 
            act_cmd_add_session, send_buf_, sizeof(*req) + sizeof(*hash));
}

int SessionLoginCmdProcessor::proc_pkg_from_serv_aft_add_session(
        client_info_t* client, userid_t ack_uid, const char* body, int bodylen)
{
    act_add_session_ack_t* ack = 
        (act_add_session_ack_t *)body;

    if (bodylen != sizeof(*ack)) {
        ERROR_TLOG("%u invalid add session body len %d, expect %u",
                client->userid, bodylen, sizeof(*ack));
        return send_err_to_client(client, cli_err_sys_err); 
    }

    session_login_session_t* session =
        (session_login_session_t *)client->session;

    session->is_active = 1;
    memcpy(session->session, ack->session, SESSION_LEN);

    char session_hex[SESSION_LEN * 2 + 1] = {0};
    bin2hex_frm(session_hex, ack->session, 
            sizeof(ack->session), 0);
    memcpy(client->login_session, session_hex, sizeof(session_hex));
    if (session->is_active == false) {//没有激活
        cli_out_.Clear();
        cli_out_.set_status(onlineproto::LOGIN_STATUS_NOT_ACTIVE);
        cli_out_.set_session(client->login_session);
        return send_msg_to_client(client, client->cli_cmd, cli_out_);
    }
    db_in_.Clear();
    db_in_.set_server_id(client->on_server_id);
    return g_dbproxy->send_msg(client, client->userid, 0,
            db_cmd_get_base_info, db_in_);
}

int SessionLoginCmdProcessor::proc_pkg_from_serv_aft_get_base_info(
        client_info_t* client,
        userid_t ack_uid,
        const char* body,
        int bodylen)
{
    db_out_.Clear();
    if (parse_message(body, bodylen, &db_out_)) {
        return send_err_to_client(client, cli_err_sys_err);
    }

#if 0
    const commonproto::player_base_info_t& base_info = db_out_.base_info();
    // 检查用户是否被冻结
    uint32_t now = get_now_tv()->tv_sec;
    if (base_info.frozen_end_time() != 0 && base_info.frozen_end_time() >= now) {
        onlineproto::sc_0x0008_noti_act_frozen noti; 
        noti.set_type((commonproto::act_frozen_type_t)base_info.frozen_reason());
        noti.set_data(base_info.frozen_end_time() - now);
        send_msg_to_client(client, cli_cmd_cs_0x0008_noti_act_frozen, noti);
        send_to_client(client, client->cli_cmd, NULL, 0);
        write_msglog_new(client->userid, client->on_server_id, "用户监控", "session登陆", "角色被冻结");
        return 0;
    }
#endif
    return response_to_client(
            client, db_out_.mutable_base_info());
}

int SessionLoginCmdProcessor::response_to_client(client_info_t* client, 
        google::protobuf::RepeatedPtrField<commonproto::player_base_info_t> *base_info)
{
    session_login_session_t* session = 
        (session_login_session_t *)client->session;

    cli_out_.Clear();
    if (base_info) {
        for (int i = 0; i < base_info->size(); i++) {
            onlineproto::login_base_info_t *inf = cli_out_.add_base_infos();
            inf->mutable_base_info()->CopyFrom(base_info->Get(i));
            client->svr_create_tm_map->insert(make_pair(inf->base_info().server_id(), inf->base_info().create_tm()));

            dbproto::cs_set_attr db_in; 
            commonproto::attr_data_t* attr_data = db_in.add_attrs();
            attr_data->set_type(kAttrFromGameId);
            attr_data->set_value(session->from_gameid);
            g_dbproxy->send_msg(NULL, client->userid, inf->base_info().create_tm(),
                    db_cmd_set_attr, db_in);
        }
    }

    cli_out_.set_is_active(session->is_active);
    char session_hex[SESSION_LEN * 2 + 1] = {0};
    bin2hex_frm(session_hex, session->session,
            sizeof(session->session), 0);
    cli_out_.set_session(session_hex);
    cli_out_.set_server_id(client->on_server_id);
    write_msglog_new(client->userid, client->on_server_id, "用户监控", "登陆", "登陆成功");

    //设置初始序列号
    uint32_t ini_seq = get_now_tv()->tv_sec % ranged_random(1, 1000000);
    cli_out_.set_ini_seq(ini_seq);
    send_msg_to_client(client, client->cli_cmd, cli_out_);
    client->cli_seq = ini_seq;
    return 0;
}

int SessionLoginCmdProcessor::proc_errno_from_serv(client_info_t *client, int ret)
{
    if (client->cur_serv_cmd == db_cmd_get_base_info) {
        if (ret == db_err_user_not_find) {
            return response_to_client(client, NULL); 
        }
    } else if (client->cur_serv_cmd == act_cmd_check_session) {
        if (ret == act_err_check_session_failed) {
            return send_err_to_client(client, cli_err_check_session_failed); 
        }
    }

    return send_err_to_client(client, ret); 
}

int ChooseServerCmdProcessor::proc_pkg_from_client(
        client_info_t *client, const char *body, int bodylen)
{
    cli_in_.Clear();
    if (parse_message(body, bodylen, &cli_in_)) {
        return send_err_to_client(client, cli_err_proto_format_err);
    }
    if (g_servers_map.count(cli_in_.server_id()) == 0) {
        return send_err_to_client(client, cli_err_server_not_exist);
    }
    client->on_server_id = cli_in_.server_id();
    db_gbi_in_.Clear();
    db_gbi_in_.set_server_id(cli_in_.server_id());
    return g_dbproxy->send_msg(client, client->userid, 0, 
            db_cmd_get_base_info, db_gbi_in_);
}

int ChooseServerCmdProcessor::proc_pkg_from_serv(
        client_info_t *client, userid_t ack_uid, const char *body, int bodylen)
{
    db_gbi_out_.Clear();
    if (parse_message(body, bodylen, &db_gbi_out_)) {
        return send_err_to_client(client, cli_err_sys_err); 
    }
    cli_out_.Clear();
    cli_out_.set_server_id(client->on_server_id);
    for (int i = 0; i < db_gbi_out_.base_info_size(); i++) {
        cli_out_.mutable_base_info()->mutable_base_info()->CopyFrom(db_gbi_out_.base_info(i));
        break;
    }
    return send_msg_to_client(client, client->cli_cmd, cli_out_);
}
