#include<algorithm>

#include "task_utils.h"
#include "player.h"
#include "service.h"
#include "proto/db/dbproto.task.pb.h"
#include "proto/db/db_cmd.h"
#include "proto/client/cli_cmd.h"
#include "global_data.h"
#include "data_proto_utils.h"
#include "proto/client/cli_errno.h"
#include "proto/client/pb0x04.pb.h"
#include "attr_utils.h"
#include "pet_utils.h"
#include "utils.h"
#include "statlogger/statlogger.h"
#include "player_utils.h"
#include "prize.h"
#include "rune.h"
#include "rune_utils.h"
#include "rank_utils.h"
#include "task_info.h"

#include <stack>

extern "C" {
#include <libtaomee/timer.h>
}

uint32_t TaskUtils::judge_task_in_conf(uint32_t task_id)
{
    if (!g_task_conf_mgr.is_task_conf_exist(task_id)) {
        return cli_err_task_info_invalid;
    }
    return 0;
}

bool TaskUtils::judge_task_conf_abandon(uint32_t task_id)
{
    return g_task_conf_mgr.is_task_conf_abandon(task_id);
}

uint32_t TaskUtils::judge_task_accept_cond_state(player_t* player, uint32_t task_id)
{
    int ret = 0;
    
	//判断任务是否存在
    if(!g_task_conf_mgr.is_task_conf_exist(task_id)) {
        return cli_err_task_info_invalid;
    }
    //如果没有配置接去任务条件，则表示可以直接接任务
    if(0 == (g_task_conf_mgr.find_task_conf(task_id))->accept_con_vec.size()) {
        return 0;
    }

	//读取任务配表信息
    task_conf_t task_conf = *(g_task_conf_mgr.find_task_conf(task_id));
    std::vector<std::string> ret_vec(task_conf.accept_con_exp_vec.begin(),
			task_conf.accept_con_exp_vec.end());
    
    for(uint32_t i = 0; i < (g_task_conf_mgr.find_task_conf(task_id))->accept_con_vec.size(); i++) {                
        uint32_t cond_id = (g_task_conf_mgr.find_task_conf(task_id))->accept_con_vec[i].id;
        uint32_t type = (g_task_conf_mgr.find_task_conf(task_id))->accept_con_vec[i].type;
        std::stringstream ss;
        ss << cond_id;
        if(g_condition_fun.count(type)) {
			bool fun_ret = g_condition_fun[type](player, task_conf.accept_con_vec[i], task_id);
            std::stringstream ss_fun_ret;
            ss_fun_ret << fun_ret;
            replace_if(ret_vec.begin(), ret_vec.end(), bind2nd(equal_to<std::string>(), ss.str()), ss_fun_ret.str());
		}
	}
            //process ret_vec...
	//解析表达式过程 详情咨询 chrisli
	std::stack<std::string> ret_sta;
	FOREACH(ret_vec, it) {
		if(!(*it == "A" || *it == "O" || *it == "(" || *it == ")")) {
			if(!ret_sta.empty()) {
				std::string opSym = ret_sta.top();
				ret_sta.pop();
				
				if(opSym == "A") {
					std::string op1 = ret_sta.top();
					ret_sta.pop();
					int iop2 = atoi(it->c_str());

					int iop1 = atoi(op1.c_str());
					int opret = iop1 && iop2;

					stringstream opret_ss;
					opret_ss << opret;
					ret_sta.push(opret_ss.str());
					
				} else if(opSym == "O") {
					std::string op1 = ret_sta.top();
					ret_sta.pop();
					int iop2 = atoi(it->c_str());

					int iop1 = atoi(op1.c_str());
					int opret = iop1 || iop2;

					stringstream opret_ss;
					opret_ss << opret;
					ret_sta.push(opret_ss.str());
					
				} else {
					ret_sta.push(*it);
				}
			} else {
	            ret_sta.push(*it);
            }
		} else if(*it == "A" || *it == "O" || *it == "(") {
			ret_sta.push(*it);
		} else if(*it == ")") {
			std::string op1 = ret_sta.top();
			ret_sta.pop();                                        
			int iop1 = atoi(op1.c_str());
			ret_sta.pop(); //left_bracket
			if(ret_sta.size() > 0) {
				std::string opSym = ret_sta.top();
				ret_sta.pop();
				
				if(opSym == "A") {
					std::string op2 = ret_sta.top();
					ret_sta.pop();                                        
					int iop2 = atoi(op2.c_str());                    
					int opret = iop1 && iop2;
					stringstream opret_ss;
					opret_ss << opret;
					ret_sta.push(opret_ss.str());
					
				} else if(opSym == "O") {
					std::string op2 = ret_sta.top();
					ret_sta.pop();                                        
					int iop2 = atoi(op2.c_str());                    
					int opret = iop1 && iop2;
					stringstream opret_ss;
					opret_ss << opret;
					ret_sta.push(opret_ss.str());
				} else {
					ret_sta.push(*it);
				}
			} else {
				ret_sta.push(op1);
			}
		}
	}
	//解析成功，读取结果
	if(ret_sta.size() == 1) {
        uint32_t result = 0;
		std::string str_ret = ret_sta.top();
		ret_sta.pop();
		result = atoi(str_ret.c_str());

        if (result == 0) {
            ret = cli_err_task_cannot_accept;
        } 
	}
    return ret;
}

uint32_t TaskUtils::judge_step_complete_cond_state(
        player_t* player, uint32_t task_id, uint32_t step_gap)
{
    int ret = 0;

    if(!g_task_conf_mgr.is_task_conf_exist(task_id)) {
        return cli_err_task_info_invalid;
    }

    task_conf_t task_conf = *(g_task_conf_mgr.find_task_conf(task_id));
    std::vector<task_t> task_list;
	player->task_info->get_task(task_id, task_list);
	if (task_list.size() == 0) {
		return cli_err_task_not_accept;
	}
	task_t task_info = task_list[0];
		
	//找到当前任务在第几步(已完成任务的下一步)
	uint32_t step_count = TaskUtils::get_task_step(player, task_id) + 1 + step_gap;

    // 任务已完成
    if (step_count > task_conf.step_count) {
        return 0;
    }

	//取出当前步的信息
	step_conf_t *step_conf = &(task_conf.step_conf_vec[step_count - 1]);
	if (step_conf == NULL){
		return cli_err_task_info_invalid;
    }

    std::vector<std::string> ret_vec(
            step_conf->complete_con_exp_vec.begin(), step_conf->complete_con_exp_vec.end());

	bool match_achieve_multi_cond = true;
    for(uint32_t i = 0; i < step_conf->complete_con_vec.size(); i++) {
        uint32_t type = step_conf->complete_con_vec[i].type;
        uint32_t id = step_conf->complete_con_vec[i].id;
        std::stringstream ss;
        ss << id;
        
        if(g_condition_fun.count(type)) {
            bool fun_ret = g_condition_fun[type](player, step_conf->complete_con_vec[i], task_id);

			if (fun_ret == false && step_conf->complete_con_vec.size() > 1 &&
					((task_id >= 90001 && task_id <= 99999) || task_conf.multi_condition)) {
				match_achieve_multi_cond = false;
			}

            std::stringstream ss_fun_ret;
            ss_fun_ret << fun_ret;
                
            replace_if(ret_vec.begin(), ret_vec.end(), bind2nd(equal_to<std::string>(), ss.str()), ss_fun_ret.str());
		}
	}
	//Confirm kevin : task_id属于成就且多条件，在此判断并return 结果
	if (((task_id >= 90001 && task_id <= 99999) || task_conf.multi_condition) && step_conf->complete_con_vec.size() > 1) {
		return  match_achieve_multi_cond? 0 : cli_err_task_not_complete;
	}

	//解析表达式过程 详情咨询 chrisli
	std::stack<std::string> ret_sta;
	FOREACH(ret_vec, it) {
		if(!(*it == "A" || *it == "O" || *it == "(" || *it == ")")) {
			if(!ret_sta.empty()) {
				std::string opSym = ret_sta.top();
				ret_sta.pop();
				
				if(opSym == "A") {
					std::string op1 = ret_sta.top();
					ret_sta.pop();
					int iop2 = atoi(it->c_str());

					int iop1 = atoi(op1.c_str());
					int opret = iop1 && iop2;

					stringstream opret_ss;
					opret_ss << opret;
					ret_sta.push(opret_ss.str());
				} else if(opSym == "O") {
					std::string op1 = ret_sta.top();
					ret_sta.pop();
					int iop2 = atoi(it->c_str());

					int iop1 = atoi(op1.c_str());
					int opret = iop1 || iop2;

					stringstream opret_ss;
					opret_ss << opret;
					ret_sta.push(opret_ss.str());
					
					
				} else {
					ret_sta.push(*it);
				}
			} else {
                ret_sta.push(*it);
            }
		} else if(*it == "A" || *it == "O" || *it == "(") {
			ret_sta.push(*it);
		} else if(*it == ")") {
			std::string op1 = ret_sta.top();
			ret_sta.pop();                                        
			int iop1 = atoi(op1.c_str());
			
			ret_sta.pop(); //left_bracket
			
			if(ret_sta.size() > 0) {

				std::string opSym = ret_sta.top();
				ret_sta.pop();
				
				if(opSym == "A") {
					std::string op2 = ret_sta.top();
					ret_sta.pop();                                        
					int iop2 = atoi(op2.c_str());                    
					int opret = iop1 && iop2;

					stringstream opret_ss;
					opret_ss << opret;
					ret_sta.push(opret_ss.str());
					
				} else if(opSym == "O") {
					std::string op2 = ret_sta.top();
					ret_sta.pop();                                        
					int iop2 = atoi(op2.c_str());                    
					int opret = iop1 && iop2;

					stringstream opret_ss;
					opret_ss << opret;
					ret_sta.push(opret_ss.str());
												
				} else {
					ret_sta.push(*it);
				}
			} else {
				ret_sta.push(op1);
			}
		}
    }
	//解析成功，读取结果
	if(ret_sta.size() == 1) {
        uint32_t result = 0;
		std::string str_ret = ret_sta.top();
		ret_sta.pop();
		result = atoi(str_ret.c_str());

        if (result == 0) {
            ret = cli_err_task_not_complete;
        } 
	}
    return ret;
}
/** 
 * @brief 取已完成的任务步数
 * 
 * @param player 
 * @param task_id 
 * 
 * @return 
 */
