#include "common.h"
#include "pet_utils.h"
#include "pet.h"
#include "player.h"
#include "service.h"
#include "global_data.h"
#include "data_proto_utils.h"
#include "attr_utils.h"
#include "map_utils.h"
#include "utils.h"
#include "sys_ctrl.h"
#include "item.h"
#include "player_utils.h"
#include "statlogger/statlogger.h"
#include "item.h"
#include "task_utils.h"
#include "rank_utils.h"
#include "skill_conf.h"
#include "duplicate_utils.h"
#include "exped.h"
#include "achieve.h"
#include "prize_processor.h"


int PetUtils::set_pet_loc(player_t *player, uint32_t create_tm, pet_location_type_t to_loc)
{
    std::map<uint32_t, Pet>::iterator ptr = player->pets->find(create_tm);
    if (ptr == player->pets->end()) {
        return cli_err_pet_not_exist;
    }
    Pet &pet = ptr->second;
    if (pet.loc() == to_loc) {
        return 0;
    }

    if (PetUtils::pets_full(player, to_loc)) {
        return cli_err_target_pet_loc_full;
    }

    switch (to_loc) {
    case PET_LOC_BAG:
        (*player->bag_pets)[create_tm] = &pet;
        break;
    case PET_LOC_STORE:
        (*player->store_pets)[create_tm] = &pet;
        break;
    case PET_LOC_ELITE_STORE:
        (*player->room_pets)[create_tm] = &pet;
        break;
    case PET_LOC_ROOM:
        (*player->elite_pets)[create_tm] = &pet;
        break;
    default:
        break;
    }

    switch (pet.loc()) {
    case PET_LOC_BAG:
        player->bag_pets->erase(create_tm);
        break;
    case PET_LOC_STORE:
        player->store_pets->erase(create_tm);
        break;
    case PET_LOC_ELITE_STORE:
        player->elite_pets->erase(create_tm);
        break;
    case PET_LOC_ROOM:
        player->room_pets->erase(create_tm);
        break;
    default:
        break;
    }

    pet.set_loc(to_loc);

    if (pet.fight_pos() && to_loc != PET_LOC_BAG) {
        player->fight_pet[pet.fight_pos()-1] = NULL;
        pet.set_fight_pos(0);
        MapUtils::sync_map_player_info(player, commonproto::PLAYER_FOLLOW_PET_CHANGE);
    }
    save_pet(player, pet, false, true);
    return 0;
}

uint32_t PetUtils::get_pet_count_by_telent(player_t* player,
		pet_talent_level_t telent_lv, uint32_t& count)
{
	count = 0;
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet == NULL) {
			continue;
		}
		if (pet->talent_level() != telent_lv) {
			continue;
		}
		++count;
	}
	return 0;
}

uint32_t PetUtils::get_pet_count_by_quality(player_t* player,
		uint32_t quality, uint32_t& count)
{
	count = 0;
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet == NULL) {
			continue;
		}
		if (pet->quality() != quality) {
			continue;
		}
		++count;
	}
	return 0;
}

uint32_t PetUtils::add_pet_exp(player_t* player, Pet* pet, 
        uint32_t add_exp, uint32_t &real_add_exp,
	   	bool addict_detec, onlineproto::exp_reason_t reason)
{
	//防沉迷
	if(addict_detec && check_player_addicted_threshold_none(player)){
		return 0;
	} else if (addict_detec && check_player_addicted_threshold_half(player)){
		 add_exp /= 2;
	}

    real_add_exp = 0;
    if (add_exp == 0) {
        return 0; 
    }
   // uint32_t player_level = GET_A(kAttrLv);
    //uint32_t level_limit = player_level > kMaxPetLevel ?kMaxPetLevel :player_level;
    uint32_t level_limit = kMaxPetLevel; 
    uint32_t cur_level = pet->level();
    uint32_t exp = 0;

    if (cur_level > level_limit) {//达到等级上限限制
        return 0;

    } else if (cur_level == level_limit) {//等级达到最大 则经验可以加到当前的最大值
        uint32_t level_up_need_exp = get_level_up_need_exp(pet->pet_id(), cur_level);
        if (unlikely(level_up_need_exp < pet->exp())) {
            WARN_TLOG("P:%u, Pet:id_%u tm_%u exp:%u lv:%u exceed max:%u",
                    player->userid, pet->pet_id(), 
                    pet->create_tm(), pet->exp(), pet->level(),
                    level_up_need_exp);
            return 0;
        }
        uint32_t max_can_add_exp = level_up_need_exp - pet->exp();
        if (add_exp >= max_can_add_exp) {
            exp = level_up_need_exp;
            real_add_exp = max_can_add_exp;
        } else {
            exp = pet->exp() + add_exp;
            real_add_exp = add_exp;
        }

    } else {
        uint64_t total_exp = pet->exp() + add_exp;
        uint64_t need_exp = get_level_up_need_exp(pet->pet_id(), cur_level);
        while (need_exp <= total_exp) {
            total_exp -= need_exp;
            real_add_exp += need_exp;
            cur_level++;
            if (cur_level >= level_limit) {
                need_exp = get_level_up_need_exp(pet->pet_id(), cur_level);
                break;
            }
            need_exp = get_level_up_need_exp(pet->pet_id(), cur_level);
        }

        if (total_exp >= need_exp) {
            exp = need_exp;
        } else {
            exp = total_exp;
        }
        real_add_exp += exp;
        real_add_exp -= pet->exp();
    }

    uint32_t add_level = cur_level - pet->level();
    pet->set_exp(exp);
    int ret = 0;
    if (add_level) {
        if (reason == onlineproto::EXP_FROM_BATTLE) {
            ret = PetUtils::pet_level_up(player, pet, add_level, false, 
                    onlineproto::LEVELUP_FROM_BATTLE); 
        } else {
            ret = PetUtils::pet_level_up(player, pet, add_level, false,
                    onlineproto::LEVELUP_FROM_OTHER); 
        }
    } else {
        ret = PetUtils::save_pet(
                player, *pet, false, true, true, onlineproto::POWER_CHANGE_FROM_BATTLE);
    }

    //同步前端经验值变化
    if (reason == onlineproto::EXP_FROM_DRUGS && add_exp > 0) {
        onlineproto::sc_0x030B_pet_exp_notify inform_exp;
        inform_exp.set_create_tm(pet->create_tm());
        inform_exp.set_exp_change(add_exp);
        send_msg_to_player(player, cli_cmd_cs_0x030B_pet_exp_notify, inform_exp);
    }

    return ret;
}

