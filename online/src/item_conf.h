#ifndef __ITEM_CONF_H__
#define __ITEM_CONF_H__

#include "common.h"
#include "player_conf.h"
#include "utils.h"

//收费免费的道具ID差(F freee, C charge)
#define ITEM_FC_DIFF (1000000)

#define EQUIP_LEVEL_MAX (10)

#define CALC_EQUIP_ATTR_VALUE(factor, quality) \
    (factor * (quality * quality - quality + 2) / 2)

//装备部位
enum equip_body_pos_t {
    EQUIP_BODY_POS_NONE = commonproto::EQUIP_BODY_POS_NONE, //非装备
    EQUIP_BODY_POS_HEAD = commonproto::EQUIP_BODY_POS_HEAD, //头部
    EQUIP_BODY_POS_BODY = commonproto::EQUIP_BODY_POS_BODY, //上身
    EQUIP_BODY_POS_HAND = commonproto::EQUIP_BODY_POS_HAND, //手持
    EQUIP_BODY_POS_LEG = commonproto::EQUIP_BODY_POS_LEG, //腿部
    EQUIP_BODY_POS_FOOT = commonproto::EQUIP_BODY_POS_FOOT, //脚
    EQUIP_BODY_POS_ATTR = commonproto::EQUIP_BODY_POS_ATTR, //属性克制位
    EQUIP_FASHION_BODY_POS_HEAD   = commonproto::EQUIP_FASHION_BODY_POS_HEAD, //时装头部
    EQUIP_FASHION_BODY_POS_FACE   = commonproto::EQUIP_FASHION_BODY_POS_FACE, //时装脸部
    EQUIP_FASHION_BODY_POS_CLOTH  = commonproto::EQUIP_FASHION_BODY_POS_CLOTH, //时装衣服
    EQUIP_FASHION_BODY_POS_WEAPON = commonproto::EQUIP_FASHION_BODY_POS_WEAPON, //时装武器
    EQUIP_FASHION_BODY_POS_COAT   = commonproto::EQUIP_FASHION_BODY_POS_COAT, //时装翅膀
    EQUIP_FASHION_BODY_POS_WING   = commonproto::EQUIP_FASHION_BODY_POS_WING, //时装翅膀

    EQUIP_BODY_POS_MOUNT    = commonproto::EQUIP_BODY_POS_MOUNT, // 坐骑
    EQUIP_BODY_POS_WING     = commonproto::EQUIP_BODY_POS_WING, // 翅膀

    EQUIP_FASHION_BODY_POS_TAIL   = commonproto::EQUIP_FASHION_BODY_POS_TAIL, //尾巴时装

    EQUIP_BODY_POS_END,

};

//装备增加属性类型
enum equip_add_attr_t {
    EQUIP_ADD_ATTR_NONE = commonproto::EQUIP_ADD_ATTR_NONE, //啥都不加

    // 数量加成属性
    EQUIP_ADD_ATTR_HP   = commonproto::EQUIP_ADD_ATTR_HP  , //生命
    EQUIP_ADD_ATTR_NATK = commonproto::EQUIP_ADD_ATTR_NATK, //普攻
    EQUIP_ADD_ATTR_NDEF = commonproto::EQUIP_ADD_ATTR_NDEF, //普防
    EQUIP_ADD_ATTR_SATK = commonproto::EQUIP_ADD_ATTR_SATK, //技功
    EQUIP_ADD_ATTR_SDEF = commonproto::EQUIP_ADD_ATTR_SDEF, //技防

    EQUIP_ADD_ATTR_CRIT         = commonproto::EQUIP_ADD_ATTR_CRIT        ,   // 暴击
    EQUIP_ADD_ATTR_ANTI_CRIT    = commonproto::EQUIP_ADD_ATTR_ANTI_CRIT   ,   // 防暴
    EQUIP_ADD_ATTR_HIT          = commonproto::EQUIP_ADD_ATTR_HIT         ,   // 命中
    EQUIP_ADD_ATTR_DODGE        = commonproto::EQUIP_ADD_ATTR_DODGE       ,   // 闪避
    EQUIP_ADD_ATTR_BLOCK        = commonproto::EQUIP_ADD_ATTR_BLOCK       ,   // 格挡
    EQUIP_ADD_ATTR_BREAK_BLOCK  = commonproto::EQUIP_ADD_ATTR_BREAK_BLOCK ,   // 破格

