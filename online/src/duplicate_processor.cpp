#include "player.h"
#include "duplicate_utils.h"
#include "global_data.h"
#include "service.h"
#include "duplicate_processor.h"
#include "data_proto_utils.h"
#include "pet_utils.h"
#include "map_utils.h"
#include "player_utils.h"
#include "prize.h"
#include "item.h"
#include "task_info.h"
#include "tran_card.h"
#include "task_utils.h"
#include "builder_conf.h"
#include "rank_utils.h"
#include "bless_pet_utils.h"
#include "buff.h"
#include "family_utils.h"
#include "family_conf.h"
#include "sys_ctrl.h"
#include "proto_processor.h"
#include "utils.h"
#include "pet_pass_dup_conf.h"
#include <sstream>

#define RELAY_IN_IF_WAIT(if_wait) \
    do { \
        if (!DupUtils::is_player_in_duplicate(player)) {\
            return send_err_to_player(player, \
                    player->cli_wait_cmd, cli_err_player_not_in_this_duplicate);\
        }\
        if (!DupUtils::can_send_to_battle(player)) {\
            return send_err_to_player(player, player->cli_wait_cmd, cli_err_duplicate_ended);\
        }\
        btl_in_.Clear();\
        btl_in_.set_uid(player->userid);\
        btl_in_.set_create_tm(GET_A(kAttrCreateTm));\
        btl_in_.set_cmd(cmd);\
        btl_in_.set_pkg(body, bodylen);\
        return DupUtils::send_to_battle(player, btl_cmd_msg_relay, btl_in_, if_wait);\
    } while(0)

#define RELAY_IN RELAY_IN_IF_WAIT(WAIT_SVR)

#define RELAY_OUT \
    do {\
        PARSE_SVR_MSG(btl_out_);\
        parse_message(btl_out_.pkg().c_str(), btl_out_.pkg().size(), &cli_out_);\
        return send_buff_to_player(player, btl_out_.cmd(), \
                btl_out_.pkg().c_str(), btl_out_.pkg().size());\
    } while(0)

void EnterDuplicateCmdProcessor::register_proc_func()
{
    //元素挑战
    before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_ELEM_DUP] = before_enter_trial_attr;
    after_enter_func_map_[(uint32_t)DUP_MODE_TYPE_ELEM_DUP] = after_enter_trial_attr;

    //一桶天下
    before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_BUCKET] = before_enter_bucket;
    after_enter_func_map_[(uint32_t)DUP_MODE_TYPE_BUCKET] = after_enter_bucket;

	//试练(勇者试练or 霸者领域)
	before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_TRIAL] = before_enter_trial;
	after_enter_func_map_[(uint32_t)DUP_MODE_TYPE_TRIAL] = after_enter_trial;

    //进入新手副本
    before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_STARTER] = before_enter_starter;
	//怪物危机
	before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_MONSTER_CRISIS] = before_enter_monster_crisis;

    before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_RPVP] = before_enter_rpvp;
    after_enter_func_map_[(uint32_t)DUP_MODE_TYPE_RPVP] = after_enter_rpvp;
    after_pack_func_map_[(uint32_t)DUP_MODE_TYPE_RPVP] = after_pack_rpvp;

	//夜袭
	before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_NIGHT_RAID] = before_enter_night_raid;
    after_enter_func_map_[(uint32_t)DUP_MODE_TYPE_NIGHT_RAID] = after_enter_night_raid;

    // 进入家族副本
	before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_FAMILY] = before_enter_family;

    // 世界boss
	before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_WORLD_BOSS] = before_enter_world_boss;
    after_enter_func_map_[(uint32_t)DUP_MODE_TYPE_WORLD_BOSS] = after_enter_world_boss;

	//玛丽试练
	after_enter_func_map_[(uint32_t)DUP_MODE_TYPE_MAYIN_BUCKET] = after_enter_mayin_bucket;
	//挑战修罗武士
	before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_CHALLENGE_DEMON] = before_enter_challenge_demon;
    after_enter_func_map_[(uint32_t)DUP_MODE_TYPE_CHALLENGE_DEMON] = after_challenge_demon;
	
	//明星副本
	before_enter_func_map_[(uint32_t)DUP_MODE_TYPE_STAR_PET] = before_enter_star_pet;
}

int EnterDuplicateCmdProcessor::before_enter_dup(player_t *player, uint32_t dup_id)
{
    uint32_t mode = (uint32_t)(DupUtils::get_duplicate_mode(dup_id));
    if (before_enter_func_map_.count(mode) == 0) {
        return 0;
    }

    // 更新悬赏任务
    std::map<uint32_t, uint32_t> reward_task_map;
    reward_task_map[DUP_MODE_TYPE_NORMAL] = REWARD_TASK_ITEM_NORMAL_DUP;
    reward_task_map[DUP_MODE_TYPE_ELITE] = REWARD_TASK_ITEM_ELITE_DUP;
    reward_task_map[DUP_MODE_TYPE_TRIAL] = REWARD_TASK_ITEM_KING_FIELD;
    reward_task_map[DUP_MODE_TYPE_ELEM_DUP] = REWARD_TASK_ITEM_ELEM_FIGHT;
    reward_task_map[DUP_MODE_TYPE_MONSTER_CRISIS] = REWARD_TASK_ITEM_MONSTER_CRISIS;
    reward_task_map[DUP_MODE_TYPE_BUCKET] = REWARD_TASK_ITEM_BUCKET_DUP;
    reward_task_map[DUP_MODE_TYPE_NIGHT_RAID] = REWARD_TASK_ITEM_NIGHT_RAID;
    std::map<uint32_t, uint32_t>::iterator iter = reward_task_map.find(mode);
    if (iter != reward_task_map.end()) {
        TaskUtils::update_task_step_condition_attr(
                player, iter->second, 1, 1);
    }

    // 更新新手任务完成记录
    TaskUtils::update_new_player_task_dup_step(player, dup_id, false, 0);

    return (before_enter_func_map_.find(mode)->second)(player, dup_id);
}

int EnterDuplicateCmdProcessor::after_pack_btl(player_t *player, 
        battleproto::cs_battle_duplicate_enter_map &btl_in)
{
    uint32_t mode = (uint32_t)(DupUtils::get_duplicate_mode(btl_in.dup_id()));
    if (after_pack_func_map_.count(mode) == 0) {
        return 0;
    }
    return (after_pack_func_map_.find(mode)->second)(player, btl_in);
}

int EnterDuplicateCmdProcessor::after_enter_dup(player_t *player, uint32_t dup_id,
        battleproto::sc_battle_duplicate_enter_map &btl_out,
        onlineproto::sc_0x0201_duplicate_enter_map &cli_out)
{
    uint32_t mode = (uint32_t)(DupUtils::get_duplicate_mode(dup_id));
    if (after_enter_func_map_.count(mode) == 0) {
        return 0;
    }
    return (after_enter_func_map_.find(mode)->second)(player, dup_id, btl_out, cli_out);
}

int EnterDuplicateCmdProcessor::before_enter_monster_crisis(
		player_t *player, uint32_t dup_id)
{
	uint32_t mode = DupUtils::get_duplicate_mode(dup_id);
	if (mode != (uint32_t)DUP_MODE_TYPE_MONSTER_CRISIS) {
		return cli_err_duplicate_btl_mode_err;
	}
	
	std::vector<uint32_t> dup_ids;
	g_duplicate_conf_mgr.get_dup_ids_by_mode(mode, dup_ids);
	assert(dup_ids.size());
	uint32_t lock_dup_id = GET_A(kAttrDupLowestLock1Id);
	if (0 == lock_dup_id) {
		SET_A(kAttrDupLowestLock1Id, dup_ids.front());
		lock_dup_id = dup_ids.front();
	}
	//由于怪物危机副本是严格的解锁玩法，只能玩当前未解锁的最小关卡
	//所以 前端传来的dup_id 必须与 未解锁最小关卡相等
	if (dup_id != lock_dup_id) {
		return cli_err_dup_id_not_unlock;
	}
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet == NULL) {
			continue;
		}
		//第一次玩怪物危机，将所有精灵的mon_cris_hp至满
		if (0 == GET_A(kAttrDupHighestUnlock1Id)) {
			pet->set_mon_cris_hp(pet->max_hp());
		}
		pet->set_hp(pet->mon_cris_hp());
		PetUtils::save_pet(player, *pet, false, true);
	}
	if (0 == GET_A(kAttrDupHighestUnlock1Id)) {
		SET_A(kMonCrisHp, GET_A(kAttrHpMax));
	}
	SET_A(kAttrHp, GET_A(kMonCrisHp));
	//将精灵的 hp 用 mon_cris_hp代替
	(*player->temp_info.mon_cris_pets).clear();
	return 0;
}

int EnterDuplicateCmdProcessor::before_enter_rpvp(
		player_t *player, uint32_t dup_id)
{
    if (!TimeUtils::is_current_time_valid(TM_CONF_KEY_RPVP_OPEN_TM, 0)) {
        return cli_err_duplicate_closed;
    }
    LOCK_MSG;
    //精灵满级
    PetUtils::all_pets_lv_full(player, true);

    //玩家满级
    player->temp_info.cache_level = GET_A(kAttrLv);
    SET_A(kAttrLv, PLAYER_MAX_LEVEL);
    PlayerUtils::calc_player_battle_value(player);

    //改临时最大血量
    player->temp_info.tmp_max_hp = PlayerUtils::obj_hp_add_common(GET_A(kAttrHpMax), 0);
    SET_A(kAttrHp, player->temp_info.tmp_max_hp);

    UNLOCK_MSG;

    return 0;
}

int EnterDuplicateCmdProcessor::after_pack_rpvp(
		player_t *player, battleproto::cs_battle_duplicate_enter_map &btl_in)
{
    LOCK_MSG;
    //还原精灵等级(false表示不还原血量)
    PetUtils::all_pets_lv_recover(player, false);
    //还原玩家等级
    SET_A(kAttrLv, player->temp_info.cache_level);
    //还原属性 HpMax会被重新计算
    PlayerUtils::calc_player_battle_value(player);
    SET_A(kAttrHp, player->temp_info.tmp_max_hp);
    UNLOCK_MSG;

    return 0;
}

int EnterDuplicateCmdProcessor::after_enter_rpvp(player_t *player, uint32_t dup_id,
        battleproto::sc_battle_duplicate_enter_map &btl_out,
        onlineproto::sc_0x0201_duplicate_enter_map &cli_out)
{
    for (int i = 0; i < btl_out.players_size(); i++) {
        const commonproto::battle_player_data_t &p_inf = btl_out.players(i);
        if (p_inf.base_info().user_id() != player->userid) {
            player->temp_info.op_rpvp_score = p_inf.rpvp_score();
            break;
        }
    }
    return 0;
}

int EnterDuplicateCmdProcessor::before_enter_trial(player_t *player, uint32_t dup_id)
{
	uint32_t mode = DupUtils::get_duplicate_mode(dup_id);
	if (mode != (uint32_t)DUP_MODE_TYPE_TRIAL) {
		return cli_err_duplicate_btl_mode_err;
	}
	const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
	if (dup == NULL) {
		ERROR_TLOG("before enter trial,dup id not exist,dup_id=[%u]", dup_id);
		return cli_err_duplicate_id_not_found;
	} 
    if (dup->prev_id) {
        uint32_t pass_time = GET_A(AttrUtils::get_duplicate_pass_time_attr(dup->prev_id));
        //前置副本通关时间若是0，则说明没有玩过前置副本
        if (pass_time == 0) {
            return cli_err_cur_dup_id_still_lock;
        }
    }
	return 0;
}

int EnterDuplicateCmdProcessor::before_enter_family(player_t *player, uint32_t dup_id)
{
    uint32_t mode = DupUtils::get_duplicate_mode(dup_id);
    if (mode != (uint32_t)DUP_MODE_TYPE_FAMILY) {
        return cli_err_duplicate_btl_mode_err;
    }
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
    if (dup == NULL) {
        ERROR_TLOG("before enter family,dup id not exist,dup_id=[%u]", dup_id);
        return cli_err_duplicate_id_not_found;
    } 

    if (!FamilyUtils::is_valid_family_id(GET_A(kAttrFamilyId))) {
		return cli_err_family_id_illegal;
    }

    // 检查玩家等级
    uint32_t player_lv = GET_A(kAttrLv);
    if (player_lv < commonproto::MIN_FAMILY_DUP_ENTER_LEVEL) {
		return cli_err_level_too_low;
    }

    // 检查进入次数
    uint32_t family_title = GET_A(kAttrFamilyTitle);
    std::map<uint32_t, uint32_t> limit_map;
    limit_map[commonproto::FAMILY_TITLE_LEADER] = 
        commonproto::FAMILY_DUP_LIMIT_TIMES_CONFIG_1;
    limit_map[commonproto::FAMILY_TITLE_VICE_LEADER] = 
        commonproto::FAMILY_DUP_LIMIT_TIMES_CONFIG_2;
    limit_map[commonproto::FAMILY_TITLE_MEMBER] = 
        commonproto::FAMILY_DUP_LIMIT_TIMES_CONFIG_3;
    std::map<uint32_t, uint32_t>::iterator iter = limit_map.find(family_title);
    if (iter == limit_map.end()) {
		return cli_err_family_title_illegal;
    }

    uint32_t limit_times = g_family_conf_mgr.get_common_config(iter->second, 0);
    if (GET_A(kAttrFamilyBossFightTimes) >= limit_times) {
		return cli_err_family_over_dup_enter_limit;
    }

    // 检查是否通关
    uint32_t cur_stage = GET_A(kAttrFamilyBossStage);
    if (cur_stage >= commonproto::MAX_FAMILY_DUP_STAGE_ID) {
		return cli_err_family_dup_pass;
    }

    // 扣除挑战次数，如果战胜再返还,避免失败刷新不扣减次数
    ADD_A(kAttrFamilyBossFightTimes, 1);

    uint32_t last_reset_time = GET_A(kAttrFamilyBossLastResetTime);
    uint32_t nowtime = NOW();
    if (TimeUtils::check_is_week_past(last_reset_time, nowtime)) {
        // 每周重置一次家族副本
        // 适配boss等级 
        const family_dup_boss_conf_t *boss_conf = 
            g_family_conf_mgr.get_family_dup_boss_conf(1, player_lv);
        if (boss_conf == NULL) {
		    return cli_err_family_dup_stage_id_illegal;
        }
        SET_A(kAttrFamilyBossLv, boss_conf->lv);
        SET_A(kAttrFamilyPlayerLv, player_lv);

        // 所有记录重置
        SET_A(kAttrFamilyBossStage, 0);
        SET_A(kAttrFamilyBossLastFightFailedTime, 0);
        SET_A(kAttrFamilyBossFightTimes, 0);
        SET_A(kAttrFamilyBossDamage, 0);
        SET_A(kAttrFamilyBossHp, boss_conf->hp);

        SET_A(kAttrFamilyBossLastResetTime, nowtime);
    }

    uint32_t boss_hp = GET_A(kAttrFamilyBossHp);
    if (!FamilyUtils::is_valid_family_dup_stage_id(cur_stage + 1)) {
        return cli_err_family_dup_stage_id_illegal;
    }
    uint32_t init_player_lv = GET_A(kAttrFamilyPlayerLv);
    const family_dup_boss_conf_t *boss_conf = 
        g_family_conf_mgr.get_family_dup_boss_conf(cur_stage + 1, init_player_lv);
    if (boss_conf == NULL) {
        return cli_err_family_dup_stage_id_illegal;
    }             

    if (boss_hp == 0) {
        // 初始化boss血量记录
        SET_A(kAttrFamilyBossHp, boss_conf->hp);
    }

    // 设置boss战斗属性
    player->temp_info.family_dup_boss_lv = GET_A(kAttrFamilyBossLv) + 5 * GET_A(kAttrFamilyBossStage);
    player->temp_info.family_dup_boss_hp = GET_A(kAttrFamilyBossHp);
    player->temp_info.family_dup_boss_maxhp = boss_conf->hp;

	return 0;
}

