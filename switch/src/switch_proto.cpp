#include "player.h"
#include "service.h"
#include "server_manager.h"
#include "player_manager.h"
#include "switch.h"
#include "switch_proto.h"
#include "proto/switch/switch.pb.h"
#include "proto/db/dbproto.attr.pb.h"
#include "proto/db/db_cmd.h"
#include "proto/db/dbproto.transaction.pb.h"
#include "proto/db/dbproto.base_info.pb.h"
#include "proto/client/pb0x01.pb.h"
#include "statlogger/statlogger.h"

static std::string to_string(uint32_t n)
{
    static char hex_buf[64];
    snprintf(hex_buf, sizeof(hex_buf), "%u", n);
    return std::string(hex_buf);
}

static inline bool verify_passwd(const char *passwd)
{
    return true;
}

static inline bool wanted_server_type(server_t *svr, uint32_t type)
{
    if (type == commonproto::SERVER_TYPE_ALL) return true;

    if (type == svr->server_type()) return true;

    return false;
}

void pack_server_list(uint32_t server_type, uint32_t server_id, 
        switchproto::sc_get_server_list &msg, uint32_t idc_zone, bool recommend)
{
    msg.clear_server_list();
    server_t *svr;
    const server_manager_t::svrid_to_server_map_t &s_map = SERVER_MGR.all_server_map();
    std::multimap<uint32_t, server_t*> tmp_svr_map;
    uint32_t limit = 50;
    uint32_t add = 50;
    uint32_t try_cnt = 0;
    // TODO toby confirm limit num
    while (tmp_svr_map.empty() && try_cnt < 20) {
        server_manager_t::svrid_to_server_map_const_iter_t it = s_map.begin();
        for (; it != s_map.end(); it++) {
            svr = it->second;
            if (!wanted_server_type(svr, server_type)) {
                continue;
            }

            if (svr->server_id() != server_id) {
                continue;
            }

            if (idc_zone != svr->idc_zone()) {
                continue;
            }
            if (recommend && svr->get_total_player_num() >= limit + try_cnt*add) {
                continue;
            }
            tmp_svr_map.insert(make_pair(svr->get_total_player_num(), svr));
        }
        try_cnt++;
    }
    REVERSE_FOREACH(tmp_svr_map, it) {
        /*
        struct in_addr in;
        in = inet_makeaddr(AF_INET, svr->fdsess()->remote_ip);
        char *ipaddr = inet_ntoa(in);
        */
        svr = it->second;
        commonproto::svr_info_t *info = msg.add_server_list();
        info->set_svr_id(svr->server_id());
        info->set_online_id(svr->online_id());
        info->set_ip(svr->host_ip());
        info->set_port(svr->listen_port());
        info->set_type((commonproto::server_type_t)svr->server_type());
        info->set_total_user_num(svr->get_total_player_num());
        info->set_vip_user_num(svr->get_total_vip_player_num());
    }
    return;
}

//向服务器发送T人通知
int send_kick_noti_to_server(server_t *svr, uint32_t uid, int32_t create_tm)
{
    assert(svr);
    switchproto::sc_sw_notify_kick_player_off kick_noti;
    switchproto::sw_player_basic_info_t *tmp = kick_noti.mutable_basic();
    tmp->set_userid(uid);
    tmp->set_create_tm(create_tm);
    tmp->set_is_vip(0);
    return send_msg_to_server(svr, sw_cmd_sw_notify_kick_player_off, kick_noti, DONT_CLEAR_CMD);
}


////////////协议部分/////////////////////
int TransmitMsgCmdProcessor::proc_pkg_from_client(
        server_t *svr, const char *body, int bodylen) 
{
    if (parse_message(body, bodylen, &in_)) {
        return send_err_to_server(svr, 
                svr->waiting_cmd(), sw_err_proto_format_err);
    }
    out_.Clear();

    player_t *p = 0;
    server_t *dst_svr = 0;
    uint32_t transmit_type = in_.transmit_type();
    out_.set_transmit_type(transmit_type);
    out_.set_cmd(in_.cmd());
    out_.set_pkg(in_.pkg());

    //第三方通过switch传递系统消息
    if (transmit_type == switchproto::SWITCH_TRANSMIT_SYSNOTI) {
        if (!in_.has_passwd() || !verify_passwd(in_.passwd().c_str())) {
            return send_err_to_server(svr, svr->waiting_cmd(), sw_err_wrong_passwd);
        }
		
		if (in_.servers_size()) {
			for (int i = 0; i < in_.servers_size(); i++) {
				const commonproto::svr_info_t &s_info = in_.servers(i);
				uint32_t ret = SERVER_MGR.transmit_msg_to_dest_svr(s_info.svr_id(),
						sw_cmd_sw_transmit_only, out_);
				if (ret) {
					return send_err_to_server(svr, svr->waiting_cmd(), ret);
				}
			}
		} else {
			server_manager_t::svrid_to_server_map_const_iter_t it;
			it = SERVER_MGR.all_server_map().begin();
			for(; it != SERVER_MGR.all_server_map().end(); it++) {
				dst_svr = it->second;
				send_msg_to_server(dst_svr, sw_cmd_sw_transmit_only, out_, DONT_CLEAR_CMD);
			}
		}
    } else if (transmit_type == switchproto::SWITCH_TRANSMIT_USERS) {
        for (int i = 0; i < in_.receivers_size(); i++) {
            out_.clear_receivers();
            const switchproto::sw_player_basic_info_t &player = in_.receivers(i);
            p = PLAYER_MGR.get_player_by_uid(player.userid());
            if (!p) {
                send_err_to_server(svr, svr->waiting_cmd(), sw_err_player_not_exist, player.userid());
                continue;
            }
            dst_svr = SERVER_MGR.get_server_by_olid(p->online_id());
            if (!dst_svr) {
                send_err_to_server(svr, svr->waiting_cmd(), sw_err_server_not_exist);
                continue;
            }

            //TODO(singku) 目前只在online之间转发消息
            if (dst_svr->server_type() != commonproto::SERVER_TYPE_ONLINE) {
                send_err_to_server(svr, svr->waiting_cmd(), sw_err_server_type_invalid);
                continue;
            }
            
            if (dst_svr == svr) {
                WARN_TLOG("SW_TRASMIT_ONLY_PKG_TO_SELF svr:%u ol:%u", 
                        svr->server_id(), svr->online_id());
                continue;
            }
            switchproto::sw_player_basic_info_t *tmp = out_.add_receivers();
            *tmp = in_.receivers(i);
            send_msg_to_server(dst_svr, sw_cmd_sw_transmit_only, out_, DONT_CLEAR_CMD);
        }
    } else if (transmit_type == switchproto::SWITCH_TRANSMIT_SERVERS) {
        for (int i = 0; i < in_.servers_size(); i++) {
            const commonproto::svr_info_t &s_info = in_.servers(i);
			uint32_t ret = SERVER_MGR.transmit_msg_to_dest_svr(s_info.svr_id(),
					sw_cmd_sw_transmit_only, out_);
			if (ret) {
				return send_err_to_server(svr, svr->waiting_cmd(), ret);
			}
        }
    } else if (transmit_type == switchproto::SWITCH_TRANSMIT_WORLD) {
        server_manager_t::svrid_to_server_map_const_iter_t it;
        it = SERVER_MGR.all_server_map().begin();
        for(; it != SERVER_MGR.all_server_map().end(); it++) {
            dst_svr = it->second;
            send_msg_to_server(dst_svr, sw_cmd_sw_transmit_only, out_, DONT_CLEAR_CMD);
        }
    } else {
        ERROR_TLOG("TRANSMIT_MSG:UNKNOW TRANSMIT_TYPE: svr:%u, ol:%u, type:%d", 
                svr->server_id(), svr->online_id(), transmit_type);
        send_err_to_server(svr, svr->waiting_cmd(), sw_err_invalid_transmit_type);
    }
    svr->clear_waiting_cmd();
    return 0;
}

