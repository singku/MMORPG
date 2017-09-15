#include "player.h"
#include "tran_card.h"
#include "item.h"
#include "tran_card_conf.h"
#include "tran_card_processor.h"
#include "global_data.h"
#include "task_utils.h"

int UptarLvCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t card_id = cli_in_.card_id();
	if (!player->m_tran_card->is_tran_id_exsit(card_id)) {
		ERROR_TLOG("user_id=[%u],card_id=[%u]", player->userid, card_id);
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_tran_id_not_exsit);
	}
	tran_card_t tran_card_info;
	player->m_tran_card->get_tranCard(card_id, tran_card_info);
	//1先判断升星条件是否达到；等级是否已经满级；
	uint32_t target_start_level = tran_card_info.card_star_level + 1;
	if (target_start_level > KMaxCardStarLv) {
		ERROR_TLOG("id=[%u],cur_lv=[%u]", player->userid, target_start_level);
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_data_error);
	}
	tran_card_conf_t* tran_ptr = g_tran_card_conf_mgr.get_tran_card_conf_info(card_id);
	if (tran_ptr == NULL) {
		ERROR_TLOG("card_id_not_exsit_in_conf;card_id=[%u]", card_id);
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_tran_id_not_exsit_in_conf);
	}
	
	//1判断材料是否足够
	uint32_t evolve_item_id;
	if (!tran_ptr->evolve_items.empty()) {
		evolve_item_id = tran_ptr->evolve_items[0];
	}
	uint32_t consume_item_count = target_start_level * 2;
	uint32_t cost_price = target_start_level * 10000;
	reduce_item_info_t consume_item;
	consume_item.item_id = evolve_item_id;
	consume_item.count = consume_item_count;
	std::vector<reduce_item_info_t> redu_vec;
	redu_vec.push_back(consume_item);
	int ret = check_swap_item_by_item_id(player, &redu_vec, NULL, false);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	if (!AttrUtils::is_player_gold_enough(player, cost_price)) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_gold_not_enough);
    }
	
	//升星引起的技能升级（即技能子id的变化）
	//升星引起的技能开启（也是子id变化）
	
	//目前卡牌的已经开通的技能数量
	uint32_t num = target_start_level / 2;
	if (num  > KMaxTranSkillNum - 1) {
		num = KMaxTranSkillNum - 1;
	}
	if (num + 1 > tran_ptr->skill_ids.size()) {
		ERROR_TLOG("skill_ids,ivec;err;will_atctive_skill_num=[%u],[%zu]", num + 1, tran_ptr->skill_ids.size());
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_active_skill_num_not_match_conf);
	}
	//tran_ptr->skill_ids;
	//已经开启的父技能
	std::vector<uint32_t> new_skill_child_ids;
	for (uint32_t i = 0; i < num + 1; ++i) {
		new_skill_child_ids.push_back(tran_ptr->skill_ids[i] * 10 + num);
	}
	tran_card_info.card_star_level = target_start_level;
	tran_card_info.skill_ids = new_skill_child_ids;
	
	player->m_tran_card->update_tranCard(card_id, tran_card_info);
	//已经开启的父技能ids
	std::vector<tran_card_t> tem_vec;
	tem_vec.push_back(tran_card_info);
	sync_notify_tran_card_info(player, tem_vec, 2);
	//配表从40001开始
	uint32_t ser_card_id = card_id - 40000;
	attr_type_t attr = AttrUtils::get_tran_card_id_attr(ser_card_id);
	uint32_t tmp_card = GET_A(attr);
	//现在卡牌没有获得
	if (!tmp_card) {
		ERROR_TLOG("attr_DB_not_exsit_tran_id:[%u]", tmp_card);
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_attr_tran_id_exp);
	}
	
	//理论上不会执行到该if分支
	if (tmp_card && !g_tran_card_conf_mgr.is_tran_card_conf_exist(tmp_card)) {
		ERROR_TLOG("get_card_id_from_attr_table_err;tem_id=[%u],card_id=[%u]", tmp_card, card_id);
		RET_ERR(cli_err_attr_tran_id_exp);
	}
	SET_A((attr_type_t)(attr + 1), target_start_level);

	cli_out_.set_up_flag(1);
	FOREACH(new_skill_child_ids, it) {
		cli_out_.add_skill_ids(*it);
	}
	//扣材料，扣金币
	swap_item_by_item_id(player, &redu_vec, NULL, false);
	
	AttrUtils::sub_player_gold(player, cost_price, "变身卡升级");
	
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int ChooseRoleCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	uint32_t card_id = cli_in_.card_id();
	if (!player->m_tran_card->is_tran_id_exsit(card_id)) {
		ERROR_TLOG("id=[%u],card_id=[%u]", player->userid, card_id);
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_data_error);
	}
	/*
	if (card_id == GET_A(kTransCardChooseFlag)) {
		ERROR_TLOG("can not choose the card which has been choose");
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_data_error);
	}
	*/
	tran_card_t tran_card_info;
	player->m_tran_card->get_tranCard(card_id, tran_card_info);
	tran_card_info.choose_flag = 1;
	player->m_tran_card->update_tranCard(card_id, tran_card_info);
	cli_out_.set_choose_flag(1);
	//SET_A(kTransCardChooseFlag, card_id);
	SET_A(kAttrTransCardChooseFlag, card_id);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