uint32_t TaskUtils::get_task_step(player_t *player, uint32_t task_id)
{
    uint32_t step = 0;
    std::vector<task_t> task_list;
	player->task_info->get_task(task_id, task_list);
	if (task_list.size() == 0) {
		return step;
	}
	task_t task_info = task_list[0];
	uint32_t status = task_info.status;

	for (step = 2; step <= 32 ; step++) {
        if (!(taomee::test_bit_on(status, step) == 1)) {
            break;
        }
	}

    return (step - 2);
}

bool TaskUtils::is_task_over_time_limit(player_t *player, uint32_t task_id)
{
    if (player == NULL) {
        return false;
    }
    const task_conf_t *task_conf = g_task_conf_mgr.find_task_conf(task_id);
    if (task_conf == NULL) {
        return false;
    }


    if (task_conf->end_time > 0  && task_conf->end_time < NOW()) {
        return true;
    }

    return false;
}

bool TaskUtils::is_task_finished(player_t *player, uint32_t task_id)
{
    if (player == NULL) {
        return false;
    }
    const task_conf_t *task_conf = g_task_conf_mgr.find_task_conf(task_id);
    if (task_conf == NULL) {
        return false;
    }

    std::vector<task_t> task_list;
	player->task_info->get_task(task_id, task_list);
	if (task_list.size() == 0) {
		return false;
	}
	task_t task_info = task_list[0];
	uint32_t status = task_info.status;

    if (taomee::test_bit_on(status, task_conf->step_count + 1)) {
        return true;
    }

    return false;
}

uint32_t TaskUtils::total_fini_task(player_t *player, uint32_t start_task_id, uint32_t end_task_id)
{
    uint32_t sum = 0;
    for (uint32_t i = start_task_id; i <= end_task_id; i++) {
        if (TaskUtils::is_task_finished(player, i)) {
            sum++;
        }
    }
    return sum;
}

bool TaskUtils::is_task_accept(player_t *player, uint32_t task_id)
{
    bool flag = false;
     if (player->task_info->is_in_task_list(task_id)){
        flag = true; 
    }

    return flag;
}

//将配表中的信息读入内存
uint32_t  TaskUtils::add_task_to_player(player_t* player, uint32_t task_id)
{        
    const task_conf_t* task_conf = TaskUtils::get_task_config(task_id);
    task_t task_data;
    task_data.task_id = task_id;
    task_data.status = 1;
    task_data.done_times = 0;
	task_data.bonus_status = kTaskBonusUnAccept;
	for (uint32_t i = 0; i < task_conf->step_count; i++) {
		step_t step_info;
		step_info.step = 1;
		step_info.condition_status = 0;
		task_data.step_list.push_back(step_info);
	}
    player->task_info->put_task(task_data);

    return 0;
}

int TaskUtils::del_task_record(player_t *player, uint32_t task_id)
{
    TaskUtils::del_task_from_player(player, task_id);
    TaskUtils::db_del_task(player, task_id);
    return 0;
}

//从内存中删除任务信息，并同步给客户端
uint32_t TaskUtils::del_task_from_player(player_t* player, uint32_t task_id)
{
    std::vector<task_t> task_vec;
    player->task_info->get_task(task_id, task_vec);
    if (task_vec.empty()) {
        return 0;
    }
    
    player->task_info->del_task(task_id);
    task_vec[0].status = 0;
    task_vec[0].done_times = 0;
    task_vec[0].bonus_status = kTaskBonusUnAccept;
    onlineproto::sc_0x0404_sync_task sc_sync_task;
	commonproto::task_data_t* each_task_info = sc_sync_task.add_task_list();

    if(task_vec.size() == 1) {
        uint32_t ret = DataProtoUtils::pack_task_data(&(task_vec[0]), each_task_info);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret); 
		}
    }
    return  send_msg_to_player(player, cli_cmd_cs_0x0404_sync_task, sc_sync_task);
}

//发协议给db删除任务
uint32_t TaskUtils::db_del_task(player_t* player, uint32_t task_id)
{
    dbproto::cs_task_del db_cs_task_del;
    db_cs_task_del.set_task_id(task_id);

    return g_dbproxy->send_msg(NULL, player->userid, 
			player->create_tm,
            db_cmd_task_del, db_cs_task_del);
}


uint32_t TaskUtils::update_task_to_player(player_t* player, const task_t& task_data) 
{
    player->task_info->set_task(task_data);
	
    return 0;
}

uint32_t TaskUtils::db_save_new_task(player_t* player, uint32_t task_id)
{
    dbproto::cs_task_save db_cs_task_save;
	//从配表中读取任务信息
    task_conf_t* task_conf = g_task_conf_mgr.find_task_conf(task_id);
	if (task_conf == NULL)
		return cli_err_task_info_invalid;

	//从内存中读取任务信息
	std::vector<task_t> task_vec;
	player->task_info->get_task(task_id, task_vec);
	if (task_vec.size() == 0) {
		return cli_err_task_info_invalid;
	}
	//打包协议，并将协议发给db来存新的任务
	commonproto::task_data_t* task_info = db_cs_task_save.mutable_task_list()->add_task_list();
	task_info->set_task_id(task_vec[0].task_id);
	task_info->set_status(task_vec[0].status);
	task_info->set_done_times(task_vec[0].done_times);
	task_info->set_bonus_status(task_vec[0].bonus_status);
	task_info->set_fin_statue_tm(task_vec[0].fin_statue_tm);

	for (uint32_t j = 0; j < task_conf->step_count; j++) {
		task_info->add_cond_status(task_vec[0].step_list[j].condition_status);		
	}

    return g_dbproxy->send_msg(NULL, player->userid, 
			player->create_tm,
            db_cmd_task_save, db_cs_task_save);
}

//打包任务信息并发给db更新
uint32_t TaskUtils::db_update_task(player_t* player, const task_t& task)
{
    dbproto::cs_task_save db_cs_task_save;

    commonproto::task_data_t* task_info = db_cs_task_save.mutable_task_list()->add_task_list();
    uint32_t ret = DataProtoUtils::pack_task_data(&task, task_info);

	if (ret) {
		return ret;
	}
    return g_dbproxy->send_msg(NULL, player->userid, 
			player->create_tm,
            db_cmd_task_save, db_cs_task_save);
}

