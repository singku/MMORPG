#ifndef __PET_H__
#define __PET_H__

#include "common.h"
#include "pet_conf.h"

const uint32_t UNLOCK_LV = 40;

const uint32_t kMaxEquipRunesNum = 4;	//精灵最大装备符文个数(不能大于6，数据库中只预留6个)

class Pet
{
public:
    inline Pet() { clear(); }

public: //inline functions
    inline void clear() {
        pet_id_ = 0;
        level_ = 0;
        create_tm_ = 0;
        fight_pos_ = 0;
        is_excercise_ = false;
		exercise_pos_ = 0;
		exercise_tm_ = 0;
		last_add_exp_tm_ = 0;
        hp_ = 0;
        maxhp_ = 0;
        use_last_hp_ = false;
        exp_ = 0;
        loc_ = PET_LOC_STORE;
        power_ = 0;
        pet_bag_pos_ = 0;
        chisel_pos_ = 0;
        talent_level_ = kPetTalentLevelOne;
		rune_sp_pos_flag_ = 1;
        memset(effort_values_, 0, sizeof(effort_values_));
		memset(effort_lv_, 0, sizeof(effort_lv_));
        memset(anti_values_, 0, sizeof(anti_values_));
        memset(battle_values_, 0, sizeof(battle_values_));
        memset(battle_values_hide_, 0, sizeof(battle_values_hide_));
		rune_array_.clear();
		rune_array_.resize(kMaxEquipRunesNum, 0);
		state_.clear();
        total_swim_exp = 0;
        cur_swim_exp = 0;
        quality_ = 1;
		exped_cur_hp_ = 0;
		exped_flag_ = EXPED_NO_JOINED;
		mon_cris_hp_ = 0;
		mon_night_raid_hp_ = 0;
        tmp_max_hp_ = 0;
		mine_fight_hp_ = 0;
		mine_flag_ = 0;
		defend_mine_id_ = 0;
		pet_opt_attr.clear();
    }

	inline uint32_t tmp_max_hp() const { return tmp_max_hp_; }
	inline void set_tmp_max_hp(uint32_t hp) { tmp_max_hp_ = hp; }

	inline pet_exped_flag_t exped_flag() const { return exped_flag_; }
	inline void set_exped_flag(pet_exped_flag_t exped_flag) { exped_flag_ = exped_flag; }
	inline uint64_t pet_mine_flag() const { return mine_flag_;}
	inline void set_mine_flag(uint64_t mine_id) { mine_flag_ = mine_id; }
	inline uint64_t defend_mine_id() const { return defend_mine_id_; }
	inline void set_defend_mine_id(uint64_t mine_id) { defend_mine_id_ = mine_id; }
    inline uint32_t pet_id() const { return pet_id_; }
    inline void set_pet_id(uint32_t petid) { pet_id_ = petid; }
    inline uint32_t create_tm() const { return create_tm_; }
    inline void set_create_tm(uint32_t time) { create_tm_ = time; }
    inline uint32_t exp() const { return exp_; }
    inline void set_exp(uint32_t exp) { exp_ = exp; }
    inline uint32_t level() const { return level_;  }
    inline void set_level(uint32_t level) { level_ = level; }
    inline uint32_t cache_level() const { return cache_level_;  }
    inline void set_cache_level(uint32_t cache_level) { cache_level_ = cache_level; }
    inline uint32_t fight_pos() const { return fight_pos_; }
    inline void set_fight_pos(uint32_t fight_pos) { fight_pos_ = fight_pos; }
	inline uint32_t exercise_pos() const { return exercise_pos_; }
	inline void set_exercise_pos(uint32_t exercise_pos) { exercise_pos_ = exercise_pos; }
    inline bool is_excercise() const { return is_excercise_; }
    inline void set_is_excercise(bool is_excercise) { is_excercise_ = is_excercise; }
	inline uint32_t exercise_tm() const { return exercise_tm_;}
	inline void set_exercise_tm(uint32_t exercise_tm) { exercise_tm_ = exercise_tm; }
	inline uint32_t last_add_exp_tm() const { return last_add_exp_tm_;}
	inline void set_last_add_exp_tm(uint32_t last_add_exp_tm) { last_add_exp_tm_ = last_add_exp_tm_; }
    inline int hp() const { return hp_; }
    inline uint32_t quality() const { return quality_; }
    inline void set_quality(uint32_t quality) {
        quality_ = quality;
    }

