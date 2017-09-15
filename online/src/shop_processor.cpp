#include "player.h"
#include "item.h"
#include "pet.h"
#include "pet_utils.h"
#include "attr_utils.h"
#include "shop.h"
#include "global_data.h"
#include "service.h"
#include "package.h"
#include "shop_processor.h"
#include "utils.h"
#include "arena.h"
#include "exchange_conf.h"
#include "player_utils.h"

int BuyProductCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    uint32_t pd_id = cli_in_.product_id();
    uint32_t buy_cnt = cli_in_.buy_cnt();
    uint32_t market = (uint32_t)(cli_in_.market());
    attr_type_t pd_attr = (attr_type_t)(cli_in_.attr_id());
    attr_type_t cnt_attr = (attr_type_t)(pd_attr+1);
    bool need_set_shop_attr = false;

    uint32_t err = 0;
    const product_t *pd = g_product_mgr.find_product(pd_id);

    //商品不存在
    if (!pd) {
        err = cli_err_product_not_exist;
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_product_not_exist);
    } 

    if (pd->in_markets.size()) {
        if (pd->in_markets.count(market) == 0) {
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_invalid_market_type);
        } 
    }

    // 检查商店出售条件
    if (market) {
        err = ShopRegisterFun::check_market_before_buy(player, market, cli_in_);
        if (err) {
            return send_err_to_player(player, player->cli_wait_cmd, err);
        }
        //商店中的物品统计
        const string stat_name = g_market_mgr.get_market_product_stat_name(market, pd_id);
        if (!stat_name.empty()) {
            Utils::write_msglog_new(player->userid, "分类商店", "商店购买" + Utils::to_string(market), stat_name);
        } else {
            Utils::write_msglog_new(player->userid, "分类商店", "商店购买" + Utils::to_string(market), pd->name);
        }
    }

	if (pd->service) {
        //非商店购买的 服务
		err = buy_attr_and_use(player, (attr_type_t)pd->service, 
				pd_id, buy_cnt);
	} else {
        // 购买普通商品
        err = buy_product(player, pd_id, buy_cnt);
    }
    if (err) {
        return send_err_to_player(player, player->cli_wait_cmd, err); 
    }
    if (need_set_shop_attr) {
        SET_A(cnt_attr, 0);
    }

    // 更新商店记录
    if (market) {
        err = ShopRegisterFun::update_market_after_buy(player, market, cli_in_);
        if (err) {
            return send_err_to_player(player, player->cli_wait_cmd, err);
        }
    }

    //回包
    cli_out_.set_product_id(pd_id);
    cli_out_.set_buy_cnt(buy_cnt);
    
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int SellProductCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    std::map<uint32_t, uint32_t> slot_cnt_map;
    uint32_t get_gold = 0;

    for (int i = 0; i < cli_in_.item_list_size(); i++) {
        const commonproto::item_info_t &inf = cli_in_.item_list(i);
        uint32_t slot_id = inf.slot_id();
        uint32_t count = inf.count();
        const item_t *item = player->package->get_const_item_in_slot(slot_id);
        if (!item) {
            return send_err_to_player(player, player->cli_wait_cmd, 
                    cli_err_slot_id_invalid);
        }
        const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(item->item_id);
        if (!item_conf) {
            WARN_TLOG("P:%u item_id:%u's conf not exist", player->userid, item->item_id);
            continue;
        }
        if (item_conf->sale_price == 0) {
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_item_unsellable);
        }
        uint32_t sellable_cnt = (item->count > item->using_count) 
            ?item->count - item->using_count :0;

        get_gold += item_conf->sale_price *count;
        count += slot_cnt_map[slot_id];
        if (count > sellable_cnt) {
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_sell_item_id_count_invalid);
        }
        slot_cnt_map[slot_id] = count;
    }
    //全部可卖
    FOREACH(slot_cnt_map, it) {
        reduce_item_by_slot_id(player, it->first, it->second);
    }
    AttrUtils::add_player_gold(player, get_gold, false, "出售物品");

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int ShopRefreshCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    static uint32_t daily_shop_product_id = 90000;
    int err = 0;
    attr_type_t shop_fresh_cnt_attr;
    attr_type_t shop_fresh_tm_attr;
    switch (cli_in_.market()) {
    case onlineproto::MARKET_TYPE_DAILY: 
        shop_fresh_cnt_attr = kDailyShopUserRefreshTimes;
        shop_fresh_tm_attr = kAttrSpShopLastRefreshTm;
        break;

    case onlineproto::MARKET_TYPE_FAMILY:
        shop_fresh_cnt_attr = kDailyShopUserFamilyRefreshTimes;
        shop_fresh_tm_attr = kAttrFamilyShopLastRefreshTm;
        break;

    case onlineproto::MARKET_TYPE_ARENA:
        shop_fresh_cnt_attr = kDailyShopUserArenaRefreshTimes;
        shop_fresh_tm_attr = kAttrArenaShopLastRefreshTm;
        break;

    case onlineproto::MARKET_TYPE_ELEM_DUP: 
        shop_fresh_cnt_attr = kDailyShopUserElemDupRefreshTimes;
        shop_fresh_tm_attr = kAttrElemDupShopLastRefreshTm;
        break;

	case onlineproto::MARKET_TYPE_EXPED:
        shop_fresh_cnt_attr = kDailyShopUserExpedRefreshTimes;
        shop_fresh_tm_attr = kAttrExpedShopLastRefreshTm;
        break;

	case onlineproto::MARKET_TYPE_NIGHT_RAID:
        shop_fresh_cnt_attr = kDailyShopUserNightRaidRefreshTimes;
        shop_fresh_tm_attr = kAttrNightRaidShopLastRefreshTm;
        break;
	case onlineproto::MARKET_TYPE_SMELT_MONEY:
		shop_fresh_cnt_attr = kDailyShopUserSmeltMoneyRefreshTimes;
		shop_fresh_tm_attr = kAttrSmeltMoneyLastRefreshTm;
		break;
	case onlineproto::MARKET_TYPE_SMELT_GOLD:
		shop_fresh_cnt_attr = kDailyShopUserSmeltGoldRefreshTimes;
		shop_fresh_tm_attr = kAttrSmeltGoldLastRefreshTm;
		break;
    default:
        return send_err_to_player(player, player->cli_wait_cmd, 
                cli_err_invalid_market_type);
    }
    uint32_t cnt = GET_A(shop_fresh_cnt_attr);
    cnt += 1;
    uint32_t buy_cnt = pow(2, (cnt-1)/2);
    err = buy_attr_and_use(player, kServiceBuyShopRefreshTimes,
            daily_shop_product_id, buy_cnt);
    if (err) {
        return send_err_to_player(player, player->cli_wait_cmd, err);
    }
    try_flush_shop(player, DO_IT_NOW, cli_in_.market());
    SET_A(shop_fresh_tm_attr, NOW());
    SET_A(shop_fresh_cnt_attr, cnt);
    cli_out_.set_market(cli_in_.market());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int ExchangeCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;

    uint32_t exchange_id = cli_in_.exchange_id();
    uint32_t count = cli_in_.count();

	uint32_t ret = before_exchange_first_check(exchange_id);	
	if (ret) {
		RET_ERR(ret);
	}
    
    int err = transaction_proc_exchange(player, exchange_id, count);
    if (err) {
        return send_err_to_player(player, player->cli_wait_cmd, err);
    }

    cli_out_.set_exchange_id(exchange_id);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int ShopGetProductInfoCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    cli_out_.Clear();
    std::map<uint32_t, ol_market_item_t> *shop_items;
    if (cli_in_.market() == onlineproto::MARKET_TYPE_ELEM_DUP) {
        shop_items = player->elem_dup_shop_items;
    } else if (cli_in_.market() == onlineproto::MARKET_TYPE_ARENA) {
        shop_items = player->arena_shop_items;
    } else if (cli_in_.market() == onlineproto::MARKET_TYPE_EXPED) {
        shop_items = player->exped_shop_items;
    } else if (cli_in_.market() == onlineproto::MARKET_TYPE_NIGHT_RAID) {
        shop_items = player->night_raid_shop_items;
    } else if (cli_in_.market() == onlineproto::MARKET_TYPE_FAMILY) {
        shop_items = player->family_shop_items;
    } else if (cli_in_.market() == onlineproto::MARKET_TYPE_DAILY) {
        shop_items = player->daily_shop_items;
    } else if (cli_in_.market() == onlineproto::MARKET_TYPE_SMELT_MONEY) {
		shop_items = player->smelter_money_shop_items;
	} else if (cli_in_.market() == onlineproto::MARKET_TYPE_SMELT_GOLD) {
		shop_items = player->smelter_gold_shop_items;
	}
    FOREACH((*shop_items), iter) {
        commonproto::market_item_t *p_item = cli_out_.mutable_item_info()->add_items();
        p_item->set_item_id(iter->second.item_id);
        p_item->set_count(iter->second.count);
    }
    cli_out_.set_market(cli_in_.market());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
