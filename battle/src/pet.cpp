#include <math.h>
#include "pet_conf.h"
#include "pet.h"
#include "builder_conf.h"
#include "global_data.h"

int Pet::calc_power()
{
    //NOTI(singku) 精灵战斗力为五项基本战斗数值的和
    calc_battle_value();
    uint32_t sum = 0;
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
    this->set_power((int)sum);
    return sum;
}

//野怪才计算
void Pet::calc_battle_value()
{
    //NOTI(singku) 精灵战斗数值包括基础战斗数值及隐藏战斗数值
    //基础战斗数值为 生命、普攻、普防、技攻、技防
    //隐藏战斗数值为 命中、闪避、暴击、防爆、格挡、破格、攻速
    //基础战斗数值= 天生基础值 + 等级*base_成长率*1.30^(t_lv) + 学习力*成长点
    //隐藏战斗数值= 天生基础值 + 等级*base_成长率*1.30^(t_lv)

    const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id_);
    const builder_conf_t *builder_conf = g_builder_conf_mgr.find_builder_conf(pet_id_);
    if (pet_conf) {
        this->req_power_ = pet_conf->req_power;
        this->max_dp_ = pet_conf->max_dp;
    } else {
        this->req_power_ = builder_conf->req_power;
        this->max_dp_ = builder_conf->max_dp;
    }

    if (!pet_conf && !builder_conf) {
        return ; 
    }
    static float effort_grow_point[kMaxBattleValueTypeNum] = {
         3, 1, 1, 1, 1
    };
	if (pet_conf) {
		//应斌斌明星副本动态boss血量的需求，
		//max_dp也调整为动态。
		//规则如下：
		//当mon_type=0或者1，max_dp始终为0（即不存在盾条）
		//当mon_type=2，
		//max_dp的值如果未填写 或 值为0，则max_dp值为boss的最大hp × 0.42
		//max_dp的值如果填写>0,则max_dp的值即为填写的值。
		if (pet_conf->mon_type != MON_TYPE_BOSS) {
			this->set_max_dp(0);
		} else {
			if (pet_conf->max_dp == 0) {
				this->set_max_dp(this->max_hp() * 0.42);
			}
		}
	}

    uint32_t talent_level;
    if (talent_level_ <= kPetTalentLevelNone) {
        talent_level = 0;
    } else {
        talent_level = talent_level_ - 1;
    }
	float conf1_value[5] = {9.23, 0.62, 0.62, 0.29, 0.29};
	float conf2_value[5] = {4.0, 5.0, 5.0, 5.0, 5.0};
	float conf3_value[5] = {-56.80, 16.59, 16.59, 9.40, 9.40};
	uint32_t mon_type = 0;
	if (pet_conf) {
		mon_type = pet_conf->mon_type;
		if (mon_type > MON_TYPE_BOSS) {
			mon_type = MON_TYPE_BOSS;
		}
	}
	float conf4_value[][5] = { {1, 1, 1, 1, 1},
							   {2, 1.6, 1.6, 1.6, 1.6},
								{19, 1.6, 1.6, 1.6, 1.6} };
    for (int i = 0; i < kMaxBattleValueTypeNum && i < 5; ++i) {
        float bn;
        float bng;
        if (pet_conf) {
            //bn = pet_conf->basic_normal_battle_values[i];
            //bng = pet_conf->basic_normal_battle_values_grow[i];
			TRACE_TLOG("zjun0723,pet_conf_level=%u,level=%u,pet_id_=%u", pet_conf->level, this->level(), pet_id_);
			float M = conf1_value[i] * pow(this->level() + conf2_value[i], 2) + conf3_value[i]; 
			float N = ceil(pet_conf->extra_battle_values[i] / 100);
			float L = conf4_value[mon_type][i];
			float Y = pet_conf->basic_normal_battle_values[i];
			battle_values_[i] = M * N * L + Y;
			TRACE_TLOG("zjun0723,M=%f,N=%f,L=%f,Y=%f, i=%d,conf1=%f,conf2=%f,conf3=%f,conf4=%f,mon_type=%u,btl_val=%d", 
					M, N, L, Y, i, conf1_value[i], conf2_value[i], conf3_value[i], conf4_value[mon_type][i], mon_type, battle_values_[i]);
        } else {
            bn = builder_conf->basic_normal_battle_values[i];
            bng = builder_conf->basic_normal_battle_values_grow[i];
			battle_values_[i] = bn + bng/100.0 * level_  * (0.7+ 0.3 * talent_level) + 
				(float)effort_values_[i] * effort_grow_point[i];
        }
		/*
        battle_values_[i] = bn + bng/100.0 * level_  * (0.7+ 0.3 * talent_level) + 
            (float)effort_values_[i] * effort_grow_point[i];
		*/
    }

    //根据精灵特性获得的隐藏属性
    for (int i = 0; i < kMaxBattleValueHideTypeNum; ++i) {
        float bh;
        float bhg;
        if (pet_conf) {
            bh = pet_conf->basic_hide_battle_values[i];
            bhg = pet_conf->basic_hide_battle_values_grow[i];
        } else {
            bh = builder_conf->basic_hide_battle_values[i];
            bhg = builder_conf->basic_hide_battle_values_grow[i];
        }
        battle_values_hide_[i] = bh + bhg/100.0 * level_ * (0.7 + 0.3 * talent_level);
    }

    this->set_hp(max_hp());
}