    EQUIP_ADD_ATTR_ANTI_WATER = commonproto::EQUIP_ADD_ATTR_ANTI_WATER,
    EQUIP_ADD_ATTR_ANTI_FIRE = commonproto::EQUIP_ADD_ATTR_ANTI_FIRE,
    EQUIP_ADD_ATTR_ANTI_GRASS = commonproto::EQUIP_ADD_ATTR_ANTI_GRASS,
    EQUIP_ADD_ATTR_ANTI_LIGHT = commonproto::EQUIP_ADD_ATTR_ANTI_LIGHT,
    EQUIP_ADD_ATTR_ANTI_DARK = commonproto::EQUIP_ADD_ATTR_ANTI_DARK,
    EQUIP_ADD_ATTR_ANTI_GROUND = commonproto::EQUIP_ADD_ATTR_ANTI_GROUND,
    EQUIP_ADD_ATTR_ANTI_FORCE = commonproto::EQUIP_ADD_ATTR_ANTI_FORCE,

    EQUIP_ADD_ATTR_DAMAGE_RATE_WATER = commonproto::EQUIP_ADD_ATTR_DAMAGE_RATE_WATER,
    EQUIP_ADD_ATTR_DAMAGE_RATE_FIRE = commonproto::EQUIP_ADD_ATTR_DAMAGE_RATE_FIRE,
    EQUIP_ADD_ATTR_DAMAGE_RATE_GRASS = commonproto::EQUIP_ADD_ATTR_DAMAGE_RATE_GRASS,
    EQUIP_ADD_ATTR_DAMAGE_RATE_LIGHT = commonproto::EQUIP_ADD_ATTR_DAMAGE_RATE_LIGHT,
    EQUIP_ADD_ATTR_DAMAGE_RATE_DARK = commonproto::EQUIP_ADD_ATTR_DAMAGE_RATE_DARK,
    EQUIP_ADD_ATTR_DAMAGE_RATE_GROUND = commonproto::EQUIP_ADD_ATTR_DAMAGE_RATE_GROUND,
    EQUIP_ADD_ATTR_DAMAGE_RATE_FORCE = commonproto::EQUIP_ADD_ATTR_DAMAGE_RATE_FORCE,

    EQUIP_ADD_ATTR_MAGIC_ENERGY = commonproto::EQUIP_ADD_ATTR_MAGIC_ENERGY,

    // 百分比加成属性
    EQUIP_ADD_ATTR_PERCENT_ALL  = commonproto::EQUIP_ADD_ATTR_PERCENT_ALL , // 全属性加成(能量不加)
    EQUIP_ADD_ATTR_PERCENT_HP   = commonproto::EQUIP_ADD_ATTR_PERCENT_HP  , // 生命
    EQUIP_ADD_ATTR_PERCENT_NATK = commonproto::EQUIP_ADD_ATTR_PERCENT_NATK, // 普攻
    EQUIP_ADD_ATTR_PERCENT_NDEF = commonproto::EQUIP_ADD_ATTR_PERCENT_NDEF, // 普防
    EQUIP_ADD_ATTR_PERCENT_SATK = commonproto::EQUIP_ADD_ATTR_PERCENT_SATK, // 技攻
    EQUIP_ADD_ATTR_PERCENT_SDEF = commonproto::EQUIP_ADD_ATTR_PERCENT_SDEF, // 技防

    EQUIP_ADD_ATTR_PERCENT_CRIT         = commonproto::EQUIP_ADD_ATTR_PERCENT_CRIT        ,   // 暴击
    EQUIP_ADD_ATTR_PERCENT_ANTI_CRIT    = commonproto::EQUIP_ADD_ATTR_PERCENT_ANTI_CRIT   ,   // 防暴
    EQUIP_ADD_ATTR_PERCENT_HIT          = commonproto::EQUIP_ADD_ATTR_PERCENT_HIT         ,   // 命中
    EQUIP_ADD_ATTR_PERCENT_DODGE        = commonproto::EQUIP_ADD_ATTR_PERCENT_DODGE       ,   // 闪避
    EQUIP_ADD_ATTR_PERCENT_BLOCK        = commonproto::EQUIP_ADD_ATTR_PERCENT_BLOCK       ,   // 格挡
    EQUIP_ADD_ATTR_PERCENT_BREAK_BLOCK  = commonproto::EQUIP_ADD_ATTR_PERCENT_BREAK_BLOCK ,   // 破格

