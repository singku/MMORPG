#ifndef __PET_CONF_H__
#define __PET_CONF_H__

#include "common.h"

enum mon_type_t {
    MON_TYPE_NORMAL = 0,
    MON_TYPE_ELITE  = 1,
    MON_TYPE_BOSS   = 2,
};

// 精灵元素属性
enum pet_elem_type_t {
    kMinPetElemTypeValue = 1,
    kPetElemTypeWater = 1, // 水
    kPetElemTypeFire = 2, // 火 
    kPetElemTypeGrass = 3, // 草
    kPetElemTypeLight = 4, // 光
    kPetElemTypeDark = 5, // 暗
    kPetElemTypeGround = 6, // 地
    kPetElemTypeForce = 7, // 武
    kMaxPetElemTypeValue = kPetElemTypeForce, // 属性数量
};

// 精灵成长类型
enum pet_grow_type_t {
    kPetGrowType1      = 1,
    kPetGrowType2      = 2,
};

// 精灵战斗普通数值类型
enum battle_value_normal_type_t {
    kBattleValueNormalTypeHp = 0, //生命
    kBattleValueNormalTypeNormalAtk = 1, //普攻
    kBattleValueNormalTypeNormalDef = 2, //普防
    kBattleValueNormalTypeSkillAtk = 3, //技功
    kBattleValueNormalTypeSkillDef = 4, //技防
    kMaxBattleValueTypeNum,
};

// 隐藏战斗数值类型
enum battle_value_hide_type_t {
    kBattleValueHideTypeCrit = 0,//暴击值，
    kBattleValueHideTypeAntiCrit = 1,//防爆，同上
    kBattleValueHideTypeHit = 2,//命中值，同上
    kBattleValueHideTypeDodge = 3,//闪避值，同上
    kBattleValueHideTypeBlock = 4,//格挡值，同上
    kBattleValueHideTypeBreakBlock = 5,//破格值，同上
    kBattleValueHideTypeCritAffectRate = 6, //暴击加成率
    kBattleValueHideTypeBlockAffectRate = 7, //格挡加成率
    kBattleValueHideTypeAtkSpeed = 8, //攻击速度
    kMaxBattleValueHideTypeNum,
}; 
 

//学习力类型
enum effort_type_t {
    kEffortTypeHp = 0,
    kEffortTypeNormalAtk = 1,
    kEffortTypeNormalDef = 2,
    kEffortTypeSkillAtk = 3,
    kEffortTypeSkillDef = 4,
    kMaxEffortNum,
};

//抗性类型
enum anti_type_t {
    kAntiTypeWater = 0,
    kAntiTypeFire = 1,
    kAntiTypeGrass = 2,
    kAntiTypeLight = 3,
    kAntiTypeDark = 4,
    kAntiTypeGround = 5,
    kAntiTypeForce = 6,
    kMaxAntiNum,
};


//天赋等级种类
enum pet_talent_level_t {
    kPetTalentLevelNone = 0,
    kPetTalentLevelOne = 1,
    kPetTalentLevelTwo = 2,
    kPetTalentLevelThree = 3,
    kPetTalentLevelFour = 4,
    kPetTalentLevelFive = 5,
    kPetTalentLevelFull = kPetTalentLevelFive,
};

enum pet_constant_t {
    kMaxPetLevel    = 100,
};

struct pet_conf_t {
    pet_conf_t() {
        clear();
    }
    void clear() {
        id = 0; sex = 0; level = 1; mon_type = 0;
        elem_type = kPetElemTypeWater; growth_type = kPetGrowType1;
        memset(basic_normal_battle_values, 0, sizeof(basic_normal_battle_values));
        memset(basic_normal_battle_values_grow, 0, sizeof(basic_normal_battle_values_grow));
        memset(basic_hide_battle_values, 0, sizeof(basic_hide_battle_values));
        memset(basic_hide_battle_values_grow, 0, sizeof(basic_hide_battle_values_grow));
		memset(extra_battle_values, 0, sizeof(extra_battle_values));
        is_level_add = 1; skill_ids.clear();
        req_power = 0;
        max_dp = 0;
    }
    uint32_t id; //精灵ID
    uint32_t sex; //性别
    uint32_t level; //野怪会有等级
    uint32_t mon_type; //野怪会配是否boss

    pet_elem_type_t elem_type; //元素属性: 水、火...
    pet_grow_type_t growth_type; //成长类型
    uint32_t basic_normal_battle_values[kMaxBattleValueTypeNum]; // 基础战斗属性
    uint32_t basic_normal_battle_values_grow[kMaxBattleValueTypeNum]; //基础战斗属性的等级成长率
    uint32_t basic_hide_battle_values[kMaxBattleValueHideTypeNum]; //基础的隐藏战斗属性值

    uint32_t basic_hide_battle_values_grow[kMaxBattleValueHideTypeNum]; //基础的隐藏战斗属性按等级的成长率
	uint32_t extra_battle_values[kMaxBattleValueTypeNum];
    uint32_t is_level_add; //是否按等级成长
    std::vector<uint32_t> skill_ids;
    std::vector<std::vector<uint32_t> > affix_list_pool; // 怪物词缀库
    uint32_t req_power; //战力压制时的战力(配表或副本表得到)
    uint32_t max_dp;
};

//精灵配置管理器
class pet_conf_manager_t {
public:
    pet_conf_manager_t() {
        pet_conf_map_.clear();
    }
    ~pet_conf_manager_t() {
        pet_conf_map_.clear();
    }
public: //inline functions
    inline bool pet_conf_exist(uint32_t pet_id) {
        if (pet_conf_map_.count(pet_id) > 0) {
            return true;
        }
        return false;
    }
    inline bool add_pet_conf(pet_conf_t &pet_conf) {
        if (pet_conf_exist(pet_conf.id)) {
            return false;
        }
        pet_conf_map_[pet_conf.id] = pet_conf;
        return true;
    }
    const inline pet_conf_t *find_pet_conf(uint32_t pet_id) {
        if (pet_conf_map_.count(pet_id) == 0) {
            return NULL;
        }
        return &((pet_conf_map_.find(pet_id))->second);
    }
    inline void copy_from(const pet_conf_manager_t &m) {
        pet_conf_map_ = m.const_pet_conf_map();
    }
    const inline std::map<uint32_t, pet_conf_t>& const_pet_conf_map() const{
        return pet_conf_map_;
    }
private:
    std::map<uint32_t, pet_conf_t> pet_conf_map_;
};

extern pet_conf_manager_t g_pet_conf_mgr;

inline bool is_valid_effort_type(int effort_type)
{
    if (effort_type >= 0 && effort_type < kMaxEffortNum) {
        return true; 
    } else {
        return false; 
    }
}

inline pet_elem_type_t get_pet_elem_type(uint32_t pet_id) 
{
    const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
    if (!pet_conf) {
        return kPetElemTypeWater;
    }
    return pet_conf->elem_type;
}

#endif //__PET_CONF_H__