uint32_t PetUtils::add_pet_effort(player_t* player, 
		Pet* pet, int idx, int add_val, uint32_t &real_add_val,
		bool addict_detec)
{
	//防沉迷
	if(addict_detec && check_player_addicted_threshold_none(player)){
		return 0;
	} else if (addict_detec && check_player_addicted_threshold_half(player)){
		 add_val /= 2;
	}

	real_add_val = add_val;
	if (add_val == 0) {
		return 0;
	}
	uint32_t level_limit = pet->level() > kMaxPetLevel ?kMaxPetLevel :pet->level();
	uint32_t cur_effort_lv = pet->effort_lv(idx);
	if (cur_effort_lv >= level_limit) {
		return 0;
	}
	uint64_t total_val = pet->effort_value(idx) + add_val;
	uint64_t need_val = get_effort_lv_up_need_val(pet->pet_id(), cur_effort_lv);
	while (need_val <= total_val) {
		total_val -= need_val;
		cur_effort_lv++;
		if (cur_effort_lv >= pet->level()) {
			need_val = get_effort_lv_up_need_val(pet->pet_id(), cur_effort_lv);
			break;
		}
		need_val = get_effort_lv_up_need_val(pet->pet_id(), cur_effort_lv);
	}
	
	uint32_t val = 0;
	if (total_val >= need_val) {
		val = need_val;
	} else {
		val = total_val;
	}
	uint32_t old_effort_lv = pet->effort_lv(idx);
	uint32_t add_effort_lv = cur_effort_lv - old_effort_lv;
	pet->set_effort_value(idx, val);
	int ret = 0;
	if (add_effort_lv) {
		pet->set_effort_lv(idx, cur_effort_lv);
		if (cur_effort_lv >= kMaxPetLevel) {
			pet->set_effort_value(idx, 0);
		}
		//特训等级若提升，则额外通知前端
		onlineproto::sc_0x0332_pet_notify_effort_lv_up noti_out;
		if (pet->effort_lv(idx) != old_effort_lv) {
			noti_out.set_create_tm(pet->create_tm());
			noti_out.set_old_effort_lv(old_effort_lv);
			noti_out.set_new_effort_lv(cur_effort_lv);
			noti_out.set_idx(idx + 1);
			send_msg_to_player(player,
					cli_cmd_cs_0x0332_pet_notify_effort_lv_up,
					noti_out);
		}
		if (cur_effort_lv > GET_A(kAttrMaxEffortLv)) {
			SET_A(kAttrMaxEffortLv, cur_effort_lv);
			//成就监听
					//ACH_08_INC_PET_EFFORT
		}
	}
	//特殊处理
	uint32_t tmp_need_val = get_effort_lv_up_need_val(
			pet->pet_id(), pet->effort_lv(idx));
	if (pet->effort_value(idx) == tmp_need_val) {
		pet->set_effort_value(idx, pet->effort_value(idx) - 1);
	}
	ret = PetUtils::save_pet(player, *pet, false, true);	
	if (ret) {
		return ret;
	}

    // 更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_PET_EFFORT, 1);
    return 0;
}

/*
void PetUtils::add_pet_effort(player_t* player, Pet* pet, onlineproto::effort_alloc_data_t* alloc_data)
{
    PetUtils::add_pet_effort(player, pet, kEffortTypeNormalAtk, alloc_data->normal_atk()); 
    PetUtils::add_pet_effort(player, pet, kEffortTypeNormalDef, alloc_data->normal_def()); 
    PetUtils::add_pet_effort(player, pet, kEffortTypeSkillAtk, alloc_data->skill_atk()); 
    PetUtils::add_pet_effort(player, pet, kEffortTypeSkillDef, alloc_data->skill_def()); 
    PetUtils::add_pet_effort(player, pet, kEffortTypeHp, alloc_data->hp()); 
}
*/

uint32_t PetUtils::check_effort_overflow(Pet* pet, 
        onlineproto::effort_alloc_data_t* alloc_data)
{
    if (alloc_data->normal_atk() + pet->effort_value(kEffortTypeNormalAtk) > kMaxSingleEffortLevel) { 
        return cli_err_effort_value_reach_max_limit;
    }
    if (alloc_data->normal_def() + pet->effort_value(kEffortTypeNormalDef) > kMaxSingleEffortLevel) {
        return cli_err_effort_value_reach_max_limit;
    }
    if (alloc_data->skill_atk() + pet->effort_value(kEffortTypeSkillAtk) > kMaxSingleEffortLevel) {
        return cli_err_effort_value_reach_max_limit;
    }
    if (alloc_data->skill_def() + pet->effort_value(kEffortTypeSkillDef) > kMaxSingleEffortLevel) {
        return cli_err_effort_value_reach_max_limit;
    }
    if (alloc_data->hp() + pet->effort_value(kEffortTypeHp) > kMaxSingleEffortLevel) {
        return cli_err_effort_value_reach_max_limit;
    }

    if (alloc_data->normal_atk() + alloc_data->normal_def() + 
        alloc_data->skill_atk() + alloc_data->skill_def() + 
        alloc_data->hp() + pet->get_effort_sum() > kMaxTotalEffortLevel) { 
        return cli_err_total_effort_value_reach_max_limit; 
    }

    return 0;
}

int PetUtils::pet_level_up(player_t* player, Pet *pet, uint32_t add_level, bool wait,
        onlineproto::level_up_reason_t reason)
{
    assert(pet);

    if (pet->level() == kMaxPetLevel) {
        return 0; 
    }

    const pet_conf_t* pet_conf = get_pet_conf(pet->pet_id());
    if (pet_conf == NULL) {
        ERROR_TLOG("not find pet config %u", pet->pet_id());
        return cli_err_pet_id_invalid;
    }

    uint32_t new_level = pet->level() + add_level;
    if (new_level >= kMaxPetLevel) {
        new_level = kMaxPetLevel; 
        pet->set_exp(0); // 达到最大等级后经验归零
        ADD_A(kAttrMaxLvPetCount, 1);
    }

#if 0 //精灵全部主动进化
    // 获取进化后的配置
    while (pet_conf->evolve_to && pet_conf->evolve_lv != 0 && new_level >= pet_conf->evolve_lv) {
        pet_conf = get_pet_conf(pet_conf->evolve_to);

        if (pet_conf == NULL) {
            ERROR_TLOG("pet %u has no evolve pet info ", 
                    pet->pet_id()); 
            return cli_err_sys_err;
        }

        if (pet_conf->evolve_item) {//只能通过物品进化
            ERROR_TLOG("pet %u evolve must use item %u",
                    pet_conf->id, pet_conf->evolve_item); 
            return cli_err_pet_must_evolve_by_item;
        }
    }
#endif

    onlineproto::sc_0x0306_pet_level_up_notify noti_msg;
    noti_msg.Clear();
    noti_msg.set_reason(reason);
    commonproto::pet_info_t *old_pet_info = noti_msg.mutable_old_pet_info();
    DataProtoUtils::pack_player_pet_info(player, pet, old_pet_info);

#if 0
    if (pet->pet_id() != pet_conf->id) {//进化
        if (reason == onlineproto::LEVELUP_FROM_BATTLE) {
            err = PetUtils::pet_evolve(player, 
                    pet, pet_conf->id, new_level, wait, onlineproto::EVOLUTION_FROM_BATTLE);
        } else {
            err = PetUtils::pet_evolve(player, 
                    pet, pet_conf->id, new_level, wait, onlineproto::EVOLUTION_FROM_OTHER);
        }
        if (err) {
            return err;
        }
    } else {//只升级
    }
#endif

    pet->set_level(new_level);
    pet->calc_battle_value(player);
    pet->set_hp(pet->max_hp());
    if (reason == onlineproto::LEVELUP_FROM_BATTLE) {
        PetUtils::save_pet(player, *pet, wait, true, true, onlineproto::POWER_CHANGE_FROM_BATTLE); 
    } else {
        PetUtils::save_pet(player, *pet, wait, true, true, onlineproto::POWER_CHANGE_FROM_LEVELUP); 
    }

    commonproto::pet_info_t *new_pet_info = noti_msg.mutable_new_pet_info();
    DataProtoUtils::pack_player_pet_info(player, pet, new_pet_info);
    noti_msg.set_create_tm(pet->create_tm());

    send_msg_to_player(player, cli_cmd_cs_0x0306_pet_level_up_notify, noti_msg);

    return 0;
}

