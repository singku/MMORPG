#ifndef __DUPLICATE_H__
#define __DUPLICATE_H__

#include "common.h"

#define MAX_DUPLICATE_ID    (100000)

enum duplicate_limit_type_t {
    DUP_LIMIT_DAILY = 1, //每日限制次数
    DUP_LIMIT_WEEKLY = 2, //每周限制次数
};

enum duplicate_pet_type_t {
    DUP_PET_TYPE_ENEMY = 0, //敌军
    DUP_PET_TYPE_FRIEND = 1, //友军
};

enum duplicate_battle_type_t {
    DUP_BTL_TYPE_ERR = 0,
    DUP_BTL_TYPE_PVE = 1,   //pve
    DUP_BTL_TYPE_PPVE = 2, //多人pve
    DUP_BTL_TYPE_PVP = 3, //pvp
    DUP_BTL_TYPE_WORLD_BOSS = 4, //世界BOSS
    DUP_BTL_TYPE_RPVP = 5, //真实手动PVP
    DUP_BTL_TYPE_PVEP = 6, //半手动PVP
    DUP_BTL_TYPE_PEVE = 7, //带ai队友打副本
	DUP_BTL_TYPE_END,
};

//副本难度及活动类型(普通 精英 元素挑战 试炼 一桶天下...)
enum duplicate_mode_t {
    DUP_MODE_TYPE_ERR    = 0,
    DUP_MODE_TYPE_NORMAL = 1, //普通
    DUP_MODE_TYPE_ELITE  = 2, //精英
    DUP_MODE_TYPE_ELEM_DUP  = 3, //元素挑战
    DUP_MODE_TYPE_BUCKET = 4, //一桶天下
	DUP_MODE_TYPE_TRIAL = 5,	//试练
    DUP_MODE_TYPE_STARTER   = 6, //新手副本
    DUP_MODE_TYPE_MONSTER_CRISIS = 7, //怪物危机
    DUP_MODE_TYPE_FAMILY = 8, //家族副本
    DUP_MODE_TYPE_RPVP = 9, //即时竞技场
    DUP_MODE_TYPE_WORLD_BOSS = 10, //世界boss
    DUP_MODE_TYPE_NIGHT_RAID = 11, //夜袭
	DUP_MODE_TYPE_MAYIN_BUCKET = 12,	//玛音打桶
	DUP_MODE_TYPE_MAYIN_DEFEAT_EMPIRE = onlineproto::DUP_MODE_TYPE_MAYIN_DEFEAT_EMPIRE,	
	DUP_MODE_TYPE_BLESS_PET  = onlineproto::DUP_MODE_TYPE_BLESS_PET,	//伙伴祈福
    DUP_MODE_TYPE_DARK_EYE_WILL = onlineproto::DUP_MODE_TYPE_DARK_EYE_WILL, //黑瞳的决心
    DUP_MODE_TYPE_CHALLENGE_DEMON = onlineproto::DUP_MODE_TYPE_CHALLENGE_DEMON, //挑战修罗化身
	DUP_MODE_TYPE_DAILY_ACTIVITY = onlineproto::DUP_MODE_TYPE_DAILY_ACTIVITY,	//活动副本
	DUP_MODE_TYPE_ACTIVITY = onlineproto::DUP_MODE_TYPE_ACTIVITY,
	DUP_MODE_TYPE_STAR_PET = onlineproto::DUP_MODE_TYPE_STAR_PET,
    DUP_MODE_TYPE_END
};

enum duplicate_pet_flush_type_t {
    DUP_PET_FLUSH_TYPE_DEFAULT = 0, //不按波刷怪
    DUP_PET_FLUSH_TYPE_PHASE = 1, //按波刷怪
};

enum duplicate_pet_flush_trigger_t {
    DUP_PET_FLUSH_TRIG_TYPE_SVR = 0, //服务器自动刷怪
    DUP_PET_FLUSH_TRIG_TYPE_CLI = 1, //前端请求刷怪
};

//副本开放类型
enum duplicate_open_type_t {
    DUP_OPEN_TYPE_ALL = 0, //全开放
    DUP_OPEN_TYPE_DATE = 1, //限时开放
};

/** 
 * @brief 副本结算类型
 */
enum duplicate_result_compute_type_t {
   DUP_RESULT_NORMAL_TYPE = 0, // 正常战斗
   DUP_RESULT_ONE_KEY_TYPE = 1, // 扫荡
};


