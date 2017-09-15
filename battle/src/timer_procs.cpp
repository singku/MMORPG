#include "common.h"
#include "timer_procs.h"
#include "global_data.h"
#include "player.h"
#include "player_manager.h"
#include "duplicate_conf.h"
#include "duplicate_entity.h"
#include "duplicate_trigger.h"

#define REGISTER_TIMER_TYPE(nbr_, cb_) \
    do { \
        if (register_timer_callback(nbr_, cb_) == -1) { \
            ERROR_LOG("register timer type error\t[%u]", nbr_); \
            return -1; \
        }\
    } while(0)

int dup_count_down(void *owner, void *data)
{
    duplicate_entity_t *entity = (duplicate_entity_t*)(owner);
    uint64_t tmp = (uint64_t)data;
    uint32_t dup_id = tmp;
    if (entity->dup_id != dup_id) {
        ERROR_TLOG("duplicate timer count down, but dup:%d not exist",
                dup_id);
        return 0;
    }
    //副本时间到，则副本结束
    entity->time_up = true;
    std::vector<uint32_t> params;
    duplicate_entity_trig(entity, (uint32_t)dup_cond_type_time_up, &params);
    //如果副本可以结束了
    if (entity->state == DUPLICATE_STATE_CAN_END) {
        g_dup_entity_mgr->destroy_entity(entity);
    } else {
        entity->dup_time_limit_timer = 0;
    }
    return 0;
}

int dup_mon_flush_trigger(void *owner, void *data)
{
    duplicate_entity_t *entity = (duplicate_entity_t*)(owner);
    uint64_t tmp = (uint64_t)data;
    uint32_t dup_id = tmp;
    if (entity->dup_id != dup_id) {
        ERROR_TLOG("duplicate timer count down, but dup:%d not exist",
                dup_id);
        return 0;
    }
    if (entity->state == DUPLICATE_STATE_CAN_END) {
        entity->flush_mon_timer = 0;
        return 0;
    }

    //抛出一个刷怪的条件
    duplicate_entity_trig(entity, (uint32_t)dup_cond_type_mon_flush_timer_up, 0);
    entity->flush_mon_timer = 0;
    return 0;
}

int dup_phase_end(void *owner, void *data)
{
    duplicate_entity_t *entity = (duplicate_entity_t*)(owner);
    DEBUG_TLOG("P:%u, phase timer up",
            dup_entity_mgr_t::get_entity_one_uid(entity));

    uint64_t tmp = (uint64_t)data;
    uint32_t dup_id = tmp;
    if (entity->dup_id != dup_id) {
        ERROR_TLOG("duplicate timer count down, but dup:%d not exist",
                dup_id);
        return 0;
    }
    entity->phase_timer = NULL;
    //抛出一个阶段时间到的条件
    std::vector<uint32_t> args;
    args.push_back(entity->cur_map_phase);
    duplicate_entity_trig(entity, (uint32_t)dup_cond_type_phase_timer_up, &args);
    return 0;
}

int destroy_dup(void *owner, void *data)
{
    duplicate_entity_t *entity = (duplicate_entity_t*)(owner);
    uint64_t tmp = (uint64_t)data;
    uint32_t dup_id = tmp;
    if (entity->dup_id != dup_id) {
        ERROR_TLOG("duplicate timer count down, but dup:%d not exist",
                dup_id);
        return 0;
    }
    g_dup_entity_mgr->destroy_entity(entity);
    return 0;
}

int boss_show_timer_up(void *owner, void *data) 
{
    duplicate_entity_t *entity = (duplicate_entity_t*)(owner);
    uint64_t tmp = (uint64_t)data;
    uint32_t dup_id = tmp;
    if (entity->dup_id != dup_id) {
        ERROR_TLOG("duplicate timer count down, but dup:%d not exist",
                dup_id);
        return 0;
    }
    if (entity->state == DUPLICATE_STATE_CAN_END) {
        return 0;
    }
    entity->boss_show_timer = NULL;
    //抛出一个阶段时间到的条件
    DEBUG_TLOG("P:%u, DupBossShowTimerUp", dup_entity_mgr_t::get_entity_one_uid(entity));
    duplicate_entity_trig(entity, (uint32_t)dup_cond_type_boss_show_time_up, 0);
    return 0;
}

int force_all_ready_timer_func(void *owner, void *data)
{
    duplicate_entity_t *entity = (duplicate_entity_t*)(owner);
    uint64_t tmp = (uint64_t)data;
    uint32_t dup_id = tmp;
    if (entity->dup_id != dup_id) {
        ERROR_TLOG("duplicate timer count down, but dup:%d not exist",
                dup_id);
        return 0;
    }
    entity->force_ready_timer = NULL;

    if (entity->state != DUPLICATE_STATE_WAIT_PLAYER_READY) {
        return 0;
    }
    if (g_duplicate_conf_mgr.get_duplicate_type(dup_id) != DUP_BTL_TYPE_PPVE) {
        return 0;
    }

    //PPVE下等待所有玩家准备好的定时器到 且还是等待状态 则强制所有玩家准备好
    FOREACH((*(entity->battle_players)), it) {
        std::set<player_t*> &p_set = it->second;
        FOREACH(p_set, it2) {
            player_t *player = *it2;
            player->player_dup_state = PLAYER_READY;
        }
    }
    duplicate_entity_trig(entity, (uint32_t)dup_cond_type_all_ready);

    return 0;
}

int register_timers()
{
    REGISTER_TIMER_TYPE(kTimerTypeDupCountDownTimer, dup_count_down);
    REGISTER_TIMER_TYPE(kTimerTypeDupMonFlushTimer, dup_mon_flush_trigger);
    REGISTER_TIMER_TYPE(kTimerTypeDupPhaseTimer, dup_phase_end);
    REGISTER_TIMER_TYPE(kTimerTypeDupDestroyTimer, destroy_dup);
    REGISTER_TIMER_TYPE(kTimerTypeBossSHowTimer, boss_show_timer_up);
    REGISTER_TIMER_TYPE(kTimerTypeForceReadyTimer, force_all_ready_timer_func);

    return 0;
}
