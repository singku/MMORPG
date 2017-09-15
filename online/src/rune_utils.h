#ifndef __RUNE_UTILS_H__
#define __RUNE_UTILS_H__
#include "rune.h"
#include "player.h"

class RuneUtils {
public:
	/*获得符文结构体信息*/
	static uint32_t get_rune(player_t *player, const uint32_t rune_id, rune_t& rune);
	/*db保存rune*/
	static uint32_t db_save_rune(player_t* player, const rune_t& rune);
	/*db删除符文 */
	static uint32_t db_del_rune(player_t* player, uint32_t rune_id);
	/*初始化符文*/
	static uint32_t init_rune(player_t* player, uint32_t conf_id, rune_t& rune, uint32_t level = 1);
	/*添加一个符文（包括更新内存以及数据库）*/
	static int add_rune(player_t *player, uint32_t conf_id, uint32_t level = 1, 
			get_rune_channel_t flag = GET_RUNE_IN_CALL_LEVEL);
	/*消耗经验瓶中经验来升级符文*/
	static uint32_t up_rune_grade_by_exp(player_t *player, uint32_t exp, rune_t& rune, uint32_t& bottle_exp);
	/*获得符文配表信息*/
	static uint32_t get_rune_conf_data(uint32_t conf_id, rune_conf_t& rune_conf);
	/*获得符文level下一级所需要的经验值(rune_exp.xml配表中符文等级对应的经验值要求)*/
	static uint32_t get_rune_exp(uint32_t rune_type, uint32_t level, uint32_t& exp);
	/*检查精灵装备的符文条件*/
	static int check_equit_rune(player_t *player, Pet* pet, rune_t& rune, uint32_t pos);

	/*@根据背包类型，判断该背包是否已满
	 */
	static uint32_t judge_rune_full_by_packet_type(player_t *player, rune_pack_type_t pack_type);
	/*根据符文经验，获得当前符文等级*/
	static uint32_t get_rune_level_by_exp(uint32_t rune_exp, uint32_t rune_color_type, uint32_t& rune_level);
	/**
	 * @brief 召唤符文TODO:delete
	 */
	//static uint32_t call_rune(player_t* player, uint32_t level, uint32_t& conf_id, uint32_t& open_flag, uint32_t& price);
	/**
	 *@brief 检查召唤条件
	 *@param player
	 *@param level
	 *@return 成功：0； 失败：返回错误码
	 */
	static uint32_t check_rune_call_condition(player_t *player, uint32_t level);
	/**
	 *@brief 在召唤阵level中概率性的获得符文conf_id
	 *@param player
	 *@param level
	 *@param conf_id
	 *@return 成功：0； 失败：返回错误码
	 */
	static uint32_t cal_rune_conf_id_on_calling(
			player_t* player, 
			uint32_t level, uint32_t& conf_id);
	/**
	 * @brief: 更新召唤阵
	 * @param: player
	 * @param: level: 当前的召唤等级
	 *@return 成功：0； 失败：返回错误码
	 */
	static uint32_t update_call_level_vec(player_t* player, uint32_t level);
	/**
	 * @brief: 符文改动前端通知包
	 * @param: player
	 * @param:rune_vec;使用create_rune_cli_t_vec函数创建
	 */
	static int sync_notify_rune_data_info(player_t *player, const std::vector<rune_cli_t>& rune_vec);
	/*符文属性改变引起的更新（内存，DB, 前端）*/
	static uint32_t alter_rune_info(player_t *player, rune_t& rune);
	/*删除一个符文（内存，DB, 前端）*/
	static uint32_t del_rune_info(player_t *player, uint32_t rune_id);

	// 已经废弃；使用RuneUtils::add_rune代替
	//static uint32_t create_rune_info(player_t *player, uint32_t conf_id);
	
	/**
	 * @brief: 创建符文数组:用于前端的通知包
	 * @param: rune
	 * @param: flag(1:获得符文；0：删除符文)
	 * @return 生成的数组
	 */
	static std::vector<rune_cli_t> create_rune_cli_t_vec(rune_t& rune, uint32_t flag);
	/**
	 *@brief 获得该类型符文背包的极限容量
	 *@param： pack_type 背包类型
	 *@return：rune_count_limit 该类型背包的符文极限数量 
	 */
	static uint32_t get_rune_limit_count_by_pack_type(const player_t *player, rune_pack_type_t pack_type);
	/**
	 *@brief 符文吞噬：获得移动符文，静止符文的信息
	 */
	static uint32_t rune_swallow_get_rune_info(player_t* player, uint32_t stand_id, uint32_t mov_id, rune_t& stand_rune, rune_t& mov_rune);
	/**
	 *@brief 符文吞噬：检查符文吞噬条件
	 */
	static uint32_t rune_swallow_check_mov_conditon(const rune_t& stand_rune, const rune_t& mov_rune);
	
	/**
	 *@brief 符文吞噬：符文吞噬：确定吞噬与被吞的符文，以及吞噬的符文增加经验，升级
	 */
	static uint32_t swallow_rule_cal(player_t* player, rune_t& stand_rune, rune_t& mov_rune);
	/**
	 *@brief 清除符文身上的pos位置,warning:仅用于两个吞噬的符文都在精灵身上!!!
	 *@param player
	 *@param rune 要被吞噬掉的符文
	 *@return 成功：0； 失败：返回错误码
	 */
	static uint32_t clean_rune_pos_in_pet(player_t* player, const rune_t& rune);
	/**
	 *@brief 符文吞噬: 刷新精灵信息
	 */
	static uint32_t update_pet_info(player_t* player, const rune_t& rune);
	/**
	 *@brief 更新精灵身上的符文id;warning:仅用于符文吞噬协议中：只有一个符文在精灵身上的吞噬情形!!!
	 *此刻，符文一定在后台代码中经过了交换
	 *@param player
	 *@param stand_rune 交换后的主动吞噬符文
	 *@param mov_rune 交换后的被吞噬的符文
	 *@return 成功：0； 失败：返回错误码
	 */
	static uint32_t update_rune_pos_in_pet(player_t* player, rune_t& stand_rune, const rune_t& mov_rune);
	
	//测试符文与精灵身上符文是否相同
	static uint32_t check_runes_func_same_or_not(player_t* player, uint32_t rune_id, Pet* pet, uint32_t stand_id = 0);

	static rune_attr_type_t get_rune_attr_type(player_t* player, uint32_t rune_id);
};

#endif
