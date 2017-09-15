#include "item_func_processor.h"
#include "player.h"
#include "pet_conf.h"
#include "pet_utils.h"
#include "item.h"
#include "player_utils.h"
#include "package.h"
#include "proto/db/db_cmd.h"
#include "proto/client/cli_cmd.h"
#include "data_proto_utils.h"
#include "service.h"
#include "attr_utils.h"
#include "utils.h"
#include "map_utils.h"
#include "item_conf.h"
#include "task_utils.h"
#include "prize.h"
#include "achieve_conf.h"
#include "achieve.h"

#define USE_ITEM_COMMON_CHECK \
    uint32_t item_id = cli_in_.item_id();\
    uint32_t count = cli_in_.count(); \
    uint32_t slot_id = cli_in_.slot_id(); \
    item_t *item = player->package->get_mutable_item_in_slot(slot_id); \
    if (!item || item->count < count) { \
        return cli_err_lack_usable_item; \
    } \
    const item_conf_t* item_conf = g_item_conf_mgr.find_item_conf(item_id); \
    if(NULL == item_conf) { \
        return cli_err_item_not_exist; \
    }

//玩家经验值药水
int PlayerAddExpItemFuncProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    USE_ITEM_COMMON_CHECK;

    // 计算一下最大需要的经验果个数
    uint32_t single_fruit_exp = item_conf->fun_args.size() ?item_conf->fun_args[0] :0;
    uint32_t get_total_exp = single_fruit_exp * count;
    uint32_t level_full_need_exp = PlayerUtils::calc_level_up_need_exp(GET_A(kAttrLv), PLAYER_MAX_LEVEL);
    if (get_total_exp > level_full_need_exp) {
        count = (level_full_need_exp + single_fruit_exp - 1) / single_fruit_exp;
    }

    if(0 == count) {
        return cli_err_player_level_max;
    }

    uint32_t ret = reduce_item_by_slot_id(player, slot_id, count, false, 
            onlineproto::SYNC_REASON_USE_ITEM);
    if (ret) {
        return ret;
    }

    uint32_t add_exp = single_fruit_exp * count;
    uint32_t real_add_exp = 0;

    ret = PlayerUtils::add_player_exp(player, add_exp, 
            &real_add_exp, ADDICT_DETEC, onlineproto::SYNC_EXP_FROM_DRUGS);
    if (ret) {
        return ret;
    }
    return 0;
}

//玩家血瓶
int PlayerAddHpItemFuncProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    USE_ITEM_COMMON_CHECK;

#if 0
    // 计算一下最大需要的血瓶个数
    uint32_t single_fruit_hp = item_conf->fun_args.size() ?item_conf->fun_args[0] :0;
    uint32_t get_total_hp = single_fruit_hp * count;
    uint32_t recover_need_to_full_hp = GET_A(kAttrHpMax);
    if (get_total_hp > recover_need_to_full_hp) {
        count = (recover_need_to_full_hp + single_fruit_hp - 1) / single_fruit_hp;
    }

    if(0 == count) {
        return 0;
    }
#endif
    int ret = reduce_item_by_slot_id(player, slot_id, count, false, 
            onlineproto::SYNC_REASON_USE_ITEM);
    if (ret) {
        return ret;
    }
#if 0
    uint32_t add_hp = single_fruit_hp * count;
    uint32_t real_add_hp = 0;

    ret = PlayerUtils::add_player_hp(player, add_hp, 
            &real_add_hp, onlineproto::SYNC_HP_FROM_DRUGS);
    if (ret) {
        return ret;
    }
#endif

    return 0;
}

