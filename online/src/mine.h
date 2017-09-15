#ifndef __MINE_H__
#define __MINE_H__
#include <stdint.h>
#include <common.h>

enum mine_attack_t {
	MINE_NOT_BE_ATTACKED = 0,
	MINE_BE_ATTACKED = 1,
};

enum mine_type_t {
	MINE_GOLD = commonproto::MINE_GOLD,
	MINE_EXP = commonproto::MINE_EXP,
	MINE_EFFORT = commonproto::MINE_EFFORT,
};

enum mine_size_t {
	SMALL_MINE = commonproto::SMALL_MINE,
	MID_MINE = commonproto::MID_MINE,
	BIG_MINE = commonproto::BIG_MINE,
};

enum start_occupy_step_t {
	CHECK_PET_EXPLOIT,
	CHECK_OlD_BTL_DATA,
};

enum mine_ownership_t {
	MINE_BELONGED_TO_ME,
	MINE_BELONGED_TO_OTHER,
};

enum search_step_t {
	MATCH_NEW_OCCUPYED_MINE,
	SEARCH_NEW_PET_INFO,
	JUST_REFRESH_PET_INFO,
	JUST_REFRESH_MINE_INFO,
};

enum mine_reward_type_t {
	MINE_TIME_IS_UP,
	MINE_IS_BE_OCCUPY,
	MINE_GIVE_UP,
};

const uint32_t NEED_SEARCH_MINE_COUNT = 5;
const uint32_t MATCH_MINE_ID_COUNT = 4;
const uint32_t MINE_HP_COE = 20;

struct pet_simple_info_t
{
	uint32_t pet_create_tm;
	uint32_t pet_cur_hp;
	uint32_t pet_power;
};

struct mine_info_t
{
	mine_info_t() {
		mine_id = 0;
		mine_type = MINE_GOLD;
		mine_size = SMALL_MINE;
		mine_create_tm = 0;
		user_id = 0;
		u_create_tm = 0;
		top_pet_power = 0;
		duration_time = 0;
		def_player_cnt = 0;
		is_been_attacked = 0;
		last_fight_time = 0;
		elem_id = 0;
	}
	uint64_t mine_id;
	uint32_t mine_create_tm;
	mine_type_t mine_type;
	mine_size_t mine_size;
	uint32_t user_id;
	uint32_t u_create_tm;
	uint32_t top_pet_power;
	uint32_t duration_time;
	uint32_t def_player_cnt;
	uint32_t is_been_attacked;
	uint32_t last_fight_time;
	uint32_t elem_id;
};

struct rob_resource_info_t
{
	uint32_t userid;
	uint32_t u_create_tm;
	uint32_t rob_id;
	uint32_t rob_num;
};

//采矿玩家的基础信息
struct team_simple_info_t
{
	team_simple_info_t() {
		userid = 0;
		u_create_tm = 0;
		exploit_start_time = 0;
		total_power = 0;
	}
	uint32_t userid;
	uint32_t u_create_tm;
	uint32_t exploit_start_time;
	uint32_t total_power;
};

typedef std::map<uint64_t, mine_info_t> MineTmpInfoMap;
typedef std::map<uint32_t, team_simple_info_t> TeamInfoMap;
typedef std::map<uint64_t, std::vector<pet_simple_info_t> > PetInfoMap;

class MineInfo
{
public:
	MineInfo() { clear(); }
	~MineInfo() { clear(); }
	inline void clear() {
		def_uid_key_ = 0;
		mine_id_ = 0;

		mine_id_vec_.clear();
		pet_info_map_.clear();
		mine_info_map_.clear();
		mine_tmp_info_map_.clear();
		fight_pets_ctm_.clear();
		def_pet_info_.clear();
		match_mine_info_.clear();
	}


	uint32_t add_mine_tmp_info_map(struct mine_info_t& mine_info);

	uint32_t save_mine_tmp_info_to_memory(uint64_t mine_id,
			struct mine_info_t& mine_info);

	uint32_t get_mine_tmp_info(uint64_t index, struct mine_info_t& mine_info);

	uint32_t erase_single_mine_from_mine_tmp_info(uint64_t mine_id);

	inline void get_mine_tmp_map(MineTmpInfoMap& mine_tmp_map) {
		mine_tmp_map = mine_tmp_info_map_;
	}

	uint32_t get_mine_info_from_memory(uint64_t mine_id, struct mine_info_t& mine_info);

	bool check_mine_id_exist_in_mine_info_map(uint64_t mine_id);

	uint32_t delete_pet_info_in_memory(uint64_t mine_id);

	uint32_t get_pet_info_from_memory(uint64_t mine_id, std::string& pet_info_pkg);

	uint32_t insert_my_mine_ids_to_memory(uint64_t mine_id);

