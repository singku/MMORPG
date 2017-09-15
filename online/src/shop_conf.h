#ifndef __SHOP_CONF_H__
#define __SHOP_CONF_H__

#include "common.h"

#define MAX_ELEMENT_PER_TYPE    (10)

#define DAILY_MARKET_COLUMN (10)
//商店类型
enum market_type_t {
    MARKET_TYPE_NONE  = onlineproto::MARKET_TYPE_NONE, //默认可购买的商品0
    MARKET_TYPE_DAILY = onlineproto::MARKET_TYPE_DAILY, //每日商店1
    MARKET_TYPE_FAMILY = onlineproto::MARKET_TYPE_FAMILY,  //家族商店2
    MARKET_TYPE_ARENA = onlineproto::MARKET_TYPE_ARENA, //竞技场商店3
    MARKET_TYPE_ELEM_DUP = onlineproto::MARKET_TYPE_ELEM_DUP, // 元素挑战商店4
	MARKET_TYPE_EXPED = onlineproto::MARKET_TYPE_EXPED,	//远征（伙伴激战）商店6
	MARKET_TYPE_NIGHT_RAID = onlineproto::MARKET_TYPE_NIGHT_RAID,	//夜袭商店7
	MARKET_TYPE_SMELT_MONEY = onlineproto::MARKET_TYPE_SMELT_MONEY, //普通熔炉商店8
	MARKET_TYPE_SMELT_GOLD = onlineproto::MARKET_TYPE_SMELT_GOLD,	//特殊熔炉商店9
    MARKET_TYPE_END,
};

enum change_vip_time_flag_t {
	CHANGE_SILIVER_VIP = 1,
	CHANGE_GOLD_30_VIP = 2,
	CHANGE_GOLD_180_VIP = 3,
};

enum {
    PRODUCT_TYPE_NORMAL = 0,
    PRODUCT_TYPE_VIP    = 1,

    MIN_PRODUCT_DISCOUNT = 1,
    MAX_PRODUCT_DISCOUNT = 99,

    ELEMENT_TYPE_ITEM   = 1,
    ELEMENT_TYPE_ATTR   = 2,
    ELEMENT_TYPE_PET    = 3,
    ELEMENT_TYPE_BUFF   = 4,

    PRODUCT_PRICE_TYPE_GOLD = 1,
    PRODUCT_PRICE_TYPE_DIAMOND = 2,

    SUB_PRICE_TYPE_ATTR = 1,
    SUB_PRICE_TYPE_ITEM = 2,
};

enum shop_internal_err_t {
    shop_err_product_already_exist = 1,
    shop_err_item_id_not_exist = 2,
    shop_err_discount_invalid = 3,
    shop_err_multi_element = 4,
    shop_err_product_type_err = 5,
};

enum buy_vip_type_t {
	BUY_SILVER_VIP = 1,
	BUY_GOLD_VIP = 2,
	BUY_GOLD_180_VIP = 3,
};

struct product_element_t {
    product_element_t() {
        type = 0;
        id = 0;
        count = 0;
        level = 0;
        duration = 0;
        price = 0;
        range_min = 0;
        range_max = 0;
        method = 0;
    }
    uint32_t type;
    uint32_t id;
    int32_t count;
    uint32_t method;
    uint32_t range_min;
    uint32_t range_max;
    uint32_t level;
    uint32_t duration;
    float price;

    uint32_t target_attr;//购买服务清理CD的目标属性ID
};

