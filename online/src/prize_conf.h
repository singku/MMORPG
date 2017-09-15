#ifndef __PRIZE_CONF_H__
#define __PRIZE_CONF_H__

#include "common.h"
#include "pet.h"

#define SELF_RAND_MODE  (-1)

enum prize_incr_type_t {
    prize_incr_type_default = 0, //默认值 不对prize的count改变
    prize_incr_type_muti_level = 1, //count乘以玩家的等级
    prize_incr_type_max,
};

struct prize_elem_t {
    void clear() {
        id = count = level = talent_level = award_rate = display_rate = 0;
        weekly_limit_key = weekly_limit_max = 0;
        daily_limit_key = daily_limit_max = 0;
        monthly_limit_key = monthly_limit_max = 0;
        forever_limit_key = forever_limit_max = 0;
        calc_type = prize_incr_type_default;
        adjust_type = 0;
		notice = 0;
        show = 0;
		// price_rate = 0;
		price_type = 0;
		price = 0;
		//first_dup_must_drop = 0;
    }
    uint32_t id; //物品ID 属性ID 精灵ID 称号ID 符文id
    int32_t count; //物品数量 属性数量 
    prize_incr_type_t calc_type;
    uint32_t duration; //有效期时间 以天为单位
    uint32_t level; //精灵等级, 符文等级
    int talent_level; //精灵天赋等级
    uint32_t award_rate; //获得概率
    uint32_t display_rate; //展示概率
    uint32_t daily_limit_key; // 每日上限key
    uint32_t daily_limit_max; // 每日上限max
    uint32_t weekly_limit_key; // 每周上限key
    uint32_t weekly_limit_max; // 每周上限max
    uint32_t monthly_limit_key; // 每月上限key
    uint32_t monthly_limit_max; // 每月上限max
    uint32_t forever_limit_key; // 永久上限key
    uint32_t forever_limit_max; // 永久上限max
    uint32_t notice ; // 是否加入奖励榜单（一般是好奖励）
    uint32_t show; //跑马灯的展示类型
    uint32_t adjust_type; //奖励限时翻倍的类型 默认为0 不调整
    // uint32_t price_rate; //明星招募，决定获取物品是否要使用price_type 获得
    uint32_t price_type; //明星招募，使用何种物品获得钻石，招募券、金币等
    uint32_t price; //明星招募，消耗物品个数
//	uint32_t first_dup_must_drop; //第一次玩副本击杀boss必掉该物品
};

struct prize_config_t {
    prize_config_t() {
        clear();
    }
    void clear() {
        prize_id = 0;
        rand_mode = 0;
        display_cnt = 0;
		show = 0;
        prize_items.clear();
        prize_attrs.clear();
        prize_pets.clear();
		prize_runes.clear();
		prize_titles.clear();
    }
    uint32_t prize_id;
    int32_t rand_mode;
	uint32_t show;
    uint32_t display_cnt;
    std::vector<prize_elem_t> prize_items;
    std::vector<prize_elem_t> prize_attrs;
    std::vector<prize_elem_t> prize_pets;
	std::vector<prize_elem_t> prize_runes;
	std::vector<prize_elem_t> prize_titles;
    char desc[128];
};

class prize_conf_manager_t {
public:
    prize_conf_manager_t() {
        clear_prize_conf();
    }
    ~prize_conf_manager_t() {
        clear_prize_conf();
    }

public:
    typedef std::map<uint32_t, prize_config_t> prize_map_t;
    typedef std::map<uint32_t, prize_config_t>::iterator prize_map_iter_t;

public: //inline
    inline void clear_prize_conf() {
        prizes_map_.clear();
    }
    inline bool is_prize_exist(uint32_t prize_id) {
        return (prizes_map_.count(prize_id) > 0);
    }
    inline bool add_prize_conf(prize_config_t &prize, bool is_by_force = true) {
        if (is_prize_exist(prize.prize_id)) return false;
        prizes_map_[prize.prize_id] = prize; return true;
    }
    inline const prize_map_t &get_const_prize_map() {
        return prizes_map_;
    }
    inline void copy_from(prize_conf_manager_t &src) {
        prizes_map_ = src.get_const_prize_map();
    }
public:
    const prize_config_t* get_prize_conf(uint32_t prize_id) {
        if (!is_prize_exist(prize_id)) return 0;
        return &(prizes_map_.find(prize_id)->second);
    }
    prize_config_t *get_mutable_prize_conf(uint32_t prize_id) {
        if (!is_prize_exist(prize_id)) return 0;
        return &(prizes_map_.find(prize_id)->second);
    }
    const string get_prize_desc(uint32_t prize_id) {
        const prize_config_t *p_conf = get_prize_conf(prize_id);
        if (!p_conf) {
            return "未知奖励";
        }
        return std::string(p_conf->desc);
    }
    uint32_t get_prize_show_type(uint32_t prize_id) {
        const prize_config_t *prize_conf = get_prize_conf(prize_id);
        if (prize_conf) {
            return prize_conf->show;
        }
        return 0;
    }
private:
    prize_map_t prizes_map_;
};


#endif
