#include "data_proto_utils.h"
#include "player.h"
#include "pet.h"
#include "builder_conf.h"
#include "global_data.h"
#include <boost/lexical_cast.hpp>

int DataProtoUtils::pack_player_base_info(player_t* player,
        commonproto::player_base_info_t* base_info)
{
    base_info->ParsePartialFromString(player->proto_base_info);
    return 0;
}

int DataProtoUtils::unpack_player_base_info(player_t *player, 
        const commonproto::player_base_info_t &base_info)
{
    base_info.SerializeToString(&(player->proto_base_info));
    return 0;
}

int DataProtoUtils::pack_player_battle_info(player_t *player,
        commonproto::battle_info_t *battle_info)
{
    battle_info->ParsePartialFromString(player->proto_battle_info);
    battle_info->set_cur_hp(player->cur_hp);
    return 0;
}

int DataProtoUtils::unpack_player_battle_info(player_t *player,
        const commonproto::battle_info_t &battle_info)
{
    battle_info.SerializeToString(&(player->proto_battle_info));
    return 0;
}

int DataProtoUtils::pack_player_equip_info(player_t *player,
        commonproto::equip_list_t *equip_list)
{
    equip_list->ParsePartialFromString(player->proto_equip_info);
    return 0;
}

int DataProtoUtils::unpack_player_equip_info(player_t *player,
        const commonproto::equip_list_t &equip_list)
{
    equip_list.SerializeToString(&(player->proto_equip_info));
    return 0;
}

int DataProtoUtils::pack_player_chisel_pet_info(player_t *player,
        commonproto::battle_pet_list_t *chisel_pet_list)
{
    chisel_pet_list->ParsePartialFromString(player->proto_chisel_pet_info);
    return 0;
}

int DataProtoUtils::unpack_player_chisel_pet_info(player_t *player,
        const commonproto::battle_pet_list_t &chisel_pet_list)
{
    chisel_pet_list.SerializeToString(&(player->proto_chisel_pet_info));
    return 0;
}

int DataProtoUtils::pack_pet_rune_info(Pet *pet,
        google::protobuf::RepeatedPtrField <commonproto::rune_data_t> *rune_info)
{
    rune_info->Clear();
    commonproto::rune_list_t rune_list;
    rune_list.ParseFromString(pet->proto_rune_info());
    rune_info->CopyFrom(rune_list.rune_data());
    return 0;
}

int DataProtoUtils::unpack_pet_rune_info(Pet *pet,
        const google::protobuf::RepeatedPtrField <commonproto::rune_data_t> &rune_list)
{
    string tmp;
    commonproto::rune_list_t rune_info;
    rune_info.mutable_rune_data()->CopyFrom(rune_list);
    rune_info.SerializeToString(&tmp);
    pet->set_proto_rune_info(tmp);
    return 0;
}