struct product_t {
    product_t() {
        id = 0;
		pd_type = 1;
        price = 0;
        price_type = 0;
        is_attr_price_type = 0;
        sub_price_type = 0;
        sub_price_id = 0;
        sub_price_rate = 1;
        sub_price = 0;
        vip_level = 0;
        discount_type = 0;
        discount_rate = 0;
        msg_id = 0; //统计项id
        daily_key = 0;
        weekly_key = 0;
        monthly_key = 0;
        forever_key = 0;
        service = 0;
        item_elem.clear();
        attr_elem.clear();
        pet_elem.clear();
        buff_elem.clear();
        in_markets.clear();
        name.clear();
    }
    uint32_t id;
	uint32_t pd_type;
    uint32_t price;
    uint32_t price_type;
    uint32_t is_attr_price_type;
    uint32_t sub_price_type;
    uint32_t sub_price_id;
    uint32_t sub_price_rate;
    uint32_t sub_price;
    uint32_t vip_level;
    uint32_t discount_type;
    uint32_t discount_rate;
    uint32_t msg_id;
    string name;
    string stat_name;

    //同一类型的物品ID必须唯一
    std::map<uint32_t, product_element_t> item_elem;
    std::map<uint32_t, product_element_t> attr_elem;
    std::map<uint32_t, product_element_t> pet_elem;
    std::map<uint32_t, product_element_t> buff_elem;

    uint32_t daily_key;
    uint32_t weekly_key;
    uint32_t monthly_key;
    uint32_t forever_key;
    uint32_t service;

    //为商店系统所配置的表项
    //商品所处的商店列表
    std::set<uint32_t> in_markets;
public:
    inline void show() const {
        TRACE_TLOG("Product ID:%u, Price:%u, VIP:%u, Discount:%u",
                id, price, vip_level, discount_rate);
        FOREACH(item_elem, it) {
            TRACE_TLOG("\tITEM ID:%u, Count:%d, Level:%u, Duration:%u",
                    it->second.id, it->second.count, it->second.level, it->second.duration);
        }
        FOREACH(attr_elem, it) {
            TRACE_TLOG("\tATTR ID:%u, Count:%d, Level:%u, Duration:%u",
                    it->second.id, it->second.count, it->second.level, it->second.duration);
        }
        FOREACH(pet_elem, it) {
            TRACE_TLOG("\tPETS ID:%u, Count:%d, Level:%u, Duration:%u",
                    it->second.id, it->second.count, it->second.level, it->second.duration);
        }
        FOREACH(buff_elem, it) {
            TRACE_TLOG("\tBUFF ID:%u, Count:%d, Level:%u, Duration:%u",
                    it->second.id, it->second.count, it->second.level, it->second.duration);
        }
    }
    inline bool is_elem_exist(uint32_t id, uint32_t type) {
        switch (type) {
        case ELEMENT_TYPE_ITEM:
            return (item_elem.count(id) > 0 );
        case ELEMENT_TYPE_ATTR:
            return (attr_elem.count(id) > 0 );
        case ELEMENT_TYPE_PET:
            return (pet_elem.count(id) > 0 );
        case ELEMENT_TYPE_BUFF:
            return (buff_elem.count(id) > 0);
        default:
            return true;
        }
        return true;
    }
    inline bool add_elem(const product_element_t elem, uint32_t type) {
        switch (type) {
        case ELEMENT_TYPE_ITEM:
            if (item_elem.size() == MAX_ELEMENT_PER_TYPE) {
                return false;
            }
            item_elem[elem.id] = elem;
            return true;
        case ELEMENT_TYPE_ATTR:
            if (attr_elem.size() == MAX_ELEMENT_PER_TYPE) {
                return false;
            }
            attr_elem[elem.id] = elem;
            return true;
        case ELEMENT_TYPE_PET:
            if (pet_elem.size() == MAX_ELEMENT_PER_TYPE) {
                return false;
            }
            pet_elem[elem.id] = elem;
            return true;
        case ELEMENT_TYPE_BUFF:
            if (buff_elem.size() == MAX_ELEMENT_PER_TYPE) {
                return false;
            }
            buff_elem[elem.id] = elem;
            return true;
        default:
            return false;
        }
        return false;
    }
};