int PetUtils::pet_evolve(player_t* player, Pet *pet, uint32_t to_pet_id, uint32_t new_level, bool wait, 
        onlineproto::evolution_reason_t reason)
{
    if (!PetUtils::check_can_pet_evlove(player, pet->pet_id())) {
        return cli_err_already_own_pet; 
    }

    const pet_conf_t *pet_conf = get_pet_conf(to_pet_id); 
    if (!pet_conf) {
        return cli_err_pet_id_invalid;
    }

    //同步到前端的数据
    onlineproto::sc_0x0305_pet_evolution sc_pet_evolution;
    sc_pet_evolution.Clear();
    commonproto::pet_info_t *old_pet_info = sc_pet_evolution.mutable_old_pet_info();
    DataProtoUtils::pack_player_pet_info(player, pet, old_pet_info);

    pet->set_pet_id(pet_conf->id);
    if (new_level != 0) {
        pet->set_level(new_level);
    }
    
    // 更新血量
    pet->calc_battle_value(player);
    pet->calc_power(player);
    pet->set_hp(pet->max_hp());
    
    sc_pet_evolution.set_create_tm(pet->create_tm());
    sc_pet_evolution.set_reason(reason);
    commonproto::pet_info_t *new_pet_info = sc_pet_evolution.mutable_new_pet_info();
    DataProtoUtils::pack_player_pet_info(player, pet, new_pet_info);

    int err = 0;
    if (reason == onlineproto::EVOLUTION_FROM_BATTLE) {
        err = PetUtils::save_pet(player, *pet, wait, true, true, onlineproto::POWER_CHANGE_FROM_BATTLE);
    } else {
        err = PetUtils::save_pet(player, *pet, wait, true, true, onlineproto::POWER_CHANGE_FROM_EVOLUTION);
    }

    //进化包晚于保存包发送
    send_msg_to_player(player, cli_cmd_cs_0x0305_pet_evolution, sc_pet_evolution);

    if (err) {
        return err;
    }
    return 0; 
}


int PetUtils::create_pet(player_t *player, uint32_t pet_id, uint32_t level, 
        bool wait_db, uint32_t *create_tm, pet_talent_level_t born_talent)
{
    const pet_conf_t* pet_conf = get_pet_conf(pet_id);
    if (pet_conf == NULL) {
        ERROR_TLOG("pet %u have no config", pet_id); 
        return cli_err_pet_id_invalid;
    }

    // 检查唯一性
    if (!PetUtils::check_can_create_pet(player, pet_id)) {
        ERROR_TLOG("check_can_create_pet %u to failed", pet_id); 
        return cli_err_already_own_pet; 
    }

    Pet pet;

    int ret = 0;
    if (born_talent == kPetTalentLevelNone) {
        born_talent = (pet_talent_level_t)pet_conf->born_talent;
    } 
    init_pet(&pet, pet_id, level, born_talent, NOW());

    //初始化战斗力
    //pet.calc_battle_value(player);
    pet.calc_power(player);
	pet.set_tmp_max_hp(pet.max_hp());
    pet.set_hp(pet.max_hp());
	//初始化怪物危机，远征（伙伴激战）等专用血量
	pet.set_mon_cris_hp(pet.max_hp());
	pet.set_night_raid_hp(pet.max_hp());
	pet.set_exped_cur_hp(pet.max_hp());
	pet.set_mine_attack_hp(pet.max_hp());
	pet.set_first_pos_unlock();
    if (create_tm) {
        *create_tm = pet.create_tm();
    }

    // 是否可以放入背包中
    if (!PetUtils::pets_full(player, PET_LOC_BAG)) {
        pet.set_loc(PET_LOC_BAG); 
    } else if (!PetUtils::pets_full(player, PET_LOC_STORE)) { //放入仓库
        pet.set_loc(PET_LOC_STORE);
    } else {
        return cli_err_pets_full;
    }
 
    ret = PetUtils::add_pet_to_player(player, pet);
    if (ret) {
        ERROR_TLOG("add pet %u to db failed", pet.pet_id()); 
        return cli_err_sys_err;
    }
    ret = PetUtils::save_pet(player, pet, wait_db, true);
    if (ret) {
        ERROR_TLOG("add pet %u to db failed", pet.pet_id()); 
        return cli_err_sys_err;
    }
    SET_A(kAttrPetCount, PetUtils::pets_total_count(player, PET_LOC_UNDEF));
	//相关成就监听
	//获得精灵的数量
	//ACH_05_GET_PET
	

    g_stat_logger->log("精灵获得", pet_conf->name, Utils::to_string(player->userid), "");
    ADD_A(kAttrGoldVipActivity150611Pets, 1);
    return 0;
}

int PetUtils::init_pet(Pet *pet, uint32_t pet_id, uint32_t pet_level,
        pet_talent_level_t talent, uint32_t create_tm)
{
    pet->clear();
    pet->set_pet_id(pet_id);
    pet->set_create_tm(create_tm);
    pet->set_exp(0);
    pet->set_level(pet_level > kMaxPetLevel ?kMaxPetLevel: pet_level);
    pet->set_talent_level(talent);
    pet->set_quality(commonproto::MIN_PET_QUALITY);
    return 0;
}

int PetUtils::add_pet_to_player(player_t* player, Pet& pet)
{
    bool pet_change = false;
    while (player->pets->count(pet.create_tm()) != 0) {
        pet.set_create_tm(pet.create_tm() + 1);
        pet_change = true;
    }

    (*player->pets)[pet.create_tm()] = pet;

    std::map<uint32_t, Pet>::iterator it;
    it = player->pets->find(pet.create_tm());
    assert(it != player->pets->end());
    Pet *p_pet = &(it->second);

    // 加入到pet_id索引的map中
    std::map<uint32_t, std::vector<Pet*> >::iterator pid_it;
    pid_it = player->pet_id_pets->find(p_pet->pet_id());

    if (pid_it == player->pet_id_pets->end()) {
        std::vector<Pet*> pet_list;
        pet_list.push_back(p_pet);
        (*player->pet_id_pets)[p_pet->pet_id()] = pet_list;
    } else {
        std::vector<Pet*>& pet_list = pid_it->second;
        pet_list.push_back(p_pet);
    }

    int ret = 0;
    if (pets_full(player, p_pet->loc())) {
        if (p_pet->loc() == PET_LOC_BAG) {
            // 如果原来在背包中，放入精灵仓库
            ret = PetUtils::set_pet_loc(player, p_pet->create_tm(), PET_LOC_STORE);
            if (ret) return ret;
            pet_change = true;
        } else {
            return -1;
        }
    }

    switch (p_pet->loc()) {
    case PET_LOC_STORE:
        (*player->store_pets)[p_pet->create_tm()] = p_pet;
        break;
    case PET_LOC_BAG:
        (*player->bag_pets)[p_pet->create_tm()] = p_pet;
        break;
    case PET_LOC_ELITE_STORE:
        (*player->elite_pets)[p_pet->create_tm()] = p_pet;
        break;
    case PET_LOC_ROOM:
        (*player->room_pets)[p_pet->create_tm()] = p_pet;
        break;
    default :
        (*player->store_pets)[p_pet->create_tm()] = p_pet;
        break;
    }

    if (p_pet->fight_pos()) {
        if (!PetUtils::is_valid_fight_pos(p_pet->fight_pos())) {
            p_pet->set_fight_pos(0);
            pet_change = true;
        } else {
            uint32_t can_fight_num = PetUtils::can_fight_pets(player);
            if (PetUtils::fight_pet_num(player) < can_fight_num) {
                player->fight_pet[p_pet->fight_pos() - 1] = p_pet;
            } else {
                p_pet->set_fight_pos(0);
                pet_change = true;
            }
        }
    }
    if (pet_change) {
        bool syn_cli = false;
        if (player->is_login) {
            syn_cli = true;
        }
        save_pet(player, *p_pet, false, syn_cli);
    }
    return 0;
}

