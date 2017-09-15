#ifndef __PET_H__
#define __PET_H__

#include "common.h"
#include "pet_conf.h"

class Pet
{
public:
    inline Pet() { clear(); }

public: //inline functions
    inline void clear() {
        pet_id_ = 0;
        create_tm_ = 0;
        hp_ = 0;
        maxhp_ = 0;
        level_ = 0;
        fight_pos_ = 0;
        power_ = 0;
        req_power_ = 0;
        max_dp_ = 0;
        talent_level_ = 0;
        x_pos_ = 0;
        y_pos_ = 0;
        heading_ = 0;
        elem_type_ = kPetElemTypeWater;
        exp_ = 0;
        exercise_pos_ = 0;
        chisel_pos_ = 0 ;
        loc_ = 1;
        rune_3_unlock_flag_ = 0;
        quality_ = 1;
        exercise_tm_ = 0;
        exped_cur_hp_ = 0;
        exped_flag_ = 0;
        mon_cris_hp_ = 0;
        last_add_exp_tm_ = 0;
        state_bytes_.clear();
        proto_rune_info_.clear();
        memset(effort_values_, 0, sizeof(effort_values_));
        memset(anti_values_, 0, sizeof(anti_values_));
        memset(battle_values_, 0, sizeof(battle_values_));
        memset(battle_values_hide_, 0, sizeof(battle_values_hide_));
		memset(effort_lv_, 0, sizeof(effort_lv_));
		mine_fight_hp_ = 0;
        mine_flag_ = 0;
        defend_mine_id_ = 0;
        pet_opt_attr.clear();

    }

    inline uint32_t pet_id() const { return pet_id_; }
    inline void set_pet_id(uint32_t petid) { pet_id_ = petid; }
    inline uint32_t create_tm() const { return create_tm_; }
    inline void set_create_tm(uint32_t time) { create_tm_ = time; }
    inline uint32_t level() const { return level_;  }
    inline void set_level(uint32_t level) { level_ = level; }
    inline uint32_t fight_pos() const { return fight_pos_;  }
    inline void set_fight_pos(uint32_t pos) { fight_pos_ = pos; }
    inline void set_elem_type(uint32_t type) { elem_type_ = (pet_elem_type_t)type; }
    inline pet_elem_type_t elem_type() { return elem_type_; }

    inline int hp() const { return hp_; }
    inline void set_hp(int hp) { hp_ = hp; }
    inline int max_hp() const { return battle_values_[kBattleValueNormalTypeHp]; }
    inline int set_max_hp(uint32_t maxhp) { 
        battle_values_[kBattleValueNormalTypeHp] = maxhp; 
        return 0;
    }
	inline int mine_attack_hp() const { return mine_fight_hp_; }
	inline void set_mine_attack_hp(uint32_t hp) {
        mine_fight_hp_ = hp; 
    }
	inline uint64_t pet_mine_flag() const { return mine_flag_;}
	inline void set_mine_flag(uint64_t mine_id) { mine_flag_ = mine_id; }
	inline uint64_t defend_mine_id() const { return defend_mine_id_; }
	inline void set_defend_mine_id(uint64_t mine_id) { defend_mine_id_ = mine_id; }

    inline uint32_t power() const { return power_; }
    inline void set_power(int val) { power_ = val; }
    inline uint32_t req_power() const { return req_power_; }
    inline void set_req_power(int val) { req_power_ = val; }
    inline int talent_level() const { return talent_level_; }
    inline uint32_t max_dp() const { return max_dp_; }
    inline void set_max_dp(int val) { max_dp_ = val; }
    inline void set_talent_level(int level) { talent_level_ = level; }
    inline int x_pos() const { return x_pos_; }
    inline void set_x_pos(int pos) { x_pos_ = pos; }
    inline int y_pos() const { return y_pos_; }
    inline void set_y_pos(int pos) { y_pos_ = pos; }
    inline int heading() const { return heading_; }
    inline void set_heading(int heading) { heading_ = heading; }
    inline const string &state_bytes() const { return state_bytes_; }
    inline void set_state_bytes(const string &state_bytes) { state_bytes_ = state_bytes; }
    inline const string &proto_rune_info() const { return proto_rune_info_; }
    inline void set_proto_rune_info(const string &proto_rune_info) { proto_rune_info_ = proto_rune_info; }

    inline void set_exp(uint32_t exp) { exp_ = exp; }
    inline uint32_t exp() { return exp_;}

    inline void set_exercise_pos(uint32_t pos) { exercise_pos_ = pos; }
    inline uint32_t exercise_pos() { return exercise_pos_;}