class product_manager_t {
public:
    product_manager_t() {
        init();
    }
    ~product_manager_t() {
        init();
    }

public:
    inline void init() {
        products_.clear();
    }
    inline bool is_product_exist(uint32_t product_id) {
        return (products_.count(product_id) > 0);
    }
    inline int add_product(const product_t &product) {
        if (is_product_exist(product.id)) {
            return shop_err_product_already_exist;
        }
        products_[product.id] = product;
        return 0;
    }
    inline const product_t *find_product(uint32_t product_id) {
        if (is_product_exist(product_id) == false) return 0;
        return &((products_.find(product_id))->second);
    }
    inline product_t *find_mutable_product(uint32_t product_id) {
        if (is_product_exist(product_id) == false) return 0;
        return &((products_.find(product_id))->second);
    }

    inline const std::map<uint32_t, product_t> &const_products_map() {
        return products_;
    }
    //copy之前 需要自己检查in的有效性和正确性
    inline void copy_from(product_manager_t &in) {
        products_.clear();
        products_ = in.const_products_map();
    }

private:
    std::map<uint32_t, product_t> products_;
};

struct market_product_t {
    market_product_t() {
        shop_id = 0;
        market_type = 0;
        product_type = 0;
        weight = 0;
        stat_name.clear();
    }
    uint32_t shop_id; //商品id
    uint32_t market_type; //所处商店类型
    uint32_t product_type; //在刷新商店中的商品类型 0\1\2\3\4
    uint32_t weight;    //刷新商店中的权重
    string stat_name; //统计名称
};

struct ol_market_item_t {
    uint32_t item_id;
    uint32_t count;
};


//<product_type, set::product_s>
typedef std::map<uint32_t, std::vector<market_product_t> > market_t;
typedef market_t::iterator market_iter_t;
class market_manager_t {
public:

    market_manager_t() {
        clear();
    }
    ~market_manager_t() {
        clear();
    }
    void clear() {
        markets_.clear();
    }
public:
    inline bool market_exist(uint32_t market_type) {
        return (markets_.count(market_type) > 0);
    }
    inline bool is_valid_market_type(uint32_t market_type) {
        if (market_type >= MARKET_TYPE_NONE &&
                market_type < MARKET_TYPE_END) {
            return true;
        }
        return false;
    }

    const inline market_t *get_market(uint32_t market_type) {
        if (!market_exist(market_type)) {
            return 0;
        }
        return &(markets_.find(market_type)->second);
    }
    const inline string get_market_product_stat_name(uint32_t market_type, uint32_t pd_id) {
        const market_t *market = get_market(market_type);
        if (!market) {
            return "";
        }
        if (market->count(pd_id) == 0) {
            return "";
        }
        const std::vector<market_product_t> m_vec = market->find(pd_id)->second;
        if (m_vec.empty()) {
            return "";
        }
        return m_vec.begin()->stat_name;
    }

    inline bool add_product(uint32_t market_type, market_product_t &product) {
        if (!is_valid_market_type(market_type)) {
            return false;
        }
        if (!market_exist(market_type)) {
            market_t market;
            std::vector<market_product_t> type_products_vec;
            type_products_vec.push_back(product);
            market[product.product_type] = type_products_vec;
            markets_[market_type] = market;
        } else {
            market_t *market = &(markets_.find(market_type)->second);
            market_iter_t it;
            it = market->find(product.product_type);
            if (it == market->end()) {
                std::vector<market_product_t> product_vec;
                product_vec.push_back(product);
                market->insert(make_pair(product.product_type, product_vec));
            } else {
                std::vector<market_product_t> &product_vec = it->second;
                product_vec.push_back(product);
            }
        }
        return true;
    }
    const inline std::map<uint32_t, market_t> &const_market_map() {
        return markets_;
    }
    inline void copy_from(market_manager_t &mmgr) {
        markets_ = mmgr.const_market_map();
    }
private:
    //<market_type, market>
    std::map<uint32_t, market_t> markets_;
};

#endif
