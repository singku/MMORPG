#include "player.h"
#include "player_manager.h"
#include "duplicate_conf.h"
#include "builder_conf.h"
#include "duplicate_trigger.h"
#include "duplicate_entity.h"
#include "global_data.h"

void dup_entity_mgr_t::clear()
{
    FOREACH(entity_map_, it) {
        destroy_entity(*it);   
    }
    entity_map_.clear();

    world_boss_entity_ = 0;
}

duplicate_entity_t* dup_entity_mgr_t::create_entity(uint32_t dup_id, uint32_t init_map_id)
{
    duplicate_entity_t *entity = (duplicate_entity_t*)calloc(1, sizeof(duplicate_entity_t));
    if (!entity) {
        goto ERR_RT;
    }

    INIT_LIST_HEAD(&(entity->timer_list));
    entity->dup_time_limit_timer = NULL;
    entity->flush_mon_timer = NULL;
    entity->phase_timer = NULL;
    entity->destroy_dup_timer = NULL;
    entity->boss_show_timer = NULL;
    entity->force_ready_timer = NULL;

    entity->dup_id = dup_id;
    entity->cur_map_phase = 1;//默认为第1阶段
    entity->start = 0;
    entity->life_counter = 0;
    entity->state = DUPLICATE_STATE_CREATE;
    entity->cur_map_id = init_map_id;
    entity->ready_map_id = init_map_id;
    entity->skill_affect_obj_type = 0;
    entity->skill_affect_obj_create_tm = 0;

    entity->cur_map_enemy = new std::map<uint32_t, duplicate_map_pet_t>();
    if (!entity->cur_map_enemy) {
        goto ERR_RT;
    }
    entity->cur_map_non_enemy = new std::map<uint32_t, duplicate_map_pet_t>();
    if (!entity->cur_map_non_enemy) {
        delete entity->cur_map_enemy;
        goto ERR_RT;
    }
    entity->fini_map_id = new std::set<uint32_t>();
    if (!entity->fini_map_id) {
        delete entity->cur_map_enemy;
        delete entity->cur_map_non_enemy;
        goto ERR_RT;
    }
    entity->battle_players = new std::map<uint32_t, std::set<player_t*> >();
    if (!entity->battle_players) {
        delete entity->cur_map_enemy;
        delete entity->cur_map_non_enemy;
        delete entity->fini_map_id;
        goto ERR_RT;
    }

    entity->line_players = new std::map<uint32_t, battle_side_players_t>();
    if (!entity->line_players) {
        delete entity->cur_map_enemy;
        delete entity->cur_map_non_enemy;
        delete entity->fini_map_id;
        delete entity->battle_players;
        goto ERR_RT;
    }

    entity->cur_map_dead_boss_set = new std::set<uint32_t>();
    if (!entity->cur_map_dead_boss_set) {
        delete entity->cur_map_enemy;
        delete entity->cur_map_non_enemy;
        delete entity->fini_map_id;
        delete entity->battle_players;
        delete entity->line_players;
        goto ERR_RT;
    }
    entity->born_area_born_record = new std::map<uint32_t, uint32_t>();
    if (!entity->born_area_born_record) {
        delete entity->cur_map_enemy;
        delete entity->cur_map_non_enemy;
        delete entity->fini_map_id;
        delete entity->battle_players;
        delete entity->cur_map_dead_boss_set;
        delete entity->line_players;

        goto ERR_RT;
    }

    entity->born_area_dead_record = new std::map<uint32_t, uint32_t>();
    if (!entity->born_area_dead_record) {
        delete entity->cur_map_enemy;
        delete entity->cur_map_non_enemy;
        delete entity->fini_map_id;
        delete entity->battle_players;
        delete entity->cur_map_dead_boss_set;
        delete entity->born_area_born_record;
        delete entity->line_players;

        goto ERR_RT;
    }

    if (g_duplicate_conf_mgr.get_duplicate_type(entity->dup_id) == DUP_BTL_TYPE_WORLD_BOSS) {
        world_boss_entity_ = entity;
    } else {
        entity_map_.insert(entity);
    }

    return entity;

ERR_RT:
    ERROR_TLOG("Failed to Create Duplicate entity for dup:%u", dup_id);
    if (entity) {
        delete entity;
    }
    return 0;
}