    EQUIP_ADD_ATTR_PERCENT_ANTI_WATER = commonproto::EQUIP_ADD_ATTR_PERCENT_ANTI_WATER,
    EQUIP_ADD_ATTR_PERCENT_ANTI_FIRE = commonproto::EQUIP_ADD_ATTR_PERCENT_ANTI_FIRE,
    EQUIP_ADD_ATTR_PERCENT_ANTI_GRASS = commonproto::EQUIP_ADD_ATTR_PERCENT_ANTI_GRASS,
    EQUIP_ADD_ATTR_PERCENT_ANTI_LIGHT = commonproto::EQUIP_ADD_ATTR_PERCENT_ANTI_LIGHT,
    EQUIP_ADD_ATTR_PERCENT_ANTI_DARK = commonproto::EQUIP_ADD_ATTR_PERCENT_ANTI_DARK,
    EQUIP_ADD_ATTR_PERCENT_ANTI_GROUND = commonproto::EQUIP_ADD_ATTR_PERCENT_ANTI_GROUND,
    EQUIP_ADD_ATTR_PERCENT_ANTI_FORCE = commonproto::EQUIP_ADD_ATTR_PERCENT_ANTI_FORCE,

    EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_WATER = commonproto::EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_WATER,
    EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_FIRE = commonproto::EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_FIRE,
    EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_GRASS = commonproto::EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_GRASS,
    EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_LIGHT = commonproto::EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_LIGHT,
    EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_DARK = commonproto::EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_DARK,
    EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_GROUND = commonproto::EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_GROUND,
    EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_FORCE = commonproto::EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_FORCE,
};

//装备品级
enum equip_quality_t {
    EQUIP_QUALITY_NONE     = commonproto::EQUIP_QUALITY_NONE     , //非装备
    EQUIP_QUALITY_WHITE    = commonproto::EQUIP_QUALITY_WHITE    , //白色
    EQUIP_QUALITY_GREEN    = commonproto::EQUIP_QUALITY_GREEN    , //绿色
    EQUIP_QUALITY_GREEN1   = commonproto::EQUIP_QUALITY_GREEN1   , //绿色+1
    EQUIP_QUALITY_BLUE     = commonproto::EQUIP_QUALITY_BLUE     , //蓝色
    EQUIP_QUALITY_BLUE1    = commonproto::EQUIP_QUALITY_BLUE1    , //蓝色+1
    EQUIP_QUALITY_BLUE2    = commonproto::EQUIP_QUALITY_BLUE2    , //蓝色+2
    EQUIP_QUALITY_PURPLE   = commonproto::EQUIP_QUALITY_PURPLE   , //紫色
    EQUIP_QUALITY_PURPEL1  = commonproto::EQUIP_QUALITY_PURPEL1  , //紫色+1
    EQUIP_QUALITY_PURPLE2  = commonproto::EQUIP_QUALITY_PURPLE2  , //紫色+2
    EQUIP_QUALITY_PURPLE3  = commonproto::EQUIP_QUALITY_PURPLE3  , //紫色+3
    EQUIP_QUALITY_PINK     = commonproto::EQUIP_QUALITY_PINK     , //粉色
    EQUIP_QUALITY_PINK1    = commonproto::EQUIP_QUALITY_PINK1    , //粉色+1
    EQUIP_QUALITY_PINK2    = commonproto::EQUIP_QUALITY_PINK2    , //粉色+2
    EQUIP_QUALITY_PINK3    = commonproto::EQUIP_QUALITY_PINK3    , //粉色+3
    EQUIP_QUALITY_PINK4    = commonproto::EQUIP_QUALITY_PINK4    , //粉色+4
    EQUIP_QUALITY_ORANGE   = commonproto::EQUIP_QUALITY_ORANGE   , //橙色
    EQUIP_QUALITY_ORANGE1  = commonproto::EQUIP_QUALITY_ORANGE1  , //橙色+1
    EQUIP_QUALITY_ORANGE2  = commonproto::EQUIP_QUALITY_ORANGE2  , //橙色+2
    EQUIP_QUALITY_ORANGE3  = commonproto::EQUIP_QUALITY_ORANGE3  , //橙色+3
    EQUIP_QUALITY_ORANGE4  = commonproto::EQUIP_QUALITY_ORANGE4  , //橙色+4
    EQUIP_QUALITY_ORANGE5  = commonproto::EQUIP_QUALITY_ORANGE5  , //橙色+5
    EQUIP_QUALITY_GOLD     = commonproto::EQUIP_QUALITY_GOLD     , //金色
};

