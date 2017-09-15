#include "task_processor.h"
#include "proto/client/cli_errno.h"
#include "player.h"
#include "proto/db/db_cmd.h"
#include "global_data.h"
#include "service.h"
#include "data_proto_utils.h"
#include "task_utils.h"
#include "utils.h"
#include "statlogger/statlogger.h"
#include "item.h"
#include "utils.h"
#include "prize.h"
#include "duplicate_utils.h"

int AcceptTaskCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
    uint32_t task_id = cli_in_.task_id();

	//防沉迷设置
	if(check_player_addicted_threshold_none(player)){
		ERROR_TLOG("%u exceed online time threshold time, can't accept task id:%u", player->userid, task_id);
		return send_err_to_player(player, player->cli_wait_cmd, cli_err_try_to_accept_task_with_addiction);
	}	

    ////  检查接取任务条件
    uint32_t ret = TaskUtils::judge_task_accept_cond_state(player, task_id);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret); 
    }

    // 判断任务在配置文件中
    ret = TaskUtils::judge_task_in_conf(task_id);
    if (ret != 0) {
        return send_err_to_player(player, player->cli_wait_cmd, ret); 
    }
	//读取task的配表信息
    const task_conf_t* task_conf = TaskUtils::get_task_config(task_id);
    if(NULL == task_conf) {
        return send_err_to_player(player, player->cli_wait_cmd, 
                cli_err_task_info_invalid); 
    }

    // 检查任务是否废弃
    if (TaskUtils::judge_task_conf_abandon(task_id)) {
        return send_err_to_player(player, player->cli_wait_cmd, 
                cli_err_task_info_invalid);
    }

    // 检查父任务是否完成
    // TODO toby confirm
    FOREACH(task_conf->parent_list, iter) {
        uint32_t parent_task_id = *iter;
        if (!TaskUtils::is_task_finished(player, parent_task_id)) {
            return send_err_to_player(player, player->cli_wait_cmd, 
                cli_err_task_cannot_accept);
        }
    }

    uint32_t step_count = task_conf->step_count; 
    // 判断接取过任务
    if (player->task_info->is_in_task_list(task_id)) {
        //任务重温：完成的任务则直接接取成功
        if (TaskUtils::is_accept_task_bonus(player, task_id, step_count)) {
            cli_out_.set_task_id(task_id);
            return send_msg_to_player(player, player->cli_wait_cmd, 
                    cli_out_);
        }
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_task_already_accept); 
    }
    // 载入任务信息到内存
    ret = TaskUtils::add_task_to_player(player, task_id);
    if (ret != 0) {
        return send_err_to_player(player, player->cli_wait_cmd, ret); 
    }

    //向db同步新的任务信息
    ret = TaskUtils::db_save_new_task(player, task_id);
    if (ret != 0) {
        return send_err_to_player(player, player->cli_wait_cmd, ret); 
    }
    std::vector<task_t> task_vec;
    player->task_info->get_task(task_id, task_vec);

    for (uint32_t i = 0; i < task_vec.size(); i++) {
        TaskUtils::save_and_syn_task(player, task_vec[i]);
    }

    // 任务统计
    TaskUtils::stat_task_info(player, task_id, commonproto::TASK_OPER_TYPE_ACCEPT);

    //任务接取成功
    cli_out_.set_task_id(task_id);
    return send_msg_to_player(player, player->cli_wait_cmd, 
            cli_out_);

}

int AcceptTaskCmdProcessor::proc_pkg_from_serv(
        player_t* player, const char* body, int bodylen) 
{
    return 0;
}

int AbandonTaskCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{

    PARSE_MSG;
    return send_err_to_player(player, player->cli_wait_cmd, cli_err_task_info_invalid); 

    uint32_t task_id = cli_in_.task_id();
    // 判断任务在配置文件中
    uint32_t ret = TaskUtils::judge_task_in_conf(task_id);
    if (ret != 0) {
        return send_err_to_player(player, player->cli_wait_cmd, ret); 
    }

    // 判断接取过任务
    if (!player->task_info->is_in_task_list(task_id)){
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_task_not_accept); 
    }

    // 判断任务是否已完成
    if (TaskUtils::is_task_finished(player, task_id)) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_task_repeat_reward); 
    }

    // 删除任务
    TaskUtils::del_task_record(player, task_id);

    // 任务统计
    TaskUtils::stat_task_info(
            player, task_id, commonproto::TASK_OPER_TYPE_ABANDON);

    cli_out_.set_task_id(task_id);
    return send_msg_to_player(player, player->cli_wait_cmd, 
            cli_out_);
}

int AbandonTaskCmdProcessor::proc_pkg_from_serv(
        player_t* player, const char* body, int bodylen) 
{
    return 0;
}

int TaskCompleteCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    uint32_t task_id = cli_in_.task_id();

    // 判断任务在配置文件中
    uint32_t ret = TaskUtils::judge_task_in_conf(task_id);
    if (ret != 0) {
        return send_err_to_player(player, player->cli_wait_cmd, ret); 
    }

    // 检查任务配置是否废弃
    if (TaskUtils::judge_task_conf_abandon(task_id)) {
        return send_err_to_player(player, player->cli_wait_cmd, 
                cli_err_task_info_invalid);
    }

    // 判断接取过任务
    if (!player->task_info->is_in_task_list(task_id)){
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_task_not_accept); 
    }

    // 检查是否超时
    if (TaskUtils::is_task_over_time_limit(player, task_id)){
        // 删除超时任务
		TRACE_TLOG("uid=%u,task_time_limit,task_id=[%u]", player->userid,
				task_id);
        TaskUtils::del_task_record(player, task_id);
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_task_over_time_limit); 
    }

    // 判断步数范围
    if (!(cli_in_.step() >= 2 && cli_in_.step() <= 32)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_data_error);
    }

    // 检查任务条件是否达成
    ret = TaskUtils::judge_step_complete_cond_state(player, task_id, 0);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret); 
    }

    // 从配表和内存中读取信息    
    const task_conf_t* task_conf =  g_task_conf_mgr.find_task_conf(task_id);
	std::vector<task_t> task_vec;
	player->task_info->get_task(task_id, task_vec);

    // 检查是否重复领奖
    for (uint32_t i = 0; i < task_vec.size(); i++) {
        if (task_vec[i].bonus_status == kTaskBonusAccept &&
                task_vec[i].done_times >= task_conf->repeate) {
            //已领奖就把步骤都设置完成
            TaskUtils::set_task_finished(player, task_id, false);
            return send_err_to_player(
                    player, player->cli_wait_cmd, cli_err_task_repeat_reward); 
        }
    }

    //  检查是否重复完成任务步骤
    uint32_t cur_step = TaskUtils::get_task_step(player, task_id) + 1;
    if (cur_step >= cli_in_.step()) {
        return send_err_to_player(
                    player, player->cli_wait_cmd, cli_err_task_repeat_reward);
    }

	//更新任务信息到内存和db
    std::map<uint32_t, uint32_t>::const_iterator iter = 
        task_conf->step_jump_map.find(cli_in_.step() - 1);

    // 检查跳步条件
    ret = TaskUtils::judge_step_complete_cond_state(player, task_id, 1);
    if (iter != task_conf->step_jump_map.end() && ret == 0) {
        // 跳过配表指定步骤,统计要分步落
        ret = TaskUtils::update_task_range_step_and_sync_db(
                player, task_vec, iter->first, iter->second, true);
        if (ret != 0) {
            return send_err_to_player(player, player->cli_wait_cmd, ret); 
        }

        // 任务统计
        for (uint32_t step = iter->first; step < iter->second; step++) {
            TaskUtils::stat_task_info(
                    player, task_id, commonproto::TASK_OPER_TYPE_COMPLETE, step);
        }
    } else {
        // 完成单步更新
        ret = TaskUtils::update_task_and_sync_db(player, task_vec, cli_in_.step(), true);
        if (ret != 0) {
            return send_err_to_player(player, player->cli_wait_cmd, ret); 
        }

        // 任务统计
        uint32_t step = TaskUtils::get_task_step(player, task_id);
        TaskUtils::stat_task_info(
                player, task_id, commonproto::TASK_OPER_TYPE_COMPLETE, step);
    }

    if (task_id >= 90000 && task_id <= 100000) {
        ADD_A(kAttrGoldVipActivity150611Tasks, 1);
    }
   
    cli_out_.set_task_id(task_id);    
    return send_msg_to_player(player, player->cli_wait_cmd, 
            cli_out_);
}

int TaskCompleteCmdProcessor::proc_pkg_from_serv(
        player_t* player, const char* body, int bodylen)
{
    return 0;
}

int CompleteEvilKnifeLegendCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    uint32_t dup_ids[] = {
        commonproto::EVIL_KNIFE_LEGEND_DUP_1,
        commonproto::EVIL_KNIFE_LEGEND_DUP_2,
        commonproto::EVIL_KNIFE_LEGEND_DUP_3,
        commonproto::EVIL_KNIFE_LEGEND_DUP_4,
        commonproto::EVIL_KNIFE_LEGEND_DUP_5,
        commonproto::EVIL_KNIFE_LEGEND_DUP_6,
        commonproto::EVIL_KNIFE_LEGEND_DUP_7,
        commonproto::EVIL_KNIFE_LEGEND_DUP_8,
        commonproto::EVIL_KNIFE_LEGEND_DUP_9,
        commonproto::EVIL_KNIFE_LEGEND_DUP_10,
    };
    
    for (uint32_t i = 0; i < array_elem_num(dup_ids);i++) {
        if(!DupUtils::is_duplicate_passed(player, dup_ids[i])) {
		    return send_err_to_player(
                    player, player->cli_wait_cmd, 
                    cli_err_evil_knife_lengend_task_not_finish);
        }
    }

    SET_A(kAttrEvilKnifeLegendRewardTime, NOW());

    cli_out_.Clear();
    return send_msg_to_player(player, player->cli_wait_cmd, 
            cli_out_);
}

