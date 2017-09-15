#include "switch.h"
#include "server.h"
#include "server_manager.h"
#include "player_manager.h"

std::map<int, server_t*> g_pending_proto_svrs;
std::map<uint32_t, StatLogger*> g_stat_logger_map;

const uint32_t TEMP_SVR_TYPE = 0xFFFFFFFF;

server_t *server_manager_t::create_new_server(server_basic_info_t *info)
{
	assert(info);
	assert(info->server_id_);
    assert(info->online_id_);
    assert(info->fdsess_);

    server_t *svr = new server_t();
    if (!svr) {
        ERROR_TLOG("Error: failed calloc server_t, svrid=%u", info->server_id_);
        return 0;
    }

    svr->set_server_id(info->server_id_);
    svr->set_online_id(info->online_id_);
    svr->set_server_type(info->server_type_);
    svr->set_listen_port(info->listen_port_);
    svr->set_host_ip(info->host_ip_);
    svr->set_fdsess(info->fdsess_);
    svr->clear_waiting_cmd();
    svr->process_seq = 0;
    svr->set_idc_zone(info->idc_zone_);
    
	DEBUG_TLOG("create_new_server, svrid=%u, olid=%u, ip:%u, port:%u", 
                svr->server_id(), svr->online_id(), svr->fdsess()->remote_ip, svr->listen_port());
	return svr;
}

server_t *server_manager_t::create_temp_server(fdsession_t *fdsess)
{
    assert(fdsess);
    server_t *svr = new server_t();
    if (!svr) {
        ERROR_TLOG("Error: failed calloc server_t");
        return 0;
    }

    svr->set_server_id(0);
    svr->set_online_id(0);
    svr->set_server_type(TEMP_SVR_TYPE);
    svr->set_fdsess(fdsess);
    svr->clear_waiting_cmd();
    svr->process_seq = 0;
     
	DEBUG_TLOG("create_tmp_server, fd=%u, ip:%u", 
                svr->fdsess()->fd, svr->fdsess()->remote_ip);
	return svr;
}

void server_manager_t::add_server(server_t *svr)
{
    assert(svr);

    if (svr->server_type() == TEMP_SVR_TYPE) {
        DEBUG_TLOG("add_temp_connection fd=%u", svr->fdsess()->fd);
    } else {
        DEBUG_TLOG("add_server, svrid=%u", svr->online_id());
        svrid_to_server_map_.insert(make_pair(svr->online_id(), svr));
    }
    fd_to_server_map_.insert(make_pair(svr->fdsess()->fd, svr));
}

void server_manager_t::del_server(server_t *svr)
{
	// 释放player相关资源 服务器断开的操作不是频繁操作
    // 且同时在线的人数最多不过百万.轮询所有用户产生的消耗不大. 
    // TODO(singku) 可让每个server记录本server的玩家索引 这样只需轮询
    // 本server的记录即可.可牺牲内存提高效率.目前来说 没有必要

    if (svr->server_type() != TEMP_SVR_TYPE) {//非临时svr
        PLAYER_MGR.batch_del_players(svr->online_id());
        svrid_to_server_map_.erase(svr->online_id());
        DEBUG_TLOG("del_server, svrid=%u olid=%u", svr->server_id(), svr->online_id());
    } else {
        DEBUG_TLOG("del_temp_connection fd=%u", svr->fdsess()->fd);
    }

    if (svr->server_type() == commonproto::SERVER_TYPE_BATTLE) {
        switchproto::sc_notify_server_stat noti_msg;
        noti_msg.set_up_or_done(false);
        commonproto::svr_info_t *svr_info = noti_msg.mutable_svr();
        svr_info->set_type(commonproto::SERVER_TYPE_BATTLE);
        svr_info->set_svr_id(svr->server_id());
        svr_info->set_online_id(svr->online_id());
        svr_info->set_ip(svr->host_ip());
        svr_info->set_port(svr->listen_port());
        svr_info->set_svr_name(svr->name());

        FOREACH(svrid_to_server_map_, it) {
            server_t *asvr = it->second;
            if (asvr->server_type() !=  commonproto::SERVER_TYPE_ONLINE) {
                continue;
            }

            send_msg_to_server(asvr, sw_cmd_sw_notify_server_stat, noti_msg,
                    DONT_CLEAR_WAITING_CMD);
        }
    }

    fd_to_server_map_.erase(svr->fdsess()->fd);
    delete svr;
}

uint32_t server_manager_t::transmit_msg_to_dest_svr(
		uint32_t svr_id, uint16_t cmd,
		google::protobuf::Message& message)
{
	std::set<server_t*> online_set;
	online_set = SERVER_MGR.get_onlines_by_svrid(svr_id);
	FOREACH(online_set, it) {
		server_t * dst_svr = *it; 
		if (!dst_svr) {
			return sw_err_server_not_exist;
		}
		send_msg_to_server(dst_svr, cmd, message, DONT_CLEAR_CMD);
	}
	return 0;
}

StatLogger *get_stat_logger(uint32_t svr_id)
{
    static StatLogger default_logger;
    static bool default_logger_init = false;
    if (unlikely(default_logger_init == false)) {
        default_logger.init(config_get_intval("gameid", 19));
        default_logger_init = true;
    }

    if (svr_id == 0) {
        return &default_logger;
    }
    if (g_stat_logger_map.count(svr_id) == 0) {
        StatLogger *stat_logger = new StatLogger(config_get_intval("gameid", 19), -1, svr_id);
        if (!stat_logger) {
            return &default_logger;
        }
        g_stat_logger_map[svr_id] = stat_logger;
        return stat_logger;
    }
    return g_stat_logger_map.find(svr_id)->second;
}
