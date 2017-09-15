#ifndef __SHOP_H__
#define __SHOP_H__

#include "common.h"
#include "shop_conf.h"

enum product_id_t {
	GRATEFULL_GIFT = 50012, 
	FIN_ESCORT_IMMEDIATELY = 90006,
	GOLD_VIP_180_DAYS = 90028,
	GOLD_VIP_30_DAYS = 90029,
    GOLD_VIP_30_DAYS_0611_DISCOUNT = 90100,
	SILVER_VIP_30_DAYS = 90030,
	OPEN_EXERCISE_POS_3 = 90037,
	OPEN_EXERCISE_POS_4 = 90038,
	OPEN_EXERCISE_POS_5 = 90039,
	MONTH_CARD_30_DAYS = 90035,
	BUY_SWIM_CHAIR = 90010,
	BUY_LUXURY_SUIT = 90008,
	OPEN_HM_BOX_POS_3 = 90032,
	OPEN_HM_BOX_POS_4 = 90025,
	OPEN_HM_BOX_POS_5 = 90026,
	MAYIN_BUCKET_RECHARGE_ENERGY = 90047,
	MAYIN_DEFEAT_EMPIRE_DUP_1 = 90056,
	MAYIN_DEFEAT_EMPIRE_DUP_2 = 90057,
	MAYIN_DEFEAT_EMPIRE_DUP_3 = 90058, 
	ERASE_MINE_COLD_TIME = 90065,
	MINE_BUY_CHALLENGE_TIMES = 90066,
	BUY_SUMMER_WEEKLY_SIGNED = 90071,

};

typedef uint32_t (*after_buy_pd_func) (player_t* player);
typedef uint32_t (*before_buy_pd_func) (player_t* player);

typedef uint32_t (*check_market_before_buy_product)(
        player_t *player, uint32_t market,
        onlineproto::cs_0x012E_buy_product &buy_in_);
typedef uint32_t (*update_market_after_buy_product)(
        player_t *player, uint32_t market,
        onlineproto::cs_0x012E_buy_product &buy_in_);

class ShopRegisterFun
{
public:
	void register_buy_product_proc_func();
	void clear();
	uint32_t call_reg_func_after_buy_product(player_t* player, uint32_t type);
	uint32_t call_reg_func_before_buy_product(player_t* player, uint32_t type);

    static uint32_t check_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

    static uint32_t update_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

#if 0
    std::map<uint32_t, check_market_before_buy_product> check_before_market_buy_map_;
    std::map<uint32_t, update_market_after_buy_product> update_after_market_buy_map_;
#endif

private:
	static uint32_t after_clear_escort_last_tm(player_t* player);
	static uint32_t before_clear_escort_last_tm(player_t* player);

	static uint32_t after_buy_gold_vip_180_days(player_t* player);
	static uint32_t before_buy_gold_vip_180_days(player_t* player);

	static uint32_t after_buy_gold_vip_30_days(player_t* player);
	static uint32_t before_buy_gold_vip_30_days(player_t* player);
	static uint32_t before_buy_gold_vip_30_days_0611(player_t* player);

	static uint32_t after_buy_silver_vip_30_days(player_t* player);
	static uint32_t before_buy_silver_vip_30_days(player_t* player);

	static uint32_t after_buy_month_card_30_days(player_t* player);
	static uint32_t before_buy_month_card_30_days(player_t* player);

	static uint32_t before_buy_swim_chair(player_t* player);
	// static uint32_t after_buy_swim_chair(player_t* player);
	
	static uint32_t after_buy_luxury_suit(player_t* player);

	static uint32_t before_buy_open_hm_box_pos_3(player_t* player);
	static uint32_t before_buy_open_hm_box_pos_4(player_t* player);
	static uint32_t before_buy_open_hm_box_pos_5(player_t* player);

	static uint32_t after_buy_open_hm_box_pos_3(player_t* player);
	static uint32_t after_buy_open_hm_box_pos_4(player_t* player);
	static uint32_t after_buy_open_hm_box_pos_5(player_t* player);

	static uint32_t before_buy_open_exercise_3(player_t* player);
	static uint32_t before_buy_open_exercise_4(player_t* player);
	static uint32_t before_buy_open_exercise_5(player_t* player);

	static uint32_t after_buy_open_exercise_3(player_t* player);
	static uint32_t after_buy_open_exercise_4(player_t* player);
	static uint32_t after_buy_open_exercise_5(player_t* player);

