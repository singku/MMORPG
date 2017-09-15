#ifndef TASK_UTILS_H
#define TASK_UTILS_H

#include <stdint.h>
#include <libtaomee/project/types.h>
#include "proto/client/common.pb.h"
#include "player.h"
#include "task_info.h"
#include "duplicate_conf.h"

enum stat_story_task_status_t {
    STAT_STORY_TASK_STATUS_ENTER = 1,
    STAT_STORY_TASK_STATUS_PASS = 2,
    STAT_STORY_TASK_STATUS_BOSS = 3,
};

enum task_common_data_t {
	TASK_TYPE_BASE = 10000,
};

enum task_type_t {
	TASK_FOR_ACHIEVE = 9,
};

class TaskUtils
{
public:


    /*----------------------------db操作------------------------------------*/
    //增加新任务到db中
    static uint32_t db_save_new_task(player_t* player, uint32_t task_id);

    //db更新任务信息 
    static uint32_t db_update_task(player_t* player, const task_t& task_data);

    //从db中删除任务
    static uint32_t db_del_task(player_t* player, uint32_t task_id);


    /*----------------------------player操作------------------------------------*/
    //增加新任务到player中
    static uint32_t add_task_to_player(player_t* player, uint32_t task_id);

    //从player中删除任务
    static uint32_t del_task_from_player(player_t* player, uint32_t task_id);

    // 删除任务记录，包括内存和db
    static int del_task_record(player_t* player, uint32_t task_id);

    //更新内存task
    static uint32_t update_task_to_player(player_t* player, const task_t& task_data);

    // 完成指定步骤任务
    static uint32_t update_task_and_sync_db(
            player_t* player, std::vector<task_t>& task_vec, 
            uint32_t step, bool reward_flag = false);

    // 完成指定范围的任务步骤
    static uint32_t update_task_range_step_and_sync_db(
        player_t* player, std::vector<task_t>& task_vec , 
        uint32_t start_step, uint32_t end_step, bool reward_flag = false);

    /*----------------------------check------------------------------------*/
    //判断任务id在配置文件中
    static uint32_t judge_task_in_conf(uint32_t task_id);

    // 判断任务配置是否废弃已废弃
    static bool judge_task_conf_abandon(uint32_t task_id);

    // 任务是否超时
    static bool is_task_over_time_limit(player_t *player, uint32_t task_id);

    // 任务是否接取
    static bool is_task_accept(player_t *player, uint32_t task_id);

    // 任务是否完成
    static bool is_task_finished(player_t *player, uint32_t task_id);

    // 在指定的任务ID段中完成了多少任务
    static uint32_t total_fini_task(player_t *player, uint32_t start_task_id, uint32_t end_task_id);

    // 获取任务进度，当前是第几步
    static uint32_t get_task_step(player_t *player, uint32_t task_id);

    //判断设置任务完成状态条件
    static uint32_t check_set_task_finish(player_t* player, uint32_t task_id);


    //判断任务奖励领取
    static bool is_accept_task_bonus(player_t* player, uint32_t task_id, uint32_t step_count);

    //判断任务接受条件
    static uint32_t judge_task_accept_cond_state(player_t* player, uint32_t task_id);

    static uint32_t judge_step_complete_cond_state(
            player_t* player, uint32_t task_id, uint32_t step_gap);

    //同步客户端,保存当前条件任务信息
    static uint32_t save_and_syn_task(player_t* player, const task_t &task_data);

	static uint32_t syn_task(player_t* player, const commonproto::task_data_t& task_info);

    static const task_conf_t* get_task_config(uint32_t taskid);

	//副本监听
	static uint32_t listen_duplicate_complete(player_t* player, uint32_t dup_id);

    // 更新属性条件
    static uint32_t update_task_condition_attr(player_t* player, uint32_t task_id, uint32_t value);

    // 更新TASK_COND_ATTR 类型特定步骤的任务关联属性值,不修改任务状态，可以用来完成未接取的任务
    static uint32_t update_task_step_condition_attr(
        player_t* player, uint32_t task_id, uint32_t value, uint32_t step);

	//任务条件
    static bool condition_fun(player_t* player, condition_conf_t condition_conf, uint32_t task_id, int attr);
    static bool condition_fun_1(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_2(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_3(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_5(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_6(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
	static bool condition_fun_8(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_9(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_10(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_11(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_13(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_14(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_15(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_17(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_18(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_19(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_20(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_21(player_t* player, condition_conf_t condition_conf, uint32_t task_id);
    static bool condition_fun_22(player_t* player, condition_conf_t condition_conf, uint32_t task_id);

    // 悬赏任务相关
    // 是否悬赏任务
    static bool is_reward_task(uint32_t task_id);
    // 判断task_id是否reward_task, 并取出对应reward_task的等级
    static uint32_t get_reward_task_level(uint32_t task_id);
    // 重置每日悬赏任务
    static int reset_daily_reward_task(player_t *player);
    // 重置有时间限制任务
    static int reset_time_limit_task(player_t *player);

    static int update_daily_reward_task_duplicate_step(
        player_t *player, const duplicate_t *dup, uint32_t value);
    static int update_reward_task_item_step(
        player_t *player, const std::vector<add_item_info_t> *add_items);

    static uint32_t update_task_condition_server(
        player_t* player, uint32_t task_id, 
        uint32_t task_step, uint32_t cond_id);

    // 新手任务
    static int update_new_player_task_dup_step(
        player_t *player, uint32_t dup_id, bool win, uint32_t type = 0);

    static int stat_task_info(
            player_t *player, uint32_t task_id, uint32_t oper_type, uint32_t step = 0);
    static int stat_dup_story_task(
        player_t *player, uint32_t dup_id, uint32_t boss_id, uint32_t dup_status);

    static int set_task_finished(player_t *player, uint32_t task_id, bool reward_flag = true);

    // 任务配表修改处理
    static int task_update_to_config(player_t *player);
};

#endif
