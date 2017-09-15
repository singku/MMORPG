#include "item_processor.h"
#include "item.h"
#include "item_func_processor.h"
#include "map_utils.h"
#include "pet_utils.h"
#include "attr.h"
#include "attr_utils.h"
#include "utils.h"
#include "item_conf.h"

ItemRestrictInterface* ItemUseCmdProcessor::get_restrict_processor(
        uint32_t res_type)
{
    item_restrict_funcs_map::iterator it = restrict_funcs_.find(res_type);
    if (it == restrict_funcs_.end()) {
        return NULL; 
    }

    return it->second;
}

void ItemUseCmdProcessor::register_restrict_funcs(
        uint32_t res_type, ItemRestrictInterface* item_restrict_processor)
{
    restrict_funcs_[res_type] = item_restrict_processor;
}

void ItemUseCmdProcessor::init()
{
    item_funcs_.register_iface(ITEM_FUNC_ADD_PLAYER_EXP, new PlayerAddExpItemFuncProcessor());
    item_funcs_.register_iface(ITEM_FUNC_ADD_PLAYER_HP, new PlayerAddHpItemFuncProcessor());
	item_funcs_.register_iface(ITEM_FUNC_EQUIP_ARM, new PlayerEquipArmItemFuncProcessor());
    item_funcs_.register_iface(ITEM_FUNC_ADD_PET_EFFORT, new PetAddEffortItemFuncProcessor());
    item_funcs_.register_iface(ITEM_FUNC_ADD_PET_EXP, new PetAddExpItemFuncProcessor());
    item_funcs_.register_iface(ITEM_FUNC_ADD_PET_HP, new PetAddHpItemFuncProcessor());
    item_funcs_.register_iface(ITEM_FUNC_IMPROVE_TALENT, new PetImproveTalentItemFuncProcessor());
    item_funcs_.register_iface(ITEM_FUNC_ADD_VP, new PlayerAddVpItemFuncProcessor());
    item_funcs_.register_iface(ITEM_FUNC_OPEN_BOX, new PlayerOpenBoxItemFuncProcessor());

    // register_restrict_funcs(kItemRestrictPetList, new ItemRestrictPetList());
}


int ItemUseCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
	
    uint32_t item_id = cli_in_.item_id();
    uint32_t slot_id = cli_in_.slot_id();

    const item_conf_t* item_conf = g_item_conf_mgr.find_item_conf(item_id);
    if(!item_conf) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_item_not_exist);
    }

    if (item_conf->level_limit > GET_A(kAttrLv)) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_level_too_low);
    }

    // 检查vip限制
    if (!(item_conf->req_vip_lv <= (uint32_t)get_player_vip_flag(player))) {
        int err = cli_err_use_item_need_silver_vip;
        if (item_conf->req_vip_lv == GOLD_VIP) {
            err = cli_err_use_item_need_gold_vip;
        }
        return send_err_to_player(player, player->cli_wait_cmd, err);
    }

    item_t *item = player->package->get_mutable_item_in_slot(slot_id);
    if (!item) {
        if (item_conf->auto_use == false) { 
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_lack_usable_item);
        } else {
            RET_MSG;
        }

    }

    uint32_t func = (uint32_t)(item_conf->fun_type);
    CmdProcessorInterface* item_func = item_funcs_.get_iface(func);
    if(!item_func){
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_item_unusable);
    }
    player->temp_info.cur_use_item_slot_id = cli_in_.slot_id();

#if 0
    if (item_conf->res_type) {
        ItemRestrictInterface* res_func = get_restrict_processor(item_conf->res_type);
        if (res_func) {
            uint32_t err = res_func->check_restrict(player, item_conf->res_args, cs_use_item_); 
            if (err) {
                return send_err_to_player(player, player->cli_wait_cmd, err); 
            }
        }
    }
    #endif
    int ret = item_func->proc_pkg_from_client(player, body, bodylen);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }
    cli_out_.Clear();
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int ItemUseCmdProcessor::proc_pkg_from_serv(
        player_t* player, const char* body, int bodylen)
{ 
    uint32_t slot_id = player->temp_info.cur_use_item_slot_id;
    item_t *item = player->package->get_mutable_item_in_slot(slot_id);
    if (!item) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_lack_usable_item);
    }
    const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(item->item_id);
    if (!item_conf) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_item_not_exist);
    }
    if (item_conf->level_limit > GET_A(kAttrLv)) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_level_too_low);
    }
    uint32_t fun_type = (uint32_t)(item_conf->fun_type);
    CmdProcessorInterface* item_use_processor = item_funcs_.get_iface(fun_type);

    if (item_use_processor == NULL) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_item_unusable);
    }

    return item_use_processor->proc_pkg_from_serv(player, body, bodylen);
}

