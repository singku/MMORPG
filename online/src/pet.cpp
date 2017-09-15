#include "pet_conf.h"
#include "pet.h"
#include "global_data.h"
#include "pet_utils.h"
#include "rune.h"
#include "rune_utils.h"
#include "player_utils.h"


/*
int Pet::change_effort_value(int idx, int val, int* new_value)
{
    if (idx < 0 || idx > (int)array_elem_num(effort_values_)) {
        return -1;    
    }
    int max_can_add = kMaxTotalEffortValue - (int)get_effort_sum();
    if (val > max_can_add) {
        val = max_can_add; 
    }
    int single_max_can_add =  kMaxSingleEffortValue - effort_values_[idx];
    if (val > single_max_can_add) {
        val = single_max_can_add; 
    }

    effort_values_[idx] += val;

    if (new_value) {
        *new_value = effort_values_[idx];
    }

    return 0;
}
*/

/*
int Pet::change_effort_value(int idx, int val, int* new_value)
{
    if (idx < 0 || idx > (int)array_elem_num(effort_values_)) {
        return -1;    
    }
	//获取当前等级的特训，升级需要的经验值
	uint32_t ef_lv = effort_lv(idx);
	uint32_t cur_val = effort_value(idx);
	uint32_t up_val = ceil(((ef_lv + 1) * (ef_lv + 1) + 10) / 10.0);
	uint32_t up_need_val = up_val - cur_val;
	//=====不能大于精灵自身等级=====
	if (this->level() == ef_lv) {
		uint32_t real_add = 0;
		if (val >= (int)up_need_val) {
			real_add = up_val - 1;
		} else {
			real_add = cur_val + val;
		}
		set_effort_value(idx, real_add);
		//返回没有用完的value
		return val - (real_add - cur_val);
	}
	//===============================
	if (val >= (int)up_need_val) {
		set_effort_lv(idx, ef_lv + 1);
		set_effort_value(idx, 0);
		return change_effort_value(idx, val - up_need_val, NULL);
	} else {
		set_effort_value(idx, effort_value(idx) + val);	
	}
	return 0;
}
*/

int Pet::calc_power(player_t* player)
{
    //NOTI(singku) 精灵战斗力为五项基本战斗数值的和
    calc_battle_value(player);
    const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(this->pet_id());
    if (!pet_conf) {
        return cli_err_pet_not_exist;;
    }
    uint32_t sum = 0;
    sum = pet_conf->basic_battle_value + level_ * 50 + talent_level_ * 1000
        + quality_ * 200 + this->effort_lv_sum() * 8;

    uint32_t rune_sum = 0;
	for (uint32_t i = 0; i < rune_array_.size(); i++) {
		rune_t rune;
		int ret = RuneUtils::get_rune(player, rune_array_[i], rune);	
		if (ret != 0) {
			continue;
		}
		if (rune.level == 0 || rune.level > kMaxRuneLv) {
			continue;
		}
		rune_conf_t* rune_conf_ptr = g_rune_conf_mgr.get_rune_conf_t_info(rune.conf_id);
		if (rune_conf_ptr == NULL) {
			continue;
		}
        uint32_t rune_val[] = {0, 20, 50, 100, 200}; //200暂时不用
        if ((uint32_t)rune_conf_ptr->rune_type >= (uint32_t)sizeof(rune_val)) {
            rune_sum += 0;        
        } else {
            rune_sum += rune_val[rune_conf_ptr->rune_type] * rune.level;
        }
	}
    sum += rune_sum;
    /*
    sum = 0.5 *  battle_values_[kBattleValueNormalTypeHp] +
        1.6 *  battle_values_[kBattleValueNormalTypeNormalAtk] +
        3.0 *  battle_values_[kBattleValueNormalTypeNormalDef] +
        1.5 *  battle_values_[kBattleValueNormalTypeSkillAtk] +
        1.8 *  battle_values_[kBattleValueNormalTypeSkillDef] +
        1.2 *  battle_values_hide_[kBattleValueHideTypeCrit] +
        0.8 *  battle_values_hide_[kBattleValueHideTypeAntiCrit] +
        0.8 *  battle_values_hide_[kBattleValueHideTypeHit] +
        1.2 *  battle_values_hide_[kBattleValueHideTypeDodge] +
        1.1 *  battle_values_[kBattleValueHideTypeBlock] +
        0.9 *  battle_values_[kBattleValueHideTypeBreakBlock];
    */
    this->set_power(sum);
    return sum;
}

