#ifndef __PACKAGE_H__
#define __PACKAGE_H__

#include <map>
#include <set>
#include <stdio.h>
#include <vector>
#include <libtaomee/timer.h>
#include <stdint.h>

#include "item_conf.h"
#include "proto/client/common.pb.h"

struct player_t;

const uint32_t kMaxSlotVolume = 99999;

/**
 * @brief  item_t 存储用户物品信息
 */
struct item_t {
    uint32_t item_id; // 物品id
    uint32_t slot_id; // 物品在背包中的位置
    uint32_t count; // 物品数量
    uint32_t expire_time; // 物品过期时间，0没有过期时间
    uint32_t using_count; //物品在使用中的数量
    std::string opt_attr;
    
    inline uint32_t usable_count() const 
    {
        uint32_t now = get_now_tv()->tv_sec;
        if (expire_time && now > expire_time) {
            return 0; 
        }

        if (using_count >= count) {
            return 0; 
        } else {
            return (count - using_count); 
        }
    }

    inline int reduce_n(uint32_t n) 
    {
        uint32_t usable_count = this->usable_count();
        if (n > usable_count) {
            return -1; 
        } else {
            count -= n;             
        }
     
        return 0;
    }
};


/**
 * @brief  Package 背包，物品集合
 */
class Package
{
public:
    Package();
    ~Package();

    int put_item(const item_t* item);
    
    int add_item(const item_t* item, std::vector<item_t>& result_list);

    int reduce_item_by_slot_id(uint32_t slot_id, uint32_t n, item_t* result);

    int reduce_item_by_item_id(uint32_t item_id, uint32_t n, std::vector<item_t>& result);
    
    int dump(FILE* file);

    int get_all_items(std::vector<item_t>& results);

    int get_fun_type_items(std::vector<item_t>& results, uint32_t fun_type);

    bool has_item(uint32_t item_id, bool must_not_expire = true);
public://inline
    //必须没过期
    item_t *find_usable_item(uint32_t item_id, int &err, uint32_t n = 1);

    inline const item_t *get_const_item_in_slot(uint32_t slot_id)
    {
        return get_item_in_slot(slot_id);
    }

    inline item_t *get_mutable_item_in_slot(uint32_t slot_id)
    {
        return get_item_in_slot(slot_id);
    }

    // 使用一件物品
    inline int use_item(uint32_t slot_id, item_t* result)
    {
        item_t* item = get_mutable_item_in_slot(slot_id);    

        if (item && item->using_count < item->count) {
            item->using_count++; 
            *result = *item;
        } else {
            return -1; 
        }

        return 0;
    }

    // 卸下一件物品
    inline int unuse_item(uint32_t slot_id, item_t* result)
    {
        item_t* item = get_mutable_item_in_slot(slot_id); 

        if (item && item->using_count > 0) {
            item->using_count--; 
            *result = *item;
        } else {
            return -1; 
        }

        return 0;
    }

    // 根据item_id使用一个物品
    int use_item_by_item_id(uint32_t item_id, item_t* result);

    // 根据item_id取消一个物品的使用
    int unuse_item_by_item_id(uint32_t item_id, item_t* result);

    uint32_t get_total_usable_item_count(uint32_t item_id);
 
   /**
     * @brief  get_all_usable_items 获取可使用的物品列表
     *
     * @param item_id 物品id
     * @param item_list 物品列表
     * @param usable_total_num 总数量
     */
    void get_items_by_id(uint32_t item_id, std::vector<item_t*>& item_list) {
        uint32_t usable_total_num;
        get_all_usable_items(item_id, item_list, &usable_total_num);
    }

    // 获取物品总数
    uint32_t get_total_item_count(uint32_t item_id);

    // 获取过期物品列表
    void clean_expired_items(std::vector<item_t>& result);

    void use_item_by_item_id(uint32_t item_id, uint32_t count, std::vector<item_t>& result_list);
    
    void update_item_id(item_t *item, uint32_t new_item_id);

    /**
     * @brief  计算出指定的品级装备的数量
     *
     * @param slot_item 对应背包格子上的物品
     * @param n 扣除数量n
     *
     * @return 0 扣除成功 -1 物品不足
     */
	uint32_t get_equip_count_by_quality(player_t* player,
			equip_quality_t quality, uint32_t& count);

private:
    typedef uint64_t item_slot_t;

    inline item_t* get_item_in_slot(uint32_t slot_id)
    {
        slots_ptr_t ptr = slots_.find(slot_id); 

        if (ptr == slots_.end()) {
            return NULL; 
        } else {
            return &ptr->second; 
        }
    }