    inline void set_hp(uint32_t hp) { 
        hp_ = hp; 
        if (hp_ > (int)tmp_max_hp()) {
            hp_ = (int)tmp_max_hp();
        }
    }

    inline uint32_t exped_cur_hp() const { return exped_cur_hp_;}
	inline void set_exped_cur_hp(uint32_t exped_cur_hp) {
		exped_cur_hp_ = exped_cur_hp;
		if (exped_cur_hp_ > (uint32_t)tmp_max_hp()) {
			exped_cur_hp_ = (uint32_t)tmp_max_hp();
		}
	}
	inline int mon_cris_hp() const { return mon_cris_hp_;}
	inline void set_mon_cris_hp(uint32_t mon_cris_hp) {
		mon_cris_hp_ = mon_cris_hp;
		if (mon_cris_hp_ > (int)tmp_max_hp()) {
			mon_cris_hp_ = (int)tmp_max_hp();
		}
	}

	inline int night_raid_hp() const { return mon_night_raid_hp_;}
	inline void set_night_raid_hp(uint32_t hp) {
		mon_night_raid_hp_ = hp;
		if (mon_night_raid_hp_ > (int)tmp_max_hp()) {
			mon_night_raid_hp_ = (int)tmp_max_hp();
		}
	}
	inline int mine_attack_hp() const { return mine_fight_hp_; }
	inline void set_mine_attack_hp(uint32_t hp) {
		mine_fight_hp_ = hp;
		if (mine_fight_hp_ > (int)tmp_max_hp()) {
			mine_fight_hp_ = (int)tmp_max_hp();
		}
	}
    inline bool is_use_last_hp() const { return use_last_hp_; }
    inline void set_use_last_hp(bool use_last_hp) { use_last_hp_ = use_last_hp; }
    inline int max_hp() const { return battle_values_[kBattleValueNormalTypeHp]; }
    inline pet_location_type_t loc() const { return loc_; }
    inline void set_loc(pet_location_type_t loc) { loc_ = loc; }
    inline uint32_t power() const { return power_; }
    inline void set_power(int val) { power_ = val; }
    inline uint32_t bag_pos() const { return pet_bag_pos_; }
    inline void set_bag_pos(uint32_t bag_pos) { pet_bag_pos_ = bag_pos; }
    inline uint32_t chisel_pos() const { return chisel_pos_; }
    inline void set_chisel_pos(uint32_t chisel_pos) { chisel_pos_ = chisel_pos; }
    inline int talent_level() const { return talent_level_; }
    inline void set_talent_level(int level) { talent_level_ = level; }
	inline uint32_t rune_specified_pos_flag() const { return rune_sp_pos_flag_; }
	inline void set_rune_sp_pos_flag(uint32_t pos_flag) { rune_sp_pos_flag_ = pos_flag; }
	//测试指定的符文格子解锁状态 0,1,2,3 四个格子；0号格子默认开启，无需检测
	inline bool test_rune_pos_flag(uint32_t pos) {
		if (!(pos >= 1 && pos <= 3)) {
			return false;
		}
		std::bitset<4> b((unsigned long)rune_sp_pos_flag_);
		if (b.test(pos)) {
			return true;
		}
		return false;
	}
	inline void unlock_rune_pos(uint32_t pos) {
		if (!(pos >= 1 && pos <= 3)) {
			return;
		}
		std::bitset<4> b((unsigned long)rune_sp_pos_flag_);
		if (!b.test(pos)) {
			b.set(pos);
		}
		rune_sp_pos_flag_ = b.to_ulong();
	}
	inline void set_first_pos_unlock() {
		std::bitset<4> b((unsigned long)rune_sp_pos_flag_);
		if (!b.test(0)) {
			b.set(0);
			rune_sp_pos_flag_ = b.to_ulong();
		}
	}
		
