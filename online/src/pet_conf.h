#ifndef __PET_CONF_H__
#define __PET_CONF_H__

#include "common.h"
#include "item.h"

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

// 精灵品质
enum pet_qualiy_type_t {
    kPetQualityWhite        = 1,
    kPetQualityGreen        = 2,
    kPetQualityGreen_1      = 3,
    kPetQualityBlue         = 4,
    kPetQualityBlue_1       = 5,
    kPetQualityBlue_2       = 6,
    kPetQualityPuple        = 7,
    kPetQualityPuple_1      = 8,
    kPetQualityPuple_2      = 9,
    kPetQualityPuple_3      = 10,
};

//精灵位置
enum pet_location_type_t {
    PET_LOC_STORE                       = commonproto::PET_LOC_STORE,
    PET_LOC_BAG                     = commonproto::PET_LOC_BAG,
    PET_LOC_ELITE_STORE             = commonproto::PET_LOC_ELITE_STORE,
    PET_LOC_ROOM                    = commonproto::PET_LOC_ROOM,
    PET_LOC_SYS_STORE               = commonproto::PET_LOC_SYS_STORE,
    PET_LOC_PRE_ABANDON_STORE       = commonproto::PET_LOC_PRE_ABANDON_STORE,
    PET_LOC_ALREADY_ABANDON_STORE   = commonproto::PET_LOC_ALREADY_ABANDON_STORE,

    PET_LOC_UNDEF                   = 1000000,
};

enum pet_exped_flag_t {
	EXPED_NO_JOINED = commonproto::EXPED_NO_JOINED,
	EXPED_JOINED = commonproto::EXPED_JOINED, //参加远征，但还未开始过战斗
	EXPED_FIGHT = commonproto::EXPED_FIGHT, //当前参战
	EXPED_HAS_FIGHTED = commonproto::EXPED_HAS_FIGHTED, //参加过战斗，但现在未参战
	EXPED_HAS_DIED = commonproto::EXPED_HAS_DIED, //参加战斗，挂了
};

//每个位置的最大量
enum pet_location_limit_t {
    MAX_PET_IN_STORE        = commonproto::MAX_PET_IN_STORE,
    MAX_PET_IN_BAG          = commonproto::MAX_PET_IN_BAG,
    MAX_PET_IN_ELITE_STORE  = commonproto::MAX_PET_ELITE_STORE,
    MAX_PET_IN_ROOM         = commonproto::MAX_PET_IN_ROOM,
    MAX_PET_IN_SYS_STORE    = commonproto::MAX_PET_IN_SYS_STORE, 
    MAX_PET_IN_PRE_ABANDON_STORE    = commonproto::MAX_PET_IN_PRE_ABANDON_STORE, 
    MAX_FIGHT_POS           = commonproto::MAX_FIGHT_POS,

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

//不同的天赋等级升级对道具的数量要求
enum pet_talent_level_up_require_t {
    kFiveLevelTalentItemCnt    =   commonproto::FiveLevelTalentItemCnt,
    kFourLevelTalentItemCnt    =   commonproto::FourLevelTalentItemCnt,
    kThreeLevelTalentItemCnt    =   commonproto::ThreeLevelTalentItemCnt,
    kTwoLevelTalentItemCnt    =   commonproto::TwoLevelTalentItemCnt,
    kOneLevelTalentItemCnt    =   commonproto::OneLevelTalentItemCnt,

