#ifndef __DUPLICATE_UTILS_H__
#define __DUPLICATE_UTILS_H__

#include "common.h"
#include "player.h"
#include "duplicate_conf.h"
#include "proto/client/pb0x02.pb.h"
#include "prize.h"

#define DUP_OBJ_TYPE_PLAYER     (1) 
#define DUP_OBJ_TYPE_PET        (2) 
#define DUP_OBJ_TYPE_MON        (3)
#define DUP_OBJ_TYPE_BUILDER    (4)

#define DUP_REVIVAL_TYPE_PLAYER (1)
#define DUP_REVIVAL_TYPE_PET (2)
#define DUP_REVIVAL_TYPE_ALL (3)

enum dup_mode_first_dup_id_t {
	MONSTER_CRISIS_FIRST_DUP_ID = 701,
};

enum dup_relate_com_data {
	MONSTER_CRISIS_RESET_COUNT_LIMIT = commonproto::MONSTER_CRISIS_RESET_COUNT_LIMIT,
	MONSTER_CRISIS_RESET_COUNT_LIMIT_VIP = commonproto::MONSTER_CRISIS_RESET_COUNT_LIMIT_VIP,
};

class DupUtils {
public:
    /**
     * @brief 判定玩家能否进入副本
     * @param player
     * @param dup_id 副本id
     * @return 成功:0 失败:不能进入的原因错误码
     */
    static int can_enter_duplicate(player_t *player, uint32_t dup_id);

    /**
     * @brief 判定能否能够重置该模式的副本
     * @param player
     * @param mode 副本模式
     * @return 成功:0 失败:不能进入的原因错误码
     */
	static uint32_t can_reset_dups(player_t* player, uint32_t mode);

    /** 
     * @brief 扣除通用副本消耗
     * 
     * @param player 
     * @param dup_id  副本id
     * 
     * @return 
     */
    static int reduce_duplicate_cost(player_t *player, uint32_t dup_id);

    static bool can_send_to_battle(player_t *player);

    static int send_to_battle(player_t *player, uint32_t cmd, 
            const google::protobuf::Message &msg, uint32_t wait_svr);
    /**
     * @brief 结束副本时清楚玩家的副本信息
     */
    static void clear_player_dup_info(player_t *player);

    //helpet functions
    //玩家是否在副本中
    static bool is_player_in_duplicate(player_t *player);

    static bool is_duplicate_passed(player_t *player, uint32_t dup_id);

    //获取玩家的副本对战类型
    static duplicate_battle_type_t get_player_duplicate_battle_type(player_t *player);

    static uint32_t duplicate_need_vp(uint32_t dup_id);

    const static std::vector<duplicate_consume_item_t> *duplicate_need_items(uint32_t dup_id);

    static duplicate_mode_t get_duplicate_mode(uint32_t dup_id);
    static string get_duplicate_name(uint32_t dup_id);

    static int proc_duplicate_reward(
            player_t *player, const duplicate_t *dup, 
            onlineproto::sc_0x0210_duplicate_notify_result &noti_result);

	//根据副本mode发奖励
    static int duplicate_reward_dynamic(
            player_t *player, const duplicate_mode_t mode, 
            onlineproto::sc_0x0210_duplicate_notify_result &noti_result);

	static uint32_t reset_dups_record(player_t* player, uint32_t mode);

	static uint32_t deal_with_trail_dup_after_login(player_t* player);

    static void proc_mon_drop_prize(player_t *player, std::vector<uint32_t> &drop_prize_vec,
            uint32_t pos_x, uint32_t pos_y, bool is_player_dead = false);

    static int proc_dup_area_prize(
        player_t *player,
        const duplicate_t *dup, uint32_t old_star, uint32_t new_star);

    static int tell_btl_exit(player_t *player, bool wait_btl);

	static uint32_t set_mayin_train_gift_state(player_t* player);

	static uint32_t use_diamond_buy_pass_dup(player_t* player, uint32_t dup_id);

	static uint32_t clean_dup_pass_record(player_t* player, uint32_t dup_id);

	static uint32_t clean_mayin_defeat_empire_dup(player_t* player);

	//将通关记录需要日清的副本记录清掉
	static uint32_t clean_daily_activity_dup(player_t* player);

	static uint32_t use_diamond_buy_pet_pass_dup(player_t* player,
			uint32_t pet_id, uint32_t dup_id);
};


#endif