int DataProtoUtils::pack_pet_info(Pet *pet,
        commonproto::pet_info_t *pet_info)
{
    //base_info
    commonproto::pet_base_info_t *base_info = pet_info->mutable_base_info();
    base_info->set_pet_id(pet->pet_id());
	base_info->set_create_tm(pet->create_tm());
	base_info->set_level(pet->level());
	base_info->set_exp(pet->exp());
	base_info->set_fight_pos(pet->fight_pos());
	base_info->set_chisel_pos(pet->chisel_pos());
	base_info->set_loc((commonproto::pet_loc_type_t)pet->loc());
	base_info->set_exercise_pos(pet->exercise_pos());
    base_info->set_power(pet->power());

    //talent
    pet_info->set_talent_level(pet->talent_level());

    //effort
    commonproto::pet_effort_value_t *effort = pet_info->mutable_effort();
    effort->set_hp(pet->effort_value(kEffortTypeHp));
    effort->set_normal_atk(pet->effort_value(kEffortTypeNormalAtk));
    effort->set_normal_def(pet->effort_value(kEffortTypeNormalDef));
    effort->set_skill_atk(pet->effort_value(kEffortTypeSkillAtk));
    effort->set_skill_def(pet->effort_value(kEffortTypeSkillDef));

	commonproto::pet_effort_lv_t *effort_lv = pet_info->mutable_effort_lv();
	effort_lv->set_effort_hp_lv(pet->effort_lv(kEffortTypeHp));
	effort_lv->set_effort_normal_atk_lv(pet->effort_lv(kEffortTypeNormalAtk));
	effort_lv->set_effort_normal_def_lv(pet->effort_lv(kEffortTypeNormalDef));
	effort_lv->set_effort_skill_atk_lv(pet->effort_lv(kEffortTypeSkillAtk));
	effort_lv->set_effort_skill_def_lv(pet->effort_lv(kEffortTypeSkillDef));

    //battle_info
    commonproto::battle_info_t *battle = pet_info->mutable_battle_info();
    battle->set_crit(pet->battle_value_hide(kBattleValueHideTypeCrit));
    battle->set_anti_crit(pet->battle_value_hide(kBattleValueHideTypeAntiCrit));
    battle->set_hit(pet->battle_value_hide(kBattleValueHideTypeHit));
    battle->set_dodge(pet->battle_value_hide(kBattleValueHideTypeDodge));
    battle->set_block(pet->battle_value_hide(kBattleValueHideTypeBlock));
    battle->set_break_block(pet->battle_value_hide(kBattleValueHideTypeBreakBlock));
    battle->set_atk_speed(pet->battle_value_hide(kBattleValueHideTypeAtkSpeed));
    battle->set_crit_affect_rate(pet->battle_value_hide(kBattleValueHideTypeCritAffectRate));
    battle->set_block_affect_rate(pet->battle_value_hide(kBattleValueHideTypeBlockAffectRate));
    battle->set_max_hp(pet->max_hp());
    battle->set_cur_hp(pet->hp());
    battle->set_normal_atk(pet->battle_value(kBattleValueNormalTypeNormalAtk));
    battle->set_normal_def(pet->battle_value(kBattleValueNormalTypeNormalDef));
    battle->set_skill_atk(pet->battle_value(kBattleValueNormalTypeSkillAtk));
    battle->set_skill_def(pet->battle_value(kBattleValueNormalTypeSkillDef));
    battle->set_elem_type((commonproto::equip_elem_type_t)pet->elem_type()); 
    battle->set_req_power(pet->req_power());
    battle->set_max_dp(pet->max_dp());
    //anti
    commonproto::anti_value_t *anti = battle->mutable_anti();
    anti->set_water(pet->anti_value(kAntiTypeWater));
    anti->set_fire(pet->anti_value(kAntiTypeFire));
    anti->set_grass(pet->anti_value(kAntiTypeGrass));
    anti->set_light(pet->anti_value(kAntiTypeLight));
    anti->set_dark(pet->anti_value(kAntiTypeDark));
    anti->set_ground(pet->anti_value(kAntiTypeGround));
    anti->set_force(pet->anti_value(kAntiTypeForce));


    //rune_info
    pack_pet_rune_info(pet, pet_info->mutable_rune_info());

	pet_info->set_rune_specified_pos_flag(pet->rune_3_unlock_flag());
    pet_info->set_quality(pet->quality());
    pet_info->set_exercise_tm(pet->exercise_tm());
    pet_info->set_last_add_exp_tm(pet->last_add_exp_tm());
	pet_info->set_exped_hp(pet->exped_cur_hp());
	pet_info->set_exped_flag((commonproto::pet_exped_flag_t)pet->exped_flag());
	pet_info->set_mon_cris_hp(pet->mon_cris_hp());

	pet_info->set_mine_fight_hp(pet->mine_attack_hp());
	uint64_t mine_flag = pet->pet_mine_flag();
	std::string str_mine_flag = boost::lexical_cast<string>(mine_flag);
	pet_info->set_mine_flag(str_mine_flag);
	uint64_t def_mine_id = pet->defend_mine_id();
    std::string str_def_mine_id = boost::lexical_cast<string>(def_mine_id);
    pet_info->set_defend_mine_id(str_def_mine_id);
    if (pet->pet_opt_attr.size() > 0) { 
        pet_info->mutable_pet_optional_attr()->ParseFromString(pet->pet_opt_attr);
    }
    return 0;
}