enum item_func_type_t {
    ITEM_FUNC_NONE             = commonproto::ITEM_FUNC_NONE            , //没有使用功能
    ITEM_FUNC_ADD_PLAYER_HP    = commonproto::ITEM_FUNC_ADD_PLAYER_HP   , //给玩家加血
    ITEM_FUNC_ADD_PLAYER_EXP   = commonproto::ITEM_FUNC_ADD_PLAYER_EXP  , //给玩家加经验
    ITEM_FUNC_ADD_PET_HP       = commonproto::ITEM_FUNC_ADD_PET_HP      , //给精灵加血
    ITEM_FUNC_ADD_PET_EXP      = commonproto::ITEM_FUNC_ADD_PET_EXP     , //给精灵加经验
    ITEM_FUNC_IMPROVE_TALENT   = commonproto::ITEM_FUNC_IMPROVE_TALENT  , //天赋升级道具
    ITEM_FUNC_PET_EVOLUTION    = commonproto::ITEM_FUNC_PET_EVOLUTION   , //精灵进化道具
    ITEM_FUNC_ADD_PET_EFFORT   = commonproto::ITEM_FUNC_ADD_PET_EFFORT  , //精灵学习力药水
    ITEM_FUNC_EQUIP_ARM        = commonproto::ITEM_FUNC_EQUIP_ARM       , //穿戴装备
    ITEM_FUNC_BUFF             = commonproto::ITEM_FUNC_BUFF            , //掉落BUFF
    ITEM_FUNC_CALL             = commonproto::ITEM_FUNC_CALL            , //招募道具
    ITEM_FUNC_ADD_VP           = commonproto::ITEM_FUNC_ADD_VP          , //体力丹道具
    ITEM_FUNC_OPEN_BOX         = commonproto::ITEM_FUNC_OPEN_BOX        , //开宝箱
    ITEM_FUNC_MAX
};

enum equip_elem_type_t {
    EQUIP_ELEM_TYPE_NONE    = commonproto::EQUIP_ELEM_TYPE_NONE, //没有属性
    EQUIP_ELEM_TYPE_WATER   = commonproto::EQUIP_ELEM_TYPE_WATER, //水
    EQUIP_ELEM_TYPE_FIRE    = commonproto::EQUIP_ELEM_TYPE_FIRE, //火
    EQUIP_ELEM_TYPE_GRASS   = commonproto::EQUIP_ELEM_TYPE_GRASS, //草
    EQUIP_ELEM_TYPE_LIGHT   = commonproto::EQUIP_ELEM_TYPE_LIGHT, //光
    EQUIP_ELEM_TYPE_DARK    = commonproto::EQUIP_ELEM_TYPE_DARK, //暗
    EQUIP_ELEM_TYPE_GROUND  = commonproto::EQUIP_ELEM_TYPE_GROUND, //地
    EQUIP_ELEM_TYPE_FORCE   = commonproto::EQUIP_ELEM_TYPE_FORCE, //武
};

enum pet_fragment_id_index_t
{
	PET_FRAGMENT_ID_INDEX_START = 40000,
	PET_FRAGMENT_ID_INDEX_END = 49999,
};

enum smelter_type_t
{
	EQUIP_SMELTER = 1,
	PET_FRAGMENT_SMELTER = 2,
};

struct item_attr_conf_t {
    uint32_t type;
    uint32_t value;
};