//同步内存 db和客户端
uint32_t TaskUtils::save_and_syn_task(player_t* player, const task_t& task_data)
{
    // 同步内存
    player->task_info->set_task(task_data);

    // 同步db
    uint32_t ret = TaskUtils::db_update_task(player, task_data);
    if (ret != 0) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    task_conf_t* task_conf = g_task_conf_mgr.find_task_conf(task_data.task_id);

	if (task_conf == NULL)
		return cli_err_task_info_invalid;

    // 同步到客户端
    onlineproto::sc_0x0404_sync_task sc_sync_task;

	commonproto::task_data_t* task_info = sc_sync_task.add_task_list();
    task_info->set_task_id(task_data.task_id);
    task_info->set_status(task_data.status);
	task_info->set_done_times(task_data.done_times);
	task_info->set_bonus_status(task_data.bonus_status);

	for (uint32_t j = 0; j < task_conf->step_count; j++) {
		task_info->add_cond_status(task_data.step_list[j].condition_status);		
	}
    ret =  send_msg_to_player(player, cli_cmd_cs_0x0404_sync_task, sc_sync_task);

    return ret;
}

uint32_t TaskUtils::syn_task(player_t* player, const commonproto::task_data_t& task_info)
{
    // 同步到客户端
    onlineproto::sc_0x0404_sync_task sc_sync_task;
	commonproto::task_data_t* each_task_info = sc_sync_task.add_task_list();
    each_task_info->CopyFrom(task_info);

    return  send_msg_to_player(player, cli_cmd_cs_0x0404_sync_task, sc_sync_task);

}

//判断任务奖励是否可以领取
bool TaskUtils::is_accept_task_bonus(player_t* player, uint32_t task_id, uint32_t step_count)
{
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);

    for (uint32_t i = 0; i < task_list.size(); i++) {
        return task_list[i].status & (1 << (step_count - 1));
    }

    return false;
}

//更新任务到内存，数据库，并同步给客户端
uint32_t TaskUtils::update_task_and_sync_db(
        player_t* player, std::vector<task_t>& task_vec , uint32_t step, bool reward_flag)
{

    for (uint32_t i = 0; i < task_vec.size(); i++) {
        const task_conf_t* task_conf =  g_task_conf_mgr.find_task_conf(task_vec[i].task_id);
        if (task_conf == NULL) {
            return cli_err_task_info_invalid;
        }
        task_vec[i].status |= (1 << (step-1));
		task_vec[i].fin_statue_tm = NOW();

        //当可以领取奖励时
        if (reward_flag) {
            if (TaskUtils::is_accept_task_bonus(
                        player, task_vec[i].task_id, task_conf->step_count)) {
                //读奖励列表
                std::vector<uint32_t> prize_list;
				//如果是悬赏任务，不领取奖励，而增加是积分
				reward_task_conf_t* conf_ptr = g_reward_task_conf_mgr.find_reward_task_conf(task_vec[i].task_id);
				if (conf_ptr) {
					ADD_A(kDailyRewardTaskScore, conf_ptr->score);
				} else {
					std::vector<std::string> args_list = split(task_conf->bonus_id, ',');
					FOREACH(args_list, it) {
						prize_list.push_back(atoi_safe((*it).c_str()));
					}
				}

                //设置标志位
				for (uint32_t i = 0; i < task_vec.size(); i++) {
					task_vec[i].bonus_status = kTaskBonusAccept;
					task_vec[i].done_times += 1;
					//如果任务类型是成就
					if (task_vec[i].task_id / TASK_TYPE_BASE == TASK_FOR_ACHIEVE) {
						ADD_A(kAttrAchievePoint, 10);
						uint32_t achieve_point = GET_A(kAttrAchievePoint);
						uint32_t btl_record = GET_A(kAttrBattleValueRecord);
						uint64_t score = ((uint64_t)achieve_point << 32) | btl_record;
						RankUtils::rank_user_insert_score(
								player->userid, player->create_tm,
								commonproto::RANKING_ACHIEVEMENT,
								0, score);
					}
				}

                //发奖
                onlineproto::sc_0x0112_notify_get_prize msg;
                int ret = 0;
                for (uint32_t i = 0; i < prize_list.size(); i++) {
                    ret = transaction_proc_prize(player, prize_list[i], msg);
                    if (ret) {
                        return ret;
                    }
                }
            }
        }

        //更新内存
        player->task_info->set_task(task_vec[i]);
        //更新db
        uint32_t ret = TaskUtils::db_update_task(player, task_vec[i]);
        if (ret != 0) {
            return ret;
        }
    }

	//同步客户端
    onlineproto::sc_0x0404_sync_task sc_sync_task;
    for(unsigned int i=0; i<task_vec.size(); i++) {
        commonproto::task_data_t* each_task_info = sc_sync_task.add_task_list();
        uint32_t ret = DataProtoUtils::pack_task_data(&(task_vec[i]), each_task_info);
		if (ret) {
			return ret;
		}
    }    
    
    return  send_msg_to_player(player, cli_cmd_cs_0x0404_sync_task, sc_sync_task);
}

//更新任务到内存，数据库，并同步给客户端
uint32_t TaskUtils::update_task_range_step_and_sync_db(
        player_t* player, std::vector<task_t>& task_vec , 
        uint32_t start_step, uint32_t end_step, bool reward_flag)
{
    for (uint32_t i = 0; i < task_vec.size(); i++) {
        const task_conf_t* task_conf =  g_task_conf_mgr.find_task_conf(task_vec[i].task_id);
        if (task_conf == NULL) {
            continue;
        }

        for (uint32_t step = start_step; step <= end_step; step++) {
            if (step > task_conf->step_count) {
                return cli_err_task_info_invalid;
            }
            task_vec[i].status |= (1 << (step-1));
			//更新当前的任务完成的时间戳
			task_vec[i].fin_statue_tm = NOW();
        }

        //当可以领取奖励时
        if (reward_flag) {
            if (TaskUtils::is_accept_task_bonus(
                        player, task_vec[i].task_id, task_conf->step_count)) {
                //读奖励列表
                std::vector<std::string> args_list = split(task_conf->bonus_id, ',');
                std::vector<uint32_t> prize_list;
                FOREACH(args_list, it) {
                    prize_list.push_back(atoi_safe((*it).c_str()));
                }

                //设置标志位
                for (uint32_t i = 0; i < task_vec.size(); i++) {
                    task_vec[i].bonus_status = kTaskBonusAccept;
                    task_vec[i].done_times += 1;
                }

                //发奖
                onlineproto::sc_0x0112_notify_get_prize msg;
                int ret = 0;
                for (uint32_t i = 0; i < prize_list.size(); i++) {
                    ret = transaction_proc_prize(player, prize_list[i], msg);
                    if (ret) {
                        return ret;
                    }
                }
            }
        }

        //更新内存
        player->task_info->set_task(task_vec[i]);
        //更新db
        uint32_t ret = TaskUtils::db_update_task(player, task_vec[i]);
        if (ret != 0) {
            return ret;
        }
    }

	//同步客户端
    onlineproto::sc_0x0404_sync_task sc_sync_task;
    for(unsigned int i=0; i<task_vec.size(); i++) {
        commonproto::task_data_t* each_task_info = sc_sync_task.add_task_list();
        uint32_t ret = DataProtoUtils::pack_task_data(&(task_vec[i]), each_task_info);
		if (ret) {
			return ret;
		}
    }    
    
    return  send_msg_to_player(player, cli_cmd_cs_0x0404_sync_task, sc_sync_task);
}

const task_conf_t* TaskUtils::get_task_config(uint32_t taskid)
{
    return g_task_conf_mgr.find_task_conf(taskid);

}

//监听副本结束
uint32_t TaskUtils::listen_duplicate_complete(player_t* player, uint32_t dup_id) 
{

    std::vector<task_t> task_vec;
    player->task_info->get_all_tasks(task_vec);
    for (uint32_t i = 0; i < task_vec.size(); i++) {
		if (!g_task_conf_mgr.is_task_conf_exist(task_vec[i].task_id)) {
			return cli_err_task_info_invalid;
		}
		
		if (task_vec[i].bonus_status == kTaskBonusAccept) {
			continue;
		}
		//找到当前任务在第几步
		uint32_t step_count = 0;
		uint32_t status = task_vec[i].status;

		for (step_count = 1; taomee::test_bit_on(status, step_count) == 1; step_count++) {
			if (step_count >= 32) {
				break;
			}
		}

		task_conf_t *task_conf = g_task_conf_mgr.find_task_conf(task_vec[i].task_id);
        if (task_conf == NULL) {
            ERROR_TLOG("invalid task id:%u", task_vec[i].task_id); 
            continue;
        }

        // 任务已完成
        if (step_count > task_conf->step_count + 1) {
            continue;
        }

		//取出当前步的信息
		step_t& step_info = task_vec[i].step_list[step_count - 2];
		
		
		//取出当前步的配置信息
        bool change_flag = false;
		step_conf_t& step_conf = task_conf->step_conf_vec[step_count - 2];
		for (uint32_t j = 0; j < step_conf.complete_con_vec.size(); j++) {
			if (step_conf.complete_con_vec[j].type == TASK_COND_PASS_DUP && 
                    (uint32)atoi(step_conf.complete_con_vec[j].params) == dup_id) {
				if (step_conf.complete_con_vec[j].id >= 32) {
					return cli_err_task_info_invalid;
				}
				step_info.condition_status = taomee::set_bit_on(
                        step_info.condition_status, step_conf.complete_con_vec[j].id);
                change_flag = true;
			}
		}

        if (change_flag) {
            uint32_t ret = save_and_syn_task(player, task_vec[i]);
            if (ret != 0) {
                return ret;
            }
        }
    }
	return 0;
}
////////////////////////////
//以下代码均没有联调过！！！！！！！！！！！！！！！！！！！！！！
////////////////////////
//玩家等级

