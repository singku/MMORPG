#ifndef __PRIZE_H__
#define __PRIZE_H__

#include "common.h"
#include "player.h"
#include "prize_conf.h"
#include "time_utils.h"

//不同天赋值对应转化的碎片数量
enum pet_tlevel_trans_item_cnt_t {
	kFiveTLevelItemCnt   =   commonproto::FiveTLevelItemCnt,
	kFourTLevelItemCnt   =   commonproto::FourTLevelItemCnt,
	kThreeTLevelItemCnt  =   commonproto::ThreeTLevelItemCnt,
	kTwoTLevelItemCnt    =   commonproto::TwoTLevelItemCnt,
	kOneTLevelItemCnt    =   commonproto::OneTLevelItemCnt,
};

#define COUNT_TRANS(count, multi, calc_type) \
    (multi * get_count_by_type(player, count, calc_type))

static inline int32_t get_count_by_type(player_t *player, 
        int32_t count, prize_incr_type_t type = prize_incr_type_default)
{
    switch (type) {
    case prize_incr_type_default: 
        return count;
    case prize_incr_type_muti_level:
        return GET_A(kAttrLv) * count;
    default:
        return count;
    }
}

static inline bool is_prize_exceed_limit(player_t* player, const prize_elem_t &prize_elem, 
        prize_incr_type_t type = prize_incr_type_default) 
{
    uint32_t multi = TimeUtils::get_current_time_prize_multi(prize_elem.adjust_type);

    if (prize_elem.daily_limit_key && prize_elem.daily_limit_max) {
        uint32_t daily_value = AttrUtils::get_attr_value(
                player, (attr_type_t)prize_elem.daily_limit_key); 
        if (daily_value + multi * get_count_by_type(player, prize_elem.count, type) > prize_elem.daily_limit_max) {
            return true; 
        }
    }

    if (prize_elem.weekly_limit_key && prize_elem.weekly_limit_max) {
        uint32_t weekly_value = AttrUtils::get_attr_value(
                player, (attr_type_t)prize_elem.weekly_limit_key); 
        if (weekly_value + multi * get_count_by_type(player, prize_elem.count, type) > prize_elem.weekly_limit_max) {
            return true; 
        }
    }

    if (prize_elem.monthly_limit_key && prize_elem.weekly_limit_max) {
        uint32_t monthly_value = AttrUtils::get_attr_value(
                player, (attr_type_t)prize_elem.monthly_limit_key);
        if (monthly_value + multi * get_count_by_type(player, prize_elem.count, type) > prize_elem.monthly_limit_max) {
            return true; 
        }
    }

    if (prize_elem.forever_limit_key && prize_elem.forever_limit_max) {
        uint32_t forever_value = AttrUtils::get_attr_value(
                player, (attr_type_t)prize_elem.forever_limit_key); 
        if (forever_value + multi * get_count_by_type(player, prize_elem.count, type) > prize_elem.forever_limit_max) {
            return true; 
        }
    }

    return 0;
}

static inline void add_prize_limit(player_t* player, const prize_elem_t& prize_elem, 
        prize_incr_type_t type= prize_incr_type_default)
{
    uint32_t multi = TimeUtils::get_current_time_prize_multi(prize_elem.adjust_type);

    if (prize_elem.daily_limit_key && prize_elem.daily_limit_max) {
        AttrUtils::add_attr_value(player, 
                (attr_type_t)prize_elem.daily_limit_key, multi * get_count_by_type(player, prize_elem.count, type), false); 
    }

    if (prize_elem.weekly_limit_key && prize_elem.weekly_limit_max) {
        AttrUtils::add_attr_value(player, 
                (attr_type_t)prize_elem.weekly_limit_key, multi * get_count_by_type(player, prize_elem.count, type), false); 
    }
    if (prize_elem.monthly_limit_key && prize_elem.monthly_limit_max) {
        AttrUtils::add_attr_value(player, 
                (attr_type_t)prize_elem.monthly_limit_key, multi * get_count_by_type(player, prize_elem.count, type), false); 
    }

    if (prize_elem.forever_limit_key && prize_elem.forever_limit_max) {
        AttrUtils::add_attr_value(player, 
                (attr_type_t)prize_elem.forever_limit_key, multi * get_count_by_type(player, prize_elem.count, type), false); 
    }
}

//将奖励id随机出来的物品打包到result中,包含防沉迷 排除item_id物品(全服限制输出)
int transaction_pack_prize_except_item(player_t *player,  
        uint32_t prize_id,
        std::vector<cache_prize_elem_t> &award_elems,
        std::vector<cache_prize_elem_t> *display_elems = 0,
        bool addict_detec = true,
        uint32_t except_id = 0);

//发放打包好的物品
int transaction_proc_packed_prize(player_t *player, 
		const std::vector<cache_prize_elem_t> &award_elems,
        onlineproto::sc_0x0112_notify_get_prize *msg = 0,
        commonproto::prize_reason_t reason = commonproto::PRIZE_REASON_NO_REASON,
        string stat_name = "未知奖励");

/**
 * @brief 通用奖励处理接口
 * @paras player
 * @paras prize_id 奖励的ID
 * @paras reason 奖励的原因
 * @paras need_display 是否需要展示的未获的奖励(翻牌子之类的奖励)
 * @paras msg 返回给客户端的信息体
 * @ret 0 成功 各种物品都会加好
 *      !0 返回错误码
 */
int transaction_proc_prize(player_t *player, 
        uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &msg,
        commonproto::prize_reason_t reason = commonproto::PRIZE_REASON_NO_REASON,
        onlineproto::sync_item_reason_t item_reason = onlineproto::SYNC_REASON_PRIZE_ITEM, 
        bool addict_detec = true);

//将奖励id随机出来的物品打包到result中(简化接口)
int transaction_pack_prize(player_t *player, uint32_t prize_id,
        std::vector<cache_prize_elem_t> &result, bool addict_detec = true);

void cache_prize_to_proto_prize(std::vector<cache_prize_elem_t> &cache_vec,
        google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> &pb_list,
        bool only_get_show = false);

//num是放大系数，num=2 merge后数量乘以2
void merge_cache_prize(std::vector<cache_prize_elem_t> &cache_vec, uint32_t num = 1);
void half_cache_prize(std::vector<cache_prize_elem_t> &cache_vec);

void refresh_player_charge_diamond_draw_prize_info(player_t *player, 
        std::vector<cache_prize_elem_t> &result);

bool check_is_need_update_power(uint32_t elem_id);

//精灵转化碎片
int  pet_transform_chips(const cache_prize_elem_t &pet_elem);
#endif
