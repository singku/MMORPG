#ifndef ATTR_H
#define ATTR_H

#include "common.h"

enum attr_const_t{
    kAttrMaxNoLimit = 0xFFFFFFFF,
    kMinDailyAttrType = 10001,
    kMaxDailyAttrType = 20000,
    kMinWeeklyAttrType = 20001,
    kMaxWeeklyAttrType = 30000,
    kMinMonthlyAttrType = 30001,
    kMaxMonthlyAttrType = 40000,
    kMinPetOwnAttrType = 60001,
    kMaxPetOwnAttrType = 70000,
    kAttrServiceStartID = 70001,
    kAttrServiceEndID = 90000,

    kMinDupDailyAttr = 2000001,
    kMinDupDailyAttr1 = 2100001,
    kMaxDupDailyAttr = 3000000,
    kMinDupWeeklyAttr = 3000001,
    kMaxDupWeeklyAttr = 4000000,
};

struct attr_config_t
{
    attr_config_t() {
        id = 0;
        max = kAttrMaxNoLimit;
        vip_max = kAttrMaxNoLimit;
        svip_max = kAttrMaxNoLimit;
        initial = 0;
        vip_initial = 0;
        svip_initial = 0;
        daily_limit_key = 0;
        daily_limit_max = 0;
        daily_restrict = 0;
    }
    uint32_t id;
    uint32_t max;
    uint32_t vip_max;
    uint32_t svip_max;
    uint32_t initial;
    uint32_t vip_initial;
    uint32_t svip_initial;
    uint32_t daily_limit_key;
    uint32_t daily_limit_max;
    uint32_t daily_restrict;
};

struct attr_data_info_t
{
    uint32_t type;
    uint32_t value;
};

struct attr_data_chg_info_t
{
	attr_data_chg_info_t() {
		type = 0;
		change_value = 0;
		max_value = 0xFFFFFFFF;
	}
    uint32_t type;
    uint32_t change_value;
	bool is_minus;	//是否减属性;
	uint32_t max_value;
};

class Attr
{
public:

    uint32_t get_value(enum attr_type_t type);
    
    //插入一个attr，对于重复的key为更新操作。
    inline void put_attr(uint32_t type, uint32_t value) {
        attrs[type] = value;
    }
    inline void put_attr(const attr_data_info_t &attr_data) {
        attrs[attr_data.type] = attr_data.value;
    }

    //返回attr的数目
    inline int get_attr_num()
    {
        return attrs.size();
    }

    //返回非0表示type不存在
    int get_value_by_type(enum attr_type_t type, uint32_t* value);
    inline std::map<uint32_t, uint32_t>* get_attrs()
    {
        return &attrs;
    }

    /**
     * @brief  ranged_clear 清除指定范围的attr
     *
     * @param low 最小值, 包含
     * @param high 最大值, 包含
     * @param type_list 返回删除列表
     */
    void ranged_clear(uint32_t low, uint32_t high, std::vector<uint32_t>& type_list);

    inline void clear() {
        attrs.clear(); 
    }

    inline bool has_attr(uint32_t type) {
        return (attrs.count(type) != 0);
    }
private:

    typedef std::map<uint32_t, uint32_t>::iterator attr_ptr_t;
    
    std::map<uint32_t, uint32_t> attrs;
};

#endif