    inline void set_state(std::string state) {state_ = state;}
    inline std::string &state() { return state_;};
    inline uint32_t sum_battle_value() const {
        uint32_t sum = 0;
        for (uint32_t i = 0; i < kMaxBattleValueTypeNum; i++) {
            sum = sum + battle_values_[i];
        }
        return sum;
    }

    //对战中改变内存精灵的血量
    inline void change_hp(int hp) {
        hp_ += hp;
        if (hp_ < 0) {
            hp_ = 0;  
        }
        if (hp_ >= (int)tmp_max_hp()) {
            hp_ = tmp_max_hp();
        }
    }

	inline void change_mon_night_raid_hp(int hp) {
		mon_night_raid_hp_  += hp;
		if (mon_night_raid_hp_  < 0) {
			mon_night_raid_hp_  = 0;
		}
		if (mon_night_raid_hp_  >= (int)tmp_max_hp()) {
			mon_night_raid_hp_  = (int)tmp_max_hp();
		}
	}
    inline uint32_t effort_value(int idx) const {
        if (!(idx >= 0 && idx < (int)array_elem_num(effort_values_))) {
            return 0; 
        }
        return effort_values_[idx]; 
    }

	inline uint32_t effort_lv(int idx) const {
		if (!(idx >= 0 && idx < (int)array_elem_num(effort_lv_))) {
			return 0;
		}
		return effort_lv_[idx];
	}

	inline uint32_t effort_lv_sum() const {
        uint32_t sum = 0;
        for (int i = 0; i < (int)array_elem_num(effort_lv_); i++) {
            sum += effort_lv_[i];
        }
        return sum;
	}

    inline void set_effort_value(int idx, int val) {
        if (!(idx >= 0 && idx < (int)array_elem_num(effort_values_))) {
            return ; 
        }
        effort_values_[idx] = val;
    }

	inline void set_effort_lv(int idx, int val) {
		if (!(idx >=0 && idx <(int)array_elem_num(effort_lv_))) {
			return ;
		}
		effort_lv_[idx] = val;
	}

    inline int anti_value(int idx) const {
        if (!(idx >= 0 && idx < (int)array_elem_num(anti_values_))) {
            return 0; 
        }
        return anti_values_[idx]; 
    }
    inline void set_anti_value(int idx, int val) {
        if (!(idx >= 0 && idx < (int)array_elem_num(anti_values_))) {
            return ; 
        }
        anti_values_[idx] = val;
    }

    uint32_t get_effort_sum() {
        uint32_t sum = 0;
        for (int i = 0; i < (int)array_elem_num(effort_values_); ++i) {
            sum += effort_values_[i];
        }
        return sum;
    }

    inline uint32_t get_effort_lv_sum() {
        uint32_t sum = 0;
        for (int i = 0; i < (int)array_elem_num(effort_lv_); ++i) {
            sum += effort_lv_[i];
        }
        return sum;
    }

    inline uint32_t battle_value(int type) const {
        if (type >= 0 && type < (int)array_elem_num(battle_values_)) {
            return battle_values_[type];
        } else {
            return 0; 
        }
    }
    inline void add_battle_value(int type, int value) {
        uint32_t old_value = this->battle_value(type);
        this->set_battle_value(type, old_value + value);
    }
    inline void set_battle_value(int type, uint32_t val) {
        if (type >= 0 && type < (int)array_elem_num(battle_values_)) {
            battle_values_[type] = val;
        }
    }

    inline uint32_t battle_value_hide(int type) const {
        if (type >= 0 && type < (int)array_elem_num(battle_values_hide_)) {
            return battle_values_hide_[type];
        } else {
            return 0; 
        }
    }
    inline void set_battle_value_hide(int type, uint32_t val) {
        if (type >= 0 && type < (int)array_elem_num(battle_values_hide_)) {
            battle_values_hide_[type] = val;
        }
    }
    inline void add_battle_value_hide(int type, int value) {
        uint32_t old_value = this->battle_value_hide(type);
        this->set_battle_value_hide(type, old_value + value);
    }

	/*@获得精灵身上安装的符文id
	 */
	inline int get_rune_array(uint32_t idx) const {
		if (idx >= kMaxEquipRunesNum) {
			return -1;
		}
		return rune_array_[idx]; 
	}