bool TaskUtils::condition_fun(player_t* player, condition_conf_t cond_conf, uint32_t task_id, int attr) 
{
	std::vector<task_t> task_list;
	player->task_info->get_task(task_id, task_list);
	if (task_list.size() == 0) {
		return cli_err_data_error;
	}
	task_t task_info = task_list[0];

	//找到当前任务在第几步
    uint32_t step_count = TaskUtils::get_task_step(player, task_id) + 1;

    task_conf_t task_conf = *(g_task_conf_mgr.find_task_conf(task_id));
    // 任务已完成
    if (step_count > task_conf.step_count) {
        return false;
    }

	//取出当前步的信息
	step_t& step_info = task_info.step_list[step_count - 1];
	std::vector<std::string> params = split(cond_conf.params, ',');

	bool flag = 0;
	//根据玩家自身等级，operate和param判断等级是否符合要求
	switch (cond_conf.operate) {
		case 0:
			if (params.size() == 0) {
				return false;
			}
			if (attr >= atoi(params[0].c_str())){
				flag = 1;
			}
		break;
		case 1:
			if (params.size() == 0) {
				return false;
			}
			if (attr <= atoi(params[0].c_str())){
				flag = 1;
			}
		break;
		case 2:
			if (params.size() < 2) {
				return false;
			}
			if (attr >= atoi(params[0].c_str()) && attr <= atoi(params[1].c_str())){
				flag = 1;
			}
		break;
		case 3:
			if (params.size() == 0) {
				return false;
			}
			if (attr == atoi(params[0].c_str())){
				flag = 1;
			}
		break;
		case 4:
			if (params.size() == 0) {
				return false;
			}
			if (taomee::test_bit_on(attr, atoi(params[0].c_str()))) {
				flag = 1;
			}
		break;
		case 5:
			if (params.size() == 0) {
				return false;
			}
			if (!taomee::test_bit_on(attr, atoi(params[0].c_str()))) {
				flag = 1;
			}
		break;
	}

	if (flag) {
		taomee::set_bit_on(step_info.condition_status, cond_conf.id);
		return true;
	}
	return false;
}

/** 
 * @brief 更新TASK_COND_ATTR 类型的任务当前步骤的关联属性值,必须先接取
 * 
 * @param player 
 * @param task_id 
 * @param value 
 * 
 * @return 
 */
uint32_t TaskUtils::update_task_condition_attr(player_t* player, uint32_t task_id, uint32_t value) 
{
    if (player == NULL) {
        return 0; 
    }

    std::vector<task_t> task_vec;
    player->task_info->get_task(task_id, task_vec);
    for (uint32_t i = 0; i < task_vec.size(); i++) {
		if (!g_task_conf_mgr.is_task_conf_exist(task_vec[i].task_id)) {
			return cli_err_task_info_invalid;
		}
		
		if (task_vec[i].bonus_status == kTaskBonusAccept) {
			continue;
		}
		//找到当前任务在第几步
		uint32_t step_count = 0;
		uint32_t status = task_vec[i].status;

		for (step_count = 1; taomee::test_bit_on(status, step_count) == 1; step_count++) {
			if (step_count >= 32) {
				break;
			}
		}
		
		task_conf_t *task_conf = g_task_conf_mgr.find_task_conf(task_vec[i].task_id);
		if (task_conf == NULL)
			return cli_err_task_info_invalid;
		//取出当前步的配置信息
        bool change_flag = false;
		step_conf_t& step_conf = task_conf->step_conf_vec[step_count - 2];
		for (uint32_t j = 0; j < step_conf.complete_con_vec.size(); j++) {
			if (step_conf.complete_con_vec[j].type == TASK_COND_ATTR) {
		        std::vector<std::string> args_list = split(step_conf.complete_con_vec[j].params, ',');
				if (step_conf.complete_con_vec[j].id >= 32) {
					return cli_err_task_info_invalid;
				}

                if (args_list.size() != 2) {
					return cli_err_task_info_invalid;
                }

                // 检查任务等级条件
                uint32_t level = GET_A(kAttrLv);
                uint32_t cond_level = get_reward_task_level(task_id);
                if (level >= cond_level) {
                    // 更新属性值
                    uint32_t attr_type = (uint32_t)atoi(args_list[1].c_str());
                    uint32_t val = GET_A((enum attr_type_t)attr_type);

                    uint32_t cond_val = (uint32_t)atoi(args_list[0].c_str());
                    if (val < cond_val) {
                        ADD_A((enum attr_type_t)attr_type, value);
                        change_flag = true;
                    }
                }
			}
		}

        // 同步任务数据
        if (change_flag) {
            uint32_t ret = save_and_syn_task(player, task_vec[i]);
            if (ret != 0) {
                return ret;
            }
        }
    }

    return 0;
}

/** 
 * @brief 更新TASK_COND_ATTR 类型特定步骤的任务关联属性值
 *        不修改任务状态，可以用来完成未接取的任务
 * 
 * @param player 
 * @param task_id 
 * @param value 
 * @param step
 * 
 * @return 
 */
uint32_t TaskUtils::update_task_step_condition_attr(
        player_t* player, uint32_t task_id, uint32_t value, uint32_t step) 
{
    if (player == NULL) {
        return 0; 
    }

    task_conf_t *task_conf = g_task_conf_mgr.find_task_conf(task_id);
    if (task_conf == NULL) {
        return cli_err_task_info_invalid;
    }

    if (step > task_conf->step_conf_vec.size()) {
        return cli_err_task_info_invalid;
    }

    //取出当前步的配置信息
    step_conf_t& step_conf = task_conf->step_conf_vec[step - 1];
    for (uint32_t j = 0; j < step_conf.complete_con_vec.size(); j++) {
        if (step_conf.complete_con_vec[j].type == TASK_COND_ATTR) {
            std::vector<std::string> args_list = split(step_conf.complete_con_vec[j].params, ',');
            if (step_conf.complete_con_vec[j].id >= 32) {
                return cli_err_task_info_invalid;
            }

            if (args_list.size() != 2) {
                return cli_err_task_info_invalid;
            }

            // 检查任务等级条件
            uint32_t level = GET_A(kAttrLv);
            uint32_t cond_level = get_reward_task_level(task_id);
            if (level >= cond_level) {
                // 更新属性值
                uint32_t attr_type = (uint32_t)atoi(args_list[1].c_str());
                uint32_t val = GET_A((enum attr_type_t)attr_type);

                uint32_t cond_val = (uint32_t)atoi(args_list[0].c_str());
                if (val < cond_val) {
                    ADD_A((enum attr_type_t)attr_type, value);
                }
            }
        }
    }

    return 0;
}

/** 
 * @brief 设置TASK_COND_SERVER类型的任务条件完成
 * 
 * @param player 
 * @param task_id   任务id 
 * @param task_step 任务步骤
 * @param cond_id   条件id
 * 
 * @return 
 */