int PetUtils::improve_pet_talent_level(player_t* player, Pet *pet)
{
    assert(pet);
    if (pet->talent_level() >= kPetTalentLevelFull) {//达到最大
        return cli_err_talent_level_full;
    }
    const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet->pet_id());
    if (!pet_conf) {
        return cli_err_pet_id_invalid;
    }

    if ((uint32_t)pet->talent_level() >= (uint32_t)kPetTalentLevelFull) {
        return 0;
    }

    static uint32_t default_item_cnt[kPetTalentLevelFive] = {
        kOneLevelTalentItemCnt,
        kTwoLevelTalentItemCnt,
        kThreeLevelTalentItemCnt,
        kFourLevelTalentItemCnt,
        kFiveLevelTalentItemCnt
    };
    static uint32_t default_gold_cnt[kPetTalentLevelFive] = {
        kOneLevelTalentGoldCnt,
        kTwoLevelTalentGoldCnt,
        kThreeLevelTalentGoldCnt,
        kFourLevelTalentGoldCnt,
        kFiveLevelTalentGoldCnt
    };

    //判断金币是否足够
    uint32_t need_gold = default_gold_cnt[pet->talent_level()];
    if (!AttrUtils::is_player_gold_enough(player, need_gold)) {
        return cli_err_gold_not_enough;
    }

    uint32_t item_id = pet_conf->talent_item;
    reduce_item_info_t reduce;
    reduce.count = default_item_cnt[pet->talent_level()];
    reduce.item_id = item_id;

    std::vector<reduce_item_info_t> reduce_vec;
    reduce_vec.push_back(reduce);
    int ret = swap_item_by_item_id(player, &reduce_vec, 0, false);
    if (ret) {
        return ret;
    }

    //扣金币
    ret = AttrUtils::sub_player_gold(player, need_gold, pet_conf->name + "升到" + Utils::to_string(pet->talent_level() + 1) + "星"); 
    if (ret) {
        return ret;
    }

    pet->set_talent_level(pet->talent_level() + 1);

    if ((uint32_t)pet->talent_level() >= pet_conf->evolve_talent) {
        // 取最高进化形态
        while((uint32_t)pet->talent_level() >= pet_conf->evolve_talent) {
            const pet_conf_t *p_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_to);
            if (p_conf == NULL) {
                break;
            }
            pet_conf = p_conf;
        }

        int ret = PetUtils::pet_evolve(player, pet, pet_conf->id, pet->level(), false,
                onlineproto::EVOLUTION_FROM_OTHER);
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd, ret);
        }
    }
    save_pet(player, *pet, false, true);
    return 0;
}

int PetUtils::cond_set_talent_level(player_t *player, Pet *pet, uint32_t level)
{
    assert(pet);
    if (level > kPetTalentLevelFull) {
        return cli_err_talent_level_invalid;
    }
    pet->set_talent_level(level);
    save_pet(player, *pet, false, true);
    return 0;
}

uint32_t PetUtils::save_pet(player_t* player, Pet& pet, bool wait, bool is_syn_cli, bool noti_map,
        onlineproto::synpower_reason_t change_reason)
{
    dbproto::cs_pet_save cs_pet_save;
    cs_pet_save.Clear();

    commonproto::pet_info_t* pet_info = cs_pet_save.mutable_pet_info();

    //计算并刷新精灵属性值和战斗力值
    if (player->is_login) { //登录后的用户才重新计算这两个属性值,登录时未完成登录的用户不计算
        pet.set_power(pet.calc_power(player));
        if (PetUtils::is_valid_chisel_pos(pet.chisel_pos())) {
            PlayerUtils::calc_player_battle_value(player);
        }

        //更新精灵战斗力榜（伙伴总战力榜）
        uint32_t pet_total_power = PetUtils::calc_all_pets_total_power(player);
        if (pet_total_power != GET_A(kPetTotalPower) && 
                NOW() - GET_A(kLastUpdatePetPowerTm) > UPDATE_POWER_TM_INTERVL) {
            RankUtils::rank_user_insert_score(
                    player->userid, player->create_tm, 
                    (uint32_t)commonproto::RANKING_SPIRIT_TOTAL_POWER,
                    0, pet_total_power);
            SET_A(kLastUpdatePetPowerTm, NOW());
        }
        std::set<uint32_t> create_tm_set;
        uint32_t n_top_power;
        PetUtils::get_pets_n_topest_power(
                player, create_tm_set, 
                commonproto::PET_N_TOPESET_POWER, 
                n_top_power);
        if (n_top_power != GET_A(kPetNTopestPower) &&
                NOW() - GET_A(kLastUpdateTopestPowerTm) > UPDATE_POWER_TM_INTERVL) {
            RankUtils::rank_user_insert_score(
                    player->userid, player->create_tm, 
                    (uint32_t)commonproto::RANKING_SPIRIT_TOP_5_POWER,
                    0, n_top_power);
            SET_A(kLastUpdateTopestPowerTm, NOW());
        }
        SET_A(kPetTotalPower, pet_total_power);
        SET_A(kPetNTopestPower, n_top_power);
    }

    DataProtoUtils::pack_player_pet_info(player, &pet, pet_info);
    // 更新换装计划记录
    update_change_clothes_plan_record(player, pet_info);

    int ret = 0;
    if (wait) {
        ret = g_dbproxy->send_msg(
				player, player->userid, 
				player->create_tm,
                db_cmd_pet_save, cs_pet_save);
    } else {
        ret = g_dbproxy->send_msg(
				NULL, player->userid, 
				player->create_tm,
                db_cmd_pet_save, cs_pet_save);
    }

    if (ret != 0) {
        return cli_err_sys_err; 
    }
    if (!DupUtils::is_player_in_duplicate(player)) {
        pet.set_tmp_max_hp(pet.max_hp());
    }
    // 同步到客户端
    if (is_syn_cli) {
        onlineproto::sc_0x0301_sync_pet sc_sync_pet;
        commonproto::pet_info_t* pet_info = sc_sync_pet.mutable_pet_info();
        DataProtoUtils::pack_player_pet_info(player, &pet, pet_info, EXPAND_DATA);
		//如果玩家在副本中
		if (player->temp_info.dup_id) {
			const commonproto::lamp_info_list_t& pb_attr = 
				pet_info->pet_optional_attr().lamp();
			uint32_t cur_hp = pet_info->battle_info().cur_hp();
			uint32_t max_hp = pet_info->battle_info().max_hp();
			bool exist_lamp_state = false;
			uint32_t total_cur_hp = 0, total_max_hp = 0;
			for (int i = 0; i < pb_attr.lamp_list_size(); ++i) {
				uint32_t hp_lv = pb_attr.lamp_list(i).hp_lv();
				uint32_t new_cur_hp = ceil(cur_hp * (1 + hp_lv * 3 / 100.0));
				uint32_t new_max_hp = ceil(max_hp * (1 + hp_lv * 3 / 100.0));
				total_cur_hp += new_cur_hp;
				total_max_hp += new_max_hp;
				exist_lamp_state = true;
			}
			if (exist_lamp_state) {
				pet_info->mutable_battle_info()->set_cur_hp(total_cur_hp);
				pet_info->mutable_battle_info()->set_max_hp(total_max_hp);
				TRACE_TLOG("Add Diju Lamp Hp Attr When Save Pet");
			}
		}
		send_msg_to_player(player, cli_cmd_cs_0x0301_sync_pet, sc_sync_pet);
		if (pet.fight_pos() && noti_map) {
            MapUtils::sync_map_player_info(player, commonproto::PLAYER_FOLLOW_PET_CHANGE);
        }
    }

    // 记录最大等级 
    uint32_t max_level = GET_A(kAttrPetMaxLevel);
    if (pet.level() > max_level) {
        SET_A(kAttrPetMaxLevel, pet.level());
    }

    return 0;
}

