#ifndef EQUIP_UTILS_H
#define EQUIP_UTILS_H

#include "common.h"

#define QUENCH_SEGMENT (10)
#define QUENCH_WEIGHT_SUM (200)

class EquipUtils {
    public:
        static int cultivate_equip(
            player_t *player, uint32_t type, uint32_t buy_flag, uint32_t &up_flag);

        static int cultivate_equip_addition(
                player_t *player, std::map<uint32_t, uint32_t> &attr_map);

        static int add_cult_equip_level_reward(player_t *player);

        static int add_init_mount(player_t *player);
        static int add_init_wing(player_t *player);

        //装备洗练 根据10个区间的权重 随机出一个区间的下标并返回 返回值是0-9
        static inline uint32_t random_get_equip_quench_idx() {
            static uint32_t weight[QUENCH_SEGMENT] = {20, 30, 30, 40, 36, 30, 8, 4, 1, 1};
            uint32_t value = taomee::ranged_random(1, QUENCH_WEIGHT_SUM);
            for (uint32_t i = 0; i < QUENCH_SEGMENT; i++) {
                if (value <= weight[i]) {
                    return i;
                } else {
                    value = value - weight[i];
                }
            }
            return QUENCH_SEGMENT-1;
        }

        //根据输入的洗练最小最大值 随机出洗练的数值
        static inline uint32_t random_quench_value(uint32_t min, uint32_t max) {
            uint32_t hit_idx = random_get_equip_quench_idx();
            uint32_t len = max - min + 1;
            float unit = (float)len / QUENCH_SEGMENT;
            float low = (float)len * hit_idx / QUENCH_SEGMENT;
            float high = low + unit;

            if (hit_idx == QUENCH_SEGMENT - 1) {
                high = max;
            }
            if (high > max) {
                high = max;
            }
            return taomee::ranged_random((int)low, (int)high);
        }

        static inline uint32_t random_select_quench_1_type() {
            uint32_t idx_set[] = {
                commonproto::EQUIP_QUENCH_TYPE_1_CRIT ,
                commonproto::EQUIP_QUENCH_TYPE_1_ANTI_CRIT ,
                commonproto::EQUIP_QUENCH_TYPE_1_HIT ,
                commonproto::EQUIP_QUENCH_TYPE_1_DODGE ,
                commonproto::EQUIP_QUENCH_TYPE_1_BLOCK ,
                commonproto::EQUIP_QUENCH_TYPE_1_BREAK_BLOCK ,

                commonproto::EQUIP_QUENCH_TYPE_1_HP ,
                commonproto::EQUIP_QUENCH_TYPE_1_NATK ,
                commonproto::EQUIP_QUENCH_TYPE_1_NDEF ,
                commonproto::EQUIP_QUENCH_TYPE_1_SATK ,
                commonproto::EQUIP_QUENCH_TYPE_1_SDEF ,

                commonproto::EQUIP_QUENCH_TYPE_1_ANTI_WATER ,
                commonproto::EQUIP_QUENCH_TYPE_1_ANTI_FIRE ,
                commonproto::EQUIP_QUENCH_TYPE_1_ANTI_GRASS ,
                commonproto::EQUIP_QUENCH_TYPE_1_ANTI_LIGHT ,
                commonproto::EQUIP_QUENCH_TYPE_1_ANTI_DARK ,
                commonproto::EQUIP_QUENCH_TYPE_1_ANTI_GROUND ,
                commonproto::EQUIP_QUENCH_TYPE_1_ANTI_FORCE
            };
            uint32_t hit = ranged_random(0, sizeof(idx_set)/sizeof(idx_set[0])-1);
            return idx_set[hit];
        }

        static int update_trial_cult_equip_expire_time(
                player_t *player, uint32_t expire_time);

        static uint32_t calc_equip_btl_value(commonproto::item_optional_attr_t &equip_opt_attr);
};

#endif