    kFiveLevelTalentGoldCnt     = commonproto::FiveLevelTalentGoldCnt,
    kFourLevelTalentGoldCnt     = commonproto::FourLevelTalentGoldCnt,
    kThreeLevelTalentGoldCnt     = commonproto::ThreeLevelTalentGoldCnt,
    kTwoLevelTalentGoldCnt     = commonproto::TwoLevelTalentGoldCnt,
    kOneLevelTalentGoldCnt     = commonproto::OneLevelTalentGoldCnt,
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

//符文装备位置
enum rune_equip_idx_t {
    kRuneIdx1 = 1,
    kRuneIdx2 = 2,
    kRuneIdx3 = 3,
};

//一些常量
enum pet_constant_t {
    kMaxSingleEffortLevel               = commonproto::MAX_EFFORT_LEVEL,
    kMaxTotalEffortLevel                = commonproto::MAX_TOTAL_EFFORT_LEVEL,
    kMaxPetLevel                        = commonproto::MAX_PET_LEVEL,
    kMaxPetSkill                        = 4,
    kMonPetIDStart                      = 10000,
    kMaxChiselPos                       = 6,
};

//精灵锻炼找到的物品
enum pet_exercise_found_item_t {
	HM_NORMAL_BOX = 39501,
	HM_BRONZE_BOX = 39502,
	HM_GOLD_BOX = 39503,
	HM_NORNAL_KEY = 39504,
	HM_BRONZE_KEY = 39505,
	HM_GOLD_KEY = 39506,
};

enum hm_pet_op_type_t {
	HM_PET_EXERCISE = onlineproto::HM_PET_EXERCISE,
	HM_PET_FIND_BOX = onlineproto::HM_PET_FIND_BOX,
};

struct pet_conf_t {
    pet_conf_t() {
        clear();
    }
    void clear() {
        id = 0; sex = 0; level = 1; mon_type = 0; prize_id_list.clear();
        task_prize_list.clear();
        elem_type = kPetElemTypeWater; /*growth_type = kPetGrowType1;*/
        memset(basic_normal_battle_values, 0, sizeof(basic_normal_battle_values));
        memset(basic_normal_battle_values_grow, 0, sizeof(basic_normal_battle_values_grow));
        memset(basic_hide_battle_values, 0, sizeof(basic_hide_battle_values));
        memset(basic_hide_battle_values_grow, 0, sizeof(basic_hide_battle_values_grow));
        memset(basic_hide_battle_values_coeff, 0, sizeof(basic_hide_battle_values_coeff));

        is_level_add = 1; skill_ids_vec.clear(); evolve_from = 0; evolve_to = 0;
        evolve_talent = 0;
        /*evolve_item = 0; evolve_item_cnt = 0;*/ own_max = 1; /*egg_item = 0;*/ talent_item = 0; 
        born_talent = 1;
        /*egg_drop_rate = 0;*/ is_hide = 0;
        sys_pet_flag = 0; abandon_flag = 0;
        waken_type = 0;
        group_ids.clear();
        name.clear();
        hit_prize_id = 0;
        hit_prize_count = 0;
        basic_battle_value = 0;
		dup_pass_time_attr = 0;
		must_drop_prize = 0;
		rand_drop_prize = 0;
    }
    uint32_t id; //精灵ID
    uint32_t sex; //性别
    uint32_t level; //野怪会有等级
    uint32_t mon_type; //野怪会配是否boss
    std::vector<uint32_t> prize_id_list; //野怪会配置掉落奖励
    //<taskid, prize_list>
    std::map<uint32_t, std::vector<uint32_t> > task_prize_list; //与任务绑定的掉落奖励

    pet_elem_type_t elem_type; //元素属性: 水、火...
    //pet_grow_type_t growth_type; //成长类型
    uint32_t basic_normal_battle_values[kMaxBattleValueTypeNum]; // 基础战斗属性
    uint32_t basic_normal_battle_values_grow[kMaxBattleValueTypeNum]; //基础战斗属性的等级成长率
    uint32_t basic_hide_battle_values[kMaxBattleValueHideTypeNum]; //基础的隐藏战斗属性值
    uint32_t basic_hide_battle_values_grow[kMaxBattleValueHideTypeNum]; //基础的隐藏战斗属性按等级的成长率
    uint32_t basic_hide_battle_values_coeff[kMaxBattleValueHideTypeNum]; //隐藏战斗属性成长系数

    uint32_t is_level_add; //是否按等级成长

