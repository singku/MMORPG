#ifndef TASK_INFO_H 
#define TASK_INFO_H

#include <stdio.h>
#include <stdint.h>
#include "proto.h"
#include "proto/db/dbproto.data.pb.h"
#include <map>
#include <vector>
#include "common.h"

/**
 * @brief  taskInfo 
 */

enum condition_type_t{
    TASK_COND_PLAYER_LEVEL = 1,
    TASK_COND_PLAYER_BATTLE_VALUE = 2,
    TASK_COND_PLAYER_PROFESSION = 3,
    TASK_COND_PET_NUM = 5,
    TASK_COND_PET_LEVEL = 6,//有伙伴到达了指定的等级
	TASK_COND_EFFORT  = 8,
	TASK_COND_RUNE = 9,
    TASK_COND_ATTR = 10,
    TASK_COND_PASS_DUP = 11,
    TASK_COND_COLOR_EQUIP_NUM = 13,
    TASK_COND_STAR_PET_NUM = 14,
    TASK_COND_PET_QUALITY_NUM = 15,
	TASK_COND_PET_GET_X_STAR = 17,
	TASK_COND_PET_GET_X_RUNE = 18,
	TASK_COND_PET_GET_X_EFFORT = 19,
	TASK_COND_PET_DIJU_AWAKE = 20,	//指定的伙伴帝具觉醒
	TASK_COND_PET_DIJU_LV = 21,	//指定伙伴帝具到达的等级
	TASK_COND_THIS_PET_LEVEL = 22,	//指定伙伴升到指定的等级
};

enum task_id_type_t{
   REWARD_TASK_ID_START = 79999, 
   REWARD_TASK_ID_END  = 89999,
};

// 悬赏任务id
enum reward_task_item_t {
    REWARD_TASK_ITEM_ONLINE_REWARD              = 80001,
    REWARD_TASK_ITEM_GOLD_TREASURE              = 80003,
    REWARD_TASK_ITEM_RECRUIT                    = 80002,
    REWARD_TASK_ITEM_DIVING                     = 80017,
    REWARD_TASK_ITEM_ESCORT                     = 80018,

    REWARD_TASK_ITEM_NORMAL_DUP                 = 80004,
    REWARD_TASK_ITEM_ELITE_DUP                  = 80005,
    REWARD_TASK_ITEM_KING_FIELD                  = 80007,
    REWARD_TASK_ITEM_ELEM_FIGHT                  = 80006,
    REWARD_TASK_ITEM_MONSTER_CRISIS              = 80009,

    REWARD_TASK_ITEM_RUNE_CALL                  = 80011,
    REWARD_TASK_ITEM_PET_EFFORT                 = 80012,
    REWARD_TASK_ITEM_PET_WAKEN_UP               = 80014,
    REWARD_TASK_ITEM_PET_STAR_UP                = 80015,
    REWARD_TASK_ITEM_EXP_FRUIT                  = 80008,

    REWARD_TASK_ITEM_CHALLENGE_RANK             = 80010,
    REWARD_TASK_ITEM_BUCKET_DUP                 = 80020,
    REWARD_TASK_ITEM_EXPDITION                  = 80019,    
    REWARD_TASK_ITEM_WORLD_BOSS                 = 80013,
    REWARD_TASK_ITEM_CHALLENGE_ARENA            = 80016, 

	REWARD_TASK_ITEM_PPVE						= 80022,
	REWARD_TASK_ITEM_NIGHT_RAID					= 80023,
	REWARD_TASK_ITEM_GRATEFULL_HALL				= 80024,
	REWARD_TASK_ITEM_MEDAL_HALL					= 80025,
	REWARD_TASK_ITEM_DAILY_SHARE				= 80026,
	REWARD_TASK_BUSINESS_STREET					= 80027,
};

struct task_bonus_t
{
    uint32_t item_id;   //奖励物品id
    uint32_t item_count;//奖励物品数量
};