struct item_conf_t {
    uint32_t item_id; // 物品id
    equip_add_attr_t add_attr_type; //增加的属性类型
    uint32_t factor; //成长系数
    std::vector<equip_add_attr_t> chisel_attrs; //刻印转化的属性类型
    equip_quality_t base_quality;
    equip_quality_t quality_max;
    uint32_t next_quality_item;
    equip_body_pos_t equip_body_pos;
    //fashion_body_pos_t fashion_body_pos;
    //fashion_add_attr_t fashion_add_attr_type;
    //uint32_t fashion_add_attr_value;
    equip_elem_type_t elem_type; //装备属性特性
    uint32_t expire; //过期时间
    uint32_t own_max; //拥有上限
    prof_type_t role_type; //所属职业
    //<id, cnt>
    std::map<uint32_t, uint32_t> material; //升级需要消耗的物品
    item_func_type_t fun_type;//一次性物品type
    std::vector<uint32_t> fun_args;//一次性物品args
    std::vector<item_attr_conf_t> item_attr_list; // 加成多种属性
    uint32_t level_limit; //玩家等级限制
    uint32_t slot_max;
    uint32_t sale_price;
    uint32_t attr_id;//属性物品对应的属性
    uint32_t buff_id; //坐骑翅膀对应的前端战斗buff_id, buff.xml里面的
    uint32_t cult_level_limit;  // 坐骑或翅膀使用等级限制
    uint32_t req_vip_lv;    // 使用物品需要的vip等级
    uint32_t auto_use; //得到物品后是否自动使用
    uint32_t is_magic_equip; //是否魔法装备
    uint32_t star; //魔法装备的星级
    uint32_t base_buff1_ID; //固定的随机BUFF包ID
    uint32_t base_buff2_ID; //魔法的随机BUFF包1
    uint32_t base_buff3_ID; //魔法的随机buff包2
    uint32_t base_buff4_ID; //洗练的随机buff包
    string name;
};


class item_conf_mgr_t {
public:
    item_conf_mgr_t() {
        clear();
    }
    ~item_conf_mgr_t() {
        clear();
    }
    inline void clear() {
        item_conf_map_.clear();
    }

    inline const std::map<uint32_t, item_conf_t> &const_item_conf_map() const {
        return item_conf_map_;
    }

    inline void copy_from(const item_conf_mgr_t &m) {
        item_conf_map_ = m.const_item_conf_map();
    }

    inline bool is_item_conf_exist(uint32_t item_id) {
        if (item_conf_map_.count(item_id) > 0) return true;
        return false;
    }

    inline bool is_equip(uint32_t item_id) {
        const item_conf_t *item_conf = find_item_conf(item_id);
        if (!item_conf) {
            return false;
        }
        if (item_conf->base_quality == EQUIP_QUALITY_NONE) {
            return false;
        }
        return true;
    }

    inline bool is_quench_equip(uint32_t item_id) {
        const item_conf_t *item_conf = find_item_conf(item_id);
        if (!item_conf) {
            return false;
        }
        //特殊不可洗练装备
        switch (item_id) {
        case 12099:
            return false;
        default:
            break;
        }
        if (item_conf->equip_body_pos >= EQUIP_BODY_POS_HEAD 
            && item_conf->equip_body_pos <= EQUIP_BODY_POS_FOOT) {
            return true;
        }
        return false;
    }

    inline bool is_cultivate_equip(uint32_t item_id) {
        const item_conf_t *item_conf = find_item_conf(item_id);
        if (!item_conf) {
            return false;
        }

        if (item_conf->equip_body_pos == EQUIP_BODY_POS_MOUNT ||
                item_conf->equip_body_pos == EQUIP_BODY_POS_WING) {
            return true;
        }

        return false;
    }

    int can_equip_cultivate_item(player_t *player, uint32_t item_id);

    inline bool add_item_conf(const item_conf_t &item) {
        if (is_item_conf_exist(item.item_id)) return false;
        item_conf_map_[item.item_id] = item; return true;
    }

    inline const item_conf_t *find_item_conf(uint32_t item_id) {
        if (!is_item_conf_exist(item_id)) return 0;
        return &((item_conf_map_.find(item_id))->second);
    }

    static inline bool is_valid_equip_body_pos(uint32_t pos) {
        return (pos && pos < (uint32_t)(EQUIP_BODY_POS_END));
    }

    static inline bool is_valid_equip_add_attr(uint32_t attr) {
        // return (attr && attr <= (uint32_t)(EQUIP_ADD_ATTR_DAMAGE_RATE_FORCE));
        return (attr && attr <= (uint32_t)(EQUIP_ADD_ATTR_MAGIC_ENERGY));
    }

