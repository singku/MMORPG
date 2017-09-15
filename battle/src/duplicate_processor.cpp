#include "player.h"
#include "global_data.h"
#include "duplicate_processor.h"
#include "duplicate_conf.h"
#include "duplicate_trigger.h"
#include "duplicate_entity.h"
#include "data_proto_utils.h"
#include "player_manager.h"
#include "map_conf.h"

int DuplicateRelayCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    if (!player->dup_entity) {
        return send_err_to_player(player, player->wait_cmd,
                cli_err_player_not_in_this_duplicate);
    }
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(
            player->dup_entity->dup_id);
    if (!dup) {
        return send_err_to_player(player, player->wait_cmd,
                cli_err_duplicate_id_not_found);
    }

    switch(cli_in_.cmd()) {
    case cli_cmd_cs_0x0203_duplicate_leave_map:
        return proc_duplicate_leave_map(player, cli_in_.pkg());
    case cli_cmd_cs_0x0205_duplicate_battle_ready:
        return proc_duplicate_battle_ready(player, cli_in_.pkg());
    case cli_cmd_cs_0x0208_duplicate_exit:
        return proc_duplicate_exit(player, cli_in_.pkg());
    case cli_cmd_cs_0x020A_duplicate_hit_character:
        return proc_duplicate_hit_character(player, cli_in_.pkg());
    case cli_cmd_cs_0x0211_duplicate_to_next_phase:
        return proc_duplicate_to_next_phase(player, cli_in_.pkg());
    case cli_cmd_cs_0x0106_player_change_state:
        return proc_duplicate_change_state(player, cli_in_.pkg());
    case cli_cmd_cs_0x0229_duplicate_switch_fight_pet:
        return proc_duplicate_switch_fight_pet(player, cli_in_.pkg());
    case cli_cmd_cs_0x022B_duplicate_mon_flush_request:
        return proc_duplicate_front_mon_flush_req(player, cli_in_.pkg());
    case cli_cmd_cs_0x022D_skill_affect:
        return proc_duplicate_skill_affect(player, cli_in_.pkg());
    default:
        break;
    }
    return 0;
}

int DuplicateRelayCmdProcessor::proc_duplicate_leave_map(
        player_t *player, const string &pkg)
{
    TRACE_TLOG("Player:%u relay leave_map", player->uid);
    //回包
    onlineproto::sc_0x0203_duplicate_leave_map cli_out;
    relay_msg_to_player(player, cli_cmd_cs_0x0203_duplicate_leave_map, cli_out);
    //设置玩家离开地图
    player->player_dup_state = PLAYER_LEAVE;
    //通知其他人玩家已经离开
    onlineproto::sc_0x0204_duplicate_notify_leave_map noti_msg;
    noti_msg.add_userid_list(player->uid);
    //relay_notify_msg_to_entity_except(player->dup_entity, 
    //cli_cmd_cs_0x0204_duplicate_notify_leave_map, noti_msg, player);

    // TODO toby 分线test
    relay_notify_msg_to_entity_line_except(player->dup_entity, player->line_id,
            cli_cmd_cs_0x0204_duplicate_notify_leave_map, noti_msg, player);

    return 0;
}

int DuplicateRelayCmdProcessor::proc_duplicate_battle_ready(
        player_t *player, const string &pkg)
{
    TRACE_TLOG("Player:%u relay ready", player->uid);
 
    //回包
    onlineproto::sc_0x0205_duplicate_battle_ready cli_out;
    relay_msg_to_player(player, cli_cmd_cs_0x0205_duplicate_battle_ready, cli_out);

    //如果副本状态不是等待所有人装备 则准备无效
    if (player->dup_entity->state != DUPLICATE_STATE_WAIT_PLAYER_READY) {
        return 0;
    }

    player->player_dup_state = PLAYER_READY;

    // pve
    //通知其他人准备好
    onlineproto::sc_0x0206_duplicate_notify_battle_ready noti_msg;
    noti_msg.add_userid_list(player->uid);
    relay_notify_msg_to_entity_line_except(player->dup_entity, player->line_id,
            cli_cmd_cs_0x0206_duplicate_notify_battle_ready,
            noti_msg, player);

    if (g_duplicate_conf_mgr.get_duplicate_type(player->dup_entity->dup_id) ==
            DUP_BTL_TYPE_WORLD_BOSS) {//世界boss的准备不触发所有人准备好
        return 0;
    }
    //触发全准备好条件
    duplicate_entity_trig(player->dup_entity, (uint32_t)dup_cond_type_all_ready);

    return 0;
}

int DuplicateRelayCmdProcessor::proc_duplicate_exit(
        player_t *player, const string &pkg)
{
    TRACE_TLOG("Player:%u relay exit", player->uid);
    //玩家退出副本
    player->player_dup_state = PLAYER_LEAVE;
    DEBUG_TLOG("P:%u, ACTION: exit dup:%u", player->uid, player->dup_entity->dup_id);
    //回包
    onlineproto::sc_0x0208_duplicate_exit cli_out;
    relay_msg_to_player(player, cli_cmd_cs_0x0208_duplicate_exit, cli_out);

    //通知其他人玩家已经离开
    //onlineproto::sc_0x0204_duplicate_notify_leave_map noti_msg;
    //noti_msg.add_userid_list(player->uid);
    //relay_notify_msg_to_entity_except(player->dup_entity, 
            //cli_cmd_cs_0x0204_duplicate_notify_leave_map, noti_msg, player);

    // TODO toby 分线test
    onlineproto::sc_0x0209_duplicate_notify_exit noti_msg;
    noti_msg.add_userid_list(player->uid);
    relay_notify_msg_to_entity_line_except(player->dup_entity, player->line_id,
            cli_cmd_cs_0x0209_duplicate_notify_exit, noti_msg, player);

    duplicate_entity_t *entity = player->dup_entity;
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(entity->dup_id);

    //世界boss中有玩家退出
    if (dup->mode == onlineproto::DUP_MODE_TYPE_WORLD_BOSS) {
        //删除player在dup_entity中的索引
        duplicate_entity_del_player(entity, player); 
        g_dup_entity_mgr->remove_battle_player(entity, player);
        g_player_manager->del_player(player);

    } else if (dup->battle_type == DUP_BTL_TYPE_PVEP) { 
        //PVEP玩家退出则让所有玩家死亡
        FOREACH((*(entity->battle_players)), it) {
            FOREACH((it->second), it2) {
                player_t *dest = *it2;
                dest->cur_hp = 0;
            }
        }
        //触发玩家死亡
        duplicate_entity_trig(entity, (uint32_t)dup_cond_type_player_dead);
        //触发所有玩家死亡
        duplicate_entity_trig(entity, (uint32_t)dup_cond_type_all_player_dead);
        //直接结束副本
        g_dup_entity_mgr->destroy_entity(entity);

    } else {

        //让玩家死亡
        player->cur_hp = 0;
        //触发玩家死亡
        duplicate_entity_trig(entity, (uint32_t)dup_cond_type_player_dead);
        //触发所有玩家死亡
        duplicate_entity_trig(entity, (uint32_t)dup_cond_type_all_player_dead);

        duplicate_entity_del_player(entity, player); 
        g_dup_entity_mgr->remove_battle_player(entity, player);
        g_player_manager->del_player(player);
        duplicate_entity_new_captain(entity);

        if (entity->state == DUPLICATE_STATE_WAIT_PLAYER_READY) {
            //触发剩余的人全准备好条件
            duplicate_entity_trig(entity, (uint32_t)dup_cond_type_all_ready);
        }

        //如果没有人在了 可以直接结束副本
        g_dup_entity_mgr->try_destroy_entity(entity);

    }

    return 0;
}

