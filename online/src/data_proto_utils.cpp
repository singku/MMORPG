#include "data_proto_utils.h"
#include "player.h"
#include "item.h"
#include "rune.h"
#include "attr_utils.h"
#include "pet_utils.h"
#include "item_conf.h"
#include "task_info.h"
#include "task_utils.h"
#include "tran_card.h"
#include "player_utils.h"
#include "friend.h"
#include "proto/client/pb0x06.pb.h"
#include "proto/db/dbproto.friend.pb.h"
#include "service.h"
#include "rune_utils.h"
#include "home_data.h"
#include "rank_utils.h"
#include "arena.h"
#include "achieve.h"
#include "title.h"
#include "mine_utils.h"
#include "suit_conf.h"
#include <boost/lexical_cast.hpp>

int DataProtoUtils::pack_player_base_info(player_t* player,
        commonproto::player_base_info_t* base_info)
{
    base_info->set_user_id(player->userid);
    base_info->set_nick(player->nick);
    base_info->set_sex(GET_A(kAttrSex));
    base_info->set_create_tm(GET_A(kAttrCreateTm));
    base_info->set_cur_prof(GET_A(kAttrCurProf));
    base_info->set_level(GET_A(kAttrLv));
    base_info->set_exp(GET_A(kAttrExp));
    base_info->set_vip_begin_time(GET_A(kAttrVipBeginTime));
    base_info->set_vip_end_time(GET_A(kAttrVipEndTime));
    base_info->set_vip_level(GET_A(kAttrVipLevel));
    //base_info->set_is_yearly_vip(is_this_year_vip(player));
    base_info->set_is_yearly_vip(GET_A(kAttrIsYearlyVip));
	base_info->set_power(GET_A(kAttrCurBattleValue));
    bool is_newbe = GET_A(kAttrGuideFinished)? false :true;
    base_info->set_is_newbe(is_newbe);
    base_info->set_family_id(GET_A(kAttrFamilyId));
    base_info->set_family_name(player->family_name);
	base_info->set_gold_vip_end_time(GET_A(kAttrGoldVipEndTime));
	base_info->set_silver_vip_end_time(GET_A(kAttrSilverVipEndTime));
    base_info->set_init_server_id(player->init_server_id);
    base_info->set_server_id(player->server_id);
    base_info->set_mount_show_flag(GET_A(kAttrMountShowFlag));
    base_info->set_wing_show_flag(GET_A(kAttrWingShowFlag));
	if (GET_A(kAttrEquipTitleId)) {
		base_info->set_equip_title_id(GET_A(kAttrEquipTitleId));
	}
    return 0;
}


int DataProtoUtils::pack_player_attrs(player_t* player,
        commonproto::attr_data_list_t* attr_list)
{
    std::map<uint32_t, uint32_t>* attrmap = player->attrs->get_attrs();    
    for(std::map<uint32_t, uint32_t>::const_iterator it=attrmap->begin(); it!= attrmap->end(); ++it) {
		commonproto::attr_data_t* addr_data = attr_list->add_attrs();
		addr_data->set_type(it->first);
		addr_data->set_value(it->second);

    }    

    return 0;
}

int DataProtoUtils::pack_player_battle_base(player_t *player,
        commonproto::battle_info_t *battle_info, bool expand_data)
{
    if (expand_data) {
        battle_info->set_max_hp(player->temp_info.tmp_max_hp);
    } else {
        battle_info->set_max_hp(GET_A(kAttrHpMax));
    }

    battle_info->set_cur_hp(GET_A(kAttrHp));
    battle_info->set_normal_atk(GET_A(kAttrNormalAtk));
    battle_info->set_normal_def(GET_A(kAttrNormalDef));
    battle_info->set_skill_atk(GET_A(kAttrSkillAtk));
    battle_info->set_skill_def(GET_A(kAttrSkillDef));
    battle_info->set_crit(GET_A(kAttrCrit));
    battle_info->set_anti_crit(GET_A(kAttrAntiCrit));
    battle_info->set_hit(GET_A(kAttrHit));
    battle_info->set_dodge(GET_A(kAttrDodge));
    battle_info->set_block(GET_A(kAttrBlock));
    battle_info->set_break_block(GET_A(kAttrBreakBlock));
    battle_info->set_crit_affect_rate(GET_A(kAttrCritDamageRate));
    battle_info->set_block_affect_rate(GET_A(kAttrBlockDamageRate));
    battle_info->set_atk_speed(GET_A(kAttrAtkSpeed));
    battle_info->set_elem_type((commonproto::equip_elem_type_t)GET_A(kAttrPlayerElemType));
    battle_info->set_elem_damage_percent(GET_A(kAttrPlayerElemDamageRate));
    battle_info->set_tp(GET_A(kAttrTp));
    battle_info->set_req_power(GET_A(kAttrCurBattleValue));
    //NOTI(singku)玩家的破罩能量
    battle_info->set_max_dp(0);
	//打包玩家最大能量
    battle_info->set_max_tp(GET_A(kAttrMaxTp));

    //打包玩家的抗性
    battle_info->mutable_anti()->set_water(GET_A(kAttrAntiWater));
    battle_info->mutable_anti()->set_fire(GET_A(kAttrAntiFire));
    battle_info->mutable_anti()->set_grass(GET_A(kAttrAntiGrass));
    battle_info->mutable_anti()->set_light(GET_A(kAttrAntiLight));
    battle_info->mutable_anti()->set_dark(GET_A(kAttrAntiDark));
    battle_info->mutable_anti()->set_ground(GET_A(kAttrAntiGround));
    battle_info->mutable_anti()->set_force(GET_A(kAttrAntiForce));

    return 0;
}

int DataProtoUtils::pack_player_equip_info_by_attr(player_t *player,
        commonproto::item_info_t *item_info,
        uint32_t attr_id)
{
    item_info->Clear();
    uint32_t slot_id = GET_A((attr_type_t)attr_id);
    if (!slot_id) {
        return -1;
    }
    const item_t *item = player->package->get_const_item_in_slot(slot_id);
    if (!item) {
        return -1;
    }
    if (!g_item_conf_mgr.is_equip(item->item_id)) {
        return -1;
    }
    pack_item_info(item, item_info);
    return 0;
}

int DataProtoUtils::pack_player_equip_info_by_attr(player_t *player,
        commonproto::equip_list_t *equip_list,
        uint32_t attr_id)
{
    uint32_t slot_id = GET_A((attr_type_t)attr_id);
    if (!slot_id) {
        return -1;
    }
    const item_t *item = player->package->get_const_item_in_slot(slot_id);
    if (!item) {
        return -1;
    }
    if (!g_item_conf_mgr.is_equip(item->item_id)) {
        return -1;
    }
    pack_item_info(item, equip_list->add_equips());
    return 0;
}

int DataProtoUtils::pack_player_equip_info(player_t *player,
        commonproto::equip_list_t *equip_list)
{
    equip_list->Clear();
    pack_player_equip_info_by_attr(player, equip_list, kAttrHead);
    pack_player_equip_info_by_attr(player, equip_list, kAttrBody);
    pack_player_equip_info_by_attr(player, equip_list, kAttrHand);
    pack_player_equip_info_by_attr(player, equip_list, kAttrLeg);
    pack_player_equip_info_by_attr(player, equip_list, kAttrFoot);
    pack_player_equip_info_by_attr(player, equip_list, kAttrOtherEquipAttr);

    if (GET_A(kAttrHideFashion) == 0) {
        pack_player_equip_info_by_attr(player, equip_list, kAttrHeadDecorate);
        pack_player_equip_info_by_attr(player, equip_list, kAttrFaceDecorate);
        pack_player_equip_info_by_attr(player, equip_list, kAttrClothesDecorate);
        pack_player_equip_info_by_attr(player, equip_list, kAttrWeaponDecorate);
        pack_player_equip_info_by_attr(player, equip_list, kAttrCoatDecorate);
        pack_player_equip_info_by_attr(player, equip_list, kAttrWingDecorate);
    }

    pack_player_equip_info_by_attr(player, equip_list, kAttrOtherEquip2);
    pack_player_equip_info_by_attr(player, equip_list, kAttrOtherEquip3);
    return 0;
}