uint32_t TaskUtils::update_task_condition_server(
        player_t* player, uint32_t task_id, 
        uint32_t task_step, uint32_t cond_id) 
{
    if (player == NULL) {
        return 0; 
    }

    std::vector<task_t> task_vec;
    player->task_info->get_task(task_id, task_vec);
    for (uint32_t i = 0; i < task_vec.size(); i++) {
		if (!g_task_conf_mgr.is_task_conf_exist(task_vec[i].task_id)) {
			return cli_err_task_info_invalid;
		}
		
		if (task_vec[i].bonus_status == kTaskBonusAccept) {
			continue;
		}

		task_conf_t *task_conf = g_task_conf_mgr.find_task_conf(task_vec[i].task_id);
        if (task_step > task_conf->step_count) {
			return cli_err_task_info_invalid;
        }

		if (task_conf == NULL) {
			return cli_err_task_info_invalid;
        }

		//取出当前步的配置信息
        step_t& step_info = task_vec[i].step_list[task_step - 1];
		step_conf_t& step_conf = task_conf->step_conf_vec[task_step - 1];

        if (cond_id > step_conf.complete_con_vec.size()) {
			return cli_err_task_info_invalid;
        }

        // 设置完成
        step_info.condition_status = taomee::set_bit_on(step_info.condition_status, cond_id);

        // 同步任务数据
        uint32_t ret = save_and_syn_task(player, task_vec[i]);
        if (ret != 0) {
            return ret;
        }


        ////更新内存
        //player->task_info->set_task(task_vec[i]);
		//onlineproto::sc_0x0404_sync_task sc_sync_task;

		//commonproto::task_data_t* task_info = sc_sync_task.add_task_list();
		//task_info->set_task_id(task_vec[i].task_id);
		//task_info->set_status(task_vec[i].status);
		//task_info->set_done_times(task_vec[i].done_times);
		//task_info->set_bonus_status(task_vec[i].bonus_status);

		//uint32_t status = task_vec[i].status;
		//uint32_t step_count = 0;
		//for (step_count = 1; taomee::test_bit_on(status, step_count) == 1; step_count++) {
			//if (step_count >= 32) {
				//break;
			//}
		//}

		//for (uint32_t j = 0; j < step_count; j++) {
			//task_info->add_cond_status(task_vec[i].step_list[j].condition_status);		
		//}

		//uint32_t ret =  send_msg_to_player(player, cli_cmd_cs_0x0404_sync_task, sc_sync_task);
		//if (ret != 0) {
			//return ret;
		//}
        ////更新db
        //ret = TaskUtils::db_update_task(player, task_vec[i]);
        //if (ret != 0) {
            //return ret;
        //}
    }

    return 0;
}

bool TaskUtils::condition_fun_1(player_t* player, condition_conf_t cond_conf, uint32_t task_id) 
{
    condition_type_t type = cond_conf.type;
    if (type != TASK_COND_PLAYER_LEVEL) {
        return false;
    }

	int level = GET_A(kAttrLv);
	return TaskUtils::condition_fun(player, cond_conf, task_id, level);
}


//玩家战力
bool TaskUtils::condition_fun_2(player_t* player, condition_conf_t cond_conf, uint32_t task_id) 
{
    condition_type_t type = cond_conf.type;
    if (type != TASK_COND_PLAYER_BATTLE_VALUE) {
        return false;
    }

	int power = GET_A(kAttrCurBattleValue);
	return TaskUtils::condition_fun(player, cond_conf, task_id, power);
}

//玩家职业
bool TaskUtils::condition_fun_3(player_t* player, condition_conf_t cond_conf, uint32_t task_id) 
{
    condition_type_t type = cond_conf.type;
    if (type != TASK_COND_PLAYER_PROFESSION) {
        return false;
    }

	int prof = GET_A(kAttrCurProf);
	return TaskUtils::condition_fun(player, cond_conf, task_id, prof);
}

//伙伴数量
bool TaskUtils::condition_fun_5(player_t* player, condition_conf_t cond_conf, uint32_t task_id) 
{
    condition_type_t type = cond_conf.type;
    if (type != TASK_COND_PET_NUM) {
        return false;
    }
   
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }

    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 1) {
        return false;
    }

    uint32_t cond_num = (uint32_t)atoi(args_list[0].c_str());

    bool cond_res = false;
    if (player->pets && player->pets->size() >= cond_num) {
        cond_res = true;
    }
    return cond_res;
}

//伙伴等级
bool TaskUtils::condition_fun_6(player_t* player, condition_conf_t cond_conf, uint32_t task_id) 
{
    condition_type_t type = cond_conf.type;
    if (type != TASK_COND_PET_LEVEL) {
        return false;
    }
   
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }

    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 1) {
        return false;
    }

    uint32_t cond_num = (uint32_t)atoi(args_list[0].c_str());

    bool cond_res = false;
    FOREACH(*(player->pets), iter) {
        if (iter->second.level() >= cond_num) {
            cond_res = true;
            break;
        }
    }
    return cond_res;
}

bool TaskUtils::condition_fun_8(player_t* player, condition_conf_t cond_conf, uint32_t task_id)
{
	condition_type_t type = cond_conf.type;
	if (type != TASK_COND_EFFORT) {
		return false;
	}
	std::vector<task_t> task_list;
	player->task_info->get_task(task_id, task_list);
	if (task_list.size() == 0) {
		return false;
	}
	std::vector<std::string> args_list = split(cond_conf.params, ',');
	if (args_list.size() != 2) {
		return false;
	}
	uint32_t cond_count = (uint32_t)atoi(args_list[0].c_str());
	uint32_t cond_effort_lv = (uint32_t)atoi(args_list[1].c_str());
	uint32_t pet_count = 0;
	if (player->pets) {
		FOREACH(*(player->pets), iter) {
			Pet& pet = iter->second;
			for (uint32_t idx = 0; idx < kMaxEffortNum; ++idx) {
				if (pet.effort_lv(idx) >= cond_effort_lv) {
					++pet_count;
					break;
				}
			}
		}
	}

    bool cond_res = (pet_count >= cond_count) ? true : false;
    return cond_res;
}

bool TaskUtils::condition_fun_9(player_t* player, 
		condition_conf_t cond_conf, uint32_t task_id)
{
	condition_type_t type = cond_conf.type;
	if (type != TASK_COND_RUNE) {
		return false;
	}
	std::vector<task_t> task_list;
	player->task_info->get_task(task_id, task_list);
	if (task_list.size() == 0) {
		return false;
	}
	std::vector<std::string> args_list = split(cond_conf.params, ',');
	if (args_list.size() != 2) {
		return false;
	}
	uint32_t cond_pet_count = (uint32_t)atoi(args_list[0].c_str());
	uint32_t cond_rune_count = (uint32_t)atoi(args_list[1].c_str());
	uint32_t pet_count = 0;
	if (player->pets) {
		FOREACH(*(player->pets), iter) {
			Pet& pet = iter->second;
			uint32_t rune_count = 0;
			for (uint32_t idx = 0; idx < kMaxEquipRunesNum; ++idx) {
				uint32_t rune_id = pet.get_rune_array(idx);
				rune_t rune;
				if (RuneUtils::get_rune(player, rune_id, rune)) {
					continue;
				}
				rune_conf_t rune_conf;
				if (RuneUtils::get_rune_conf_data(rune.conf_id, rune_conf)) {
					continue;
				}
				if (rune_conf.rune_type == 3) {
					++rune_count;
				}
			}
			if (rune_count >= cond_rune_count) {
				++pet_count;
			}
		}
	}
    bool cond_res = (pet_count >= cond_pet_count) ? true : false;
    return cond_res;
}

// 属性
bool TaskUtils::condition_fun_10(player_t* player, condition_conf_t cond_conf, uint32_t task_id) 
{
    condition_type_t type = cond_conf.type;
    if (type != TASK_COND_ATTR) {
        return false;
    }
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }

    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 2) {
        return false;
    }

    uint32_t attr_type = (uint32_t)atoi(args_list[1].c_str());
    uint32_t val = GET_A((enum attr_type_t)attr_type);

	return TaskUtils::condition_fun(player, cond_conf, task_id, val);
}

//副本
bool TaskUtils::condition_fun_11(player_t* player, condition_conf_t cond_conf, uint32_t task_id) 
{
	std::vector<task_t> task_list;
	player->task_info->get_task(task_id, task_list);
	if (task_list.size() == 0) {
		return -1;
	}
	task_t task_info = task_list[0];

	//找到当前任务在第几步
	uint32_t step_count = 0;
	uint32_t status = task_info.status;

	for (step_count = 1; taomee::test_bit_on(status, step_count) == 1; step_count++) {
		if (step_count >= 32) {
			break;
		}
	}
	//取出当前步的信息
	step_t& step_info = task_info.step_list[step_count - 2];
	if (cond_conf.id < 1 || cond_conf.id > 32) {
		return -1;
	}
	bool cond_res = taomee::test_bit_on(step_info.condition_status, cond_conf.id);
	return cond_res;
}