int DuplicateRelayCmdProcessor::proc_duplicate_hit_character(
        player_t *player, const string &pkg)
{
    TRACE_TLOG("Player:%u relay hit", player->uid);

    onlineproto::cs_0x020A_duplicate_hit_character cli_in;
    parse_message(pkg.c_str(), pkg.size(), &cli_in);

    if (player->dup_entity->state == DUPLICATE_STATE_WAIT_PLAYER_READY) {
        WARN_TLOG("Player:%u hit invalid, still wait player ready");
        onlineproto::sc_0x020A_duplicate_hit_character cli_out; 
        return relay_msg_to_player(player, cli_cmd_cs_0x020A_duplicate_hit_character, cli_out); 
    }

    if (cli_in.def_cur_hp() == 0) {//请求包中被打的人血量已经为0了//属于鞭尸
        onlineproto::sc_0x020A_duplicate_hit_character cli_out; 
        return relay_msg_to_player(player, cli_cmd_cs_0x020A_duplicate_hit_character, cli_out); 
    }

    uint32_t hit_cnt = 0;
    uint32_t def_id = 0;
    //PVE 攻击双方都会发攻击包
    //PVP 只有受击者发被攻击包
    //判定对应的对象是否合法
    //NOTI(singku)前端把builder特殊处理 但后台统一当mon处理
    uint32_t cli_in_def_type = cli_in.def_type();
    uint32_t cli_in_atk_type = cli_in.atk_type();
    if (cli_in.def_type() == DUP_OBJ_TYPE_BUILDER) {
        cli_in_def_type = DUP_OBJ_TYPE_MON;
    } else if (cli_in.def_type() == DUP_OBJ_TYPE_PET) {
        if (cli_in.def_id() == 0) {//野精灵 友军
            cli_in_def_type = DUP_OBJ_TYPE_MON;
        }
    }
    if (cli_in.atk_type() == DUP_OBJ_TYPE_PET) {
        if (cli_in.atk_id() == 0) {//友军攻击
            cli_in_atk_type = DUP_OBJ_TYPE_MON;
        }
    } else if (cli_in.atk_type() == DUP_OBJ_TYPE_BUILDER) {
        cli_in_atk_type = DUP_OBJ_TYPE_MON;
    }

    bool atk_artifacial = false;
    bool def_artifacial = false;
    duplicate_battle_type_t btl_type = g_duplicate_conf_mgr.get_duplicate_type(player->dup_entity->dup_id);

    if (btl_type == DUP_BTL_TYPE_PVEP) {
        if (cli_in.atk_id() == player->uid && cli_in.def_id() != player->uid) {
            atk_artifacial = false;
            def_artifacial = true;

        } else if (cli_in.atk_id() == player->uid && cli_in.def_id() == player->uid) {
            atk_artifacial = false;
            def_artifacial = false;

        } else if (cli_in.atk_id() != player->uid && cli_in.def_id() != player->uid) {
            atk_artifacial = true;
            def_artifacial = true;
        } else {
            atk_artifacial = true;
            def_artifacial = false;
        }
    }

#define PLAYER_MUST_ALIVE(uid, is_artifacial_u) \
    do {\
        player_t *dest = g_player_manager->get_player_by_uid(uid, is_artifacial_u);\
        if (!dest) {\
            return send_err_to_player(player, player->wait_cmd,\
                    cli_err_duplicate_btl_obj_not_found);\
        } \
        if (dest->cur_hp == 0) { \
            ERROR_TLOG("P:%u PLAYER_DEAD", uid); \
            /*鞭尸或者已经死亡*/\
            onlineproto::sc_0x020A_duplicate_hit_character cli_out; \
            return relay_msg_to_player(player, cli_cmd_cs_0x020A_duplicate_hit_character, cli_out); \
        }\
    } while(0)

#define PET_MUST_EXIST(uid, pet_create_tm, is_artifacial_u) \
    do {\
        player_t *dest = g_player_manager->get_player_by_uid(uid, is_artifacial_u);\
        if (!dest || (dest->fight_pets.count(pet_create_tm) == 0 && dest->switch_pets.count(pet_create_tm) == 0)) {\
            return send_err_to_player(player, player->wait_cmd,\
                    cli_err_duplicate_btl_obj_not_found);\
        }\
    } while(0)

#define PET_MUST_ALIVE(uid, pet_create_tm, is_artifacial_u) \
    do {\
        player_t *dest = g_player_manager->get_player_by_uid(uid, is_artifacial_u);\
        if (!dest || dest->fight_pets.count(pet_create_tm) == 0) {\
            return send_err_to_player(player, player->wait_cmd,\
                    cli_err_duplicate_btl_obj_not_found);\
        }\
        Pet &pet = (dest->fight_pets.find(pet_create_tm))->second;\
        if (pet.hp() == 0) {\
            ERROR_TLOG("P:%u PET_DEAD", uid); \
            /*鞭尸或者已经死亡*/\
            onlineproto::sc_0x020A_duplicate_hit_character cli_out; \
            return relay_msg_to_player(player, cli_cmd_cs_0x020A_duplicate_hit_character, cli_out); \
        }\
    } while(0)

#define MON_MUST_ALIVE(mon_create_tm) \
    do { \
        if (player->dup_entity->cur_map_enemy->count(mon_create_tm) == 0 \
            && player->dup_entity->cur_map_non_enemy->count(mon_create_tm) == 0) {\
            /*鞭尸或者已经死亡*/\
            ERROR_TLOG("MON_DEAD"); \
            onlineproto::sc_0x020A_duplicate_hit_character cli_out; \
            return relay_msg_to_player(player, cli_cmd_cs_0x020A_duplicate_hit_character, cli_out); \
        }\
    } while(0)

#define MUST_DIFF_TEAM \
    do { \
        duplicate_actor_team_t atk_team; \
        duplicate_actor_team_t def_team; \
        atk_team = get_team_by_type(player->dup_entity, cli_in_atk_type, cli_in.atk_create_tm());\
        def_team = get_team_by_type(player->dup_entity, cli_in_def_type, cli_in.def_create_tm());\
        if (atk_team == def_team) { \
            return send_err_to_player(player, player->wait_cmd, cli_err_duplicate_atk_obj_invalid); \
        } \
    } while(0)

    if (cli_in_atk_type == DUP_OBJ_TYPE_PLAYER) { //攻击者是玩家
        //TODO(singku) 由于前后端判定的不一致 把玩家攻击时必须活着的判定取消
        //PLAYER_MUST_ALIVE((cli_in.atk_id()), atk_artifacial);

    } else if (cli_in_atk_type == DUP_OBJ_TYPE_PET) { //攻击者是精灵
        PET_MUST_EXIST((cli_in.atk_id()), (cli_in.atk_create_tm()), atk_artifacial);

    } else if (cli_in_atk_type == DUP_OBJ_TYPE_MON) { //攻击者是野怪
        MON_MUST_ALIVE((cli_in.atk_create_tm()));
    }

    if (cli_in_def_type == DUP_OBJ_TYPE_PET) { //防御者是精灵
        PET_MUST_EXIST((cli_in.def_id()), (cli_in.def_create_tm()), def_artifacial);

    } else if (cli_in_def_type == DUP_OBJ_TYPE_PLAYER) { //防御者是玩家
        PLAYER_MUST_ALIVE((cli_in.def_id()), def_artifacial);

    } else if (cli_in_def_type == DUP_OBJ_TYPE_MON) { //防御者是野怪
        MON_MUST_ALIVE((cli_in.def_create_tm()));
    }

    //攻防双方不能同team
    //MUST_DIFF_TEAM;

    //野怪是否死亡
    bool cur_mon_dead = false;
    bool boss_dead = false;
    uint32_t dead_boss_id = 0;
    bool wild_dead = false;

    // 野怪血量
    uint32_t mon_cur_hp = 0;
    uint32_t mon_max_hp = 0;

    bool obj_dead = false;
    bool sync_dead_state = false;
    battleproto::sc_battle_duplicate_notify_kill_character noti_kill_msg;
    noti_kill_msg.set_pos_x(cli_in.x_pos());
    noti_kill_msg.set_pos_y(cli_in.y_pos());
    noti_kill_msg.set_def_type(cli_in.def_type());
    noti_kill_msg.set_is_player_dead(false);

    if ((cli_in_def_type == DUP_OBJ_TYPE_PLAYER || cli_in_def_type == DUP_OBJ_TYPE_PET)
        && cli_in.def_id() != player->uid && btl_type != DUP_BTL_TYPE_PVEP) {
        //存在多人参战的副本
        //别的玩家报告某玩家被打, 以这个玩家自己提交的结果为准,服务器只转发这个攻击
        ;//do nothing

    } else if (cli_in_def_type == DUP_OBJ_TYPE_PLAYER) { //玩家受击
        player_t *dest = g_player_manager->get_player_by_uid(cli_in.def_id(), def_artifacial);
        bool player_dead = false;
        if (cli_in.is_dead()) {
            player_dead = true;
            dest->cur_hp = 0;
        } else if (cli_in.damage() < 0) {
            duplicate_entity_trig(player->dup_entity, (uint32_t)dup_cond_type_player_hp_less, 0);
            if ((int)dest->cur_hp <= (-1)*cli_in.damage()) {
                //TODO(singku) 后台认为死了 前端认为没死 可能有加血BUFF 则后台血量强制置1
                //player_dead = true;
                dest->cur_hp = 1;
            } else {
                dest->cur_hp -= (-1)*cli_in.damage();
            }

        } else {
            dest->cur_hp += cli_in.damage();
            if (dest->cur_hp > dest->max_hp) {
                dest->cur_hp = dest->max_hp;
            }
        }

        if (player_dead) {
            obj_dead = true;
            sync_dead_state = true;
            noti_kill_msg.set_def_id(dest->uid);
            noti_kill_msg.set_def_create_tm(dest->create_tm);

            dest->cur_hp = 0;
            dest->player_dup_state = PLAYER_DEAD;
            uint32_t dup_id = dest->dup_entity ? (dest->dup_entity->dup_id):0; 
            const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
            if (dup && dup->battle_type == DUP_BTL_TYPE_WORLD_BOSS) {
                ;//ppve
            } else {
                duplicate_entity_trig(dest->dup_entity, (uint32_t)dup_cond_type_player_dead);
                duplicate_entity_trig(dest->dup_entity, (uint32_t)dup_cond_type_all_player_dead);
            }
        }

        mon_cur_hp = dest->cur_hp;
        mon_max_hp = dest->max_hp;
    } else if (cli_in_def_type == DUP_OBJ_TYPE_PET) { //精灵受击
        player_t *dest = g_player_manager->get_player_by_uid(cli_in.def_id(), def_artifacial);
        Pet &pet = (dest->fight_pets.find(cli_in.def_create_tm()))->second;
        bool pet_dead = false;
        if (cli_in.is_dead()) {
            pet_dead = true;
            pet.set_hp(0);
        } else if (cli_in.damage() < 0) {
            if (pet.hp() <= (-1) * cli_in.damage()) {
                //TODO(singku) 后台认为死了 前端认为没死 可能有加血BUFF 则后台血量强制置1
                pet.set_hp(1);
                //pet_dead = true;
            } else {
                pet.change_hp(cli_in.damage());
            }

        } else {
            pet.change_hp(cli_in.damage());
        }

        if (pet_dead) {
            pet.set_hp(0);
            obj_dead = true;
            sync_dead_state = true;
            noti_kill_msg.set_def_id(dest->uid);
            noti_kill_msg.set_def_create_tm(pet.create_tm());
            //通知精灵死亡
            /*
            battleproto::sc_battle_duplicate_notify_kill_character noti_msg;
            noti_msg.set_def_type(cli_in.def_type());
            noti_msg.set_def_id(dest->uid);
            noti_msg.set_def_create_tm(pet.create_tm());
            noti_msg.set_pos_x(cli_in.x_pos());
            noti_msg.set_pos_y(cli_in.y_pos());
            send_msg_to_entity_except(player->dup_entity, btl_cmd_notify_kill_character, noti_msg);
            */
        }

        mon_cur_hp = pet.hp();
        mon_max_hp = pet.max_hp();
    } else if (cli_in_def_type == DUP_OBJ_TYPE_MON){ //野怪受击
        //TODO(singku) 玩家精灵输出伤害检验
        duplicate_map_pet_t *dpet = get_entity_map_pet_by_create_tm(player->dup_entity, cli_in.def_create_tm());
        def_id = dpet->pet_id;
        hit_cnt = ++(dpet->hit_cnt);
        if (cli_in.is_dead()) {
            wild_dead = true;
            dpet->cur_hp = 0;
        } if (cli_in.damage() < 0) {
            if ((int)dpet->cur_hp <= (-1) * cli_in.damage()) {
                //TODO(singku) 后台认为死了 前端认为没死 可能有加血BUFF 则后台血量强制置1
                const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(
                        player->dup_entity->dup_id);
                if (dup && dup->mode == onlineproto::DUP_MODE_TYPE_WORLD_BOSS) {
                    dpet->cur_hp = 0;
                    wild_dead = true;
                } else {
                    dpet->cur_hp = 1;
                    //wild_dead = true;
                }
            } else {
                dpet->cur_hp -= (-1) * cli_in.damage();
            }

        } else {
            dpet->cur_hp += cli_in.damage();
            if (dpet->cur_hp > dpet->max_hp) {
                dpet->cur_hp = dpet->max_hp;
            }
        }

        if (dpet->mon_type == MON_TYPE_BOSS) {
            duplicate_entity_trig(player->dup_entity, (uint32_t)dup_cond_type_boss_hp_less, 0);
        }

        if (wild_dead) {
            obj_dead = true;
            player->dup_entity->cur_dead_obj_key = dpet->uniq_key;

            if (dpet->phase != player->dup_entity->cur_map_phase) {
                ;
            } else {//当前阶段敌人死亡
                player->dup_entity->cur_dead_obj_idx = dpet->born_area_idx;
                uint32_t cnt = (*(player->dup_entity->born_area_dead_record))[dpet->born_area_idx];
                (*(player->dup_entity->born_area_dead_record))[dpet->born_area_idx] = cnt+1;

                cur_mon_dead = true;
                if (dpet->mon_type == MON_TYPE_BOSS) {
                    boss_dead = true;
                    dead_boss_id = dpet->pet_id;
                    //如果有boss生存定时器
                    if (player->dup_entity->boss_show_timer) {
                        REMOVE_TIMER(player->dup_entity->boss_show_timer);
                    }
                }

                if (dpet->team == DUP_ACTOR_TEAM_ENEMY) {
                    player->dup_entity->cur_phase_enemy_num --;
                }

                if (dpet->no_stat == 0) {
                    player->dup_entity->cur_phase_dead_pet_num++;
#if 1
                    DEBUG_TLOG("P:%u, DUP_STAT: dup:%u map:%u phase:%u kill:%u(tm-%u) total_kill:%u rest:%u",
                            player->uid, player->dup_entity->dup_id,
                            player->dup_entity->cur_map_id,
                            player->dup_entity->cur_map_phase,
                            dpet->pet_id,
                            cli_in.def_create_tm(),
                            player->dup_entity->cur_phase_dead_pet_num,
                            player->dup_entity->cur_map_enemy->size() - 1);
#endif
                } 
            }
            if (cli_in.kill_by_me() == true) {
                sync_dead_state = true;
                noti_kill_msg.set_def_id(dpet->pet_id);
                noti_kill_msg.set_def_create_tm(dpet->create_tm);
                //给玩家发通知包 杀死了怪
                /*
                battleproto::sc_battle_duplicate_notify_kill_character noti_msg;
                noti_msg.set_def_type(cli_in.def_type());
                noti_msg.set_def_id(dpet->pet_id);
                noti_msg.set_def_create_tm(dpet->create_tm);
                noti_msg.set_pos_x(cli_in.x_pos());
                noti_msg.set_pos_y(cli_in.y_pos());
                if (player->cur_hp == 0) {
                    noti_msg.set_is_player_dead(true);
                }
                send_msg_to_entity_except(player->dup_entity, 
                        btl_cmd_notify_kill_character, 
                        noti_msg);
                */
            }
            //死了删掉
            player->dup_entity->cur_map_enemy->erase(cli_in.def_create_tm());
            player->dup_entity->cur_map_non_enemy->erase(cli_in.def_create_tm());
        }

        mon_cur_hp = dpet->cur_hp;
        mon_max_hp = dpet->max_hp;
    }
    //回包
    onlineproto::sc_0x020A_duplicate_hit_character cli_out;
    relay_msg_to_player(player, cli_cmd_cs_0x020A_duplicate_hit_character, cli_out);
    //通知其他人 有人攻击,并把攻击信息返回给自己
    onlineproto::sc_0x020B_duplicate_notify_hit_character noti_msg;
    noti_msg.set_atk_type(cli_in.atk_type());
    noti_msg.set_atk_id(cli_in.atk_id());
    noti_msg.set_atk_create_tm(cli_in.atk_create_tm());
    noti_msg.set_def_type(cli_in.def_type());
    noti_msg.set_def_id(def_id ?def_id :cli_in.def_id());
    noti_msg.set_def_create_tm(cli_in.def_create_tm());
    noti_msg.set_skill_id(cli_in.skill_id());
    noti_msg.set_damage(cli_in.damage());
    noti_msg.set_is_dead(cli_in.is_dead());
    noti_msg.set_x_pos(cli_in.x_pos());
    noti_msg.set_y_pos(cli_in.y_pos());
    noti_msg.set_heading(cli_in.heading());
    noti_msg.set_hit_cnt(hit_cnt);
    noti_msg.set_mon_cur_hp(mon_cur_hp);
    noti_msg.set_mon_max_hp(mon_max_hp);

    if (btl_type == DUP_BTL_TYPE_RPVP) {
        relay_notify_msg_to_entity_except(player->dup_entity, 
                cli_cmd_cs_0x020B_duplicate_notify_hit_character, 
                noti_msg, player);
    } else {
        relay_notify_msg_to_entity_except(player->dup_entity, 
                cli_cmd_cs_0x020B_duplicate_notify_hit_character, 
                noti_msg, 0);
    }


    if (obj_dead && sync_dead_state) {
        send_msg_to_entity_except(player->dup_entity, btl_cmd_notify_kill_character, noti_kill_msg);

        onlineproto::sc_0x022F_duplicate_notify_kill_character relay_kill_msg;
        relay_kill_msg.set_def_type(noti_kill_msg.def_type());
        relay_kill_msg.set_def_id(noti_kill_msg.def_id());
        relay_kill_msg.set_def_create_tm(noti_kill_msg.def_create_tm());
        relay_kill_msg.set_pos_x(noti_kill_msg.pos_x());
        relay_kill_msg.set_pos_y(noti_kill_msg.pos_y());
        relay_kill_msg.set_is_player_dead(noti_kill_msg.is_player_dead());
        relay_notify_msg_to_entity_except(player->dup_entity, cli_cmd_cs_0x022F_duplicate_notify_kill_character,
                relay_kill_msg, 0);
    }
    //如果副本可以结束了
    if (player->dup_entity->state == DUPLICATE_STATE_CAN_END) {
        g_dup_entity_mgr->destroy_entity(player->dup_entity);
        return 0;
    }

    //如果野怪死亡 触发相关触发器
    if (wild_dead) {
        //如果boss死亡
        if (boss_dead) {
            const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(
                    player->dup_entity->dup_id);
            uint32_t index = 0;
            for (uint32_t i = 0; i < dup->mon_vec.size(); i++) {
                if (dup->mon_vec[i] == dead_boss_id) {
                    index = i+1;
                    break;
                }
            }

            for (uint32_t i = 0; i < dup->builder_vec.size(); i++) {
                if (dup->builder_vec[i] == dead_boss_id) {
                    index = i+1;
                    break;
                }
            }

            player->dup_entity->cur_map_dead_boss_set->insert(index);
            //触发boss_dead
            std::vector<uint32_t> args;
            args.push_back(index);
            duplicate_entity_trig(player->dup_entity, (uint32_t)dup_cond_type_phase_kill_boss, &args);
            if (player->dup_entity->state == DUPLICATE_STATE_CAN_END) {
                g_dup_entity_mgr->destroy_entity(player->dup_entity);
                return 0;
            }
        }

        //由于phase_kill可能会触发endphase endphase会清理当前阶段的杀怪数据
        //所以必修先触发monster_kill判断要不要刷怪 再触发phase_kill
        //触发monster_kill
        duplicate_entity_trig(player->dup_entity, (uint32_t)dup_cond_type_mon_killed);

        if (cur_mon_dead) {
            //触发phase_kill
            duplicate_entity_trig(player->dup_entity, (uint32_t)dup_cond_type_phase_kill);
        }

        //如果副本可以结束了
        if (player->dup_entity->state == DUPLICATE_STATE_CAN_END) {
            g_dup_entity_mgr->destroy_entity(player->dup_entity);
            return 0;
        }
    }
    return 0;
}

