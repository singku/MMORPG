#include "player.h"
#include "duplicate_conf.h"
#include "global_data.h"
#include "player_manager.h"

player_t *player_manager_t::create_new_player(uint32_t uid, fdsession_t *fdsess)
{

    player_t *p = new player_t();
    if (!p) {
        ERROR_TLOG("Error: failed calloc player_t, uid=%u", uid);
        return 0;
    }
    p->uid = uid;
    p->fdsess = fdsess;
    p->is_artifacial = false;
    add_player(p);
	return p;
}

player_t *player_manager_t::create_artifacial_player(uint32_t uid, fdsession_t *fdsess)
{

    player_t *p = new player_t();
    if (!p) {
        ERROR_TLOG("Error: failed calloc player_t, uid=%u", uid);
        return 0;
    }
    p->uid = uid;
    p->fdsess = fdsess;
    p->is_artifacial = true;
    add_player(p);
	return p;
}

void player_manager_t::add_player(player_t *p)
{
    assert(p);
    if (!p->is_artifacial) {
        uid_to_player_map_.insert(make_pair(p->uid, p));
    } else {
        uid_to_artifacial_player_map_.insert(make_pair(p->uid, p));
    }
    inc_player_num();
}

void player_manager_t::del_player(player_t *p)
{
    assert(p);
    if (!p->is_artifacial) {
        uid_to_player_map_.erase(p->uid);
    } else {
        uid_to_artifacial_player_map_.erase(p->uid);
    }
    dec_player_num();
    DEBUG_TLOG("P:%u ACTION: been deleted", p->uid);
    delete p;
}

void player_manager_t::batch_del_players(int fd)
{
    uid_to_player_map_iter_t it = uid_to_player_map_.begin();
    uid_to_player_map_iter_t del_it;
    std::set<duplicate_entity_t *> del_entities;
    for(; it != uid_to_player_map_.end();) {
        if (it->second->fdsess->fd != fd) {
            it ++;
            continue;
        }
        del_it = it;
        it ++;
        if (del_it->second->dup_entity) {
            del_entities.insert(del_it->second->dup_entity);
            //解索引
            duplicate_entity_del_player(del_it->second->dup_entity, 
                    del_it->second);
        }
        del_player(del_it->second);
    }
    it = uid_to_artifacial_player_map_.begin();
    for(; it != uid_to_artifacial_player_map_.end();) {
        if (it->second->fdsess->fd != fd) {
            it ++;
            continue;
        }
        del_it = it;
        it ++;
        if (del_it->second->dup_entity) {
            del_entities.insert(del_it->second->dup_entity);
            //解索引
            duplicate_entity_del_player(del_it->second->dup_entity, 
                    del_it->second);
        }
        del_player(del_it->second);
    }
    FOREACH(del_entities, it) {
        g_dup_entity_mgr->try_destroy_entity(*it);
    }
}