//条件配置
struct condition_conf_t
{
    condition_conf_t() {
        id = 0;
        type = TASK_COND_PASS_DUP;
        memset(params, 0x0, sizeof(params));
        operate = 0;
    }
    uint32_t id;
	condition_type_t type;
	char params[128];
	uint32_t operate;
};
//步骤配置
struct step_conf_t
{
    step_conf_t() {
        step_id = 0;
        func_type = 0;
    }
    uint32_t step_id;
    uint32_t func_type;//操作类型
    //uint32_t func_args;//采集数量，打怪数量等
    std::vector<uint32_t> func_args;//采集数量，打怪数量等
    std::vector<uint32_t> parent_list;   //步骤跳转依赖
    std::vector<condition_conf_t> complete_con_vec; //完成该步骤条件列表
    std::vector<std::string> complete_con_exp_vec; //完成该步骤的条件表达式
};


//任务配置
struct task_conf_t
{
    task_conf_t() {
        task_id = 0;
        memset(task_name, 0x0, sizeof(task_name));
        type = 0;
        repeate = 1;
        step_count = 0;
        memset(bonus_id, 0x0, sizeof(bonus_id));
        daily_reset = 0;
        start_time = 0;
        end_time = 0;
		multi_condition = 0;
    }
    uint32_t task_id;
    char task_name[128];
	uint32_t type;//任务类型
    uint32_t repeate; // 任务可重复次数 默认1
    uint32_t step_count;//任务完成所需总步数
	char bonus_id[128];//完成后奖励物品id
    uint32_t daily_reset; // 每日重置标志 0 不重置 1重置
    uint32_t start_time;
    uint32_t end_time;
	uint32_t multi_condition; //任务完成需要多个子条件同时完成： 0：不需要；1：需要

    uint32_t abandon_flag;
    uint32_t auto_finish;
    uint32_t auto_prize;
    std::vector<uint32_t> parent_list; //解锁所需任务列表
    std::vector<condition_conf_t> accept_con_vec; //接受任务条件列表
    std::vector<std::string> accept_con_exp_vec; //接受任务条件表达式
    std::vector<step_conf_t> step_conf_vec; //任务步骤配置信息
    std::map<uint32_t, uint32_t> step_jump_map; // 跳过任务步骤信息
};

//悬赏任务配置
struct reward_task_conf_t
{
    reward_task_conf_t() {
        id = 0;
        task_id = 0;
        level = 0;
        attr_id = 0;
		score = 0;
    }

    uint32_t id;            // 悬赏任务id
    uint32_t task_id;       // 对应的tasks.xml任务id
    uint32_t level;
    uint32_t attr_id;
	uint32_t score;
};

//数据库存入内存中的步骤结构
struct step_t
{
    step_t() {
        step = 0;
        step_args = 0;
        condition_status = 0;
    }
    uint32_t step;         //步骤id
    std::string step_buff; //客户端使用 只在任务放弃时清掉
    uint32_t step_args;    //保存状态step信息 如采集
    //pro_status_t status;   // 0 分支未完成 1分支完成 2奖励已领取
	uint32_t condition_status; //条件完成情况
};

enum bonus_status_t{
    kTaskBonusUnAccept = 0, //任务奖励未领取
    kTaskBonusAccept = 1, //任务奖励已领取
};

//数据库存入内存中的任务结构
struct task_t
{
    task_t() {
        task_id = 0;
        status = 0;
        done_times = 0;
        bonus_status = kTaskBonusUnAccept;
		fin_statue_tm = 0;
    }
    uint32_t task_id;
    //uint32_t pro_id;
	uint32_t status;
	uint32_t done_times;
	std::vector<step_t> step_list;
	bonus_status_t bonus_status;
	uint32_t fin_statue_tm;
};