int ServerRegCmdProcessor::proc_pkg_from_client(
        server_t *svr, const char *body, int bodylen)
{
    out_.Clear();
    return send_msg_to_server(svr, svr->waiting_cmd(), out_);
}

int OnlineSyncPlayerInfoCmdProcessor::proc_pkg_from_client(
        server_t *svr, const char *body, int bodylen)
{
    if (parse_message(body, bodylen, &in_)) {
        return send_err_to_server(svr, svr->waiting_cmd(), 
                sw_err_proto_format_err);
    }

    uint32_t uid = 0;
    uint32_t olid = 0;
    player_t *p = 0;

    olid = svr->online_id();

    for (int32_t i = 0; i < in_.player_list_size(); i++) {
        const switchproto::sw_player_basic_info_t &pinfo = in_.player_list(i);
        uid = pinfo.userid();
        if (!uid) {
            WARN_TLOG("BUG!!!online report invaild uid:0 to switch");
            continue;
        }
        p = PLAYER_MGR.get_player_by_uid(uid);
        if (p && p->online_id() == olid) {
            //p存在且是同一svr的重复报告
            continue;
        } else if (p && p->online_id() != olid) {
            //p已经存在但不在该svr上 这种情况只会发生在server断开后,其他server
            //上报了p的信息, 所以该server的p应该是先登录的 应该要被T掉的.
            send_kick_noti_to_server(svr, uid, pinfo.create_tm());
        } else {//p不存在
            player_basic_t basic;
            prepare_player_basic(pinfo, basic);
            p = PLAYER_MGR.create_new_player(&basic);
            PLAYER_MGR.add_player(p, svr->server_id(), olid);
        }
    }//for   

    svr->clear_waiting_cmd();
    return 0;
}

int OnlineReportPlayerStateCmdProcessor::proc_pkg_from_client(
        server_t *svr, const char *body, int bodylen)
{
    if (parse_message(body, bodylen, &in_)) {
        return send_err_to_server(svr, svr->waiting_cmd(), 
                sw_err_proto_format_err);
    }

    uint8_t login_or_logout = in_.login_or_logout();
    player_basic_t basic;
    prepare_player_basic(in_.basic(), basic);

    player_t *p = PLAYER_MGR.get_player_by_uid(in_.basic().userid());

    if (!p && login_or_logout == switchproto::PLAYER_LOGOUT) {
        WARN_TLOG("BAD CMD: svr[id:%u ol:%u] report player[%u] log out, but player not exist", 
                svr->server_id(), svr->online_id(), in_.basic().userid());
    } else if (p && login_or_logout == switchproto::PLAYER_LOGIN) {
        if (p->online_id() != svr->online_id()) {//从p->online_id T掉
            //发T人包到p->online_id;
            server_t *kick = SERVER_MGR.get_server_by_olid(p->online_id());
            send_kick_noti_to_server(kick, p->uid(), p->create_tm());
            //删除p的对象
            PLAYER_MGR.del_player(p);
            //创建新的p对象
            p = PLAYER_MGR.create_new_player(&basic);
            //加到本svr
            PLAYER_MGR.add_player(p, svr->server_id(), svr->online_id());
        } else {//重复报告登录
            WARN_TLOG("DUP REPORT PLAYER LOGIN svr:%u, ol:%u, player:%u", 
                    p->server_id(), p->online_id(), p->uid());
        }
    } else if (!p && login_or_logout == switchproto::PLAYER_LOGIN) {
        //创建
        p = PLAYER_MGR.create_new_player(&basic);
        PLAYER_MGR.add_player(p, svr->server_id(), svr->online_id());
    } else if (p->create_tm() == in_.basic().create_tm()) {//p && LOGOUT && 同服 删除
        PLAYER_MGR.del_player(p);
    }

    svr->clear_waiting_cmd();
    return 0;

}

int GetSvrListCmdProcessor::proc_pkg_from_client(
        server_t *svr, const char *body, int bodylen)
{
    if (parse_message(body, bodylen, &in_)) {
        return send_err_to_server(svr, svr->waiting_cmd(), 
                sw_err_proto_format_err);
    }
    out_.Clear();

    pack_server_list(in_.server_type(), in_.my_server_id(), 
            out_, in_.idc_zone(), in_.svr_recommend());
    return send_msg_to_server(svr, svr->waiting_cmd(), out_);
}

int IsPlayerOnlineCmdProcessor::proc_pkg_from_client(
		server_t *svr, const char *body, int bodylen)
{
    if (parse_message(body, bodylen, &in_)) {
        return send_err_to_server(svr, svr->waiting_cmd(), 
                sw_err_proto_format_err);
    }
    out_.Clear();
	for (int i = 0; i < in_.ol_info_size(); i++) {
		uint32_t userid = in_.ol_info(i).userid();
		player_t *p = PLAYER_MGR.get_player_by_uid(userid);
		switchproto::sw_player_online_info_t* online_info = out_.add_ol_info();
		online_info->set_userid(userid);
		online_info->set_team(in_.ol_info(i).team());
        online_info->set_u_create_tm(in_.ol_info(i).u_create_tm());
		if (p == NULL || (uint32_t)p->create_tm() != in_.ol_info(i).u_create_tm()) {
			online_info->set_is_online(0);
		}
		else {
			online_info->set_is_online(1);
		}
		
	}
	
    return send_msg_to_server(svr, svr->waiting_cmd(), out_);
}

int GetUseridListCmdProcessor::proc_pkg_from_client(
		server_t *svr, const char *body, int bodylen)
{
	if (parse_message(body, bodylen, &in_)) {
		return send_err_to_server(svr, svr->waiting_cmd(), 
				sw_err_proto_format_err);
	}

	std::vector<role_info_t> user_list;
	PLAYER_MGR.get_player_list(user_list, in_.server_id());
    out_.Clear();
	for (uint32_t i = 0; i < user_list.size(); i++) {
        commonproto::role_info_t *inf = out_.add_users();
        inf->set_userid(user_list[i].userid);
        inf->set_u_create_tm(user_list[i].u_create_tm);
	}
    return send_msg_to_server(svr, svr->waiting_cmd(), out_);
}