void dup_entity_mgr_t::destroy_entity(duplicate_entity_t *dup_entity)
{
    if (entity_map_.count(dup_entity) == 0 && dup_entity != world_boss_entity_) {
        return;
    }
    if (dup_entity == world_boss_entity_) {
        world_boss_entity_ = 0;
    }

    FOREACH((*(dup_entity->battle_players)), it) {
        std::set<player_t*> &p_set = it->second;
        FOREACH(p_set, it2) {
            player_t *player = *it2;
            g_player_manager->del_player(player);
        }
    }

     //删除定时器
    REMOVE_TIMERS(dup_entity);

    delete dup_entity->cur_map_enemy;
    delete dup_entity->cur_map_non_enemy;
    delete dup_entity->fini_map_id;
    delete dup_entity->battle_players;
    delete dup_entity->line_players;
    delete dup_entity->cur_map_dead_boss_set;
    delete dup_entity->born_area_born_record;
    delete dup_entity->born_area_dead_record;

    entity_map_.erase(dup_entity);

    DEBUG_TLOG("Dup:%u ACTION: been deleted", dup_entity->dup_id);
    delete dup_entity;
}

void dup_entity_mgr_t::try_destroy_entity(duplicate_entity_t *dup_entity)
{
    assert(dup_entity);
    if (!dup_entity->battle_players->empty()) {
        return;
    }
    destroy_entity(dup_entity);
}

uint32_t dup_entity_mgr_t::get_entity_one_uid(duplicate_entity_t *dup_entity)
{
    FOREACH((*(dup_entity->battle_players)), it) {
        std::set<player_t*> &p_set = it->second;
        return (*(p_set.begin()))->uid;
    }
    return 0;
}

duplicate_entity_t *dup_entity_mgr_t::get_world_boss_entity()
{
    return world_boss_entity_;
}

uint32_t dup_entity_mgr_t::add_battle_player(
        duplicate_entity_t *dup_entity, uint32_t side, player_t *player)
{
    assert(dup_entity);
    assert(dup_entity->line_players);
    uint32_t line_id = 0;

    std::map<uint32_t, battle_side_players_t>::iterator iter = dup_entity->line_players->find(side);
    if (iter == dup_entity->line_players->end() ) {
        // 新增加用户,初始地图分线id从1开始
        battle_side_players_t side_info;
        line_id = 1;
        std::set<player_t *> p_set;
        p_set.insert(player);
        side_info.player_map.insert(std::pair<uint32_t, std::set<player_t *> >(line_id, p_set));
        dup_entity->line_players->insert(std::pair<uint32_t, battle_side_players_t>(side, side_info));
    } else {
        //把用户插入有空余的分线
        FOREACH(iter->second.player_map, it){
            if (it->second.size() < commonproto::WORLD_BOSS_MAP_LINE_PLAYER_NUM) {
                it->second.insert(player);
                line_id = it->first;
                return line_id;
            }
       } 
       iter->second.max_line_id++;
       line_id = iter->second.max_line_id;
        std::set<player_t *> p_set;
        p_set.insert(player);
        iter->second.player_map.insert(std::pair<uint32_t, std::set<player_t *> >(line_id, p_set));
    }

    return line_id;
}

int dup_entity_mgr_t::remove_battle_player(
        duplicate_entity_t *dup_entity, player_t *player)
{
    assert(dup_entity);
    assert(dup_entity->line_players);

    FOREACH_NOINCR_ITER(*(dup_entity->line_players), iter) {
        FOREACH_NOINCR_ITER(iter->second.player_map, it) {
            if (it->second.find(player) != it->second.end()) {
                it->second.erase(player);
            }

            if (it->second.empty()) {
                std::map<uint32_t, std::set<player_t *> >::iterator tmp_it = it;
                it++;
                iter->second.player_map.erase(tmp_it);
            } else {
                it++;
            }
        }

        if (iter->second.player_map.empty()) {
            std::map<uint32_t, battle_side_players_t>::iterator tmp_it = iter;
            iter++;
            dup_entity->line_players->erase(tmp_it);
        } else {
            iter++;
        }
    }
    return 0;
}