//玩家特定颜色装备数
bool TaskUtils::condition_fun_13(player_t* player, condition_conf_t cond_conf, uint32_t task_id) 
{
    condition_type_t type = cond_conf.type;
    if (type != TASK_COND_COLOR_EQUIP_NUM) {
        return false;
    }
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }

    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 2) {
        return false;
    }

    uint32_t cond_quality = (uint32_t)atoi(args_list[1].c_str());
    uint32_t cond_count = (uint32_t)atoi(args_list[0].c_str());

    uint32_t item_count = 0;
    std::vector<item_t> items;

    if (player->package) {
        player->package->get_fun_type_items(items, ITEM_FUNC_EQUIP_ARM);
    }

    FOREACH(items, iter) {
        item_t &item = *iter;
        const item_conf_t *item_conf = 
            g_item_conf_mgr.find_item_conf(item.item_id);
        if (item_conf && (uint32_t)item_conf->base_quality >= cond_quality) {
            item_count += item.count;
        }
    }

    bool cond_res = (item_count >= cond_count) ? true : false;
    return cond_res;
}

//玩家特定星级的伙伴数量
bool TaskUtils::condition_fun_14(player_t* player, condition_conf_t cond_conf, uint32_t task_id) 
{
    condition_type_t type = cond_conf.type;
    if (type != TASK_COND_STAR_PET_NUM) {
        return false;
    }
   
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }

    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 2) {
        return false;
    }

    uint32_t cond_count = (uint32_t)atoi(args_list[0].c_str());
    uint32_t cond_talent_lv = (uint32_t)atoi(args_list[1].c_str());

    uint32_t pet_count = 0;
    if (player->pets) {
        FOREACH(*(player->pets), iter) {
            if ((uint32_t)iter->second.talent_level() >= cond_talent_lv) {
                ++pet_count;
            }
        }
    }

    bool cond_res = (pet_count >= cond_count) ? true : false;
    return cond_res;
}

//玩家特定品质的伙伴数量
bool TaskUtils::condition_fun_15(player_t* player, condition_conf_t cond_conf, uint32_t task_id) 
{
    condition_type_t type = cond_conf.type;
    if (type != TASK_COND_PET_QUALITY_NUM) {
        return false;
    }
   
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }

    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 2) {
        return false;
    }

    uint32_t cond_count = (uint32_t)atoi(args_list[0].c_str());
    uint32_t cond_quality = (uint32_t)atoi(args_list[1].c_str());

    uint32_t pet_count = 0;
    if (player->pets) {
        FOREACH(*(player->pets), iter) {
            if (iter->second.quality() >= cond_quality) {
				++pet_count;
            }
        }
    }

    bool cond_res = (pet_count >= cond_count) ? true : false;
    return cond_res;
}

bool TaskUtils::condition_fun_17(player_t* player, condition_conf_t cond_conf, uint32_t task_id)
{
	condition_type_t type = cond_conf.type;
	if (type != TASK_COND_PET_GET_X_STAR) {
		return false;
	}

    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }

    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 2) {
        return false;
    }

    uint32_t cond_pet_id = (uint32_t)atoi(args_list[0].c_str());
    int cond_pet_talent_level = (uint32_t)atoi(args_list[1].c_str());
	//找出赤瞳
	bool cond_res = false;
	if (player->pets) {
		FOREACH(*(player->pets), it) {
			if (it->second.pet_id() != cond_pet_id) {
				continue;
			} 
			if (it->second.talent_level() >= cond_pet_talent_level) {
				cond_res = true;
			}
		}
	}
	return cond_res;
}

bool TaskUtils::condition_fun_18(player_t* player, condition_conf_t cond_conf, uint32_t task_id)
{
	condition_type_t type = cond_conf.type;
	if (type != TASK_COND_PET_GET_X_RUNE) {
		return false;
	}

    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }

    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 2) {
        return false;
    }
    uint32_t cond_pet_id = (uint32_t)atoi(args_list[0].c_str());
    uint32_t cond_rune_cnt = (uint32_t)atoi(args_list[1].c_str());
	uint32_t pet_count = 0;
	if (player->pets) {
		FOREACH(*(player->pets), it) {
			Pet &pet = it->second;
			if (pet.pet_id() != cond_pet_id) {
				continue;
			}
			std::vector<uint32_t> rune_info;
			pet.get_rune_info_equiped_in_pet(rune_info);
			FOREACH(rune_info, it) {
				rune_attr_type_t rune_type = RuneUtils::get_rune_attr_type(player, *it);
				if (rune_type == kRunePurple) {
					++pet_count;
				}
			}
		}
	}
	bool cond_res = (pet_count >= cond_rune_cnt) ? true : false;
	return cond_res;
}

bool TaskUtils::condition_fun_19(player_t* player, condition_conf_t cond_conf, uint32_t task_id)
{
    condition_type_t type = cond_conf.type;
    if (type != TASK_COND_PET_GET_X_EFFORT) {
        return false;
    }
   
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }

    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 2) {
        return false;
    }
    uint32_t cond_pet_id = (uint32_t)atoi(args_list[0].c_str());
    uint32_t cond_effort_lv = (uint32_t)atoi(args_list[1].c_str());
	bool cond_res = false;
	if (player->pets) {
		FOREACH(*(player->pets), it) {
			Pet &pet = it->second;
			if (pet.pet_id() != cond_pet_id) {
				continue;
			}
			if (pet.effort_lv_sum() >= cond_effort_lv) {
				cond_res = true;
				break;
			}
		}
	}
	return cond_res;
}

bool TaskUtils::condition_fun_20(player_t* player,
		condition_conf_t cond_conf, uint32_t task_id)
{
	condition_type_t type = cond_conf.type;
	if (type != TASK_COND_PET_DIJU_AWAKE) {
		return false;
	}
   
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }
    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 1) {
        return false;
    }
    uint32_t cond_pet_id = (uint32_t)atoi(args_list[0].c_str());
	bool cond_res = false;
	if (player->pets) {
		FOREACH(*(player->pets), it) {
			Pet &pet = it->second;
			if (pet.pet_id() == cond_pet_id) {
				commonproto::pet_optional_attr_t pb_pet_attr;
				pb_pet_attr.ParseFromString(pet.pet_opt_attr);
				if (pb_pet_attr.awake_state()) {
					cond_res = true;
				}
				break;
			}
		}
	}
	return cond_res;
}

bool TaskUtils::condition_fun_21(player_t* player,
		condition_conf_t cond_conf, uint32_t task_id)
{
	condition_type_t type = cond_conf.type;
	if (type != TASK_COND_PET_DIJU_LV) {
		return false;
	}
   
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }
    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 3) {
        return false;
    }
    uint32_t cond_pet_id = (uint32_t)atoi(args_list[0].c_str());
    uint32_t attr_index = (uint32_t)atoi(args_list[1].c_str());
    uint32_t cond_lv = (uint32_t)atoi(args_list[2].c_str());
	bool cond_res = false;
	if (player->pets) {
		FOREACH(*(player->pets), it) {
			Pet &pet = it->second;
			if (pet.pet_id() == cond_pet_id) {
				commonproto::pet_optional_attr_t pb_pet_attr;
				pb_pet_attr.ParseFromString(pet.pet_opt_attr);
				if (pb_pet_attr.lamp().lamp_list_size() == 0) {
					return false;
				} 
				uint32_t diju_lv = 0;
				if (attr_index == 1) {
					diju_lv = pb_pet_attr.lamp().lamp_list(0).hp_lv();
				} else if (attr_index == 2) {
					diju_lv = pb_pet_attr.lamp().lamp_list(0).normal_atk_lv();
				} else if (attr_index == 3) {
					diju_lv = pb_pet_attr.lamp().lamp_list(0).normal_def_lv();
				} else if (attr_index == 4) {
					diju_lv = pb_pet_attr.lamp().lamp_list(0).skill_atk_lv();
				} else if (attr_index == 5) {
					diju_lv = pb_pet_attr.lamp().lamp_list(0).skill_def_lv();
				}
				if (diju_lv >= cond_lv) {
					cond_res = true;
				}
				break;
			}
		}
	}
	return cond_res;
}

bool TaskUtils::condition_fun_22(player_t* player,
		condition_conf_t cond_conf, uint32_t task_id)
{
	condition_type_t type = cond_conf.type;
	if (type != TASK_COND_THIS_PET_LEVEL) {
		return false;
	}
   
    std::vector<task_t> task_list;
    player->task_info->get_task(task_id, task_list);
    if (task_list.size() == 0) {
        return false;
    }
    std::vector<std::string> args_list = split(cond_conf.params, ',');
    if (args_list.size() != 2) {
        return false;
    }
    uint32_t cond_lv = (uint32_t)atoi(args_list[0].c_str());
    uint32_t cond_pet_id = (uint32_t)atoi(args_list[1].c_str());
	bool cond_res = false;
	if (player->pets) {
		FOREACH(*(player->pets), it) {
			Pet &pet = it->second;
			if (pet.pet_id() == cond_pet_id) {
				if (pet.level() >= cond_lv) {
					cond_res = true;
				}
				break;
			}
		}
	}
	return cond_res;
}