int DataProtoUtils::unpack_pet_info(Pet *pet, const commonproto::pet_info_t &pet_info)
{
    pet->clear();
    //base_info
    pet->set_pet_id(pet_info.base_info().pet_id());
    pet->set_create_tm(pet_info.base_info().create_tm());
    pet->set_level(pet_info.base_info().level());
    pet->set_exp(pet_info.base_info().exp());
    pet->set_fight_pos(pet_info.base_info().fight_pos());
	pet->set_exercise_pos(pet_info.base_info().exercise_pos());
    pet->set_chisel_pos(pet_info.base_info().chisel_pos());
    pet->set_loc(pet_info.base_info().loc());
    pet->set_power(pet_info.base_info().power());

    //talent
    pet->set_talent_level(pet_info.talent_level());

    //effort
    pet->set_effort_value(kEffortTypeHp, pet_info.effort().hp());
    pet->set_effort_value(kEffortTypeNormalAtk, pet_info.effort().normal_atk());
    pet->set_effort_value(kEffortTypeNormalDef, pet_info.effort().normal_def());
    pet->set_effort_value(kEffortTypeSkillAtk, pet_info.effort().skill_atk());
    pet->set_effort_value(kEffortTypeSkillDef, pet_info.effort().skill_def());
	pet->set_effort_lv(kEffortTypeHp, pet_info.effort_lv().effort_hp_lv());
	pet->set_effort_lv(kEffortTypeNormalAtk, pet_info.effort_lv().effort_normal_atk_lv());
	pet->set_effort_lv(kEffortTypeNormalDef, pet_info.effort_lv().effort_normal_def_lv());
	pet->set_effort_lv(kEffortTypeSkillAtk, pet_info.effort_lv().effort_skill_atk_lv());
	pet->set_effort_lv(kEffortTypeSkillDef, pet_info.effort_lv().effort_skill_def_lv());

    //battle_info
    pet->set_battle_value_hide(kBattleValueHideTypeCrit, pet_info.battle_info().crit());
    pet->set_battle_value_hide(kBattleValueHideTypeAntiCrit, pet_info.battle_info().anti_crit());
    pet->set_battle_value_hide(kBattleValueHideTypeHit, pet_info.battle_info().hit());
    pet->set_battle_value_hide(kBattleValueHideTypeDodge, pet_info.battle_info().dodge());
    pet->set_battle_value_hide(kBattleValueHideTypeBlock, pet_info.battle_info().block());
    pet->set_battle_value_hide(kBattleValueHideTypeBreakBlock, pet_info.battle_info().break_block());
    pet->set_battle_value_hide(kBattleValueHideTypeCritAffectRate, pet_info.battle_info().crit_affect_rate());
    pet->set_battle_value_hide(kBattleValueHideTypeBlockAffectRate, pet_info.battle_info().block_affect_rate());
    pet->set_battle_value_hide(kBattleValueHideTypeAtkSpeed, pet_info.battle_info().atk_speed());
    pet->set_battle_value(kBattleValueNormalTypeHp, pet_info.battle_info().max_hp());
    pet->set_battle_value(kBattleValueNormalTypeNormalAtk, pet_info.battle_info().normal_atk());
    pet->set_battle_value(kBattleValueNormalTypeNormalDef, pet_info.battle_info().normal_def());
    pet->set_battle_value(kBattleValueNormalTypeSkillAtk, pet_info.battle_info().skill_atk());
    pet->set_battle_value(kBattleValueNormalTypeSkillDef, pet_info.battle_info().skill_def());

    //anti
    pet->set_anti_value(kAntiTypeWater, pet_info.battle_info().anti().water());
    pet->set_anti_value(kAntiTypeFire, pet_info.battle_info().anti().fire());
    pet->set_anti_value(kAntiTypeGrass, pet_info.battle_info().anti().grass());
    pet->set_anti_value(kAntiTypeLight, pet_info.battle_info().anti().light());
    pet->set_anti_value(kAntiTypeDark, pet_info.battle_info().anti().dark());
    pet->set_anti_value(kAntiTypeGround, pet_info.battle_info().anti().ground());
    pet->set_anti_value(kAntiTypeForce, pet_info.battle_info().anti().force());


    pet->set_hp(pet_info.battle_info().cur_hp());
    pet->set_elem_type((uint32_t)pet_info.battle_info().elem_type());
    pet->set_req_power(pet_info.battle_info().req_power());
    pet->set_max_dp(pet_info.battle_info().max_dp());
    unpack_pet_rune_info(pet, pet_info.rune_info());

	pet->set_rune_3_unlock_flag(pet_info.rune_specified_pos_flag());
	pet->set_quality(pet_info.quality());
	pet->set_exercise_tm(pet_info.exercise_tm());
	pet->set_exped_cur_hp(pet_info.exped_hp());
	pet->set_exped_flag(pet_info.exped_flag());
	pet->set_mon_cris_hp(pet_info.mon_cris_hp());

	pet->set_mine_attack_hp(pet_info.mine_fight_hp());
	uint64_t mine_flag = 0;
	try {
		mine_flag = boost::lexical_cast<uint64_t>(pet_info.mine_flag());
	} catch (const boost::bad_lexical_cast &) {
		std::string str_mine_flag = pet_info.mine_flag();
		ERROR_TLOG("Mine Flag Str To Int Err,mine_flag=%s", str_mine_flag.c_str());
	}
	pet->set_mine_flag(mine_flag);
	uint64_t def_mine_id = 0;
	try {
		def_mine_id = boost::lexical_cast<uint64_t>(pet_info.defend_mine_id());
	} catch (const boost::bad_lexical_cast &) {
		std::string str_def_mine_id = pet_info.defend_mine_id();
		ERROR_TLOG("Defend Mine Id Str To Int Err,str_def_mine_id=%s", str_def_mine_id.c_str());
	}
	pet->set_defend_mine_id(def_mine_id);
	pet_info.pet_optional_attr().SerializeToString(&pet->pet_opt_attr);
    return 0;
}