uint32_t PetUtils::del_pet(player_t* player, Pet* pet)
{
    //从player的各引用点删除
    std::map<uint32_t, std::vector<Pet*> >::iterator ptr = 
        player->pet_id_pets->find(pet->pet_id());
    if (ptr == player->pet_id_pets->end()) {
        return 0; 
    }
    std::vector<Pet*>::iterator pet_ptr = ptr->second.begin();
    for (; pet_ptr != ptr->second.end(); pet_ptr++) {
        Pet* pet1 = *pet_ptr;
        if (pet1->create_tm() == pet->create_tm()) {
            ptr->second.erase(pet_ptr);
            break;
        }
    }
    uint32_t create_tm = pet->create_tm();
    player->bag_pets->erase(create_tm);
    player->store_pets->erase(create_tm);
    player->elite_pets->erase(create_tm);
    player->room_pets->erase(create_tm);

    if (pet->fight_pos()) {
        player->fight_pet[pet->fight_pos() - 1] = NULL;
        pet->set_fight_pos(0);
    }
    if (PetUtils::is_valid_chisel_pos(pet->chisel_pos())) {
        SET_A(((attr_type_t)(kAttrChisel1PetCreateTm + pet->chisel_pos() - 1)), 0);
        pet->set_chisel_pos(0);
    }

    //通知客户端删除
    onlineproto::sc_0x0301_sync_pet sc_sync_pet;
    commonproto::pet_info_t* pet_info = sc_sync_pet.mutable_pet_info();
    DataProtoUtils::pack_player_pet_info(player, pet, pet_info);
    sc_sync_pet.set_is_delete(true);
    send_msg_to_player(player, cli_cmd_cs_0x0301_sync_pet, sc_sync_pet);

    //DB删除
    dbproto::cs_pet_del_info cs_pet_del_info;
    cs_pet_del_info.set_create_tm(create_tm);
	g_dbproxy->send_msg(0, player->userid, 
			player->create_tm,
			db_cmd_pet_delete, cs_pet_del_info);

    //最后真正从内存删除
    player->pets->erase(create_tm);

    return 0;
}

uint32_t PetUtils::get_in_exercise_pets(player_t* player, std::vector<Pet*>& pets_vec)
{
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet && pet->exercise_tm()) {
			pets_vec.push_back(pet);
		}
	}
	return 0;
}

bool PetUtils::check_pet_exercise_over(Pet* pet, const uint32_t exercise_duration)
{
	if (pet->exercise_tm() + exercise_duration > NOW()) {
		return true;
	}
	return false;
}

int PetUtils::pet_group_addition(player_t *player, std::map<uint32_t, uint32_t> &attr_map)
{
    if (player == NULL) {
        return 0;
    }

    uint32_t effect_attrs[11] = {
        kAttrHpMax,
        kAttrNormalAtk,
        kAttrNormalDef,
        kAttrSkillAtk,
        kAttrSkillDef,
        kAttrCrit,
        kAttrAntiCrit,
        kAttrHit,
        kAttrDodge,
        kAttrBreakBlock,
        kAttrBlock,
    };

    FOREACH(g_pet_group_mgr.const_pet_group_conf_map(), it) {
        const pet_group_info_t &pgi = it->second;   
        if (!PetUtils::pet_group_activated(player, it->first)) {
            continue;
        }
        //团队效果被激活
        uint32_t group_cur_btl_val = 0;
        uint32_t group_limit_btl_val = 0;
        FOREACH(pgi.pet_ids, it2) {
            group_cur_btl_val += PetUtils::get_pet_type_cur_max_battle_value(player, *it2);
            group_limit_btl_val += PetUtils::get_pet_type_limit_max_battle_value(player, *it2);
        }

        if (group_limit_btl_val == 0) {
            continue;
        }
        double r = ((double)group_cur_btl_val) / group_limit_btl_val;

        FOREACH(pgi.effects, iter) {
            if (iter->type >= 1 && iter->type <= 11) {
                double add_val = iter->basic_value + r * (iter->max_value - iter->basic_value);
                int32_t value = attr_map[(attr_type_t)effect_attrs[(*iter).type - 1]];
                value += ceil(add_val);
                if (value < 0) {
                    value = 0;
                }
                attr_map[(attr_type_t)effect_attrs[(*iter).type - 1]] = (uint32_t)value;
            }
        }
    }

    return 0;
}

uint32_t PetUtils::calc_all_pets_total_power(player_t* player)
{
	uint32_t total_power = 0;
	FOREACH(*(player->pets), it) {
		Pet& pet = it->second;
		total_power += pet.power();
	}
	return total_power;
}

uint32_t PetUtils::get_exped_pets_by_flag(
		player_t* player, std::vector<Pet*>& pet_vec,
		pet_exped_flag_t flag) 
{
	pet_vec.clear();
	FOREACH(*player->pets, it) {
		Pet& pet = it->second;
		if (pet.exped_flag() == flag) {
			pet_vec.push_back(&pet);
		}
	}
	return 0;
}

uint32_t PetUtils::get_pets_n_topest_power(
		player_t* player, std::set<uint32_t>& create_tm_set, 
		uint32_t need_num, uint32_t& total_power)
{
	std::vector<pet_power_t> pets_vec;
	FOREACH(*player->pets, it) {
		Pet& pet = it->second;
		uint32_t power = pet.power();
		uint32_t create_tm = pet.create_tm();
		pet_power_t tmp;
		tmp.create_tm = create_tm;
		tmp.power = power;
		pets_vec.push_back(tmp);
	}
	std::sort(pets_vec.begin(), pets_vec.end(), compare_function);
	create_tm_set.clear();
	uint32_t count = 1;
	total_power = 0;
	FOREACH(pets_vec, it) {
		if (count > need_num) {
			break;
		}
		create_tm_set.insert(it->create_tm);
		total_power += it->power;
		++count;
	}
	return 0;
}