    // 根据item_id和slot_id获取一个item_slot_t
    inline item_slot_t get_item_slot(uint32_t item_id, uint32_t slot_id)
    {
        item_slot_t item_slot = (((uint64_t)item_id << 32) | slot_id);
        return item_slot;
    }

    // 根据item_slot_t解析出item_id和slot_id
    inline void parse_item_slot(item_slot_t item_slot, 
            uint32_t* item_id, uint32_t* slot_id)
    {
        *item_id = item_slot >> 32; 
        *slot_id = item_slot & ((1L << 32) - 1);
    }

    // 查找物品对应的最小item_slot_t值
    inline item_slot_t get_item_min_item_slot(uint32_t item_id)
    {
        return get_item_slot(item_id, 1);
    }

    // 查找物品对应的最大item_slot_t值
    inline item_slot_t get_item_max_item_slot(uint32_t item_id)
    {
        return get_item_slot(item_id, ((1L << 32) - 1)); 
    }

    // 查找放着这个物品的最小格子号
    inline bool find_min_slot_has_item(uint32_t item_id, uint32_t* slot_id)
    {
        item_slot_t min_item_slot = get_item_min_item_slot(item_id); 
        item_slot_t max_item_slot = get_item_max_item_slot(item_id);
        item_slot_set_ptr_t ptr = item_slot_set_.lower_bound(min_item_slot);

        if (ptr == item_slot_set_.end()) {
            return false; 
        }

        if ((*ptr) >= min_item_slot && (*ptr) <= max_item_slot) {
            parse_item_slot(*(ptr), &item_id, slot_id);
            return true; 
        } else {
            return false;
        }
    }

    // 查找所有的格子放着这个物品
    void find_all_slot_has_item(uint32_t item_id, std::vector<uint32_t>& slot_list);

    inline bool is_exist_item_slot(uint32_t item_id, uint32_t slot_id)
    {
        item_slot_t item_slot = get_item_slot(item_id, slot_id);
        item_slot_set_ptr_t ptr = item_slot_set_.find(item_slot);
         
        if (ptr == item_slot_set_.end()) {
            return false; 
        } else {
            return true;  
        }
    }

    inline void put_item_slot(uint32_t item_id, uint32_t slot_id)
    {
        item_slot_t item_slot = get_item_slot(item_id, slot_id);    
        item_slot_set_.insert(item_slot);
    }

    inline void remove_item_slot(uint32_t item_id, uint32_t slot_id)
    {
        item_slot_t item_slot = get_item_slot(item_id, slot_id);
        item_slot_set_ptr_t ptr = item_slot_set_.find(item_slot);

        if (ptr != item_slot_set_.end()) {
            item_slot_set_.erase(ptr); 
        }
    }

    inline void remove_item_in_slot(uint32_t slot_id)
    {
        slots_ptr_t ptr = slots_.find(slot_id);  

        if (ptr != slots_.end()) {
            slots_.erase(ptr); 
        }
    }

    /**
     * @brief  clear_slot_item 物品数量为零时，清理这个槽
     *
     * @param slot_item 所在的槽
     *
     * @return 0 成功 -1 失败
     */
    int clear_slot_item(const item_t* slot_item);

    /**
     * @brief  get_all_usable_items 获取可使用的物品列表
     *
     * @param item_id 物品id
     * @param item_list 物品列表
     * @param usable_total_num 总数量
     */
    void get_all_usable_items(uint32_t item_id, 
            std::vector<item_t*>& item_list, uint32_t* usable_total_num);

    int find_next_free_slot_id(uint32_t start_id);

    /**
     * @brief  reduce_item 指定slot_item扣除n个
     *
     * @param slot_item 对应背包格子上的物品
     * @param n 扣除数量n
     *
     * @return 0 扣除成功 -1 物品不足
     */
    int reduce_item(item_t* slot_item, uint32_t n);

    typedef std::set<item_slot_t>::iterator item_slot_set_ptr_t;
    typedef std::map<uint32_t, item_t>::iterator slots_ptr_t;

    std::map<uint32_t, item_t> slots_; // slot_id => 物品信息
    std::set<item_slot_t> item_slot_set_; // 辅助数据结构，方便通过item_id查到slot_id
    uint32_t min_free_slot_id_; // 最小的空余格子
};

int check_item_in_pos(const player_t *p, uint32_t pos, uint32_t type = 0, uint32_t subtype = 0);

#endif