int DataProtoUtils::pack_player_battle_pet_info(player_t *player,
        commonproto::battle_pet_list_t *pet_list)
{
    pet_list->Clear();
    if (player->fight_pets.empty()) {
        return 0;
    }
    FOREACH((player->fight_pets), it) {
        Pet &pet = it->second;
        commonproto::battle_pet_data_t *btl_pet = pet_list->add_pet_list();
        pack_pet_info(&pet, btl_pet->mutable_pet_info());

        btl_pet->set_x_pos(pet.x_pos());
        btl_pet->set_y_pos(pet.y_pos());
        btl_pet->set_heading(pet.heading());    
        btl_pet->set_state_bytes(pet.state_bytes());
    }

    return 0;
}

int DataProtoUtils::unpack_player_battle_pet_info(player_t *player,
        const commonproto::battle_pet_list_t &pet_list)
{
    for (int i = 0;  i < pet_list.pet_list_size(); i++) {
        Pet pet;
        unpack_pet_info(&pet, pet_list.pet_list(i).pet_info());
        pet.set_x_pos(pet_list.pet_list(i).x_pos());
        pet.set_y_pos(pet_list.pet_list(i).y_pos());
        pet.set_heading(pet_list.pet_list(i).heading());
        pet.set_state_bytes(pet_list.pet_list(i).state_bytes());
        player->fight_pets.insert(make_pair(pet.create_tm(), pet));
    }
    return 0;
}