int equip_one_item(player_t *player, item_t *item, uint32_t pos, std::vector<item_t> &change_item_vec)
{
    attr_type_t slot_attr = AttrUtils::get_player_attr_by_equip_pos((equip_body_pos_t)pos);
    uint32_t old_slot_id = GET_A(slot_attr);
    if(old_slot_id) {
        item_t* old_item = player->package->get_mutable_item_in_slot(old_slot_id);
        if (old_item == NULL) {
            KERROR_LOG(player->userid, "old_slot not exist, id:%u, attr_type:%u", 
                    old_slot_id, slot_attr);
            SET_A(slot_attr, 0);
            return cli_err_item_not_exist;
        }
        if (old_item != item) { //不同的一件物品
            if (old_item->using_count) {
                old_item->using_count--;
            } else {
                old_item->using_count = 0;
            }        
            change_item_vec.push_back(*old_item);
        }
    }

    if(item->using_count) {//在使用中 则脱下来
        item->using_count--;
        if (item->using_count == 0) {//脱了
            SET_A(slot_attr, 0);
        }
    } else {//装上去
        item->using_count++;
        SET_A(slot_attr, item->slot_id);
    }
    change_item_vec.push_back(*item);

    if ((equip_body_pos_t)pos == EQUIP_FASHION_BODY_POS_HEAD) {
        cacheproto::cs_set_cache cache_msg;
        ostringstream value;
        value.clear();
        value << item->item_id;
        cache_msg.set_key(PlayerUtils::make_head_cache_key(player->userid, player->create_tm));
        cache_msg.set_value(value.str());
        cache_msg.set_ttl(0);
        g_dbproxy->send_msg(NULL, player->userid, player->create_tm, cache_cmd_set_cache, cache_msg);
    }

    // 支线任务
    if ((equip_body_pos_t)pos == EQUIP_BODY_POS_MOUNT) {
        SET_A(kAttrRookieGuide14Mount, 1);
    }

    SET_A(kAttrRookieGuide2Weapon, 1);

    return 0;
}

int PlayerEquipArmItemFuncProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    USE_ITEM_COMMON_CHECK;

    // 更新新手引导任务
    SET_A(kAttrRookieGuide2Weapon, 1);

    if(!g_item_conf_mgr.is_equip(item->item_id)) {
        return cli_err_equip_not_found;
    }

    if (item_conf->level_limit > GET_A(kAttrLv)) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_level_too_low);
    }

    // 翅膀坐骑装备时检查培养等级
    if (item->using_count == 0 && g_item_conf_mgr.is_cultivate_equip(item->item_id)) {
        int ret = g_item_conf_mgr.can_equip_cultivate_item(player, item->item_id);
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd, ret);
        }
    }

    commonproto::item_info_t item_info;
    DataProtoUtils::pack_item_info(item, &item_info);
    //commonproto::item_optional_attr_t item_optional_attr;
    uint32_t pos = (uint32_t)item_info.item_optional_attr().part();
    if (!g_item_conf_mgr.is_valid_equip_body_pos(pos)) {
        return cli_err_invalid_equip_pos;
    }

    std::vector<item_t> change_item_vec;
    equip_one_item(player, item, pos, change_item_vec);
    int ret = db_item_change(player, change_item_vec, onlineproto::SYNC_REASON_EQUIP_ARM, true);
    if (ret) {
        return ret;
    }

	//加时装buf
	PlayerUtils::calc_player_suit_buff_info(player);
    PlayerUtils::calc_player_battle_value(player);
    g_dbproxy->send_buf(0, player->userid, cache_cmd_set_user_info_outdate, 0, 0, 0);
    MapUtils::sync_map_player_info(player, commonproto::PLAYER_EQUIP_CHANGE);

    return 0;
}

int PlayerOneKeyEquipArmItemFuncProcessor::proc_pkg_from_client(
        player_t* player,const char*body, int bodylen)
{
    PARSE_MSG;

    // 更新新手引导任务
    SET_A(kAttrRookieGuide2Weapon, 1);

    //检查装备合法性
    //<pos, item>
    std::map<uint32_t, item_t*> item_map;

    for (int i = 0; i < cli_in_.slot_id_size(); i++) {
        uint32_t slot_id = cli_in_.slot_id(i); 
        item_t *item = player->package->get_mutable_item_in_slot(slot_id); 
        if (!item) { 
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_item_not_exist);
        }
        const item_conf_t* item_conf = g_item_conf_mgr.find_item_conf(item->item_id); 
        if(!item_conf) { 
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_item_not_exist);
        }

        if (item_conf->level_limit > GET_A(kAttrLv)) {
            return send_err_to_player(player, player->cli_wait_cmd, cli_err_level_too_low);
        }

        if(!g_item_conf_mgr.is_equip(item->item_id)) {
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_equip_not_found);
        }
        commonproto::item_info_t item_info;
        DataProtoUtils::pack_item_info(item, &item_info);
        commonproto::item_optional_attr_t item_optional_attr;
        uint32_t pos = (uint32_t)item_info.item_optional_attr().part();
        if (!g_item_conf_mgr.is_valid_equip_body_pos(pos)) {
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_invalid_equip_pos);
        }
        item_map[pos] = item;
    }

    std::vector<item_t> change_item_vec;
    //都可以装备了
    FOREACH(item_map, it) {
        item_t *item = it->second;
        uint32_t pos = it->first;

        if(g_item_conf_mgr.is_cultivate_equip(item->item_id)) {
            continue;
        }
 
        equip_one_item(player, item, pos, change_item_vec);
    }
    int ret = db_item_change(player, change_item_vec, onlineproto::SYNC_REASON_EQUIP_ARM, true);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }
    PlayerUtils::calc_player_battle_value(player);
    MapUtils::sync_map_player_info(player, commonproto::PLAYER_EQUIP_CHANGE);

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

