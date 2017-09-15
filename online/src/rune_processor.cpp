#include "rune_processor.h"
#include "player.h"
#include "rune.h"
#include "rune_utils.h"
#include "pet_utils.h"
#include "data_proto_utils.h"
#include "shop.h"
#include "item.h"
#include "task_utils.h"
#include "../../proto/client/cli_errno.h"
#include "prize_processor.h"

int UpRuneByExpCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t rune_id = cli_in_.runeid();
	uint32_t exp = cli_in_.exp();
	bool flag = player->rune_meseum->has_rune(rune_id);
	int ret = 0;
	if (!flag) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_rune_has_exist);
	}
	rune_t rune;
	rune_conf_t rune_conf;
	RuneUtils::get_rune(player, rune_id, rune);
	ret = RuneUtils::get_rune_conf_data(rune.conf_id, rune_conf);
	if (ret != 0) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	//满级经验值
	uint32_t top_level_exp;	
	//符文最高等级为6
	ret = RuneUtils::get_rune_exp(rune_conf.rune_type, 5, top_level_exp);
	if (ret != 0) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	//若前端传来的经验值大于经验升级空间,则更新为升级空间值
	if (exp > (top_level_exp - rune.exp)) {
		exp = top_level_exp - rune.exp;
	}
	
	uint32_t bottle_exp = 0;
	uint32_t need_exp = exp;
	ret = RuneUtils::up_rune_grade_by_exp(player, need_exp, rune, bottle_exp);
	if (ret != 0) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	Pet* pet = PetUtils::get_pet_in_loc(player, rune.pet_catch_time, PET_LOC_BAG);
	ret = RuneUtils::alter_rune_info(player, rune);
	if (ret != 0) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	commonproto::rune_data_t*  rune_rata_ptr = cli_out_.mutable_rune_data();
	DataProtoUtils::pack_single_rune_data(rune, rune_rata_ptr);
	cli_out_.set_exp(bottle_exp);
	send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
	//如果符文在精灵身上，刷新精灵属性
	if (pet && rune.pack_type == kPetPack) {
		ret = PetUtils::save_pet(player, *pet, false, true);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret); 
		}
	}
	return 0;
}

int UpRuneByExpCmdProcessor::proc_pkg_from_serv(player_t *player, const char *body, int bodylen)
{
	return 0;
}

int SwallowRuneCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t mov_rune_id = cli_in_.mov_runeid();
	uint32_t stand_rune_id = cli_in_.stand_runeid();
	rune_t mov_rune;
	rune_t stand_rune;
	uint32_t ret = RuneUtils::rune_swallow_get_rune_info(player, stand_rune_id, mov_rune_id, stand_rune, mov_rune);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	//1:判断吞并条件：
	ret = RuneUtils::rune_swallow_check_mov_conditon(stand_rune, mov_rune);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	
	//若被吞的符文在精灵身上，则需要判断吞噬后，是否会导致精灵身上带有相同功能的符文
	Pet* pet = NULL;
	if (stand_rune.pack_type == (uint32_t)kPetPack) {
		pet  = PetUtils::get_pet_in_loc(player, stand_rune.pet_catch_time, PET_LOC_BAG);
		if (pet == NULL) {
			RET_ERR(cli_err_bag_pet_not_exist);
		}
		ret = RuneUtils::check_runes_func_same_or_not(player, mov_rune_id, pet, stand_rune_id);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
	}
	
	ret = RuneUtils::swallow_rule_cal(player, stand_rune, mov_rune);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	
	ret = RuneUtils::alter_rune_info(player, stand_rune);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	ret = RuneUtils::del_rune_info(player, mov_rune.id);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	commonproto::rune_data_t *rune_data_ptr = cli_out_.mutable_rune_data();
	DataProtoUtils::pack_single_rune_data(stand_rune, rune_data_ptr);
	if (pet) {
		PetUtils::save_pet(player, *pet, 0, true);
	}

	cli_out_.set_sec_runeid(mov_rune.id);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);

}

int EquipRuneCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;

    // 更新新手任务
    SET_A(kAttrRookieGuide11RuneGet, 1);

	//uint32_t pet_id = cli_in_.petid();
	uint32_t create_tm = cli_in_.create_tm();
	uint32_t rune_id = cli_in_.runeid();
	uint32_t pos = cli_in_.pos();
	if (!(pos >= RUNE_PET_POS1 && pos <= RUNE_PET_POS4)) {
		return send_err_to_player(
				player, player->cli_wait_cmd, cli_err_data_error);
	}
	bool flag = player->rune_meseum->has_rune(rune_id);
	int ret = 0;
	if (!flag) {
		return send_err_to_player(
				player, player->cli_wait_cmd, cli_err_rune_not_exit);
	}
	rune_t rune;
	RuneUtils::get_rune(player, rune_id, rune);
	//判断该符文是否已经安装在其他精灵身上
	if (rune.grid_id) {
		RET_ERR(cli_err_rune_has_equiped);
	}
	Pet* pet  = PetUtils::get_pet_in_loc(player, create_tm, PET_LOC_BAG);
	if (pet == NULL) {
		RET_ERR(cli_err_bag_pet_not_exist);
	}

	if (pos >= RUNE_PET_POS2 && pos <= RUNE_PET_POS4) {
		if (pet->test_rune_pos_flag(pos - 1) == false) {
			RET_ERR(cli_err_rune_pos_3_still_lock);
		}
	}
		
	/*检查精灵装备的符文条件*/
	--pos;
	ret = RuneUtils::check_equit_rune(player, pet, rune, pos);
	if (ret != 0) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	uint32_t pre_pack_type = rune.pack_type;
	
	rune.pack_type = kPetPack;
	rune.pet_catch_time = create_tm;
	rune.grid_id = pos + 1;

	/*设置精灵符文信息*/
	//若前面代码中判断过get_rune_array!=-1，则在此无需判断pos是否越界情况
	pet->set_rune_array(pos, rune_id);

	/*刷新精灵的属性*/
	PetUtils::save_pet(player, *pet, false, true);
	
	uint32_t ret1 = RuneUtils::alter_rune_info(player, rune);
	if (ret1 != 0) {
		return send_err_to_player(player, player->cli_wait_cmd, ret1);
	}

	cli_out_.set_bagtype(pre_pack_type);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);	
}

int EquipRuneCmdProcessor::proc_pkg_from_serv(player_t *player, const char *body, int bodylen)
{
	return 0;
}

/* @brief 背包之间转换
	type: 1. 收藏背包 -> 转化背包
		  2. 转化背包 -> 收藏背包
 */
int ChgRunePackCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t rune_id = cli_in_.runeid();
	uint32_t type = cli_in_.type();
	if (type != 1 && type != 2) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_data_error);
	}

	/*阻止非法数据*/
	rune_t rune;
	uint32_t ret = RuneUtils::get_rune(player, rune_id, rune);
	if (ret != 0) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	if (type == 1 && rune.pack_type != kCollectPack) {
		ERROR_TLOG("chg rune pack,pack_type not match:id=[%u],pack_type=[%u]", player->userid, rune.pack_type);
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_rune_not_in_collect_pack);
	} else if (type == 2 && rune.pack_type != kTransferPack) {
		ERROR_TLOG("chg rune pack yet,pack_type not match:id=[%u],pack_type=[%u]", player->userid, rune.pack_type);
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_rune_not_in_collect_pack);
	}
	uint32_t packet_type[] = {kTransferPack, kCollectPack};
	
	//判断该类型背包是否已满
	ret = RuneUtils::judge_rune_full_by_packet_type(player, (rune_pack_type_t)packet_type[type - 1]);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	rune.pack_type = packet_type[type - 1];
	ret = RuneUtils::alter_rune_info(player, rune);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	//回包
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int ChgRunePackCmdProcessor::proc_pkg_from_serv(player_t *player, const char *body, int bodylen)
{
	return 0;
}

int RunetoExpBottleCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t rune_id = cli_in_.runeid();
	rune_t rune;
	int ret = RuneUtils::get_rune(player, rune_id, rune);
	if (ret != 0) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	/*阻止非法数据*/
	//只有收藏背包
	if (rune.pack_type != kTransferPack) {
		ERROR_TLOG("rune to exp exp err,pack_type not match:id=[%u],pack_type=[%u]", player->userid, rune.pack_type);
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_data_error);
	}

	/*更新经验瓶中经验*/
	uint32_t bottle_exp = GET_A(kAttrRuneExpBottle);
	rune_conf_t rune_conf;
	RuneUtils::get_rune_conf_data(rune.conf_id, rune_conf);	
	bottle_exp = bottle_exp + rune.exp + rune_conf.tran_add_exp;
	SET_A(kAttrRuneExpBottle, bottle_exp);

	ret = RuneUtils::del_rune_info(player, rune_id);
	if (ret != 0) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	//回包
	cli_out_.set_bottleexp(bottle_exp);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int RunetoExpBottleCmdProcessor::proc_pkg_from_serv(player_t *player, const char *body, int bodylen)
{
	return 0;
}


int RuneCallCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t level = cli_in_.level();
	if (level < 0 || level > KMaxCallLevel) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_rune_call_level_invalid);
	}

    //判断召唤的条件
	uint32_t ret = RuneUtils::check_rune_call_condition(player, level);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	uint32_t conf_id;
	ret = RuneUtils::cal_rune_conf_id_on_calling(player, level, conf_id);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	if (conf_id == RUNE_FRAGMENT_CONF_ID) {
        AttrUtils::add_player_gold(player, KGrayRunePrice, false, "符文召唤金币碎片");
	} else {
		//ret = RuneUtils::create_rune_info(player, conf_id);
		ret = RuneUtils::add_rune(player, conf_id);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
		//活动范围内若是召唤获得到紫色符文 则 加1
		// rune_conf_t rune_conf;
		// RuneUtils::get_rune_conf_data(conf_id, rune_conf);
		// if (rune_conf.rune_type == kRunePurple) {
			// AttrUtils::add_attr_in_special_time_range(player,
					// TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
					// kAttrActivGetPurpleRuneCnt);
		// }
	}

	uint32_t callrate_value = 0, price = 0;
	g_rune_rate_conf_mgr.get_call_info_by_level(level, callrate_value, price);
	AttrUtils::sub_player_gold(player, price, "符文召唤第" + Utils::to_string(level) + "级");

	//更新召唤阵，即使更新失败，也无需返回错误码
	RuneUtils::update_call_level_vec(player, level);

    // 更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_RUNE_CALL, 1);

	//给客户端回包
	rune_t rune = {0};
	cli_out_.Clear();
	commonproto::rune_data_t* rune_data_ptr = cli_out_.mutable_rune_data();
	DataProtoUtils::pack_single_rune_data(rune, rune_data_ptr);

	//保存召唤阵level信息至属性表（即持久化到DB）
	player->rune_meseum->save_call_level_set(player);
	std::set<uint32_t> call_level_list_set;
	player->rune_meseum->get_call_level_list_from_set(call_level_list_set);

	if (call_level_list_set.empty()) {
		cli_out_.add_call_level_list(0);
	} else {
		FOREACH(call_level_list_set, it) {
			cli_out_.add_call_level_list(*it);
		}
	}
	cli_out_.set_price(KGrayRunePrice);
	if (conf_id == RUNE_FRAGMENT_CONF_ID) { 
		cli_out_.set_rune_id(0);
	} else {
		cli_out_.set_rune_id(GET_A(kAttrRuneId));
	}
	cli_out_.set_level(level);
	
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int GetRuneFromRpToTpCmdProcessor:: proc_pkg_from_client(player_t *player, const char *body, int bodylen) {
	PARSE_MSG;
	/*检查转换背包中格子数量是否已满*/
	uint32_t ret = RuneUtils::judge_rune_full_by_packet_type(player, kTransferPack);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	uint32_t used_count, rune_count_limit;
	used_count = player->rune_meseum->get_rune_num_by_packet_type(kTransferPack);
	rune_count_limit = RuneUtils::get_rune_limit_count_by_pack_type(player, kTransferPack);
	uint32_t can_use_count = rune_count_limit - used_count;
	/*将符文馆中的符文放进转换背包*/
	std::vector<uint32_t> tmp_rune_id;
	std::vector<rune_cli_t> rune_cli_vec;
	RuneMeseum::RuneMap rune_map;
	player->rune_meseum->get_rune_map_info(rune_map);
	FOREACH(rune_map, it) {
		if (it->second.pack_type == kRunePack) {
			rune_conf_t* rune_conf_ptr = g_rune_conf_mgr.get_rune_conf_t_info(it->second.conf_id);
			if (rune_conf_ptr == NULL) {
				ERROR_TLOG("conf id err:[%u]", it->second.conf_id);
				return send_err_to_player(player, player->cli_wait_cmd, cli_err_data_error);
			}
			if (rune_conf_ptr->rune_type != kRuneGray && rune_conf_ptr->rune_type != kRuneRed) {
				if (0 == can_use_count) {
					break;
				}
				it->second.pack_type = kTransferPack;
				player->rune_meseum->update_rune(it->second);
				RuneUtils::db_save_rune(player, it->second);
				tmp_rune_id.push_back(it->second.id);	
				rune_cli_t tem_vec;
				tem_vec.rune = it->second;
				tem_vec.flag = 1;
				rune_cli_vec.push_back(tem_vec);
				--can_use_count;

				//活动范围内若是召唤获得到紫色符文 则 加1
				rune_conf_t rune_conf;
				RuneUtils::get_rune_conf_data(it->second.conf_id, rune_conf);
				if (rune_conf.rune_type == kRunePurple) {
					AttrUtils::add_attr_in_special_time_range(player,
							TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
							kAttrActivGetPurpleRuneCnt);
				}
			}
			///////////////////////////////////
			/////处理脏数据（将没卖出的碎片卖出）
			if (rune_conf_ptr->rune_type == kRuneGray) {
				uint32_t del_rune_id = it->second.id;
				rune_cli_t tem_vec;
				tem_vec.rune = it->second;
				tem_vec.flag = 0;
				rune_cli_vec.push_back(tem_vec);
				ret = player->rune_meseum->del_rune(del_rune_id);
				if (ret) {
					return ret;
				}
				ret = RuneUtils::db_del_rune(player, del_rune_id);
				if (ret) {
					return ret;
				}
                AttrUtils::add_player_gold(player, KGrayRunePrice, false, "符文召唤金币碎片");
			}
			///////////////////////////////////
			/////处理脏数据（将没卖出的碎片卖出）
		}
	}
	RuneUtils::sync_notify_rune_data_info(player, rune_cli_vec);

	uint32_t tmp_runeid_size = tmp_rune_id.size();
	for (uint32_t i = 0; i < tmp_runeid_size; ++i) {
		cli_out_.add_rune_id_list(tmp_rune_id[i]);
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int SellGrayRuneCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen) {
	PARSE_MSG;
	RuneMeseum::RuneMap tem_rune_map;
	player->rune_meseum->get_rune_map_info(tem_rune_map);
	uint32_t gray_rune_count = 0;
	uint32_t gray_rune_cost = 0;
	FOREACH(tem_rune_map, it) {
		rune_conf_t* rune_conf_ptr = g_rune_conf_mgr.get_rune_conf_t_info(it->second.conf_id);
		if (rune_conf_ptr == NULL) {
			ERROR_TLOG("conf id err:[%u]", it->second.conf_id);
			continue;
		}
		if (it->second.pack_type == kRunePack && rune_conf_ptr->rune_type == kRuneGray) {
			++gray_rune_count;
			gray_rune_cost = rune_conf_ptr->fun_type;
			player->rune_meseum->del_rune(it->second.id);
			RuneUtils::db_del_rune(player, it->second.id);
		}
	}
	//to do 根据gray_rune_count数量获得金钱 
	uint32_t money = gray_rune_count * gray_rune_cost;
	cli_out_.set_money(money);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

//一键转化到经验瓶中
int OneKeyToBottleCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen) {
	PARSE_MSG;
	uint32_t rune_quality = cli_in_.rune_quality();
	if (rune_quality > kRuneOrange) {
		RET_ERR(cli_err_rune_quality_err);
	}
	RuneMeseum::RuneMap tem_rune_map;
	player->rune_meseum->get_rune_map_info(tem_rune_map);
	uint32_t add_exp = 0;
	std::vector<rune_cli_t> rune_cli_vec;
	FOREACH(tem_rune_map, it) {
		rune_conf_t rune_conf;
		if (RuneUtils::get_rune_conf_data(it->second.conf_id, rune_conf)) {
			ERROR_TLOG("conf_id_err;[%u]", it->second.conf_id);
			continue;
		}
		TRACE_TLOG("zjun0519,pack_type,=%u", it->second.pack_type);
		bool reach_condtion = false;
		if (cli_in_.bag_type() == onlineproto::RUNE_BAG 
				&& it->second.pack_type == kTransferPack
				&& rune_conf.rune_type <= rune_quality) {
			reach_condtion = true;
		} else if (cli_in_.bag_type() == onlineproto::RUNE_MUSEUM
				&& it->second.pack_type == kRunePack 
				&& rune_conf.rune_type <= rune_quality) {
			reach_condtion = true;
		}
		if (reach_condtion == false) {
			continue;
		} 
		add_exp = it->second.exp + rune_conf.tran_add_exp + add_exp;
		player->rune_meseum->del_rune(it->second.id);
		RuneUtils::db_del_rune(player, it->second.id);
		rune_cli_t rune_cli;
		rune_cli.rune = it->second;
		rune_cli.flag = 0;
		rune_cli_vec.push_back(rune_cli);
	}
	uint32_t bottle_exp = GET_A(kAttrRuneExpBottle);
	bottle_exp += add_exp;
	SET_A(kAttrRuneExpBottle, bottle_exp);
	cli_out_.set_bottleexp(bottle_exp);
	RuneUtils::sync_notify_rune_data_info(player, rune_cli_vec);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int UnlockPetRunePosCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen) {
	PARSE_MSG;
	uint32_t pos = cli_in_.pos(); //pos 传2，3，4；
	if (!(pos >= RUNE_PET_POS2 && pos <= RUNE_PET_POS4)) {
		return send_err_to_player(
				player, player->cli_wait_cmd, cli_err_data_error);
	}

	std::vector<reduce_item_info_t> reduce_vec;
	reduce_item_info_t reduce;
	reduce.item_id = 38000;
	reduce.count = 1;
	reduce_vec.push_back(reduce);
	uint32_t ret = check_swap_item_by_item_id(player, &reduce_vec, 0, false);
	if (ret) {
		ERROR_TLOG("item count not enouth:item_id=[%u],item_cout=[%u]", reduce.item_id, reduce.count);
		return  send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm(), PET_LOC_BAG);
	if (!pet) {
		return  send_err_to_player(player, player->cli_wait_cmd, cli_err_bag_pet_not_exist);
	}

	uint32_t need_level[4] = {RUNE_EQUIP_POS1_NEED_LV, RUNE_EQUIP_POS2_NEED_LV, 
		RUNE_EQUIP_POS3_NEED_LV, RUNE_EQUIP_POS4_NEED_LV};
	if (pet->level() < need_level[pos - 1]) {
		RET_ERR(cli_err_pet_lv_can_unlock_this_pos);
	}

	if (pet->test_rune_pos_flag(pos - 1)) {
		RET_ERR(cli_err_rune_pos_has_open);
	}
	pet->unlock_rune_pos(pos - 1);
	PetUtils::save_pet(player, *pet, false, true);
	
	swap_item_by_item_id(player, &reduce_vec, 0, false);
	cli_out_.set_flag(1);	
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int UnlockRuneCollectBagCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen) {
	PARSE_MSG;
	//MAX_UNLOCK_NUM暂定为7 ,若更改，注意检查，避免数组下标越界的情况
	const uint32_t MAX_UNLOCK_NUM = 7;
	uint32_t open_num = GET_A(kAttrOpenRunePosNum);
	if (open_num >= MAX_UNLOCK_NUM) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_collect_pos_all_unlock);
	}
	
	const uint32_t consume_price[] = {100, 200, 300, 400, 500, 600, 700};
	if(!AttrUtils::is_player_gold_enough(player, consume_price[open_num])) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_gold_not_enough);
    }
	
	uint32_t new_open_num = open_num + 1;
	SET_A(kAttrOpenRunePosNum, new_open_num);
	SET_A(kAttrLockRunePosNum, MAX_UNLOCK_NUM - new_open_num);
	AttrUtils::sub_player_gold(player, consume_price[open_num], "符文收藏包解锁");
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int BuyRuneTransPackPageCmdProcessor::proc_pkg_from_client(player_t* player,
		const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t has_buy_page = GET_A(kAttrBuyRuneTransPackPage);
	if (has_buy_page >= AttrUtils::get_attr_max_limit(player, kAttrBuyRuneTransPackPage)) {
		RET_ERR(cli_err_rune_transpack_page_has_get_limit);
	}
	//确定买几份 has_buy_page 即为需要买的份数	
	uint32_t shop_id = 52316;
	const product_t *pd = g_product_mgr.find_product(shop_id);
	if (pd == NULL) {
		RET_ERR(cli_err_product_not_exist);
	}
	int ret = buy_attr_and_use(player, (attr_type_t)pd->service,
			shop_id, has_buy_page + 1);
	if (ret) {
		RET_ERR(ret);
	}
	ADD_A(kAttrBuyRuneTransPackPage, 1);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