int DataProtoUtils::pack_player_switch_pet_info(player_t *player,
        commonproto::battle_pet_list_t *pet_list)
{
    pet_list->Clear();
    if (player->switch_pets.empty()) {
        return 0;
    }
    FOREACH((player->switch_pets), it) {
        Pet &pet = it->second;
        commonproto::battle_pet_data_t *btl_pet = pet_list->add_pet_list();
        pack_pet_info(&pet, btl_pet->mutable_pet_info());

        btl_pet->set_x_pos(pet.x_pos());
        btl_pet->set_y_pos(pet.y_pos());
        btl_pet->set_heading(pet.heading());    
        btl_pet->set_state_bytes(pet.state_bytes());
    }

    return 0;
}

int DataProtoUtils::unpack_player_switch_pet_info(player_t *player,
        const commonproto::battle_pet_list_t &pet_list)
{
    for (int i = 0;  i < pet_list.pet_list_size(); i++) {
        Pet pet;
        unpack_pet_info(&pet, pet_list.pet_list(i).pet_info());
        pet.set_x_pos(pet_list.pet_list(i).x_pos());
        pet.set_y_pos(pet_list.pet_list(i).y_pos());
        pet.set_heading(pet_list.pet_list(i).heading());
        pet.set_state_bytes(pet_list.pet_list(i).state_bytes());
        player->switch_pets.insert(make_pair(pet.create_tm(), pet));
    }
    return 0;
}

int DataProtoUtils::pack_duplicate_pet_info(duplicate_map_pet_t &dup_pet, 
        commonproto::battle_pet_data_t *btl_pet)
{
    btl_pet->Clear();
    Pet pet;
    pet.init(dup_pet.pet_id, dup_pet.pet_level, dup_pet.create_tm);
    if (dup_pet.is_pet) {
        pet.set_elem_type(get_pet_elem_type(pet.pet_id()));
    } else {
        pet.set_elem_type(get_builder_elem_type(pet.pet_id()));
    }
    pet.calc_battle_value();
    pet.set_hp(dup_pet.cur_hp);
    pet.set_max_hp(dup_pet.max_hp);
    pet.set_req_power(dup_pet.req_power);
    pack_pet_info(&pet, btl_pet->mutable_pet_info());

    btl_pet->set_x_pos(dup_pet.pos_x);
    btl_pet->set_y_pos(dup_pet.pos_y);
    FOREACH(dup_pet.patrol_paths, it) {
        btl_pet->add_patrol_path(*it);
    }

    FOREACH(dup_pet.affix_list, it) {
        btl_pet->add_affix_list(*it);
    }

    btl_pet->set_born_effect(dup_pet.born_effect);
    btl_pet->set_float_height(dup_pet.float_height);
    btl_pet->set_around_type(dup_pet.around_type);
    btl_pet->set_around_create_tm(dup_pet.around_create_tm);
    btl_pet->set_around_radius(dup_pet.around_radius);

    btl_pet->set_born_action(dup_pet.born_action);
    btl_pet->set_born_action_args(dup_pet.born_action_args);
    btl_pet->set_ai_start_delay(dup_pet.ai_start_delay);
    btl_pet->set_heading(dup_pet.heading);
    btl_pet->set_life_time(dup_pet.life_time);
    btl_pet->set_uniq_key(dup_pet.uniq_key);
    btl_pet->set_dynamic_params(dup_pet.dynamic_params);
    btl_pet->set_team(dup_pet.team);
    return 0;
}

int DataProtoUtils::pack_duplicate_map_born_pet_info(std::vector<duplicate_map_pet_t> &pet_vec,
        google::protobuf::RepeatedPtrField<commonproto::battle_pet_data_t> *pet_list)
{
    pet_list->Clear();
    FOREACH(pet_vec, it) {
        //pack_duplicate_pet_info(*it, pet_list->Add());
		pack_duplicate_pet_info_when_born(*it, pet_list->Add());
    }
    return 0;
}