	static uint32_t after_mayin_recharge_energy(player_t* player);
	static uint32_t before_mayin_defeat_empire_dup1(player_t* player);
	static uint32_t after_mayin_defeat_empire_dup1(player_t* player);
	static uint32_t before_mayin_defeat_empire_dup2(player_t* player);
	static uint32_t after_mayin_defeat_empire_dup2(player_t* player);
	static uint32_t before_mayin_defeat_empire_dup3(player_t* player);
	static uint32_t after_mayin_defeat_empire_dup3(player_t* player);
	static uint32_t after_buy_mayin(player_t* player);
	static uint32_t after_buy_erase_mine_cold_time(player_t* player);
	static uint32_t after_mine_buy_challenge_times(player_t* player);

	static uint32_t before_buy_summer_weekly_signed(player_t* player);

	static uint32_t after_buy_gratefull_gift(player_t* player);
	std::map<uint32_t, after_buy_pd_func> after_buy_pd_map_;
	std::map<uint32_t, before_buy_pd_func> before_buy_pd_map_;


#if 0
    static uint32_t check_daily_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

    static uint32_t update_daily_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

    static uint32_t check_elem_dup_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

    static uint32_t update_elem_dup_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

    static uint32_t check_arena_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

    static uint32_t update_arena_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);
	
    static uint32_t check_exped_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

    static uint32_t update_exped_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

    static uint32_t check_night_raid_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

    static uint32_t update_night_raid_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);
    static uint32_t check_family_market_before_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);

    static uint32_t update_family_market_after_buy(
        player_t *player, uint32_t market, 
        onlineproto::cs_0x012E_buy_product &buy_in_);
#endif
};

enum service_type_t
{
	ARENA_CLEAN_CD = 70002,	//购买竞技场清除cd的服务
};

void gen_db_transaction(player_t *player, const product_t *product, 
        uint32_t buy_cnt, int32_t money_chg, 
        dbproto::channel_type_t chn, dbproto::transaction_info_t *inf);

/**
 * @brief  buy_product 购买一件商品
 *
 * @param pd_id
 * @param buy_cnt
 *
 * @return 错误码
 */
uint32_t buy_product(player_t* player, uint32_t pd_id, uint32_t buy_cnt);

struct buy_product_t {
    uint32_t pd_id;
    uint32_t buy_cnt;
};
uint32_t batch_buy_product(player_t* player, std::vector<buy_product_t> buy_vec, string batch_name, bool sync_cli = true);


/**
 * @brief  buy_attr_and_use 
 *
 * @param attr_type 需要购买的属性
 * @param product_id 属性对应的商品id
 *
 * @return 错误码
 */
uint32_t buy_attr_and_use(player_t* player, attr_type_t attr_type, 
        uint32_t product_id, uint32_t count = 1);

int random_select_product_from_market_default(
        uint32_t market,
        std::vector<market_product_t> &result);

int transaction_proc_exchange(player_t *player, uint32_t exchange_id, uint32_t count);

uint32_t  change_vip_end_time(player_t* player, change_vip_time_flag_t flag);

uint32_t set_buy_vip_trans(player_t* player,
		uint32_t last_gold_begin_time,
		uint32_t last_gold_end_time,
		uint32_t last_silver_begin_time,
		uint32_t last_silver_end_time,
		buy_vip_type_t type);

uint32_t pack_vip_info(player_t* player, 
		uint32_t begin_time, uint32_t end_time,
		commonproto::player_vip_type_t vip_type);

uint32_t pack_buy_vip_trans(player_t* player, uint32_t begin_time,
		uint32_t end_time, bool is_gold_180_vip,
		commonproto::player_vip_type_t vip_type);

uint32_t deal_first_buy_time(player_t* player, buy_vip_type_t vip_type);

uint32_t before_exchange_first_check(uint32_t exchange_id);

uint32_t get_caiwu_id(uint32_t product_elem_type, uint32_t id);

/*
void gen_db_diamond_transaction(player_t* player,
		const product_t *product, uint32_t buy_cnt, int32_t money_chg,
		dbproto::channel_type_t chn);
*/

void gen_db_diamond_transaction_new(player_t* player,
		const product_t *product, uint32_t buy_cnt, int32_t money_chg,
		dbproto::channel_type_t chn);

#endif