int DataProtoUtils::pack_player_battle_pet_info(player_t *player,
        commonproto::battle_pet_list_t *pet_list, bool expand_data)
{
    pet_list->Clear();
    if (!player_has_fight_pet(player)) {
        return 0;
    }

    int angle = rand() % 360;
    int pet_x;
    int pet_y;
    for (int i = 0; i < MAX_FIGHT_POS; i++) {
        if (player->fight_pet[i] == 0) {
            continue;
        }
        commonproto::battle_pet_data_t *btl_pet = pet_list->add_pet_list();
        pack_player_pet_info(player, player->fight_pet[i], btl_pet->mutable_pet_info(), expand_data);

#if 1   //NOTI(singku) 前端要求随机摆放角色的伙伴位置
        Utils::get_x_y(player->map_x, player->map_y, 65, angle+i*30, pet_x, pet_y);
        if (pet_x < 0) {
            pet_x += 10;
        }
        if (pet_y < 0) {
            pet_y += 10;
        }
        btl_pet->set_x_pos(pet_x);
        btl_pet->set_y_pos(pet_y);
#endif
    }
    return 0;
}

int DataProtoUtils::pack_player_battle_exped_joined_pet_info(player_t* player,
		commonproto::battle_pet_list_t *pet_list)
{
	pet_list->Clear();
	FOREACH(*player->pets, it) {
		Pet& pet = it->second;
		if (pet.exped_flag() == EXPED_NO_JOINED) {
			continue;
		}
		commonproto::battle_pet_data_t* btl_pet = pet_list->add_pet_list();
		pack_player_pet_info(player, &pet, btl_pet->mutable_pet_info());
	}
	return 0;
}

int DataProtoUtils::pack_player_battle_exped_fight_pet_info(player_t* player,
		commonproto::battle_pet_list_t *pet_list)
{
	pet_list->Clear();
	FOREACH(*player->pets, it) {
		Pet& pet = it->second;
		if (pet.exped_flag() != EXPED_FIGHT) {
			continue;
		}
		commonproto::battle_pet_data_t* btl_pet = pet_list->add_pet_list();
		pack_player_pet_info(player, &pet, btl_pet->mutable_pet_info());
	}
	return 0;
}

int DataProtoUtils::pack_player_battle_exped_n_topest_pet_info(player_t* player,
        commonproto::battle_pet_list_t *pet_list)
{
    const uint32_t NEED_NUM = 5; 
    pet_list->Clear();
    std::set<uint32_t> create_tm_set;
    uint32_t total_power = 0; 
    PetUtils::get_pets_n_topest_power(player, create_tm_set, NEED_NUM, total_power);
    FOREACH(*player->pets, it) {
        Pet& pet = it->second;
        uint32_t create_tm = pet.create_tm();
        if (create_tm_set.count(create_tm) == 0) { 
            continue;
        }    
        commonproto::battle_pet_data_t* btl_pet = pet_list->add_pet_list();
        pack_player_pet_info(player, &pet, btl_pet->mutable_pet_info());
    }    
    return 0;
}

//废弃
int DataProtoUtils::pack_player_battle_exped_n_topest_pet_info(player_t* player,
		onlineproto::exped_pet_info_list* pet_list)
{
	const uint32_t NEED_NUM = 5;
	pet_list->Clear();
	std::set<uint32_t> create_tm_set;
	uint32_t total_power = 0;
	PetUtils::get_pets_n_topest_power(player, create_tm_set, NEED_NUM, total_power);
	FOREACH(*player->pets, it) {
		Pet& pet = it->second;
		uint32_t create_tm = pet.create_tm();
		if (create_tm_set.count(create_tm) == 0) {
			continue;
		}
		DataProtoUtils::pack_exped_unit_pet_info(pet, pet_list->add_exped_pets());
	}
	return 0;
}

int DataProtoUtils::pack_exped_unit_pet_info(
		Pet& pet, 
		onlineproto::exped_pet_info* pb_pet_ptr) 
{
	pb_pet_ptr->set_pet_id(pet.pet_id());
	pb_pet_ptr->set_level(pet.level());
	pb_pet_ptr->set_talent_level(pet.talent_level());
	pb_pet_ptr->set_exped_hp(pet.exped_cur_hp());
	pb_pet_ptr->set_max_hp(pet.max_hp());
	return 0;
}

int DataProtoUtils::pack_player_chisel_pet_info(player_t *player,
        commonproto::battle_pet_list_t *pet_list)
{
    pet_list->Clear();
    FOREACH((*(player->pets)), it) {
        Pet &pet = it->second;
        if (!PetUtils::is_valid_chisel_pos(pet.chisel_pos())) {
            continue;
        }
        commonproto::battle_pet_data_t *btl_pet = pet_list->add_pet_list();
        pack_player_pet_info(player, &pet, btl_pet->mutable_pet_info());
    }
    return 0;
}

int DataProtoUtils::pack_player_non_fight_bag_pet_info(player_t *player,
        commonproto::battle_pet_list_t *pet_list, bool expand_data)
{
    pet_list->Clear();

    FOREACH((*(player->bag_pets)), it) {
        Pet *pet = it->second;
        if (pet->fight_pos()) {
            continue;
        }
        commonproto::battle_pet_data_t *btl_pet = pet_list->add_pet_list();
        pack_player_pet_info(player, pet, btl_pet->mutable_pet_info(), expand_data);
    }
    return 0;
}

int DataProtoUtils::pack_player_item_info(player_t *player,
        commonproto::item_list_t *item_list)
{
    Package* package = player->package;
    std::vector<item_t> result;
    if(package) {
        package->get_all_items(result);
    }

    FOREACH(result, it) {	  
        commonproto::item_info_t* item_info =  item_list->add_item_list();
        pack_item_info(&(*it), item_info);
    }

    return 0;
}

int DataProtoUtils::unpack_player_item_info(player_t *player,
        commonproto::item_list_t *item_list)
{
    bool item_change = false;
    std::vector<item_t> change_vec;
    Package* package = player->package;
    if(package) {
        item_t item; 
        for(int i=0; i<item_list->item_list_size(); i++) {
            unpack_item_info(item_list->item_list(i), &item, item_change);

            if (g_item_conf_mgr.is_equip(item.item_id)) {
                commonproto::item_optional_attr_t opt_attr;
                opt_attr.ParseFromString(item.opt_attr);
                if (opt_attr.equip_power() == 0) {
                    item_change = true;
                }
            }

            if (item_change) {
                init_equip_attr(&item);
                change_vec.push_back(item);
                item_change = false;
            }
            if (g_item_conf_mgr.is_equip(item.item_id) && item.using_count) {
                commonproto::item_optional_attr_t opt_attr;
                opt_attr.ParseFromString(item.opt_attr);
                attr_type_t attr = AttrUtils::get_player_attr_by_equip_pos((equip_body_pos_t)opt_attr.part());
                uint32_t slot_id = GET_A(attr);
                if (slot_id != item.slot_id) {
                    SET_A(attr, item.slot_id);
                }
            }
            package->put_item(&item);
        }
    }
    if (change_vec.size()) {
        db_item_change(player, change_vec, onlineproto::SYNC_REASON_NONE, false);
    }

    return 0;
} 

int DataProtoUtils::pack_pet_info(Pet *pet,
        commonproto::pet_info_t *pet_info) 
{
    return pack_player_pet_info(0, pet, pet_info);
}