int DataProtoUtils::pack_duplicate_map_monster_info(duplicate_entity_t *entity,
        google::protobuf::RepeatedPtrField<commonproto::battle_pet_data_t> *pet_list)
{
    pet_list->Clear();

    FOREACH((*(entity->cur_map_enemy)), it) {
        duplicate_map_pet_t &dup_pet = it->second;
        if (dup_pet.type == DUP_ACTOR_TYPE_BUILDER) {
            continue;
        }
		init_map_pet_info_dynamic(entity, dup_pet);
        pack_duplicate_pet_info(dup_pet, pet_list->Add());
    }
    FOREACH((*(entity->cur_map_non_enemy)), it) {
        duplicate_map_pet_t &dup_pet = it->second;
        if (dup_pet.type == DUP_ACTOR_TYPE_BUILDER) {
            continue;
        }
        pack_duplicate_pet_info(dup_pet, pet_list->Add());
    }
    return 0;
}

int DataProtoUtils::pack_duplicate_map_builder_info(duplicate_entity_t *entity,
        google::protobuf::RepeatedPtrField<commonproto::battle_pet_data_t> *pet_list)
{
    pet_list->Clear();
    FOREACH((*(entity->cur_map_enemy)), it) {
        duplicate_map_pet_t &dup_pet = it->second;
        if (dup_pet.type != DUP_ACTOR_TYPE_BUILDER) {
            continue;
        }
        pack_duplicate_pet_info(dup_pet, pet_list->Add());
    }
    FOREACH((*(entity->cur_map_non_enemy)), it) {
        duplicate_map_pet_t &dup_pet = it->second;
        if (dup_pet.type != DUP_ACTOR_TYPE_BUILDER) {
            continue;
        }
        pack_duplicate_pet_info(dup_pet, pet_list->Add());
    }
    return 0;
}

int DataProtoUtils::pack_duplicate_player_info(player_t *player, duplicate_entity_t *entity,
        google::protobuf::RepeatedPtrField <commonproto::battle_player_data_t> *players)
{
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(entity->dup_id);
    if (dup->battle_type == DUP_BTL_TYPE_WORLD_BOSS) {
        std::set<player_t *> *p_set = 
            g_dup_entity_mgr->get_line_players(entity, player->side, player->line_id);
        if (p_set) {
            FOREACH(*p_set, iter) {
                commonproto::battle_player_data_t *info = players->Add();
                DataProtoUtils::pack_player_battle_all_info(*iter, info);
            }
        }
    } else {
        FOREACH((*(entity->battle_players)), it) {
            std::set<player_t *> &p_set = it->second;
            FOREACH(p_set, it2) {
                commonproto::battle_player_data_t *info = players->Add();
                DataProtoUtils::pack_player_battle_all_info(*it2, info);
            }
        }
    }
    return 0;
}

int DataProtoUtils::pack_duplicate_all_object(player_t *player, duplicate_entity_t *entity,
        battleproto::sc_battle_duplicate_enter_map &btl_msg)
{
    pack_duplicate_player_info(player, entity, btl_msg.mutable_players());
    pack_duplicate_map_monster_info(entity, btl_msg.mutable_monsters());
    pack_duplicate_map_builder_info(entity, btl_msg.mutable_builders());
    return 0;
}

int DataProtoUtils::pack_player_battle_all_info(player_t *player,
        commonproto::battle_player_data_t *battle_all_info)
{
    battle_all_info->Clear();
    pack_player_base_info(player, battle_all_info->mutable_base_info());
    pack_player_battle_info(player, battle_all_info->mutable_battle_info());
    pack_player_equip_info(player, battle_all_info->mutable_equip_list());
    pack_player_battle_pet_info(player, battle_all_info->mutable_pet_list());
    pack_player_chisel_pet_info(player, battle_all_info->mutable_chisel_pet_list());
    pack_player_switch_pet_info(player, battle_all_info->mutable_switch_pet_list());

    battle_all_info->set_x_pos(player->x_pos);
    battle_all_info->set_y_pos(player->y_pos);
    battle_all_info->set_heading(player->heading);
    battle_all_info->set_state_bytes(player->state_bytes);
	battle_all_info->set_tran_card_id(player->card_id);
	battle_all_info->set_tran_card_level(player->tran_card_level);
    FOREACH(player->skills, iter) {
        battle_all_info->add_skills(*iter);
    }
    battle_all_info->set_team(player->team);
    battle_all_info->set_is_captain(player->is_captain);
    battle_all_info->set_rpvp_score(player->rpvp_score);
    return 0;
}

