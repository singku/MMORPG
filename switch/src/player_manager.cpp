#include "player.h"
#include "server.h"
#include "server_manager.h"
#include "player_manager.h"
#define MAX_USER_ID 500

player_t *player_manager_t::create_new_player(player_basic_t *basic)
{
    assert(basic);
    assert(basic->uid_);
    assert(basic->create_tm_);

    player_t *p = new player_t();
    if (!p) {
        ERROR_LOG("Error: failed calloc player_t, uid=%u", basic->uid_);
        return 0;
    }

    p->set_uid(basic->uid_);
    p->set_create_tm(basic->create_tm_);
    p->set_is_vip(basic->is_vip_);
     
	DEBUG_TLOG("create_new_player, uid=%u, create_tm=%d",
			p->uid(), p->create_tm());
	return p;
}

void player_manager_t::add_player(player_t *p, uint32_t in_server_id, uint32_t in_online_id)
{
    assert(p);
    assert(in_server_id);
    assert(in_online_id);

    server_t *svr = SERVER_MGR.get_server_by_olid(in_online_id);
    assert(svr);
    svr->inc_player_num(p->is_vip());

    p->set_server_id(in_server_id);
    p->set_online_id(in_online_id);
	uid_to_player_map_.insert(make_pair(p->uid(), p));
    inc_player_num(p->is_vip(), in_server_id, svr->idc_zone());

    DEBUG_TLOG("add_player uid=%u, create_tm=%d, to server_id=%u online_id=%u",
            p->uid(), p->create_tm(), p->server_id(), p->online_id());

    DEBUG_TLOG("Player_Stat: SVR[id:%u, olid:%u, total:%u, vip:%u]   AREA[total:%u, vip:%u, telcom:%u, netcom:%u]",
            p->server_id(), p->online_id(), svr->get_total_player_num(), svr->get_total_vip_player_num(),
            get_total_player_num(0), get_total_vip_player_num(0), get_total_tel_player_num(0), get_total_net_player_num(0));
}

void player_manager_t::del_player(player_t *p)
{
    assert(p);

    server_t *svr = SERVER_MGR.get_server_by_olid(p->online_id());
    assert(svr);
    svr->dec_player_num(p->is_vip());

    uid_to_player_map_.erase(p->uid());
    dec_player_num(p->is_vip(), p->server_id(), svr->idc_zone());
 
    DEBUG_TLOG("del_player uid=%u, create_tm=%d, from server_id=%u online_id=%u", 
            p->uid(), p->create_tm(), p->server_id(), p->online_id());

    DEBUG_TLOG("Player_Stat: SVR[id:%u, olid:%u total:%u, vip:%u]   AREA[total:%u, vip:%u,  telcom:%u, netcom:%u]",
            p->server_id(), p->online_id(), svr->get_total_player_num(), svr->get_total_vip_player_num(),
            get_total_player_num(0), get_total_vip_player_num(0), get_total_tel_player_num(0), get_total_net_player_num(0));

	// 释放player相关资源
    delete p;
}

void player_manager_t::batch_del_players(uint32_t online_id)
{
    uid_to_player_map_iter_t it = uid_to_player_map_.begin();
    uid_to_player_map_iter_t del_it;

    for(; it != uid_to_player_map_.end();) {
        if (it->second->online_id() != online_id) {
            it ++;
            continue;
        }
        del_it = it;
        it ++;
        del_player(del_it->second);
    }
}

void player_manager_t::get_player_list(std::vector<role_info_t> &players, uint32_t server_id)
{
	uid_to_player_map_iter_t it = uid_to_player_map_.begin();
	uint32_t count = 0;	
	for(; it != uid_to_player_map_.end(); ++it) {
        if (it->second->server_id() != server_id) {
            continue;//不同服
        }
        role_info_t tmp;
        tmp.userid = it->first;
        tmp.u_create_tm = it->second->create_tm();
        players.push_back(tmp);
		++count;
		if (count >= 100) {
			break;
		}
	}
}