int EnterDuplicateCmdProcessor::before_enter_world_boss(player_t *player, uint32_t dup_id)
{
	uint32_t mode = DupUtils::get_duplicate_mode(dup_id);
	if (mode != (uint32_t)DUP_MODE_TYPE_WORLD_BOSS) {
		return cli_err_duplicate_btl_mode_err;
	}
	const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
	if (dup == NULL) {
		ERROR_TLOG("before enter world boss,dup id not exist,dup_id=[%u]", dup_id);
		return cli_err_duplicate_id_not_found;
	} 

    if (player == NULL) {
		return cli_err_user_not_exist;
    }

    // 判断副本状态
    world_boss_dup_info_t *dup_info = g_world_boss_mgr.get_world_boss_dup_info(dup_id);
    if (dup_info == NULL) {
		return cli_err_duplicate_id_not_found;
    }

    // 世界boss活动没开始
    if (dup_info->status == commonproto::WORLD_BOSS_DUP_CLOSED) {
        return cli_err_world_boss_not_open;
    }

    if (dup_info->reward_flag == true) {
        return cli_err_duplicate_ended;
    }

    uint32_t last_enter_time = GET_A(kAttrWorldBossLastEnterTime);
    if (last_enter_time != dup_info->start_time) {
        // 进入新一轮活动，清除旧记录
        g_world_boss_mgr.clear_player_record(
                dup_id, player->userid, player->create_tm, g_online_id);

        SET_A(kAttrWorldBossLastEnterTime, dup_info->start_time);
        SET_A(kAttrWorldBossLastSvrId, g_online_id);
    } else {
        // 进入同一轮活动
            
        // 已经领取过奖励,不能进入
        if (g_world_boss_mgr.get_reward_status(dup_id, GET_A(kAttrWorldBossRewardRecord)) == 
                (uint32_t)commonproto::WORLD_BOSS_REWARD_ALREADY_GET) {
            return cli_err_world_boss_repeat_reward;
        }

        uint32_t last_svr_id = GET_A(kAttrWorldBossLastSvrId);
        if (last_svr_id != g_online_id) {
            // 进入不同服活动，还没领取奖励，清除旧记录 TODO(vince) 清除老服的伤害记录
            g_world_boss_mgr.clear_player_record(
                    dup_id, player->userid, player->create_tm, g_online_id);
            SET_A(kAttrWorldBossLastSvrId, g_online_id);
        }

        // 设置下次自动复活时间
        uint32_t next_revival_time = GET_A(kAttrWorldBossNextRevivalTime);
        if (next_revival_time == 0 && 
                dup_info->status == commonproto::WORLD_BOSS_DUP_FIGHT) {
            uint32_t time_gap = g_module_mgr.get_module_conf_uint32_def(
                    module_type_world_boss, "revival_time_gap", 20);
            SET_A(kAttrWorldBossNextRevivalTime, NOW() + time_gap);
        }
    }

    if (GET_A(kAttrWorldBossNextRevivalTime) > NOW()) {
        // 还在复活时间内
        if (dup_info->status == commonproto::WORLD_BOSS_DUP_FIGHT) {
            SET_A(kAttrHp, 0);
        }
    } else {
        SET_A(kAttrWorldBossNextRevivalTime, 0);
    }

    player->temp_info.team = 1;

	return 0;
}

int EnterDuplicateCmdProcessor::after_enter_world_boss(player_t *player, uint32_t dup_id,
		battleproto::sc_battle_duplicate_enter_map &btl_out,
		onlineproto::sc_0x0201_duplicate_enter_map &cli_out) 
{
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
    if (dup == NULL || 
            (dup  && dup->mode != (uint32_t)DUP_MODE_TYPE_WORLD_BOSS)) {
		return cli_err_duplicate_btl_mode_err;
	}

    world_boss_dup_info_t *dup_info = g_world_boss_mgr.get_world_boss_dup_info(dup_id);
    if (dup_info == NULL) {
        return cli_err_duplicate_id_not_found;
    } 

    // 悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_WORLD_BOSS, 1);

    if (btl_out.monsters_size() > 0) {
        const commonproto::battle_pet_data_t &mon = btl_out.monsters(0);
        dup_info->boss_hp = mon.pet_info().battle_info().cur_hp();
        dup_info->boss_maxhp = mon.pet_info().battle_info().max_hp();
    }

    return 0;
}

int EnterDuplicateCmdProcessor::before_enter_trial_attr(player_t *player, uint32_t dup_id)
{
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
    if (dup == NULL) {
        return cli_err_duplicate_id_not_found;
    }

    uint32_t cur_progress = GET_A((attr_type_t)dup->element_progress_attr);
    // 检查进度
    if ((cur_progress + 1) < dup->element_level) {
        return cli_err_must_finish_last_duplicate_map;
    }

    return 0;
}

int EnterDuplicateCmdProcessor::after_enter_trial_attr(player_t *player, uint32_t dup_id,
        battleproto::sc_battle_duplicate_enter_map &btl_out,
        onlineproto::sc_0x0201_duplicate_enter_map &cli_out)
{
    return 0;
}

int EnterDuplicateCmdProcessor::after_enter_trial(player_t *player, uint32_t dup_id,
		battleproto::sc_battle_duplicate_enter_map &btl_out,
		onlineproto::sc_0x0201_duplicate_enter_map &cli_out) 
{
	return 0;
}

int EnterDuplicateCmdProcessor::before_enter_bucket(player_t *player, uint32_t dup_id)
{
    if(!TimeUtils::is_current_time_valid(TM_CONF_KEY_BUCKET, 0)) {
        return cli_err_duplicate_closed;
    }
    return 0;
}

int EnterDuplicateCmdProcessor::after_enter_bucket(player_t *player, uint32_t dup_id,
        battleproto::sc_battle_duplicate_enter_map &btl_out,
        onlineproto::sc_0x0201_duplicate_enter_map &cli_out)
{
    //清理临时变量
    player->temp_info.bucket_cnt = 0;
    player->temp_info.bucket_score = 0;
    //清理玩家所带的精灵
    commonproto::battle_player_data_t *bp = btl_out.mutable_players(0);
    bp->mutable_pet_list()->Clear();
    return 0;
}

int EnterDuplicateCmdProcessor::before_enter_challenge_demon(player_t *player, uint32_t dup_id)
{
	uint32_t mode = DupUtils::get_duplicate_mode(dup_id);
	if (mode != (uint32_t)DUP_MODE_TYPE_CHALLENGE_DEMON) {
		return cli_err_duplicate_btl_mode_err;
	}
	const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
	if (dup == NULL) {
		ERROR_TLOG("before enter challenge demon,dup id not exist,dup_id=[%u]", dup_id);
		return cli_err_duplicate_id_not_found;
	} 

    if (player == NULL) {
		return cli_err_user_not_exist;
    }

	uint32_t free_times = GET_A(kDailyChallengeDemonTimes);
	uint32_t buy_times = GET_A(kDailyBuyChallengeDemonTimes);

	//免费和购买次数都用完
	if((free_times >= 3) && (buy_times == 0)){
		return send_err_to_player(player, 
				player->cli_wait_cmd, cli_err_duplicate_cnt_run_out);
	}
	return 0;
}

int EnterDuplicateCmdProcessor::before_enter_star_pet(player_t* player, uint32_t dup_id)
{
	const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
	if (dup == NULL) {
		return cli_err_duplicate_id_not_found;
	}
	bool found = false;
	FOREACH(*player->pets, it) {
		Pet &pet = it->second;
		if (pet.pet_id() == dup->have_pet_id) {
			found = true;
			break;
		}
	}
	if (found == false) {
		return cli_err_dup_must_own_special_pet;
	}
	return 0;
}

int EnterDuplicateCmdProcessor::after_challenge_demon(
		player_t* player, uint32_t dup_id,
		battleproto::sc_battle_duplicate_enter_map &btl_out,
		onlineproto::sc_0x0201_duplicate_enter_map &cli_out)
{
	uint32_t free_times = GET_A(kDailyChallengeDemonTimes);
	uint32_t buy_times = GET_A(kDailyBuyChallengeDemonTimes);

	if(free_times < 3){
		ADD_A(kDailyChallengeDemonTimes, 1);
	} else if (buy_times > 0){
		SUB_A(kDailyBuyChallengeDemonTimes, 1);
	} else {
        return send_err_to_player(player, 
                player->cli_wait_cmd, cli_err_duplicate_cnt_run_out);
	}
	//清理精灵
	commonproto::battle_player_data_t *bp = btl_out.mutable_players(0);
	bp->mutable_pet_list()->Clear();
	bp->mutable_switch_pet_list()->Clear();
	return 0;
}

int EnterDuplicateCmdProcessor::after_enter_mayin_bucket(
		player_t* player, uint32_t dup_id,
		battleproto::sc_battle_duplicate_enter_map &btl_out,
		onlineproto::sc_0x0201_duplicate_enter_map &cli_out)
{
	player->temp_info.mayin_bucket_cnt = 0;
	player->temp_info.mayin_bucket_cur_energy = 0;	
	const uint32_t send_energy = commonproto::MAYIN_GET_ENERGY_WHEN_ENTER_DUP;
	const uint32_t energy_limit = commonproto::MAYIN_ENERGY_TOTAL_HIGH_LIMIT;
	if (GET_A(kAttrMayinBucketTotalEnergy) + send_energy > energy_limit) {
		SET_A(kAttrMayinBucketTotalEnergy, energy_limit);
	} else {
		ADD_A(kAttrMayinBucketTotalEnergy, send_energy);
	}
	DupUtils::set_mayin_train_gift_state(player);

	commonproto::battle_player_data_t *bp = btl_out.mutable_players(0);
	bp->mutable_pet_list()->Clear();
	return 0;
}

int EnterDuplicateCmdProcessor::before_enter_starter(
        player_t *player, uint32_t dup_id)
{
    if (GET_A(kAttrGuideFinished) != 0) {
        return cli_err_guide_finished;
    }
    //改变玩家等级
    uint32_t need_exp = PlayerUtils::calc_level_up_need_exp(GET_A(kAttrLv), PLAYER_MAX_LEVEL);
    SET_A(kAttrLvBeforeStarter, GET_A(kAttrLv));
    PlayerUtils::add_player_exp(player, need_exp, 0, ADDICT_DETEC, onlineproto::SYNC_ATTR_STARTER);
    player->temp_info.tmp_max_hp = GET_A(kAttrHpMax);
    //增加99个新手阶段的药水
    //add_single_item(player, 30007, 99);
    return 0;
}

int EnterDuplicateCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    //副本之间不可直接跳转(如果已在A副本 则先退出A副本才能进B副本)
    if (player->temp_info.dup_id 
        && player->temp_info.dup_id != cli_in_.dup_id()) {
        return send_err_to_player(player, 
                player->cli_wait_cmd, cli_err_duplicate_jump_err);
    }
    if (player->temp_info.ready_enter_dup_id) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_already_in_duplicate_map);
    }

    if (!g_duplicate_conf_mgr.find_duplicate(cli_in_.dup_id())) {
        return send_err_to_player(player, player->cli_wait_cmd, 
                cli_err_duplicate_id_not_found);
    }
    //NOTI(singku)为了支持断线重连
    int ret = 0;
    //如果是新进入一个副本 则判定能否进入
	bool new_duplicate = false;
    bool re_enter = false;
    if (player->temp_info.dup_id != cli_in_.dup_id()) {
        ret = DupUtils::can_enter_duplicate(player, cli_in_.dup_id());
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd, ret);
        }
		new_duplicate = true;
        //玩家满血满魔进入
        SET_A(kAttrHp, GET_A(kAttrHpMax));
        // SET_A(kAttrTp, 100);
        SET_A(kAttrTp,GET_A(kAttrMaxTp));
        //背包精灵满血进入
        PetUtils::bag_pets_hp_recover(player);

        //还原精灵临时最大血量
        FOREACH(*player->pets, it) {
            Pet* pet = &it->second;
            if (pet == NULL) {
                continue;
            }
            pet->set_tmp_max_hp(pet->max_hp());
        }
        //还原玩家临时最大血量
        player->temp_info.tmp_max_hp = GET_A(kAttrHpMax);

        player->temp_info.dup_state = PLAYER_DUP_NONE;

        ret = this->before_enter_dup(player, cli_in_.dup_id());
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd,
                    ret);
        }

    } else if (player->temp_info.dup_map_id == cli_in_.map_id()) { 
        //同一个副本的同一个场景
        if (player->temp_info.dup_state == PLAYER_DUP_OFFLINE) {
            re_enter = true;
        } else {
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_already_in_duplicate_map);
        }
    }   

    player->temp_info.ready_enter_dup_id = cli_in_.dup_id();
    player->temp_info.dup_ready_seq = player->seqno;
    duplicate_battle_type_t type = 
        DupUtils::get_player_duplicate_battle_type(player);
    bool wait_svr = true;
    if (type == DUP_BTL_TYPE_RPVP || type == DUP_BTL_TYPE_PPVE) {
        wait_svr = false;
        if (type == DUP_BTL_TYPE_PPVE) {
            player->temp_info.team = 1;
			TaskUtils::update_task_step_condition_attr(player,
					REWARD_TASK_ITEM_PPVE, 1, 1);
		}
    } else if (type == DUP_BTL_TYPE_PEVE){
            player->temp_info.team = 1;
	}

    player->map_x = cli_in_.x_pos();
    player->map_y = cli_in_.y_pos();
    player->heading = cli_in_.heading();

    btl_in_.Clear();
    btl_in_.set_dup_id(cli_in_.dup_id());
    btl_in_.set_map_id(cli_in_.map_id());
    btl_in_.set_re_enter(re_enter);
    DataProtoUtils::pack_player_battle_all_info(player, btl_in_.mutable_player(), EXPAND_DATA);
	//打包player2信息
	uint32_t mode = DupUtils::get_duplicate_mode(cli_in_.dup_id());
	if (mode == (uint32_t)DUP_MODE_TYPE_NIGHT_RAID) {
		commonproto::battle_player_data_t *btl_info = btl_in_.add_other_players();
		player->nightraid->get_cur_night_raid_battle_info(btl_info);
	} else if (mode == (uint32_t)DUP_MODE_TYPE_BLESS_PET){
		const std::vector<std::string> &info_vec = 
			*(player->temp_info.bless_team_member_info);
		FOREACH(info_vec, it){
			commonproto::battle_player_data_t *btl_info = 
				btl_in_.add_other_players();
			BlessPetUtils::parse_battle_info_from_string(btl_info, *it);
		}
		ADD_A(kDailyBlessPetTimes, 1);
	}

    btl_in_.set_svr_id(g_server_id);
    btl_in_.set_btl_type(g_duplicate_conf_mgr.get_duplicate_btl_type(cli_in_.dup_id()));
    btl_in_.set_btl_name(g_battle->service_name());
    btl_in_.set_least_players(g_duplicate_conf_mgr.find_duplicate(cli_in_.dup_id())->least_users);

    //NOTI 副本对战中可能会切换精灵,所以将背包中可换的精灵都同步到副本中
    DataProtoUtils::pack_player_non_fight_bag_pet_info(player, 
             btl_in_.mutable_player()->mutable_switch_pet_list(), EXPAND_DATA);
	//将帝具属性加到精灵身上
	const commonproto::battle_player_data_t& pb_inf = btl_in_.player();
	std::string debug_str = pb_inf.Utf8DebugString();
	std::string name = pb_inf.GetTypeName();
	TRACE_TLOG("Pack Dup before:'%s'\nmsg:[%s]\n",
			name.c_str(), debug_str.c_str());
	DataProtoUtils::pack_pet_diju_extra_attr(*btl_in_.mutable_player());
	const commonproto::battle_player_data_t& pb_inf02 = btl_in_.player();
	debug_str.clear(); name.clear();
	debug_str = pb_inf02.Utf8DebugString();
	name = pb_inf02.GetTypeName();
	TRACE_TLOG("Pack Dup After:'%s'\nmsg:[%s]\n",
			name.c_str(), debug_str.c_str());

    this->after_pack_btl(player, btl_in_);

    return DupUtils::send_to_battle(player, btl_cmd_enter_duplicate, btl_in_, wait_svr);
}

int EnterDuplicateCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(btl_out_);
    cli_out_.Clear();

    if (btl_out_.match_result() == false) {
        DupUtils::clear_player_dup_info(player);
        return send_err_to_player(player, cli_cmd_cs_0x0201_duplicate_enter_map,
                cli_err_dup_match_failed);
    }
    
    MapUtils::leave_map(player);
    MapUtils::enter_map(player, btl_out_.map_id());
    player->seqno = player->temp_info.dup_ready_seq;
    player->temp_info.ready_enter_dup_id = 0;
	player->temp_info.revival_cnt = 0;

    //新进副本需扣除各种道具体力
    if (player->temp_info.dup_id != btl_out_.dup_id() && !btl_out_.re_enter()) {
        player->temp_info.cache_dup_drop_prize->clear();
        player->temp_info.dup_enter_tm = NOW();

        int ret = DupUtils::reduce_duplicate_cost(player, btl_out_.dup_id());
        if (ret) {
            return send_err_to_player(player, cli_cmd_cs_0x0201_duplicate_enter_map, ret);
        }
        this->after_enter_dup(player, btl_out_.dup_id(), btl_out_, cli_out_);
        //如果上次和当前都是在怪物危机中 则不保存出战精灵
        if (!(MapUtils::is_mon_crisis_map_id(player->last_map_id)
            && MapUtils::is_mon_crisis_map_id(player->cur_map_id))) {
            PetUtils::save_fight_pet_pos(player);
        }
    }

    //附加玩家的buff
    DataProtoUtils::pack_player_buff(player, cli_out_.mutable_buffs());
   

    player->temp_info.dup_id = btl_out_.dup_id();
    player->temp_info.dup_map_id = btl_out_.map_id();
    player->temp_info.dup_state = PLAYER_DUP_PLAY;
    cli_out_.mutable_players()->CopyFrom(btl_out_.players());
    cli_out_.mutable_monsters()->CopyFrom(btl_out_.monsters());
    cli_out_.mutable_builders()->CopyFrom(btl_out_.builders());
	//统计
	uint32_t dup_id = player->temp_info.dup_id;	
	string name = g_duplicate_conf_mgr.get_duplicate_name(dup_id);
	string id = Utils::to_string(dup_id);
	// stringstream ss;
	// ss << dup_id;
	// string id;
	// ss >> id;
	string stat;
	stat = "副本"+id+"_"+name;
	Utils::write_msglog_new(player->userid, "副本", "每日进入人数次数", stat);

    TaskUtils::stat_dup_story_task(player, dup_id, 0, STAT_STORY_TASK_STATUS_ENTER);

    return send_msg_to_player(player, cli_cmd_cs_0x0201_duplicate_enter_map, cli_out_);
}

int DuplicateMatchResultCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    CmdProcessorInterface *processor 
        = g_proto_processor->get_processor(cli_cmd_cs_0x0201_duplicate_enter_map);
    if (processor) {
        return processor->proc_pkg_from_serv(player, body, bodylen);
    }
    return 0;
}

int LeaveDuplicateCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    uint32_t cmd = cli_cmd_cs_0x0203_duplicate_leave_map;
    PARSE_MSG;
    RELAY_IN;
}

int LeaveDuplicateCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    RELAY_OUT;
}

int ReadyDuplicateCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    uint32_t cmd = cli_cmd_cs_0x0205_duplicate_battle_ready;
    PARSE_MSG;

    uint32_t load_diff = NOW() - player->temp_info.dup_enter_tm;
    Utils::write_msglog_new(player->userid, "用户监控", "副本加载耗时", Utils::to_string(load_diff) + "秒");
    RELAY_IN;
}

int ReadyDuplicateCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    RELAY_OUT;
}

int ExitDuplicateCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    if (!DupUtils::is_player_in_duplicate(player)) {
        return send_err_to_player(player, 
                player->cli_wait_cmd, cli_err_player_not_in_this_duplicate);
    }
    //强退算做是在副本中掉线
    player->temp_info.dup_state = PLAYER_DUP_OFFLINE;
    return DupUtils::tell_btl_exit(player, WAIT_SVR);
}

int ExitDuplicateCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	RELAY_OUT;
}

int HitDuplicateObjCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	uint32_t cmd = cli_cmd_cs_0x020A_duplicate_hit_character;
	PARSE_MSG;
	if (!DupUtils::can_send_to_battle(player)) {
		return send_err_to_player(player, player->cli_wait_cmd, 0);
	}

    //如果被攻击方当前血量为0(前端告知) 且收到伤害 则判定为鞭尸 不做任何处理
    //if (cli_in_.def_cur_hp() == 0 && cli_in_.damage() < 0) {    
        //RET_MSG;
    //}

    bool is_starter = false;
    if (DupUtils::get_duplicate_mode(player->temp_info.dup_id) == DUP_MODE_TYPE_STARTER) {
        is_starter = true;
    }
	if (!g_test_for_robot && cli_in_.damage() < 0 && !is_starter) {//外网不是新手且是伤害
        bool exception = false;
        uint32_t max_atk = 0;
        if (cli_in_.atk_id() == player->userid) {//主动攻击
            //玩家出击
            if (cli_in_.atk_type() == DUP_OBJ_TYPE_PLAYER) {
                max_atk = std::max(GET_A(kAttrNormalAtk), GET_A(kAttrSkillAtk));
            //伙伴出击
            } else {
                Pet *pet = PetUtils::get_pet_in_loc(player, cli_in_.atk_create_tm());
                if (pet) {
                    max_atk = std::max(pet->battle_value(kBattleValueNormalTypeNormalAtk), 
                            pet->battle_value(kBattleValueNormalTypeSkillAtk));
                }
            }
            max_atk *= 24;
            if ((uint32_t)(fabs(cli_in_.damage())) > max_atk) {
                exception = true;
            }
        }
		if (exception) {
			SEND_ERR(cli_err_you_are_hacker);
			close_client_conn(player->fdsess->fd);
			return 0;
		}
	}

	// 非pvp类型，收到clientA通知服务器clientB的攻击包,直接返回
	duplicate_battle_type_t type = DupUtils::get_player_duplicate_battle_type(player);
	switch (type) {
		case DUP_BTL_TYPE_PVE:
		case DUP_BTL_TYPE_PPVE:
		case DUP_BTL_TYPE_WORLD_BOSS:
			if ((cli_in_.atk_type() == DUP_OBJ_TYPE_PLAYER || cli_in_.atk_type() == DUP_OBJ_TYPE_PET) 
					&& (cli_in_.atk_id() != player->userid)) {
				if (type == DUP_BTL_TYPE_PPVE) { 
					//Confirm kevin:前端没有正确过滤掉 非自己的攻击包，如果这里返回错误码,
					//那么组队副本中会一直弹出cli_err_data_error的错误码，故后台先帮前端做下屏蔽
					//后续维护人员最好能够督促前端过滤掉 非自己的攻击包
					RET_MSG;
				} else {
					return send_err_to_player(player, player->cli_wait_cmd, cli_err_data_error);
				}
			}
			break;
		default:
			break;
	}

	uint32_t mode = (uint32_t)(DupUtils::get_duplicate_mode(player->temp_info.dup_id));
	if (cli_in_.def_id() == player->userid) {
		if (cli_in_.def_type() == DUP_OBJ_TYPE_PET) {
			//玩家的精灵被攻击 记录血量
			Pet *pet = PetUtils::get_pet_in_loc(player, cli_in_.def_create_tm());
			if (pet) {
				pet->change_hp(cli_in_.damage());
				if (mode == (uint32_t)DUP_MODE_TYPE_MONSTER_CRISIS) {
					std::map<uint32_t, int>& pets_hp = *player->temp_info.mon_cris_pets;
					pets_hp[cli_in_.def_create_tm()] = pet->hp();
				} else if(mode == (uint32_t)DUP_MODE_TYPE_NIGHT_RAID){
					std::map<uint32_t, int>& pets_hp = *player->temp_info.night_raid_pets;
					pets_hp[cli_in_.def_create_tm()] = pet->hp();//+= cli_in_.damage();
				}
			}
        } else if (cli_in_.def_type() == DUP_OBJ_TYPE_PLAYER) {
            //改变血量不要频繁同步DB 锁定服务消息
            LOCK_SVR_MSG;
            PlayerUtils::chg_player_hp(player, cli_in_.damage());
            UNLOCK_SVR_MSG;
			if(mode == (uint32_t)DUP_MODE_TYPE_NIGHT_RAID){
				player->temp_info.night_raid_player_hp = GET_A(kAttrHp); //+= cli_in_.damage();	
			}
        }
    } else if (mode == (uint32_t)DUP_MODE_TYPE_NIGHT_RAID && cli_in_.def_id() == player->nightraid->get_cur_opponent_uid()){
        if (cli_in_.def_type() == DUP_OBJ_TYPE_PET) {
			std::map<uint32_t, int>& pets_hp = *player->temp_info.night_raid_op_pets;
			pets_hp[cli_in_.def_create_tm()] += cli_in_.damage();//+= cli_in_.damage();
        } else if (cli_in_.def_type() == DUP_OBJ_TYPE_PLAYER) {
			player->temp_info.night_raid_op_player_hp += cli_in_.damage(); //+= cli_in_.damage();	
		}
	}

    RELAY_IN;
    return 0;
}

int HitDuplicateObjCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    RELAY_OUT;
}

void StatDuplicateCmdProcessor::register_proc_func()
{
    after_battle_func_map_[(uint32_t)DUP_MODE_TYPE_ELEM_DUP] = after_trial_attr;
    after_battle_func_map_[(uint32_t)DUP_MODE_TYPE_BUCKET] = after_bucket;
    after_battle_func_map_[(uint32_t)DUP_MODE_TYPE_MAYIN_BUCKET] = after_mayin_bucket;
    //新手副本结束后
    after_battle_func_map_[(uint32_t)DUP_MODE_TYPE_STARTER] = after_starter;
	//试练
    after_battle_func_map_[(uint32_t)DUP_MODE_TYPE_TRIAL] = after_trial;
	after_battle_func_map_[(uint32_t)DUP_MODE_TYPE_MONSTER_CRISIS] = after_monster_crisis;
	after_battle_func_map_[(uint32_t)DUP_MODE_TYPE_NIGHT_RAID] = after_night_raid;

    //家族副本结束后
    after_battle_func_map_[(uint32_t)DUP_MODE_TYPE_FAMILY] = after_family;

    after_battle_func_map_[(uint32_t)DUP_MODE_TYPE_RPVP] = after_rpvp;
}

int StatDuplicateCmdProcessor::after_battle(player_t *player, uint32_t dup_id, bool win)
{
    uint32_t mode = (uint32_t)(DupUtils::get_duplicate_mode(dup_id));
    if (mode != DUP_MODE_TYPE_MONSTER_CRISIS) {
        PetUtils::retrieve_fight_pet_pos(player);
    } else {
        if (!win) {
            PetUtils::retrieve_fight_pet_pos(player);
        }
    }

    if (after_battle_func_map_.count(mode) == 0) {
        return 0;
    } else if (mode == DUP_MODE_TYPE_NIGHT_RAID){
        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(player->temp_info.dup_id);
		uint32_t fini_time = NOW() - player->temp_info.dup_enter_tm;
		if(fini_time >= dup->time_limit){
			return 0;
		}
	} 

    return (after_battle_func_map_.find(mode)->second)(player, dup_id, win);
}

int StatDuplicateCmdProcessor::after_trial_attr(player_t *player, uint32_t dup_id, bool win)
{
    if (!(DupUtils::get_duplicate_mode(dup_id) == DUP_MODE_TYPE_ELEM_DUP)) {
        return 0;
    }

    if (win == false) {
        return 0;
    }

    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
    //更新进度
    uint32_t cur_progress = GET_A((attr_type_t)dup->element_progress_attr);
    if (cur_progress < dup->element_level) {
        //SET_A(kAttrStarFairyDuplicateProgess, dup->element_level);
        SET_A((attr_type_t)dup->element_progress_attr, dup->element_level);
        //cur_progress = dup->element_level;
    }

    // 更新单元素挑战副本排行记录
    attr_type_t attr = AttrUtils::get_duplicate_best_time_attr(dup_id);
    uint32_t best_time = GET_A(attr);
    RankUtils::rank_user_insert_score(
            player->userid, player->create_tm, 
            commonproto::RANKING_SINGLE_TRIAL_ATTR_BEST_TIME, 
            dup_id, best_time);

    // 更新元素挑战副本累计最好时间排行记录
    uint32_t total_stage_time = 0;
    uint32_t total_progress = 0;
    std::set<uint32_t> prog_sets;
    //cur_progress = GET_A(kAttrStarFairyDuplicateProgess);
    FOREACH(g_duplicate_conf_mgr.const_dup_map(), it) {
        const duplicate_t &conf_dup = it->second;
        cur_progress = GET_A((attr_type_t)conf_dup.element_progress_attr);
        if (DupUtils::get_duplicate_mode(conf_dup.duplicate_id) == DUP_MODE_TYPE_ELEM_DUP && 
                conf_dup.element_level <= cur_progress) {
            attr_type_t stage_time_attr = 
                AttrUtils::get_duplicate_best_time_attr(conf_dup.duplicate_id);
            uint32_t stage_best_time = GET_A(stage_time_attr);
            total_stage_time += stage_best_time;
        }

        if (DupUtils::get_duplicate_mode(conf_dup.duplicate_id) == DUP_MODE_TYPE_ELEM_DUP &&
                conf_dup.element_progress_attr > 0 &&
                prog_sets.find(conf_dup.element_progress_attr) == prog_sets.end()) {
            total_progress += GET_A((attr_type_t)conf_dup.element_progress_attr);
            prog_sets.insert(conf_dup.element_progress_attr);
        }
    }

    // 进度越高，排名越靠前,要求元素挑战副本配表id不能高于MAX_TRIAL_ATTR_PROGRESS
    uint32_t revers_value = (uint32_t)commonproto::RANK_REVERSE_INT64_HIGH_SCORE;
    uint32_t progress = (total_progress < revers_value) ? 
        (revers_value - total_progress) : revers_value;
    uint64_t score = ((uint64_t)progress << 32) + total_stage_time;
    RankUtils::rank_user_insert_score(
            player->userid, player->create_tm,
			commonproto::RANKING_TRIAL_ATTR_BEST_TIME, 0, score);

    Utils::write_msglog_new(player->userid, "功能", "元素挑战", "成功完成元素挑战");
    return 0;
}

int StatDuplicateCmdProcessor::after_bucket(player_t *player, uint32_t dup_id, bool win)
{
    //更新数量排行
    uint32_t max_score = GET_A(kAttrBucketMaxScore);
    if (player->temp_info.bucket_score > max_score) {
        //更新最大值
        SET_A(kAttrBucketMaxScore, player->temp_info.bucket_score);
        //更新排行
        RankUtils::rank_user_insert_score(player->userid, 
				player->create_tm,
                (uint32_t)commonproto::RANKING_TYPE_BUCKET,
                0, //TimeUtils::get_last_x_time(NOW(), 5), //上周五的0点时间作为sub_key
                player->temp_info.bucket_score,
                0xFFFFFFFF); //变成永久排行
    }
    SET_A(kAttrBucketCurCnt, player->temp_info.bucket_cnt);
    SET_A(kAttrBucketCurScore, player->temp_info.bucket_score);

    //积分3比1换体力
    uint32_t add_vp = player->temp_info.bucket_score / 3;
    add_vp = add_vp > 60 ?60 :add_vp;
    ADD_A(kAttrCurVp, add_vp);

    //统计成功完成一统天下
    Utils::write_msglog_new(player->userid, "功能", "一桶天下", "成功完成一桶天下");
    return 0;
}

int StatDuplicateCmdProcessor::after_mayin_bucket(player_t* player, uint32_t dup_id, bool win)
{
	const uint32_t cur_energy = player->temp_info.mayin_bucket_cur_energy;
	const uint32_t energy_limit = commonproto::MAYIN_ENERGY_TOTAL_HIGH_LIMIT;
	if (GET_A(kAttrMayinBucketTotalEnergy) + cur_energy > energy_limit) {
		SET_A(kAttrMayinBucketTotalEnergy, energy_limit);
	} else {
		ADD_A(kAttrMayinBucketTotalEnergy, cur_energy);
	}
	DupUtils::set_mayin_train_gift_state(player);

    /*
	uint32_t add_vp = cur_energy / 6;
	add_vp = add_vp > 30 ?30:add_vp;
	ADD_A(kAttrCurVp, add_vp);
    */
	return 0;
}

int StatDuplicateCmdProcessor::after_trial(player_t *player, uint32_t dup_id, bool win)
{
    //统计霸者领域完成
    Utils::write_msglog_new(player->userid, "功能", "霸者领域", "成功完成霸者领域");
	return 0;
}

int StatDuplicateCmdProcessor::after_rpvp(player_t *player, uint32_t dup_id, bool win)
{
    //自己积分
    double a = GET_A(kAttrRpvpScore);
    //对手积分
    double b = player->temp_info.op_rpvp_score;
    double p = 1.0 / ( 1 + pow(10, (b-a)/400.0));
    double w;
    if (win) {
        w = 1;
    } else {
        w = 0;
    }
    double k;
    if (a <= 2000) {
        k = 32;
    } else if (a > 2400) {
        k = 8;
    } else {
        k = 16;
    }
    a = a + k * ( w - p );
    if (a < 0) {
        a = 0;
    }
    SET_A(kAttrRpvpScore, a);
    uint32_t score = GET_A(kAttrRpvpScore);
    if (score < 1000) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分1000以下");
    } else if (score < 1500) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分1000到1500");
    } else if (score < 1700) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分1500到1700");
    } else if (score < 1900) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分1700到1900");
    } else if (score < 2000) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分1900到2000");
    } else if (score < 2100) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分2000到2100");
    } else if (score < 2300) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分2100到2300");
    } else if (score < 2400){
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分2300到2400");
    } else if (score < 2450) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分2400到2450");
    } else if (score < 2500) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分2450到2500");
    } else if (score < 2550) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分2500到2550");
    } else if (score < 2600) {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分2550到2600");
    } else {
       Utils::write_msglog_new(player->userid, "系统", "竞技场", "积分2600及以上");
    }

    //更新总排行
    RankUtils::rank_user_insert_score(player->userid, player->create_tm,
            (uint32_t)commonproto::RANKING_RPVP, 0, a, 0xFFFFFFFF); //永久排行

    if (win) {
        ADD_A(kAttrRpvpWinStreak, 1);
    } else {
        SET_A(kAttrRpvpWinStreak, 0);
    }

	//加手动竞技场玩的次数
	AttrUtils::add_attr_in_special_time_range(player,
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivManualArena);
    return 0;
}