int EraseEscortInfoCmdProcessor::proc_pkg_from_client(
		server_t *svr, const char *body, int bodylen)
{
    if (parse_message(body, bodylen, &req_in_)) {
        return send_err_to_server(svr, svr->waiting_cmd(), 
                sw_err_proto_format_err);
    }
	server_t *dst_svr = 0;
	dst_svr = SERVER_MGR.get_server_by_olid(req_in_.old_online_id());
	if (!dst_svr) {
		ERROR_TLOG("svr not exist;ol_id=[%u]", req_in_.old_online_id());
		return send_err_to_server(svr, svr->waiting_cmd(), sw_err_server_not_exist);
	}
    svr->clear_waiting_cmd();
	noti_out_.Clear();
	noti_out_.set_uid(req_in_.uid());
    noti_out_.set_u_create_tm(req_in_.u_create_tm());
	return send_msg_to_server(dst_svr, sw_cmd_sw_notify_erase_player_escort_info, 
            noti_out_, DONT_CLEAR_WAITING_CMD);
}

int ChangeOtherAttrCmdProcessor::proc_pkg_from_client(
	server_t *svr, const char *body, int bodylen)
{
    if (parse_message(body, bodylen, &req_in_)) {
        return send_err_to_server(svr, svr->waiting_cmd(), 
                sw_err_proto_format_err);
    }
	svr->clear_waiting_cmd();
	player_t *p = 0;
	p = PLAYER_MGR.get_player_by_uid(req_in_.uid());
	if (!p || (uint32_t)p->create_tm() != req_in_.u_create_tm()) {//玩家不在线, 直接修改db
		dbproto::cs_change_attr_value db_msg;
		db_msg.set_type(req_in_.attr_list(0).type());
		int change_value = 0;
		if (!req_in_.attr_list(0).is_minus()) {
			change_value = req_in_.attr_list(0).change_value();
		} else {
			change_value = -req_in_.attr_list(0).change_value();
		}
		db_msg.set_change(change_value);
		db_msg.set_max_value(req_in_.attr_list(0).max_value());
		db_msg.set_is_minus(req_in_.attr_list(0).is_minus());
		return g_dbproxy->send_msg(
				NULL, req_in_.uid(), req_in_.u_create_tm(), 
                db_cmd_change_attr_value, db_msg, false);
	}
    //玩家在线
	server_t *dst_svr = SERVER_MGR.get_server_by_olid(p->online_id());
	if (!dst_svr) {
		ERROR_TLOG("svr not exist, ol_id=[%u]", p->online_id());
		return send_err_to_server(svr, svr->waiting_cmd(), sw_err_server_not_exist);
	}
	noti_out_.Clear();
	noti_out_.mutable_attr_list()->CopyFrom(req_in_.attr_list());
	noti_out_.set_uid(req_in_.uid());
	noti_out_.set_u_create_tm(req_in_.u_create_tm());
	return send_msg_to_server(
			dst_svr, sw_cmd_sw_notify_attr_changed_by_other, 
			noti_out_, DONT_CLEAR_WAITING_CMD);
}

//GM部分
int GmNewMailCmdProcessor::proc_pkg_from_client(
		server_t *svr, const char *body, int bodylen)
{
    PARSE_MSG;

    in_.mutable_mail_data()->set_mailid(gen_uuid());
    in_.mutable_mail_data()->set_send_time(NOW());
	in_.mutable_mail_data()->set_status(commonproto::UNREAD);

    dbproto::cs_mail_new db_in;
    db_in.mutable_mail_data()->CopyFrom(in_.mail_data());

    return g_dbproxy->send_msg(svr, in_.userid(), in_.u_create_tm(),
            db_cmd_mail_new, db_in, true);
}

int GmNewMailCmdProcessor::proc_pkg_from_serv(
	server_t *svr, const char* body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);

    player_t *p = PLAYER_MGR.get_player_by_uid(svr->process_uid);
	//若玩家在线 则通知
	if (p && (uint32_t)p->create_tm() == svr->process_u_create_tm) {
        switchproto::sc_sw_notify_new_mail noti_msg;
        noti_msg.set_userid(p->uid());
        noti_msg.set_u_create_tm(p->create_tm());
        noti_msg.mutable_mail_data()->CopyFrom(db_out_.mail_data());
		server_t *dst_svr = SERVER_MGR.get_server_by_olid(p->online_id());
		if (dst_svr) {
			send_msg_to_server(dst_svr,
					sw_cmd_sw_notify_new_mail, 
					noti_msg, DONT_CLEAR_WAITING_CMD);
		}
	}

	//是否等回调
	bool wait_cmd = in_.wait_cmd();

	if(!wait_cmd){
        svr->clear_waiting_cmd();
		return 0;
	}

    //给客服回包
    out_.Clear();
    out_.set_userid(svr->process_uid);
    out_.set_u_create_tm(svr->process_u_create_tm);
    out_.mutable_mail_data()->CopyFrom(db_out_.mail_data());
    return send_msg_to_server(svr, svr->waiting_cmd(), out_);
}

int GmNewMailToSvrCmdProcessor::proc_pkg_from_client(
		server_t *svr, const char *body, int bodylen)
{
    PARSE_MSG;
    in_.mutable_mail_data()->set_mailid(gen_uuid());
    in_.mutable_mail_data()->set_send_time(NOW());
	in_.mutable_mail_data()->set_status(commonproto::UNREAD);

	switchproto::sc_sw_notify_new_mail noti_out;
	noti_out.mutable_mail_data()->CopyFrom(in_.mail_data());

	uint32_t svr_ids_size = in_.svr_ids_size();
	//若为0,则说明是全服
	if (svr_ids_size == 0) {
		FOREACH(SERVER_MGR.all_server_map(), it) {
			server_t* dst_svr = it->second;
			send_msg_to_server(dst_svr, 
					sw_cmd_sw_notify_new_mail_to_svr,
					noti_out, DONT_CLEAR_CMD);
		}
	} else {
		//若不为0，则说明是指定的服发放
		for (uint32_t i = 0; i < svr_ids_size; ++i) {
			uint32_t ret = SERVER_MGR.transmit_msg_to_dest_svr(in_.svr_ids(i),
					sw_cmd_sw_notify_new_mail_to_svr, noti_out);
			if (ret) {
				send_err_to_server(svr, svr->waiting_cmd(), ret);
			}
		}
	}
    return send_msg_to_server(svr, svr->waiting_cmd(), out_);
}