//task配表信息的管理类
class task_conf_mgr_t {
public:
    task_conf_mgr_t() {
        clear();
    }
    ~task_conf_mgr_t() {
        clear();
    }
    inline void clear() {
        task_conf_map_.clear();
    }
    inline const std::map<uint32_t, task_conf_t> &const_task_conf_map() const {
        return task_conf_map_;
    }
    inline void copy_from(const task_conf_mgr_t &m) {
        task_conf_map_.clear();
        task_conf_map_ = m.const_task_conf_map();
    }
    inline bool is_task_conf_exist(uint32_t task_id) {
        if (task_conf_map_.count(task_id) > 0) return true;
        return false;
    }
    inline bool add_task_conf(const task_conf_t &task) {
        if (is_task_conf_exist(task.task_id)) return false;
        task_conf_map_[task.task_id] = task; return true;
    }

    inline bool is_task_conf_abandon(uint32_t task_id) {
        task_conf_t * conf = find_task_conf(task_id);
        if (conf == NULL) {
            return true;
        }
        
        if (conf->abandon_flag) {
            return true;
        }
        return false;
    }

    inline task_conf_t *find_task_conf(uint32_t task_id) {
        if (!is_task_conf_exist(task_id)) return 0;
        return &((task_conf_map_.find(task_id))->second);
    }

private:
    std::map<uint32_t, task_conf_t> task_conf_map_;
};

// 悬赏任务配表信息的管理类
class reward_task_conf_mgr_t {
public:
    reward_task_conf_mgr_t() {
        clear();
    }
    ~reward_task_conf_mgr_t() {
        clear();
    }
    inline void clear() {
        reward_task_conf_map_.clear();
    }
    inline uint32_t max_reward_task_id() {
        return reward_task_conf_map_.size();
    }
    const inline std::map<uint32_t, reward_task_conf_t> &const_reward_task_conf_map() const {
        return reward_task_conf_map_;
    }

    inline void copy_from(const reward_task_conf_mgr_t &m) {
        reward_task_conf_map_ = m.const_reward_task_conf_map();
    }
    inline bool is_reward_task_conf_exist(uint32_t task_id) {
        if (reward_task_conf_map_.count(task_id) > 0) return true;
        return false;
    }
    inline bool add_reward_task_conf(const reward_task_conf_t &reward_task) {
        if (is_reward_task_conf_exist(reward_task.task_id)) return false;
        reward_task_conf_map_[reward_task.task_id] = reward_task; return true;
    }

    inline reward_task_conf_t *find_reward_task_conf(uint32_t task_id) {
        if (!is_reward_task_conf_exist(task_id)) return 0;
        return &((reward_task_conf_map_.find(task_id))->second);
    }

    inline void print_reward_task_info() {
		FOREACH(reward_task_conf_map_, it) {
			uint32_t id = it->second.id;
			uint32_t task_id = it->second.task_id;
			uint32_t level = it->second.level;
			uint32_t attr_id = it->second.attr_id;
			TRACE_TLOG("load config reward task:[%u],[%u],[%u],[%u]", 
					id, task_id, level, attr_id);
		}
		
	}
private:
    std::map<uint32_t, reward_task_conf_t> reward_task_conf_map_;
};

//内存中任务信息的管理类
class TaskInfo
{
public:
    TaskInfo();
    ~TaskInfo();

    int put_task(const task_t& task_data);
    int get_all_tasks(std::vector<task_t>& task_list);
    int get_all_tasks_id_list(std::vector<uint32_t>& task_id_list);
    int get_task(uint32_t task_id, std::vector<task_t>& task_list);
    int get_finish_task_id_list(std::vector<uint32_t>& task_id_list);
    void set_task(const task_t& task_data); 
    void del_task(uint32_t task_id);
    bool is_in_task_list(uint32_t task_id);
    bool is_in_task_list(uint32_t task_id, uint32_t pro_id);
    
private:
    typedef  std::map<uint32_t, task_t > TaskInfoMap;
	std::map<uint32_t, task_t> task_infos_;
};

#endif