int StatDuplicateCmdProcessor::after_family(player_t *player, uint32_t dup_id, bool win)
{
    if (!(DupUtils::get_duplicate_mode(dup_id) == DUP_MODE_TYPE_FAMILY)) {
        return 0;
    }

    uint32_t construct_value = 0;
    uint32_t reward_gold = g_family_conf_mgr.get_common_config(7, 10000);
    uint32_t reward_item = g_family_conf_mgr.get_common_config(9, 36001);
    uint32_t reward_item_cnt = g_family_conf_mgr.get_common_config(8, 10);

    uint32_t cur_stage = GET_A(kAttrFamilyBossStage);
    if (win == true) {
        // 结算奖励
        construct_value = g_family_conf_mgr.get_common_config(5, 3);
        // 补偿次数
        SUB_A(kAttrFamilyBossFightTimes, 1);

        // 进入下一关
        if (cur_stage < commonproto::MAX_FAMILY_DUP_STAGE_ID) {
            ADD_A(kAttrFamilyBossStage, 1);
        }
    } else {
        // 战斗失败
        construct_value = g_family_conf_mgr.get_common_config(6, 1);
    }

    // 发奖
    if (cur_stage == commonproto::MAX_FAMILY_DUP_STAGE_ID) {
        // 通关奖励
        AttrUtils::add_player_gold(player, reward_gold, false, "家族副本阶段打通");
        add_single_item(player, reward_item, reward_item_cnt);
    } else {
        // 副本结束奖励 按伤血比例计算
        uint32_t total_boss_hp = 0;
        for (uint32_t stage_id = 1; stage_id <= commonproto::MAX_FAMILY_DUP_STAGE_ID;stage_id++) {
            uint32_t player_lv = GET_A(kAttrFamilyPlayerLv);
            const family_dup_boss_conf_t *boss_conf = 
                g_family_conf_mgr.get_family_dup_boss_conf(stage_id, player_lv);
            if (boss_conf == NULL) {
                continue;
            }
            total_boss_hp += boss_conf->hp;
        }

        float step_percent = 0;
        if (total_boss_hp > 0) {
            uint32_t total_damage = GET_A(kAttrFamilyBossDamage);
            if (total_damage > total_boss_hp) {
                total_damage = total_boss_hp;
            }

            for (uint32_t i = 0 ; i < 10; i++) {
                if (total_damage >= (total_boss_hp * (i/10.0)) && 
                        total_damage < (total_boss_hp * ((i + 1)/10.0))) {
                    step_percent = i + 1;
                    break;
                }
            }
        }
        step_percent = step_percent / 100;

        reward_gold = reward_gold * step_percent;
        reward_item_cnt = reward_item_cnt * step_percent;

        AttrUtils::add_player_gold(player, reward_gold, false, "家族boss伤害兑换");
        add_single_item(player, reward_item, reward_item_cnt);
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    if (FamilyUtils::is_valid_family_id(family_id)) {
        // 更新家族建设值
        dbproto::cs_family_change_info db_change_info_in_;
        dbproto::family_info_change_data_t *family_change_data = 
            db_change_info_in_.mutable_change_info();
        family_change_data->set_family_id(family_id);
        family_change_data->set_construct_value(construct_value);
        g_dbproxy->send_msg(
                0, family_id, 0, db_cmd_family_change_info, db_change_info_in_);

        // 更新个人累计贡献
        dbproto::cs_family_change_member_info db_change_member_info_in_;
        dbproto::family_member_change_data_t *change_data = 
            db_change_member_info_in_.mutable_change_member_info();
        change_data->set_family_id(family_id);
        change_data->set_userid(player->userid);
        change_data->set_left_construct_value(construct_value);
        change_data->set_total_construct_value(construct_value);
        g_dbproxy->send_msg(
                0, family_id, 0, db_cmd_family_change_member_info, 
                db_change_member_info_in_);

        // 更新个人可消耗贡献
        ADD_A(kAttrFamilyContributeValue, construct_value);

        // 更新家族日志
        std::vector<commonproto::family_log_para_t> paras;
        commonproto::family_log_para_t para;
        para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_UID);
        para.set_value(Utils::to_string(player->userid));
        paras.push_back(para);
        para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_DATA);
        para.set_value(Utils::to_string(construct_value));
        paras.push_back(para);
        para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_DATA);
        para.set_value(Utils::to_string(construct_value));
        paras.push_back(para);
        FamilyUtils::insert_family_log(
                family_id, commonproto::FAMILY_LOG_TYPE_CONSTRUCT_DUP, paras);
    }

	return 0;
}

int StatDuplicateCmdProcessor::after_monster_crisis(
	player_t *player, uint32_t dup_id, bool win)
{
	if (win) {
		uint32_t unlock_id = GET_A(kAttrDupHighestUnlock1Id);
		std::vector<uint32_t> dup_ids;
		g_duplicate_conf_mgr.get_dup_ids_by_mode(DUP_MODE_TYPE_MONSTER_CRISIS, dup_ids);
		//第一次玩(未重置)，只要传来的是第一关701，就更新kAttrDupLowestLock1Id
		// 和kAttrDupHighestUnlock1Id
		bool insert_redis = false;
		if (dup_id >= dup_ids.front() && dup_id <= dup_ids.back()) {
			if (0 == unlock_id && dup_id == dup_ids.front()) {
				SET_A(kAttrDupHighestUnlock1Id, dup_id);
				SET_A(kAttrDupLowestLock1Id, dup_id + 1);
				insert_redis = true;
			} else if (dup_id == unlock_id + 1) {
				SET_A(kAttrDupHighestUnlock1Id, dup_id);
				SET_A(kAttrDupLowestLock1Id, dup_id + 1);
				if (dup_ids.back() == dup_id) {
					Utils::write_msglog_new(player->userid, "功能", "怪物危机", "完成怪物危机");
				}
				insert_redis = true;
			} else if (dup_id < unlock_id + 1) {
				SET_A(kAttrDupLowestLock1Id, dup_id + 1);
			}
		}
		if (insert_redis) {
			uint32_t total_time = 0;
			uint32_t new_unlock_id = GET_A(kAttrDupHighestUnlock1Id);
			for (uint32_t i = dup_ids.front(); i <= new_unlock_id; ++i) {
				attr_type_t attr = AttrUtils::get_duplicate_best_time_attr(i);
				uint32_t best_time = GET_A(attr);
				total_time += best_time;
				SET_A(kAttrMonsterHistoryBestTime, total_time);
			}
			uint32_t revers_value = (uint32_t)commonproto::RANK_REVERSE_INT64_HIGH_SCORE;
			uint32_t progress = revers_value - dup_id;
			uint64_t score = ((uint64_t)progress << 32) | total_time;
			RankUtils::rank_user_insert_score(
					player->userid, player->create_tm,
					commonproto::RANKING_MONSTER_CRISIS, 
					0, score);
		}
		/* 
		else if (dup_id <= unlock_id) {
			uint32_t best_time = GET_A(kAttrMonsterHistoryBestTime);
			uint32_t total_time = 0;
			for (uint32_t i = dup_ids.front(); i <= unlock_id; ++i) {
				attr_type_t attr = AttrUtils::get_duplicate_best_time_attr(i);
				uint32_t best_time = GET_A(attr);
				total_time += best_time;
			}
			if (total_time < best_time) {
				//更新记录，与排行榜
				SET_A(kAttrMonsterHistoryBestTime, total_time);
				uint32_t revers_value = (uint32_t)commonproto::RANK_REVERSE_INT64_HIGH_SCORE;
				uint32_t progress = revers_value - unlock_id;
				uint64_t score = ((uint64_t)progress << 32) | total_time;
				RankUtils::rank_user_insert_score(
						player->userid, player->create_tm,
					commonproto::RANKING_MONSTER_CRISIS, 
					0, score);
			}
		}
		*/
	}
	
	//所有精灵的血量都回满
	uint32_t cur_hp = GET_A(kAttrHp);
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet == NULL) {
			continue;
		}
		std::map<uint32_t, int>& pets_hp_map = *player->temp_info.mon_cris_pets;
		//如果胜利，出战的精灵血量继承
		if (win && pets_hp_map.count(pet->create_tm())) {
			std::map<uint32_t, int>::iterator it = pets_hp_map.find(pet->create_tm());
			pet->set_mon_cris_hp(it->second);
		}
		pet->set_hp(pet->max_hp());
		PetUtils::save_pet(player, *pet, false, true);
	}
	if (win) {
		SET_A(kMonCrisHp, cur_hp);
	}
	(*player->temp_info.mon_cris_pets).clear();
	return 0;
}

int StatDuplicateCmdProcessor::after_starter(player_t *player, uint32_t dup_id, bool win)
{
    //改变等级
    SET_A(kAttrLv, GET_A(kAttrLvBeforeStarter));
    SET_A(kAttrLvBeforeStarter, 0);
    //重算战力
    PlayerUtils::calc_player_battle_value(player, onlineproto::SYNC_ATTR_STARTER);

    //删除新手添加的药水
    uint32_t total = player->package->get_total_item_count(30007);
    reduce_single_item(player, 30007, total);

    //设置新手完成
    if (win ==  true) {
        SET_A(kAttrGuideFinished, 1);
    }
    return 0;
}

int StatDuplicateCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    //TODO(singku)
    //处理统计信息
    //统计回包
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    player->temp_info.tmp_max_hp = GET_A(kAttrHpMax);

    //根据情况结算副本 奖励 星级等
    onlineproto::sc_0x0210_duplicate_notify_result noti_result;
    noti_result.set_dup_id(player->temp_info.dup_id);

    if (!DupUtils::is_player_in_duplicate(player)) {
        RET_ERR(cli_err_player_not_in_this_duplicate);
    }

    bool win;
    if (player->temp_info.dup_state == PLAYER_DUP_END_WIN) {
        win = true;
    } else {
        win = false;
    }
    noti_result.set_win(win);
    uint32_t fini_time = NOW() - player->temp_info.dup_enter_tm;

    noti_result.set_time(fini_time);

    SET_A(AttrUtils::get_duplicate_last_play_time_attr(player->temp_info.dup_id), NOW());

	if (g_duplicate_conf_mgr.get_duplicate_mode(player->temp_info.dup_id) == DUP_MODE_TYPE_ELITE) {
		AttrUtils::add_attr_in_special_time_range(player,
				TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
				kAttrActivEliteDupCnt);
	}


    if (win) {
        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(player->temp_info.dup_id);
		if (dup == NULL) {
			RET_ERR(cli_err_duplicate_id_not_found);
		}

		//副本中指定出战的精灵，都设置为精灵通关状态
		if (!(dup->can_fight_pets_id.empty())) {
			FOREACH(dup->can_fight_pets_id, it) {
				TRACE_TLOG("will set pet pass dup,%u,%u", *it, dup->duplicate_id);
				DupUtils::use_diamond_buy_pet_pass_dup(player, *it, dup->duplicate_id);
			}
		}

		//检查该副本成就是否已经获得
		if (!TaskUtils::is_task_finished(player, dup->achieve_task_id)) {
			SET_A(AttrUtils::get_dup_pass_tm_duration_attr(player->temp_info.dup_id), fini_time);
			uint32_t hp_percent = (100 * GET_A(kAttrHp)) / (GET_A(kAttrHpMax) * 1.0);
			TRACE_TLOG("Dup_stat,U:[%u],hp_percent:[%u]", player->userid, hp_percent);
			SET_A(AttrUtils::get_dup_surplus_hp_percent(player->temp_info.dup_id), hp_percent);
			SET_A(AttrUtils::get_dup_revival_cnt_attr(player->temp_info.dup_id), player->temp_info.revival_cnt);
			SET_A(AttrUtils::get_dup_power_record_attr(player->temp_info.dup_id), GET_A(kAttrCurBattleValue));
		}

        uint32_t star = 0;
        if (fini_time <= dup->star_3_time) {
            star = DUP_PASS_STAR_THREE;
        } else if (fini_time <= dup->star_2_time) {
            star = DUP_PASS_STAR_TWO;
        } else if (fini_time <= dup->star_1_time) {
            star = DUP_PASS_STAR_ONE;
        } else {
            star = 0;
        }
        noti_result.set_star(star);
        attr_type_t attr = AttrUtils::get_duplicate_pass_time_attr(player->temp_info.dup_id);
        uint32_t pass_time = GET_A(attr);
        if (!pass_time) {
            SET_A(attr, NOW());
        }

        attr = AttrUtils::get_duplicate_best_time_attr(player->temp_info.dup_id);
        uint32_t best_time = GET_A(attr);
        if (best_time == 0 || fini_time < best_time) {
            SET_A(attr, fini_time);
        }

        attr = AttrUtils::get_duplicate_best_star_attr(player->temp_info.dup_id);
        uint32_t best_star = GET_A(attr);
        if (best_star < star) {
            SET_A(attr, star);

            // 更新分区累计星级
            if (dup->area_star_sum_attr) {
                uint32_t cur_area_star = GET_A((attr_type_t)dup->area_star_sum_attr);
                ADD_A((attr_type_t)dup->area_star_sum_attr, star - best_star);
                uint32_t new_area_star = GET_A((attr_type_t)dup->area_star_sum_attr);
                DupUtils::proc_dup_area_prize(player, dup, cur_area_star, new_area_star);
            }
        }

		//TODO kevin 中间上阵过的伙伴，设置为通关副本的状态

	   TaskUtils::listen_duplicate_complete(player, player->temp_info.dup_id);

        //副本结算奖励
        if (dup) {
            DupUtils::proc_duplicate_reward(player, dup, noti_result);
        }

        // 更新悬赏任务
        TaskUtils::update_daily_reward_task_duplicate_step(player, dup, 1);
		
    }

    this->after_battle(player, player->temp_info.dup_id, win);


    if (!win) { //失败则直接返回
        if (noti_result.dup_id()) {
            noti_result.set_star(0);
            DupUtils::clear_player_dup_info(player);
            return send_msg_to_player(player, cli_cmd_cs_0x0210_duplicate_notify_result, noti_result);
        } else {
            // 返回dup_id 0前端会出错
            DupUtils::clear_player_dup_info(player);
            return 0;
        }
    }

	duplicate_battle_type_t type = DupUtils::get_player_duplicate_battle_type(player);
	if (type == DUP_BTL_TYPE_PPVE) {
		AttrUtils::add_attr_in_special_time_range(player,
				TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
				kAttrActivTeamDupCnt);
	}

    // 更新新手任务完成记录
    TaskUtils::update_new_player_task_dup_step(player, player->temp_info.dup_id, win, 1);

    DupUtils::clear_player_dup_info(player);
    send_msg_to_player(player, cli_cmd_cs_0x0210_duplicate_notify_result, noti_result);
    return 0;
}

void ToDuplicateNextPhaseCmdProcessor::register_func()
{
    func_map_[(uint32_t)DUP_MODE_TYPE_STARTER] = phase_func_starter;
    func_map_[(uint32_t)DUP_MODE_TYPE_DARK_EYE_WILL] = phase_func_dark_eye_will;
}

int ToDuplicateNextPhaseCmdProcessor::before_to_next_phase(
        player_t *player, uint32_t dup_id, uint32_t phase_id)
{
    uint32_t mode = (uint32_t)(DupUtils::get_duplicate_mode(dup_id));
    if (func_map_.count(mode) == 0) {
        if (phase_id) {
            return cli_err_invalid_to_next_phase;
        }
    }
    return (func_map_.find(mode)->second)(player, phase_id);
}

int ToDuplicateNextPhaseCmdProcessor::phase_func_starter(player_t *player, uint32_t phase_id)
{
    return 0;
}

int ToDuplicateNextPhaseCmdProcessor::phase_func_dark_eye_will(player_t *player, uint32_t phase_id)
{
    return 0;
}

int ToDuplicateNextPhaseCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    uint32_t cmd = cli_cmd_cs_0x0211_duplicate_to_next_phase;
    PARSE_MSG;
    RELAY_IN;
}

int ToDuplicateNextPhaseCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    RELAY_OUT;
}

int RevivalDuplicateCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    if (!player->temp_info.dup_id) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_duplicate_end);
    }

    auto_revival_flag = false;
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(player->temp_info.dup_id);
    if (cli_in_.mode() == 1 && dup 
        && (uint32_t)dup->mode == (uint32_t)onlineproto::DUP_MODE_TYPE_WORLD_BOSS) {
        // 世界boss挑战可以自动复活
        // 检查上次自动复活时间,加2s时间差
        if (NOW() + 2 < GET_A(kAttrWorldBossNextRevivalTime)) {
            return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_revival_too_fast);
        }

        auto_revival_flag = true;
    } else {
		ADD_A(kAttrAccumulateReviveCnt, 1);
        // 主动复活，消耗物品
        uint32_t need_item_id = 31001;
        uint32_t need_item_cnt = 0;
        if (cli_in_.type() == DUP_REVIVAL_TYPE_PLAYER) {
            need_item_cnt = 1;
        } else if (cli_in_.type() == DUP_REVIVAL_TYPE_PET) {
            need_item_cnt = 1;
        } else if (cli_in_.type() == DUP_REVIVAL_TYPE_ALL) {
            need_item_cnt = 2;
        } else {
            RET_ERR(cli_err_duplicate_revival_target_err);
        }

        std::vector<reduce_item_info_t> reduce_vec;
        reduce_item_info_t rd;
        rd.item_id = need_item_id;
        rd.count = need_item_cnt;
        reduce_vec.push_back(rd);
        IF_ERR_THEN_RET(check_swap_item_by_item_id(player, &reduce_vec, 0));

        uint32_t *buf = (uint32_t*)(player->session);
        *buf = need_item_id;
        buf++;
        *buf = need_item_cnt;
    }

    btl_in_.Clear();
    btl_in_.set_type(cli_in_.type());
    btl_in_.set_id(cli_in_.id());
    btl_in_.set_create_tm(cli_in_.create_tm());

    return DupUtils::send_to_battle(player, btl_cmd_revival, btl_in_, WAIT_SVR);
}

int RevivalDuplicateCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(btl_out_);

    //TODO(singku)
    if (!auto_revival_flag) {
        uint32_t *buf = (uint32_t*)(player->session);
        uint32_t id = *buf;
        buf++;
        uint32_t cnt = *buf;
        reduce_single_item(player, id, cnt);
    } else {
        SET_A(kAttrWorldBossNextRevivalTime, 0);
    }

    cli_out_.set_type(btl_out_.type());
    cli_out_.set_id(btl_out_.id());
    cli_out_.set_create_tm(btl_out_.create_tm());
    SET_A(kAttrHp, GET_A(kAttrHpMax));
    // SET_A(kAttrTp, 100);
	SET_A(kAttrTp,GET_A(kAttrMaxTp));
	//增加复活的次数
	//ADD_A(AttrUtils::get_dup_revival_cnt_attr(player->temp_info.dup_id), 1);
	++(player->temp_info.revival_cnt);
    
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

//爆落装备的拾取
int DuplicatePickUpCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    if (player->temp_info.cache_dup_drop_prize->count(cli_in_.index()) == 0) {
        return send_err_to_player(player, player->cli_wait_cmd, 
                cli_err_duplicate_drop_not_exist);
    }
    const cache_prize_elem_t &prize = 
        (player->temp_info.cache_dup_drop_prize->find(cli_in_.index()))->second;
    player->temp_info.cache_dup_drop_prize->erase(cli_in_.index());
    std::vector<cache_prize_elem_t> prize_vec;
    prize_vec.push_back(prize);
    int ret = transaction_proc_packed_prize(player, prize_vec, 0, 
            commonproto::PRIZE_REASON_NO_REASON, "副本野怪掉落奖励");
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    cli_out_.set_index(cli_in_.index());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

void DuplicateBtlNotifyKillCharacterCmdProcessor::register_proc_func()
{
    kill_func_map_[(uint32_t)DUP_MODE_TYPE_BUCKET] = when_kill_in_bucket;
	kill_func_map_[(uint32_t)DUP_MODE_TYPE_MAYIN_BUCKET] = when_kill_in_mayin_bucket;
	kill_func_map_[(uint32_t)DUP_MODE_TYPE_DARK_EYE_WILL] = when_kill_in_dark_eye_will;
}

int DuplicateBtlNotifyKillCharacterCmdProcessor::when_kill(player_t *player, 
        battleproto::sc_battle_duplicate_notify_kill_character &msg)
{
    uint32_t mode = (uint32_t)(DupUtils::get_duplicate_mode(player->temp_info.dup_id));
    if (kill_func_map_.count(mode) == 0) {
        return 0;
    }
    return (kill_func_map_.find(mode)->second)(player, msg);
}

int DuplicateBtlNotifyKillCharacterCmdProcessor::when_kill_in_bucket(
        player_t *player, battleproto::sc_battle_duplicate_notify_kill_character &msg)
{
    //根据杀死的id计算积分
    uint32_t builder_id = msg.def_id();
    const builder_conf_t *conf = g_builder_conf_mgr.find_builder_conf(builder_id);
    if (!conf) {
        return 0;
    }
    player->temp_info.bucket_cnt++;

    //找到builder
    int32_t points = conf->points;
    player->temp_info.bucket_score += points;
    if (player->temp_info.bucket_score < 0) {
        player->temp_info.bucket_score = 0;
    }
    SET_A(kAttrBucketCurScore, player->temp_info.bucket_score);
    return 0;
}

int DuplicateBtlNotifyKillCharacterCmdProcessor::when_kill_in_mayin_bucket(
		player_t* player, battleproto::sc_battle_duplicate_notify_kill_character &msg)
{
    uint32_t builder_id = msg.def_id();
    const builder_conf_t *conf = g_builder_conf_mgr.find_builder_conf(builder_id);
    if (!conf) {
        return 0;
    }
    player->temp_info.mayin_bucket_cnt++;

	int32_t points = conf->points;
	if (player->temp_info.mayin_bucket_cur_energy + points > commonproto::MAYIN_ENERGY_HIGH_LIMIT_IN_DUP) {
		player->temp_info.mayin_bucket_cur_energy = commonproto::MAYIN_ENERGY_HIGH_LIMIT_IN_DUP;
	} else {
		player->temp_info.mayin_bucket_cur_energy += points;
	}

	SET_A(kAttrMayinBucketCurScore, player->temp_info.mayin_bucket_cur_energy);

	return 0;
}

int DuplicateBtlNotifyKillCharacterCmdProcessor::when_kill_in_dark_eye_will(
		player_t* player, battleproto::sc_battle_duplicate_notify_kill_character &msg)
{
    const pet_conf_t *conf = g_pet_conf_mgr.find_pet_conf(msg.def_id());
    if (!conf) {
        return 0;
    }
    if (msg.def_id() == 29304 || msg.def_id() == 29305) {
        ADD_A(kDailyDarkEyeWillKillMonsters, 1);
    }

	return 0;
}

int DuplicateBtlNotifyKillCharacterCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen) 
{
    PARSE_SVR_MSG_WITHOUT_IN(btl_out_);

    if (btl_out_.def_type() == DUP_OBJ_TYPE_PET) {
        if (btl_out_.def_id() == player->userid) {//自己的精灵被杀死了
            uint32_t create_tm = btl_out_.def_create_tm();
            Pet *pet = PetUtils::get_pet_in_loc(player, create_tm);
            if (!pet) {
                return 0;
            }
            pet->set_hp(0);//精灵死亡
            return 0;
        }
        return 0;
    }

    player->temp_info.dup_kill++;

    if (btl_out_.def_type() != DUP_OBJ_TYPE_MON
        && btl_out_.def_type() != DUP_OBJ_TYPE_BUILDER) {
        return 0;
    }

    this->when_kill(player, btl_out_);

    std::vector<uint32_t> dead_prize;
    uint32_t id = btl_out_.def_id();
    if (btl_out_.def_type() == DUP_OBJ_TYPE_MON) {
        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(id);
        if (!pet_conf) {
            return 0;
        }
        dead_prize = pet_conf->prize_id_list;
        FOREACH(pet_conf->task_prize_list, it) {
            uint32_t task_id = it->first;
            if (!player->task_info->is_in_task_list(task_id)) {
                continue;
            }
            dead_prize.insert(dead_prize.end(), it->second.begin(), it->second.end());
        }
		attr_type_t attr = AttrUtils::get_duplicate_pass_time_attr(
				player->temp_info.dup_id);
		if (GET_A(attr) == 0) {
			uint32_t must_drop_prize = pet_conf->must_drop_prize;
			const prize_config_t* prize_ptr = g_prize_conf_mgr.get_prize_conf(
					must_drop_prize);
			if (prize_ptr) {
				dead_prize.push_back(must_drop_prize);
			}
		} else {
			uint32_t rand_drop_prize = pet_conf->rand_drop_prize;
			const prize_config_t* prize_ptr = g_prize_conf_mgr.get_prize_conf(
					rand_drop_prize);
			if (prize_ptr) {
				dead_prize.push_back(rand_drop_prize);
			}
		}
    } else if (btl_out_.def_type() == DUP_OBJ_TYPE_BUILDER) {
        const builder_conf_t *builder_conf = g_builder_conf_mgr.find_builder_conf(id);
        if (!builder_conf) {
            return 0;
        }
        dead_prize = builder_conf->prize_id_list;
    }
    player->temp_info.dup_kill_mon_id = id;
    DupUtils::proc_mon_drop_prize(player, dead_prize, 
            btl_out_.pos_x(), btl_out_.pos_y(), btl_out_.is_player_dead());
    return 0;
}

int DuplicateBtlNotifyEndCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    btl_out_.Clear();
    if (parse_message(body, bodylen, &btl_out_)) {
        ERROR_TLOG("Btl send bad proto to player:%u", player->userid);
        player->temp_info.dup_state = PLAYER_DUP_END_LOSE;
    } else {
        if (btl_out_.win()) {
            player->temp_info.dup_state = PLAYER_DUP_END_WIN;
			//统计
			uint32_t dup_id = player->temp_info.dup_id;	
			string name = g_duplicate_conf_mgr.get_duplicate_name(dup_id);
			string id = Utils::to_string(dup_id);
			string stat;
			stat = "副本"+id+"_"+name;
			Utils::write_msglog_new(player->userid, "副本", "每日通关人数次数", stat);

            TaskUtils::stat_dup_story_task(player, dup_id, 0, STAT_STORY_TASK_STATUS_PASS);

			uint32_t last_tm = GET_A(AttrUtils::get_duplicate_last_play_time_attr(dup_id));
			uint32_t fini_time = NOW() - player->temp_info.dup_enter_tm;
			const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(player->temp_info.dup_id);
			if(dup && 0 == last_tm){
				if (fini_time <= dup->star_3_time) {
					Utils::write_msglog_new(player->userid, "副本", "副本初次进入3星通关人数", stat);
				} else if (fini_time <= dup->star_2_time) {
					Utils::write_msglog_new(player->userid, "副本", "副本初次进入2星通关人数", stat);
				} else if (fini_time <= dup->star_1_time) {
					Utils::write_msglog_new(player->userid, "副本", "副本初次进入1星通关人数", stat);
				}
			}
		} else {//失败
            if (player->temp_info.dup_state != PLAYER_DUP_OFFLINE) {
                player->temp_info.dup_state = PLAYER_DUP_END_LOSE;
            }
			//统计
			uint32_t dup_id = player->temp_info.dup_id;	
			uint32_t last_tm = GET_A(AttrUtils::get_duplicate_last_play_time_attr(dup_id));
			if(0 == last_tm){//第一次
				string name = g_duplicate_conf_mgr.get_duplicate_name(dup_id);
				string id = Utils::to_string(dup_id);
				string stat;
				stat = "副本"+id+"_"+name;
				Utils::write_msglog_new(player->userid, "副本", "副本初次进入失败人数", stat);
			}
		}
	}
	//副本结束 设置CD时间
	SET_A(AttrUtils::get_duplicate_last_play_time_attr(player->temp_info.dup_id), NOW());
	player->buff_id_map->clear();
	PlayerUtils::sync_player_buff(player);
	//背包精灵回血
	PetUtils::bag_pets_hp_recover(player);

	onlineproto::sc_0x020E_duplicate_notify_end noti_msg;
	return send_msg_to_player(player, cli_cmd_cs_0x020E_duplicate_notify_end, noti_msg);
}

void DuplicateBtlNotifyRelayCmdProcessor::register_phase_func()
{
	func_map_[(uint32_t)DUP_MODE_TYPE_STARTER] = before_notify_next_phase_starter;
	func_map_[(uint32_t)DUP_MODE_TYPE_DARK_EYE_WILL] = before_notify_next_phase_dark_eye_will;

	relay_cmd_func_map_[cli_cmd_cs_0x020C_duplicate_notify_monster_born] = relay_notify_monster_born; 
	relay_cmd_func_map_[cli_cmd_cs_0x0213_duplicate_notify_to_next_phase] = relay_notify_to_next_phase; 
	relay_cmd_func_map_[cli_cmd_cs_0x022A_duplicate_notify_role_recover] = relay_notify_role_recover; 
	relay_cmd_func_map_[cli_cmd_cs_0x020B_duplicate_notify_hit_character] = relay_notify_hit_charactor; 
}

int DuplicateBtlNotifyRelayCmdProcessor::before_notify_next_phase(
		player_t *player, uint32_t dup_id,
		onlineproto::sc_0x0213_duplicate_notify_to_next_phase &next_msg)
{
	player->temp_info.dup_phase = next_msg.new_phase();
	uint32_t mode = (uint32_t)(DupUtils::get_duplicate_mode(dup_id));
	if (func_map_.count(mode) == 0) {
		return 0;
	}
	return (func_map_.find(mode)->second)(player, dup_id, next_msg);
}

int DuplicateBtlNotifyRelayCmdProcessor::before_notify_next_phase_starter(
		player_t *player, uint32_t dup_id,
		onlineproto::sc_0x0213_duplicate_notify_to_next_phase &next_msg)
{
	if (next_msg.new_phase() != 15) {
		return 0;
	}
	//改变等级
	SET_A(kAttrLv, GET_A(kAttrLvBeforeStarter));
	SET_A(kAttrLvBeforeStarter, 0);
	//重算战力
	PlayerUtils::calc_player_battle_value(player, onlineproto::SYNC_ATTR_STARTER);
	return 0;
}

int DuplicateBtlNotifyRelayCmdProcessor::before_notify_next_phase_dark_eye_will(
		player_t *player, uint32_t dup_id,
		onlineproto::sc_0x0213_duplicate_notify_to_next_phase &next_msg)
{
    SET_A(kDailyDarkEyeWillCurDupPhase, next_msg.new_phase());
	return 0;
}

int DuplicateBtlNotifyRelayCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG_WITHOUT_IN(btl_out_);
	if (relay_cmd_func_map_.find(btl_out_.cmd()) == relay_cmd_func_map_.end()) {
		return send_buff_to_player(player, btl_out_.cmd(),
				btl_out_.pkg().c_str(), btl_out_.pkg().size());
	}

	relay_cmd_func_map_[btl_out_.cmd()](this, player, btl_out_);

    return send_buff_to_player(player, btl_out_.cmd(),
            btl_out_.pkg().c_str(), btl_out_.pkg().size());
}

int DuplicateBtlNotifyRelayCmdProcessor::relay_notify_monster_born(
		DuplicateBtlNotifyRelayCmdProcessor * const p_this,
		player_t *player, battleproto::sc_battle_relay &btl_out)
{
	onlineproto::sc_0x020C_duplicate_notify_monster_born tmp_msg;
	parse_message(btl_out.pkg().c_str(), btl_out.pkg().size(), &tmp_msg);

	for (int i = 0; i < tmp_msg.monsters_size();i++) {
		TaskUtils::stat_dup_story_task(
				player, player->temp_info.dup_id, 
				tmp_msg.monsters(i).pet_info().base_info().pet_id(), 
				STAT_STORY_TASK_STATUS_BOSS);
	}

	return 0;
}

int DuplicateBtlNotifyRelayCmdProcessor::relay_notify_to_next_phase(
		DuplicateBtlNotifyRelayCmdProcessor * const p_this,
		player_t *player, battleproto::sc_battle_relay &btl_out)
{
	onlineproto::sc_0x0213_duplicate_notify_to_next_phase next_msg;
	parse_message(btl_out.pkg().c_str(), btl_out.pkg().size(), &next_msg);
    p_this->before_notify_next_phase(player, player->temp_info.dup_id, next_msg);

    return 0;
}

int DuplicateBtlNotifyRelayCmdProcessor::relay_notify_role_recover(
        DuplicateBtlNotifyRelayCmdProcessor * const p_this,
        player_t *player, battleproto::sc_battle_relay &btl_out)
{
    onlineproto::sc_0x022A_duplicate_notify_role_recover tmp_msg;
    parse_message(btl_out.pkg().c_str(), btl_out.pkg().size(), &tmp_msg);
    if (tmp_msg.id() == player->userid) {
        if (tmp_msg.type() == 0) {
            if (tmp_msg.hp()) {
                SET_A(kAttrHp, tmp_msg.hp());
            }
            if (tmp_msg.tp()) {
                SET_A(kAttrTp, tmp_msg.tp());
            }
        } else if (tmp_msg.type() == 1) {
            Pet *pet = PetUtils::get_pet_in_loc(player, tmp_msg.create_tm());
            if (pet && tmp_msg.hp()) {
                pet->set_hp(tmp_msg.hp());
            }
        }
    }

    return 0;
}