std::set<player_t *> * dup_entity_mgr_t::get_line_players(
        duplicate_entity_t *entity, uint32_t side, uint32_t line_id)
{
    if (entity == NULL || entity->line_players == NULL) {
        return NULL;
    }

    std::map<uint32_t, battle_side_players_t>::iterator iter = entity->line_players->find(side);
    if ( iter == entity->line_players->end() ) {
        return NULL;
    }

    std::map<uint32_t, std::set<player_t *> >::iterator it = iter->second.player_map.find(line_id);
    if ( it == iter->second.player_map.end()) {
        return NULL;
    }

    return &(it->second);
}

int relay_notify_msg_to_entity_except(duplicate_entity_t *dup_entity, 
        uint32_t cmd, const google::protobuf::Message &msg, player_t *except)
{
    assert(dup_entity);
    FOREACH((*(dup_entity->battle_players)), it) {
        std::set<player_t*> &p_set = it->second;
        FOREACH(p_set, it2) {
            if (*it2 == except || (*it2)->player_dup_state == PLAYER_LEAVE) {
                continue;
            }
            relay_notify_msg_to_player(*it2, cmd, msg);
        }
    }
    return 0;
}

int send_msg_to_entity_except(duplicate_entity_t *dup_entity, 
        uint32_t cmd, const google::protobuf::Message &msg, player_t *except)
{
    assert(dup_entity);
    FOREACH((*(dup_entity->battle_players)), it) {
        std::set<player_t*> &p_set = it->second;
        FOREACH(p_set, it2) {
            if (*it2 == except || (*it2)->player_dup_state == PLAYER_LEAVE) {
                continue;
            }
            send_msg_to_player(*it2, cmd, msg);
        }
    }
    return 0;
}

/** 
 * @brief 副本中同步通知消息(ppve玩法可分线)
 * 
 * @param dup_entity 
 * @param line_id 0 发送副本中的所有玩家 >0 在多人玩法中发送给对应line_id的玩家
 * @param cmd 
 * @param msg 
 * @param except 
 * 
 * @return 
 */
int relay_notify_msg_to_entity_line_except(duplicate_entity_t *dup_entity, uint32_t line_id,
        uint32_t cmd, const google::protobuf::Message &msg, player_t *except)
{
    assert(dup_entity);
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_entity->dup_id);
    if (line_id > 0 && dup->battle_type == DUP_BTL_TYPE_WORLD_BOSS) {
        // ppve只发送消息给同分线玩家
        FOREACH((*(dup_entity->line_players)), it) {
            battle_side_players_t &p_side= it->second;
            std::map<uint32_t, std::set<player_t *> >::iterator iter = p_side.player_map.find(line_id);
            FOREACH(iter->second, it2) {
                if (*it2 == except || (*it2)->player_dup_state == PLAYER_LEAVE) {
                    continue;
                }
                relay_notify_msg_to_player(*it2, cmd, msg);
            }
        } 
    } else {
        relay_notify_msg_to_entity_except(dup_entity, cmd, msg, except);
    }

    return 0;
}

int duplicate_entity_trig(duplicate_entity_t *entity, uint32_t type, std::vector<uint32_t> *args)
{
    if (entity->state == DUPLICATE_STATE_CAN_END) {//防止重复触发
        return 0;
    }

    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(entity->dup_id);
    if (!dup) {
        ERROR_TLOG("Dup %u of dup_entity not exist", entity->dup_id);
        return -1;
    }
    //抛出条件
    if (dup->cond_map.count(type) == 0) {
        return 0;
    }
    const std::vector<DupCondBase*> &cond_vec = 
        (dup->cond_map.find(type))->second;
    FOREACH(cond_vec, it) {
        DupCondBase *cond = *it;
        if (args && !(cond->args_match(*args))) {
            continue;
        }
        cond->trig(entity);
    }
    return 0;
}

//返回获胜的一方
uint32_t duplicate_entity_win_side(duplicate_entity_t *entity)
{
    uint32_t live_side = 0;
    uint32_t side = 0;

    uint32_t rpvp_side1_hp;
    uint32_t rpvp_side2_hp;

    FOREACH((*(entity->battle_players)), it) {
        std::set<player_t*> &p_set = it->second;
        FOREACH(p_set, it2) {
            player_t *player = *it2;
            if (player->cur_hp != 0) {
                live_side ++;
                side = it->first;
                if (side == SIDE1) {
                    rpvp_side1_hp = player->cur_hp;
                } else {
                    rpvp_side2_hp = player->cur_hp;
                }
                break;
            }
        }
    }
    if (live_side > 1) {
        duplicate_battle_type_t type = g_duplicate_conf_mgr.get_duplicate_type(entity->dup_id);
        if (type == DUP_BTL_TYPE_RPVP) {
            return rpvp_side1_hp >= rpvp_side2_hp ? SIDE1 :SIDE2;
        } else if (type == DUP_BTL_TYPE_PVEP) {
            return SIDE2;
        }
        return 0;
    }
    return side;
}

