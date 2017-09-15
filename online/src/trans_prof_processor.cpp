#include "trans_prof_processor.h"
#include "player.h"
#include "trans_prof.h"
#include "global_data.h"
#include "player_utils.h"
#include "item.h"

int TransProfCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
	//获取当前阶段和职业
	uint32_t cur_stage = GET_A(kAttrStage);
	uint32_t cur_prof = GET_A(kAttrCurProf);

	//获取下一阶段进阶需的配表信息
	trans_prof_conf_t trans_prof_conf;
	int ret = g_trans_prof_conf_manager.get_conf_by_prof_and_stage(cur_prof, cur_stage + 1, trans_prof_conf);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}

	//如果当前等级达不到要求，就报错
	uint32_t level = GET_A(kAttrLv);
	if (level < trans_prof_conf.level) {
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_level_too_low);
	}

    //金币是否足够
    if (!AttrUtils::is_player_gold_enough(player, trans_prof_conf.gold)) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_gold_not_enough);
    }

    //扣道具
    std::vector<reduce_item_info_t> reduce_vec;
    FOREACH(trans_prof_conf.consume_item_map, it) {
        reduce_item_info_t reduce;
        reduce.item_id = it->first;
        reduce.count = it->second;
        reduce_vec.push_back(reduce);
    }
    ret = swap_item_by_item_id(player, &reduce_vec, 0);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }
    
    //扣金币
    ret = AttrUtils::sub_player_gold(player, trans_prof_conf.gold, "转职");
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    ADD_A(kAttrStage, 1);
    PlayerUtils::calc_player_battle_value(player);
		
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