    static inline bool is_valid_equip_add_percent_attr(uint32_t attr) {
        return (attr >= (uint32_t)EQUIP_ADD_ATTR_PERCENT_ALL && 
                attr <= (uint32_t)(EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_FORCE));
    }

    static inline bool is_valid_equip_quality(uint32_t quality) {
        return (quality && quality <= (uint32_t)(EQUIP_QUALITY_GOLD));
    }
    
    /*
    static inline bool is_valid_fashion_body_pos(uint32_t pos) {
        return (pos && pos <= (uint32_t)(FASHION_BODY_POS_WING));
    }
    */
    static inline bool is_valid_item_func_type(uint32_t type) {
        return (type && type < (uint32_t)(ITEM_FUNC_MAX));
    }

    /*
    static inline bool is_valid_fashion_add_attr(uint32_t type) {
        return (type && type <= (uint32_t)(FASHION_ADD_ATTR_SDEF));
    }
    */
    static inline bool is_valid_elem_type(uint32_t type) {
        return (type && type <= (uint32_t)(EQUIP_ELEM_TYPE_FORCE));
    }

    static uint32_t get_default_equip_item_id(uint32_t equip_pos) {
        switch ((equip_body_pos_t)equip_pos) {
        case EQUIP_BODY_POS_HEAD:
            return 10000;
        case EQUIP_BODY_POS_BODY:
            return 11000;
        case EQUIP_BODY_POS_HAND:
            return 12000;
        case EQUIP_BODY_POS_LEG:
            return 13000;
        case EQUIP_BODY_POS_FOOT:
            return 14000;
        default:
            return 0;
        }
    }
    static uint32_t get_equip_trans_major_rate_by_quality(uint32_t quality) {
        static uint32_t _rate_table_[EQUIP_QUALITY_GOLD] = {
            20, 22, 25, 29, 34, 
            40, 47, 55, 64, 74,
            85, 97, 110, 124, 139,
            155, 172, 190, 209, 229,
            250, 272
        };
        if (!is_valid_equip_quality(quality)) {
            return 0;
        }
        return _rate_table_[quality - 1];
    }

    static uint32_t get_equip_trans_minor_rate_by_quality(uint32_t quality) {
        if (!is_valid_equip_quality(quality)) {
            return 0;
        }
        return 4 + quality;
    }

    static uint32_t get_equip_elem_damage_rate(uint32_t quality) {
        static uint32_t _rate_table_[EQUIP_QUALITY_GOLD] = {
            5, 10, 15, 20, 25, 
            30, 35, 40, 45, 50, 
            55, 60, 65, 70, 75, 
            80, 85, 90, 95, 100,
            105, 110
        };
        if (!is_valid_equip_quality(quality)) {
            return 0;
        }
        return _rate_table_[quality - 1];
    }

    static uint32_t get_equip_anti_value(uint32_t quality) {
        return 50 * quality;
    }

    //是否属性物品
    inline bool is_attr_item(uint32_t item_id) {
        const item_conf_t *item_conf = find_item_conf(item_id);
        if (!item_conf) return false;
        return (item_conf->attr_id ?true :false);
    }

    //是否可以防沉迷减半的属性物品
    inline bool is_addicted_attr_item(uint32_t item_id) {
        const item_conf_t *item_conf = find_item_conf(item_id);
        if (!item_conf) return false;
        if (item_conf->attr_id == kAttrExp 
            || item_conf->attr_id == kAttrGold
            || item_conf->attr_id == kAttrDiamond
            || item_conf->attr_id == kAttrExpeditionMoney
            || item_conf->attr_id == kAttrElemDupCoin
            || item_conf->attr_id == kAttrPetExp) {
            return true;
        }
        return false;
    }

    //根据当前物品ID找出另一个相同的物品的ID(收费->免费, 免费->收费)
    static inline uint32_t get_another_item_id(uint32_t item_id) {
        if (item_id >= ITEM_FC_DIFF) {
            return item_id - ITEM_FC_DIFF;
        } else {
            return item_id + ITEM_FC_DIFF;
        }
    }

private:
    std::map<uint32_t, item_conf_t> item_conf_map_;
};

extern item_conf_mgr_t g_item_conf_mgr;