bool TaskUtils::is_reward_task(uint32_t task_id)
{
    if (task_id > REWARD_TASK_ID_START && task_id <= REWARD_TASK_ID_END) {
        return true;
    }
    return false;
}

/** 
 * @brief 判断task_id是否reward_task, 并取出对应reward_task的等级
 * 
 * @param task_id
 * 
 * @return 
 */
uint32_t TaskUtils::get_reward_task_level(uint32_t task_id)
{
    if (!is_reward_task(task_id)) {
        return 0;
    }

    const std::map<uint32_t, reward_task_conf_t> & reward_task_map = 
        g_reward_task_conf_mgr.const_reward_task_conf_map();

    FOREACH(reward_task_map, it) {
        if (it->second.task_id == task_id) {
            return it->second.level;
        }
    }

    return 0;
}

int TaskUtils::reset_daily_reward_task(player_t *player)
{
    uint32_t last_date = TimeUtils::time_to_date(GET_A(kAttrLastLoginTm));
    if (last_date != TimeUtils::get_today_date()) {
        const std::map<uint32_t, reward_task_conf_t> &reward_tasks = 
            g_reward_task_conf_mgr.const_reward_task_conf_map();

        FOREACH(reward_tasks, iter) {
            const reward_task_conf_t *reward_task = &(iter->second);
            if (reward_task == NULL) {
                continue;
            }
            uint32_t task_id = reward_task->task_id;
            TaskUtils::del_task_record(player, task_id);
        }
    }

    return 0;    
}

int TaskUtils::reset_time_limit_task(player_t *player)
{
    if (player == NULL) {
        return 0;
    }
    uint32_t last_date = TimeUtils::time_to_date(GET_A(kAttrLastLoginTm));
	if (last_date == TimeUtils::get_today_date()) {
		return 0;
	}
    std::vector<task_t> task_vec;
    player->task_info->get_all_tasks(task_vec);
    for (uint32_t i = 0; i < task_vec.size(); i++) {
        uint32_t task_id = task_vec[i].task_id;
        if (!g_task_conf_mgr.is_task_conf_exist(task_id)) {
            continue;
        }

        task_conf_t *task_conf = g_task_conf_mgr.find_task_conf(task_id);
        if (task_conf == NULL) {
            ERROR_TLOG("reset_daily_task invalid task id:%u", task_id); 
            continue;
        }

        // 每日重置任务,限时任务
        if (task_conf->daily_reset ||
                TaskUtils::is_task_over_time_limit(player, task_id)) {
            TaskUtils::del_task_record(player, task_id);
        }
    }
    return 0;    
}

/** 
 * @brief 更新副本相关的悬赏任务进度
 * 
 * @param player 
 * @param dup 
 * @param value 
 * 
 * @return 
 */
int TaskUtils::update_daily_reward_task_duplicate_step(
        player_t *player, const duplicate_t *dup, uint32_t value)
{
    if (dup == NULL || player == NULL) {
        return 0;
    }

    uint32_t dup_task[][2] = {
        {DUP_MODE_TYPE_NORMAL, REWARD_TASK_ITEM_NORMAL_DUP},
        {DUP_MODE_TYPE_ELITE, REWARD_TASK_ITEM_ELITE_DUP},
        {DUP_MODE_TYPE_ELEM_DUP, REWARD_TASK_ITEM_ELEM_FIGHT},
        {DUP_MODE_TYPE_BUCKET, REWARD_TASK_ITEM_BUCKET_DUP},
    };

    for (uint32_t i = 0; i < array_elem_num(dup_task);i++){
        if (dup->mode == (enum duplicate_mode_t)dup_task[i][0]) {
            TaskUtils::update_task_condition_attr(player, dup_task[i][1], 1);
        }
    }

    return 0;
}

/** 
 * @brief 更新物品输出相关的悬赏任务进度
 * 
 * @param player 
 * @param dup 
 * @param step 
 * @param value 
 * 
 * @return 
 */
int TaskUtils::update_reward_task_item_step(
        player_t *player, const std::vector<add_item_info_t> *add_items)
{
    if (!(player && add_items)) {
        return 0;
    }
    FOREACH(*add_items, iter) {
        const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(iter->item_id);
        if(item_conf && item_conf->fun_type == ITEM_FUNC_IMPROVE_TALENT) {
            TaskUtils::update_task_step_condition_attr(
                    player, REWARD_TASK_ITEM_PET_STAR_UP, 1, 1);
        }
    }
    return 0;
}

/** 
 * @brief 更新副本相关的新手任务
 * 
 * @param player 
 * @param dup_id 
 * @param win 
 * @param type 0 进副本算完成 1 通过副本算完成
 * 
 * @return 
 */
int TaskUtils::update_new_player_task_dup_step(
        player_t *player, uint32_t dup_id, bool win, uint32_t type)
{
    if (type == 0) {
        // <副本mode，任务id>
        std::map<uint32_t, uint32_t> dup_mode_task_map;
        dup_mode_task_map[DUP_MODE_TYPE_TRIAL] = kAttrRookieGuide13PassTrial;
        dup_mode_task_map[DUP_MODE_TYPE_ELEM_DUP] = kAttrRookieGuide12PassElemDup;

        const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
        if (dup) {
            std::map<uint32_t, uint32_t>::iterator iter = dup_mode_task_map.find(dup->mode);
            if (iter != dup_mode_task_map.end()) {
                uint32_t attr_type = iter->second;
                SET_A((attr_type_t)attr_type, 1);
            }
        }
    } else if (type == 1 && win) {
    }

    return 0;
}

/** 
 * @brief 统计任务信息
 * 
 * @param player    
 * @param task_id   任务id 
 * @param oper_type 任务操作类型commonproto::task_oper_type_t
 * @param step      任务当前完成步骤
 * 
 * @return 
 */
int TaskUtils::stat_task_info(
        player_t *player, uint32_t task_id, uint32_t oper_type, uint32_t step)
{
    if (player == NULL ) {
        return 0;
    }

    const task_conf_t* task_conf = TaskUtils::get_task_config(task_id);
    if(NULL == task_conf) {
        return 0;
    }

    // 统计接取任务
    if (oper_type == commonproto::TASK_OPER_TYPE_ACCEPT) {
        if (task_conf->type == commonproto::TASK_TYPE_MAIN) {
            std::string stat_task_info = 
                Utils::to_string(task_conf->task_id) + "_" + task_conf->task_name;
            g_stat_logger->accept_task(
                    StatLogger::task_story, Utils::to_string(player->userid), 
                    stat_task_info, GET_A(kAttrLv));
        } else if (task_conf->type == commonproto::TASK_TYPE_BRANCH) {
            std::string stat_task_info = 
                Utils::to_string(task_conf->task_id) + "_" + task_conf->task_name;
            g_stat_logger->accept_task(
                    StatLogger::task_supplement, Utils::to_string(player->userid), 
                    stat_task_info, GET_A(kAttrLv));
        }             

        std::string stat_task_info = 
            Utils::to_string(task_conf->task_id) + "_接取_" + task_conf->task_name;
        Utils::write_msglog_new(player->userid, "基本", "任务", stat_task_info);
    }

    // 统计放弃任务
    if (oper_type == commonproto::TASK_OPER_TYPE_ABANDON) {
        if (task_conf->type == commonproto::TASK_TYPE_MAIN) {
            std::string stat_task_info = 
                Utils::to_string(task_conf->task_id) + "_" + task_conf->task_name;
            g_stat_logger->abort_task(
                    StatLogger::task_story, Utils::to_string(player->userid), 
                    stat_task_info, GET_A(kAttrLv));
        } else if (task_conf->type == commonproto::TASK_TYPE_BRANCH) {
            std::string stat_task_info = 
                Utils::to_string(task_conf->task_id) + "_" + task_conf->task_name;
            g_stat_logger->abort_task(
                    StatLogger::task_supplement, Utils::to_string(player->userid), 
                    stat_task_info, GET_A(kAttrLv));
        } 

        std::string stat_task_info = 
            Utils::to_string(task_conf->task_id) + "_放弃_" + task_conf->task_name;
        Utils::write_msglog_new(player->userid, "基本", "任务", stat_task_info);
    }


    // 统计完成或者完成第几步
    if (oper_type == commonproto::TASK_OPER_TYPE_COMPLETE) {
        if (step > 0) {
            std::string stat_task_info = 
                Utils::to_string(task_conf->task_id) + "_结束第" + 
                Utils::to_string(step) + "步_" + task_conf->task_name;
            Utils::write_msglog_new(player->userid, "基本", "任务", stat_task_info);
        }  

        if (TaskUtils::is_task_finished(player, task_id)) {
            if (task_conf->type == commonproto::TASK_TYPE_MAIN) {
                std::string stat_task_info = 
                    Utils::to_string(task_conf->task_id) + "_" + task_conf->task_name;
                g_stat_logger->finish_task(
                        StatLogger::task_story, Utils::to_string(player->userid), 
                        stat_task_info, GET_A(kAttrLv));
            } else if (task_conf->type == commonproto::TASK_TYPE_BRANCH) {
                std::string stat_task_info = 
                    Utils::to_string(task_conf->task_id) + "_" + task_conf->task_name;
                g_stat_logger->finish_task(
                        StatLogger::task_supplement, Utils::to_string(player->userid), 
                        stat_task_info, GET_A(kAttrLv));
            }

            std::string stat_task_info = 
                Utils::to_string(task_conf->task_id) + "_完成_" + task_conf->task_name;
            Utils::write_msglog_new(player->userid, "基本", "任务", stat_task_info);
        } 
    }

    return 0;
}