void Pet::calc_battle_value(player_t* player)
{
    //NOTI(singku) 精灵战斗数值包括基础战斗数值及隐藏战斗数值
    //基础战斗数值为 生命、普攻、普防、技攻、技防
    //隐藏战斗数值为 命中、闪避、暴击、防爆、格挡、破格、攻速
    // 基础战斗数值=伙伴基础值 + 伙伴等级 * 对应属性成长率/100 *（ 0.7 + 当前星级 * 0.3） + 特训等级 * 特训每级提升数值 + 符文加成 +  （当前品级-1） * 0.04 * （伙伴基础值 + 110 * 对应属性成长率）
    // 隐藏战斗数值=伙伴基础值 + 伙伴等级 * 对应属性成长率/100 *（ 0.7 + 当前星级 * 0.3） + 特训等级 * 特训每级提升数值 + 符文加成 +  （当前品级-1） * 0.04 * （伙伴基础值 + 110 * 对应属性成长率）

    const pet_conf_t* pet_conf = PetUtils::get_pet_conf(pet_id_);

    if (pet_conf == NULL) {
        return ; 
    }

    //再加上符文的影响
	std::vector<uint32_t> tem_vec;
	get_rune_info_equiped_in_pet(tem_vec);
	int tem_battle_values_[kMaxBattleValueTypeNum];	
	int tem_battle_values_hide_[kMaxBattleValueHideTypeNum];
	memset(tem_battle_values_, 0, sizeof(tem_battle_values_));
	memset(tem_battle_values_hide_, 0, sizeof(tem_battle_values_hide_));
	uint32_t tem_vec_size = tem_vec.size();
	for (uint32_t i = 0; i < tem_vec_size; ++i) {
		rune_t rune;
		int ret = RuneUtils::get_rune(player, tem_vec[i], rune);	
		if (ret != 0) {
			continue;
		}
		if (rune.level == 0 || rune.level > kMaxRuneLv) {
			continue;
		}
		rune_conf_t* rune_conf_ptr = g_rune_conf_mgr.get_rune_conf_t_info(rune.conf_id);
		if (rune_conf_ptr == NULL) {
			continue;
		}
		/*理论上不会执行，因为碎片和能量不能装备到精灵身上*/
		if (rune_conf_ptr->rune_type == kRuneGray && rune_conf_ptr->rune_type == kRuneRed) {
			continue;
		}
		uint32_t tem_fun_type = rune_conf_ptr->fun_type;
		uint32_t tem_fun_value = rune_conf_ptr->fun_value;
		uint32_t tem_arg_rate = rune_conf_ptr->arg_rate;
		uint32_t add_value = tem_fun_value + (rune.level - 1) * tem_arg_rate;
		if (tem_fun_type >= 0 && tem_fun_type < kMaxBattleValueTypeNum) {
			tem_battle_values_[tem_fun_type] += add_value;
		} else if ((tem_fun_type - kMaxBattleValueTypeNum) <= kBattleValueHideTypeBreakBlock) {
			tem_battle_values_hide_[tem_fun_type - kMaxBattleValueTypeNum] += add_value;
		}
	}

    for (int i = 0; i < kMaxBattleValueTypeNum; ++i) {
        //battle_values_[i] = (float)pet_conf->basic_normal_battle_values[i] +
            //(float)pet_conf->basic_normal_battle_values_grow[i]/100.0 * level_  * pow(1.3, talent_level_) + 
            //(float)effort_values_[i] * g_effort_grow_point[i] + tem_battle_values_[i];

        battle_values_[i] = (float)pet_conf->basic_normal_battle_values[i] 
            + (float)pet_conf->basic_normal_battle_values_grow[i]/100.0 * level_  
            * (0.7 + 0.3 * talent_level_) 
            //+ (float)effort_values_[i] * g_effort_grow_point[i] 
            + (float)effort_lv_[i] * g_effort_grow_point[i] 
            + tem_battle_values_[i]
            + (quality_ - 1) * 0.04 * ((float)pet_conf->basic_normal_battle_values[i] 
                    + (float)pet_conf->basic_normal_battle_values_grow[i]/100.0 * 110);
    }

    //根据精灵特性获得的隐藏属性
    for (int i = 0; i < kMaxBattleValueHideTypeNum; ++i) {
        //battle_values_hide_[i] = pet_conf->basic_hide_battle_values[i] + 
            //pet_conf->basic_hide_battle_values_grow[i]/100.0 * level_ * pow(1.3, talent_level_) + tem_battle_values_hide_[i];
        battle_values_hide_[i] = pet_conf->basic_hide_battle_values[i] 
            + pet_conf->basic_hide_battle_values_grow[i]/100.0 * level_ 
            * (0.7 + 0.3 * talent_level_) 
            + tem_battle_values_hide_[i]
            + (quality_ - 1) * 0.04 * ((float)pet_conf->basic_normal_battle_values[i] 
                    + (float)pet_conf->basic_normal_battle_values_grow[i]/100.0 * 110);

        //double p = (double)battle_values_hide_[i] / pet_conf->basic_hide_battle_values_coeff[i];
        //uint32_t rate = p * 100 / (1 + p);
        //battle_values_hide_[i] = rate;
    }

    //TODO(singku)暴击加成和格挡加成默认值
    battle_values_hide_[kBattleValueHideTypeCritAffectRate] = 80;
    battle_values_hide_[kBattleValueHideTypeBlockAffectRate] = 40;
    //set_tmp_max_hp(max_hp());
}