int DuplicateRelayCmdProcessor::proc_duplicate_to_next_phase(
        player_t *player, const string &pkg)
{
    TRACE_TLOG("Player:%u relay to_next_phase", player->uid);

    //只有pve才可以,不然很多人同时发next_phase服务器不知道该怎么办
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(
            player->dup_entity->dup_id);
    if (dup->battle_type != DUP_BTL_TYPE_PVE) {
        return send_err_to_player(player, player->wait_cmd,
                cli_err_duplicate_not_pve);
    }
    onlineproto::cs_0x0211_duplicate_to_next_phase cli_in;
    cli_in.ParseFromString(pkg);
    //正常回包
    onlineproto::sc_0x0211_duplicate_to_next_phase cli_out;
    relay_msg_to_player(player, cli_cmd_cs_0x0211_duplicate_to_next_phase, cli_out);
 
    //开始下一阶段的处理   
    if (cli_in.has_phase_id() && cli_in.phase_id()) {
        /*
        if (cli_in.phase_id() <= player->dup_entity->cur_map_phase) {
            //要去的阶段小于等于当前处于的阶段
            WARN_TLOG("P:%u try to phase:%u current_phase_is:%u",
                    player->uid, cli_in.phase_id(), player->dup_entity->cur_map_phase);
            return 0;
        }
        */
        player->dup_entity->cur_map_phase = cli_in.phase_id();
    } else {
        player->dup_entity->cur_map_phase++;
    }
    DEBUG_TLOG("P:%u, ACTION: start phase:%u", player->uid, player->dup_entity->cur_map_phase);
    player->dup_entity->born_area_born_record->clear();
    player->dup_entity->born_area_dead_record->clear();
    player->dup_entity->cur_phase_enemy_num = 0;
    player->dup_entity->cur_phase_born_obj_cnt = 0;
    player->dup_entity->cur_phase_dead_pet_num = 0;
    player->dup_entity->cur_phase_boss_out = 0;

    if (player->dup_entity->phase_timer) {
        REMOVE_TIMER(player->dup_entity->phase_timer);
        player->dup_entity->phase_timer = NULL;
    }

    //到下一阶段的通知包
    onlineproto::sc_0x0213_duplicate_notify_to_next_phase noti_msg;
    noti_msg.set_new_phase(player->dup_entity->cur_map_phase);
    relay_notify_msg_to_player(player, cli_cmd_cs_0x0213_duplicate_notify_to_next_phase, noti_msg);

    //抛出条件
    std::vector<uint32_t> args;
    args.push_back(player->dup_entity->cur_map_phase);
    duplicate_entity_trig(player->dup_entity, (uint32_t)dup_cond_type_start_phase, &args);

    
    return 0;
}