int FrozenAccountCmdProcessor::proc_pkg_from_client(
		server_t *svr, const char *body, int bodylen)
{
    PARSE_MSG;
	//TODO kevin 确认封号最大时长 10年
	const uint32_t MAX_DUR = 315360000;
	if (in_.frozen_reason() != commonproto::ACT_NONE && 
			in_.unfreeze_reason() != commonproto::ACT_UNFREEZE_NONE) {
		return send_err_to_server(svr, svr->waiting_cmd(),
			sw_err_frozen_ope_err);
	}

	dbproto::cs_set_attr db_in;
	commonproto::attr_data_t *attr_ptr;
	frozen_end_time = NOW() + in_.dur();
	if (frozen_end_time > MAX_DUR + NOW()) {
		frozen_end_time = MAX_DUR + NOW();
	}
	attr_ptr = db_in.add_attrs();
	attr_ptr->set_type(kAttrFrozenEndTime);
	attr_ptr->set_value(frozen_end_time);
	attr_ptr = db_in.add_attrs();
	attr_ptr->set_type(kAttrFrozenReason);
	attr_ptr->set_value(in_.frozen_reason());
	attr_ptr = db_in.add_attrs();
	attr_ptr->set_type(kAttrFrozenType);
	attr_ptr->set_value(in_.unfreeze_reason());
	attr_ptr = db_in.add_attrs();
	attr_ptr->set_type(kAttrFrozenTmType);
	attr_ptr->set_value(in_.frozen_tm_type());
	return g_dbproxy->send_msg(svr, in_.userid(), in_.u_create_tm(),
			db_cmd_set_attr, db_in, true);
}

int FrozenAccountCmdProcessor::proc_pkg_from_serv(
		server_t *svr, const char *body, int bodylen)
{
	switch (svr->serv_cmd()) {
		case db_cmd_set_attr:
			return proc_pkg_from_change_other_attr(svr, body, bodylen);
		default:
			return 0;
	}
	return 0;
}

int FrozenAccountCmdProcessor::proc_pkg_from_change_other_attr(
		server_t *svr, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);
	player_t *p = PLAYER_MGR.get_player_by_uid(svr->process_uid);
	//若玩家在线，则通知
	if (p && (uint32_t)p->create_tm() == svr->process_u_create_tm) {
		switchproto::sc_sw_notify_player_frozen_account sw_out_;
		sw_out_.set_userid(p->uid());
		sw_out_.set_u_create_tm(p->create_tm());
		sw_out_.set_dur(in_.dur());
		sw_out_.set_frozen_reason(in_.frozen_reason());
		sw_out_.set_unfreeze_reason(in_.unfreeze_reason());
		sw_out_.set_frozen_tm_type(in_.frozen_tm_type());
		server_t* dst_svr = SERVER_MGR.get_server_by_olid(p->online_id());
		if (dst_svr) {
			send_msg_to_server(
					dst_svr, sw_cmd_sw_notify_player_frozen_account, 
					sw_out_, DONT_CLEAR_WAITING_CMD);
		}
	}
	out_.Clear();
	out_.set_userid(svr->process_uid);
    out_.set_u_create_tm(svr->process_u_create_tm);
	out_.set_dur(in_.dur());
	out_.set_frozen_reason(in_.frozen_reason());
	out_.set_unfreeze_reason(in_.unfreeze_reason());
	out_.set_frozen_tm_type(in_.frozen_tm_type());
	return send_msg_to_server(svr, svr->waiting_cmd(), out_);	
}

int ActRechargeDiamondCmdProcessor::proc_pkg_from_client(
		server_t *svr, const char* body, int bodylen)
{
	platform_recharge_diamond_req_t* req_body = 
		(platform_recharge_diamond_req_t*)body;
	if (bodylen < (int)sizeof(body)) {
		ERROR_TLOG("too small ack body %d, expect %lu",
				bodylen, sizeof(body));
		return send_err_to_server(svr, 
				svr->waiting_cmd(), sw_err_proto_format_err);
	}
	TRACE_TLOG("PlatFrom Recharge Diamond:uid:%u, svr_id:%u, diamond_cnt:%u,trans_id:%u", 
			req_body->dest_user, req_body->dest_server_id, req_body->diamond_cnt, req_body->trans_id);

	dbproto::cs_get_base_info db_in;
	if (req_body->dest_server_id == 0) {
		return send_err_to_server(svr,
			svr->waiting_cmd(), sw_err_must_give_svr_id);
	}
	in_.Clear();
	in_.set_dest_userid(req_body->dest_user);
	in_.set_diamond_cnt(req_body->diamond_cnt / 100);
	in_.set_trans_id(req_body->trans_id);
	in_.set_server_id(req_body->dest_server_id);
	svr->cache_string.clear();
	in_.SerializeToString(&(svr->cache_string));

	db_in.set_server_id(req_body->dest_server_id);
	return g_dbproxy->send_msg(
			svr, req_body->dest_user, 0,
			db_cmd_get_base_info,
			db_in);
}

int ActRechargeDiamondCmdProcessor::proc_pkg_from_serv(
		server_t *svr, const char* body, int bodylen)
{
	switch(svr->serv_cmd()) {
		case db_cmd_get_base_info:
			return proc_pkg_from_svr_after_get_base_info(svr, body, bodylen);
		case db_cmd_new_transaction:
			return proc_pkg_from_svr_after_insert_new_trans(svr, body, bodylen);
		case db_cmd_change_attr_value:
			return proc_pkg_from_svr_after_recharge_diamond(svr, body, bodylen);
		default:
			return 0;
	}
	return 0;
}

int ActRechargeDiamondCmdProcessor::proc_pkg_from_svr_after_get_base_info(
		server_t* svr, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	uint32_t base_info_size = db_out_.base_info_size();
	uint32_t create_tm = 0;
	if (!base_info_size) {
		return send_err_to_server(svr, svr->waiting_cmd(), sw_not_role_info);
    }
    const commonproto::player_base_info_t& pb_inf = db_out_.base_info(0);
    create_tm = pb_inf.create_tm();

    //落充值日志
    bool is_vip = false;
    if (pb_inf.gold_vip_end_time() > NOW() || pb_inf.silver_vip_end_time() > NOW()) {
        is_vip = true;
    }
    //get_stat_logger(pb_inf.server_id())->pay(to_string(in_.dest_userid()), is_vip, in_.diamond_cnt() / 10 *100,
      //      StatLogger::ccy_mibi, StatLogger::pay_buy, to_string(334400), in_.diamond_cnt(), to_string(1));

	dbproto::cs_new_transaction trans_msg;
	dbproto::transaction_info_t* tran_ptr = trans_msg.mutable_info();
	tran_ptr->set_transaction_time(NOW());
	tran_ptr->set_server_no(in_.server_id());
	tran_ptr->set_account_id(in_.dest_userid());
	tran_ptr->set_s_create_tm(create_tm);
	tran_ptr->set_dest_account_id(in_.dest_userid());
	tran_ptr->set_d_create_tm(create_tm);
	tran_ptr->set_channel_id(dbproto::CHANNEL_TYPE_DEPOSIT);
	tran_ptr->set_pay_gate_trans_id(in_.trans_id());
	tran_ptr->set_product_id(0);
	tran_ptr->set_product_type(dbproto::PRODUCT_TYPE_FOREVER);
	tran_ptr->set_product_duration(0);
	tran_ptr->set_product_count(1);
    tran_ptr->set_money_num(in_.diamond_cnt() * 100);
	tran_ptr->set_result(0);
	return g_dbproxy->send_msg(svr, in_.dest_userid(), 
			create_tm,
			db_cmd_new_transaction, trans_msg);
}