enum equip_buff_elem_type_t {
    equip_buff_elem_type_start = 1,
    equip_buff_elem_type_attr   = 1, //加属性
    equip_buff_elem_type_buff   = 2, //加buff
    equip_buff_elem_type_end,
};

struct equip_buff_conf_t {
    equip_buff_conf_t() {
        elem_id = 0;
        type = equip_buff_elem_type_start;
        target_id = 0;
        weight = 0;
        min = 0;
        max = 0;
        method = 1;
        magic_power = 0;
    }
    uint32_t elem_id;
    equip_buff_elem_type_t type;
    uint32_t target_id; //属性ID buffID
    uint32_t weight; //权重
    uint32_t min; //对于范围类型的min
    uint32_t max; //对于范围类型的max
    uint32_t method; //对于范围类型的attr 计算方式 1: rand(min, max) 2:公式值rand(min, max)
    uint32_t magic_power; //魔法战力值
};

struct equip_buff_rand_group_t {
    uint32_t group_id;
    std::vector<equip_buff_conf_t> equip_buff_conf_vec;
};

class equip_buff_rand_manager_t {
public:
    equip_buff_rand_manager_t() {
        equip_buff_map_.clear();
    }
    ~equip_buff_rand_manager_t() {
        equip_buff_map_.clear();
    }
    typedef std::map<uint32_t, equip_buff_rand_group_t> equip_buff_map_t;
    typedef std::map<uint32_t, equip_buff_rand_group_t>::iterator equip_buff_map_iter_t;
public:
    bool equip_buff_rand_group_exist(uint32_t group_id) {
        if (equip_buff_map_.count(group_id) == 0) {
            return false;
        }
        return true;
    }

    const equip_buff_rand_group_t *find_equip_buff_rand_group(uint32_t group_id) {
        if (!equip_buff_rand_group_exist(group_id)) {
            return 0;
        }
        return &(equip_buff_map_.find(group_id)->second);
    }

    equip_buff_rand_group_t *find_mutable_equip_buff_rand_group(uint32_t group_id) {
        if (!equip_buff_rand_group_exist(group_id)) {
            return 0;
        }
        return &(equip_buff_map_.find(group_id)->second);
    }
    bool add_equip_buff_rand_group(const equip_buff_rand_group_t &ebrg) {
        if (equip_buff_rand_group_exist(ebrg.group_id)) {
            return false;
        }
        equip_buff_map_[ebrg.group_id] = ebrg;
        return true;
    }

    const equip_buff_conf_t* rand_from_equip_buff_group(uint32_t group_id) {
        const equip_buff_rand_group_t *ebrg = find_equip_buff_rand_group(group_id);
        if (!ebrg) return 0;
        std::map<uint32_t, uint32_t> weight_map;
        FOREACH((ebrg->equip_buff_conf_vec), it) {
            weight_map[(*it).elem_id] = (*it).weight;
        }
        std::set<uint32_t> hit_set;
        Utils::rand_select_uniq_m_from_n_with_r(weight_map, hit_set, 1);
        uint32_t idx = *(hit_set.begin());
        if (idx > ebrg->equip_buff_conf_vec.size()) {
            ERROR_TLOG("Rand from equip_buff_group:%u get_idx:%u out of range[0-%u]", 
                    group_id, idx, ebrg->equip_buff_conf_vec.size() - 1);
            return 0;
        }
        return &(ebrg->equip_buff_conf_vec[idx - 1]);
    }

    const equip_buff_map_t& const_equip_buff_group_map() {
        return equip_buff_map_;
    }

    void copy_from(equip_buff_rand_manager_t &mgr) {
        equip_buff_map_ = mgr.const_equip_buff_group_map();
    }

    void show() {
        FOREACH(equip_buff_map_, it) {
            TRACE_TLOG("EquipBuffRandGroup id:%u", it->first);
            FOREACH((it->second.equip_buff_conf_vec), it2) {
                const equip_buff_conf_t &ebc = *it2;
                TRACE_TLOG("\telem_id:%u type:%u target:%u weight:%u min:%u max:%u",
                        ebc.elem_id, (uint32_t)(ebc.type), ebc.target_id, ebc.weight, ebc.min, ebc.max);
            }
        }
    }
private:
    equip_buff_map_t equip_buff_map_;
};

extern equip_buff_rand_manager_t g_equip_buff_rand_mgr;

#endif //__ITEM_CONF_H__