int DuplicateRelayCmdProcessor::proc_duplicate_change_state(
        player_t *player, const string &pkg)
{
    TRACE_TLOG("Player:%u relay change_state", player->uid);

    onlineproto::cs_0x0106_player_change_state cli_in;
    onlineproto::sc_0x0106_player_change_state cli_out;

    cli_in.ParseFromString(pkg);
    uint32_t type = cli_in.type();
    uint32_t create_tm = cli_in.create_tm();
    if (type == 1) { //玩家
        player->state_bytes.assign(cli_in.state_bytes());
    } else if (type == 2) { //精灵
        if (player->fight_pets.count(create_tm)) {
            Pet &pet = (player->fight_pets.find(create_tm))->second;
            pet.set_state_bytes(cli_in.state_bytes());
        }
    } else { //野怪
        if (!player->is_captain) {//不是队长 不同步野怪到其他玩家
            return 0; //return relay_msg_to_player(player, cli_cmd_cs_0x0106_player_change_state, cli_out);
        }
    }

    //同步状态到其他玩家
    onlineproto::sc_0x0107_notify_player_change_state noti_msg;
    noti_msg.set_type(type);
    noti_msg.set_userid(player->uid);
    noti_msg.set_create_tm(create_tm);
    noti_msg.set_state_bytes(cli_in.state_bytes());
    //relay_notify_msg_to_entity_except(player->dup_entity, 
    //cli_cmd_cs_0x0107_notify_player_change_state, noti_msg);

    // TODO toby 分线test
    relay_notify_msg_to_entity_line_except(player->dup_entity, player->line_id,
            cli_cmd_cs_0x0107_notify_player_change_state, noti_msg, 0);

    return 0;
}