int ActRechargeDiamondCmdProcessor::proc_pkg_from_svr_after_insert_new_trans(
		server_t *svr, const char* body, int bodylen)
{
	//向数据库充值钻石，需要等返回
	if (svr) {
		in_.ParseFromString(svr->cache_string);
	}
	dbproto::cs_change_attr_value db_msg;
	db_msg.set_type(kAttrPaidDiamond);
	db_msg.set_change(in_.diamond_cnt());
	db_msg.set_is_minus(false);
	db_msg.set_max_value(0xFFFFFFFF);
	db_msg.set_get_new_val(true);
	return g_dbproxy->send_msg(
			svr, in_.dest_userid(), svr->process_u_create_tm, 
			db_cmd_change_attr_value, db_msg);
}

int ActRechargeDiamondCmdProcessor::proc_pkg_from_svr_after_recharge_diamond(
		server_t *svr, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_chg_value_);
	//能够到达这里，说明充值钻石操作数据库成功

	player_t* p = NULL;
	p = PLAYER_MGR.get_player_by_uid(svr->process_uid);
	if (p && (uint32_t)p->create_tm() == svr->process_u_create_tm) {
		server_t *dst_svr = SERVER_MGR.get_server_by_olid(p->online_id());
		switchproto::sc_sw_only_notify_attr_changed noti_out;
		commonproto::attr_data_t* ptr = noti_out.add_attr();
		ptr->set_type(kAttrPaidDiamond);
		ptr->set_value(db_chg_value_.new_value());
		noti_out.set_uid(p->uid());
		noti_out.set_u_create_tm(p->create_tm());

		if (dst_svr) {
			send_msg_to_server(
					dst_svr, sw_cmd_sw_only_notify_player_attr_changed, 
					noti_out, DONT_CLEAR_WAITING_CMD);
		}
	} else {//如果离线
		//更新最近充值的时间戳
		dbproto::cs_set_attr db_in;
		commonproto::attr_data_t* attr_ptr = db_in.add_attrs();
		attr_ptr->set_type(kAttrFirstRechargeFlag);
		attr_ptr->set_value(NOW());
		g_dbproxy->send_msg(0, svr->process_uid,
				svr->process_u_create_tm,
				db_cmd_set_attr, db_in, false);

        dbproto::cs_change_attr_value db_chg;
        db_chg.set_type(kAttrChargeDiamondDrawPrizeChargeCnt);
        db_chg.set_change(in_.diamond_cnt());
        db_chg.set_max_value(0xFFFFFFFF);
		g_dbproxy->send_msg(0, svr->process_uid,
				svr->process_u_create_tm,
				db_cmd_change_attr_value, db_chg, false);

		//更新充值榜数据
        db_chg.Clear();
        db_chg.set_type(kAttrDiamondRechargeRankingCount);
        db_chg.set_change(in_.diamond_cnt());
        db_chg.set_max_value(0xFFFFFFFF);
		g_dbproxy->send_msg(0, svr->process_uid,
				svr->process_u_create_tm,
				db_cmd_change_attr_value, db_chg, false);

        // 更新每日充值记录
        db_chg.Clear();
		db_chg.set_type(kDailyChargeDiamondCnt);
		db_chg.set_change(in_.diamond_cnt());
        db_chg.set_max_value(0xFFFFFFFF);
		g_dbproxy->send_msg(0, svr->process_uid,
				svr->process_u_create_tm,
				db_cmd_change_attr_value, db_chg, false);

		//Comfirm kevin获得范围内充值次数
		db_chg.Clear();
		db_chg.set_type(kAttrActivRechargeDiamondTimes);
		db_chg.set_change(1);
        db_chg.set_max_value(0xFFFFFFFF);
		g_dbproxy->send_msg(0, svr->process_uid,
				svr->process_u_create_tm,
				db_cmd_change_attr_value, db_chg, false);

		db_chg.Clear();
		db_chg.set_type(kAttrActivRechargeDiamondCnt);
		db_chg.set_change(in_.diamond_cnt());
        db_chg.set_max_value(0xFFFFFFFF);
		g_dbproxy->send_msg(0, svr->process_uid,
				svr->process_u_create_tm,
				db_cmd_change_attr_value, db_chg, false);

		db_chg.Clear();
		db_chg.set_type(kAttrChargeDiamondGetGiftChargeCnt);
		db_chg.set_change(in_.diamond_cnt());
        db_chg.set_max_value(0xFFFFFFFF);
		g_dbproxy->send_msg(0, svr->process_uid,
				svr->process_u_create_tm,
				db_cmd_change_attr_value, db_chg, false);

#if 0   //取消了
        // 更新充值礼记录
        db_chg.Clear();
		db_chg.set_type(kAttrChargeDiamondCnt);
		db_chg.set_change(in_.diamond_cnt());
        db_chg.set_max_value(0xFFFFFFFF);
		g_dbproxy->send_msg(0, svr->process_uid,
				svr->process_u_create_tm,
				db_cmd_change_attr_value, db_chg, false);
#endif
	}
	
	platform_recharge_diamond_ack_t ack_body;
	ack_body.trans_id = in_.trans_id();
	return send_msg_to_act(
			svr, svr->waiting_cmd(), 
			(const char *)(&ack_body),
			sizeof(ack_body));
}

/*
int ActRechargeDiamondCmdProcessor::proc_pkg_from_serv(
		server_t *svr, const char* body, int bodylen)
{
	switchproto::platform_recharge_diamond tmp_pb;
	player_t* p = NULL;
	p = PLAYER_MGR.get_player_by_uid(svr->process_uid);
	if (p && (uint32_t)p->create_tm() == svr->process_u_create_tm) {
		server_t *dst_svr = SERVER_MGR.get_server_by_olid(p->online_id());
		noti_out_.Clear();
		noti_out_.set_uid(tmp_pb.dest_userid());
		noti_out_.set_u_create_tm(tmp_pb.dest_create_tm());
		commonproto::attr_data_change_t* pb_ptr = noti_out_.add_attr_list();
		pb_ptr->set_type(kAttrPaidDiamond);
		pb_ptr->set_change_value(tmp_pb.diamond_cnt());
		pb_ptr->set_is_minus(false);
		if (dst_svr) {
			send_msg_to_server(
					dst_svr, sw_cmd_sw_notify_attr_changed_by_other, 
					noti_out_, DONT_CLEAR_WAITING_CMD);
		}
	} else {
		dbproto::cs_change_attr_value db_msg;
		db_msg.set_type(kAttrPaidDiamond);
		db_msg.set_change(tmp_pb.diamond_cnt());
		db_msg.set_is_minus(false);
		g_dbproxy->send_msg(
				NULL, tmp_pb.dest_userid(), tmp_pb.dest_create_tm(), 
				db_cmd_change_attr_value, db_msg, true);
	}
	platform_recharge_diamond_ack_t ack_body;
	ack_body.trans_id = tmp_pb.trans_id();
	return send_msg_to_act(
			svr, svr->waiting_cmd(), 
			(const char *)(&ack_body),
			sizeof(ack_body));
}
*/