	/*@设置rune_id对应的符文到精灵身上
	 */
	inline uint32_t set_rune_array(uint32_t idx, uint32_t rune_id) {
		if (idx >= kMaxEquipRunesNum) {
			return cli_err_pet_equip_rune_num;
		}
		rune_array_[idx] = rune_id;
		return 0;
	}
	
	/*@ brief根据id获得符文的安装位置, pos [0 至 size-1 ]
	 */
	inline uint32_t get_rune_idx_by_id(uint32_t rune_id, uint32_t& pos) {
		for (uint32_t i = 0; i < rune_array_.size() && i < kMaxEquipRunesNum; ++i) {
			if (rune_array_[i] == rune_id) {
				pos = i;
				return 0;
			}
		}
		return	cli_err_rune_id_not_equiped_in_pet; 
	}
	
	/* @brief获得精灵身上符文信息
	 */
	inline void get_rune_info_equiped_in_pet(std::vector<uint32_t>& rune_info) {
		rune_info = rune_array_;
	}

	/* @ brief获得精灵身上第二个符文格子解锁状态
	 */
	inline uint32_t get_rune_lock_flag_by_lv() {
		if (this->level() >= UNLOCK_LV) {
			return 1;
		}
		return 0;
	}
	
public:
    //int change_effort_value(int idx, int val, int* new_value);
    void calc_battle_value(player_t* player);
    int calc_power(player_t *player);

public:
    uint32_t total_swim_exp;
    uint32_t cur_swim_exp;
	std::string pet_opt_attr;
private:
    uint32_t power_; //战斗力
    uint32_t pet_id_; //精灵ID
    uint32_t create_tm_; //捕获时间
    uint32_t fight_pos_; //所在出战位
    bool is_excercise_; //是否锻炼
	//Confirm kevin(该字段不再代表锻炼位，高16位表示操作类型，低16位表示操作位置)
	uint32_t  exercise_pos_;  //
	uint32_t exercise_tm_;	//开始锻炼的时间戳（没有锻炼则为0）
	uint32_t last_add_exp_tm_;	//锻炼中最近一次加经验的时间戳
    uint32_t exp_; //当前级到下一级已经增加的经验
    uint32_t level_; //等级
    uint32_t cache_level_; //等级缓存
    pet_location_type_t loc_; //精灵所在位置
	//uint32_t rune_array_[kMaxEquipRunesNum];	//精灵安装符文的id
	std::vector<uint32_t> rune_array_;	//精灵安装符文的id

    int hp_; //当前血量
    int maxhp_; //最大血量
    bool use_last_hp_; //是否继承血量
    uint32_t effort_values_[kMaxEffortNum]; //学习力||改为特训经验值
	uint32_t effort_lv_[kMaxEffortNum];	//特训等级
    int talent_level_; //天赋等级
    int anti_values_[kMaxAntiNum]; //抗性

    int battle_values_[kMaxBattleValueTypeNum]; // 战斗数值 - 精灵天生数值(等级决定) + 符文 + 天赋 +学习力
    int battle_values_hide_[kMaxBattleValueHideTypeNum];//精灵隐藏属性。
    int chisel_pos_; //精灵刻印在的位置
    uint32_t pet_bag_pos_; // 所在精灵背包位置(暂时不用)
	//需要道具开启的符文格子,用位来标志比如：1110 ：表示3个道具格子都开启
	//1100:表示第3，4个格子开启，1000，表示第四个格子开启
	uint32_t rune_sp_pos_flag_;	//需要道具开启的符文格子,用位来标志比如：1110 ：表示3个
    uint32_t quality_;   // 精灵品质(颜色)

    string state_; //跟随精灵的当前状态
	uint32_t exped_cur_hp_;	//远征剩余血量
	pet_exped_flag_t exped_flag_;	//远征信息：1.参加远征；2.出战; 0.未参加远征
	int mon_cris_hp_;	//怪物危机精灵血量
	int mon_night_raid_hp_;	//夜袭精灵血量
    int tmp_max_hp_; //临时保存最大血量
	int mine_fight_hp_;	//矿战血量
	uint64_t mine_flag_;	//矿战时正在攻打的矿id
	uint64_t defend_mine_id_;	//该伙伴正在守的矿
};


#endif
