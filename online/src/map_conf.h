#ifndef __MAP_CONF_H__
#define __MAP_CONF_H__

#define HOME_MAP_ID    (15)
#define FAMILY_MAP_ID  (24)
//alter by kevin[2014/08/08] 15号地图被小屋使用
#define VOCATION_MAP_ID (15)
#define MEDAL_HALL_MAP_ID (16)
#define FAMILY_BOSS_MAP_ID (21)
#define SNOWBALL_MAP_ID (22)
#define FAMILY_BATTLE_MAP_ID (6004)
#define MAP_ID_ARENA (16)
#define SOLON_FIELD_MAP_ID (6006)
#define THUNDER_GOD_FIELD_MAP_ID (6007)
#define SOLID_STONE_FIELD_MAP_ID (6018)
#define MIRACLE7_MAP_ID (5007)
#define DARK_TUHNDER_MAP_ID (6026)
#define BUSINESS_STREET (3)

#include "common.h"
#include <set>

const uint32_t kMaxMapMonsterNum = 20;

struct point_t {
point_t(uint32_t xx, uint32_t yy) : x(xx), y(yy) {}
  uint32_t x;
  uint32_t y;
    
};

struct map_conf_t {
    uint32_t id; // 地图id
    std::vector<point_t> init_pos;//初始坐标
    uint32_t big_map_id; //大地图显示id
    uint32_t is_dup_scene; //是否地图场景
    std::set<uint32_t> switchable_mapid_set; //连接地图id集合
    uint32_t levelLimit; //推荐等级
    uint32_t index;//副本所属地图顺序
    bool be_auto_fight;//是否可以挂机
    bool be_pvp;//是否可以对战
    bool be_peace;//是否可以和平模式
    bool be_into_die_map;//死亡后进入死亡地图 
    bool be_use_skill;//0不可以, 1可以
    bool die_map; //0死亡返回主城, 1死亡原进原出
    char name[64];
    uint32_t is_private;
};

class map_conf_mgr_t {
public:
    map_conf_mgr_t() {
        clear();
    }
    ~map_conf_mgr_t() {
        clear();
    }
    inline void clear() {
        map_conf_map_.clear();
    }
    inline const std::map<uint32_t, map_conf_t> &const_map_conf_map() const {
        return map_conf_map_;
    }
    inline void copy_from(const map_conf_mgr_t &m) {
        map_conf_map_ = m.const_map_conf_map();
    }
    inline bool is_map_conf_exist(uint32_t map_id) {
        if (map_conf_map_.count(map_id) > 0) return true;
        return false;
    }
    inline bool add_map_conf(const map_conf_t &map) {
        if (is_map_conf_exist(map.id)) return false;
        map_conf_map_[map.id] = map; return true;
    }
    inline const map_conf_t *find_map_conf(uint32_t map_id) {
        if (!is_map_conf_exist(map_id)) return 0;
        return &((map_conf_map_.find(map_id))->second);
    }

private:
    std::map<uint32_t, map_conf_t> map_conf_map_;
};

extern map_conf_mgr_t g_map_conf_mgr;
#endif //__MAP_CONF_H__