//废弃
uint32_t PetUtils::get_pets_n_topest_power_from_cache(
		const commonproto::battle_pet_list_t& btl_pet_list,
		std::set<uint32_t>& create_tm_set, uint32_t need_num)
{
	std::vector<pet_power_t> pets_vec;
	uint32_t pet_list_size = btl_pet_list.pet_list_size();
	for (uint32_t i = 0; i < pet_list_size; ++i) {
		uint32_t power = btl_pet_list.pet_list(i).pet_info().base_info().power();
		uint32_t create_tm = btl_pet_list.pet_list(i).pet_info().base_info().create_tm();
		pet_power_t tmp;
		tmp.power = power;
		tmp.create_tm = create_tm;
		pets_vec.push_back(tmp);
	}
	std::sort(pets_vec.begin(), pets_vec.end(), compare_function);
	create_tm_set.clear();
	uint32_t count = 1;
	FOREACH(pets_vec, it) {
		if (count > need_num) {
			break;
		}
		create_tm_set.insert(it->create_tm);
		++count;
	}
	return 0;
}

uint32_t PetUtils::update_pets_expedtion_hp(player_t* player,
	const onlineproto::expedition_pet_cur_hp_list& pet_list)
{
	//先验证精灵的create_tm合法性以及精灵的远征状态
	//将pet_list中精灵的create_tm放入set容器
	std::set<uint32_t> pet_create_tm;
	for (int i = 0; i < pet_list.cur_hp_size(); ++i) {
		uint32_t create_tm = pet_list.cur_hp(i).create_tm();
		Pet* pet = get_pet_in_loc(player, create_tm);
		if (pet == NULL) {
			return cli_err_bag_pet_not_exist;
		}
		if (pet->exped_flag() != EXPED_FIGHT) {
			return cli_err_exped_pet_in_fight;
		}
		//血量只可能比当前血量要少
		//如果策划需求，精灵战斗后会回血，则考虑去掉此判断
		/*
		if (pet->exped_cur_hp() < pet_list.cur_hp(i).exped_cur_hp() / (EXPED_HP_COE * 1.0)) {
			ERROR_TLOG("Hp Sent From Client Err,uid=[%u],cur_hp=[%u],client_hp=[%u]", 
					player->userid, pet->exped_cur_hp(), pet_list.cur_hp(i).exped_cur_hp());
			return cli_err_fight_pets_cur_hp_err;
		}
		*/
		pet_create_tm.insert(create_tm);
	}
	//找出玩家出战的精灵
	std::vector<Pet*> fight_pet;
	PetUtils::get_exped_pets_by_flag(player, fight_pet, EXPED_FIGHT);
	if (fight_pet.size() != pet_create_tm.size()) {
		ERROR_TLOG("Fight Pets Num Not Match:%u, %u", 
				pet_create_tm.size(), fight_pet.size());
		return cli_err_exped_pet_num_err;
	}

	for (int i = 0; i < pet_list.cur_hp_size(); ++i) {
		uint32_t create_tm = pet_list.cur_hp(i).create_tm();
		Pet* pet = get_pet_in_loc(player, create_tm);
		//前面代码已经判断过 pet 为 NULL的情况,这里无需再次判断
		pet->set_exped_cur_hp(pet_list.cur_hp(i).exped_cur_hp() / (EXPED_HP_COE * 1.0));
		if (pet->exped_cur_hp() == 0) {
			pet->set_exped_flag(EXPED_HAS_DIED);
		}
		save_pet(player, *pet, false, true);
	}
	return 0;
}

void PetUtils::deal_pets_after_died_in_exped(player_t* player)
{
	//找出玩家出战的精灵
	std::vector<Pet*> fight_pet;
	PetUtils::get_exped_pets_by_flag(player, fight_pet, EXPED_FIGHT);
	FOREACH(fight_pet, it) {
		Pet* pet = *it;
		pet->set_exped_cur_hp(0);
		pet->set_exped_flag(EXPED_HAS_DIED);
		save_pet(player, *pet, false, true);
	}
}

void PetUtils::deal_pets_after_reset_exped(player_t* player) 
{
	FOREACH(*player->pets, it) {
		Pet& pet = it->second;
		if (pet.exped_flag() != EXPED_NO_JOINED) {
			pet.set_exped_flag(EXPED_NO_JOINED);
			pet.set_exped_cur_hp(0);
			save_pet(player, pet, false, true);
		}
	}
}

void PetUtils::change_pets_exped_flag(
		player_t* player,
		pet_exped_flag_t cur_flag, 
		pet_exped_flag_t des_flag)
{
	std::vector<Pet*> pet_vec;
	if (cur_flag == EXPED_FIGHT && 
			des_flag == EXPED_JOINED) {
		PetUtils::get_exped_pets_by_flag(player, pet_vec, EXPED_FIGHT);
		FOREACH(pet_vec, it) {
			Pet* pet = *it;
			pet->set_exped_flag(des_flag);
			save_pet(player, *pet, false, true);
		}
	}

	//下面是清除掉玩家远征的信息
	if (des_flag == EXPED_NO_JOINED) {
		FOREACH(pet_vec,it) {
			Pet* pet = *it;
			if (pet->exped_flag() != EXPED_NO_JOINED) {
				pet->set_exped_flag(EXPED_NO_JOINED);
				pet->set_exped_cur_hp(0);
				save_pet(player, *pet, false, true);
			}
		}
	}
}

uint32_t PetUtils::pick_pets_joined_exped(player_t* player,
	const std::vector<uint32_t>& create_tm_vec)
{
	std::set<Pet*> joined_pets; 
	FOREACH(create_tm_vec, it) {
		Pet* pet = PetUtils::get_pet_in_loc(player, *it);
		if (pet == NULL) {
			return cli_err_exped_pet_create_tm_err;
		}
		if (pet->exped_flag() == EXPED_FIGHT || 
				pet->exped_flag() == EXPED_HAS_FIGHTED) {
			continue;
		}
		if (pet->exped_flag() == EXPED_HAS_DIED) {
			continue;
		}
		joined_pets.insert(pet);
	}
	//换下一批精灵 EXPED_JOINED 转为 EXPED_NO_JOINED
	std::vector<Pet*> old_pets;
	PetUtils::get_exped_pets_by_flag(player, old_pets, EXPED_JOINED);
	FOREACH(old_pets, it) {
		Pet* pet = *it;
		if (pet == NULL) continue;
		if (joined_pets.count(pet) == 0) {
			pet->set_exped_flag(EXPED_NO_JOINED);
			pet->set_exped_cur_hp(0);
			save_pet(player, *pet, false, true);
		}
	}
	//换上一批精灵 exped_flag 统一设置为EXPED_JOINED
	FOREACH(joined_pets, iter) {
		Pet* pet = *iter;
		pet->set_exped_flag(EXPED_JOINED);
		pet->set_exped_cur_hp(pet->max_hp());
		save_pet(player, *pet, false, true);
	}
	return 0;
}