int DataProtoUtils::unpack_player_battle_all_info(player_t *player,
        const commonproto::battle_player_data_t &battle_all_info)
{
    unpack_player_base_info(player, battle_all_info.base_info());
    unpack_player_battle_info(player, battle_all_info.battle_info());
    unpack_player_equip_info(player, battle_all_info.equip_list());
    unpack_player_battle_pet_info(player, battle_all_info.pet_list());
    unpack_player_chisel_pet_info(player, battle_all_info.chisel_pet_list());
    unpack_player_switch_pet_info(player, battle_all_info.switch_pet_list());

    player->level  = battle_all_info.base_info().level();
    player->create_tm = battle_all_info.base_info().create_tm();
    player->cur_hp = battle_all_info.battle_info().cur_hp();
    player->max_hp = battle_all_info.battle_info().max_hp();
    player->max_tp = battle_all_info.battle_info().tp();
    player->cur_tp = player->max_tp;
    player->x_pos = battle_all_info.x_pos();
    player->y_pos = battle_all_info.y_pos();
    player->heading = battle_all_info.heading();
    player->state_bytes = battle_all_info.state_bytes();
	player->card_id = battle_all_info.tran_card_id();
	player->tran_card_level = battle_all_info.tran_card_level();
    for (int i = 0; i < battle_all_info.skills_size();i++) {
        player->skills.push_back(battle_all_info.skills(i));
    }
    player->family_dup_boss_lv = battle_all_info.family_dup_boss_lv();
    player->family_dup_boss_hp = battle_all_info.family_dup_boss_hp();
    player->family_dup_boss_maxhp = battle_all_info.family_dup_boss_maxhp();
    player->team = battle_all_info.team();
    player->rpvp_score = battle_all_info.rpvp_score();

    DEBUG_TLOG("P:%u Enter team:%u", player->uid, player->team);
    return 0;
}

int DataProtoUtils::pack_duplicate_pet_info_when_born(duplicate_map_pet_t &dup_pet,
	commonproto::battle_pet_data_t *btl_pet)
{
	btl_pet->Clear();
	Pet pet;
	pet.init(dup_pet.pet_id, dup_pet.pet_level, dup_pet.create_tm);
	if (dup_pet.is_pet) {
		pet.set_elem_type(get_pet_elem_type(pet.pet_id()));
	} else {
		pet.set_elem_type(get_builder_elem_type(pet.pet_id()));
	}
	pet.calc_battle_value();
	pet.set_req_power(dup_pet.req_power);
	pack_pet_info(&pet, btl_pet->mutable_pet_info());

	btl_pet->set_x_pos(dup_pet.pos_x);
	btl_pet->set_y_pos(dup_pet.pos_y);
	FOREACH(dup_pet.patrol_paths, it) {
		btl_pet->add_patrol_path(*it);
	}

	FOREACH(dup_pet.affix_list, it) {
		btl_pet->add_affix_list(*it);
	}

	btl_pet->set_born_effect(dup_pet.born_effect);
	btl_pet->set_float_height(dup_pet.float_height);
	btl_pet->set_around_type(dup_pet.around_type);
	btl_pet->set_around_create_tm(dup_pet.around_create_tm);
	btl_pet->set_around_radius(dup_pet.around_radius);

	btl_pet->set_born_action(dup_pet.born_action);
	btl_pet->set_born_action_args(dup_pet.born_action_args);
	btl_pet->set_ai_start_delay(dup_pet.ai_start_delay);
	btl_pet->set_heading(dup_pet.heading);
	btl_pet->set_life_time(dup_pet.life_time);
	btl_pet->set_uniq_key(dup_pet.uniq_key);
	btl_pet->set_dynamic_params(dup_pet.dynamic_params);
	btl_pet->set_team(dup_pet.team);
	return 0;
}