int DuplicateRelayCmdProcessor::proc_duplicate_switch_fight_pet(
        player_t *player, const string &pkg)
{
    TRACE_TLOG("Player:%u relay switch_pet", player->uid);

    onlineproto::cs_0x0229_duplicate_switch_fight_pet cli_in;
    cli_in.ParseFromString(pkg);
    if (player->fight_pets.count(cli_in.on_pet_create_tm())) {
        return send_err_to_player(player, player->wait_cmd, cli_err_pet_already_in_fight_pos);
    }
    if (!player->switch_pets.count(cli_in.on_pet_create_tm())) {
        return send_err_to_player(player, player->wait_cmd, cli_err_pet_can_not_fight);
    }
    if (cli_in.off_pet_create_tm() && !player->fight_pets.count(cli_in.off_pet_create_tm())) {
        return send_err_to_player(player, player->wait_cmd, cli_err_target_pet_not_exist);
    }
    if (cli_in.off_pet_create_tm() == cli_in.on_pet_create_tm()) {
        return send_err_to_player(player, player->wait_cmd, cli_err_op_the_same_pet);
    }
    
    //都可以切换了
    if (player->fight_pets.count(cli_in.off_pet_create_tm())) {
        Pet pet = (player->fight_pets.find(cli_in.off_pet_create_tm()))->second;
        player->fight_pets.erase(cli_in.off_pet_create_tm());
        player->switch_pets[pet.create_tm()] = pet;
    }
    Pet pet = (player->switch_pets.find(cli_in.on_pet_create_tm()))->second;
    pet.set_fight_pos(cli_in.pos());
    player->fight_pets[pet.create_tm()] = pet;
    player->switch_pets.erase(cli_in.on_pet_create_tm());
    //online发起的不等传递包 不用回包
    //onlineproto::sc_0x0229_duplicate_switch_fight_pet cli_out;
    //relay_msg_to_player(player, cli_cmd_cs_0x0229_duplicate_switch_fight_pet, cli_out);
    //通知包
    onlineproto::sc_0x0230_duplicate_notify_switch_fight_pet noti_msg;
    noti_msg.set_uid(player->uid);
    noti_msg.set_off_pet_create_tm(cli_in.off_pet_create_tm());
    DataProtoUtils::pack_pet_info(&pet, noti_msg.mutable_on_pet()->mutable_pet_info());
    //relay_notify_msg_to_entity_except(player->dup_entity, cli_cmd_cs_0x0230_duplicate_notify_switch_fight_pet,
            //noti_msg, 0);

    // TODO toby 分线test
    relay_notify_msg_to_entity_line_except(player->dup_entity, player->line_id,
            cli_cmd_cs_0x0230_duplicate_notify_switch_fight_pet, noti_msg, player);

 
    return 0;
}

