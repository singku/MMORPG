#ifndef PET_UTILS_H
#define PET_UTILS_H

#include "common.h"
#include "proto/client/pb0x03.pb.h"
#include "proto/client/pb0x01.pb.h"
#include "global_data.h"
#include "data_proto_utils.h"
#include <math.h>
#include "player.h"
#include "pet_conf.h"
#include "player_utils.h"
#include "pet.h"

struct pet_power_t {
	uint32_t create_tm;
	uint32_t power;
};

bool compare_function(const pet_power_t& lhs, const pet_power_t& rhs);

class PetUtils {
public: //inline functions;

    /**
     * @brief  get_pet_basic_info_by_id 获取精灵配置信息
     * @param id 精灵id
     * @return 配置属性结构体
     */
    const static inline pet_conf_t* get_pet_conf(uint32_t pet_id) {
        return g_pet_conf_mgr.find_pet_conf(pet_id);
    }

    /** 
     * @brief 取同进化线的初级精灵id
     * 
     * @param pet_id 
     * 
     * @return 
     */
    static uint32_t get_origin_pet_id(uint32_t pet_id) {
       uint32_t origin_pet_id = pet_id;
       uint32_t tmp_id = pet_id;
       while(tmp_id) {
            const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(tmp_id);
            if (pet_conf == NULL) {
                break;
            }
            if(pet_conf->evolve_from) {
                origin_pet_id = pet_conf->evolve_from;
            }
            tmp_id = pet_conf->evolve_from;
       }

       return origin_pet_id;
    }

    //检查精灵是否到满级
    static bool check_is_pet_level_max(const Pet* pet) {
        if (!pet) {
            return false;
        }
        return pet->level() >= kMaxPetLevel;
    }

	//检查此刻的精灵特训等级能够升级
	static bool check_is_pet_effort_lv_max(const Pet* pet, int idx) {
		if (!pet) {
			return false;
		}
		return (pet->effort_lv(idx) >= kMaxPetLevel 
				|| pet->effort_lv(idx) >= pet->level());
	}

    /**
     * @brief 是否合法的精灵位置
     **/
    static inline bool is_valid_pet_loc(uint32_t loc) {
        return (    loc == PET_LOC_STORE
                ||  loc == PET_LOC_BAG
                ||  loc == PET_LOC_ELITE_STORE
                ||  loc == PET_LOC_SYS_STORE
                ||  loc == PET_LOC_ROOM
                ||  loc == PET_LOC_PRE_ABANDON_STORE);
    }

    /**
     * @brief  get_pet_in_loc 获取用户的精灵
     * @param player 玩家结构体
     * @param create_tm 捕捉时间
     * @loc 位置 不指定则表示任意位置
     * @return 返回精灵
     */
    static inline Pet* get_pet_in_loc(player_t* player, uint32_t create_tm, 
            pet_location_type_t loc = PET_LOC_UNDEF) {
        std::map<uint32_t, Pet>::iterator ptr = player->pets->find(create_tm);
        if (ptr == player->pets->end()) {
            return NULL; 
        } else if (loc != PET_LOC_UNDEF && ptr->second.loc() != loc){
            return NULL;
        } else {
            return &(ptr->second);
        }
    }

    /**
     * @brief  pet_loc_cur_size 当前精灵loc的数量
     * @param player 玩家结构体
     * @param loc 位置标识
     * @return 精灵数量
     */
    static inline uint32_t pet_loc_cur_size(player_t* player, pet_location_type_t loc) {
        switch (loc) {
        case PET_LOC_STORE:
            return player->store_pets->size();
        case PET_LOC_BAG:
            return player->bag_pets->size();
        case PET_LOC_ELITE_STORE:
            return player->elite_pets->size();
        case PET_LOC_ROOM:
            return player->room_pets->size();
        default :
            return 0;
        }
    }

    static inline uint32_t calc_pet_bag_size(player_t *player){
        uint32_t base_size = 1000;
        if (GET_A(kAttrLv) >= PLAYER_MAX_LEVEL) {
            base_size ++;
        }
		/*
        if (is_this_year_vip(player)) {
            base_size += 6;
        } else if (is_vip(player)) {
            base_size += 3;
        }
		*/
		if (is_gold_vip(player)) {
			base_size += 6;
		} else if (is_silver_vip(player)) {
			base_size += 3;
		}
        if (GET_A(kAttrPetBagSize) != base_size) {
            SET_A(kAttrPetBagSize, base_size);
        }
        return base_size;
    }