	std::vector<uint32_t> skill_ids_vec;//技能id
    uint32_t evolve_from; // 从哪个id进化而来
    uint32_t evolve_to; // 可以进化到哪个id
    //auint32_t evolve_lv; // 进化等级
    //uint32_t evolve_item; // 进化需要物品
    //uint32_t evolve_item_cnt; //进化需要的道具数量
    uint32_t evolve_talent; //进化星级
    uint32_t own_max; // 最大拥有个数
    //uint32_t egg_item; // 精灵蛋item id
    uint32_t talent_item; //天赋升级物品
    uint32_t born_talent; //召唤时的天赋星级
    //uint32_t egg_drop_rate; // 野怪掉蛋概率
    uint32_t is_hide; //1 表示隐藏精灵，图鉴不收录
    uint32_t sys_pet_flag; //1 表示系统精灵
    uint32_t abandon_flag; //1 表示可以放生
    uint32_t waken_type; // 觉醒类型
    std::vector<uint32_t> group_ids;    // 精灵团队效果id
    string name;
    //std::vector<buff_t> buffs;
    uint32_t hit_prize_id; //受击掉落奖励
    uint32_t hit_prize_count; //受击X次掉落奖励
    uint32_t basic_battle_value; //配表基础战力
	uint32_t dup_pass_time_attr;//精灵通关副本的时间戳 所使用的属性id
	uint32_t must_drop_prize;	//必掉的奖励
	uint32_t rand_drop_prize;	//必掉的奖励
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
    //计算伙伴极限战力
    inline uint32_t get_pet_max_bv(uint32_t pet_id) {
        const pet_conf_t *conf = find_pet_conf(pet_id);
        if (!conf) {
            return 0;
        }
        return (conf->basic_battle_value + kMaxPetLevel * 50 + kPetTalentLevelFull * 1000 
                + kPetQualityPuple_3 * 200 + kMaxTotalEffortLevel * 8 + 4 * 6 * 100);
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

inline bool is_valid_effort_type(int effort_type)
{
    if (effort_type >= 0 && effort_type < kMaxEffortNum) {
        return true; 
    } else {
        return false; 
    }
}

typedef std::map<uint32_t, struct pet_quality_conf_t> pet_quality_conf_map_t;

struct pet_quality_conf_t {
    pet_quality_conf_t() {
        id = 0;
        next_id = 0;
        pet_level = 0;
        cost_gold = 0;
    }
    uint32_t id;
    uint32_t next_id;
    uint32_t pet_level;
    std::vector<reduce_item_info_t> cost_item;
    uint32_t cost_gold;
};

/** 
 * @brief 精灵品质(颜色)配置管理器
 */
class pet_quality_conf_manager_t {
    public:
        pet_quality_conf_manager_t() {
        }

        ~pet_quality_conf_manager_t() {
        }

        inline bool is_pet_quality_conf_exist(uint32_t waken_type, uint32_t id) {
            std::map<uint32_t, pet_quality_conf_map_t>::iterator iter = 
                pet_quality_class_conf_map_.find(waken_type);
            if (iter != pet_quality_class_conf_map_.end()) {
                if (iter->second.find(id) != iter->second.end()) {
                    return true;
                }
            }
            return false;
        }

        inline const struct pet_quality_conf_t *get_pet_quality_conf(
                uint32_t waken_type, uint32_t id) {
            std::map<uint32_t, pet_quality_conf_map_t>::iterator iter = 
                pet_quality_class_conf_map_.find(waken_type);

            if (iter == pet_quality_class_conf_map_.end()) {
                return NULL;
            }

            std::map<uint32_t, struct pet_quality_conf_t>::iterator iter2 = 
                iter->second.find(id);
            if (iter2 == iter->second.end()) {
                return NULL;
            }

            return &(iter2->second);
        }

        inline bool add_pet_quality_conf(
                uint32_t waken_type, uint32_t id, 
                struct pet_quality_conf_t &pet_quality) {

            std::map<uint32_t, pet_quality_conf_map_t>::iterator iter = 
                pet_quality_class_conf_map_.find(waken_type);
            if (iter != pet_quality_class_conf_map_.end()) {
                if (iter->second.find(id) != iter->second.end()) {
                    return false;
                }

                iter->second.insert(
                        std::pair<uint32_t, pet_quality_conf_t>(id, pet_quality));
            } else {
                pet_quality_conf_map_t conf_map;
                conf_map.insert(
                        std::pair<uint32_t, pet_quality_conf_t>(id, pet_quality));
                pet_quality_class_conf_map_.insert(
                        std::pair<uint32_t, pet_quality_conf_map_t>(waken_type, conf_map));
            }

            return true;
        }

        const inline std::map<uint32_t, pet_quality_conf_map_t>& const_pet_quality_conf_map() const{
            return pet_quality_class_conf_map_;
        }

        inline void copy_from(const pet_quality_conf_manager_t &tmp) {
            pet_quality_class_conf_map_ = tmp.const_pet_quality_conf_map();
        }

    private:
        /*pet_quality_conf_map_t pet_quality_conf_map_;*/
        // <class_id, conf_map>
        std::map<uint32_t ,pet_quality_conf_map_t> pet_quality_class_conf_map_;
};


struct group_add_attr_t {
    group_add_attr_t() {
        type = 0;
        basic_value = 0;
        max_value = 0;
    }
    uint32_t type;
    int32_t basic_value;
    int32_t max_value;
};

enum pet_group_active_type_t {
    pet_group_active_type_guard     = 1,    //守护
    pet_group_active_type_own       = 2,    //拥有
};

struct pet_group_info_t {
    pet_group_info_t() {
        group_id = 0;
        activate_type = pet_group_active_type_guard;
        pet_ids.clear();
        effects.clear();
    }
    uint32_t group_id;
    pet_group_active_type_t activate_type;
    std::vector<uint32_t> pet_ids;
    std::vector<group_add_attr_t> effects;
};

class pet_group_manager_t {
    public:
        pet_group_manager_t() {
            pet_group_conf_map_.clear();
        }
        ~pet_group_manager_t() {
        }

        bool add_pet_group_conf(uint32_t id, pet_group_info_t &group_conf) {
            std::map<uint32_t, pet_group_info_t>::iterator iter = 
                pet_group_conf_map_.find(id);
            if (iter != pet_group_conf_map_.end()) {
                return false;
            }
            pet_group_conf_map_.insert(
                    std::pair<uint32_t, pet_group_info_t>(id, group_conf));
            return true;
        }

        const pet_group_info_t *get_pet_group_conf(uint32_t id) {
            std::map<uint32_t, pet_group_info_t>::iterator iter = 
                pet_group_conf_map_.find(id);
            if (iter == pet_group_conf_map_.end()) {
                return NULL;
            }

            return &(iter->second);
        }

        const inline std::map<uint32_t, pet_group_info_t > &const_pet_group_conf_map() const {
            return pet_group_conf_map_;
        } 
        inline void copy_from(const pet_group_manager_t &m) {
            pet_group_conf_map_ = m.const_pet_group_conf_map();
        }

    private:
        std::map<uint32_t, pet_group_info_t> pet_group_conf_map_;
};

#endif //__PET_CONF_H__