int ActGetRoleInfoCmdProcessor::proc_pkg_from_client(server_t *svr,
		const char *body, int bodylen)
{
	platform_get_role_info_req_t* req_body =
		(platform_get_role_info_req_t*)body;
	if (bodylen < (int)sizeof(body)) {
		ERROR_TLOG("to small ack body %d, expect %lu",
			bodylen, sizeof(body));
		return send_err_to_server(svr,
				svr->waiting_cmd(), sw_err_proto_format_err);
	}
	dbproto::cs_get_base_info db_in;
	if (req_body->server_id == 0) {
		return send_err_to_server(svr,
			svr->waiting_cmd(), sw_err_must_give_svr_id);
	}
	db_in.set_server_id(req_body->server_id);
	return g_dbproxy->send_msg(
			svr, req_body->dest_user, 0,
			db_cmd_get_base_info,
			db_in);
}

int ActGetRoleInfoCmdProcessor::proc_pkg_from_serv(server_t *svr,
		const char* body, int bodylen)
{
	db_out_.Clear();
	if (parse_message(body, bodylen, &db_out_)) {
		return send_err_to_server(svr,
				svr->waiting_cmd(), sw_err_proto_format_err);
	}
	platform_get_role_info_ack_t  ack_body = {};
	//目前是一服一角色，后续游戏合服时，需要改成传数组的形式
	uint32_t base_info_size = db_out_.base_info_size();
	if (base_info_size) {
		const commonproto::player_base_info_t& pb_inf = db_out_.base_info(0);
		ack_body.level = pb_inf.level();
        memset(ack_body.name, 0, sizeof(ack_body.name));
		memcpy(ack_body.name, pb_inf.nick().c_str(), strlen(pb_inf.nick().c_str()) + 1);

        return send_msg_to_act(
                svr, svr->waiting_cmd(), 
                (const char *)(&ack_body),
                sizeof(ack_body));
    }

    // 没有角色时不返回包体,状态码置1,平台需求要和战神一致
    return send_msg_to_act(
                svr, svr->waiting_cmd(), 
                (const char *)(&ack_body),
                0, 1);
}

int PlatformIfRoleLoginDuringTmCmdProcessor::proc_pkg_from_client(
        server_t *svr, const char *body, int bodylen)
{
	platform_if_role_login_req_t* req_body =
		(platform_if_role_login_req_t*)body;
	if (bodylen < (int)sizeof(body)) {
		ERROR_TLOG("to small ack body %d, expect %lu",
			bodylen, sizeof(body));
		return send_err_to_server(svr,
				svr->waiting_cmd(), sw_err_proto_format_err);
	}
	db_in_.Clear();
	db_in_.set_from_tm(req_body->from_tm);
    db_in_.set_to_tm(req_body->to_tm);
	return g_dbproxy->send_msg(svr, svr->process_uid, 0, 
            db_cmd_get_login_tm_info, db_in_);
}

int PlatformIfRoleLoginDuringTmCmdProcessor::proc_pkg_from_serv(
        server_t *svr, const char *body, int bodylen)
{
	db_out_.Clear();
	if (parse_message(body, bodylen, &db_out_)) {
		return send_err_to_server(svr,
				svr->waiting_cmd(), sw_err_proto_format_err);
	}
    platform_if_role_login_rsp_t rsp;
    if (db_out_.login_tm_size()) {
        rsp.result = 1;
    } else {
        rsp.result = 0;
    }
    return send_msg_to_act(svr, svr->waiting_cmd(), (const char*)(&rsp), sizeof(rsp));
}

#if 0
//GM 工具
int forbid_player_talk_cmd(server_t *svr,
                        ISeer20SWProto::sw_msg_head_t *msghead,
                        ISeer20SWProto::sw_gm_forbid_talk_in *msg)
{
    if (msg->userid() == 0 || msg->create_tm() == 0) {
        return send_err_to_server(svr, ISeer20Comm::SW_ERR_PLAYER_NOT_EXIST);
    }

    //先请求DB
    ISeer20DBProto::db_forbid_player_talk_in db_msg;
    db_msg.set_duration(msg->dur());
    return send_to_db(svr, msg->userid(), msg->create_tm(), db_msg);
}

int db_forbid_player_talk_rsp(server_t *svr, 
                            ISeer20DBProto::db_msg_head_t *msghead,
                            ISeer20DBProto::db_forbid_player_talk_out *msg)
{
    //通知online
    notify_player_forbid_talk(msghead->userid(), msghead->create_tm(), msg->duration());
    //sw 回包
    ISeer20SWProto::sw_gm_forbid_talk_out rsp;
    rsp.set_userid(msghead->userid());
    rsp.set_create_tm(msghead->create_tm());
    rsp.set_dur(msg->duration());
    return send_msg_to_server(svr, rsp);
}

int notify_player_forbid_talk(uint32_t userid, int32_t create_tm, int32_t dur)
{
    player_t *p = PLAYER_MGR.get_player_by_uid(userid);
    if (!p) {
        return 0;
    }
    server_t *svr = SERVER_MGR.get_server_by_olid(p->online_id());
    if (!svr) {
        return 0;
    }

    //p在线 通知到online
    ISeer20SWProto::sw_notify_player_forbid_talk_out noti_msg;
    noti_msg.set_userid(userid);
    noti_msg.set_create_tm(create_tm);
    noti_msg.set_dur(dur);
    return send_msg_to_server(svr, noti_msg, DONT_CLEAR_WAITING_MSG);
}

int frozen_player_account_cmd(server_t *svr,
                            ISeer20SWProto::sw_msg_head_t *msghead,
                            ISeer20SWProto::sw_gm_frozen_account_in *msg)
{
    if (msg->userid() == 0 || msg->create_tm() == 0) {
        return send_err_to_server(svr, ISeer20Comm::SW_ERR_PLAYER_NOT_EXIST);
    }

    ISeer20DBProto::db_frozen_account_in db_msg;
    db_msg.set_duration(msg->dur());
    return send_to_db(svr, msg->userid(), msg->create_tm(), db_msg);
}

int db_frozen_account_rsp(server_t *svr, 
                        ISeer20DBProto::db_msg_head_t *msghead,
                        ISeer20DBProto::db_frozen_account_out *msg)
{
    notify_player_frozen_account(msghead->userid(), msghead->create_tm(), msg->duration());
    ISeer20SWProto::sw_gm_frozen_account_out rsp;
    rsp.set_userid(msghead->userid());
    rsp.set_create_tm(msghead->create_tm());
    rsp.set_dur(msg->duration());
    return send_msg_to_server(svr, rsp);
}