ItemSmelterCmdProcessor::ItemSmelterCmdProcessor() {
	item_smelter_func_map_.clear();
	item_smelter_func_map_[EQUIP_SMELTER] = equip_smelter;
	item_smelter_func_map_[PET_FRAGMENT_SMELTER] = pet_fragment_smelter; 
}

int ItemSmelterCmdProcessor::proc_pkg_from_client(player_t *player,
		const char *body, int bodylen)
{
    PARSE_MSG;
	std::vector<item_slot_info_t> items;
	const commonproto::item_slot_info_list_t& pb_inf = cli_in_.slot_list();
	for (int i = 0; i < pb_inf.slot_info_size(); ++i) {
		item_slot_info_t  item_info;
		item_info.slot_id = pb_inf.slot_info(i).slot_id();
		item_info.count = pb_inf.slot_info(i).count();
		items.push_back(item_info);
	}
	uint32_t ret = proc_pkg_when_smelter(player, cli_in_.type(), items);
	if (ret) {
		RET_ERR(ret);
	}
    RET_MSG;
}

int ItemSmelterCmdProcessor::proc_pkg_when_smelter(player_t* player, uint32_t type,
		std::vector<item_slot_info_t>& items)
{
	if (item_smelter_func_map_.count(type) == 0) {
		return cli_err_smelter_item_type_err;
	}
	return (item_smelter_func_map_.find(type)->second) (player, items);
}

int ItemSmelterCmdProcessor::equip_smelter(player_t* player,
		std::vector<item_slot_info_t>& items)
{
	//消耗装备增加相应的货币
	FOREACH(items, it) {
		uint32_t normal_currency_cnt = 0, high_currency_cnt = 0;
		uint32_t ret = smelter_equip(player, it->slot_id, it->count,
				normal_currency_cnt, high_currency_cnt);
		if (ret) {
			return ret;
		}
		ADD_A(kAttrSmeltMoney, normal_currency_cnt);
		ADD_A(kAttrSmeltGold, high_currency_cnt);
	}
	return 0;
}

int ItemSmelterCmdProcessor::pet_fragment_smelter(player_t* player,
		std::vector<item_slot_info_t>& items)
{
	FOREACH(items, it) {
		uint32_t fragment_gold = 0;
		uint32_t ret = smelter_pet_fragment(player, it->slot_id,
				it->count, fragment_gold);
		if (ret) {
			return ret;
		}
		ADD_A(kAttrPetFragmentGold, fragment_gold);
	}
	return 0;
}

int ItemReInitEquipAttrCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
	item_t* item = player->package->get_mutable_item_in_slot(cli_in_.slot_id());
	if (item == NULL) {
		RET_ERR(cli_err_item_not_exist);
	}
	commonproto::item_optional_attr_t pb_item_opt;
	pb_item_opt.ParseFromString(item->opt_attr);
	uint32_t need_smelt_money = 0;
	uint32_t need_smelt_gold = 0;
	if (pb_item_opt.level() >= EQUIP_QUALITY_PURPLE) {
		need_smelt_gold = ceil(2 * (pow(1.2, pb_item_opt.level() - EQUIP_QUALITY_PURPLE)));
	}
	int32_t level = pb_item_opt.level();
	need_smelt_money = ceil(10 * pow(2.0, level - 2));
	if (GET_A(kAttrReInitEquipAttrTime) && !(GET_A(kAttrSmeltMoney) >= need_smelt_money && GET_A(kAttrSmeltGold) >= need_smelt_gold)) {
		RET_ERR(cli_err_init_equip_attr_money_not_enough);
	}
	if (GET_A(kAttrReInitEquipAttrTime)) {
		SUB_A(kAttrSmeltMoney, need_smelt_money);
		SUB_A(kAttrSmeltGold, need_smelt_gold);
	}
	if (GET_A(kAttrReInitEquipAttrTime) == 0) {
		SET_A(kAttrReInitEquipAttrTime, NOW());
	}
	init_equip_attr(item);
	std::vector<item_t> chg_vec;
	chg_vec.push_back(*item);
	db_item_change(player, chg_vec, onlineproto::SYNC_REASON_RE_INIT_EQUIP_ATTR, true);
	PlayerUtils::calc_player_battle_value(player);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}