/** 
 * @brief 副本通过星级
 */
enum duplicate_star_type_t {
  DUP_PASS_STAR_ONE = 1,     // 一星
  DUP_PASS_STAR_TWO = 2,     // 二星
  DUP_PASS_STAR_THREE = 3,   // 三星
};

/** 
 * @brief 其他的副本常量数据
 */
enum duplicate_common_data_t {
	DUP_MONS_BOSS_CARD_BASE = 5,	//怪物危机boss关卡基值
};


struct duplicate_consume_item_t {
    uint32_t item_id;
    uint32_t cnt;
};

//副本
struct duplicate_t {
    uint32_t duplicate_id; //副本id
	uint32_t prev_id;	//前置副本id
    duplicate_mode_t mode; //副本难度类型
    duplicate_battle_type_t battle_type; //副本战斗类型 pvp pve ppve
    uint32_t open_tm_sub_key; //副本开放的时间 在tm_conf中的定义 11下的子项
    uint32_t least_users; //至少要几个人才能开打
    uint32_t time_limit; //副本完成时间限制
    uint32_t star_1_time; //一星评价需要的时间
    uint32_t star_2_time; //两星评价需要的时间
    uint32_t star_3_time; //三星评价需要的时间
    uint32_t level_limit; //玩家等级限制
    uint32_t consume_physique; //消耗的体力
    uint32_t consume_gold; //消耗的金币
    uint32_t consume_diamond; //消耗的钻石
    duplicate_limit_type_t limit_type;
    uint32_t initial_cnt;//普通玩家进入的初始次数
    uint32_t vip_initial_cnt;//vip进入的初始次数
    uint32_t svip_initial_cnt;//vip进入的初始次数
    uint32_t cd_time ;//cd时间
    uint32_t revivable; //是否可复活
    uint32_t total_enter_count_limit_key; // 副本进入次数key，多个副本可以配同一个key，表示共用限制
    uint32_t buy_total_count_limit_key; // 购买的副本进入次数key,多个副本可以配同一个key，表示共用限制
    uint32_t switch_days;  //距离元素挑战副本下一次切换的时间(天) 
    uint32_t element_id; //元素挑战副本属性id(1-7)
    uint32_t next_element_id;//元素挑战副本切换的目标属性id(1-7)
    uint32_t element_progress_attr; //记录元素挑战副本进度的属性字段id
    uint32_t element_level;         //元素挑战副本等级
    uint32_t area_num;         // 副本分区编号
    uint32_t area_star_sum_attr;    // 副本分区累计星级记录属性
    std::vector<duplicate_consume_item_t> consume_items; //消耗的物品
    std::vector<uint32_t> prize_vec; //副本结算奖励 prize.xml中的id
    std::vector<uint32_t> vip_prize_vec; //副本结算奖励 prize.xml中的id
    std::vector<uint32_t> svip_prize_vec; //副本结算奖励 prize.xml中的id

    std::vector<uint32_t> mon_vec; //要刷的怪的列表
    string name;
    uint32_t task_id; //进入副本必须先接取或完成的任务id
    uint32_t req_power; //要求战力
    uint32_t map_id;  // 副本地图id
    uint32_t area_id;   // 副本所属区域id
	uint32_t achieve_task_id;	//该副本对应的多条件成就任务id
	uint32_t buy_pass_shop_id;
	std::vector<uint32_t> can_fight_pets_id;
	uint32_t have_pet_id;
	uint32_t buy_fight_cnt_limit;
	uint32_t vip_buy_fight_cnt_limit;
	uint32_t svip_buy_fight_cnt_limit;
	uint32_t buy_fight_shop_id;
	uint32_t default_buy_fight_cnt;
};