    inline void set_chisel_pos(uint32_t pos) { chisel_pos_ = pos; }
    inline uint32_t chisel_pos() { return chisel_pos_;}

    inline void set_loc(uint32_t loc) { loc_ = loc; }
    inline uint32_t loc() { return loc_;}

    inline void set_rune_3_unlock_flag(uint32_t flag) { rune_3_unlock_flag_ = flag; }
    inline uint32_t rune_3_unlock_flag() { return rune_3_unlock_flag_;}

    inline void set_quality(uint32_t q) { quality_ = q; }
    inline uint32_t quality() { return quality_;}

    inline void set_exercise_tm(uint32_t tm) { exercise_tm_ = tm; }
    inline uint32_t exercise_tm() { return exercise_tm_;}

    inline void set_last_add_exp_tm(uint32_t tm) { last_add_exp_tm_ = tm; }
    inline uint32_t last_add_exp_tm() { return last_add_exp_tm_;}

    inline void set_exped_cur_hp(uint32_t hp) { exped_cur_hp_ = hp; }
    inline uint32_t exped_cur_hp() { return exped_cur_hp_;}

    inline void set_exped_flag(uint32_t flag) { exped_flag_ = flag; }
    inline uint32_t exped_flag() { return exped_flag_;}

    inline void set_mon_cris_hp(uint32_t hp) { mon_cris_hp_ = hp; }
    inline uint32_t mon_cris_hp() { return mon_cris_hp_;}

    inline void change_hp(int hp) {
        hp_ += hp;
        if (hp_ < 0) {
            hp_ = 0;  
        }
        if (hp_ >= max_hp()) {
            hp_ = max_hp();
        }
    }

    inline int effort_value(int idx) const {
        if (!(idx >= 0 && idx < (int)array_elem_num(effort_values_))) {
            return 0; 
        }
        return effort_values_[idx]; 
    }
    inline void set_effort_value(int idx, int val) {
        if (!(idx >= 0 && idx < (int)array_elem_num(effort_values_))) {
            return ; 
        }
        effort_values_[idx] = val;
    }

	inline uint32_t effort_lv(int idx) const {
		if (!(idx >= 0 && idx < (int)array_elem_num(effort_lv_))) {
			return 0;
		}
		return effort_lv_[idx];
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

    inline uint32_t battle_value(int type) const {
        if (type >= 0 && type < (int)array_elem_num(battle_values_)) {
            return battle_values_[type];
        } else {
            return 0; 
        }
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

    inline void init(uint32_t pet_id, uint32_t pet_level, uint32_t create_tm) {
        clear();
        set_pet_id(pet_id);
        set_create_tm(create_tm);
        set_level(pet_level > kMaxPetLevel ?kMaxPetLevel :pet_level);
        /*set_level(pet_level);*/
        set_talent_level(kPetTalentLevelOne);
    }

public:
    //新刷的野怪需要计算这个
    void calc_battle_value();
    int calc_power();

public:
	std::string pet_opt_attr;

private:
    uint32_t power_; //战斗力
    uint32_t req_power_; //战力压制时用的战力
    uint32_t max_dp_; //最大破击能量
    uint32_t pet_id_; //精灵ID
    uint32_t create_tm_; //捕获时间
    uint32_t level_; //等级
    uint32_t fight_pos_; //出战位
    uint32_t exp_;
    uint32_t exercise_pos_;
    uint32_t chisel_pos_;
    uint32_t loc_;
    uint32_t rune_3_unlock_flag_;
    uint32_t quality_;
    uint32_t exercise_tm_;
    uint32_t exped_cur_hp_;
    uint32_t exped_flag_;
    uint32_t mon_cris_hp_;
    uint32_t last_add_exp_tm_;

    int hp_; //当前血量
    int maxhp_; //最大血量
    int effort_values_[kMaxEffortNum]; //学习力
	uint32_t effort_lv_[kMaxEffortNum];	//特训等级
    int talent_level_; //天赋等级
    int anti_values_[kMaxAntiNum]; //抗性

    int battle_values_[kMaxBattleValueTypeNum]; // 战斗数值 - 精灵天生数值(等级决定) + 符文 + 天赋 +学习力
    int battle_values_hide_[kMaxBattleValueHideTypeNum];//精灵隐藏属性。

    pet_elem_type_t elem_type_;
    uint32_t x_pos_;
    uint32_t y_pos_;
    uint32_t heading_;
    string state_bytes_;
    string proto_rune_info_;
	int mine_fight_hp_;
	uint64_t mine_flag_;
	uint64_t defend_mine_id_;
};

#endif