int DuplicateBtlNotifyRelayCmdProcessor::relay_notify_hit_charactor(
        DuplicateBtlNotifyRelayCmdProcessor * const p_this,
        player_t *player, battleproto::sc_battle_relay &btl_out)
{
    // 累计家族boss伤害
    onlineproto::sc_0x020B_duplicate_notify_hit_character tmp_msg;
    parse_message(btl_out.pkg().c_str(), btl_out.pkg().size(), &tmp_msg);

    if (tmp_msg.hit_cnt()) {//有打怪掉落
        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(tmp_msg.def_id());
        uint32_t prize_id = 0;
        uint32_t hit_count = 0;
        if (!pet_conf) {
            const builder_conf_t *builder_conf = g_builder_conf_mgr.find_builder_conf(tmp_msg.def_id());
            if (builder_conf) {
                prize_id = builder_conf->hit_prize_id;
                hit_count = builder_conf->hit_prize_count;
            }
        } else {
            prize_id = pet_conf->hit_prize_id;
            hit_count = pet_conf->hit_prize_count;
        }
        if (hit_count && prize_id && (tmp_msg.hit_cnt() % hit_count == 0)) {
            std::vector<uint32_t> drop_prize_vec;
            drop_prize_vec.push_back(prize_id);
            DupUtils::proc_mon_drop_prize(player, drop_prize_vec,
                    tmp_msg.x_pos(), tmp_msg.y_pos());
        }
    }

    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(player->temp_info.dup_id);
    if (dup && (uint32_t)dup->mode == (uint32_t)onlineproto::DUP_MODE_TYPE_FAMILY) {
        // 攻击者是玩家并且攻击伤害>0,副本里配多个野怪的话这里要加boss标志判断
        if ((tmp_msg.atk_type() == 1 || tmp_msg.atk_type() == 2) && 
                tmp_msg.atk_id() == player->userid && 
                tmp_msg.damage() < 0) {
            ADD_A(kAttrFamilyBossDamage, tmp_msg.damage() * (-1));
            uint32_t cur_boss_hp = GET_A(kAttrFamilyBossHp);
            if (cur_boss_hp > (uint32_t)(tmp_msg.damage()*(-1))) {
                SUB_A(kAttrFamilyBossHp, tmp_msg.damage()*(-1));
            } else {
                SET_A(kAttrFamilyBossHp, 0);
            }

            // 攻击通知包不转发给自己,只计算伤害用
            return 0;
        }
    }

    // 世界boss伤害排行
    if (dup && dup->battle_type == DUP_BTL_TYPE_WORLD_BOSS
        && dup->mode == DUP_MODE_TYPE_WORLD_BOSS) {
        TRACE_TLOG("battle relay,[u:%u msg:%s\n[%s]",
                player ?player->userid:0,
                tmp_msg.GetTypeName().c_str(), tmp_msg.Utf8DebugString().c_str());

        // 攻击者是玩家并且攻击伤害>0,副本里配多个野怪的话这里要加boss标志判断
        if ((tmp_msg.atk_type() == DUP_OBJ_TYPE_PLAYER || 
                    tmp_msg.atk_type() == DUP_OBJ_TYPE_PET) && 
                tmp_msg.atk_id() == player->userid && 
                tmp_msg.damage() < 0) {
            ADD_A(kAttrWorldBossDamage, tmp_msg.damage() * (-1));

            uint64_t user_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
            // TODO toby 封装
            // 更新伤害排行
            RankUtils::rank_user_insert_score(
                    player->userid, player->create_tm,
                    commonproto::RANKING_TYPE_WORLD_BOSS_DAMAGE_1, 
                    g_online_id, GET_A(kAttrWorldBossDamage));

            // 更新伤害记录
            world_boss_dup_info_t *dup_info = 
                g_world_boss_mgr.get_world_boss_dup_info(dup->duplicate_id);
            if (dup_info) {
                uint32_t new_damage = GET_A(kAttrWorldBossDamage);
                std::map<uint64_t, user_damage_info_t>::iterator iter = 
                    dup_info->damage_map.find(user_key);  

                user_damage_info_t dinfo;
                dinfo.user_key = user_key; 
                dinfo.nick = player->nick;
                dinfo.damage = new_damage;

                if (iter != dup_info->damage_map.end()) {
                    iter->second = dinfo; 
                } else {
                    dup_info->damage_map.insert(
                            std::pair<uint64_t, user_damage_info_t>(user_key, dinfo));
                }

                // 阶段通知
                if (dup_info->boss_maxhp > 0) {
                    if (dup_info->boss_hp * 1.0/ dup_info->boss_maxhp >= 0.2 &&
                            tmp_msg.mon_cur_hp() * 1.0 / tmp_msg.mon_max_hp() < 0.2) {
                        std::vector<uint32_t> paras;
                        g_world_boss_mgr.world_boss_sys_notify(
                                commonproto::WORLD_BOSS_STATUS_NOTI_HP_LOW, 
                                dup->duplicate_id, paras);
                    }
                }


                // 更新boss血量记录
                dup_info->boss_hp = tmp_msg.mon_cur_hp();
                dup_info->boss_maxhp = tmp_msg.mon_max_hp();
            }

            // 判断是否击杀
            //boss被攻击
            if (tmp_msg.def_type() == DUP_OBJ_TYPE_MON && 
                    tmp_msg.mon_cur_hp() == 0 && 
                    dup_info->kill_user_key == 0) {
                dup_info->kill_user_key = user_key;
                dup_info->kill_flag = true;
                g_world_boss_mgr.give_world_boss_reward(commonproto::WORLD_BOSS_DUP_ID_1);
                //dup_info->status = commonproto::WORLD_BOSS_DUP_REWARD;

                // 阶段通知
                std::vector<uint32_t> paras;
                g_world_boss_mgr.world_boss_sys_notify(
                        commonproto::WORLD_BOSS_STATUS_NOTI_KILL, 
                        dup->duplicate_id, paras);
            }

            // 攻击通知包需要转发自己，pvp时要用到，pve时前端会过滤
        }

        // 玩家被攻击
        if (tmp_msg.def_type() == 1) {
            if (tmp_msg.is_dead() == true && 
                    tmp_msg.def_create_tm() == GET_A(kAttrCreateTm)) {
                // 设置下次自动复活时间
                uint32_t time_gap = g_module_mgr.get_module_conf_uint32_def(
                        module_type_world_boss, "revival_time_gap", 20);
                SET_A(kAttrWorldBossNextRevivalTime, NOW() + time_gap);
            }
        }
    }

    return 0;
}

void OneKeyPassDuplicateCmdProcessor::register_proc_func()
{
	one_key_pass_map_[(uint32_t)onlineproto::DUP_ONE_KEY_MODE_NORMAL] = pass_normal_dup;
	one_key_pass_map_[(uint32_t)onlineproto::DUP_ONE_KEY_MODE_MONSTER_CRISIS] = pass_monster_crisis;
}


uint32_t OneKeyPassDuplicateCmdProcessor::pass_monster_crisis(
		player_t* player, 
		const std::vector<onlineproto::pass_dup_t>& pass_dup,
		uint32_t mode) {
	//lowest_lock_id 最小值 为701,该mode对应的副本id最小值
	//highest_unlock_id 最大值 为 该mode对应的副本id最大值
	const uint32_t BASE_DUP_ID_VALUE = 700;
	//const uint32_t DUP_DIFF = 5;
	//const uint32_t GOLD_BASE = 40;
	//const uint32_t GOLD_INCR = 5;
	uint32_t highest_unlock_id = GET_A(kAttrDupHighestUnlock1Id);
	uint32_t lowest_lock_id = GET_A(kAttrDupLowestLock1Id);
	//重置过后，lowest只能小于等于历史最高通关id值的记录
	if (lowest_lock_id > highest_unlock_id) {
		return cli_err_monster_crisis_dup_id_err;
	}
	//=========计算扫荡券的逻辑规则===========
	uint32_t tmp_low_card_id = lowest_lock_id;
	uint32_t tmp_high_card_id = lowest_lock_id + pass_dup.size() - 1;

	//计算最低关卡与最高关卡间隔的区间数
	uint32_t section_no1 = ceil((tmp_low_card_id - BASE_DUP_ID_VALUE) / 5.0);
	uint32_t section_no2 = ceil((tmp_high_card_id - BASE_DUP_ID_VALUE) / 5.0);
	uint32_t ticket_count = section_no2 - section_no1 + 1;
	/*
	if (tmp_low_card_id % 5 != 0) {
		for (uint32_t i = tmp_low_card_id; i < tmp_high_card_id; ++i) {
			if (i % 5 == 0) {
				ticket_count = 1;
				break;
			}
		}
	}
	while (tmp_low_card_id < tmp_high_card_id) {
		++ticket_count;
		tmp_low_card_id += 5;
	}
	*/
	/*
	uint32_t ticket_count = tmp_high_card_id / DUP_MONS_BOSS_CARD_BASE - 
		tmp_low_card_id / DUP_MONS_BOSS_CARD_BASE;

	if (tmp_high_card_id - tmp_low_card_id <= 3) {
		for (uint32_t i = tmp_low_card_id; i < tmp_high_card_id; ++i) {
			if (i % DUP_MONS_BOSS_CARD_BASE == 0) {
				ticket_count = 1;
				break;
			}
		}
	}
	*/
	//=========计算扫荡券的逻辑规则===========
	std::vector<reduce_item_info_t> reduce_items;
	reduce_item_info_t item;
	item.item_id = 35200;
	item.count = ticket_count;
	reduce_items.push_back(item);
	uint32_t ret = swap_item_by_item_id(player, &reduce_items, 0, false,
		onlineproto::SYNC_REASON_USE_ITEM);	
	if (ret) {
		return ret;
	}
	std::vector<uint32_t> dup_ids_vec;
	//获得配表中 怪物危机模式 下所有的副本id
	ret = g_duplicate_conf_mgr.get_dup_ids_by_mode(
			(uint32_t)DUP_MODE_TYPE_MONSTER_CRISIS, dup_ids_vec);
	if (ret) {
		return ret;
	}
	//前面已经判断过 lowest_lock_id 需要小于等于 highest_unlock_id
	if (highest_unlock_id > dup_ids_vec.back() || 
			lowest_lock_id < dup_ids_vec.front()) {
		return cli_err_monster_crisis_dup_id_err;
	}
	//计算扫荡需要的金币数(20150416，策划说怪物危机的扫荡不要扣金币了)
	/*
	uint32_t need_gold_sum = 0;
	for (uint32_t i = lowest_lock_id; i <= highest_unlock_id; ++i) {
		need_gold_sum += (GOLD_BASE + (i - BASE_DUP_ID_VALUE - 1) / DUP_DIFF * GOLD_INCR);
	}
	if (need_gold_sum > AttrUtils::get_player_gold(player)) {
		return cli_err_gold_not_enough;
	}
	//扣钱
	AttrUtils::sub_player_gold(player, need_gold_sum, "怪物危机扫荡");
	*/
	//结算奖励

	//设置lowest_lock_id 的值为highest_unlock_id + 1
	SET_A(kAttrDupLowestLock1Id, tmp_high_card_id + 1);
	// 扫荡奖励
	onlineproto::sc_0x0246_duplicate_notify_multi_result multi_result;
	for (uint32_t i = lowest_lock_id; i <= tmp_high_card_id; ++i) {
		onlineproto::dup_noti_result_t *noti_result = multi_result.add_noti_results();

        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(i);
		if (dup) {
			multi_result.set_mode((onlineproto::duplicate_mode_t)dup->mode);
		}
		onlineproto::sc_0x0210_duplicate_notify_result tmp_result;
		DupUtils::proc_duplicate_reward(player, dup, tmp_result);
		noti_result->set_win(tmp_result.win());
		noti_result->mutable_vip_award_elems()->CopyFrom(tmp_result.vip_award_elems());
		noti_result->mutable_award_elems()->CopyFrom(tmp_result.award_elems());
		noti_result->set_star(tmp_result.star());
		noti_result->set_time(tmp_result.time());
		noti_result->set_dup_id(i);
		noti_result->set_reason(DUP_RESULT_ONE_KEY_TYPE);
		noti_result->set_pass_cnt(i+1);
		//noti_result->set_mode(onlineproto::DUP_MODE_TYPE_MONSTER_CRISIS);
	}
	multi_result.set_mode(onlineproto::DUP_MODE_TYPE_MONSTER_CRISIS);
	return send_msg_to_player(
			player, 
			cli_cmd_cs_0x0246_duplicate_notify_multi_result, 
			multi_result);
}

uint32_t OneKeyPassDuplicateCmdProcessor::pass_normal_dup(
		player_t* player, const std::vector<onlineproto::pass_dup_t>& pass_dup, uint32_t mode) 
{
	FOREACH(pass_dup, it) {
		uint32_t dup_id = (*it).dup_id();
        uint32_t pass_cnt = (*it).pass_cnt();
        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
        // 特定副本不能用通用模式扫荡
        if (dup->mode == DUP_MODE_TYPE_MONSTER_CRISIS) {
		    return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_duplicate_btl_mode_err);
        }

        // 通用副本条件检查
        int ret = DupUtils::can_enter_duplicate(player, dup_id);
        if (ret) {
			return ret;
        }
        // 扫荡条件检查
        // 三星通过
        attr_type_t attr = AttrUtils::get_duplicate_best_star_attr(dup_id);
        uint32_t best_star = GET_A(attr);
        if (best_star < DUP_PASS_STAR_THREE) {
            return cli_err_not_pass_duplicate_three_star;
        }
        // 扫荡道具
        std::vector<reduce_item_info_t> reduce_vec;
        reduce_item_info_t reduce;
        reduce.item_id = 35200;
        reduce.count = pass_cnt;
        reduce_vec.push_back(reduce);
        ret = swap_item_by_item_id(player, &reduce_vec, 0, false);
        if (ret) {
            ERROR_TLOG("Duplicate reduce item failed :%u", ret);
            return ret;
        }

        // 扫荡奖励
        onlineproto::sc_0x0246_duplicate_notify_multi_result multi_result;
        if (dup && dup_id > 0) {
				multi_result.set_mode((onlineproto::duplicate_mode_t)dup->mode);
            for (uint32_t i = 0; i < pass_cnt;i++) {
                onlineproto::dup_noti_result_t *noti_result = multi_result.add_noti_results();

                onlineproto::sc_0x0210_duplicate_notify_result tmp_result;
                DupUtils::proc_duplicate_reward(player, dup, tmp_result);
                noti_result->set_win(tmp_result.win());
                noti_result->mutable_vip_award_elems()->CopyFrom(tmp_result.vip_award_elems());
                noti_result->mutable_award_elems()->CopyFrom(tmp_result.award_elems());
                noti_result->set_star(tmp_result.star());
                noti_result->set_time(tmp_result.time());
                noti_result->set_dup_id(dup_id);
                noti_result->set_reason(DUP_RESULT_ONE_KEY_TYPE);
                noti_result->set_pass_cnt(i+1);

                // 通用副本记录更新
                DupUtils::reduce_duplicate_cost(player, dup_id);

                // 更新悬赏任务
                TaskUtils::update_daily_reward_task_duplicate_step(player, dup, 1);
            }
        }

        send_msg_to_player(player, cli_cmd_cs_0x0246_duplicate_notify_multi_result, multi_result);
	}
	return 0;
}

int OneKeyPassDuplicateCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
	std::vector<onlineproto::pass_dup_t> pass_dups;
	for (int i = 0 ; i < cli_in_.pass_dup_size(); ++i) {
        if (!g_duplicate_conf_mgr.duplicate_exist(cli_in_.pass_dup(i).dup_id())) {
            return send_err_to_player(
                    player, player->cli_wait_cmd, 
                    cli_err_duplicate_id_not_found);
        }
		pass_dups.push_back(cli_in_.pass_dup(i));
	}
	onlineproto::duplicate_one_key_mode_t mode = cli_in_.mode();
	if (one_key_pass_map_.count((uint32_t)mode) == 0) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_duplicate_btl_mode_err);
	}

    // 更新悬赏任务
    FOREACH(pass_dups, iter) {
        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(iter->dup_id());
        if (dup) {
            std::map<uint32_t, uint32_t> reward_task_map;
            reward_task_map[DUP_MODE_TYPE_NORMAL] = REWARD_TASK_ITEM_NORMAL_DUP;
            reward_task_map[DUP_MODE_TYPE_ELITE] = REWARD_TASK_ITEM_ELITE_DUP;
            reward_task_map[DUP_MODE_TYPE_TRIAL] = REWARD_TASK_ITEM_KING_FIELD;
            reward_task_map[DUP_MODE_TYPE_ELEM_DUP] = REWARD_TASK_ITEM_ELEM_FIGHT;
            reward_task_map[DUP_MODE_TYPE_MONSTER_CRISIS] = REWARD_TASK_ITEM_MONSTER_CRISIS;
            reward_task_map[DUP_MODE_TYPE_BUCKET] = REWARD_TASK_ITEM_BUCKET_DUP;
			reward_task_map[DUP_MODE_TYPE_NIGHT_RAID] = REWARD_TASK_ITEM_NIGHT_RAID;
            std::map<uint32_t, uint32_t>::iterator iter = reward_task_map.find(dup->mode);
            if (iter != reward_task_map.end()) {
                TaskUtils::update_task_step_condition_attr(
                        player, iter->second, 1, 1);       
            }

			if (dup->battle_type == DUP_BTL_TYPE_PPVE) {
				TaskUtils::update_task_step_condition_attr(player,
						REWARD_TASK_ITEM_PPVE, 1, 1);
			}
        }
    }

	uint32_t ret = (one_key_pass_map_.find((uint32_t)mode)->second) (player, pass_dups, (uint32_t)mode);
	if (ret) {
		return send_err_to_player(
				player, 
				player->cli_wait_cmd, 
				ret);
	}

    FOREACH(pass_dups, iter) {
        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(iter->dup_id());
        if (dup) {
			TaskUtils::listen_duplicate_complete(player, dup->duplicate_id);
			if (dup->mode == DUP_MODE_TYPE_ELITE) {
				AttrUtils::add_attr_in_special_time_range(player,
						TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
						kAttrActivEliteDupCnt, cli_in_.pass_dup(0).pass_cnt());
			}
		}
	}


	cli_out_.Clear();
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    return 0;
}

#if 0
int DupSwitchFightPetCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    uint32_t cmd = cli_cmd_cs_0x0229_duplicate_switch_fight_pet;
    PARSE_MSG;
    RELAY_IN;
}

int DupSwitchFightPetCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    RELAY_OUT;
}
#endif

int DupFrontMonFlushReqCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    uint32_t cmd = cli_cmd_cs_0x022B_duplicate_mon_flush_request;
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    RELAY_IN_IF_WAIT(NO_WAIT_SVR);
}

int DupBuyDailyCntCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    const duplicate_t *dup_conf = g_duplicate_conf_mgr.find_duplicate(cli_in_.dup_id());
    if (!dup_conf) {
        RET_ERR(cli_err_duplicate_id_not_found);
    }
    
    //重置购买
    uint32_t buy_cnt = cli_in_.cnt();
    attr_type_t reset_attr;
	uint32_t product_id = 0;
    if (cli_in_.type() == 1) {
		//type等于时适用于精英副本，一次性购买3次
        reset_attr = AttrUtils::get_duplicate_daily_reset_times_attr(cli_in_.dup_id());
        uint32_t limit;
        if (is_gold_vip(player)) {
            limit = 3;
        } else if (is_silver_vip(player)) {
            limit = 2;
        } else {
            limit = 1;
        }
        if (GET_A(reset_attr) >= limit) {
            RET_ERR(cli_err_dup_reset_cnt_limit);
        }
        buy_cnt = 3;
		product_id = 90015;
    } else {
		reset_attr = AttrUtils::get_duplicate_daily_reset_times_attr(cli_in_.dup_id());
		uint32_t limit;
		if (is_gold_vip(player)) {
			limit = dup_conf->svip_buy_fight_cnt_limit;
		} else if (is_silver_vip(player)) {
			limit = dup_conf->vip_buy_fight_cnt_limit;
		} else {
			limit = dup_conf->buy_fight_cnt_limit;
		}
		if (GET_A(reset_attr) >= limit) {
			RET_ERR(cli_err_dup_reset_cnt_limit);
		}
		buy_cnt = dup_conf->default_buy_fight_cnt;
		product_id = dup_conf->buy_fight_shop_id;
		
        //RET_ERR(cli_err_invalid_op);
    }

    int ret = buy_attr_and_use(player, kServiceBuyDuplicateDailyTimes,
            product_id, buy_cnt);
    if (ret) {
        RET_ERR(ret);
    }
	ADD_A(reset_attr, 1);

    attr_type_t cnt_attr;
    if (dup_conf->limit_type == DUP_LIMIT_DAILY) {
        cnt_attr = AttrUtils::get_duplicate_daily_times_attr(cli_in_.dup_id());
    } else {
        cnt_attr = AttrUtils::get_duplicate_weekly_times_attr(cli_in_.dup_id());
    }
    ADD_A(cnt_attr, buy_cnt);
    RET_MSG;
}

int ResetDupCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
	if ((uint32_t)(cli_in_.mode()) >= (uint32_t)DUP_MODE_TYPE_END) {
		RET_ERR(cli_err_mode_value_invalid);
	}
	uint32_t mode = cli_in_.mode();
	//判断重置副本的条件
	uint32_t ret = DupUtils::can_reset_dups(player, mode);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd,	ret);
	}
	DupUtils::reset_dups_record(player, mode);
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

int SkillEffectCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    uint32_t cmd = cli_cmd_cs_0x022D_skill_affect;
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    RELAY_IN_IF_WAIT(NO_WAIT_SVR);
}

//放弃匹配
int GiveUpMatchCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    duplicate_battle_type_t btl_type = DupUtils::get_player_duplicate_battle_type(player);
    switch (btl_type) {
    case DUP_BTL_TYPE_PPVE:
    case DUP_BTL_TYPE_RPVP:
        break;
    default:
        RET_ERR(cli_err_not_match_dup);
    }
    if (DupUtils::is_player_in_duplicate(player)) {
        RET_ERR(cli_err_battle_start);
    }
    send_buff_to_player(player, player->cli_wait_cmd, 0, 0);

    player->temp_info.ready_enter_dup_id = 0;
    battleproto::cs_battle_give_up_match btl_in;
    return g_battle_center->send_msg(0, player->userid, player->create_tm,
            btl_cmd_give_up_match, btl_in);
}

int BattleNotifyDownCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    //退出副本 设置CD时间
    SET_A(AttrUtils::get_duplicate_last_play_time_attr(player->temp_info.dup_id), NOW());
    DupUtils::clear_player_dup_info(player);
    onlineproto::sc_0x023C_notify_btl_server_down noti_msg;
    return send_msg_to_player(player, cli_cmd_cs_0x023C_notify_btl_server_down, noti_msg);
}

int GetWorldBossDupInfoCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    world_boss_dup_info_t *dup_info = 
        g_world_boss_mgr.get_world_boss_dup_info(commonproto::WORLD_BOSS_DUP_ID_1);
    if (dup_info == NULL) {
        g_world_boss_mgr.init(commonproto::WORLD_BOSS_DUP_ID_1);
        dup_info = g_world_boss_mgr.get_world_boss_dup_info(commonproto::WORLD_BOSS_DUP_ID_1);
        if (dup_info == NULL) {
		    RET_ERR(cli_err_world_boss_info_not_exist);
        }
    }

    uint32_t left_time = 
        g_world_boss_mgr.get_next_status_left_time(commonproto::WORLD_BOSS_DUP_ID_1);
    cli_out_.Clear();
    cli_out_.set_status((commonproto::world_boss_dup_status_t)dup_info->status);
    cli_out_.set_left_time(left_time);
    return send_msg_to_player(player, cli_cmd_cs_0x023F_get_world_boss_dup_info, cli_out_);
}

 
int PVEPMatchCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

	// 当前夜袭玩法所处的阶段
	uint32_t cur_phase = GET_A(kAttrNightRaidPhasesFlag);
	uint32_t card_id = player->nightraid->get_cur_id_from_attr_data(cur_phase);
	if(card_id > NIGHT_RAID_HIGHEST_CARD_ID){
        cli_out_.Clear();
        cli_out_.set_u_cur_hp(GET_A(kAttrNightRaidUserCurHp));
        cli_out_.set_u_cur_max_hp(PlayerUtils::obj_hp_add_common(GET_A(kAttrHpMax), GET_A(kAttrCurBattleValue)));
        RET_MSG;
	}
	
	// 第一次玩或者重置
	if(0 == cur_phase){
		//设置玩家血量
		cur_phase = taomee::set_bit_on(cur_phase, 1);
		SET_A(kAttrNightRaidPhasesFlag, cur_phase);
		player->nightraid->set_cur_card_id(1);
        uint32_t player_raid_hp = PlayerUtils::obj_hp_add_common(GET_A(kAttrHpMax), GET_A(kAttrCurBattleValue));;
		SET_A(kAttrNightRaidUserCurHp, player_raid_hp);
		//精灵血量
		FOREACH(*player->pets, it) {
			Pet* pet = &it->second;
			if (pet == NULL) {
				continue;
			}
            uint32_t pet_raid_hp = PlayerUtils::obj_hp_add_common(pet->max_hp(), pet->power());
            pet->set_tmp_max_hp(pet_raid_hp);
			pet->set_night_raid_hp(pet_raid_hp);
			PetUtils::save_pet(player, *pet, false, false);
		}

		uint32_t cur_power = GET_A(kAttrBattleValueRecord); 	
		match_info_t inf;
		player->nightraid->get_opponent_match_args(inf, cur_power);
		std::set<uint64_t> role_set;
		player->nightraid->get_opponent_uids(role_set);
		std::set<uint64_t> users = (*player->temp_info.dirty_users);
		role_set.insert(users.begin(), users.end());
        role_set.insert(ROLE_KEY(ROLE(player->userid, player->create_tm)));
		return RankUtils::get_users_by_score_range(player,
				commonproto::RANKING_TOTAL_POWER, 0, 
				inf.low_score, inf.high_score, role_set, inf.unit_val);
	} else {
		//已经玩过，从db拉取对手基本信息
		return PlayerUtils::get_user_raw_data(player, dbproto::NIGHT_RAID_PLAYER_BASE_INF);
	}
}
int PVEPMatchCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	switch(player->serv_cmd) {
		case ranking_cmd_get_users_by_score:
			return proc_pkg_from_serv_aft_get_infos_from_redis(
					player, body, bodylen);
		case db_cmd_user_raw_data_get:
			return proc_pkg_from_serv_aft_get_infos_from_db(
					player, body, bodylen);
		case cache_cmd_ol_req_users_info:
			return proc_pkg_from_serv_aft_get_infos_from_cache(
				player, body, bodylen);
	}
	return 0;
}


int PVEPMatchCmdProcessor::proc_pkg_from_serv_aft_get_infos_from_db(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
    if (db_out_.raw_data().size() == 0) {//db拉到的数据有误 重新匹配
        uint32_t cur_power = GET_A(kAttrBattleValueRecord); 	
        match_info_t inf;
        player->nightraid->get_opponent_match_args(inf, cur_power);
        std::set<uint64_t> role_set = (*player->temp_info.dirty_users);
        role_set.insert(ROLE_KEY(ROLE(player->userid, player->create_tm)));
        // std::set<uint64_t> role_set;
        // player->nightraid->get_opponent_uids(role_set);
        return RankUtils::get_users_by_score_range(player,
                commonproto::RANKING_TOTAL_POWER, 0, 
                inf.low_score, inf.high_score, role_set, inf.unit_val);
    }
	const dbproto::user_raw_data_type_t type = db_out_.type();
	if(type == dbproto::NIGHT_RAID_PLAYER_BASE_INF){
		commonproto::pvep_opponent_list op_list;
		op_list.ParseFromString(db_out_.raw_data());
		player->nightraid->unpack_msg_list_to_night_raid_info_map(op_list);

		uint32_t cur_phase = GET_A(kAttrNightRaidPhasesFlag);
		uint32_t cur_win = GET_A(kAttrNightRaidUnlockFlag);
		uint32_t card_id = player->nightraid->get_cur_id_from_attr_data(cur_phase);
		uint32_t win_id = player->nightraid->get_cur_id_from_attr_data(cur_win);
		player->nightraid->set_cur_card_id(card_id);
		player->nightraid->set_cur_win_id(win_id);
		if(int(card_id) != op_list.opponent_data_size()){
			ERROR_TLOG("night_raid data not match,cardid=[%u] db_cardid= [%u]",card_id, op_list.opponent_data_size());
			// return -1;
		}
		bool WIN_CUR_ID = false;
		WIN_CUR_ID = player->nightraid->is_cur_phases_win_night_raid();
		if (WIN_CUR_ID){
			//匹配新对手
			cur_phase = taomee::set_bit_on(cur_phase, card_id + 1);
			SET_A(kAttrNightRaidPhasesFlag, cur_phase);
			player->nightraid->set_cur_card_id(card_id + 1);

			uint32_t cur_power = GET_A(kAttrBattleValueRecord); 	
			match_info_t inf;
			player->nightraid->get_opponent_match_args(inf, cur_power);
			std::set<uint64_t> role_set = (*player->temp_info.dirty_users);
            role_set.insert(ROLE_KEY(ROLE(player->userid, player->create_tm)));
            // std::set<uint64_t> role_set;
			// player->nightraid->get_opponent_uids(role_set);
			return RankUtils::get_users_by_score_range(player,
					commonproto::RANKING_TOTAL_POWER, 0, 
					inf.low_score, inf.high_score, role_set, inf.unit_val);
		} else {
			return PlayerUtils::get_user_raw_data(player, dbproto::NIGHT_RAID_PLAYER_BATTLE_INF);
		}
	} else if (type == dbproto::NIGHT_RAID_PLAYER_BATTLE_INF){
		player->nightraid->btl_player_info_	= db_out_.raw_data();
		commonproto::battle_player_data_t btl_info;
		player->nightraid->get_cur_night_raid_battle_info(&btl_info);
		//更新内存
		player->nightraid->add_new_card_night_raid_info(btl_info);

		cli_out_.Clear();
		commonproto::pvep_match_data_t *op_data = cli_out_.add_match_data();
		player->nightraid->pack_cur_card_night_raid_info(op_data);
		uint32_t u_cur_hp = GET_A(kAttrNightRaidUserCurHp);
		cli_out_.set_u_cur_hp(u_cur_hp);
        cli_out_.set_u_cur_max_hp(PlayerUtils::obj_hp_add_common(GET_A(kAttrHpMax), GET_A(kAttrCurBattleValue)));
		return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
	}
	return 0;
}


int PVEPMatchCmdProcessor::proc_pkg_from_serv_aft_get_infos_from_redis(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	uint32_t op_id = rank_out_.player_info().userid();
	uint32_t op_create_tm = rank_out_.player_info().u_create_tm();
	if(0 == op_id){
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_uid_not_exist);
	} else if (0 == op_create_tm){
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_create_tm_is_null);
	}

	std::set<uint64_t> &role_set = (*player->temp_info.dirty_users);
	player->nightraid->get_opponent_uids(role_set);
	role_set.insert(ROLE_KEY(ROLE(op_id , op_create_tm)));
	//去缓存中拉取对手信息
	cacheproto::cs_batch_get_users_info cache_info_in_;
	commonproto::role_info_t* pb_ptr = cache_info_in_.add_roles();
	pb_ptr->set_userid(op_id);
	pb_ptr->set_u_create_tm(op_create_tm);
	return g_dbproxy->send_msg(player, player->userid, player->create_tm,
			cache_cmd_ol_req_users_info, cache_info_in_);
}

int PVEPMatchCmdProcessor::proc_pkg_from_serv_aft_get_infos_from_cache(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(cache_info_out_);
	uint32_t cache_info_size = cache_info_out_.user_infos_size();
	// uint32_t trytimes = 10;
	if (cache_info_size == 0) {
		// while(trytimes > 0){
			// uint32_t cur_power = GET_A(kAttrCurBattleValue); 	
			// match_info_t inf;
			// player->nightraid->get_opponent_match_args(inf, cur_power);
			// std::set<uint64_t> role_set;
			// player->nightraid->get_opponent_uids(role_set);
			// return RankUtils::get_users_by_score_range(player,
					// commonproto::RANKING_TOTAL_POWER, 0, 
					// inf.low_score, inf.high_score, role_set, inf.unit_val);
		// }
		uint32_t cur_card =  player->nightraid->get_cur_card_id();
		uint32_t cur_phase = GET_A(kAttrNightRaidPhasesFlag);
		cur_phase = taomee::set_bit_off(cur_phase, cur_card);
		SET_A(kAttrNightRaidPhasesFlag, cur_phase);
		if(cur_card > 1){
			cur_card --;
		}
		player->nightraid->set_cur_card_id(cur_card);
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_exped_can_not_get_player_info);
	}
	//==========更新精灵cur_hp为max_hp=============
	commonproto::battle_pet_list_t* tmp_pb_btl_ptr = cache_info_out_.mutable_user_infos(0)->mutable_pet_list();
	const commonproto::battle_pet_list_t& tmp_pb_btl_inf = cache_info_out_.user_infos(0).pet_list();
	for (int i = 0; i < tmp_pb_btl_inf.pet_list_size(); ++i) {
		uint32_t max_hp = tmp_pb_btl_inf.pet_list(i).pet_info().battle_info().max_hp();
        uint32_t power = tmp_pb_btl_inf.pet_list(i).pet_info().base_info().power();
        uint32_t new_hp = PlayerUtils::obj_hp_add_common(max_hp, power);
		tmp_pb_btl_ptr->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_max_hp(new_hp);
        tmp_pb_btl_ptr->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_cur_hp(new_hp);
    }

	//更新玩家血量
	commonproto::battle_player_data_t *info = cache_info_out_.mutable_user_infos(0);
	uint32_t max_hp = info->battle_info().max_hp();
    uint32_t p_power = info->base_info().power();
    uint32_t new_p_hp = PlayerUtils::obj_hp_add_common(max_hp, p_power);
	info->mutable_battle_info()->set_cur_hp(new_p_hp);	
    info->mutable_battle_info()->set_max_hp(new_p_hp);

	// 匹配到新的对手并成功获取其数据并将数据添加进内存
	commonproto::battle_player_data_t player_info = cache_info_out_.user_infos(0);
	player->nightraid->add_new_card_night_raid_info(player_info);
	// //匹配到对手 进度增加
	// uint32_t cur_phase = GET_A(kAttrNightRaidPhasesFlag);
	// uint32_t cur_id = player->nightraid->get_cur_card_id();
	// cur_phase = taomee::set_bit_on(cur_phase, cur_id);
	// SET_A(kAttrNightRaidPhasesFlag, cur_phase);

	DEBUG_TLOG("night raid player_info size = %u", player_info.ByteSize());
	player->nightraid->btl_player_info_.clear();
	//信息过长，需要清理一些不需要的信息
	commonproto::battle_player_data_t cache_player_info;
	cache_player_info.Clear();
	DataProtoUtils::trim_battle_player_info(player, player_info, cache_player_info);
	// player_info.release_switch_pet_list();
	// player_info.Swap(&cache_player_info);
	cache_player_info.SerializePartialToString(&(player->nightraid->btl_player_info_));
	DEBUG_TLOG("night raid player_info cache_player_info size = %u,uid=%u, str_size=%u",
			cache_player_info.ByteSize(), player->userid, player->nightraid->btl_player_info_.size());
	if(player->nightraid->btl_player_info_.size() >= 4096){
		ERROR_TLOG("raw_data over limit size = %u,uid=%u", player->nightraid->btl_player_info_.size(), player->userid);
	}
	//打包详细到buff 
	PlayerUtils::update_user_raw_data(
			player->userid, player->create_tm, dbproto::NIGHT_RAID_PLAYER_BATTLE_INF,
			cache_player_info, "0");
	//打包基本信息
	commonproto::pvep_opponent_list op_list_;
    op_list_.Clear();
    player->nightraid->pack_all_card_night_raid_info(op_list_);
    PlayerUtils::update_user_raw_data(
            player->userid, player->create_tm, dbproto::NIGHT_RAID_PLAYER_BASE_INF,
            op_list_, "0");
    /////////////////////////////////////////////////////////////
	cli_out_.Clear();
	commonproto::pvep_match_data_t *op_data = cli_out_.add_match_data();
	player->nightraid->pack_cur_card_night_raid_info(op_data);

	//当前夜袭所处的阶段
	// uint32_t cur_phase = GET_A(kAttrNightRaidPhasesFlag);
	// if (0 == cur_phase) {//首次玩或者重置
		// SET_A(kAttrNightRaidUserCurHp, GET_A(kAttrHpMax));
	// }
	uint32_t u_cur_hp = GET_A(kAttrNightRaidUserCurHp);
	cli_out_.set_u_cur_hp(u_cur_hp);
    cli_out_.set_u_cur_max_hp(PlayerUtils::obj_hp_add_common(GET_A(kAttrHpMax), GET_A(kAttrCurBattleValue)));

	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	

}