int DataProtoUtils::pack_player_pet_info(player_t *player, Pet *pet,
        commonproto::pet_info_t *pet_info, bool expand_data)
{
    //base_info
	DataProtoUtils::pack_pet_base_info(pet, pet_info->mutable_base_info());

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
    if (expand_data) {
        battle->set_max_hp(pet->tmp_max_hp());
    } else {
        battle->set_max_hp(pet->max_hp());
    }
    battle->set_cur_hp(pet->hp());
    battle->set_normal_atk(pet->battle_value(kBattleValueNormalTypeNormalAtk));
    battle->set_normal_def(pet->battle_value(kBattleValueNormalTypeNormalDef));
    battle->set_skill_atk(pet->battle_value(kBattleValueNormalTypeSkillAtk));
    battle->set_skill_def(pet->battle_value(kBattleValueNormalTypeSkillDef));
    battle->set_elem_type((commonproto::equip_elem_type_t)PetUtils::get_pet_elem_type(pet));
    battle->set_elem_damage_percent(50);
    battle->set_req_power(GET_A(kAttrCurBattleValue));
    //NOTI(singku)精灵的破罩能量
    battle->set_max_dp(0);

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
    if (player) {
        std::vector<uint32_t> tem_rune_info;
        pet->get_rune_info_equiped_in_pet(tem_rune_info);
        uint32_t rune_num = tem_rune_info.size();
        for (uint32_t i = 0; i < rune_num; ++i) {
            rune_t rune;
            if (RuneUtils::get_rune(player, tem_rune_info[i], rune)) {
                continue;
            }
            commonproto::rune_data_t *inf = pet_info->add_rune_info();
            inf->set_runeid(rune.id);
            inf->set_index(rune.conf_id);
            inf->set_exp(rune.exp);
            inf->set_level(rune.level);
            inf->set_pack_type(rune.pack_type);
            inf->set_pet_catch_time(rune.pet_catch_time);
            inf->set_grid_id(i+1);
        }
    }
	pet_info->set_rune_specified_pos_flag(pet->rune_specified_pos_flag());
    pet_info->set_quality(pet->quality());
    pet_info->set_exercise_tm(pet->exercise_tm());
    pet_info->set_last_add_exp_tm(pet->last_add_exp_tm());
	pet_info->set_exped_hp(pet->exped_cur_hp());
	pet_info->set_exped_flag((commonproto::pet_exped_flag_t)pet->exped_flag());
	pet_info->set_mon_cris_hp(pet->mon_cris_hp());
	pet_info->set_mon_night_raid_hp(pet->night_raid_hp());
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

int DataProtoUtils::pack_player_pet_list(player_t *player, commonproto::pet_list_t *pet_list,
        pet_location_type_t loc, bool expand_data)
{
    if (loc == PET_LOC_UNDEF) {//打包所有精灵
        FOREACH((*player->pets), it) {
            Pet *pet = &(it->second);
            pack_player_pet_info(player, pet, pet_list->add_pet_list(), expand_data);
        }
        return 0;
    }

    std::map<uint32_t, Pet*> *target_map;
    switch (loc) {
    case PET_LOC_BAG:
        target_map = player->bag_pets;
        break;
    case PET_LOC_STORE:
        target_map = player->store_pets;
        break;
    case PET_LOC_ELITE_STORE:
        target_map = player->elite_pets;
        break;
    case PET_LOC_ROOM:
        target_map = player->room_pets;
        break;
    default:
        return 0;
    }

    FOREACH((*target_map), it) {
        Pet *pet = it->second;
        pack_player_pet_info(player, pet, pet_list->add_pet_list(), expand_data);
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
    //pet->set_is_excercise(pet_info.base_info().is_excercise());
	pet->set_exercise_pos(pet_info.base_info().exercise_pos());
    pet->set_chisel_pos(pet_info.base_info().chisel_pos());
    pet->set_loc((pet_location_type_t)pet_info.base_info().loc());
    pet->set_power(pet_info.base_info().power());

    //talent
    pet->set_talent_level(pet_info.talent_level());

    //effort
    pet->set_effort_value(kEffortTypeHp, pet_info.effort().hp());
    pet->set_effort_value(kEffortTypeNormalAtk, pet_info.effort().normal_atk());
    pet->set_effort_value(kEffortTypeNormalDef, pet_info.effort().normal_def());
    pet->set_effort_value(kEffortTypeSkillAtk, pet_info.effort().skill_atk());
    pet->set_effort_value(kEffortTypeSkillDef, pet_info.effort().skill_def());

	//effort_lv
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

    pet->set_tmp_max_hp(pet->max_hp());
    pet->set_hp(pet->tmp_max_hp());

    //rune_info
	uint32_t rune_info_size = pet_info.rune_info_size();
	for (uint32_t i = 0; i < rune_info_size && i < kMaxEquipRunesNum; i++) {
		uint32_t rune_id = pet_info.rune_info(i).runeid();
		pet->set_rune_array(pet_info.rune_info(i).grid_id() - 1, rune_id);
	}
	pet->set_rune_sp_pos_flag(pet_info.rune_specified_pos_flag());
	pet->set_first_pos_unlock();
	pet->set_quality(pet_info.quality());
	pet->set_exercise_tm(pet_info.exercise_tm());
	//TODO kevin 远征
	pet->set_exped_cur_hp(pet_info.exped_hp());
	pet->set_exped_flag((pet_exped_flag_t)pet_info.exped_flag());
	pet->set_mon_cris_hp(pet_info.mon_cris_hp());

    uint32_t night_raid_max_hp = PlayerUtils::obj_hp_add_common(pet->max_hp(), pet->power());
    pet->set_tmp_max_hp(night_raid_max_hp);
    pet->set_night_raid_hp(pet_info.mon_night_raid_hp());
	pet->set_mine_attack_hp(pet_info.mine_fight_hp());
	uint64_t mine_flag = 0;
	try {
		mine_flag = boost::lexical_cast<uint64_t>(pet_info.mine_flag());
	} catch (const boost::bad_lexical_cast &) {
		ERROR_TLOG("Mine Flag Str To Int Err");
	}
	pet->set_mine_flag(mine_flag);
	uint64_t def_mine_id = 0;
	try {
		def_mine_id = boost::lexical_cast<uint64_t>(pet_info.defend_mine_id());
	} catch (const boost::bad_lexical_cast &) {
		ERROR_TLOG("Defend Mine Id Str To Int Err");
	}
	pet->set_defend_mine_id(def_mine_id);
	////////////////TRACE_TLOG///////////
	const commonproto::pet_optional_attr_t& pb_inf = pet_info.pet_optional_attr();
	std::string debug_str = pb_inf.Utf8DebugString();
	std::string name = pb_inf.GetTypeName();
	TRACE_TLOG("Pet_id=%u,Pet_ctm=%u", 
			pet_info.base_info().pet_id(),
			pet_info.base_info().create_tm());
	TRACE_TLOG("Unpack Pet Info,pet_opt_attr:'%s'\nmsg:[%s]\n",
			name.c_str(), debug_str.c_str());

	pet_info.pet_optional_attr().SerializeToString(&pet->pet_opt_attr);
    return 0;
}

int DataProtoUtils::unpack_player_pet_info(player_t *player, 
        const commonproto::pet_list_t &pet_list)
{
    // 放置精灵
    int pet_info_size = pet_list.pet_list_size();
    Pet pet;

    for (int i = 0; i < pet_info_size; i++) {
        const commonproto::pet_info_t& pet_info = pet_list.pet_list(i);
        const pet_conf_t *pet_conf = PetUtils::get_pet_conf(pet_info.base_info().pet_id());
        if (pet_conf == NULL) {
            continue; 
        }
        pet.clear();
        DataProtoUtils::unpack_pet_info(&pet, pet_info);
        // 整理符文异常数据
        // FixData::deal_pet_dirty_rune_data(player, pet);
        PetUtils::add_pet_to_player(player, pet);
    }
    if (player->bag_pets->empty() && !player->pets->empty()) {
        Pet &pet = player->pets->begin()->second;
        PetUtils::set_pet_loc(player, pet.create_tm(), PET_LOC_BAG);
    }
    /*
    if (!player_has_fight_pet(player) && !player->bag_pets->empty()) {
        Pet *pet = player->bag_pets->begin()->second;
        pet->set_fight_pos(1);
        PetUtils::save_pet(player, *pet, false, false);
        player->fight_pet[pet->fight_pos()-1] = pet;
    }
    */
    return 0;
}

int DataProtoUtils::pack_player_attr_info(player_t *player,
        commonproto::attr_data_list_t *attr_list)
{
    //TODO(singku)
    return 0;
}

int DataProtoUtils::pack_player_battle_all_info(player_t *player,
        commonproto::battle_player_data_t *battle_all_info, bool expand_data)
{
    battle_all_info->Clear();
    pack_player_base_info(player, battle_all_info->mutable_base_info());
    pack_player_battle_base(player, battle_all_info->mutable_battle_info(), expand_data);
    pack_player_equip_info(player, battle_all_info->mutable_equip_list());
    pack_player_battle_pet_info(player, battle_all_info->mutable_pet_list(), expand_data);
    pack_player_chisel_pet_info(player, battle_all_info->mutable_chisel_pet_list());
	pack_player_battle_exped_n_topest_pet_info(
			player, battle_all_info->mutable_power_pet_list());
    battle_all_info->set_x_pos(player->map_x);
    battle_all_info->set_y_pos(player->map_y);
    battle_all_info->set_heading(player->heading);
    battle_all_info->set_state_bytes(*(player->temp_info.state_buf));
	uint32_t card_id = GET_A(kAttrTransCardChooseFlag);
	battle_all_info->set_tran_card_id(card_id);
	tran_card_t tran_card_info;	
	player->m_tran_card->get_tranCard(card_id, tran_card_info);	
	battle_all_info->set_tran_card_level(tran_card_info.card_star_level);

    for (uint32_t i = 0; i < commonproto::MAX_SKILL_NUM;i++) {
        battle_all_info->add_skills(GET_A((attr_type_t)(kAttrSkill1 + i)));
    }
    battle_all_info->set_family_dup_boss_lv(player->temp_info.family_dup_boss_lv);
    battle_all_info->set_family_dup_boss_hp(player->temp_info.family_dup_boss_hp);
    battle_all_info->set_family_dup_boss_maxhp(player->temp_info.family_dup_boss_maxhp);
    
    battle_all_info->set_team(player->temp_info.team);
    battle_all_info->set_rpvp_score(GET_A(kAttrRpvpScore));
	//套装buff
	pack_player_suit_buff_info(player, battle_all_info->mutable_suit_buff_list());
    
    return 0;
}

int DataProtoUtils::pack_pet_base_info(Pet *pet, 
		commonproto::pet_base_info_t* pet_base_info) {
	pet_base_info->set_pet_id(pet->pet_id());
	pet_base_info->set_create_tm(pet->create_tm());
	pet_base_info->set_level(pet->level());
	pet_base_info->set_exp(pet->exp());
	pet_base_info->set_fight_pos(pet->fight_pos());
	pet_base_info->set_chisel_pos(pet->chisel_pos());
	pet_base_info->set_loc((commonproto::pet_loc_type_t)pet->loc());
	//pet_base_info->set_is_excercise(pet->is_excercise());
	pet_base_info->set_exercise_pos(pet->exercise_pos());
    pet_base_info->set_power(pet->power());

	return 0;
}

int DataProtoUtils::pack_map_pet_info(Pet *pet,
        commonproto::map_pet_info_t *pet_info)
{
	pack_pet_base_info(pet, pet_info->mutable_base_info());
    pet_info->set_state_bytes(pet->state());
    return 0;
}

int DataProtoUtils::pack_player_suit_buff_info(player_t *player,
        commonproto::suit_buff_list_t *suit_buf_list)
{
	const std::map<uint32_t, uint32_t>& tmp_map = *(player->suit_buff_map); 
	if(tmp_map.size() == 0){return 0;}

	FOREACH(tmp_map, it){
		uint32_t suit_id = it->first;
		uint32_t buff_id= it->second;
		uint32_t idx = 0;
		const std::map<uint32_t, uint32_t>& trigger_map = 
			g_suit_conf_mgr.find_suit_conf(suit_id)->trigger_buff_map;

		FOREACH(trigger_map, it){
			idx ++;
			if(buff_id == it->second){
				break;
			}
		}
		if(idx == 0){
			ERROR_TLOG("userid : %d suit buff pack err, suit id: %d ,buff id: %d", player->userid, suit_id, buff_id);		
		}
		commonproto::suit_buff_info_t *inf = suit_buf_list->add_suit_buff_info();
		inf->set_suit_id(suit_id);
		inf->set_trigger_num(idx);
	}
	return 0;
}
int DataProtoUtils::pack_map_player_info(player_t *player,
        commonproto::map_player_data_t *map_player_info)
{
    pack_player_base_info(player, map_player_info->mutable_base_info());
	pack_player_equip_info(player, map_player_info->mutable_equip_list());
	pack_player_map_pet_info(player, map_player_info->mutable_pet_list());
	pack_player_suit_buff_info(player, map_player_info->mutable_suit_buff_list());

    map_player_info->set_state_bytes(*(player->temp_info.state_buf));
	//拉取泳装信息
	uint32_t swim_suit = 0;
	if(GET_A(kDailySwimHasLuxurySuit)){
		swim_suit = 2;
	} else if (GET_A(kDailySwimHasSuit)){
		swim_suit = 1;
	}
    map_player_info->set_swim_suit(swim_suit);
    return 0;
}

int DataProtoUtils::unpack_item_info(const commonproto::item_info_t& item_info, item_t* item, bool &item_change)
{
    item_change = false;
    item->item_id = item_info.item_id();
    item->count = item_info.count();
    item->expire_time = item_info.expire_time();
    item->slot_id = item_info.slot_id();
    item->using_count = item_info.using_count();


    if (item->using_count > item->count) {
        ERROR_TLOG("unpack invalid item info[item_id:%d, pos:%d, using_cnt:%u, cnt:%u]",
                item->item_id, item->slot_id, item->using_count, item->count);
        item->using_count = item->count;
    }

    commonproto::item_optional_attr_t opt_attr;
    //如果是装备 则需要检查装备数值的正确性
    if (g_item_conf_mgr.is_equip(item->item_id)) {
        const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(item->item_id);
        opt_attr.CopyFrom(item_info.item_optional_attr());
        uint32_t quality = opt_attr.level();
        if (quality != (uint32_t)item_conf->base_quality || !g_item_conf_mgr.is_valid_equip_quality(quality)) {
            quality = (uint32_t)item_conf->base_quality;
            opt_attr.set_level(quality);
            item_change = true;
        }
        if ((uint32_t)opt_attr.part() != (uint32_t)item_conf->equip_body_pos) {
            ERROR_TLOG("unpack invalid item info[item_id:%u part:%u should_part:%u]",
                    item->item_id, (uint32_t)opt_attr.part(), (uint32_t)item_conf->equip_body_pos);
            opt_attr.set_part((commonproto::equip_body_pos_t)item_conf->equip_body_pos);
            item_change = true;
        }

        if (opt_attr.has_equip_attrs()) {
            commonproto::attr_data_list_t* equip_attrs = opt_attr.mutable_equip_attrs();
            commonproto::attr_data_t *single_attr = equip_attrs->attrs_size() ?equip_attrs->mutable_attrs(0) 
                :equip_attrs->add_attrs();
            if (single_attr->type() != (uint32_t)item_conf->add_attr_type) {
                single_attr->set_type((uint32_t)item_conf->add_attr_type);
                item_change = true;
            }
            if ((uint32_t)opt_attr.elem_type() != (uint32_t)(item_conf->elem_type)) {
                opt_attr.set_elem_type((commonproto::equip_elem_type_t)item_conf->elem_type);
                item_change = true;
            }
        }
        opt_attr.SerializeToString(&(item->opt_attr));
    } else {
        item->opt_attr.clear();
    }

    return 0;
}

int DataProtoUtils::pack_item_info(
        const item_t* item, commonproto::item_info_t* item_info)
{
    item_info->set_item_id(item->item_id);
    item_info->set_count(item->count);
    item_info->set_expire_time(item->expire_time);
    item_info->set_slot_id(item->slot_id);
    item_info->set_using_count(item->using_count);

	if(item->opt_attr.size() > 0) {
		item_info->mutable_item_optional_attr()->ParseFromString(item->opt_attr);
	}
	
    return 0;
}

int DataProtoUtils::unpack_base_info(player_t* player,
        const commonproto::player_base_info_t& base_info) 
{
    player->userid = base_info.user_id();
    strncpy(player->nick, base_info.nick().c_str(), sizeof(player->nick) );
    player->server_id = base_info.server_id();
    player->init_server_id = base_info.init_server_id();
    return 0;
}

int DataProtoUtils::unpack_player_attrs(player_t* player, const commonproto::attr_data_list_t& attr_list) 
{
    Attr* attrs = player->attrs;
    if(attrs) {					   
        for(int i = 0; i < attr_list.attrs_size(); i++) {
            attrs->put_attr(attr_list.attrs(i).type(), attr_list.attrs(i).value());
        }
    }
    player->temp_info.tmp_max_hp = GET_A(kAttrHpMax);
	//初始化最大能量100满
	if(GET_A(kAttrMaxTp) != 100){
		SET_A(kAttrMaxTp, 100);
	}
    return 0;
}

int DataProtoUtils::pack_task_data(const task_t* task, commonproto::task_data_t* task_info) {
	task_info->set_task_id(task->task_id);
	task_info->set_status(task->status);
	task_info->set_done_times(task->done_times);
	task_info->set_bonus_status(task->bonus_status);
	task_info->set_fin_statue_tm(task->fin_statue_tm);

	task_conf_t *task_conf = g_task_conf_mgr.find_task_conf(task->task_id);
	if (task_conf == NULL) {
		return cli_err_task_info_invalid;
	}
	uint32_t size = (uint32_t)task->step_list.size();
	if (task_conf->step_count != size) {
		return cli_err_task_info_invalid;
	}
	for (uint32_t i = 0; i < task_conf->step_count; i++) {
		task_info->add_cond_status(task->step_list[i].condition_status);
	}
	return 0;
}

int DataProtoUtils::unpack_task_data(const commonproto::task_data_t* task_info, task_t* task) {
	task->task_id = task_info->task_id();
	task->status = task_info->status();	

	task->done_times = task_info->done_times();
	task->bonus_status = (bonus_status_t)task_info->bonus_status();
	task->fin_statue_tm = task_info->fin_statue_tm();

	const task_conf_t* task_conf = TaskUtils::get_task_config(task_info->task_id());

	if (task_conf == NULL) {
		return cli_err_task_info_invalid;
	}
	if (task_conf->step_count != (uint32_t)task_info->cond_status().size()) {
		return cli_err_task_info_invalid;
	}
	for (uint32_t i = 0; i < task_conf->step_count; i++) {
		//step_t step_info;
		//step_info.step = 1;
		//step_info.condition_status = 0;
		step_t temp_step;
		temp_step.condition_status =  task_info->cond_status(i);
		task->step_list.push_back(temp_step);
	}
	return 0;
}


int DataProtoUtils::pack_player_task(player_t* player, commonproto::task_list_t* task_list) {
	std::vector<task_t> task_vec;
	player->task_info->get_all_tasks(task_vec);


	FOREACH(task_vec, it) {
		commonproto::task_data_t* task_info = task_list->add_task_list(); 
		const task_conf_t* task_conf = TaskUtils::get_task_config(it->task_id);
		if (task_conf == NULL) {
			return send_err_to_player(player, player->cli_wait_cmd, cli_err_task_info_invalid);
		}
		task_info->set_task_id(it->task_id);
		task_info->set_status(it->status);
        task_info->set_done_times(it->done_times);
        task_info->set_bonus_status(it->bonus_status);
		task_info->set_fin_statue_tm(it->fin_statue_tm);

		if (task_conf->step_count != (uint32_t)it->step_list.size()) {
			return send_err_to_player(player, player->cli_wait_cmd, cli_err_task_info_invalid);
		}
		for (uint32_t j = 0; j < task_conf->step_count; j++) {
			task_info->add_cond_status(it->step_list[j].condition_status);
		
		}
	}
	return 0;
}

int DataProtoUtils::unpack_player_task(player_t* player, const commonproto::task_list_t& task_list) {

	for(int i=0; i<task_list.task_list_size(); i++) {
		const commonproto::task_data_t& task_info = task_list.task_list(i);
		task_t task;
		task.task_id = task_info.task_id();
		task.status = task_info.status();
		task.done_times = task_info.done_times();
		task.bonus_status = (bonus_status_t)task_info.bonus_status();
		task.fin_statue_tm = task_info.fin_statue_tm();

		const task_conf_t* task_conf = TaskUtils::get_task_config(task_info.task_id());
		if (task_conf == NULL) {
            ERROR_TLOG("task data del, id illegal, userid:%u, taskid:%u", 
                    player->userid, task_info.task_id());
            TaskUtils::db_del_task(player, task_info.task_id());
            continue;
		}

		if (task_conf->step_count != (uint32_t)task_info.cond_status().size()) {
            ERROR_TLOG("task data del, step_count error, userid:%u, taskid:%u, step_count:%u, cond_size:%u", 
                    player->userid, task_info.task_id(), 
                    task_conf->step_count, task_info.cond_status().size()); 
            TaskUtils::db_del_task(player, task_info.task_id());
            continue;
		}

		for (uint32_t i = 0; i < task_conf->step_count; i++) {
			//step_t step_info;
			//step_info.step = 1;
			//step_info.condition_status = 0;
			step_t temp_step;
			temp_step.condition_status =  task_info.cond_status(i);
			task.step_list.push_back(temp_step);
		}

		player->task_info->put_task(task);
	}
	return 0;
}

int DataProtoUtils::pack_rune_data(player_t* player,
		commonproto::rune_list_cli_t* rune_list)
{
	RuneMeseum::RuneMap tmp_map;
	player->rune_meseum->get_rune_map_info(tmp_map);
	FOREACH(tmp_map, it) {
		commonproto::rune_data_cli_t* rune_data_ptr = rune_list->add_rune_data();
		commonproto::rune_data_t* rune_info = rune_data_ptr->mutable_rune_info();
		rune_info->set_runeid(it->second.id);
		rune_info->set_index(it->second.conf_id);
		rune_info->set_exp(it->second.exp);
		rune_info->set_level(it->second.level);
		rune_info->set_pack_type(it->second.pack_type);
		rune_info->set_pet_catch_time(it->second.pet_catch_time);
		rune_info->set_grid_id(it->second.grid_id);
		rune_data_ptr->set_flag(1);
	}
	return 0;
}

int DataProtoUtils::pack_all_achieves_info(player_t* player,
		commonproto::achieve_info_list_t* achv_ptr) 
{
	player->achieve->pack_achieve_info_list(achv_ptr);
	return 0;
}

int DataProtoUtils::unpack_rune_data(player_t* player, 
		const commonproto::rune_list_t& rune_list) 
{
	uint32_t rune_list_size = rune_list.rune_data_size();
	for (uint32_t i = 0; i < rune_list_size; ++i) {
		const commonproto::rune_data_t& rune_data_ref = rune_list.rune_data(i);
		rune_t rune;
		rune.id = rune_data_ref.runeid();
		rune.conf_id = rune_data_ref.index();
		rune.level = rune_data_ref.level();
		rune.exp = rune_data_ref.exp();
		rune.pack_type = rune_data_ref.pack_type();
		rune.pet_catch_time = rune_data_ref.pet_catch_time();
		rune.grid_id = rune_data_ref.grid_id();

		player->rune_meseum->save_rune(rune);
	}
	return 0;
}

int DataProtoUtils::unpack_achieves_info(player_t* player,
		const commonproto::achieve_info_list_t& achieve_list)
{
	uint32_t achv_list_size = achieve_list.ach_info_size();
	std::vector<achieve_info_t> achv_vec;
	for (uint32_t i = 0; i < achv_list_size; ++i) {
		const commonproto::achieve_info_t& achv_inf = achieve_list.ach_info(i);
		achieve_info_t achv_info;
		achv_info.id = achv_inf.achieve_id();
		if (player->achieve->is_exist_this_achieve(achv_info.id)) {
			//理论上不会走到这里
			continue;
		}
		achv_info.get_time = achv_inf.get_time();
		achv_vec.push_back(achv_info);
	}
	player->achieve->insert_achieves_to_memory(achv_vec);	
	return 0;
}

int DataProtoUtils::pack_tran_card_data(player_t* player,
		onlineproto::sc_0x0101_enter_svr& cli_out_)
{
	TranCard::TranCardMap tmp_map;
	player->m_tran_card->get_tran_map_info(tmp_map);
	FOREACH(tmp_map, it) { 
		//commonproto::tran_card_cli_t* tran_data_ptr = cli_out_.add_tran_card_list();
		//commonproto::tran_card_t* card_ptr = tran_data_ptr->mutable_card_info();
		if (it->second.card_id < 40001 || it->second.card_id > 40100) {
			continue;
		}
		commonproto::tran_card_t* card_ptr = cli_out_.add_tran_card_list();
		card_ptr->set_card_id(it->second.card_id);
		card_ptr->set_card_star_level(it->second.card_star_level);
	}
	return 0;
}

int DataProtoUtils::unpack_tran_data(player_t* player) {
	for (uint32_t i = kAttrTransCardStart + 1; i < kAttrTransCardEnd; i += 2) {
		if (!AttrUtils::has_attr(player, (attr_type_t)i)) {
			continue;
		}
		tran_card_t tran_card;
		tran_card.card_id = GET_A((attr_type_t)i);
		tran_card.card_star_level = GET_A((attr_type_t)(i + 1));
		TRACE_TLOG("p=[%u]unpack_tran_data:[card_id=%u],card_level=[%u],i=[%u]", player->userid, tran_card.card_id, tran_card.card_star_level, i);
		if (tran_card.card_id < 40000 || tran_card.card_id > 40100 || tran_card.card_star_level > 10) {
			AttrUtils::ranged_clear(player, i, i, true);
			ERROR_TLOG("dirty data;tran_card_id=[%u],tran_card_level=[%u],index=[%u]", tran_card.card_id, tran_card.card_star_level, i);
			TRACE_TLOG("dirty data;tran_card_id=[%u],tran_card_level=[%u],index=[%u]", tran_card.card_id, tran_card.card_star_level, i);
			continue;
		}
		player->m_tran_card->set_tranCard(tran_card);
	}
	return 0;
}

int DataProtoUtils::pack_single_rune_data(rune_t& rune, commonproto::rune_data_t* rune_data)
{
	rune_data->set_runeid(rune.id);
	rune_data->set_index(rune.conf_id);
	rune_data->set_exp(rune.exp);
	rune_data->set_level(rune.level);
	rune_data->set_pack_type(rune.pack_type);
	rune_data->set_pet_catch_time(rune.pet_catch_time);
	return 0;
}

uint32_t DataProtoUtils::pack_player_map_pet_info(player_t* player, 
		commonproto::map_pet_list_t *pet_list) {

	for (int i = 0; i < MAX_FIGHT_POS; i++) {
        Pet *pet = player->fight_pet[i];
        if (!pet) {
            continue;
        }
		commonproto::map_pet_info_t* map_pet = pet_list->add_pets();
		DataProtoUtils::pack_map_pet_info(pet, map_pet);
	}
	return 0;
}

int DataProtoUtils::unpack_friend_data(player_t* player,
		const commonproto::friend_list_t& friend_list)
{
	uint32_t friend_list_size = friend_list.friend_list_size();
	friend_t temp_friend;
	recent_t temp_recent;
	black_t temp_blacklist;
	temp_t temp;
	for (uint32_t i = 0; i < friend_list_size; i++) {
		const commonproto::friend_data_t& friend_data = friend_list.friend_list(i);

		if (friend_data.is_temp() == 1) {
			dbproto::cs_remove_friend cs_remove_friend_;
			cs_remove_friend_.mutable_finf()->CopyFrom(friend_data);
			/*
			cs_remove_friend_.set_userid(player->userid);
			cs_remove_friend_.set_friendid(friend_data.friendid());
			*/
			g_dbproxy->send_msg(NULL, player->userid, player->create_tm, db_cmd_remove_friend, cs_remove_friend_);

			bool has_friend = player->friend_info->has_friend(friend_data.friendid(), friend_data.f_create_tm());
			//如果对方已经在自己的好友列表中，则不通知对方
			if (has_friend == true) {
				continue;
			}
			temp.friendid = friend_data.friendid();
			temp.create_tm = friend_data.f_create_tm();
			player->friend_info->add_temp(temp);
			continue;	
		}

		if (friend_data.is_friend() == 1) {
			temp_friend.id = friend_data.friendid();
			temp_friend.create_tm = friend_data.f_create_tm();
			/*
			if (TimeUtils::is_same_day(GET_A(kAttrLastLoginTm), NOW()))	{
				temp_friend.gift_count = friend_data.gift_count();
			} else {
				dbproto::cs_update_gift_count db_msg;
				db_msg.set_userid(player->userid);
				db_msg.set_friendid(temp_friend.id);
				db_msg.set_gift_count(0);
				g_dbproxy->send_msg(
						NULL, player->userid, 
						db_cmd_update_gift_count, db_msg);	

				temp_friend.gift_count = 0;
			}
			*/
			player->friend_info->add_friend(temp_friend);
		}

		if (friend_data.is_recent() == 1) {
			temp_recent.id = friend_data.friendid();
			temp_recent.create_tm = friend_data.f_create_tm();
			player->friend_info->add_recent_back(temp_recent);
		}

		if (friend_data.is_blacklist() == 1) {
			temp_blacklist.id = friend_data.friendid();
			temp_blacklist.create_tm = friend_data.f_create_tm();
			player->friend_info->add_black(temp_blacklist);
		}
	}
	return 0;
}
int DataProtoUtils::pack_friend_data(player_t* player,
		commonproto::friend_info_list_t* friend_list)
{
	std::vector<temp_t> templist;
	player->friend_info->get_all_temps(templist);

	for (uint32_t i = 0; i < templist.size(); i++) {
		commonproto::friend_info_t* friend_info = friend_list->add_friend_info();
		friend_info->set_userid(templist[i].friendid);
		//friend_info->set_create_tm(templist[i].create_tm);
		friend_info->set_is_online(0);
		friend_info->set_team(3);
	}
	return 0;
}

int DataProtoUtils::unpack_hm_visit_log(player_t* player, 
		const commonproto::visit_log_list_t& log_list)
{
	uint32_t visit_size = log_list.visit_size();	
	for (uint32_t i = 0; i < visit_size; ++i) {
		const commonproto::visit_log_info_t& log_info = log_list.visit(i);
		player->home_data->add_visit_log(log_info);
	}
	return 0;
}

int DataProtoUtils::pack_hm_visit_log(player_t* player, 
		commonproto::visit_log_list_t* log_list) 
{
	home_data_t::visit_log_map_t visit_map;
	player->home_data->get_visit_log_map_info(visit_map);
	FOREACH(visit_map, it) {
		commonproto::visit_log_info_t * log_info = log_list->add_visit();
		log_info->set_hostid(it->second.host_id_);
		log_info->set_h_create_tm(it->second.h_create_tm_);
		log_info->set_guestid(it->second.guest_id_);
		log_info->set_g_create_tm(it->second.g_create_tm_);
		log_info->set_sex(it->second.sex_);
		log_info->set_guestname(it->second.guest_name_);
		log_info->set_date(it->second.date_);
		log_info->set_logtype((commonproto::log_type_t)it->second.log_type_);
		log_info->set_detail(it->second.detail_);
		log_info->set_gift_id(it->second.fragment_id_);
	}
	return 0;
}

int DataProtoUtils::unpack_hm_fragment_info(
		player_t* player, const std::string& buff_str)
{
	player->home_data->set_hm_frag(buff_str);
	return 0;
}

int DataProtoUtils::pack_pk_players_btl_info_include_tick(
		player_t* player, player_t* ai_player_ptr, 
		uint32_t atk_tick, uint32_t def_tick,
		commonproto::battle_data_include_tick_t* btl_ptr,
		commonproto::btl_ground_type_t type)
{
	commonproto::battle_player_data_t *def_btl_ptr = btl_ptr->add_players();
	DataProtoUtils::pack_player_battle_all_info(ai_player_ptr,	def_btl_ptr);
	commonproto::battle_player_data_t *atk_btl_ptr = btl_ptr->add_players();
	DataProtoUtils::pack_player_battle_all_info(player, atk_btl_ptr);
	//将战报数据临时保存在temp_info_t::tmp_btl_info中
	//等战斗胜负协议中，添加了胜负结果，再保存到redis中
	if (type == commonproto::GROUND_ARENA || type == commonproto::GROUND_ESCORT) {
		ArenaUtils::arena_modify_player_btl_attr(*def_btl_ptr);
		ArenaUtils::arena_modify_player_btl_attr(*atk_btl_ptr);
	}
	RankUtils::save_arena_battle_report(player, 
			*def_btl_ptr, *atk_btl_ptr, 
			atk_tick, def_tick);

	btl_ptr->set_atk_tick(atk_tick);
	btl_ptr->set_def_tick(def_tick);
	return 0;
}

int DataProtoUtils::pack_player_buff(player_t *player, commonproto::user_buffs_t *user_buffs)
{
    user_buffs->Clear();
    FOREACH((*(player->buff_id_map)), it) {
        uint32_t buff_id = it->second;
        user_buffs->add_buff_id(buff_id);
    }
    return 0;
}

int DataProtoUtils::unpack_player_buff(player_t *player, const commonproto::user_buffs_t &user_buffs)
{
    for (int i = 0; i < user_buffs.buff_id_size(); i++) {
        uint32_t buff_id = user_buffs.buff_id(i);
        PlayerUtils::add_player_buff(player, buff_id);
    }
    return 0;
}

int DataProtoUtils::unpack_player_apply_family_ids(
        player_t *player, const commonproto::family_apply_record_t &apply_record)
{
    if (player && player->family_apply_record) {
        player->family_apply_record->clear();
        for (int i = 0; i < apply_record.family_ids_size(); i++) {
            player->family_apply_record->insert(apply_record.family_ids(i));
        }

    }

    return 0;
}

int DataProtoUtils::unpack_shop_items(player_t *player, uint32_t market_type, 
        const commonproto::market_item_info_t &shop_info) 
{
    if (!player) {
        return 0;
    }
    std::map<uint32_t, ol_market_item_t> *shop_items;
    switch(market_type) {
    case MARKET_TYPE_DAILY:
        shop_items = player->daily_shop_items;
        break;
    case MARKET_TYPE_FAMILY:
        shop_items = player->family_shop_items;
        break;
    case MARKET_TYPE_ARENA:
        shop_items = player->arena_shop_items;
        break;
    case MARKET_TYPE_ELEM_DUP:
        shop_items = player->elem_dup_shop_items;
        break;
    case MARKET_TYPE_EXPED:
        shop_items = player->exped_shop_items;
        break;
    case MARKET_TYPE_NIGHT_RAID:
        shop_items = player->night_raid_shop_items;
        break;
	case MARKET_TYPE_SMELT_MONEY:
		shop_items = player->smelter_money_shop_items;
		break;
	case MARKET_TYPE_SMELT_GOLD:
		shop_items = player->smelter_gold_shop_items;
		break;
    default: 
        return 0;
    }
    if (!shop_items) {
        return 0;
    }
    shop_items->clear();
    for (int i = 0; i < shop_info.items_size(); i++) {
        ol_market_item_t item;
        item.item_id = shop_info.items(i).item_id();
        item.count = shop_info.items(i).count();
        shop_items->insert(std::pair<uint32_t, ol_market_item_t>(shop_info.items(i).item_id(), item));
    }
    return 0;
}

#if 0
int DataProtoUtils::unpack_elem_dup_shop_items(
        player_t *player, const commonproto::market_item_info_t &shop_info)
{
    if (player && player->elem_dup_shop_items) {
        player->elem_dup_shop_items->clear();
        for (int i = 0; i < shop_info.items_size(); i++) {
            ol_market_item_t item;
            item.item_id = shop_info.items(i).item_id();
            item.count = shop_info.items(i).count();
            player->elem_dup_shop_items->insert(
                    std::pair<uint32_t, ol_market_item_t>(
                        shop_info.items(i).item_id(), item));
        }

    }

    return 0;
}

int DataProtoUtils::unpack_arena_shop_items(
        player_t *player, const commonproto::market_item_info_t &shop_info)
{
    if (player && player->arena_shop_items) {
        player->arena_shop_items->clear();
        for (int i = 0; i < shop_info.items_size(); i++) {
            ol_market_item_t item;
            item.item_id = shop_info.items(i).item_id();
            item.count = shop_info.items(i).count();
            player->arena_shop_items->insert(
                    std::pair<uint32_t, ol_market_item_t>(
                        shop_info.items(i).item_id(), item));
        }

    }

    return 0;
}

int DataProtoUtils::unpack_family_shop_items(
        player_t *player, const commonproto::market_item_info_t &shop_info)
{
    if (player && player->family_shop_items) {
        player->family_shop_items->clear();
        for (int i = 0; i < shop_info.items_size(); i++) {
            ol_market_item_t item;
            item.item_id = shop_info.items(i).item_id();
            item.count = shop_info.items(i).count();
            player->family_shop_items->insert(
                    std::pair<uint32_t, ol_market_item_t>(
                        shop_info.items(i).item_id(), item));
        }

    }

    return 0;
}

int DataProtoUtils::unpack_exped_shop_items(
		player_t* player, const commonproto::market_item_info_t &shop_info)
{
    if (player && player->exped_shop_items) {
        player->exped_shop_items->clear();
        for (int i = 0; i < shop_info.items_size(); i++) {
            ol_market_item_t item;
            item.item_id = shop_info.items(i).item_id();
            item.count = shop_info.items(i).count();
            player->exped_shop_items->insert(
                    std::pair<uint32_t, ol_market_item_t>(
                        shop_info.items(i).item_id(), item));
        }

    }
	return 0;
}
#endif

int DataProtoUtils::unpack_all_titles_info(player_t* player,
		const commonproto::title_info_list_t & titles_list)
{
	uint32_t title_size = titles_list.title_info_size();	
	for (uint32_t i = 0; i < title_size; ++i) {
		title_info_t title_info;
		title_info.title_id = titles_list.title_info(i).title_id();
		title_info.get_time = titles_list.title_info(i).get_time();
		player->title->insert_one_title_to_memory(title_info);
	}
	return 0;
}

int DataProtoUtils::pack_all_titles_info(player_t* player,
		commonproto::title_info_list_t* titles_ptr)
{
	//先删除已经过期的称号
	player->title->erase_expire_titles(player);
	//再打包内存中的称号信息
	player->title->pack_all_titles(titles_ptr);
	return 0;
}

int DataProtoUtils::unpack_mine_ids(player_t* player,
		const commonproto::mine_id_list_t& mine_ids_inf)
{
	player->mine_info->clear_my_mine_ids();
	for (int i = 0; i < mine_ids_inf.mine_id_size(); ++i) {
		TRACE_TLOG("unpack_mine_ids,id=%u,uid=%u", mine_ids_inf.mine_id(i), player->userid);
		player->mine_info->insert_my_mine_ids_to_memory(mine_ids_inf.mine_id(i));
	}
	return 0;
}

int DataProtoUtils::unpack_mine_tmp_info(player_t* player,
		const commonproto::mine_info_list_t& mine_info_inf)
{
	player->mine_info->clear_mine_tmp_info();
	for (int i = 0; i < mine_info_inf.mine_info_size(); ++i) {
		struct mine_info_t mine_info;
		MineUtils::unpack_mine_info(mine_info_inf.mine_info(i), mine_info);
		player->mine_info->save_mine_tmp_info_to_memory(mine_info.mine_id, mine_info);
	}
	return 0;
}

int DataProtoUtils::unpack_mine_match_ids(player_t* player,
		const commonproto::mine_id_list_t& match_ids_inf)
{
	player->mine_info->clear_match_mine_ids();
	for (int i = 0; i < match_ids_inf.mine_id_size(); ++i) {
		player->mine_info->save_match_mine_ids_to_memory(match_ids_inf.mine_id(i));
	}
	return 0;
}

/*
int DataProtoUtils::pack_pet_diju_extra_attr(
		commonproto::battle_player_data_t& battle_all_info)
{
	const commonproto::battle_pet_list_t& pb_btl = battle_all_info.pet_list();
	for (int i = 0; i < pb_btl.pet_list_size(); ++i) {
		const commonproto::pet_optional_attr_t& pb_attr = pb_btl.pet_list(i).pet_info().pet_optional_attr();

		commonproto::battle_pet_data_t* btl_ptr = battle_all_info.mutable_pet_list()->mutable_pet_list(i);
		for (int j = 0; j < pb_attr.lamp_state_size(); ++j) {
			TRACE_TLOG("Pack Pet diju Extra Attr,pet_id=[%u],lamp_state=[%u]", 
					pb_btl.pet_list(i).pet_info().base_info().pet_id(),
					pb_attr.lamp_state(j));
			PetUtils::convert_lamp_state_to_pet_btl_attr(pb_attr.lamp_state(j),
					btl_ptr->mutable_pet_info()->mutable_battle_info());
		}
	}

	const commonproto::battle_pet_list_t& pb_sw_btl = battle_all_info.switch_pet_list();
	for (int i = 0; i < pb_sw_btl.pet_list_size(); ++i) {
		const commonproto::pet_optional_attr_t& pb_attr = pb_sw_btl.pet_list(i).pet_info().pet_optional_attr();
		commonproto::battle_pet_data_t* btl_ptr = battle_all_info.mutable_switch_pet_list()->mutable_pet_list(i);
		for (int j = 0; j < pb_attr.lamp_state_size(); ++j) {
			TRACE_TLOG("Pack Switch Pet diju Extra Attr,pet_id=[%u],lamp_state=[%u]", 
					pb_sw_btl.pet_list(i).pet_info().base_info().pet_id(),
					pb_attr.lamp_state(j));
			PetUtils::convert_lamp_state_to_pet_btl_attr(pb_attr.lamp_state(j),
					btl_ptr->mutable_pet_info()->mutable_battle_info());
		}
	}
	return 0;
}
*/

int DataProtoUtils::pack_pet_diju_extra_attr(
	commonproto::battle_player_data_t& battle_all_info)
{
	const commonproto::battle_pet_list_t& pb_btl = battle_all_info.pet_list();
	for (int i = 0; i < pb_btl.pet_list_size(); ++i) {
		const commonproto::lamp_info_list_t& pb_attr = pb_btl.pet_list(i).pet_info().pet_optional_attr().lamp();
		uint32_t awake_state = pb_btl.pet_list(i).pet_info().pet_optional_attr().awake_state();
		float tmp_conf = 1;
		if (awake_state) {
			tmp_conf = 1.5;
		}
		uint32_t cur_hp = pb_btl.pet_list(i).pet_info().battle_info().cur_hp();
		uint32_t max_hp = pb_btl.pet_list(i).pet_info().battle_info().max_hp();

		uint32_t total_cur_hp = 0, total_max_hp = 0;
		bool exsit_lamp_state = false;
		for (int j = 0; j < pb_attr.lamp_list_size(); ++j) {
			uint32_t hp_lv = pb_attr.lamp_list(j).hp_lv();
			uint32_t new_cur_hp = ceil(cur_hp * (1 + hp_lv * 3 / 100.0) * tmp_conf);
			uint32_t max_cur_hp = ceil(max_hp * (1 + hp_lv * 3 / 100.0) * tmp_conf);
			total_cur_hp += new_cur_hp;
			total_max_hp += max_cur_hp;
			exsit_lamp_state = true;
		}

		if (exsit_lamp_state)  {
			commonproto::battle_pet_data_t* btl_ptr = battle_all_info.mutable_pet_list()->mutable_pet_list(i);
			btl_ptr->mutable_pet_info()->mutable_battle_info()->set_cur_hp(total_cur_hp);
			btl_ptr->mutable_pet_info()->mutable_battle_info()->set_max_hp(total_max_hp);
		}
	}

	const commonproto::battle_pet_list_t& pb_sw_btl = battle_all_info.switch_pet_list();
	for (int i = 0; i < pb_sw_btl.pet_list_size(); ++i) {
		const commonproto::lamp_info_list_t& pb_attr = pb_sw_btl.pet_list(i).pet_info().pet_optional_attr().lamp();
		uint32_t awake_state = pb_sw_btl.pet_list(i).pet_info().pet_optional_attr().awake_state();
		float tmp_conf = 1;
		if (awake_state) {
			tmp_conf = 1.5;
		}
		uint32_t cur_hp = pb_sw_btl.pet_list(i).pet_info().battle_info().cur_hp();
		uint32_t max_hp = pb_sw_btl.pet_list(i).pet_info().battle_info().max_hp();

		bool exsit_lamp_state = false;
		uint32_t total_cur_hp = 0, total_max_hp = 0;
		for (int j = 0; j < pb_attr.lamp_list_size(); ++j) {
			uint32_t hp_lv = pb_attr.lamp_list(j).hp_lv();
			uint32_t new_cur_hp = ceil(cur_hp * (1 + hp_lv * 3 / 100.0) * tmp_conf);
			uint32_t max_cur_hp = ceil(max_hp * (1 + hp_lv * 3 / 100.0) * tmp_conf);
			exsit_lamp_state = true;
			total_cur_hp += new_cur_hp;
			total_max_hp += max_cur_hp;
		}
		if (exsit_lamp_state) {
			commonproto::battle_pet_data_t* btl_ptr = battle_all_info.mutable_switch_pet_list()->mutable_pet_list(i);
			btl_ptr->mutable_pet_info()->mutable_battle_info()->set_cur_hp(total_cur_hp);
			btl_ptr->mutable_pet_info()->mutable_battle_info()->set_max_hp(total_max_hp);
		}
	}
	return 0;
}
int DataProtoUtils::trim_battle_player_info(player_t *player, 
		const commonproto::battle_player_data_t &player_info, commonproto::battle_player_data_t &cache_player_info)
{
	cache_player_info.mutable_base_info()->CopyFrom(player_info.base_info());
	DEBUG_TLOG("night raid player_info baseinfo size = %u,uid=%u", player_info.base_info().ByteSize(), player->userid);
	cache_player_info.mutable_battle_info()->CopyFrom(player_info.battle_info());
	DEBUG_TLOG("night raid player_info battle_info size = %u,uid=%u", player_info.battle_info().ByteSize(), player->userid);
	cache_player_info.mutable_equip_list()->CopyFrom(player_info.equip_list());
	DEBUG_TLOG("night raid player_info equip_list size = %u,uid=%u", player_info.equip_list().ByteSize(), player->userid);
	cache_player_info.mutable_pet_list()->CopyFrom(player_info.pet_list());
	DEBUG_TLOG("night raid player_info pet_list size = %u,uid=%u", player_info.pet_list ().ByteSize(), player->userid);
	cache_player_info.set_x_pos(player_info.x_pos());
	cache_player_info.set_y_pos(player_info.y_pos());
	cache_player_info.set_heading(player_info.heading());
	DEBUG_TLOG("night raid before pack state_bytes,size=%u", cache_player_info.ByteSize());
	cache_player_info.set_state_bytes(player_info.state_bytes());
	DEBUG_TLOG("night raid after pack state_bytes,size=%u", cache_player_info.ByteSize());
	//DEBUG_TLOG("night raid player_info stateByte size = %u,uid=%u", player_info.state_bytes().ByteSize(), player->userid);
	//cache_player_info.mutable_chisel_pet_list()->CopyFrom(player_info.chisel_pet_list());
	DEBUG_TLOG("night raid player_info chisel_pet_list size = %u,uid=%u", player_info.chisel_pet_list().ByteSize(), player->userid);
	cache_player_info.mutable_skills()->CopyFrom(player_info.skills());
	cache_player_info.set_family_dup_boss_lv(player_info.family_dup_boss_lv());
	cache_player_info.set_family_dup_boss_hp(player_info.family_dup_boss_hp());
	cache_player_info.set_family_dup_boss_maxhp(player_info.family_dup_boss_maxhp());
	cache_player_info.set_team(player_info.team());
	cache_player_info.set_arena_rank(player_info.arena_rank());
	cache_player_info.set_is_captain(player_info.is_captain());
	cache_player_info.set_rpvp_score(player_info.rpvp_score());
	return 0;
}