    /**
     * @brief  pet_loc_max_size 精灵loc的最大容量
     * @param player 玩家结构体
     * @param loc 位置标识
     * @return 最大容量
     */
    static inline uint32_t pet_loc_max_size(player_t* player, pet_location_type_t loc) {
        switch (loc) {
        case PET_LOC_STORE:
            return MAX_PET_IN_STORE;
        case PET_LOC_BAG:
            return calc_pet_bag_size(player);
        case PET_LOC_ELITE_STORE:
            return MAX_PET_IN_ELITE_STORE;
        case PET_LOC_ROOM:
            return MAX_PET_IN_ROOM;
        default :
            return 0;
        }
    }

    /**
     * @brief  判断精灵位置的精灵是否已满
     */
    static inline bool pets_full(player_t *player, pet_location_type_t loc) {
        return (PetUtils::pet_loc_cur_size(player, loc) 
                >= PetUtils::pet_loc_max_size(player, loc));
    }

    /**
     * @brief  pets_size 精灵数量
     * @param player 用户结构
     * @param loc 背包位置
     * @return 精灵数量
     */
    static uint32_t pets_total_count(player_t *player, pet_location_type_t loc) {
        switch (loc) {
        case PET_LOC_STORE:
            return player->store_pets->size();
        case PET_LOC_BAG:
            return player->bag_pets->size();
        case PET_LOC_ELITE_STORE:
            return player->elite_pets->size();
        case PET_LOC_ROOM:
            return player->room_pets->size();
        case PET_LOC_UNDEF:
            return player->pets->size();
        default :
            return 0;
        }
        return 0;
    }
    
    /**
     * @brief 判断玩家还可以加多少个精灵
     */
    static uint32_t can_add_pets_num(player_t *player) {
        int store_left = std::max((int)PetUtils::pet_loc_max_size(player, PET_LOC_STORE) 
                - (int)PetUtils::pet_loc_cur_size(player, PET_LOC_STORE), 0);
        int bag_left = std::max((int)PetUtils::pet_loc_max_size(player, PET_LOC_BAG)
                - (int)PetUtils::pet_loc_cur_size(player, PET_LOC_BAG), 0);
        return store_left + bag_left;
    }

    /**
     * @brief 获取用户的精灵 pet_id的第一只精灵
     */
    static inline Pet *get_pet_by_id(player_t *player, uint32_t pet_id) {
        if (player->pet_id_pets->count(pet_id) == 0) {
            return NULL;
        }
        std::vector<Pet*> &pet_vec = (player->pet_id_pets->find(pet_id))->second;
        return pet_vec[0];
    }
    /**
     *@brief 获取用户某个精灵数量
     */
    static inline uint32_t get_pet_count_by_id(player_t *player, uint32_t pet_id) {
        if (player->pet_id_pets->find(pet_id) == player->pet_id_pets->end()) {
            return 0;
        }
        return player->pet_id_pets->find(pet_id)->second.size();
    }
    /**
     * @brief 是否有精灵
     */
    static inline bool has_pet(player_t* player, uint32_t pet_id) {
        if (get_pet_by_id(player, pet_id)) {
            return true;
        }
        return false;
    }

    /**
     * @brief  是否有同一类型的精灵
     * @param pet 精灵种类id
     * @return 客户端错误码
     */
    static inline bool has_pet_type(player_t* player, uint32_t pet_id) {
        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        if (!pet_conf) return false;
        while (pet_conf) {
            if (get_pet_by_id(player, pet_conf->id)) {
                return true;
            }
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_from);
        }

        pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        while (pet_conf) {
            if (get_pet_by_id(player, pet_conf->id)) {
                return true;
            }
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_to);
        }
        return false;
    }

    /**
     * @brief  是否有某种类型的精灵守护
     * @param pet 精灵种类id
     */
    static inline bool has_pet_type_chisel(player_t* player, uint32_t pet_id) {
        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        if (!pet_conf) return false;
        while (pet_conf) {
            Pet *pet = get_pet_by_id(player, pet_conf->id);
            if (pet && pet->chisel_pos()){
                return true;
            }
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_from);
        }

        pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        while (pet_conf) {
            Pet *pet = get_pet_by_id(player, pet_conf->id);
            if (pet && pet->chisel_pos()) {
                return true;
            }
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_to);
        }
        return false;
    }

    /**
     * @brief 得到某种类型精灵的当前最大战力
     */
    static inline uint32_t get_pet_type_cur_max_battle_value(player_t *player, uint32_t pet_id) {
        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        if (!pet_conf) return 0;
        uint32_t max_btl_value = 0;
        while (pet_conf) {
            Pet *pet = get_pet_by_id(player, pet_conf->id);
            if (pet && pet->power() > max_btl_value) {
                max_btl_value = pet->power();
            }
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_from);
        }

        pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        while (pet_conf) {
            Pet *pet = get_pet_by_id(player, pet_conf->id);
            if (pet && pet->power() > max_btl_value) {
                max_btl_value = pet->power();
            }
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_to);
        }
        return max_btl_value;
    }

    /**
     * @brief 得到某种类型精灵的当前极限最大战力
     */
    static inline uint32_t get_pet_type_limit_max_battle_value(player_t *player, uint32_t pet_id) {
        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        if (!pet_conf) return 0;
        uint32_t tmp = 0;
        uint32_t max_btl_value = 0;
        while (pet_conf) {
            tmp = g_pet_conf_mgr.get_pet_max_bv(pet_conf->id);
            if (tmp > max_btl_value) {
                max_btl_value = tmp;
            }
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_from);
        }

        pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        while (pet_conf) {
            tmp = g_pet_conf_mgr.get_pet_max_bv(pet_conf->id);
            if (tmp > max_btl_value) {
                max_btl_value = tmp;
            }
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_to);
        }
        return max_btl_value;
    }

    static inline uint32_t get_pet_type_max_talent_level(
            player_t* player, uint32_t pet_id) {
        uint32_t max_level = 0;
        Pet *pet = NULL;

        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        if (!pet_conf) return false;
        while (pet_conf) {
            pet = get_pet_by_id(player, pet_conf->id);
            if (pet) {
                if ((uint32_t)pet->talent_level() > max_level) {
                    max_level = pet->talent_level();
                }
            }
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_from);
        }


        pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        while (pet_conf) {
            pet = get_pet_by_id(player, pet_conf->id);
            if (pet) {
                if ((uint32_t)pet->talent_level() > max_level) {
                    max_level = pet->talent_level();
                }
            }
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_to);
        }

        return max_level;
    }

    /**
     * @brief 计算一个进化分支上可拥有的最大精灵数
     */
    static inline uint32_t max_can_own_pet(uint32_t pet_id) {
        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        if (pet_conf == 0) return 0;
        uint32_t max = 0;
        while(pet_conf) {
            max = pet_conf->own_max;
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_to);
        }
        return max;
    }
    /**
     * @brief 计算一个 进化分支上所拥有的精灵总数
     */
    static inline uint32_t max_own_pet(player_t *player, uint32_t pet_id) {
        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
        if (!pet_conf) return 0;
        uint32_t total = 0;
        const pet_conf_t *tmp = pet_conf;
        while (pet_conf) {
            total += get_pet_count_by_id(player, pet_conf->id);
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_from);
        }
        pet_conf = tmp;
        while (pet_conf) {
            total += get_pet_count_by_id(player, pet_conf->id); //多加了一次
            pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf->evolve_to);
        }
        return (total - get_pet_count_by_id(player, pet_id));
    }
    /**
     * @brief  check_can_add_egg 是否可以添加一个精灵蛋
     * @param pet_id 精灵蛋对应的精灵id
     * @return true 可以 false 不可以
     */
    static inline bool check_can_add_pet_egg(player_t* player, uint32_t pet_id) {
        if (max_can_own_pet(pet_id) <= max_own_pet(player, pet_id)) {
            return false;
        }
        return true;
    }

    /**
     * @brief  check_can_create_pet 是否可以添加一个精灵
     * @param pet_id 精灵id
     * @return true 可以 false 不可以
     */
    static bool check_can_create_pet(player_t* player, uint32_t pet_id) {
        if (max_can_own_pet(pet_id) <= max_own_pet(player, pet_id)) {
            return false;
        }
        return true;
    }

    /**
     * @brief  check_can_hatch_pet 是否可以孵化精灵
     * @param pet_id 精灵id
     * @return true 可以 false 不可以
     */
    static bool check_can_hatch_pet(player_t* player, uint32_t pet_id) {
        if (max_can_own_pet(pet_id) <= max_own_pet(player, pet_id)) {
            return false;
        }
        return true;
    }

    /**
     * @brief  check_can_pet_evlove 是否可以精灵进化 
     * @param pet_id 精灵id
     * @return true 可以 false 不可以
     */
    static bool check_can_pet_evlove(player_t* player, uint32_t pet_id) {
        if (max_can_own_pet(pet_id) < max_own_pet(player, pet_id)) {
            return false;
        }
        return true;
    }

    /**
     * @brief  get_level_up_need_expt 获取升级所需经验
     * @param level 等级
     * @return 所需经验
     */
    static inline uint32_t get_level_up_need_exp(uint32_t pet_id, uint32_t level)  {
        if (level <= 0 || level > kMaxPetLevel) {
            return (uint32_t)(-1);
        }
        const pet_conf_t* pet_conf = get_pet_conf(pet_id);
        if (pet_conf == NULL) {
            return (uint32_t)(-1);
        }
        //NOTI(singku)策划规定的精灵升到下一级所需经验
        return 5 * level * level * level + 100 * level;
    }

	static inline uint32_t get_effort_lv_up_need_val(uint32_t pet_id, uint32_t level) {
		if (level >= kMaxPetLevel) {
			return (uint32_t)(-1);
		}
		const pet_conf_t* pet_conf = get_pet_conf(pet_id);
		if (pet_conf == NULL) {
			return (uint32_t)(-1);
		}
		uint32_t need_val = ceil(((level + 1) * (level + 1) + 10) / 10.0);
		return need_val;
	}

    /**
     * @brief  pet_level_up_to_n_need_exp 某精灵从当前升级到n一共需要多少经验
     * @param pet 精灵
     * @param level_up_to_n 目标等级
     * @return 所需经验
     */
    static inline uint32_t get_level_up_to_n_need_exp(const Pet* pet, uint32_t level_up_to_n) {
        assert(pet);
        //满级
        if (check_is_pet_level_max(pet)) {
            return 0;
        }
        //精灵是否存在
        const pet_conf_t* pet_conf = get_pet_conf(pet->pet_id());
        if (!pet_conf) {
            return 0; 
        }
        //已经达到该等级
        if (level_up_to_n <= pet->level()) {
            return 0; 
        }
        uint32_t need_exp = 0;
        for (uint32_t i = pet->level(); i < level_up_to_n; i++) {
            need_exp += get_level_up_need_exp(pet->pet_id(), i);
        }
        if (pet->exp() >= need_exp) {
            return 0;
        }
        return (need_exp - pet->exp()); 
    }

	static inline uint32_t get_effort_lv_up_to_n_need_val(const Pet* pet, 
			int idx, uint32_t level_up_to_n)
	{
		assert(pet);
		if (check_is_pet_effort_lv_max(pet, idx)) {
			return 0;
		}
		const pet_conf_t* pet_conf = get_pet_conf(pet->pet_id());
		if (!pet_conf) {
			return 0;
		}
		if (level_up_to_n <= pet->effort_lv(idx)) {
			return 0;
		}
		uint32_t need_val = 0;
		for (uint32_t i = pet->effort_lv(idx); i < level_up_to_n; ++i) {
			uint32_t tmp_val = get_effort_lv_up_need_val(pet->pet_id(), i);
			need_val += tmp_val;
		}
		if (pet->effort_value(idx) >= need_val) {
			return 0;
		}
		return (need_val - pet->effort_value(idx));
	}

    static inline void set_pet_hp(player_t *player, Pet *pet, uint32_t hp) {
        pet->set_hp(hp);
    }
    static inline void recovery_pet_hp(player_t *player, Pet *pet) {
        pet->set_hp(pet->max_hp());
    }

    static inline uint32_t get_duplicate_pet_hp(uint32_t pet_id, uint32_t level) {
        const pet_conf_t* pet_conf = PetUtils::get_pet_conf(pet_id);
        if (!pet_conf) {
            return 0; 
        }
        if (pet_conf->is_level_add) {
            return ((float)pet_conf->basic_normal_battle_values[kBattleValueNormalTypeHp] / 100 +
                    (float)pet_conf->basic_normal_battle_values_grow[kBattleValueNormalTypeHp] * level);
        } else {
            return pet_conf->basic_normal_battle_values[kBattleValueNormalTypeHp] / 100;
        }
    }

    static inline bool is_valid_chisel_pos(uint32_t pos) {
        return (pos > 0 && pos <= kMaxChiselPos);
    }
    
    static inline uint32_t pet_hide_battle_value_to_rate(uint32_t pet_conf_id, 
            battle_value_hide_type_t type, uint32_t value) {
        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_conf_id);
        if (!pet_conf) {
            return 0;
        }
        double p = value*1.0 / pet_conf->basic_hide_battle_values_coeff[type];
        return (p * 100 / (1 + p));
    }
    
    static inline bool is_valid_fight_pos(uint32_t pos) {
        return  (pos >= 1 && pos <= MAX_FIGHT_POS);
    }

    static inline uint32_t fight_pet_num(player_t *player) {
        uint32_t sum = 0;
        for (int i = 0; i < MAX_FIGHT_POS; i++) {
            if (player->fight_pet[i]) {
                sum++;
            }
        }
        return sum;
    }
    static inline uint32_t can_fight_pets(player_t *player) {
        uint32_t fight_pet_num = 1;
        if (GET_A(kAttrLv) >= 15) {
            fight_pet_num++;
        }
        if (GET_A(kAttrBuyPetFightPos) != 0) {
            fight_pet_num++;
        }
        if (is_gold_vip(player)) {
            fight_pet_num++;
        }
        return fight_pet_num;
    }
    static inline void get_fight_pets(player_t *player, std::vector<Pet*> &pet_vec) {
        pet_vec.clear();
        for (int i = 0; i < MAX_FIGHT_POS; i++) {
            if (player->fight_pet[i]) {
                pet_vec.push_back(player->fight_pet[i]);
            }
        }
    }

    static inline void fight_pets_hp_recover(player_t *player) {
        for (int i = 0; i < MAX_FIGHT_POS; i++) {
            Pet *pet = player->fight_pet[i];
            if (pet) {
                pet->set_hp(pet->max_hp());
            }
        }
    }

    static inline void bag_pets_hp_recover(player_t *player) {
        FOREACH((*(player->bag_pets)), it) {
            Pet *pet = it->second;
            if (pet) {
                pet->set_hp(pet->max_hp());
            }
        }
    }

    static inline void all_pets_lv_full(player_t *player, bool expand_hp = false) {
        FOREACH((*(player->pets)), it) {
            Pet &pet = it->second;
            pet.set_cache_level(pet.level());
            pet.set_level(kMaxPetLevel);
            pet.calc_power(player);
            if (expand_hp) {
                pet.set_tmp_max_hp(PlayerUtils::obj_hp_add_common(pet.hp(), 0));
                pet.set_hp(pet.tmp_max_hp());
            }
        }
    }

    static inline void all_pets_lv_recover(player_t *player, bool recover_hp = true) {
        FOREACH((*(player->pets)), it) {
            Pet &pet = it->second;
            pet.set_level(pet.cache_level());
            pet.set_cache_level(0);
            pet.calc_power(player);
            if (recover_hp) {//血量恢复
                pet.set_tmp_max_hp(pet.max_hp());
                pet.set_hp(pet.max_hp());
            }
        }
    }

    static inline void fight_pets_lv_full(player_t *player, bool expand_hp = false) {
        for (int i = 0; i < MAX_FIGHT_POS; i++) {
            Pet *pet = player->fight_pet[i];
            if (pet) {
                pet->set_cache_level(pet->level());
                pet->set_level(kMaxPetLevel);
                pet->calc_power(player);
                if (expand_hp) {
                    pet->set_tmp_max_hp(PlayerUtils::obj_hp_add_common(pet->hp(), 0));
                    pet->set_hp(pet->tmp_max_hp());
                }
            }
        }
    }

    static inline void fight_pets_lv_recover(player_t *player, bool recover_hp = true) {
        for (int i = 0; i < MAX_FIGHT_POS; i++) {
            Pet *pet = player->fight_pet[i];
            if (pet) {
                pet->set_level(pet->cache_level());
                pet->set_cache_level(0);
                pet->calc_power(player);
                if (recover_hp) {//血量恢复
                    pet->set_tmp_max_hp(pet->max_hp());
                    pet->set_hp(pet->max_hp());
                }
            }
        }
    }

    static inline pet_elem_type_t get_pet_elem_type(Pet *pet) {
        const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet->pet_id());
        if (!pet_conf) {
            return kPetElemTypeWater;
        }
        return pet_conf->elem_type;
    }