int notify_player_frozen_account(uint32_t userid, int32_t create_tm, int32_t dur)
{
    player_t *p = PLAYER_MGR.get_player_by_uid(userid);
    if (!p) {
        return 0;
    }
    server_t *svr = SERVER_MGR.get_server_by_olid(p->online_id());
    if (!svr) {
        return 0;
    }

    //p在线 通知到online
    ISeer20SWProto::sw_notify_player_frozen_account_out noti_msg;
    noti_msg.set_userid(userid);
    noti_msg.set_create_tm(create_tm);
    noti_msg.set_dur(dur);
    return send_msg_to_server(svr, noti_msg, DONT_CLEAR_WAITING_MSG);
}


int get_player_total_info_cmd(server_t *svr,
                            ISeer20SWProto::sw_msg_head_t *msghead,
                            ISeer20SWProto::sw_gm_get_player_total_in *msg)
{
    if (msg->userid() == 0 || msg->create_tm() == 0) {
        return send_err_to_server(svr, ISeer20Comm::SW_ERR_PLAYER_NOT_EXIST);
    }

    return db_get_player_total_req(svr, msg->userid(), msg->create_tm());
}

int db_get_player_total_req(server_t *svr, uint32_t uid, int32_t create_tm)
{
    ISeer20DBProto::get_player_total_in db_msg;
    return send_to_db(svr, uid, create_tm, db_msg);
}

int db_get_player_total_info_rsp(server_t *svr, 
                                ISeer20DBProto::db_msg_head_t *msghead,
                                ISeer20DBProto::get_player_total_out *msg)
{
    ISeer20SWProto::sw_gm_get_player_total_out rsp;

    rsp.set_uid(msghead->userid());
    rsp.mutable_basic()->CopyFrom(msg->basic());
    REPEATED_MSG_CPY(*msg, mails, rsp, mails, ISeer20Comm::mail_head_t);
    REPEATED_MSG_CPY(*msg, tasks, rsp, tasks, ISeer20Comm::pb_task_info_t);
    REPEATED_MSG_CPY(msg->mon_list(), monsters, rsp, mons, ISeer20Comm::mon_info_t);
    REPEATED_MSG_CPY(msg->mon_store_info(), index, rsp, store_mons, ISeer20Comm::store_mon_index_t);
    rsp.mutable_cap_mon_list()->CopyFrom(msg->cap_mon_list());
    REPEATED_MSG_CPY(*msg, offline_msg_list, rsp, offline_msg_list, ISeer20Comm::offline_msg_t);
    REPEATED_MSG_CPY(msg->friend_list(), player, rsp, friends, ISeer20Comm::player_info_t);
    REPEATED_MSG_CPY(msg->black_list(), player, rsp, blacks, ISeer20Comm::player_info_t);
    REPEATED_MSG_CPY(*msg, train_cd_info, rsp, train_cd_info, ISeer20Comm::train_store_cd_info_t);
    REPEATED_MSG_CPY(*msg, train_mode_info, rsp, train_mode_info, ISeer20Comm::train_store_mode_info_t);
    if (msg->has_pvp_data_info()) rsp.mutable_pvp_data_info()->CopyFrom(msg->pvp_data_info());
    rsp.set_itembag_capacity(msg->itembag_capacity());
    REPEATED_MSG_CPY(msg->item_rsp(), item, rsp, items, ISeer20Comm::one_item_t);
    REPEATED_MSG_CPY(*msg, map_mine_info, rsp, map_mine_info, ISeer20Comm::map_mine_info_t);
    REPEATED_MSG_CPY(*msg, lottery_data_info, rsp, lottery_data_info, ISeer20Comm::lottery_data_info_t);
    REPEATED_MSG_CPY(*msg, exchange_code_list, rsp, exchange_code_list, ISeer20Comm::exchange_code_info_t);
    if (msg->has_switch_info()) rsp.mutable_switch_info()->CopyFrom(msg->switch_info());
    REPEATED_MSG_CPY(*msg, activity_info, rsp, activity_info, ISeer20Comm::pb_activity_info_t);
    
    return send_msg_to_server(svr, rsp);
}

int db_update_inv_code_invitee_rsp(server_t *svr, 
                                   ISeer20DBProto::db_msg_head_t *msghead,
                                   ISeer20DBProto::db_update_inv_code_invitee_out *msg)
{
    svr->clear_waiting_msg();
    player_t *p = PLAYER_MGR.get_player_by_uid(msg->receiver().userid());
    if (!p) {
        return 0;
    }
    if (p->create_tm() != msg->receiver().create_tm()) {
        return 0;
    }
    
    ISeer20SWProto::sw_notify_change_invitee_info_out rsp; 
    rsp.mutable_receiver()->CopyFrom(msg->receiver());
    rsp.mutable_invitee_info()->CopyFrom(msg->invitee_info());
    return send_msg_to_server(svr, rsp, DONT_CLEAR_WAITING_MSG);
}

int gm_send_noti_cmd(server_t *svr,
                    ISeer20SWProto::sw_msg_head_t *msghead,
                    ISeer20SWProto::sw_gm_send_noti_in *msg)
{
    std::set<server_t *> to_set;
    if (msg->server_id_size() == 0) {
        FOREACH(SERVER_MGR.all_server_map(), it) {
            if (it->second->server_type() != msg->server_type()) continue;
            to_set.insert(it->second);
        }
    } else {
        for (int i = 0; i < msg->server_id_size(); i++) {
            server_t *svr = SERVER_MGR.get_server_by_olid(msg->onlie_id(i));
            if (!svr || svr->server_type() != msg->server_type()) continue;
            to_set.insert(svr);
        }
    }

    ISeer20SWProto::sw_notify_gm_noti_out noti;
    noti.set_content(msg->content());
    FOREACH(to_set, it) {
        send_msg_to_server(*it, noti, DONT_CLEAR_WAITING_MSG);
    }
    
    svr->clear_waiting_msg();
    return 0;
}

int gm_add_item_cmd(server_t *svr,
                    ISeer20SWProto::sw_msg_head_t *msghead,
                    ISeer20SWProto::sw_gm_add_item_in *msg)
{
    if (msg->userid() == 0 || msg->create_tm() == 0) {
        return send_err_to_server(svr, ISeer20Comm::SW_ERR_PLAYER_NOT_EXIST);
    }
    //db send mail
    ISeer20DBProto::db_send_mail_in db_msg;
    ISeer20Comm::mail_head_t *mail_head = db_msg.mutable_mail_head();
    
    string mail_id;
    gen_uuid(&mail_id);
    mail_head->set_mail_id(mail_id);
    mail_head->set_mail_time(time(0));
    mail_head->set_mail_type(0); //0表示系统邮件
    mail_head->set_sender_nick("SYSTEM");
    mail_head->set_read_state(0);
    mail_head->set_sender_id(msg->userid());
    mail_head->set_sender_create_tm(msg->create_tm());
    mail_head->set_receiver_id(msg->userid());
    mail_head->set_receiver_create_tm(msg->create_tm());
    mail_head->set_mail_title("客服物品补偿");
    db_msg.set_mail_content("补偿物品请从附件领取(物品补偿附件暂只显示一个物品,领取后请在背包中查收)");
    if (msg->items_size() == 0) {
        mail_head->set_enclosure_state(0);//没有附件
    } else {
        mail_head->set_enclosure_state(1);//有新附件
        for (int i = 0; i < msg->items_size(); i++) {
            ISeer20Comm::player_new_item_info_t *inf = db_msg.mutable_enclosure()->add_items();
            inf->CopyFrom(msg->items(i));
        }
    }
    return send_to_db(svr, msg->userid(), msg->create_tm(), db_msg);
}