int DuplicateRelayCmdProcessor::proc_duplicate_front_mon_flush_req(
        player_t *player, const string &pkg)
{
    TRACE_TLOG("Player:%u relay req_mon_flush", player->uid);

    onlineproto::cs_0x022B_duplicate_mon_flush_request cli_in;
    cli_in.ParseFromString(pkg);

    //抛出条件
    std::vector<uint32_t> args;
    args.push_back(cli_in.index());
    duplicate_entity_trig(player->dup_entity, (uint32_t)dup_cond_type_front_mon_flush_req, &args);

    return 0;
}

int DuplicateRelayCmdProcessor::proc_duplicate_skill_affect(
        player_t *player, const string &pkg)
{
    TRACE_TLOG("Player:%u relay skill_affect", player->uid);

    onlineproto::cs_0x022D_skill_affect cli_in;
    cli_in.ParseFromString(pkg);

    player->dup_entity->skill_affect_obj_type = cli_in.obj_type();
    player->dup_entity->skill_affect_obj_create_tm = cli_in.create_tm();
    std::vector<uint32_t> args;
    args.push_back(cli_in.affect_key());
    //抛出条件
    switch(cli_in.affect_type()) {
    case onlineproto::SKILL_CALL_MON:
        duplicate_entity_trig(player->dup_entity, (uint32_t)dup_cond_type_use_skill, &args);
        break;
    default:
        break;
    }

    player->dup_entity->skill_affect_obj_type = 0;
    player->dup_entity->skill_affect_obj_create_tm = 0;;

    return 0;
}

int EnterDuplicateCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    //判断副本是否存在
    if (!g_duplicate_conf_mgr.duplicate_exist(cli_in_.dup_id())) {
        return send_err_to_player(player, player->wait_cmd,
                cli_err_duplicate_id_not_found);
    }
    //判断地图是否存在
    if (!g_map_conf_mgr.is_map_conf_exist(cli_in_.map_id())) {
        return send_err_to_player(player, player->wait_cmd,
                cli_err_duplicate_map_id_not_found);
    }
    //判断这个副本有没有这个地图
    if (!g_duplicate_conf_mgr.dup_has_map(cli_in_.dup_id(), cli_in_.map_id())) {
        return send_err_to_player(player, player->wait_cmd,
                cli_err_duplicate_map_id_not_found);
    }
    cli_out_.Clear();
    cli_out_.set_dup_id(cli_in_.dup_id());
    cli_out_.set_map_id(cli_in_.map_id());
    cli_out_.set_re_enter(cli_in_.re_enter());
    player->x_pos = cli_in_.player().x_pos();
    player->y_pos = cli_in_.player().y_pos();

    if (player->dup_entity) { //玩家已在副本
        if (cli_in_.re_enter()) { //重新进入
            if (player->dup_entity->dup_id != cli_in_.dup_id()
                || player->dup_entity->cur_map_id != cli_in_.map_id()) {
                return send_err_to_player(player, player->wait_cmd,
                        cli_err_player_already_in_other_dup);
            } else {//接着打
                DataProtoUtils::pack_duplicate_all_object(player, player->dup_entity, cli_out_);
                send_msg_to_player(player, player->wait_cmd, cli_out_);
                player->player_dup_state = PLAYER_PLAY;
                return 0;
            }
        } else if (player->dup_entity->dup_id != cli_in_.dup_id()) {
            //已在副本而且不是重新进入 而且是进入不同的副本ID
            return send_err_to_player(player, player->wait_cmd,
                    cli_err_player_already_in_other_dup);
        } else if (player->dup_entity->cur_map_id == cli_in_.map_id()){
            //已经在副本而且不是重新进入 而且进入同一个副本的同一个场景
            //不接收继续进入副本的发包
            return send_err_to_player(player, player->wait_cmd,
                    cli_err_player_already_in_other_dup);
        } else { 
            //场景正常切换
            //错误的切换(以服务器所在场景为准)
            if (player->dup_entity->ready_map_id != cli_in_.map_id()) {
                return send_err_to_player(player, player->wait_cmd,
                        cli_err_player_duplicate_map_not_exist);
            }
            //触发场景切换事件
            std::vector<uint32_t> args;
            args.push_back(cli_in_.map_id());
            duplicate_entity_trig(player->dup_entity, (uint32_t)dup_cond_type_into_scene, &args);

            DataProtoUtils::pack_duplicate_all_object(player, player->dup_entity, cli_out_);
            return send_msg_to_player(player, player->wait_cmd, cli_out_);
        }

    //新的副本
    } else {
        DataProtoUtils::unpack_player_battle_all_info(player, cli_in_.player());
        player->is_captain = true;
        //TODO(singku) 判断是否进入已经存在的副本如WORLD_BOSS
        //这些副本可能不需要新创建副本 只需要进入已有的副本
        duplicate_entity_t *entity = NULL;
		duplicate_battle_type_t dup_type = g_duplicate_conf_mgr.get_duplicate_type(cli_in_.dup_id());

        //世界boss如果已有副本则不需要重新创建
        if (dup_type == DUP_BTL_TYPE_WORLD_BOSS) {
            // online和battle是1:1部署，还需要传入mapid lineid做ppve副本实体分组条件
            entity = g_dup_entity_mgr->get_world_boss_entity(); 
            if (entity == NULL) {
                // 第一个进入玩家创建新副本实体
                entity = g_dup_entity_mgr->create_entity(cli_in_.dup_id(), cli_in_.map_id());
                if (!entity) {
                    return send_err_to_player(player, player->wait_cmd, cli_err_sys_err);
                }
            }
            //世界boss玩家进来就准备好
            player->player_dup_state = PLAYER_READY;

        } else {
            entity = g_dup_entity_mgr->create_entity(cli_in_.dup_id(), cli_in_.map_id());
            if (!entity) {
                return send_err_to_player(player, player->wait_cmd, cli_err_sys_err);
            }
        }

        std::set<player_t*> all_players;

        //TODO(singku)选边 默认为SIDE1
        uint32_t side = SIDE1;
        if (entity->battle_players->count(side) == 0) {
            std::set<player_t *> p_set;
            p_set.insert(player);
            entity->battle_players->insert(make_pair(side, p_set));
        } else {
            std::set<player_t *> &p_set = (entity->battle_players->find(side))->second;
            p_set.insert(player);
        }
        // TODO toby 副本分线实现
        player->line_id = g_dup_entity_mgr->add_battle_player(entity, side, player);
        player->side = (player_duplicate_side_t)side;
        player->dup_entity = entity;
   
        all_players.insert(player);

        //如果是实时竞技场/半手动竞技场 则还有二号敌对玩家
        if (dup_type == DUP_BTL_TYPE_RPVP || dup_type == DUP_BTL_TYPE_PVEP) {
            player_t *player2 = 0;
            if (dup_type == DUP_BTL_TYPE_RPVP) {
                player2 = g_player_manager->create_new_player(cli_in_.other_players(0).base_info().user_id(), player->fdsess);
            } else {
                player2 = g_player_manager->create_artifacial_player(cli_in_.other_players(0).base_info().user_id(), player->fdsess);
            }
            player2->wait_cmd = player->wait_cmd;
            player2->cur_seq = player->cur_seq;
            DataProtoUtils::unpack_player_battle_all_info(player2, cli_in_.other_players(0));
            //二号玩家选边
            uint32_t side = SIDE2;
            if (entity->battle_players->count(side) == 0) {
                std::set<player_t *> p_set;
                p_set.insert(player2);
                entity->battle_players->insert(make_pair(side, p_set));
            } else {
                std::set<player_t *> &p_set = (entity->battle_players->find(side))->second;
                p_set.insert(player2);
            }
            // TODO toby 副本分线实现
            player2->line_id = g_dup_entity_mgr->add_battle_player(entity, side, player2);
            player2->side = (player_duplicate_side_t)side;
            player2->dup_entity = entity;
            all_players.insert(player2);
			//多人打野 同一边的其他玩家
        } else if (dup_type == DUP_BTL_TYPE_PPVE || 
				dup_type == DUP_BTL_TYPE_PEVE) {//玩家带ai打野
            player_t *team_player = 0;
            for (int i = 0; i < cli_in_.other_players_size(); i++) {
				if(dup_type == DUP_BTL_TYPE_PPVE){
					team_player = g_player_manager->create_new_player(cli_in_.other_players(i).base_info().user_id(), player->fdsess);
				} else {
					team_player = g_player_manager->create_artifacial_player(cli_in_.other_players(i).base_info().user_id(), player->fdsess);
				}
                team_player->wait_cmd = player->wait_cmd;
                team_player->cur_seq = player->cur_seq;
                DataProtoUtils::unpack_player_battle_all_info(team_player, cli_in_.other_players(i));
                uint32_t side = SIDE1;
                if (entity->battle_players->count(side) == 0) {
                    std::set<player_t *> p_set;
                    p_set.insert(team_player);
                    entity->battle_players->insert(make_pair(side, p_set));
                } else {
                    std::set<player_t *> &p_set = (entity->battle_players->find(side))->second;
                    p_set.insert(team_player);
                }
                // TODO toby 副本分线实现
                team_player->line_id = g_dup_entity_mgr->add_battle_player(entity, side, team_player);
                team_player->side = (player_duplicate_side_t)side;
                team_player->dup_entity = entity;
                all_players.insert(team_player);
            }
        }

        if (dup_type == DUP_BTL_TYPE_WORLD_BOSS) {
            // 通知其他玩家有人进入副本
            onlineproto::sc_0x0202_duplicate_notify_enter_map noti_msg;
            DataProtoUtils::pack_player_battle_all_info(player, noti_msg.mutable_player());
            // TODO toby 分线test
            relay_notify_msg_to_entity_line_except(player->dup_entity, player->line_id,
                    cli_cmd_cs_0x0202_duplicate_notify_enter_map, noti_msg, player);
        } else {
            std::vector<uint32_t> args;
            args.push_back(cli_in_.map_id());
            duplicate_entity_trig(entity, (uint32_t)dup_cond_type_into_scene, &args);
        }

        DataProtoUtils::pack_duplicate_all_object(player, player->dup_entity, cli_out_);

        FOREACH(all_players, it) {
            player_t *p = *it;
            if (p->player_dup_state != PLAYER_READY) {//世界boss玩家进来就准备好
                p->player_dup_state = PLAYER_IN;
            }
        }
        
        send_msg_to_entity_except(player->dup_entity, player->wait_cmd, cli_out_);
    }
    return 0;
}

int RevivalDuplicateCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    if (!player->dup_entity) {
        return send_err_to_player(player, player->wait_cmd,
                cli_err_player_not_in_this_duplicate);
    }
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(
            player->dup_entity->dup_id);
    if (!dup) {
        return send_err_to_player(player, player->wait_cmd,
                cli_err_duplicate_id_not_found);
    }
    
    DEBUG_TLOG("P:%u, ACTION: Revival", player->uid);

    if (cli_in_.type() == 1) { //玩家
        player->cur_hp = player->max_hp;
        player->cur_tp = player->max_tp;
    } else if (cli_in_.type() == 2) { //精灵
        if (player->fight_pets.count(cli_in_.create_tm()) == 0) {
            return send_err_to_player(player, player->wait_cmd,
                    cli_err_revival_pet_not_found);
        }
        Pet *pet = &((player->fight_pets.find(cli_in_.create_tm()))->second);
        pet->set_hp(pet->max_hp());
    } else if (cli_in_.type() == 3) { //
        player->cur_hp = player->max_hp;
        player->cur_tp = player->max_tp;
        FOREACH(player->fight_pets, it) {
            Pet &pet = it->second;
            pet.set_hp(pet.max_hp());
        }
    }
    cli_out_.set_type(cli_in_.type());
    cli_out_.set_id(cli_in_.id());
    cli_out_.set_create_tm(cli_in_.create_tm());
    send_msg_to_player(player, player->wait_cmd, cli_out_);

    // TODO toby 分线test
    onlineproto::sc_0x0212_duplicate_revival revival_out_;
    revival_out_.set_type(cli_in_.type());
    revival_out_.set_id(cli_in_.id());
    revival_out_.set_create_tm(cli_in_.create_tm());
    relay_notify_msg_to_entity_line_except(player->dup_entity, player->line_id,
            cli_cmd_cs_0x0212_duplicate_revival, revival_out_, player);

    return 0;
}

int DuplicateTrigCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(cli_in_.dup_id());
    if (dup->battle_type == DUP_BTL_TYPE_WORLD_BOSS && 
            dup->mode == onlineproto::DUP_MODE_TYPE_WORLD_BOSS) {
        // 世界boss副本创建，触发开始事件
        if (cli_in_.trig_type() == commonproto::WORLD_BOSS_TRIG_OPEN) {
            duplicate_entity_t *entity = NULL;
            entity = g_dup_entity_mgr->get_world_boss_entity(); 
            if (entity == NULL) {
                entity = g_dup_entity_mgr->create_entity(cli_in_.dup_id(), cli_in_.map_id());
                if (!entity) {
                    return send_err_to_player(player, player->wait_cmd, cli_err_sys_err);
                }
            }

            if (entity->state == DUPLICATE_STATE_CREATE) {
                //先强制让玩家都准备好
                FOREACH((*(entity->battle_players)), it) {
                    std::set<player_t*> &p_set = it->second;
                    FOREACH(p_set, it2) {
                        player_t *player = *it2;
                        player->player_dup_state = PLAYER_READY;
                    }
                }

                std::vector<uint32_t> args;
                args.push_back(cli_in_.map_id());
                duplicate_entity_trig(entity, (uint32_t)dup_cond_type_into_scene, &args);
                duplicate_entity_trig(entity, (uint32_t)dup_cond_type_ready_scene, &args);
                duplicate_entity_trig(entity, (uint32_t)dup_cond_type_all_ready);
            }
        } else if (cli_in_.trig_type() == commonproto::WORLD_BOSS_TRIG_SHUTDOWN) {
        // 世界boss副本结束
            duplicate_entity_t *entity = NULL;
            entity = g_dup_entity_mgr->get_world_boss_entity(); 
            if (entity == NULL) {
                entity = g_dup_entity_mgr->create_entity(cli_in_.dup_id(), cli_in_.map_id());
                if (!entity) {
                    return send_err_to_player(player, player->wait_cmd, cli_err_sys_err);
                }
            }

            // 触发副本时间到结束事件
            entity->time_up = true;
            std::vector<uint32_t> args;
            duplicate_entity_trig(entity, (uint32_t)dup_cond_type_time_up, &args);
            entity->time_up = true;
            //如果副本可以结束了
            if (entity->state == DUPLICATE_STATE_CAN_END) {
                g_dup_entity_mgr->destroy_entity(entity);
            }

        }
    }

    return 0;
}
