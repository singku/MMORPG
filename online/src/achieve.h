#ifndef __ACHIEVE_H__
#define __ACHIEVE_H__
#include "common.h"
#include "achieve_conf.h"

//class achieve_mgr_t;

struct achieve_info_t
{
	uint32_t id;
	uint32_t get_time;
};

class Achieve
{
public:
	typedef std::map<uint32_t, achieve_info_t>  AchieveMap;

	//成就是否已经获得 
	//id:成就id号
	bool is_exist_this_achieve(uint32_t achieve_id);
	//更新成就信息
	uint32_t add_achieves_info(player_t* player,
		const std::vector<achieve_info_t> &ach_info);
	//更新成就信息到内存
	uint32_t insert_achieves_to_memory(const std::vector<achieve_info_t> &ach_info);
	//同步一个成就id到db
	uint32_t save_achieves_to_db(player_t* player,
			const std::vector<achieve_info_t>& achv_vec,
			bool need_sync_cli = false);

	uint32_t pack_achieve_info_list(commonproto::achieve_info_list_t* pb_ptr);
	uint32_t get_achieve_step_info(uint32_t achieve_type, 
			commonproto::achieve_progress_list_t& ach_pro_list,
			uint32_t condition_value);

	uint32_t init_one_achieve(player_t* player,
			uint32_t achieve_id, uint32_t get_time,
			std::vector<achieve_info_t>& achv_vec);

	uint32_t add_prize_and_point_by_achieve_id(player_t* player,
			uint32_t prize_id, uint32_t achieve_point);
private:
	//已经获得的勋章信息
	AchieveMap achieve_map;
};


enum {
	ACHIEVE_TYPE_MIN = 1,
	ACHIEVE_TYPE_MAX,
};

#define ACHIEVE_FUN_ARG player_t *player, uint32_t type, commonproto::achieve_progress_list_t& ach_list
typedef int (*achieve_func) (ACHIEVE_FUN_ARG);

class AchieveRegisterFunc
{
public:
	void register_listen_achieve_func();
	void clear();
	void call_achieve_register_func(ACHIEVE_FUN_ARG);
	static int  check_achieve_condition(
			player_t* player,
			const achieve_mgr_t::AchieveIdVec* achieves,
			uint32_t value);
	static int check_achieve_condition_with_2_args(
			player_t* player,
			const achieve_mgr_t::AchieveIdVec* achieves,
			uint32_t arg1, uint32_t arg2);
private:
	std::map<uint32_t, achieve_func> achieve_func_map_;

	static int achieve_01_increase_power(ACHIEVE_FUN_ARG);
	static int achieve_02_increase_level(ACHIEVE_FUN_ARG);
	static int achieve_03_get_equip(ACHIEVE_FUN_ARG);
	static int achieve_05_get_pet_num(ACHIEVE_FUN_ARG);
	static int achieve_06_get_telent_pet(ACHIEVE_FUN_ARG);
	static int achieve_07_get_quality_pet(ACHIEVE_FUN_ARG);
	static int achieve_08_inc_effort(ACHIEVE_FUN_ARG);
};

#endif