//精灵学习力药水
int PetAddEffortItemFuncProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    USE_ITEM_COMMON_CHECK;

    // 更新新手任务
    SET_A(kAttrRookieGuide10UseEffortItem, 1);

    Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm(), PET_LOC_BAG); 
    if (!pet) {
        return cli_err_pet_not_in_bag;
    }
	int idx = item_conf->add_attr_type - 1;
	uint32_t level_limit = pet->level() > kMaxPetLevel ?kMaxPetLevel :pet->level();
	//特训等级不能大于 伙伴等级
	if (pet->effort_lv(idx) >= level_limit) {
		return cli_err_effort_lv_surpressed_by_pet_lv;
	}

	uint32_t level_full_need_val = PetUtils::get_effort_lv_up_to_n_need_val(pet, idx, level_limit);
	int single_fruit_value = item_conf->fun_args.size() ?item_conf->fun_args[0] :0;
	uint32_t get_total_value = single_fruit_value * count;
	
	if (get_total_value > level_full_need_val) {
		count = (level_full_need_val + single_fruit_value - 1) / single_fruit_value;
	}

    uint32_t ret = reduce_item_by_slot_id(player, slot_id, count, false, 
            onlineproto::SYNC_REASON_USE_ITEM);
    if (ret) {
        return ret;
    }
	uint32_t add_effort_val = single_fruit_value * count;
	uint32_t real_add_val = 0;
	ret = PetUtils::add_pet_effort(player, pet, idx, add_effort_val, real_add_val);
	if (ret) {
		return ret;
	}

    return 0;
}


//精灵经验值药水
int PetAddExpItemFuncProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    //TODO(singku)
	USE_ITEM_COMMON_CHECK;
	uint32_t single_fruit_exp = item_conf->fun_args.size() ?item_conf->fun_args[0] :0;
    uint32_t get_total_exp = single_fruit_exp * count;
    Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm(), PET_LOC_BAG); 
    if (!pet) {
		return cli_err_pet_not_in_bag;
    }
	uint32_t level_limit = kMaxPetLevel;
   // uint32_t player_level = GET_A(kAttrLv);
	//  uint32_t level_limit = player_level > kMaxPetLevel ?kMaxPetLevel :player_level;
    if (pet->level() > level_limit) {
        return cli_err_pet_level_surpressed_by_player;
    }

	uint32_t level_full_need_exp = PetUtils::get_level_up_to_n_need_exp(pet, level_limit+1);
    if (level_full_need_exp == 0) {
        return cli_err_pet_level_surpressed_by_player;
    }

	//实际需要的物品数量count
	if (get_total_exp > level_full_need_exp) {
		count = (level_full_need_exp + single_fruit_exp - 1) / single_fruit_exp;
	}
    uint32_t ret = reduce_item_by_slot_id(player, slot_id, count, false, 
            onlineproto::SYNC_REASON_USE_ITEM);
    if (ret) {
        return ret;
    }

    // 更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_EXP_FRUIT, 1);

	uint32_t add_exp = single_fruit_exp * count;
	uint32_t real_add_exp = 0;
	ret = PetUtils::add_pet_exp(player, pet, add_exp, real_add_exp);
	if (ret) {
		return ret;
	}
	return 0;
}

//精灵血量
int PetAddHpItemFuncProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    USE_ITEM_COMMON_CHECK;
    Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm(), PET_LOC_BAG); 
    if (!pet) {
		return cli_err_pet_not_in_bag;
    }
#if 0
	uint32_t single_fruit_hp = item_conf->fun_args.size() ?item_conf->fun_args[0] :0;
	uint32_t get_total_hp = single_fruit_hp * count;
	uint32_t max_hp = pet->max_hp();
	if (get_total_hp > max_hp) {
		count = (max_hp - pet->hp() + single_fruit_hp - 1) / single_fruit_hp;
	}