/** 
 * @brief 统计特定主线任务数据
 * 
 * @param player 
 * @param dup_id 
 * @param boss_id 
 * @param dup_status 
 * 
 * @return 
 */
int TaskUtils::stat_dup_story_task(
        player_t *player, uint32_t dup_id, uint32_t boss_id, uint32_t dup_status)
{
    if (dup_id == 0) {
        return 0;
    }

    if (dup_status == STAT_STORY_TASK_STATUS_ENTER) {
        // 进入副本
        if (!TaskUtils::is_task_finished(player, 51003) && dup_id == 1) {
            Utils::write_msglog_new(player->userid, "任务", "主线前三副本", "主线中进入青木营地人数");
        } else if (!TaskUtils::is_task_finished(player, 51005) && dup_id == 2) {
            Utils::write_msglog_new(player->userid, "任务", "主线前三副本", "主线中进入青木据点人数");
        } else if (!TaskUtils::is_task_finished(player, 51007) && dup_id == 3) {
            Utils::write_msglog_new(player->userid, "任务", "主线前三副本", "主线中进入青木遗址人数");
        }
    } else if (dup_status == STAT_STORY_TASK_STATUS_PASS) {
        // 通关副本
        if (!TaskUtils::is_task_finished(player, 51003) && dup_id == 1) {
            Utils::write_msglog_new(player->userid, "任务", "主线前三副本", "主线中通关青木营地人数");
        } else if (!TaskUtils::is_task_finished(player, 51005) && dup_id == 2) {
            Utils::write_msglog_new(player->userid, "任务", "主线前三副本", "主线中通关青木据点人数");
        } else if (!TaskUtils::is_task_finished(player, 51007) && dup_id == 3) {
            Utils::write_msglog_new(player->userid, "任务", "主线前三副本", "主线中通关青木遗址人数");
        }
    } else if (dup_status == STAT_STORY_TASK_STATUS_BOSS) {
        // 与特定boss战斗
        if (!TaskUtils::is_task_finished(player, 51003) && dup_id == 1 && boss_id == 25014) {
            Utils::write_msglog_new(player->userid, "任务", "主线前三副本", "主线中与卡尔比战斗人数");
        } else if (!TaskUtils::is_task_finished(player, 51005) && dup_id == 2 && boss_id == 25007) {
            Utils::write_msglog_new(player->userid, "任务", "主线前三副本", "主线中与帝国卫兵队长战斗人数");
        } else if (!TaskUtils::is_task_finished(player, 51007) && dup_id == 3 && boss_id == 25021) {
            Utils::write_msglog_new(player->userid, "任务", "主线前三副本", "主线中与阿离战斗人数");
        }
    }
    return 0;
}

/** 
 * @brief 设置任务完成
 * 
 * @param player 
 * @param task_id 
 * @param reward_flag true 发奖励 false 不发奖励
 * 
 * @return 
 */
int TaskUtils::set_task_finished(player_t *player, uint32_t task_id, bool reward_flag)
{
    const task_conf_t* task_conf =  g_task_conf_mgr.find_task_conf(task_id);
    if (task_conf == NULL) {
        return 0;
    }
    uint32_t step_count = task_conf->step_count; 
    std::vector<task_t> task_vec;
    task_t task_info;
    task_info.task_id = task_id;
    for (uint32_t i = 0; i < step_count;i++) {
        step_t step_info;
        task_info.step_list.push_back(step_info);
    }
    task_vec.push_back(task_info);

    //当可以领取奖励时
    for (uint32_t i = 1; i <= step_count + 1; i++ ){
        if (TaskUtils::is_accept_task_bonus(player, task_id, step_count)) {
            //设置标志位
            for (uint32_t i = 0; i < task_vec.size(); i++) {
                task_vec[i].bonus_status = kTaskBonusAccept;
                task_vec[i].done_times += 1;
            }

            if (reward_flag) {
                //读奖励列表
                std::vector<std::string> args_list = split(task_conf->bonus_id, ',');
                std::vector<uint32_t> prize_list;
                FOREACH(args_list, it) {
                    prize_list.push_back(atoi_safe((*it).c_str()));
                }

                //发奖
                onlineproto::sc_0x0112_notify_get_prize msg;
                int ret = 0;
                for (uint32_t i = 0; i < prize_list.size(); i++) {
                    ret = transaction_proc_prize(player, prize_list[i], msg);
                    if (ret) {
                        return ret;
                    }
                }
            }
        }

        //更新任务信息到内存和db
        int ret = TaskUtils::update_task_and_sync_db(player, task_vec, i, false);
        if (ret != 0) {
            return send_err_to_player(player, player->cli_wait_cmd, ret); 
        }
    }

    onlineproto::sc_0x0403_complete_task noti_out_;
    noti_out_.set_task_id(task_conf->task_id);    
    send_msg_to_player(player, cli_cmd_cs_0x0403_complete_task, noti_out_);
    return 0;
}

int TaskUtils::task_update_to_config(player_t *player)
{
    if (player == NULL || player->task_info == NULL) {
        return 0;
    }

    std::vector<uint32_t> auto_task_ids;
    const std::map<uint32_t, task_conf_t> conf_map = g_task_conf_mgr.const_task_conf_map();
    FOREACH(conf_map, iter) {
        if (iter->second.abandon_flag == 0 && 
                iter->second.auto_finish == 0) {
            continue;
        }

        if (iter->second.abandon_flag) {
            // 删除废弃任务数据
            TaskUtils::del_task_record(player, iter->first);
        } else if (iter->second.auto_finish && 
                !TaskUtils::is_task_finished(player, iter->second.task_id)) {
            auto_task_ids.push_back(iter->first);
        }
    }

    // 遍历需要自动完成的任务
    FOREACH(auto_task_ids, iter3) {
        task_conf_t *auto_task_conf = g_task_conf_mgr.find_task_conf(*iter3);
        if (auto_task_conf == NULL) {
            continue;
        }

        // 遍历玩家任务记录
        std::vector<task_t> task_list;
        player->task_info->get_all_tasks(task_list);
        FOREACH(task_list, iter2) {
            task_conf_t *player_task_conf = g_task_conf_mgr.find_task_conf((*iter2).task_id);
            if (player_task_conf == NULL) {
                continue;
            }

            bool p_flag = false;
            FOREACH(player_task_conf->parent_list, iter4) {
                if (*iter4 == *iter3) {
                    p_flag = true;
                }
            }
            // 当前配表任务是已完成任务的父任务,并且配置了自动完成
            if (TaskUtils::is_task_finished(player, player_task_conf->task_id) &&
                    p_flag) {
                // 设置任务完成
                if (auto_task_conf->auto_prize) {
                    TaskUtils::set_task_finished(player, *iter3, true);
                } else {
                    TaskUtils::set_task_finished(player, *iter3, false);
                }

                //设置父任务链完成
                FOREACH(auto_task_conf->parent_list, iter5) {
                    task_conf_t *p_auto_task_conf = g_task_conf_mgr.find_task_conf(*iter5);
                    if (p_auto_task_conf == NULL) {
                        continue;
                    }

                    if (p_auto_task_conf->auto_finish &&
                            !TaskUtils::is_task_finished(player, *iter5)) {
                        if (p_auto_task_conf->auto_prize) {
                            TaskUtils::set_task_finished(player, *iter5, true);
                        } else {
                            TaskUtils::set_task_finished(player, *iter5, false);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