int notify_player_new_mail(uint32_t userid, int32_t create_tm, 
                        ISeer20DBProto::db_send_mail_out *msg)
{
    player_t *p = PLAYER_MGR.get_player_by_uid(userid);
    if (!p) {
        return 0;
    }
    server_t *svr = SERVER_MGR.get_server_by_id(p->server_id());
    if (!svr) {
        return 0;
    }

    //p在线 通知到online
    ISeer20SWProto::sw_notify_new_mail_out noti_msg;
    noti_msg.set_userid(userid);
    noti_msg.set_create_tm(create_tm);
    noti_msg.mutable_new_mail()->CopyFrom(msg->mail_head());

    return send_msg_to_server(svr, noti_msg, DONT_CLEAR_WAITING_MSG);
}


int db_send_mail_rsp(server_t *svr,
                    ISeer20DBProto::db_msg_head_t *msghead,
                    ISeer20DBProto::db_send_mail_out *msg)
{
    notify_player_new_mail(msg->mail_head().receiver_id(), 
                            msg->mail_head().receiver_create_tm(), msg);
    ISeer20SWProto::sw_gm_add_item_out rsp;
    return send_msg_to_server(svr, rsp);
}

int gm_change_attr_cmd(server_t *svr,
                    ISeer20SWProto::sw_msg_head_t *msghead,
                    ISeer20SWProto::sw_gm_change_attr_in *msg)
{
    if (msg->userid() == 0 || msg->create_tm() == 0) {
        return send_err_to_server(svr, ISeer20Comm::SW_ERR_PLAYER_NOT_EXIST);
    }

    // 如果用户不在线 直接发给db
    player_t *p = PLAYER_MGR.get_player_by_uid(msg->userid());
    server_t *svr_online = NULL;
    if(p && p->create_tm() == msg->create_tm()){
        svr_online = SERVER_MGR.get_server_by_id(p->server_id());
    }

    if(!p || !svr_online){
        if (msg->vip_exp()){
            return send_err_to_server(svr, ISeer20Comm::SW_ERR_PLAYER_NOT_ONLINE);
        }

        ISeer20DBProto::db_update_player_basic_info_in db_msg;
        ISeer20Comm::player_basic_info_t *basic = db_msg.mutable_basic_info();
        if (msg->coin()) basic->set_coin(msg->coin());
        if (msg->gcoin()) basic->set_gcoin(msg->gcoin());
        if (msg->energy()) basic->set_cur_energy(msg->energy());
        return send_to_db(svr, msg->userid(), msg->create_tm(), db_msg);                 
    } 

    // 用户在线直接发给online
    ISeer20SWProto::sw_notify_change_attr_out noti_msg;
    noti_msg.set_userid(msg->userid());
    noti_msg.set_create_tm(msg->create_tm());
    noti_msg.set_coin(msg->coin());
    noti_msg.set_gcoin(msg->gcoin());
    noti_msg.set_energy(msg->energy());
    noti_msg.set_vip_exp(msg->vip_exp());
    
    send_msg_to_server(svr_online, noti_msg, DONT_CLEAR_WAITING_MSG);
    ISeer20SWProto::sw_gm_change_attr_out rsp;
    return send_msg_to_server(svr, rsp);
}

int notify_player_attr_chg(uint32_t userid, int32_t create_tm, 
                        ISeer20DBProto::db_update_player_basic_info_out *msg)
{
    player_t *p = PLAYER_MGR.get_player_by_uid(userid);
    if (!p) {
        return 0;
    }
    server_t *svr = SERVER_MGR.get_server_by_id(p->server_id());
    if (!svr) {
        return 0;
    }

    //p在线 通知到online
    ISeer20SWProto::sw_notify_change_attr_out noti_msg;
    noti_msg.set_userid(userid);
    noti_msg.set_create_tm(create_tm);
    noti_msg.set_coin(msg->basic_info().coin());
    noti_msg.set_gcoin(msg->basic_info().gcoin());
    noti_msg.set_energy(msg->basic_info().cur_energy());
    noti_msg.set_vip_exp(msg->basic_info().vip_exp());

    return send_msg_to_server(svr, noti_msg, DONT_CLEAR_WAITING_MSG);
}

int db_change_attr_rsp(server_t *svr,
                    ISeer20DBProto::db_msg_head_t *msghead,
                    ISeer20DBProto::db_update_player_basic_info_out *msg)
{
    // 发送给db表明用户当时离线 向online广播不需要了
//    notify_player_attr_chg(msghead->userid(), msghead->create_tm(), msg);
    ISeer20SWProto::sw_gm_change_attr_out rsp;
    return send_msg_to_server(svr, rsp);
}


int gm_get_role_list_cmd(server_t *svr,
                        ISeer20SWProto::sw_msg_head_t *msghead,
                        ISeer20SWProto::sw_gm_get_role_list_in *msg)
{
    if (msg->userid() == 0) {
        return send_err_to_server(svr, ISeer20Comm::SW_ERR_PLAYER_NOT_EXIST);
    }
    ISeer20DBProto::db_get_role_list_in db_msg;
    db_msg.set_userid(msg->userid());
    return send_to_db(svr, msg->userid(), 0, db_msg);
}

int db_get_role_list_rsp(server_t *svr,
                        ISeer20DBProto::db_msg_head_t *msghead,
                        ISeer20DBProto::db_get_role_list_out *msg)
{
    if (msghead->ol_waiting() == "ISeer20SWProto.sw_gm_is_iseer_user_in") {
        ISeer20SWProto::sw_gm_is_iseer_user_out swrsp;
        if(msg->role_list_size() == 0){
            swrsp.set_userid(msghead->userid());
            swrsp.set_is_iseer(0);
        }else{
            swrsp.set_userid(msghead->userid());
            swrsp.set_userid(msghead->userid());
        }
        return send_msg_to_server(svr, swrsp);
    }
    ISeer20SWProto::sw_gm_get_role_list_out rsp;
    rsp.set_userid(msghead->userid());
    REPEATED_MSG_CPY(*msg, role_list, rsp, role_list, ISeer20Comm::server_id_role_t);
    return send_msg_to_server(svr, rsp);
}

#endif