//夜袭玩法
int EnterDuplicateCmdProcessor::before_enter_night_raid(
		player_t *player, uint32_t dup_id)
{
	uint32_t mode = DupUtils::get_duplicate_mode(dup_id);
	if (mode != (uint32_t)DUP_MODE_TYPE_NIGHT_RAID) {
		RET_ERR(cli_err_duplicate_btl_mode_err);
	}
	std::vector<uint32_t> dup_ids;
	g_duplicate_conf_mgr.get_dup_ids_by_mode(mode, dup_ids);
	assert(dup_ids.size());
	uint32_t cur_win = GET_A(kAttrNightRaidUnlockFlag);
	uint32_t cur_phase = GET_A(kAttrNightRaidPhasesFlag);
	uint32_t got_reward = GET_A(kAttrNightRaidGotPrizeFlag);
	uint32_t card_id = player->nightraid->get_cur_id_from_attr_data(cur_phase);
	uint32_t win_id = player->nightraid->get_cur_id_from_attr_data(cur_win);
	uint32_t reward_id = player->nightraid->get_cur_id_from_attr_data(got_reward );
	if (card_id != win_id + 1 || win_id != reward_id) {
		RET_ERR(cli_err_dup_id_not_unlock);
	}
	//血量为0不能进副本
	if(0 == GET_A(kAttrNightRaidUserCurHp)){
	    RET_ERR(cli_err_lack_blood_volume);
	}
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet == NULL) {
			continue;
		}
        //改临时最大血量
		pet->set_tmp_max_hp(PlayerUtils::obj_hp_add_common(pet->max_hp(), pet->power()));
		pet->set_hp(pet->night_raid_hp());
	}
	SET_A(kAttrHp, GET_A(kAttrNightRaidUserCurHp));
    //改临时最大血量
    player->temp_info.tmp_max_hp = PlayerUtils::obj_hp_add_common(GET_A(kAttrHpMax), GET_A(kAttrCurBattleValue));

	//玩家临时伤害记录
	(*player->temp_info.night_raid_pets).clear();
	player->temp_info.night_raid_player_hp = GET_A(kAttrHp);
	//对手临时伤害记录
	(*player->temp_info.night_raid_op_pets).clear();
	player->temp_info.night_raid_op_player_hp= 0;
	return 0;
}

int EnterDuplicateCmdProcessor::after_enter_night_raid(player_t *player, uint32_t dup_id,
        battleproto::sc_battle_duplicate_enter_map &btl_out,
        onlineproto::sc_0x0201_duplicate_enter_map &cli_out)
{
#if 0
    FOREACH(*player->pets, it) {
        Pet* pet = &it->second;
        if (pet == NULL) {
            continue;
        }
        //还原精灵临时最大血量
        pet->set_tmp_max_hp(pet->max_hp());
    }
    //还原玩家临时最大血量
    player->temp_info.tmp_max_hp = GET_A(kAttrHpMax);
#endif
	return 0;
}

int StatDuplicateCmdProcessor::after_night_raid( player_t *player, uint32_t dup_id, bool win)
{
    //如果是掉线 则不做结算
    if (player->temp_info.dup_state == PLAYER_DUP_OFFLINE) {
        return 0;
    }
	if (win) {
		//已经过的关卡数
		uint32_t dup_unlock_num = 0;
		uint32_t dup_unlock_flag = GET_A(kAttrNightRaidUnlockFlag);
		dup_unlock_num = player->nightraid->get_cur_id_from_attr_data(dup_unlock_flag);

		std::vector<uint32_t> dup_ids;
		g_duplicate_conf_mgr.get_dup_ids_by_mode(DUP_MODE_TYPE_NIGHT_RAID, dup_ids);
		if (dup_id == dup_ids.front() && dup_ids.size() == 1) {
			dup_unlock_flag = taomee::set_bit_on(dup_unlock_flag, dup_unlock_num + 1);
			SET_A(kAttrNightRaidUnlockFlag, dup_unlock_flag);
			player->nightraid->set_cur_win_id(dup_unlock_num + 1);
		}
		//更新夜袭最高关卡历史记录
		uint32_t cur_card_id = player->nightraid->get_cur_card_id();
		if (cur_card_id > GET_A(kAttrNightRaidHistoryMaxCard)) {
			SET_A(kAttrNightRaidHistoryMaxCard, cur_card_id);
		}
	} else {
		//输了保存对手数据
		// 对手血量
		commonproto::battle_player_data_t btl_info;
		player->nightraid->get_cur_night_raid_battle_info(&btl_info);

		//==========更新精灵cur_hp=============
		commonproto::battle_pet_list_t* tmp_pb_btl_ptr = btl_info.mutable_pet_list();
		const commonproto::battle_pet_list_t& tmp_pb_btl_inf = btl_info.pet_list();
		for (int i = 0; i < tmp_pb_btl_inf.pet_list_size(); ++i) {
			uint32_t create_tm = tmp_pb_btl_inf.pet_list(i).pet_info().base_info().create_tm();
			std::map<uint32_t, int> pet_hp_map(* (player->temp_info.night_raid_op_pets)); //对手
			std::map<uint32_t, int>::iterator it = pet_hp_map.find(create_tm);
			if (it != pet_hp_map.end()){
				uint32_t init_hp = tmp_pb_btl_inf.pet_list(i).pet_info().battle_info().cur_hp();
				uint32_t op_cur_hp = init_hp + it->second;
				tmp_pb_btl_ptr->mutable_pet_list(i)->mutable_pet_info()->mutable_battle_info()->set_cur_hp(op_cur_hp);
			}
		}
		//更新玩家血量
		uint32_t init_hp = btl_info.battle_info().cur_hp();
		uint32_t op_cur_hp = init_hp + player->temp_info.night_raid_op_player_hp;
		btl_info.mutable_battle_info()->set_cur_hp(op_cur_hp);	

		player->nightraid->btl_player_info_.clear();
		btl_info.SerializeToString(&player->nightraid->btl_player_info_);
		//打包精灵详细到buff 
		PlayerUtils::update_user_raw_data(
				player->userid, player->create_tm, dbproto::NIGHT_RAID_PLAYER_BATTLE_INF,
				btl_info, "0");
		player->nightraid->add_new_card_night_raid_info(btl_info);
		////打包基本信息
		commonproto::pvep_opponent_list op_list_;
		player->nightraid->pack_all_card_night_raid_info(op_list_);
		PlayerUtils::update_user_raw_data(
				player->userid, player->create_tm, dbproto::NIGHT_RAID_PLAYER_BASE_INF,
				op_list_, "0");
	}

	//夜袭保存并还原
	FOREACH(*player->temp_info.night_raid_pets, it) {
		Pet *pet = PetUtils::get_pet_in_loc(player, it->first);
		if (pet == NULL) {
			continue;
        }
        pet->set_night_raid_hp(it->second);
        pet->set_tmp_max_hp(pet->max_hp());
		PetUtils::save_pet(player, *pet, false, true);
    }

	uint32_t cur_hp = player->temp_info.night_raid_player_hp;
	SET_A(kAttrNightRaidUserCurHp, cur_hp);
	//清理缓存
    (*player->temp_info.night_raid_pets).clear();
	player->temp_info.night_raid_player_hp = 0;
    (*player->temp_info.night_raid_op_pets).clear();
	player->temp_info.night_raid_op_player_hp = 0;

	AttrUtils::add_attr_in_special_time_range(player,
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,			
			kAttrActivNightRaidCnt);
	return 0;
}

int PVEPRevivalCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    uint32_t revival_cnt_limit = 0;
	if (is_gold_vip(player)) {
        revival_cnt_limit = 2;

	} else if (is_silver_vip(player)) {
        revival_cnt_limit = 1;

    } else if (!is_vip(player)) {
		revival_cnt_limit = 0;
    }

    if (GET_A(kDailyNightRaidFreeReviveTimes) >= revival_cnt_limit) {
        RET_ERR(cli_err_night_raid_revive_cnt_limit);
    }

    //购买复活
    int ret = buy_attr_and_use(player, kServiceBuyNightRaidRevive, 90033, 1);
    if (ret) {
        RET_ERR(ret);
    }
    ADD_A(kDailyNightRaidFreeReviveTimes, 1);

    //重置血量
	SET_A(kAttrNightRaidUserCurHp, PlayerUtils::obj_hp_add_common(GET_A(kAttrHpMax), GET_A(kAttrCurBattleValue)));
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet == NULL) {
			continue;
		}
        uint32_t night_raid_max_hp = PlayerUtils::obj_hp_add_common(pet->max_hp(), pet->power());
        pet->set_tmp_max_hp(night_raid_max_hp);
		pet->set_night_raid_hp(night_raid_max_hp);
		PetUtils::save_pet(player, *pet, false, false);
	}

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int PVEPResetCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
	uint32_t times = GET_A(kDailyNightRaidResetTimes);
	if(times >= 1){
        RET_ERR(cli_err_dup_reset_cnt_limit);
	}

    //重置血量
	SET_A(kAttrNightRaidUserCurHp, PlayerUtils::obj_hp_add_common(GET_A(kAttrHpMax), GET_A(kAttrCurBattleValue)));
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet == NULL) {
			continue;
		}
        uint32_t night_raid_max_hp = PlayerUtils::obj_hp_add_common(pet->max_hp(), pet->power());
        pet->set_tmp_max_hp(night_raid_max_hp);
        pet->set_night_raid_hp(night_raid_max_hp);
		PetUtils::save_pet(player, *pet, false, false);
	}

	SET_A(kAttrNightRaidPhasesFlag, 0);
	SET_A(kAttrNightRaidUnlockFlag, 0);
	SET_A(kAttrNightRaidGotPrizeFlag, 0);
	player->nightraid->set_cur_card_id(0);
	player->nightraid->set_cur_win_id(0);
	player->nightraid->clear_night_raid_map();
	player->nightraid->btl_player_info_.clear();

	PlayerUtils::delete_user_raw_data(player->userid, player->create_tm, 
            dbproto::NIGHT_RAID_PLAYER_BATTLE_INF, "0");

	PlayerUtils::delete_user_raw_data(player->userid, player->create_tm, 
            dbproto::NIGHT_RAID_PLAYER_BASE_INF, "0");

	PlayerUtils::delete_user_raw_data(player->userid, player->create_tm, 
            dbproto::NIGHT_RAID_TOTAL_PRIZE, "0");

	SET_A(kDailyNightRaidResetTimes, times + 1);

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int PVEPPrizeTotalCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PlayerUtils::get_user_raw_data(player, dbproto::NIGHT_RAID_TOTAL_PRIZE);
	return 0;
}

int PVEPPrizeTotalCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	cli_out_.Clear();
	cli_out_.mutable_list()->ParseFromString(db_out_.raw_data());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int BuyDupCleanCDCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    if (!g_duplicate_conf_mgr.find_duplicate(cli_in_.dup_id())) {
        return send_err_to_player(player, player->cli_wait_cmd, 
                cli_err_duplicate_id_not_found);
    }
    int ret = buy_attr_and_use(player, kServiceBuyDupCleanCd, 90064, 1);
    if (ret) {
        RET_ERR(ret);
    }
    SET_A(AttrUtils::get_duplicate_last_play_time_attr(cli_in_.dup_id()), 0);
    RET_MSG;   
}

int BuyPassDupCmdProcessor::proc_pkg_from_client(
		player_t* player, const char *body, int bodylen)
{
    PARSE_MSG;
	const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(cli_in_.dup_id());
	if (dup == NULL) {
		RET_ERR(cli_err_duplicate_id_not_found);
	}
	if (dup->buy_pass_shop_id == 0) {
		RET_ERR(cli_err_can_not_buy_pass_dup);
	}
	const product_t *pd = g_product_mgr.find_product(dup->buy_pass_shop_id);
	if (pd == NULL) {
		RET_ERR(cli_err_product_not_exist);
	}
	uint32_t ret = buy_attr_and_use(player, (attr_type_t)pd->service, dup->buy_pass_shop_id, 1);
	if (ret) {
		RET_ERR(ret);
	}
	DupUtils::use_diamond_buy_pass_dup(player, cli_in_.dup_id());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int PetBuyPassDupCmdProcessor::proc_pkg_from_client(player_t* player,
	const char* body, int bodylen)
{
    PARSE_MSG;
	Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.pet_create_tm());
	if (pet == NULL) {
		RET_ERR(cli_err_bag_pet_not_exist);
	}
	pet_pass_dup_conf_t* conf_ptr = g_pet_pass_dup_conf_mgr.get_pet_pass_dup_info_conf(
			pet->pet_id());
	if (conf_ptr == NULL) {
		RET_ERR(cli_err_pet_not_join_pass_dup_activity);
	}
	
	
	if (cli_in_.has_dup_id()) {
		//通关activity_type下指定的副本
		//找到dup_id 对应的shop_id
		const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(cli_in_.dup_id());
		if (dup == NULL) {
			RET_ERR(cli_err_duplicate_id_not_found);
		} 
		//判断前置副本是否已经通关
		uint32_t unlock_state = conf_ptr->unlock_state_map[cli_in_.activity_type()];
		if (unlock_state) {
			std::vector<dup_shop_id_t>& dup_shop_id_vec = conf_ptr->dup_shopid_map[cli_in_.activity_type()];
			std::sort(dup_shop_id_vec.begin(), dup_shop_id_vec.end(), order_by_dup_id_des);
			std::vector<dup_shop_id_t>::iterator iter;
			uint32_t prev_dup_id = 0;
			FOREACH(dup_shop_id_vec, it) {
				if (it->dup_id == cli_in_.dup_id()) {
					iter = it;
					++iter;
					if (iter != dup_shop_id_vec.end()) {
						prev_dup_id = iter->dup_id;
					}
					break;
				}
			}
			if (prev_dup_id) {
				uint32_t type = 0;
				uint32_t ret = AttrUtils::get_pet_pass_dup_tm_attr(
						prev_dup_id, pet->pet_id(), type);
				if (ret) {
					RET_ERR(ret);
				}
				if (GET_A((attr_type_t)type) == 0) {
					RET_ERR(cli_err_cur_dup_id_still_lock);
				}
			}
		}
		uint32_t shop_id = 0;
		g_pet_pass_dup_conf_mgr.get_shop_id(pet->pet_id(),
				cli_in_.activity_type(), cli_in_.dup_id(), shop_id);
		if (shop_id) {
			const product_t *pd = g_product_mgr.find_product(shop_id);
			if (pd == NULL) {
				RET_ERR(cli_err_product_not_exist);
			}
			int ret = buy_attr_and_use(player, (attr_type_t)pd->service, shop_id, 1);
			if (ret) {
				RET_ERR(ret);
			}
		}
		uint32_t ret = DupUtils::use_diamond_buy_pet_pass_dup(player,
				pet->pet_id(), cli_in_.dup_id());
		if (ret) {
			RET_ERR(ret);
		}
	} else {
		std::vector<dup_shop_id_t>& dup_shop_id_vec = conf_ptr->dup_shopid_map[cli_in_.activity_type()];
		std::sort(dup_shop_id_vec.begin(), dup_shop_id_vec.end(), order_by_dup_id_des);
		//获得总价格
		uint32_t total_price = 0;
		FOREACH(dup_shop_id_vec, it) {
			const product_t* shop_ptr = g_product_mgr.find_product(it->shop_id);
			if (shop_ptr) {
				total_price += shop_ptr->price;
			}
		}
		//通关activity_type下所有的副本
		uint32_t shop_id = conf_ptr->base_shopid_map[cli_in_.activity_type()];
		const product_t *pd = g_product_mgr.find_product(shop_id);
		if (pd == NULL) {
			RET_ERR(cli_err_product_not_exist);
		}

		uint32_t buy_cnt = total_price / (pd->price);
		TRACE_TLOG("Pet One Key Pass Dup, act_type=%u,total_prize=%u,"
				"buy_cnt=%u,base_prize=%u",
				cli_in_.activity_type(), total_price, buy_cnt, pd->price);
		int ret = buy_attr_and_use(player, (attr_type_t)pd->service, shop_id, buy_cnt);
		if (ret) {
			RET_ERR(ret);
		}
		FOREACH(dup_shop_id_vec, it) {
			ret = DupUtils::use_diamond_buy_pet_pass_dup(player,
					pet->pet_id(), it->dup_id);
			if (ret) {
				RET_ERR(ret);
			}
		}
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}