	uint32_t delete_my_mine_id_from_memory(uint64_t mine_id);

	inline void save_match_mine_ids_to_memory(uint64_t mine_id) {
		match_mine_ids_.push_back(mine_id);
	}
	inline uint32_t delete_match_mine_ids_from_memory(uint64_t mine_id) {
		std::vector<uint64_t>::iterator it;
		it = std::find(match_mine_ids_.begin(), match_mine_ids_.end(), mine_id);
		if (it == match_mine_ids_.end()) {
			return cli_err_not_this_oppenent_mine_id;
		}
		match_mine_ids_.erase(it);
		return 0;
	}

	inline void get_match_mine_ids_from_memory(std::vector<uint64_t>& match_ids) {
		match_ids = match_mine_ids_;
	}
	inline bool find_mine_id_in_match_mine_ids(uint64_t mine_id) {
		return (std::find(match_mine_ids_.begin(), match_mine_ids_.end(), mine_id) != match_mine_ids_.end()) ? true : false;
	}

	uint32_t pack_match_mine_ids(commonproto::mine_id_list_t& pb_ids);

	uint32_t get_match_mine_info_from_memory(uint64_t mine_id,
			struct mine_info_t& mine_info);

	uint32_t pack_mine_ids(commonproto::mine_id_list_t& pb_ids);

	uint32_t get_mine_ids_from_memory(std::vector<uint64_t>& mine_ids);

	inline void clear_my_mine_ids() {
		mine_id_vec_.clear();
	}

	//清除矿的临时信息
	inline void clear_mine_tmp_info() {
		mine_tmp_info_map_.clear();
	}

	inline void clear_match_mine_ids() {
		match_mine_ids_.clear();
	}

	inline void clear_match_mine_info() {
		match_mine_info_.clear();
	}

	inline void set_search_step(search_step_t type) {
		search_step_ = type;
	}

	inline search_step_t get_search_step() {
		return search_step_;
	}

	inline void clear_start_fight_pet_ctm() {
		fight_pets_ctm_.clear();
	}
	inline void get_start_fight_pet_ctm(std::vector<uint32_t>& pet_create_tm) {
		pet_create_tm = fight_pets_ctm_;
	}
	inline void set_start_fight_pets_ctm(const std::vector<uint32_t>& pet_create_tm) {
		fight_pets_ctm_ = pet_create_tm;	
	}

	uint32_t save_matched_mine_info_to_memory(struct mine_info_t& mine_info);

	uint32_t insert_player_mine_info_to_memory(uint64_t mine_id,
			const struct mine_info_t& mine_info);

	inline void delete_player_mine_info_from_memory(uint64_t mine_id) {
		mine_info_map_.erase(mine_id);
	}

	inline void clear_player_mine_info_from_memory() {
		mine_info_map_.clear();
	}


public:
	std::string def_team_info_;
	std::string def_pet_info_; //保存下待揍的玩家pet_info
	std::string pet_hp_info_; //待删除
	uint64_t mine_id_; //正在攻打的矿id
	uint64_t def_uid_key_;	//正在攻打的防守方的玩家role_key

	//一轮矿战开始前，从redis中实时拉取出，目前正在防守的各队伍的精灵信息
	//键值为:  防守方的role_key
	PetInfoMap ai_pet_hp_map_;
	//每支队伍基础信息：包含:玩家id, 采矿时间，精灵战力总和
	std::vector<team_simple_info_t> def_team_vec_;
	//矿中玩家的战斗输赢状态 
	std::map<uint64_t, uint32_t> ai_state_map_;
	//该支队伍最近被揍的时间戳
	std::map<uint64_t, uint32_t> def_pk_time_map_;

private:
	//自己所拥有的所有矿的id
	std::vector<uint64_t> mine_id_vec_;
	//矿场中，我的开采队伍信息数据, 键值为: mine_id
	std::map<uint64_t, std::string> pet_info_map_;
	//我的矿场基本信息数据 键值为：mine_id
	std::map<uint64_t, mine_info_t> mine_info_map_;

	//我所匹配到的对手的矿id
	std::vector<uint64_t> match_mine_ids_;
	//我所匹配到的对手的矿的信息 key:mine_id
	std::map<uint64_t, mine_info_t> match_mine_info_;

	//临时存放新矿的信息
	//first: 未开采的新矿的临时id；second:矿的信息，此时还没有伙伴驻防
	MineTmpInfoMap mine_tmp_info_map_;


	//矿中每支队伍的开始采矿开始时间，用map的key弱排序来选择弃矿后的矿主
	//TeamInfoMap team_info_map_;
	search_step_t  search_step_;

	//参加进攻的精灵的 u_create_tm
	std::vector<uint32_t> fight_pets_ctm_;
};

#endif