uint32_t PetUtils::pick_pets_into_exped_fight(player_t* player,
	const std::vector<uint32_t>& create_tm_vec)
{
	std::set<Pet*> fight_pets;
	FOREACH(create_tm_vec, it) {
		Pet* pet = PetUtils::get_pet_in_loc(player, *it);
		if (pet == NULL) {
			return cli_err_exped_pet_create_tm_err;
		}
		if (pet->exped_flag() == EXPED_NO_JOINED) {
			return cli_err_exped_must_joined_before_fight;
		}
		if (pet->exped_flag() == EXPED_HAS_DIED) {
			return cli_err_exped_pets_has_died;
		}
		fight_pets.insert(pet);
	}
	std::vector<Pet*> old_pets;
	PetUtils::get_exped_pets_by_flag(player, old_pets, EXPED_FIGHT);
	FOREACH(old_pets, it) {
		Pet* pet = *it;
		if (pet == NULL) continue;
		if (fight_pets.count(pet) == 0) {
			pet->set_exped_flag(EXPED_HAS_FIGHTED);
		}
		save_pet(player, *pet, false, true);
	}

	FOREACH(fight_pets, iter) {
		Pet* pet = *iter;
		pet->set_exped_flag(EXPED_FIGHT);
		save_pet(player, *pet, false, true);
	}
	return 0;
}

int PetUtils::update_pet_skill(player_t *player)
{
    if (player == NULL) {
        return 0;
    }

    for (uint32_t i = 0; i < commonproto::MAX_SKILL_NUM;i++) {
        uint32_t skill_id = GET_A((attr_type_t)(kAttrSkill1 + i));
        uint32_t parent_skill_id = skill_id / 10;
        update_pet_skill_by_id(player, parent_skill_id);
    }

    return 0;
}

int PetUtils::update_pet_skill_by_id(player_t *player, uint32_t parent_skill_id)
{
    if (parent_skill_id > 0) {
        const skill_conf_t *skill = g_skill_conf_mgr.find_skill_conf(parent_skill_id);
        if (skill == NULL) {
            return 0;
        }

        // 检查是否拥有精灵
        bool has_skill_flag = true;
        uint32_t min_level = 0;
        FOREACH(skill->pet_from, iter) {
            uint32_t pet_id = *iter;
            if (!PetUtils::has_pet_type(player, pet_id)) {
                has_skill_flag = false;
                break;
            }

            // 如果技能需要多种精灵，取玩家同种精灵最高等级，不同类型精灵最低等级
            uint32_t max_talent_level = 
                PetUtils::get_pet_type_max_talent_level(player, pet_id);
            if (min_level == 0) {
                min_level = max_talent_level;
            } else {
                if (min_level > max_talent_level) {
                    min_level = max_talent_level;
                }
            }
        }

        if (has_skill_flag == false) {
            return 0;
        }

        // 安装技能到对应位置
        uint32_t skill_id = parent_skill_id * 10 + min_level;
        SET_A((attr_type_t)(kAttrSkill1 + skill->skill_type - 1), skill_id);
    }

    return 0;
}

uint32_t PetUtils::sync_fight_pet_info_to_client(player_t* player)
{
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet == NULL) {  
			continue;
		}
		if (pet->fight_pos() == 0) {
			continue;
		} 
		onlineproto::sc_0x0301_sync_pet sc_sync_pet;
		commonproto::pet_info_t* pet_info = sc_sync_pet.mutable_pet_info();
		DataProtoUtils::pack_player_pet_info(player, pet, pet_info);
		send_msg_to_player(player, cli_cmd_cs_0x0301_sync_pet, sc_sync_pet);
	}
	return 0;
}

uint32_t PetUtils::get_exercise_pets(
		player_t* player, std::vector<Pet*>& pets_vec, uint32_t time)
{
	pets_vec.clear();
	FOREACH(*player->pets, it) {
		Pet* pet = &it->second;
		if (pet == NULL) {
			continue;
		}
		if (pet->exercise_tm()) {
			if (NOW() > pet->exercise_tm() && 
					NOW() - pet->exercise_tm() > time) {
			pets_vec.push_back(pet);
			}
		}
	}
	return 0;
}

uint32_t PetUtils::sync_pet_exercise_stat_to_hm(player_t* player, Pet* pet)
{
	if (pet == NULL) {
		return cli_err_pet_not_exist;
	}
	onlineproto::sc_0x0333_hm_notify_pet_exercise_state_change  noti_msg;
	DataProtoUtils::pack_player_pet_info(player, pet, noti_msg.mutable_pet_info());
	homeproto::cs_home_pet_exercise_notify hm_in;
	hm_in.set_userid(player->userid);
	hm_in.set_create_tm(player->create_tm);
	hm_in.set_cmd(cli_cmd_cs_0x0333_hm_notify_pet_exercise_state_change);
	std::string pkg;
	noti_msg.SerializeToString(&pkg);
	hm_in.set_pkg(pkg);
	return g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
			home_cmd_notify_pet_exe_state_change, 
			hm_in, player->userid);
}

bool compare_function(const pet_power_t& lhs, const pet_power_t& rhs) {
	return lhs.power > rhs.power;
}

//玩家有没有pet_ids中的所有精灵(包含同一进化链上的) 且都在守护
static bool has_all_pets_chisel(player_t *player, const std::vector<uint32_t> &pet_ids)
{
    FOREACH(pet_ids, it) {
        if (!PetUtils::has_pet_type_chisel(player, *it)) return false;
    }
    return true;
}

//玩家有没有pet_ids中的所有精灵 包含同一进化链上的
static bool has_all_pets(player_t *player, const std::vector<uint32_t> &pet_ids)
{
    FOREACH(pet_ids, it) {
        if (!PetUtils::has_pet_type(player, *it)) return false;
    }
    return true;
}

bool PetUtils::pet_group_activated(player_t *player, uint32_t group_id)
{
    const pet_group_info_t *pgi = g_pet_group_mgr.get_pet_group_conf(group_id);
    if (!pgi) {
        return false;
    }

    switch (pgi->activate_type) {
    case pet_group_active_type_guard:
        return has_all_pets_chisel(player, pgi->pet_ids);
    case pet_group_active_type_own:
        return has_all_pets(player, pgi->pet_ids);
    default:
        return false;        
    }
    return false;
}

