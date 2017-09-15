#ifndef ITEM_H
#define ITEM_H

#include <libtaomee/timer.h>
#include <string>
#include <stdint.h>

#include "common.h"
#include "item_conf.h"

struct attachment_t;
struct player_t;
struct item_t;

struct item_slot_info_t {
	uint32_t slot_id;
	uint32_t count;
};

struct add_item_info_t {
    add_item_info_t(uint32_t item_id_ = 0, uint32_t count_ = 0, uint32_t expire_time_ = 0, uint32_t level_ = 0) 
        : item_id(item_id_),count(count_),expire_time(expire_time_), level(level_) { }
    
    uint32_t item_id;
    uint32_t count;//num to be add
    uint32_t expire_time;
    uint32_t level; //装备的话 给一个等级
};

struct reduce_item_info_t {
    uint32_t item_id;
    uint32_t count;//num to be reduce
};

int db_item_change(player_t* player, 
        const std::vector<item_t>& items,
        onlineproto::sync_item_reason_t reason = onlineproto::SYNC_REASON_NONE,
        bool noti_player = true);

/**
 * @brief  swap_item_by_item_id 添加/扣除物品
 *
 * @param player 玩家结构体
 * @param reduce_vec 要扣减的物品
 * @param add_vec 要增加的物品
 * @param wait 是否等待db
 * @param reason 同步前端原因, 默认未知
 * @param addict_detec 是否开启防沉迷检查,true 开启；false关闭
 *
 * @return 0 成功 其他失败
 *
 * care 不要使用这个接口去扣金币
 */
int swap_item_by_item_id(player_t* player,
        std::vector<reduce_item_info_t> *reduce_vec,
        std::vector<add_item_info_t> *add_vec,
        bool wait = false, 
        onlineproto::sync_item_reason_t reason = onlineproto::SYNC_REASON_NONE,
        bool addict_detec=true); 

/**
 * @brief  reduce_item_by_slot_id 使用扣除物品
 *
 * @param player 玩家结构体
 * @param slot_id 槽
 * @param count 使用的数量
 * @param wait 是否等待db
 * @param reason 同步前端原因, 默认未知
 * @param addict_detec 是否开启防沉迷检查,true 开启；false关闭
 *
 * @return 0 成功 其他失败
 *
 * care 不要使用这个接口去扣金币
 */
int reduce_item_by_slot_id(player_t* player,
        uint32_t slot_id,
        uint32_t count,
        bool wait = false, 
        onlineproto::sync_item_reason_t reason = onlineproto::SYNC_REASON_NONE); 

/**
 * @brief  check_swap_item_by_item_id 检查是否可以添加/删除物品
 *
 * @param player 玩家结构体
 * @param reduce_vec 扣除物品
 * @param add_vec 添加物品
 * @param addict_detec 加物品是否检查防沉迷
 * @param attach_vec 如果物品超出 则尝试放到邮件附件中
 */
uint32_t check_swap_item_by_item_id(player_t* player,
        std::vector<reduce_item_info_t> *reduce_vec,
        std::vector<add_item_info_t> *add_vec,
        bool addict_detec = true,
        std::vector<attachment_t> *attach_vec = 0);

/**
 * @brief 清理过期物品
 * @param noti_client 是否通知客户端
 */
int clean_expired_items(player_t* player, bool noti_client = true);
int clean_expired_item_related_info(
        player_t *player, std::vector<item_t> item_list);

//加一个道具,
int add_single_item(player_t* player, uint32_t item_id, uint32_t count, uint32_t expire_time = 0, bool wait = false,
        bool addict_detect = true, onlineproto::sync_item_reason_t reason = onlineproto::SYNC_REASON_NONE);

//减少一个道具
int reduce_single_item(player_t* player, uint32_t item_id, uint32_t count, bool wait = false,
        bool addict_detect = false, onlineproto::sync_item_reason_t reason = onlineproto::SYNC_REASON_NONE);

//当item生成的时候 初始化他的各项属性 
//如果item是从低阶装备进化来的,保留他的洗练属性
//初始化包括星级,配表增加属性,随机增加属性,固定随机buff,魔法装备还有额外两条随机buff
int init_equip_attr(item_t *item);
//得到装备增加属性计算公式的系数
int get_equip_attr_factor(equip_add_attr_t attr_type);
//计算装备增加的属性
int calc_equip_add_attr(equip_add_attr_t attr_type, uint32_t level, uint32_t star);
//给装备随机一个buff quench, true 洗练 false 非洗练
int rand_add_equip_buff(commonproto::item_optional_attr_t &item_optional_attr, uint32_t buff_group_id, bool quench);
//commonproto::equip_quench_type_t主属性转换为commonproto::equip_add_attr_t 
equip_add_attr_t equip_main_attr_adapter(commonproto::equip_quench_type_t  attr_type);

uint32_t smelter_equip(player_t* player, uint32_t slot_id, uint32_t smelter_item_cnt,
		uint32_t& normal_currency_cnt, uint32_t& high_currency_cnt);

uint32_t smelter_pet_fragment(player_t* player, uint32_t slot_id,
		uint32_t smelter_item_cnt, uint32_t& fragment_gold);

#endif

