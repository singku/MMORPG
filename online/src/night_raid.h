#ifndef __NIGHT_RAID_H__
#define __NIGHT_RAID_H__
#include "common.h"

enum night_raid_cnf_data_t {
	NIGHT_RAID_HIGHEST_CARD_ID = 10, //夜袭高关卡id
	NIGHT_RAID_HIGHEST_GOT_REWARD_ID = 10, //最高领奖id
};

struct night_raid_info_t
{
	night_raid_info_t() {
		card_id = 0;
		user_id = 0;
		create_tm = 0;
		sex = 0;
		fight_value= 0;
		cur_hp = 0;
		max_hp = 0;
		pet_ids[0] = 0;//出战精灵的个数
	}
	uint32_t card_id;	//关卡id
	uint32_t user_id;	//挑战者的id
	uint32_t create_tm;
	uint32_t sex;
	char nick[50];
	uint32_t fight_value;
	// uint32_t level;
	uint32_t cur_hp;
	uint32_t max_hp;
	uint32_t pet_ids[10];
};

struct match_info_t{
	match_info_t(): low_score(0),high_score(0),unit_val(0){}
	uint64_t low_score;
	uint64_t high_score;
	uint64_t unit_val;
};
class NightRaid 
{
public:
	NightRaid(){
		cur_card_id = 0;
		dup_id = 402;
		cur_win_id = 0;
		btl_player_info_.clear();
		night_raid_info_map_.clear();
	}
	typedef std::map<uint32_t, night_raid_info_t> NightRaidInfoMap;	
public:
	//当前拉取到的对手信息 战斗信息的序列化格式
	std::string btl_player_info_;

	bool is_cur_phases_win_night_raid();
	bool is_frist_time_play_night_raid();
	uint32_t  get_cur_night_raid_battle_info(
			commonproto::battle_player_data_t* btl_info);
	uint32_t get_opponent_uids(std::set<uint64_t>& uids);
	uint32_t get_cur_opponent_uid();
	uint32_t add_new_card_night_raid_info(
		   	const commonproto::battle_player_data_t& player_info);
	uint32_t pack_night_raid_info_to_msg(
			const night_raid_info_t& night_raid_info,
			commonproto::pvep_match_data_t* op_ptr);
	uint32_t unpack_msg_to_night_raid_info(
			night_raid_info_t& night_raid_info,
			const commonproto::pvep_match_data_t& op_ptr);

	uint32_t  unpack_msg_list_to_night_raid_info_map(
			commonproto::pvep_opponent_list& op_ptr);
	uint32_t pack_cur_card_night_raid_info(
			commonproto::pvep_match_data_t* op_ptr);

	uint32_t pack_all_card_night_raid_info(
		commonproto::pvep_opponent_list& op_list);

	night_raid_info_t* get_night_raid_info_by_card_id(uint32_t card_id);
	uint32_t get_cur_power_percent();
	//当前解锁id
	// int set_night_raid_cur_card_id(player_t *player, uint32_t card_id);
	// //当前打赢的id
	// int set_night_raid_cur_win_id(player_t *player, uint32_t card_id);
	void get_opponent_match_args(match_info_t &data, uint32_t cur_power);
	inline void set_cur_win_id(uint32_t card_id) {
		cur_win_id = card_id;
	} 
	inline uint32_t get_cur_card_id(){return cur_card_id;}
	inline void set_cur_card_id(uint32_t card_id) {
		cur_card_id = card_id;
	} 
	uint32_t get_cur_id_from_attr_data(uint32_t data); 
	void clear_night_raid_map(){
		night_raid_info_map_.clear();
	}

private:
	 NightRaidInfoMap night_raid_info_map_;
	//当前正在打的关卡id
	uint32_t cur_card_id;
	//已经通关的最大的关卡id
	uint32_t cur_win_id;
	uint32_t dup_id;
};

#endif