/*
uint32_t PetUtils::convert_pet_lamp_state_to_level_map(uint32_t lamp_state,
	std::map<uint32_t, uint32_t>& lamp_level_map)
{
	lamp_level_map.clear();
	std::bitset<32> bit(lamp_state);
	std::bitset<3> tmpbit;
	for (uint32_t i = 0; i < bit.size(); ++i) {
		if (bit.test(i)) {
			int index = i % 3;
			tmpbit.set(index);
		}
		if ((i + 1) % 3 == 0) {
			int j = i / 3 + 1;
			if (tmpbit.to_ulong()) {
				lamp_level_map.insert(make_pair(j, (uint32_t)tmpbit.to_ulong()));
				TRACE_TLOG("Lamp State;lamp_id=%u, level=%u", j, tmpbit.to_ulong());
			}
			tmpbit.reset();
		}
	}
	return 0;
}

uint32_t PetUtils::convert_level_map_to_pet_lamp_state(
		const std::map<uint32_t, uint32_t>& lamp_level_map,
		uint32_t& lamp_state)
{
	std::bitset<32> bit;
	FOREACH(lamp_level_map, it) {
		uint32_t level = it->second;
		std::bitset<3> tmpbit(level);
		uint32_t lamp_id = it->first;
		for (uint32_t i = 0; i < tmpbit.size(); ++i) {
			uint32_t index = 3 * (lamp_id - 1) + i;
			if (tmpbit.test(i)) {
				bit.set(index);
			} else {
				bit.reset(index);
			}
		}
	}
	lamp_state = bit.to_ulong();
	TRACE_TLOG("Convert Level Map To Lamp State:%u", lamp_state);
	return 0;
}

uint32_t PetUtils::convert_lamp_state_to_pet_btl_attr(uint32_t lamp_state,
	commonproto::battle_info_t* pb_btl)
{
	std::map<uint32_t, uint32_t> lamp_level_map;
	PetUtils::convert_pet_lamp_state_to_level_map(lamp_state, lamp_level_map);
	FOREACH(lamp_level_map, it) {
		uint32_t lamp_id = it->first;
		uint32_t level = it->second;
		if (level == 0) { return 0;}

		uint32_t cur_hp = (*pb_btl).cur_hp();
		uint32_t normal_atk = (*pb_btl).normal_atk();
		uint32_t normal_def = (*pb_btl).normal_def();
		uint32_t skill_atk = (*pb_btl).skill_atk();
		uint32_t skill_def = (*pb_btl).skill_def();

		uint32_t new_hp = 0, new_normal_atk = 0, new_normal_def = 0, new_skill_atk = 0, new_skill_def = 0;
		if (lamp_id == 1) {
			new_hp = cur_hp * (1 + (level - 1) * 0.16 + 0.32);	
			pb_btl->set_cur_hp(new_hp);
		} else if (lamp_id == 2) {
			new_normal_atk = normal_atk * (1 + (level - 1) * 0.16 + 0.32);	
			pb_btl->set_normal_atk(new_normal_atk);
		} else if (lamp_id == 3) {
			new_normal_def = normal_def * (1 + (level - 1) * 0.04 + 0.08);
			pb_btl->set_normal_def(new_normal_def);
		} else if (lamp_id == 4) {
			new_skill_atk = skill_atk * (1 + (level - 1) * 0.16 + 0.32);
			pb_btl->set_skill_atk(new_skill_atk);
		} else if (lamp_id == 5) {
			new_skill_def = skill_def * (1 + (level - 1) * 0.04 + 0.08);
			pb_btl->set_skill_def(new_skill_def);
		} else if (lamp_id == 6) {
			new_normal_atk = normal_atk * (1 + (level - 1) * 0.04 + 0.08); 
			new_skill_atk = skill_atk * (1 + (level - 1) * 0.04 + 0.08);
			new_normal_def = normal_def * (1 + (level - 1) * 0.01 + 0.02);
			new_skill_def = skill_def * (1 + (level - 1) * 0.01 + 0.02);
			new_hp = cur_hp * (1 + (level - 1) * 0.04 + 0.08);	
			pb_btl->set_cur_hp(new_hp);
			pb_btl->set_normal_atk(new_normal_atk);
			pb_btl->set_normal_def(new_normal_def);
			pb_btl->set_skill_atk(new_skill_atk);
			pb_btl->set_skill_def(new_skill_def);
		}
		TRACE_TLOG("Lamp State To Pet Btl Attr,lamp_id=%u,lv=%u\n"
				"\t\t\t\told_hp=[%u],nor_atk=[%u],nor_def=[%u],skl_atk=[%u],skl_def=[%u]\n"
				"\t\t\t\tnew_hp=[%u],new_nor_atk=[%u],new_nor_def=[%u],new_skl_atk=[%u],new_skl_def=[%u]",
				lamp_id, level, cur_hp, normal_atk, normal_def, skill_atk, skill_def,
				new_hp, new_normal_atk, new_normal_def, new_skill_atk, new_skill_def);
	}
	return 0;
}
*/

uint32_t  PetUtils::check_lamp_lv_condition(player_t* player, Pet* pet,
	const commonproto::lamp_info_t& lamp_info,
	uint32_t lamp_index, uint32_t& old_lamp_lv)
{
	if (pet == NULL) {
		return 0;
	}
	//灯泡最大等级
	const uint32_t lamp_max_lv = 20;
	if (lamp_index == 1) {
		old_lamp_lv = lamp_info.hp_lv();
	} else if (lamp_index == 2) {
		old_lamp_lv = lamp_info.normal_atk_lv();
	} else if (lamp_index == 3) {
		old_lamp_lv = lamp_info.normal_def_lv();
	} else if (lamp_index == 4) {
		old_lamp_lv = lamp_info.skill_atk_lv();
	} else {
		old_lamp_lv = lamp_info.skill_def_lv();
	}
	if (old_lamp_lv >= lamp_max_lv) {
		return cli_err_lamp_lv_get_limit;
	}
	if (pet->level() < 2 * old_lamp_lv) {
		return cli_err_pet_lv_can_not_lv_up_lamp;
	}
	std::vector<uint32_t> lv_vec;
	uint32_t old_hp_lv = lamp_info.hp_lv();
	lv_vec.push_back(old_hp_lv);
	uint32_t old_nor_atk_lv = lamp_info.normal_atk_lv();
	lv_vec.push_back(old_nor_atk_lv);
	uint32_t old_nor_def_lv = lamp_info.normal_def_lv();
	lv_vec.push_back(old_nor_def_lv);
	uint32_t old_skill_atk_lv = lamp_info.skill_atk_lv();
	lv_vec.push_back(old_skill_atk_lv);
	uint32_t old_skill_def_lv = lamp_info.skill_def_lv();
	lv_vec.push_back(old_skill_def_lv);
	//判断其他灯泡是否都到达要升级的灯泡的一半
	FOREACH(lv_vec, it) {
		if (ceil(old_lamp_lv / 2) > *it) {
			return cli_err_lamp_lv_up_err;
		}
	}
	return 0;
}

uint32_t PetUtils::lamp_lv_up(player_t* player, uint32_t lamp_index,
			commonproto::lamp_info_t& lamp_info)
{
	uint32_t old_lamp_lv = 0;
	if (lamp_index == 1) {
		old_lamp_lv = lamp_info.hp_lv();
		lamp_info.set_hp_lv(old_lamp_lv + 1);
	} else if (lamp_index == 2) {
		old_lamp_lv = lamp_info.normal_atk_lv();
		lamp_info.set_normal_atk_lv(old_lamp_lv + 1);
	} else if (lamp_index == 3) {
		old_lamp_lv = lamp_info.normal_def_lv();
		lamp_info.set_normal_def_lv(old_lamp_lv + 1);
	} else if (lamp_index == 4) {
		old_lamp_lv = lamp_info.skill_atk_lv();
		lamp_info.set_skill_atk_lv(old_lamp_lv + 1);
	} else {
		old_lamp_lv = lamp_info.skill_def_lv();
		lamp_info.set_skill_def_lv(old_lamp_lv + 1);
	}
	return 0;
}

//查看指定的伙伴是否都处在出战位
uint32_t PetUtils::check_pets_is_in_fight_pos(player_t* player,
		const std::vector<uint32_t>& pet_ids,
		std::vector<uint32_t>& not_in_fight_pets)
{
	FOREACH(pet_ids, it) {
		Pet* pet = PetUtils::get_pet_by_id(player, *it);
		if (pet == NULL) {
			return cli_err_pet_id_invalid;
		}
		if (pet->fight_pos()) {
			continue;
		}
		not_in_fight_pets.push_back(*it);
	}
	return 0;
}