void duplicate_entity_del_player(duplicate_entity_t *entity, player_t *player)
{
    FOREACH_NOINCR_ITER((*(entity->battle_players)), it) {
        std::set<player_t*> &p_set = it->second;
        FOREACH(p_set, it2) {
            player_t *p = *it2;
            if (player == p) {
                p_set.erase(p);
                break;
            }
        }
        if (p_set.empty()) {
            std::map<uint32_t, std::set<player_t*> >::iterator it1 = it;
            it++;
            entity->battle_players->erase(it1);
        } else {
            it++;
        }
    }
}

void duplicate_entity_new_captain(duplicate_entity_t *entity)
{
    bool find = false;
    player_t *captain = 0;
    FOREACH((*(entity->battle_players)), it) {
        std::set<player_t*> &p_set = it->second;
        FOREACH(p_set, it2) {
            player_t *p = *it2;
            p->is_captain = 1;
            captain = p;
            find = true;
            break;
        }
        if (find) {
            break;
        }
    }
    if (find) {
        //通知新的队长
        onlineproto::sc_0x022E_duplicate_notify_switch_captain noti_msg;
        noti_msg.set_uid(captain->uid);
        noti_msg.set_u_create_tm(captain->create_tm);
        relay_notify_msg_to_entity_except(entity, cli_cmd_cs_0x022E_duplicate_notify_switch_captain, 
                noti_msg, 0);
    }
}

uint32_t get_pet_id_by_actor_idx(uint32_t dup_id, uint32_t idx)
{
    const duplicate_t *dup_conf = g_duplicate_conf_mgr.find_duplicate(dup_id);
    if (!dup_conf) {
        return 0;
    }
    if (idx > dup_conf->mon_vec.size()) {
        return 0;
    }
    uint32_t pet_id = dup_conf->mon_vec[idx-1];
    if (!g_pet_conf_mgr.pet_conf_exist(pet_id)) {
        return 0;
    }
    return pet_id;
}

uint32_t get_builder_id_by_actor_idx(uint32_t dup_id, uint32_t idx)
{
    const duplicate_t *dup_conf = g_duplicate_conf_mgr.find_duplicate(dup_id);
    if (!dup_conf) {
        return 0;
    }
    if (idx > dup_conf->builder_vec.size()) {
        return 0;
    }
    uint32_t builder_id = dup_conf->builder_vec[idx-1];
    if (!g_builder_conf_mgr.builder_conf_exist(builder_id)) {
        return 0;
    }
    return builder_id;
}

//打包副本中的玩家怪和阻挡
uint32_t pack_duplicate_(duplicate_entity_t *entity);


//获取副本队伍的平均等级
uint32_t get_players_mean_level_by_side(
		duplicate_entity_t *entity, player_duplicate_side_t side)
{
	uint32_t total_lv = 0;
	std::map<uint32_t, std::set<player_t*> >::iterator m_iter = 
		entity->battle_players->find(side);
	if(m_iter == entity->battle_players->end()){
		return 0;
	}
	std::set<player_t*> p_set = m_iter->second; 
	if(p_set.empty()){
		return 0;
	}
	FOREACH(p_set, it) {
		total_lv += (*it)->level;
	}
	return total_lv / p_set.size();
}

//动态生成map_pet信息
uint32_t init_map_pet_info_dynamic(
		duplicate_entity_t *entity, duplicate_map_pet_t &dpet)
{
    if (entity == NULL) {
        return 0;
    }

    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(entity->dup_id);
    if (dup == NULL) {
        return 0;
    }

    // 伙伴儿祈福，根据平均等级生成boss
    if (dup->mode == onlineproto::DUP_MODE_TYPE_BLESS_PET) {
		uint32_t mean_lv = get_players_mean_level_by_side(entity, SIDE1);
		dpet.pet_level = mean_lv;
	}

	return 0;
}
