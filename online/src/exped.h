#ifndef __EXPED_H__
#define __EXPED_H__
#include "common.h"

enum exped_com_data_t {
	EXPED_HP_COE = 10,	//精灵血量系数
	EXPED_HIGHEST_CARD_ID = 12, //远征最高关卡id
	EXPED_JOINED_PETS_NUM = 15,	//远征精灵最大选择数量
	EXPED_RESET_CNT_LIMIT = 1,	//伙伴激战每日重置次数限制
};

struct player;
struct exped_info_t
{
	exped_info_t() {
		card_id = 0;
		user_id = 0;
		create_tm = 0;
		kill_flag = false;
		sex = 0;
		level = 0;
		power = 0;
		proto_btl_pets.clear();
	}
	uint32_t card_id;	//关卡id
	uint32_t user_id;	//挑战者的id
	uint32_t create_tm;
	bool kill_flag;	//是否被我干掉
	uint32_t sex;
	char nick[50];
	uint32_t level;
	uint32_t power;
	std::string proto_btl_pets;		//对手战斗力最高的五个精灵基础信息
};

//远征中，跟我pk的的各路玩家的精灵信息
class Expedition 
{
public:
	Expedition() : cur_card_id_(0) {
		btl_pet_info_.clear();
	}
	typedef std::map<uint32_t, exped_info_t> ExpedInfoMap;	
public:
	//当前拉取到的对手最高战力的精灵 战斗信息的序列化格式
	std::string btl_pet_info_;

	//初始化所有关卡信息；（不包括未解锁的关卡）
	uint32_t init_all_cards_exped_info(const onlineproto::expedition_users_pet_info_list& exped_list);

	exped_info_t* get_exped_info_by_card_id(uint32_t card_id);

	uint32_t add_new_card_exped_info(const commonproto::battle_player_data_t &player_info);

	uint32_t pack_all_exped_info_to_pbmsg(
			onlineproto::expedition_users_pet_info_list* pb_exped_ptr);

	bool check_cur_card_kill_state ();

	bool check_kill_state_by_card(uint32_t card_id);

	uint32_t get_cur_card_id() {
		return cur_card_id_;
	}

	void set_killed() {
		exped_info_map_[cur_card_id_].kill_flag = true;
	}

	uint32_t pack_cur_card_exped_info(onlineproto::expedition_users_pet_info* pet_ptr);

	//修改ai精灵的血量，并保存至DB
	//Confirm kevin : ai胜利情况下调用
	uint32_t change_ai_pets_hp(player_t* player, 
			const onlineproto::expedition_pet_cur_hp_list& pb_pets_hp);

	//清零ai精灵的血量, 并保存至DB
	//Confirm kevin : 玩家自己胜利情况下调用
	uint32_t clear_ai_pets_hp(player_t* player);

	//重置关卡，清除所有对手信息
	uint32_t clear_exped_info (player_t* player);

	//获得这次远征中已经打过（包括正在打）的对手的米米号
	uint32_t get_exped_ai_uids(std::set<uint64_t>& uids);
private:
	void set_cur_card_id(uint32_t card_id) {
		cur_card_id_ = card_id;
	} 

	//添加一个对手的信息，打包到expedition_users_pet_info中
	uint32_t pack_one_ai_exped_info_to_pbmsg(
			const exped_info_t& exped_info,
			onlineproto::expedition_users_pet_info* pet_ptr);

private:
	ExpedInfoMap exped_info_map_;
	//当前正在打或者已经通关的最大的关卡id
	uint32_t cur_card_id_;
};

#endif