public: 
	//获取指定星级（天赋等级）精灵的数量
	static uint32_t get_pet_count_by_telent(player_t* player,
			pet_talent_level_t telent_lv, uint32_t& count);

	//获取指定品级精灵的数量
	static uint32_t get_pet_count_by_quality(player_t* player,
			uint32_t quality, uint32_t& count);

    //设置精灵位置 错误则返回错误码 正确返回0
    static int set_pet_loc(player_t *player, uint32_t create_tm, pet_location_type_t to_loc);

    /**
     * @brief  add_pet_exp 给精灵加经验值
     * @param pet 精灵
     * @param add_exp 增加的经验值
     * @param real_add_exp 真实加的经验值
     * @return 客户端错误码
     */
	static uint32_t add_pet_exp(player_t* player, Pet* pet, uint32_t add_exp, uint32_t &real_add_exp, 
            bool addict_detec = true, onlineproto::exp_reason_t reason = onlineproto::EXP_FROM_OTHER);
    /**
     * @brief  add_pet_effort 增加精灵的学习力
     * @param pet 精灵
     * @param idx 学习力type
     * @param add_value 增加的学习力
     * @param real_add_exp 真实加的学习力
     * @param addict_detec防沉迷标志
     * @return 客户端错误码
     */
    static uint32_t add_pet_effort(player_t* player, Pet* pet, 
			int idx, int add_val, uint32_t& real_add_val,
			bool addict_detec = true);

    /**
     * @brief  add_effort_over_overflow 增加学习力
     *
     * @param player 玩家结构体
     * @param pet 精灵结构体
     * @param allco_data 学习力数据
     */
	/*
    static void add_pet_effort(player_t* player, Pet* pet, 
            onlineproto::effort_alloc_data_t* alloc_data);
	*/
    /**
     * @brief  check_effort_over_overflow 判断学习力是否溢出
     *
     * @param pet 精灵结构体
     * @param alloc_data 学习力数据
     */
    static uint32_t check_effort_overflow(Pet* pet, 
            onlineproto::effort_alloc_data_t* alloc_data);

    /**
     * @brief  pet_level_up 精灵升级
     * @param player 玩家结构
     * @param create_tm 捕捉时间
     * @param add_level 新增等级
     * @param wait 是否等待返回
     * @return 客户端错误码
     */
    static int pet_level_up(player_t* player, Pet *pet, uint32_t add_level, bool wait,
            onlineproto::level_up_reason_t reason = onlineproto::LEVELUP_FROM_OTHER);
    /**
     * @brief  pet_evolve 精灵进化
     * @param player 玩家结构体
     * @param create_tm 捕捉时间
     * @param pet_id 精灵id
     * @param new_level　新的等级
     * @param wait 是否等待返回
     * @param new_level如果为0 则不变
     * @return 客户端错误码
     */
    static int pet_evolve(player_t* player, Pet *pet, uint32_t to_pet_id, uint32_t new_level, bool wait, 
            onlineproto::evolution_reason_t reason = onlineproto::EVOLUTION_FROM_OTHER);
    /**
     * @brief create_pet 给用户创建一个精灵
     *
     * @param player 玩家数据
     * @param pet_id 精灵id
     * @param level 等级
     * @param wait 是否等待数据库返回
     * @param create_tm 返回精灵捕捉时间
     *
     * @return 客户端错误码
     */
    static int create_pet(player_t* player, uint32_t pet_id, uint32_t level, 
            bool wait_db, uint32_t* create_tm = NULL, 
            pet_talent_level_t born_talent = kPetTalentLevelNone);
    /**
     * @brief  init_pet 初始化精灵
     * @param pet_id 精灵id
     * @param level 精灵等级
     * @param pet 精灵
     *
     * @return 0 成功 -1 失败
     */
    static int init_pet(Pet *pet, uint32_t pet_id, uint32_t pet_level, 
            pet_talent_level_t talent, uint32_t create_tm);
    /**
     * @brief  online 添加精灵给玩家
     * @param player 玩家结构
     * @param pet 精灵
     * @return 0 成功 -1 失败
     */
    static int add_pet_to_player(player_t* player, Pet& pet);

    /**
     * @brief 提升精灵的天赋等级
     */
    static int improve_pet_talent_level(player_t* player, Pet *pet);
    static int cond_set_talent_level(player_t *player, Pet *pet, uint32_t level);
    /**
     * @brief  save_pet 保存pet到db，并同步到客户端
     * @param player 玩家数据
     * @param pet 精灵
     * @param wait 是否等待db返回
     * @return 客户端错误码
     */
    static uint32_t save_pet(player_t* player, Pet& pet, 
            bool wait = false, bool is_syn_cli = true, bool noti_map = true,
            onlineproto::synpower_reason_t change_reason = onlineproto::POWER_CHANGE_FROM_OTHER);

    static uint32_t del_pet(player_t* player, Pet* pet);

    /**
     * @brief  获得锻炼中的精灵
     * @return 
     */
	static uint32_t get_in_exercise_pets(player_t* player, std::vector<Pet*>& pets_vec);

    /**
     * @brief  精灵锻炼时间是否结束
     * @return 
     */
	static bool check_pet_exercise_over(Pet* pet, uint32_t exercise_duration); 


    /** 
     * @brief 附加团队属性加成
     * 
     * @param player 
     * @param attr_map 
     * 
     * @return 
     */
    static int pet_group_addition(player_t *player, std::map<uint32_t, uint32_t> &attr_map);

    static bool is_valid_pet_group_active_type(uint32_t type) {
        switch (type) {
        case pet_group_active_type_guard:
        case pet_group_active_type_own:
            return true;
        default:
            return false;
        }
    }
    /**
     * @brief 判断玩家是否激活某伙伴团队效果
     * @param player
     * @param group_id 团队效果ID
     * @return bool
     */
    static bool pet_group_activated(player_t *player, uint32_t group_id);
     
    /** 
     * @brief 计算玩家精灵的战斗力总和
     * 
     * @param player 
     * @param 
     * 
     * @return 
     */
	static uint32_t calc_all_pets_total_power(player_t* player);

    /** 
     * @brief 获取玩家最高战斗力的前 need_num个精灵
     * 
     * @param player 
     * @param create_tm_set
     * @param need_num
     * 
     * @return 
     */
	static uint32_t get_pets_n_topest_power(player_t* player,
			std::set<uint32_t>& create_tm_set, uint32_t need_num,
			uint32_t& total_power);

    /** 
     * @brief 获取玩家最高战斗力的前 need_num个精灵
     *        从battle_pet_data_t中获取 
     * 
     * @param player 
     * @param create_tm_set
     * @param need_num
     * 
     * @return 
     */
	static uint32_t get_pets_n_topest_power_from_cache(
			const commonproto::battle_pet_list_t& btl_pet_list,
			std::set<uint32_t>& create_tm_set, uint32_t need_num);

    /** 
     * @brief : 更新远征中参加战斗的精灵的血量
     * 
     * @param player 
     * @param pet_list
     * 
     * @return 
     */
	static uint32_t update_pets_expedtion_hp(player_t* player,
		const onlineproto::expedition_pet_cur_hp_list& pet_list);

    /** 
     * @brief : 远征中出战的精灵挂了后的处理,血量置0，状态置为EXPED_HAS_DIED
     * 
     * @param player 
     * 
     * @return 
     */
	static void deal_pets_after_died_in_exped(player_t* player);

    /** 
     * @brief : 重置远征活动后，处理精灵的状态
     * 
     * @param player 
     * 
     * @return 
     */
	static void deal_pets_after_reset_exped(player_t* player);

    /** 
     * @brief : 改变远征中伙伴的出战状态
     *   
     * @param player 
     * @param cur_flag：当前伙伴远征状态
	 （若des_flag值为EXPED_NO_JOINED,此值可以任意设取）
     * @param des_flag: 需要将伙伴设置成的目标状态
	 (若des_flag的值为EXPED_NO_JOINED，此函数功能是清楚精灵参加远征信息,包括远征血量)
     * 
     * @return 
     */
	static void change_pets_exped_flag(player_t* player, 
			pet_exped_flag_t cur_flag,
			pet_exped_flag_t des_flag);

	/** 
	 * @brief : 获取远征出战的精灵
	 * 
	 * @param player 
	 * @param pet_vec
	 * 
	 * @return 
	 */
	//static uint32_t get_exped_fight_pets(
	static uint32_t get_exped_pets_by_flag(
			player_t* player, std::vector<Pet*>& pet_vec,
			pet_exped_flag_t flag);

    /**
     * @brief 选择精灵参加远征
     * @param player
     * @param create_tm_vec 检查的精灵create_tm
     * @return 
     */
	static uint32_t pick_pets_joined_exped(
		player_t* player, const std::vector<uint32_t>& create_tm_vec);

    /**
     * @brief 在参加远征的精灵中选择精灵出战
     * @param player
     * @param create_tm_vec 检查的精灵create_tm
     * @return 
     */
	static uint32_t pick_pets_into_exped_fight(
		player_t* player, const std::vector<uint32_t>& create_tm_vec);

    /** 
     * @brief 更新守护精灵对应的技能
     * 
     * @param player 
     * 
     * @return 
     */
    static int update_pet_skill(player_t *player);


    /** 
     * @brief 更新守护精灵对应的精灵技能
     * 
     * @param player 
     * @param parent_skill_id 
     * 
     * @return 
     */
    static int update_pet_skill_by_id(player_t *player, uint32_t parent_skill_id);

    /** 
     * @brief 获取该属性的特训升级从effort_lv - 1 升到effort_lv所需要经验
     * 
     * @param player 
     * 
     * @return 
     */
	static inline uint32_t get_effort_lv_up_need_effval(Pet* pet, uint32_t effort_lv) {
		if (effort_lv <= 0 || effort_lv >= kMaxPetLevel) {
			return (uint32_t)(-1);
		}
        const pet_conf_t* pet_conf = get_pet_conf(pet->pet_id());
        if (pet_conf == NULL) {
            return (uint32_t)(-1);
        }
		return ((effort_lv + 1) * (effort_lv + 1) + 10) / 10;
	}

    /** 
     * @brief 同步出战精灵的信息给客户端
     * 
     * @param player 
     * 
     * @return 
     */
	static uint32_t sync_fight_pet_info_to_client(player_t* player);
	/*
	static uint32_t set_effort_value(Pet* pet, uint32_t effort_val, uint32_t idx) {
		if (idx <= 0 || idx >= kMaxEffortNum) {
			return cli_err_no_this_effect_idx;
		}
		if (pet->level() )}
	*/
    /** 
     * @brief 获得正在小屋中锻炼时间大于time值的精灵
     * 
     * @param player 
     * 
     * @return 
     */
	static uint32_t get_exercise_pets(
			player_t* player, std::vector<Pet*>& pets_vec, uint32_t time);

	static uint32_t sync_pet_exercise_stat_to_hm(player_t* player, Pet* pet);

    //记录玩家的出战精灵位的精灵
    static uint32_t save_fight_pet_pos(player_t *player) {
        for (int i = 0; i < MAX_FIGHT_POS; i++) {
            Pet *pet = player->fight_pet[i];
            if (!pet) {
                player->temp_info.tmp_fight_pos_pet_create_tm[i] = 0;
            } else {
                player->temp_info.tmp_fight_pos_pet_create_tm[i] = pet->create_tm();
            }
        }
        return 0;
    }
    //还原出战位精灵
    static uint32_t retrieve_fight_pet_pos(player_t *player) {
        //<pet, pos>
        std::map<Pet*, int32_t> changed_pet_map;
        for (int i = 0; i < MAX_FIGHT_POS; i++) {
            Pet *cur_pet = player->fight_pet[i];
            uint32_t org_create_tm = player->temp_info.tmp_fight_pos_pet_create_tm[i];
            Pet *org_pet = PetUtils::get_pet_in_loc(player, org_create_tm);

            //如果当前位有精灵但不是之前(有可能之前为空)的
            if (cur_pet && cur_pet->create_tm() != org_create_tm) {
                changed_pet_map[cur_pet] = 0;//当前精灵位置无效
                player->fight_pet[i] = 0;//收回
                if (org_pet) {//之前不为空
                    changed_pet_map[org_pet] = i+1;
                }
            //当前位没有精灵但之前有
            } else if (!cur_pet && org_pet) {
                changed_pet_map[org_pet] = i+1;
            } //else if 当前有且是之前的 就不变
             //else 当前位没有 之前也没有 也不变
        }
        FOREACH(changed_pet_map, it) {
            Pet *pet = it->first;
            pet->set_fight_pos(it->second);
            if (it->second) {
                player->fight_pet[it->second - 1] = pet;
            }
            PetUtils::save_pet(player, *pet, false, true, false);
        }
        /*
        onlineproto::sc_0x0108_notify_map_player_info_change noti_msg;
        DataProtoUtils::pack_map_player_info(player, noti_msg.mutable_player_info());
        noti_msg.set_reason(commonproto::PLAYER_FOLLOW_PET_CHANGE);
        send_msg_to_player(player, cli_cmd_cs_0x0108_notify_map_player_info_change, noti_msg);
        */
        return 0;
    }
	
	/*
	static uint32_t convert_pet_lamp_state_to_level_map(uint32_t lamp_state,
			std::map<uint32_t, uint32_t>& lamp_level_map);

	static uint32_t convert_level_map_to_pet_lamp_state(
		const std::map<uint32_t, uint32_t>& lamp_level_map,
		uint32_t& lamp_state);

	static uint32_t convert_lamp_state_to_pet_btl_attr(uint32_t lamp_state,
		commonproto::battle_info_t* pb_btl);
	*/
	static uint32_t check_lamp_lv_condition(player_t* player, Pet* pet,
			const commonproto::lamp_info_t& lamp_info,
			uint32_t lamp_index, uint32_t& old_lamp_lv);

	static uint32_t lamp_lv_up(player_t* player, uint32_t lamp_index,
			commonproto::lamp_info_t& lamp_info);

	static uint32_t check_pets_is_in_fight_pos(player_t* player,
			const std::vector<uint32_t>& pet_ids,
			std::vector<uint32_t>& not_in_fight_pets);
};

#endif