#endif
    uint32_t ret = reduce_item_by_slot_id(player, slot_id, count, false, 
            onlineproto::SYNC_REASON_USE_ITEM);
    if (ret) {
        return ret;
    }
#if 0
	uint32_t add_hp = single_fruit_hp * count;
	pet->set_hp(add_hp + pet->hp());
	ret = PetUtils::save_pet(player, *pet, false, true);
    if (ret) {
        return ret; 
    }
    //TODO(singku)如果在副本中 需要同步血量到副本
#endif
    return 0;
}

// 伙伴升星
int PetImproveTalentItemFuncProcessor::proc_pkg_from_client(
        player_t* player,const char*body, int bodylen)
{
    PARSE_MSG;
    USE_ITEM_COMMON_CHECK;
    
    Pet *pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm(), PET_LOC_BAG);
    if (!pet) {
        return cli_err_pet_not_in_bag;
    }

    onlineproto::sc_0x0325_pet_talent_up noti_msg;
    DataProtoUtils::pack_player_pet_info(player, pet, noti_msg.mutable_old_pet_info());
    int ret = PetUtils::improve_pet_talent_level(player, pet);
    if (ret) {
        return ret;
    }

    // 更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_PET_STAR_UP, 1);

    // 新手任务
    SET_A(kAttrRookieGuide8PetTalentUp, 1);

    // 更新精灵技能
    PetUtils::update_pet_skill(player);

	AttrUtils::add_attr_in_special_time_range(player, 
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,			
			kAttrActivPetUpTelentCnt);

    DataProtoUtils::pack_player_pet_info(player, pet, noti_msg.mutable_new_pet_info());
    send_msg_to_player(player, cli_cmd_cs_0x0325_pet_talent_up, noti_msg);

	//成就监听
    return 0;
}

//体力丹道具
int PlayerAddVpItemFuncProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    USE_ITEM_COMMON_CHECK;

    uint32_t ret = reduce_item_by_slot_id(player, slot_id, count, false, 
            onlineproto::SYNC_REASON_USE_ITEM);
    if (ret) {
        return ret;
    }

    uint32_t single_item_val = item_conf->fun_args.size() ?item_conf->fun_args[0] :0;
    uint32_t cur_vp = GET_A(kAttrCurVp);
    uint32_t limit = 0;
	/*
    if (is_this_year_vip(player)) {
        limit = SVIP_VP;
    } else if (is_vip(player)) {
        limit = VIP_VP;
    } else {
        limit = NORMAL_VP;
    }
	*/
	if (is_gold_vip(player)) {
		limit = SVIP_VP;
	} else if (is_silver_vip(player)) {
		limit = VIP_VP;
	} else {
		limit = NORMAL_VP;
	}
    uint32_t new_vp = cur_vp + single_item_val;
    if (new_vp >= limit) {
        SET_A(kAttrCurVp, limit);
    } else {
        SET_A(kAttrCurVp, new_vp);
    }
    return 0;
}

//开宝箱
int PlayerOpenBoxItemFuncProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    USE_ITEM_COMMON_CHECK;

    uint32_t ret = 0;
	ret = reduce_item_by_slot_id(player, slot_id, count, false, 
            onlineproto::SYNC_REASON_USE_ITEM);
    if (ret) {
        return ret;
    }

    uint32_t prize_id = item_conf->fun_args.size() ?item_conf->fun_args[0] : 0;
    if (!prize_id) {
        return cli_err_prize_id_not_exist;
    }

	std::vector<cache_prize_elem_t> add_vec;
	for(uint32_t i = 1; i <= count; i++){
		std::vector<cache_prize_elem_t> tmp;
		tmp.clear();
		ret = transaction_pack_prize(player, prize_id, tmp, NO_ADDICT_DETEC);
		if (ret) {
			return ret;
		}
		add_vec.insert(add_vec.begin(), tmp.begin(), tmp.end());
	}

    onlineproto::sc_0x0112_notify_get_prize noti;
	ret = transaction_proc_packed_prize(player, add_vec, &noti, 
            commonproto::PRIZE_REASON_NO_REASON, g_prize_conf_mgr.get_prize_desc(prize_id));
    if (ret) {
        return ret;
    }

    send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti);
    return 0;
}