//副本配置管理器
class duplicate_conf_manager_t {
public:
    duplicate_conf_manager_t() {
        dup_map_.clear();
    }
    ~duplicate_conf_manager_t() {
        dup_map_.clear();
    }
public: //inline functions
    inline bool duplicate_exist(uint32_t dup_id) {
        if (dup_map_.count(dup_id) > 0) {
            return true;
        }
        return false;
    }
    inline bool add_duplicate(duplicate_t &dup) {
        if (duplicate_exist(dup.duplicate_id)) {
            return false;
        }
        dup_map_[dup.duplicate_id] = dup;
        return true;
    }
	inline void add_dup_id_to_mode_dup_ids(duplicate_t &dup) {
		if (mode_dup_ids_.count((uint32_t)dup.mode)) {
			std::map<uint32_t, std::vector<uint32_t> >::iterator it 
				= mode_dup_ids_.find((uint32_t)dup.mode);
			std::vector<uint32_t>& dup_ids_inf = it->second;
			dup_ids_inf.push_back(dup.duplicate_id);
		} else {
			std::vector<uint32_t> dup_ids;
			dup_ids.push_back(dup.duplicate_id);
			mode_dup_ids_.insert(make_pair((uint32_t)dup.mode, dup_ids));
		}
	}
	uint32_t print_mode_dup_ids() {
		FOREACH(mode_dup_ids_, it) {
			FOREACH(it->second, iter) {
				TRACE_TLOG("the dup_id=[%u],mode=[%u]", *iter, it->first);
			}
		}
		return 0;
	}
	inline uint32_t get_dup_ids_by_mode(
			uint32_t mode, std::vector<uint32_t>& dup_ids) {
		if (mode_dup_ids_.count(mode) == 0) {
			return cli_err_mode_dup_ids_not_exist;
		}
		dup_ids = (mode_dup_ids_.find(mode))->second;
		std::sort(dup_ids.begin(), dup_ids.end());
		return 0;
	} 
	
    const inline duplicate_t *find_duplicate(uint32_t dup_id) {
        if (dup_map_.count(dup_id) == 0) {
            return NULL;
        }
        return &((dup_map_.find(dup_id))->second);
    }
    duplicate_battle_type_t get_duplicate_btl_type(uint32_t dup_id) {
        const duplicate_t *dup = find_duplicate(dup_id);
        if (!dup) {
            return DUP_BTL_TYPE_ERR;
        }
        return dup->battle_type;
    }

    duplicate_mode_t get_duplicate_mode(uint32_t dup_id) {
        const duplicate_t *dup = find_duplicate(dup_id);
        if (!dup) {
            return DUP_MODE_TYPE_ERR;
        }
        return dup->mode;
    }

    string get_duplicate_name(uint32_t dup_id) {
        const duplicate_t *dup = find_duplicate(dup_id);
        if (!dup) {
            return "";
        }
        return dup->name;
    }

    inline void copy_from(const duplicate_conf_manager_t &m) {
        dup_map_.clear();
        dup_map_ = m.const_dup_map();
		mode_dup_ids_.clear();
		mode_dup_ids_ = m.const_mode_dup_ids();
    }

    const std::map<uint32_t, duplicate_t>& const_dup_map() const{
        return dup_map_;
    }
	
	const std::map<uint32_t, std::vector<uint32_t> >& const_mode_dup_ids() const {
		return mode_dup_ids_;
	}

private:
    std::map<uint32_t, duplicate_t> dup_map_;
	//key:mode;  value: 该mode所有的副本id
	std::map<uint32_t, std::vector<uint32_t> > mode_dup_ids_;
};

// <star_num, prize_id>
typedef std::map<uint32_t, uint32_t> dup_area_prize_t;

class dup_area_prize_conf_manager_t {
    public:
        dup_area_prize_conf_manager_t() {
        }
        ~dup_area_prize_conf_manager_t() {
        }

        bool add_dup_area_prize_conf(uint32_t id, dup_area_prize_t &area_conf) {
            std::map<uint32_t, dup_area_prize_t>::iterator iter = 
                dup_area_conf_map_.find(id);
            if (iter != dup_area_conf_map_.end()) {
                return false;
            }
            dup_area_conf_map_.insert(
                    std::pair<uint32_t, dup_area_prize_t>(id, area_conf));
            return true;
        }

       dup_area_prize_t *get_dup_area_prize_conf(uint32_t id) {
            std::map<uint32_t, dup_area_prize_t>::iterator iter = 
                dup_area_conf_map_.find(id);
            if (iter == dup_area_conf_map_.end()) {
                return NULL;
            }

            return &(iter->second);
        }

        const inline std::map<uint32_t, dup_area_prize_t > const_dup_area_conf_map() const {
            return dup_area_conf_map_;
        } 
        inline void copy_from(const dup_area_prize_conf_manager_t &m) {
            dup_area_conf_map_ = m.const_dup_area_conf_map();
        }

    private:
        std::map<uint32_t, dup_area_prize_t> dup_area_conf_map_; 
};

#endif
