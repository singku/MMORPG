
#include <stdlib.h>
#include <libtaomee++/utils/strings.hpp>
#include <boost/lexical_cast.hpp>

#include <vector>

#include "player.h"
#include "xmlutils.h"
#include "time_utils.h"
#include "global_data.h"
#include "xml_configs.h"
#include "attr.h"
#include "attr_utils.h"
#include "utils.h"
#include "sys_ctrl.h"
#include "pet_utils.h"
#include "map_conf.h"
#include "item_conf.h"
#include "pet_conf.h"
#include "skill_conf.h"
#include "prize_conf.h"
#include "rune.h"
#include "task_info.h"
#include "duplicate_conf.h"
#include "player_conf.h"
#include "shop_conf.h"
#include "builder_conf.h"
#include "player_utils.h"

#include "task_utils.h"
#include "tran_card_conf.h"
#include "tran_card.h"
#include "exchange_conf.h"
#include "arena_conf.h"
#include "trans_prof.h"
#include "home_gift_conf.h"
#include "achieve_conf.h"
#include "buff_conf.h"
#include "achieve.h"
#include "family_conf.h"
#include "global_attr.h"
#include "exped_conf.h"
#include "cultivate_equip_conf.h"
#include "question_conf.h"
#include "title_conf.h"
#include "bless_pet_conf.h"
#include "pet_pass_dup_conf.h"
#include "suit_conf.h"

#define ERR_RT \
    do { \
        if (g_all_conf_loaded == 0) {\
            return -1; \
        } else { \
            return 0; \
        } \
    } while (0)

// 加载属性值配置
int load_attr_config(xmlNodePtr root)
{
    typeof(g_attr_configs) tmp_attr_configs;
    typeof(g_user_action_log_config) tmp_action_log_config;

    //g_attr_configs.clear();
    //g_user_action_log_config.clear();
 
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("attr"))) {
            cur = cur->next;
            continue;
        }

        attr_config_t attr_config;
        DECODE_XML_PROP_UINT32_DEFAULT(attr_config.id, cur, "id", 0); 
        DECODE_XML_PROP_UINT32_DEFAULT(attr_config.max, cur, "max", kAttrMaxNoLimit);
        DECODE_XML_PROP_UINT32_DEFAULT(attr_config.vip_max, cur, "vip_max", kAttrMaxNoLimit);
        DECODE_XML_PROP_UINT32_DEFAULT(attr_config.svip_max, cur, "svip_max", kAttrMaxNoLimit);
        DECODE_XML_PROP_UINT32_DEFAULT(attr_config.initial, cur, "initial", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(attr_config.vip_initial, cur, "vip_initial", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(attr_config.svip_initial, cur, "svip_initial", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(attr_config.daily_limit_key, cur, "daily_limit_key", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(attr_config.daily_limit_max, cur, "daily_limit_max", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(attr_config.daily_restrict, cur, "daily_restrict", 0);

        if (attr_config.max == kAttrMaxNoLimit) {
            attr_config.vip_max = kAttrMaxNoLimit;
            attr_config.svip_max = kAttrMaxNoLimit;
        } else {
            if (attr_config.vip_max == kAttrMaxNoLimit) {
                attr_config.vip_max = attr_config.max;
            }
            if (attr_config.svip_max == kAttrMaxNoLimit) {
                attr_config.svip_max = attr_config.max;
            }
        }
        if (attr_config.initial == 0) {
            attr_config.vip_initial = 0;
            attr_config.svip_initial = 0;
        } else {
            if (attr_config.vip_initial == 0) {
                attr_config.vip_initial = attr_config.initial;
            }
            if (attr_config.svip_initial == 0) {
                attr_config.svip_initial = attr_config.initial;
            }
        }

        uint32_t is_log = 0;
        DECODE_XML_PROP_UINT32_DEFAULT(is_log, cur, "log", 0);
        if (is_log || (attr_config.id >= kAttrServiceStartID && attr_config.id <= kAttrServiceEndID) ) {
            tmp_action_log_config.insert(attr_config.id); 
        }

        if (tmp_attr_configs.count(attr_config.id) > 0) {
            ERROR_TLOG("Duplicate attr_id:%u", attr_config.id);
            ERR_RT;
        }
        tmp_attr_configs[attr_config.id] = attr_config;

        cur = cur->next;
    }
    g_attr_configs = tmp_attr_configs;
    g_user_action_log_config = tmp_action_log_config;
    AttrUtils::register_stat_func();
    return 0;
}

int load_shop_config(xmlNodePtr root)
{
    product_manager_t tmp;
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("product"))) {
            cur = cur->next;
            continue;
        }

        product_t pd;
        DECODE_XML_PROP_UINT32(pd.id, cur, "id");
        if (tmp.is_product_exist(pd.id)) {
            ERROR_TLOG("Duplicate Product in shop.xml ID=%u", pd.id);
            ERR_RT;
        }

        char name_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(name_str, cur, "name", "");
        pd.name.assign(name_str);

        DECODE_XML_PROP_STR_DEFAULT(name_str, cur, "stat_name", "");
        pd.stat_name.assign(name_str);

        DECODE_XML_PROP_UINT32(pd.price, cur, "price");
        DECODE_XML_PROP_UINT32_DEFAULT(pd.price_type, cur, "price_type", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pd.is_attr_price_type, cur, "is_attr_price_type", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(pd.sub_price_type, cur, "sub_price_type", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pd.sub_price_id, cur, "sub_price_id", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pd.sub_price_rate, cur, "sub_price_rate", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pd.sub_price, cur, "sub_price", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pd.pd_type, cur, "product_type", 1);

		if (!(pd.pd_type >= 1 && pd.pd_type <= 6)) {
			ERROR_TLOG("shop product id:%u's product type:%u invalid",
					pd.id, pd.pd_type);
            ERR_RT;
		}
    

        if (pd.sub_price_rate == 0) {
            ERROR_TLOG("shop product id:%u's substitute price rate:%u invalid",
                    pd.id, pd.sub_price_rate);
            ERR_RT;
        }

        //不填抵用价格表示可全部抵用
        if (pd.sub_price_type && pd.sub_price == 0) {
            pd.sub_price = (pd.price + pd.sub_price_rate - 1) / pd.sub_price_rate;
        }

        if (pd.sub_price_type == 1) {//属性抵用
            if (g_attr_configs.count(pd.sub_price_id) == 0) {
                ERROR_TLOG("shop product id:%u's substitute price id:%u not exist",
                        pd.id, pd.sub_price_id);
                ERR_RT;
            }
        } else if (pd.sub_price_type == 2) {//道具抵用
            if (!g_item_conf_mgr.is_item_conf_exist(pd.sub_price_id)) {
                ERROR_TLOG("shop product id:%u's substitute price id:%u not exist",
                        pd.id, pd.sub_price_id);
                ERR_RT;
            }
        }

        DECODE_XML_PROP_UINT32_DEFAULT(pd.vip_level, cur, "vip_level", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(pd.discount_type, cur, "discount_type", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pd.discount_rate, cur, "discount_rate", 0);

        decode_xml_prop_uint32_default(&(pd.daily_key), cur, "dailykey", 0);
        decode_xml_prop_uint32_default(&(pd.weekly_key), cur, "weeklykey", 0);
        decode_xml_prop_uint32_default(&(pd.monthly_key), cur, "monthlykey", 0);
        decode_xml_prop_uint32_default(&(pd.forever_key), cur, "foreverkey", 0);
        decode_xml_prop_uint32_default(&(pd.service), cur, "service", 0);

        if (pd.price_type != PRODUCT_PRICE_TYPE_GOLD 
            && pd.price_type != PRODUCT_PRICE_TYPE_DIAMOND) {
            if (pd.is_attr_price_type) {
                if (g_attr_configs.count(pd.price_type) == 0) {
                    ERROR_TLOG("Shop product:%u price_type:%u attr not found",
                            pd.id, pd.price_type);
                    ERR_RT;
                }
            } else {
                if (g_item_conf_mgr.is_item_conf_exist(pd.price_type)) {
                    ERROR_TLOG("Shop product:%u price_type:%u item not found",
                            pd.id,pd.price_type);
                    ERR_RT;
                }
            }
        }

        if (pd.discount_type > 2) {
            ERROR_TLOG("Invalid Product discount_type:%u in shop.xml product_id=%u",
                    pd.discount_type, pd.id);
            ERR_RT;
        }

        //有折扣
        if (pd.discount_type == 1 || pd.discount_type == 2) {
            if (pd.discount_rate == 0) {//但是折扣率没配
                pd.discount_rate = 99;//默认99折
            }
        }

        if (pd.discount_rate != 0 && pd.discount_rate > MAX_PRODUCT_DISCOUNT) {
            ERROR_TLOG("Invalid Product discount in shop.xml ID=%u Discount=%u[1-99]", 
                    pd.discount_rate);
            ERR_RT;
        }

        xmlNodePtr child = cur->xmlChildrenNode;
        while (child) { //每个商品元素
            if (!xmlStrEqual(child->name, (const xmlChar *)("element"))) {
                child = child->next;
                continue;
            }
            product_element_t elem;
            DECODE_XML_PROP_UINT32(elem.type, child, "type");
            if (elem.type != ELEMENT_TYPE_ITEM
                && elem.type != ELEMENT_TYPE_ATTR
                && elem.type != ELEMENT_TYPE_PET
                && elem.type != ELEMENT_TYPE_BUFF) {
                ERROR_TLOG("Invalid Product Elem[id:%u] Type:%u[1-4]", 
                        elem.id, elem.type);
                ERR_RT;
            }
            DECODE_XML_PROP_UINT32(elem.id, child, "id");
            if (pd.is_elem_exist(elem.id, elem.type)) {
                ERROR_TLOG("Duplicate Element Product TYPE:%u, ID:%u",
                        elem.type, elem.id);
                ERR_RT;
            }
            DECODE_XML_PROP_INT(elem.count, child, "count");
            decode_xml_prop_uint32_default(&(elem.level), child, "level", 1);
            decode_xml_prop_uint32_default(&(elem.method), child, "method", 0);
            decode_xml_prop_uint32_default(&(elem.duration), child, "duration", 0);
            decode_xml_prop_uint32_default(&(elem.target_attr), child, "target_attr", 0);
            decode_xml_prop_float_default(&(elem.price), child, "price", 0);
            DECODE_XML_PROP_UINT32_DEFAULT(elem.range_min, child, "range_min", 0);
            DECODE_XML_PROP_UINT32_DEFAULT(elem.range_max, child, "range_max", 0);

            if (elem.type == ELEMENT_TYPE_ITEM) {
                const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(elem.id);
                if (!item_conf) {
                    ERROR_TLOG("Product elem Item not eixst[id=%u]", elem.id);
                    ERR_RT;
                }
                if (item_conf->own_max < (uint32_t)elem.count) {
                    ERROR_TLOG("Product elem Item Count exceed[cnt=%u max_own=%u]",
                            elem.count, item_conf->own_max);
                    ERR_RT;
                }
                if (elem.count <= 0) {
                    ERROR_TLOG("Priduct elem Item[%d] Count invalid[%d]",
                            elem.id, elem.count);
                    ERR_RT;
                }
            } else if (elem.type == ELEMENT_TYPE_ATTR) {
                if (g_attr_configs.count(elem.id) == 0) {
                    ERROR_TLOG("Product elem Attr not exist[id=%u]", elem.id);
                    ERR_RT;
                }
                uint32_t max = AttrUtils::get_attr_max_limit(0, (attr_type_t)elem.id);
                if (max && elem.count > 0 && max < (uint32_t)elem.count) {
                    ERROR_TLOG("Product elem Attr Count exceed[cnt=%u, max_own=%u]",
                            elem.count, max);
                    ERR_RT;
                }
            } else if (elem.type == ELEMENT_TYPE_PET) {
                if (!g_pet_conf_mgr.pet_conf_exist(elem.id)) {
                    ERROR_TLOG("Product elem Pet not exist[id=%u]", elem.id);
                    ERR_RT;
                }
                if (elem.count != 1) {
                    ERROR_TLOG("Product elem Pet must have only one[cnt=%u]", elem.count);
                    ERR_RT;
                }
                if (elem.level > kMaxPetLevel) {
                    ERROR_TLOG("Product elem Pet level exceed lv=%u[max=%u]",
                            elem.level, kMaxPetLevel);
                    ERR_RT;
                }
                if (elem.count <= 0) {
                    ERROR_TLOG("Product elem Pet[%u] count invalid[%u]",
                            elem.id, elem.count);
                    ERR_RT;
                }
            } else if (elem.type == ELEMENT_TYPE_BUFF) {
                if (!g_buff_conf_mgr.is_buff_conf_exist(elem.id)) {
                    ERROR_TLOG("Product elem Buff not exist[id=%u]", elem.id);
                    ERR_RT;
                }
                if (elem.count != 1) {
                    ERROR_TLOG("Product elem buff count != 1[cnt=%u]", elem.count);
                    ERR_RT;
                }
            }

            //加入
            bool ret = pd.add_elem(elem, elem.type);
            if (ret == false) {
                ERROR_TLOG("Add Product elem faild type:%u check size[max_per_type:%u]", 
                        elem.type, MAX_ELEMENT_PER_TYPE);
                ERR_RT;
            }
            child = child->next;
        }
        tmp.add_product(pd);
        cur = cur->next;
    }
    g_product_mgr.copy_from(tmp);
    FOREACH(g_product_mgr.const_products_map(), it) {
        it->second.show();
    }

    return 0;
}

int load_market_config(xmlNodePtr root)
{
    market_manager_t tmp;
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar*)("shop"))) {
            cur = cur->next;
            continue;
        }
        uint32_t market;
        DECODE_XML_PROP_UINT32(market, cur, "market");
        if (!g_market_mgr.is_valid_market_type(market)) {
            ERROR_TLOG("Market type:%u in market.xml invalid", market);
            ERR_RT;
        }

        uint32_t product_id;
        DECODE_XML_PROP_UINT32(product_id, cur, "id");
        if (!g_product_mgr.is_product_exist(product_id)) {
            ERROR_TLOG("Product id:%u of market.xml don't exsit in shop.xml", product_id);
            ERR_RT;
        }
        product_t *product = g_product_mgr.find_mutable_product(product_id);
        product->in_markets.insert(market);

        uint32_t type;
        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "type", 0);
        uint32_t weight;
        DECODE_XML_PROP_UINT32_DEFAULT(weight, cur, "weight", 0);
        if (market == (uint32_t)MARKET_TYPE_DAILY) {
            if (type == 0 || type > 4) {
                ERROR_TLOG("Product %u of market[%u], invalid type:%u[1-4]",
                        product_id, market, type);
                ERR_RT;
            }
            if (weight == 0) {
                ERROR_TLOG("Product %u of market[%u], invalid weight:%u[must > 0]",
                        product_id, market, weight);
                ERR_RT;
            }
        }
        market_product_t pd;
        pd.shop_id = product_id;
        pd.market_type = market;
        pd.product_type = type;
        pd.weight = weight;

        char name_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(name_str, cur, "stat_name", "");
        pd.stat_name.assign(name_str);


        tmp.add_product(market, pd);

        cur = cur->next;        
    }

    g_market_mgr.copy_from(tmp);
    return 0;
}

int load_sys_ctrl_config(xmlNodePtr root)
{
    module_manager_t tmp;
    tmp.clear();
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar*)("Module"))) {
            cur = cur->next;
            continue;
        }
        module_conf_t mc;
        DECODE_XML_PROP_UINT32(mc.module_id, cur, "ID");
        if (tmp.is_module_exist(mc.module_id)) {
            ERROR_TLOG("Duplicate Module Ctrl in sys_ctrl.xml ID=%u", mc.module_id);
            ERR_RT;
        }
        xmlNodePtr child = cur->xmlChildrenNode;
        mc.conf_map.clear();
        while (child) {
            if (!xmlStrEqual(child->name, (const xmlChar *)("Key"))) {
                child = child->next;
                continue;
            }
            char key[256] = {0};
            char val[256] = {0};
            DECODE_XML_PROP_STR(key, child, "Name");
            DECODE_XML_PROP_STR(val, child, "Val");
            string key_s;
            string val_s;
            key_s.assign(key);
            val_s.assign(val);
            mc.conf_map[key_s] = val_s;
            child = child->next;
        }//while
        tmp.add_module_conf(mc);
        cur = cur->next;
    }//while
    
    g_module_mgr.copy_from(tmp);


    FOREACH(g_module_mgr.const_module_map(), it) {
        TRACE_TLOG("Module ID:%d", it->first);
        FOREACH(it->second.conf_map, it2) {
            TRACE_TLOG("\t key:[%s] val:[%s]", it2->first.c_str(), it2->second.c_str());
        }
    }
    return 0;
}


// 加载地图配置信息
int load_map_config(xmlNodePtr root)
{
    map_conf_mgr_t tmp;
    tmp.clear();
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)"map")) {
            cur = cur->next;
            continue;
        }

        map_conf_t map;
        DECODE_XML_PROP_UINT32(map.id, cur, "id");
        if (tmp.is_map_conf_exist(map.id)) {
            ERROR_TLOG("map %u already exists", map.id);
            ERR_RT;
        }
        DECODE_XML_PROP_UINT32_DEFAULT(map.is_dup_scene, cur, "is_dup_scene", 0);

        std::string init_pos_str;
        decode_xml_prop_default<std::string, std::string>(init_pos_str, cur, "init_pos", "");

        std::vector<std::string> init_pos_pair_list = split(init_pos_str, ';');

        FOREACH(init_pos_pair_list,it) {
            std::vector<std::string> init_pos_list = split(*it, ',');
            if(init_pos_list.size() != 2) {
                ERROR_TLOG("init_pos config faield");
                ERR_RT;
            }
            point_t point(atoi(init_pos_list[0].c_str()), atoi(init_pos_list[1].c_str()));
            map.init_pos.push_back(point);	      
        }
        if (map.init_pos.size() == 0){
            ERROR_TLOG("map need init_pos, mapid:%u", map.id);
            ERR_RT;
        }

        DECODE_XML_PROP_STR_DEFAULT(map.name, cur, "name", "");
        DECODE_XML_PROP_UINT32(map.big_map_id, cur, "big_map_id");
        decode_xml_prop_uint32_default(&(map.is_private), cur, "is_private", 0);

        tmp.add_map_conf(map);

        cur = cur->next;
    }

    g_map_conf_mgr.copy_from(tmp);
    
    return 0;
}

// 加载物品配置信息
int load_item_config(xmlNodePtr root)
{
    item_conf_mgr_t tmp;
    tmp.clear();
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("item"))) {
            cur = cur->next;
            continue;
        }
        item_conf_t item;
        DECODE_XML_PROP_UINT32(item.item_id, cur, "item_id");
        if (tmp.is_item_conf_exist(item.item_id)) {
            ERROR_TLOG("item %u already exists", item.item_id);
            ERR_RT;
        }
        char name_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(name_str, cur, "name", "");
        item.name.assign(name_str);

        uint32_t type;
        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "add_attr_type", 0);
        if (type && !item_conf_mgr_t::is_valid_equip_add_attr(type)) {
            ERROR_TLOG("item %u add_attr_type:%u invalid", item.item_id, type);
            ERR_RT;
        }
        item.add_attr_type = (equip_add_attr_t)type;

        DECODE_XML_PROP_UINT32_DEFAULT(item.factor, cur, "factor", 0);

        std::string tmp_str;
        tmp_str.clear();
        decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "chisel_attrs", "");
        std::vector<std::string> attrs_list = split(tmp_str, ',');
        FOREACH(attrs_list, it) {
            uint32_t attr_id = atoi((*it).c_str());
            if (!item_conf_mgr_t::is_valid_equip_add_attr(attr_id)) {
                ERROR_TLOG("item %u chisel_trans_attr:%u invalid", item.item_id, attr_id);
                ERR_RT;
            }
            item.chisel_attrs.push_back((equip_add_attr_t)attr_id);
        }

        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "base_quality", 0);
        if (type && !item_conf_mgr_t::is_valid_equip_quality(type)) {
            ERROR_TLOG("item %u base_quality:%u invalid", item.item_id, type);
            ERR_RT;
        }
        item.base_quality = (equip_quality_t)type;

        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "quality_max", 0);
        if (type && !item_conf_mgr_t::is_valid_equip_quality(type)) {
            ERROR_TLOG("item %u quality_max:%u invalid", item.item_id, type);
            ERR_RT;
        }
        if (type == 0) {
            item.quality_max = item.base_quality;
        } else {
            item.quality_max = (equip_quality_t)type;
        }

        DECODE_XML_PROP_UINT32_DEFAULT(item.next_quality_item, cur, "next_quality_item", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(item.level_limit, cur, "level_limit", 0);

        //元素属性
        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "elem_type", 0);
        if (type && !item_conf_mgr_t::is_valid_elem_type(type)) {
            ERROR_TLOG("item %u elem_type:%u invalid", item.item_id, type);
            ERR_RT;
        }
        item.elem_type = (equip_elem_type_t)type;

        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "equip_body_pos", 0);
        if (type && !item_conf_mgr_t::is_valid_equip_body_pos(type)) {
            ERROR_TLOG("item %u equip_body_pos:%u invalid", item.item_id, type);
            ERR_RT;
        }
        item.equip_body_pos = (equip_body_pos_t)type;
        
        /*
        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "fashion_body_pos", 0);
        if (type && !item_conf_mgr_t::is_valid_fashion_body_pos(type)) {
            ERROR_TLOG("item %u fashion_body_pos:%u invalid", item.item_id, type);
            ERR_RT;
        }
        item.fashion_body_pos = (fashion_body_pos_t)type;

        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "fashion_add_attr_type", 0);
        if (type && !item_conf_mgr_t::is_valid_fashion_add_attr(type)) {
            ERROR_TLOG("item %u fashion_add_attr_type:%u invalid", item.item_id, type);
            ERR_RT;
        }
        item.fashion_add_attr_type = (fashion_add_attr_t)type;

        DECODE_XML_PROP_UINT32_DEFAULT(item.fashion_add_attr_value, cur, "fashion_add_attr_value", 0);
        */

        DECODE_XML_PROP_UINT32_DEFAULT(item.expire, cur, "expire", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.own_max, cur, "own_max", 999);
        DECODE_XML_PROP_UINT32_DEFAULT(item.attr_id, cur, "attr_id", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "role_type", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.auto_use, cur, "auto_use", 0);

        if (type && !player_conf_mgr_t::is_valid_prof(type)) {
            ERROR_TLOG("item %u role_type:%u invalid", item.item_id, type);
            ERR_RT;
        }
        item.role_type = (prof_type_t)type;

        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "fun_type", 0);
        if (type && !item_conf_mgr_t::is_valid_item_func_type(type)) {
            ERROR_TLOG("item %u func_type:%u invalid", item.item_id, type);
            ERR_RT;
        }
        item.fun_type = (item_func_type_t)type;

        tmp_str.clear();
        decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "fun_arg", "");
        std::vector<std::string> args_list = split(tmp_str, ',');
        FOREACH(args_list, it) {
            uint32_t arg = atoi((*it).c_str());
            item.fun_args.push_back(arg);
        }

        tmp_str.clear();
        decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "material", "");
        std::vector<std::string> material_list = split(tmp_str, ';');
        FOREACH(material_list, it) {
            std::vector<std::string> items = split(*it, ',');
            if (items.size() != 2) {
                ERROR_TLOG("item :%u upgrade material invalid :[%s]", item.item_id, tmp_str.c_str());
                ERR_RT;
            }
            uint32_t item_id = atoi((items[0]).c_str());
            uint32_t cnt = atoi((items[1]).c_str());
            item.material[item_id] = cnt;
        }

        DECODE_XML_PROP_UINT32_DEFAULT(item.sale_price, cur, "sale_price", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.slot_max, cur, "slot_max", 0);
        if (item.slot_max == 0 && item.base_quality != EQUIP_QUALITY_NONE) {
            //装备且没配单槽的最大值 默认为1
            item.slot_max = 1;
        }   

        tmp_str.clear();
        decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "add_attr_val_list", "");
        std::vector<std::string> attr_list = split(tmp_str, ';');
        FOREACH(attr_list, it) {
            std::vector<std::string> items = split(*it, ',');
            if (items.size() != 2) {
                ERROR_TLOG("item :%u attr list invalid :[%s]", item.item_id, tmp_str.c_str());
                ERR_RT;
            }
            uint32_t type = atoi((items[0]).c_str());
            uint32_t value = atoi((items[1]).c_str());

            item_attr_conf_t attr_list = {type, value};
            item.item_attr_list.push_back(attr_list);
        }
        DECODE_XML_PROP_UINT32_DEFAULT(item.level_limit, cur, "level_limit", 0);
		if (item.level_limit > kMaxPetLevel) {
			ERROR_TLOG("item:%u level_limit exceed:level_limit:[%u],max_level:[%u]", 
					item.item_id, item.level_limit, kMaxPetLevel);
			ERR_RT;
		}

        DECODE_XML_PROP_UINT32_DEFAULT(item.buff_id, cur, "buff_id", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.cult_level_limit, cur, "cult_level_limit", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.req_vip_lv, cur, "req_vip_lv", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.is_magic_equip, cur, "is_magic_equip", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.star, cur, "star", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.base_buff1_ID, cur, "base_buff_1", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.base_buff2_ID, cur, "base_buff_2", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.base_buff3_ID, cur, "base_buff_3", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(item.base_buff4_ID, cur, "base_buff_4", 0);

        if (item.buff_id && g_client_buff_set.count(item.buff_id) == 0) {
            ERROR_TLOG("ItemClientBuff:%u not exist, item_id:%u", item.buff_id, item.item_id);
            ERR_RT;
        }
        if (item.base_buff1_ID && !g_equip_buff_rand_mgr.equip_buff_rand_group_exist(item.base_buff1_ID)) {
            ERROR_TLOG("ItemRandBuff:%u not exist, item_id:%u", item.base_buff1_ID, item.item_id);
            ERR_RT;
        }
        if (item.base_buff2_ID && !g_equip_buff_rand_mgr.equip_buff_rand_group_exist(item.base_buff2_ID)) {
            ERROR_TLOG("ItemRandBuff:%u not exist, item_id:%u", item.base_buff2_ID, item.item_id);
            ERR_RT;
        }
        if (item.base_buff3_ID && !g_equip_buff_rand_mgr.equip_buff_rand_group_exist(item.base_buff3_ID)) {
            ERROR_TLOG("ItemRandBuff:%u not exist, item_id:%u", item.base_buff3_ID, item.item_id);
            ERR_RT;
        }
        if (item.base_buff4_ID && !g_equip_buff_rand_mgr.equip_buff_rand_group_exist(item.base_buff4_ID)) {
            ERROR_TLOG("ItemRandBuff:%u not exist, item_id:%u", item.base_buff4_ID, item.item_id);
            ERR_RT;
        }

        tmp.add_item_conf(item);
        cur = cur->next;
    }

    g_item_conf_mgr.copy_from(tmp);
    

    return 0;
}

int load_player_config(xmlNodePtr root)
{
    player_conf_mgr_t tmp;
    tmp.clear();
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("prof"))) {
            cur = cur->next;
            continue;
        }

        player_conf_t player;
        DECODE_XML_PROP_UINT32(player.player_id, cur, "id");
        if (tmp.is_player_conf_exist(player.player_id)) {
            ERROR_TLOG("player %u already exists", player.player_id);
            ERR_RT;
        }

        uint32_t type;
        DECODE_XML_PROP_UINT32(type, cur, "prof");
        if (!player_conf_mgr_t::is_valid_prof(type)) {
            ERROR_TLOG("player %u prof:%u invalid", player.player_id, type);
            ERR_RT;
        }
        player.prof = (prof_type_t)type;

        DECODE_XML_PROP_UINT32(player.basic_normal_battle_values[kBattleValueNormalTypeHp], cur, "hp");
        DECODE_XML_PROP_UINT32(player.basic_normal_battle_values[kBattleValueNormalTypeNormalAtk], cur, "normal_atk");
        DECODE_XML_PROP_UINT32(player.basic_normal_battle_values[kBattleValueNormalTypeNormalDef], cur, "normal_def");
        DECODE_XML_PROP_UINT32(player.basic_normal_battle_values[kBattleValueNormalTypeSkillAtk], cur, "skill_atk");
        DECODE_XML_PROP_UINT32(player.basic_normal_battle_values[kBattleValueNormalTypeSkillDef], cur, "skill_def");
        DECODE_XML_PROP_UINT32(player.basic_normal_battle_values_grow[kBattleValueNormalTypeHp], cur, "hp_level_add");
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_normal_battle_values_grow[kBattleValueNormalTypeNormalAtk], cur, "normal_atk_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_normal_battle_values_grow[kBattleValueNormalTypeNormalDef], cur, "normal_def_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_normal_battle_values_grow[kBattleValueNormalTypeSkillAtk], cur, "skill_atk_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_normal_battle_values_grow[kBattleValueNormalTypeSkillDef], cur, "skill_def_level_add", 0);

        DECODE_XML_PROP_UINT32(player.basic_hide_battle_values[kBattleValueHideTypeCrit], cur, "crit");
        DECODE_XML_PROP_UINT32(player.basic_hide_battle_values[kBattleValueHideTypeAntiCrit], cur, "anti_crit");
        DECODE_XML_PROP_UINT32(player.basic_hide_battle_values[kBattleValueHideTypeDodge], cur, "dodge");
        DECODE_XML_PROP_UINT32(player.basic_hide_battle_values[kBattleValueHideTypeHit], cur, "hit");
        DECODE_XML_PROP_UINT32(player.basic_hide_battle_values[kBattleValueHideTypeBlock], cur, "block");
        DECODE_XML_PROP_UINT32(player.basic_hide_battle_values[kBattleValueHideTypeBreakBlock], cur, "break_block");
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values[kBattleValueHideTypeCritAffectRate], cur, "crit_affect_rate", 100);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values[kBattleValueHideTypeBlockAffectRate], cur, "block_affect_rate", 50);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_grow[kBattleValueHideTypeCrit], cur, "crit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_grow[kBattleValueHideTypeAntiCrit], cur, "anti_crit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_grow[kBattleValueHideTypeDodge], cur, "dodge_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_grow[kBattleValueHideTypeHit], cur, "hit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_grow[kBattleValueHideTypeBlock], cur, "block_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_grow[kBattleValueHideTypeBreakBlock], cur, "break_block_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_grow[kBattleValueHideTypeCritAffectRate], cur, "crit_affect_rate_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_grow[kBattleValueHideTypeBlockAffectRate], cur, "block_affect_rate_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_coeff[kBattleValueHideTypeCrit], cur, "crit_coeff", 5000);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_coeff[kBattleValueHideTypeAntiCrit], cur, "anti_crit_coeff", 10000);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_coeff[kBattleValueHideTypeDodge], cur, "dodge_coeff", 5000);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_coeff[kBattleValueHideTypeHit], cur, "hit_coeff", 10000);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_coeff[kBattleValueHideTypeBlock], cur, "block_coeff", 5000);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_coeff[kBattleValueHideTypeBreakBlock], cur, "break_block_coeff", 10000);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_coeff[kBattleValueHideTypeCritAffectRate], cur, "crit_affect_rate_coeff", 5000);
        DECODE_XML_PROP_UINT32_DEFAULT(player.basic_hide_battle_values_coeff[kBattleValueHideTypeBlockAffectRate], cur, "block_affect_rate_coeff", 10000);

        for (int i = 0; i <= kBattleValueHideTypeBreakBlock; i++) {
            if (player.basic_hide_battle_values_coeff[i] == 0) {
                ERROR_TLOG("Player_Conf:%u Hide Battle value coeff[%u] is zero",
                        player.player_id, i);
                ERR_RT;
            }
        }
        tmp.add_player_conf(player);
        cur = cur->next;
    }

    g_player_conf_mgr.copy_from(tmp);
    

    return 0;
}


int load_duplicate_config(xmlNodePtr root)
{
    duplicate_conf_manager_t tmp;
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("duplicate"))) {
            cur = cur->next;
            continue;
        }
        duplicate_t dup;
        DECODE_XML_PROP_UINT32(dup.duplicate_id, cur, "id");
        decode_xml_prop_uint32_default(&(dup.prev_id), cur, "prev_id", 0);
        if (dup.duplicate_id > MAX_DUPLICATE_ID || tmp.duplicate_exist(dup.duplicate_id)) {
            ERROR_TLOG("Duplicate id invalid(exist or too large) in duplicate.xml id=%u", dup.duplicate_id);
            ERR_RT;
        }
        uint32_t type = 0;
        decode_xml_prop_uint32_default(&type, cur, "limit_type", 1);
        if (type != 1 && type != 2) {
            ERROR_TLOG("Duplicate limit_type invalid dup:%u, limit_type:%u",
                    dup.duplicate_id, type);
            ERR_RT;
        }
        dup.limit_type = (duplicate_limit_type_t)type;
        decode_xml_prop_uint32_default(&(dup.initial_cnt), cur, "initial_cnt", 10);
        decode_xml_prop_uint32_default(&(dup.vip_initial_cnt), cur, "vip_initial_cnt", 10);
        decode_xml_prop_uint32_default(&(dup.svip_initial_cnt), cur, "svip_initial_cnt", 10);

        decode_xml_prop_uint32_default(&(dup.total_enter_count_limit_key), cur, "total_enter_count_limit_key", 0);
        decode_xml_prop_uint32_default(&(dup.buy_total_count_limit_key), cur, "buy_total_count_limit_key", 0);
        decode_xml_prop_uint32_default(&(dup.switch_days), cur, "switch_days", 0);
        decode_xml_prop_uint32_default(&(dup.element_id), cur, "element_id", 0);
        decode_xml_prop_uint32_default(&(dup.next_element_id), cur, "next_element_id", 0);
        decode_xml_prop_uint32_default(&(dup.element_level), cur, "element_level", 0);
        decode_xml_prop_uint32_default(&(dup.element_progress_attr), cur, "element_progress_attr", 0);
        decode_xml_prop_uint32_default(&(dup.area_num), cur, "area_num", 0);
        decode_xml_prop_uint32_default(&(dup.area_star_sum_attr), cur, "area_star_sum_attr", 0);
        decode_xml_prop_uint32_default(&(dup.achieve_task_id), cur, "achieve_task_id", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(dup.revivable, cur, "revivable", 0);

        decode_xml_prop_uint32_default(&type, cur, "mode", 1);
        if (!(type >= DUP_MODE_TYPE_NORMAL && type < DUP_MODE_TYPE_END)) {
            ERROR_TLOG("Duplicate mode invalid dup:%u, mode:%u",
                    dup.duplicate_id, type);
            ERR_RT;
        }
        dup.mode = (duplicate_mode_t)type;

        if (type == DUP_MODE_TYPE_ELEM_DUP && dup.duplicate_id > MAX_TRIAL_ATTR_PROGRESS) {
            ERROR_TLOG("Duplicate trial attr id out of range dup:%u, mode:%u",
                    dup.duplicate_id, type);
            ERR_RT;
        }

        decode_xml_prop_uint32_default(&type, cur, "battle_type", 1);
        if (type < DUP_BTL_TYPE_PVE || type >= DUP_BTL_TYPE_END) {
            ERROR_TLOG("Duplicate btl_type invalid dup:%u, btl_type:%u",
                    dup.duplicate_id, type);
            ERR_RT;
        }
        dup.battle_type = (duplicate_battle_type_t)type;

        char dup_prize_id_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(dup_prize_id_str, cur, "prize_id_list", "");
        std::vector<std::string> prize_list = split(dup_prize_id_str, ',');
        std::set<uint32_t> prize_set;
        FOREACH(prize_list, it) {
            uint32_t prize_id = atoi((*it).c_str());
            if (prize_set.count(prize_id) > 0) {
                ERROR_TLOG("Duplicate prize id for duplicate:%u, prize_id:%u", 
                        dup.duplicate_id, prize_id);
                ERR_RT;
            }
#if 0
            if (!g_prize_conf_mgr.is_prize_exist(prize_id)) {
                ERROR_TLOG("Duplicate :%u prize id :%u not exist",
                        dup.duplicate_id, prize_id);
                ERR_RT;
            }
#endif
            prize_set.insert(prize_id);
        }
        FOREACH(prize_set, it) {
            dup.prize_vec.push_back(*it);
        }

        memset(dup_prize_id_str, 0, sizeof(dup_prize_id_str));
        DECODE_XML_PROP_STR_DEFAULT(dup_prize_id_str, cur, "vip_prize_id_list", "");
        prize_list.clear();
        prize_list = split(dup_prize_id_str, ',');
        prize_set.clear();
        FOREACH(prize_list, it) {
            uint32_t prize_id = atoi((*it).c_str());
            if (prize_set.count(prize_id) > 0) {
                ERROR_TLOG("Duplicate prize id for duplicate:%u, prize_id:%u", 
                        dup.duplicate_id, prize_id);
                ERR_RT;
            }
#if 0
            if (!g_prize_conf_mgr.is_prize_exist(prize_id)) {
                ERROR_TLOG("Duplicate :%u prize id :%u not exist",
                        dup.duplicate_id, prize_id);
                ERR_RT;
            }
#endif
            prize_set.insert(prize_id);
        }
        FOREACH(prize_set, it) {
            dup.vip_prize_vec.push_back(*it);
        }

        //svip
        memset(dup_prize_id_str, 0, sizeof(dup_prize_id_str));
        DECODE_XML_PROP_STR_DEFAULT(dup_prize_id_str, cur, "svip_prize_id_list", "");
        prize_list.clear();
        prize_list = split(dup_prize_id_str, ',');
        prize_set.clear();
        FOREACH(prize_list, it) {
            uint32_t prize_id = atoi((*it).c_str());
            if (prize_set.count(prize_id) > 0) {
                ERROR_TLOG("Duplicate prize id for duplicate:%u, prize_id:%u", 
                        dup.duplicate_id, prize_id);
                ERR_RT;
            }
#if 0
            if (!g_prize_conf_mgr.is_prize_exist(prize_id)) {
                ERROR_TLOG("Duplicate :%u prize id :%u not exist",
                        dup.duplicate_id, prize_id);
                ERR_RT;
            }
#endif
            prize_set.insert(prize_id);
        }
        FOREACH(prize_set, it) {
            dup.svip_prize_vec.push_back(*it);
        }

        char dup_monster_id_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(dup_monster_id_str, cur, "mon_ids", "");
        std::vector<std::string> mon_list = split(dup_monster_id_str, ',');
        FOREACH(mon_list, it) {
            uint32_t pet_id = atoi((*it).c_str());
            if (!g_pet_conf_mgr.pet_conf_exist(pet_id)) {
                ERROR_TLOG("Duplicate monster id:%u for duplicate:%u not exist", 
                        pet_id, dup.duplicate_id);
                ERR_RT; 
            }
            dup.mon_vec.push_back(pet_id);
        }
		char dup_can_fight_pets_id[256] = {0};
		DECODE_XML_PROP_STR_DEFAULT(dup_can_fight_pets_id, cur, "can_fight_pet_ids","");
		std::vector<std::string> can_fight_pet_list = split(dup_can_fight_pets_id, ',');
		FOREACH(can_fight_pet_list, it) {
			uint32_t pet_id = atoi((*it).c_str());
			if (!g_pet_conf_mgr.pet_conf_exist(pet_id)) {
				ERR_RT;
			}
			dup.can_fight_pets_id.push_back(pet_id);
		}

        decode_xml_prop_uint32_default(&(dup.time_limit), cur, "time_limit", 300);
        decode_xml_prop_uint32_default(&(dup.star_1_time), cur, "star_1_time", 250);
        decode_xml_prop_uint32_default(&(dup.star_2_time), cur, "star_2_time", 200);
        decode_xml_prop_uint32_default(&(dup.star_3_time), cur, "star_3_time", 100);
        decode_xml_prop_uint32_default(&(dup.level_limit), cur, "level_limit", 0);
        decode_xml_prop_uint32_default(&(dup.task_id), cur, "task_id", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(dup.open_tm_sub_key, cur, "open_tm_sub_key", 0);
        if (dup.open_tm_sub_key 
            && !TimeUtils::is_valid_time_conf_sub_key(TM_CONF_KEY_DUP_OPEN, dup.open_tm_sub_key)) {
            ERROR_TLOG("Duplicate open_time_sub_key err dup:%u sub_key:%u",
                    dup.duplicate_id, dup.open_tm_sub_key);
            ERR_RT;
        }

        DECODE_XML_PROP_UINT32_DEFAULT(dup.least_users, cur, "least_users", 1);

        char dup_consume_item_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(dup_consume_item_str, cur, "consume_item_id", "");
        std::vector<std::string> consume_items_list = split(dup_consume_item_str, ';');
        duplicate_consume_item_t consume;
        FOREACH(consume_items_list, it) {
            std::vector<std::string> id_vec = split((*it).c_str(), ',');
            if (id_vec.size() != 2) {
                ERROR_TLOG("Duplicate consume item format err dup:%u consume[%s]", 
                        dup.duplicate_id, (*it).c_str());
                ERR_RT;
            }
            uint32_t item_id = atoi(id_vec[0].c_str());
            uint32_t cnt = atoi(id_vec[1].c_str());
            if (!g_item_conf_mgr.is_item_conf_exist(item_id)) {
                ERROR_TLOG("Duplicate consume item not exist dup:%u, item:%u",
                        dup.duplicate_id, item_id);
                ERR_RT;
            }
            consume.item_id = item_id;
            consume.cnt = cnt;
            dup.consume_items.push_back(consume);
        }

        decode_xml_prop_uint32_default(&(dup.consume_physique), cur, "consume_vp", 0);
        decode_xml_prop_uint32_default(&(dup.consume_gold), cur, "consume_gold", 0);
        decode_xml_prop_uint32_default(&(dup.consume_diamond), cur, "consume_diamond", 0);
        decode_xml_prop_uint32_default(&(dup.cd_time), cur, "cd_time", 0);
        decode_xml_prop_uint32_default(&(dup.map_id), cur, "map_id", 0);
        decode_xml_prop_uint32_default(&(dup.area_id), cur, "area_id", 0);

        decode_xml_prop_uint32_default(&(dup.buy_pass_shop_id), cur, "buy_pass_shop_id", 0);
        decode_xml_prop_uint32_default(&(dup.have_pet_id), cur, "have_pet_id", 0);

        decode_xml_prop_uint32_default(&(dup.buy_fight_cnt_limit), cur, "buy_cnt", 0);
        decode_xml_prop_uint32_default(&(dup.vip_buy_fight_cnt_limit), cur, "vip_buy_cnt", 0);
        decode_xml_prop_uint32_default(&(dup.svip_buy_fight_cnt_limit), cur, "svip_buy_cnt", 0);
        decode_xml_prop_uint32_default(&(dup.buy_fight_shop_id), cur, "buy_fight_shop_id", 0);
        decode_xml_prop_uint32_default(&(dup.default_buy_fight_cnt), cur, "default_buy_fight_cnt", 1);

        char name_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(name_str, cur, "name", "");
        dup.name.assign(name_str);

        tmp.add_duplicate(dup);
        if (dup.duplicate_id > g_cur_max_dup_id) {
            g_cur_max_dup_id = dup.duplicate_id;
        }
		//添加以mode为索引的副本id集合
		tmp.add_dup_id_to_mode_dup_ids(dup);
        cur = cur->next;
    }

    g_duplicate_conf_mgr.copy_from(tmp);
	g_duplicate_conf_mgr.print_mode_dup_ids();
    //把副本的次数限制 写入属性值的initial配置中去
#define SET_ATTR(attr) \
        do {\
            if (!g_attr_configs.count(attr)) {\
                attr_config_t tmp;\
                tmp.id = attr;\
                g_attr_configs[attr] = tmp;\
            }\
            attr_config_t &attr_conf = (g_attr_configs.find(attr))->second;\
            attr_conf.initial = dconf.initial_cnt;\
            attr_conf.vip_initial = dconf.vip_initial_cnt;\
            attr_conf.svip_initial = dconf.svip_initial_cnt;\
        } while (0)

    FOREACH(g_duplicate_conf_mgr.const_dup_map(), it) {
        const duplicate_t &dconf = it->second;
        attr_type_t daily_attr = AttrUtils::get_duplicate_daily_times_attr(dconf.duplicate_id);
        attr_type_t weekly_attr = AttrUtils::get_duplicate_weekly_times_attr(dconf.duplicate_id);
        SET_ATTR((uint32_t)daily_attr);
        SET_ATTR((uint32_t)weekly_attr);
    }
#undef SET_ATTR
    return 0;
}

int load_pet_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    pet_conf_manager_t tmp;

    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)"pet")) {
            cur = cur->next;
            continue;
        }
        pet_conf_t pet_conf;

        DECODE_XML_PROP_UINT32(pet_conf.id, cur, "id");
        if (tmp.pet_conf_exist(pet_conf.id)) {
            ERROR_TLOG("pet %u already exists", pet_conf.id);
            ERR_RT;  
        }
        uint32_t type = 0;
        DECODE_XML_PROP_UINT32(type, cur, "elem_type");
        if (type > kMaxPetElemTypeValue) {
            ERROR_TLOG("pet %u elem_type not exist[%u]", pet_conf.id, type);
            ERR_RT;
        }
        pet_conf.elem_type = (pet_elem_type_t)type;
        /*
        DECODE_XML_PROP_UINT32(type, cur, "grow_type");
        if (type != kPetGrowType1 && type != kPetGrowType2) {
            ERROR_TLOG("pet %u grow_type not exist[%u]", pet_conf.id, type);
            ERR_RT;
        }
        pet_conf.growth_type = (pet_grow_type_t)type;
        */
        char name_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(name_str, cur, "name", "");
        pet_conf.name.assign(name_str);

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.sex, cur, "sex", 0);
        if (pet_conf.sex != 0 && pet_conf.sex != 1) {
            ERROR_TLOG("pet %u sex invalid", pet_conf.sex);
            ERR_RT;
        }
		DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.must_drop_prize, cur, "must_drop_prize", 0);
		DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.rand_drop_prize, cur, "rand_drop_prize", 0);
        char pet_prize_id_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(pet_prize_id_str, cur, "prize_id_list", "");
        std::vector<std::string> pet_prize_list = split(pet_prize_id_str, ',');
        std::set<uint32_t> pet_prize_set;
        FOREACH(pet_prize_list, it) {
            uint32_t prize_id = atoi((*it).c_str());
            if (pet_prize_set.count(prize_id) > 0) {
                ERROR_TLOG("Pet prize %u repeate pet:%u", 
                        prize_id, pet_conf.id);
                ERR_RT;
            }
            pet_prize_set.insert(prize_id);
        }
        pet_conf.prize_id_list.clear();
        FOREACH(pet_prize_set, it) {
            pet_conf.prize_id_list.push_back(*it);
        }

        //task_prize
        DECODE_XML_PROP_STR_DEFAULT(pet_prize_id_str, cur, "task_prize_list", "");
        pet_prize_list = split(pet_prize_id_str, ';');
        FOREACH(pet_prize_list, it) {
            std::vector<std::string> task_prizes = split(*it, ',');
            if (task_prizes.size() < 2) {
                ERROR_TLOG("Pet %u task_prize format invalid", pet_conf.id);
                ERR_RT;
            }
            pet_prize_set.clear();
            uint32_t task_id = atoi((task_prizes[0]).c_str());
            task_prizes.erase(task_prizes.begin());
            FOREACH(task_prizes, it2) {
                uint32_t prize_id = atoi((*it2).c_str());
                if (pet_prize_set.count(prize_id) > 0) {
                    ERROR_TLOG("Pet prize %u repeate pet:%u", 
                            prize_id, pet_conf.id);
                    ERR_RT;
                }
                pet_prize_set.insert(prize_id);
            }
            std::vector<uint32_t> prize_vec;
            FOREACH(pet_prize_set, it2) {
                prize_vec.push_back(*it2);
            }
            pet_conf.task_prize_list.insert(make_pair(task_id, prize_vec));
        }

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_battle_value, cur, "basic_bv", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.level, cur, "level", 1);

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.mon_type, cur, "mon_type", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.hit_prize_id, cur, "hit_prize_id", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.hit_prize_count, cur, "hit_prize_count", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values[kBattleValueNormalTypeHp], cur, "hp", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values[kBattleValueNormalTypeNormalAtk], cur, "normal_atk", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values[kBattleValueNormalTypeNormalDef], cur, "normal_def", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values[kBattleValueNormalTypeSkillAtk], cur, "skill_atk", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values[kBattleValueNormalTypeSkillDef], cur, "skill_def", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeHp], cur, "hp_level_add", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeNormalAtk], cur, "normal_atk_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeNormalDef], cur, "normal_def_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeSkillAtk], cur, "skill_atk_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeSkillDef], cur, "skill_def_level_add", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeCrit], cur, "crit", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeAntiCrit], cur, "anti_crit", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeDodge], cur, "dodge", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeHit], cur, "hit", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeBlock], cur, "block", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeBreakBlock], cur, "break_block", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeCritAffectRate], cur, "crit_affect_rate", 100);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeBlockAffectRate], cur, "block_affect_rate", 50);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeCrit], cur, "crit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeAntiCrit], cur, "anti_crit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeDodge], cur, "dodge_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeHit], cur, "hit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeBlock], cur, "block_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeBreakBlock], cur, "break_block_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeCritAffectRate], cur, "crit_affect_rate_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeBlockAffectRate], cur, "block_affect_rate_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_coeff[kBattleValueHideTypeCrit], cur, "crit_coeff", 5000);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_coeff[kBattleValueHideTypeAntiCrit], cur, "anti_crit_coeff", 10000);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_coeff[kBattleValueHideTypeDodge], cur, "dodge_coeff", 5000);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_coeff[kBattleValueHideTypeHit], cur, "hit_coeff", 10000);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_coeff[kBattleValueHideTypeBlock], cur, "block_cpeff", 5000);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_coeff[kBattleValueHideTypeBreakBlock], cur, "break_block_coeff", 10000);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_coeff[kBattleValueHideTypeCritAffectRate], cur, "crit_affect_rate_coeff", 5000);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_coeff[kBattleValueHideTypeBlockAffectRate], cur, "block_affect_rate_coeff", 10000);

        for (int i = 0; i <= kBattleValueHideTypeBreakBlock; i++) {
            if (pet_conf.basic_hide_battle_values_coeff[i] == 0) {
                ERROR_TLOG("Pet:%u Hide Battle value coeff[%u] is zero",
                        pet_conf.id, i);
                ERR_RT;
            }
        }
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeAtkSpeed], cur, "atk_speed", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.is_level_add, cur, "is_level_add", 1);


		char skill_ids_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(skill_ids_str, cur, "skill_ids", "");
        std::vector<std::string> skill_ids_vec = split(skill_ids_str, ',');
		FOREACH(skill_ids_vec, it) {
			int skill_id = atoi((*it).c_str());
			if (!g_skill_conf_mgr.skill_conf_exist(skill_id)) {
				ERROR_TLOG("Pet %u skill_id:%u not exist",
						   pet_conf.id, skill_id);
				ERR_RT;
			}
					
			pet_conf.skill_ids_vec.push_back(skill_id);
		}
							
			
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.evolve_from, cur, "evolve_from", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.evolve_to, cur, "evolve_to", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.evolve_talent, cur, "evolve_talent", 0);
        //DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.evolve_item, cur, "evolve_item", 0);
        //DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.evolve_item_cnt, cur, "evolve_item_count", 1);

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.own_max, cur, "own_max", 10);
        //DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.egg_item, cur, "egg_item", 0);
        //DECODE_XML_PROP_UINT32(pet_conf.egg_drop_rate, cur, "egg_drop_rate");

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.talent_item, cur, "talent_item", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.born_talent, cur, "born_talent", 1);

        if (pet_conf.born_talent == kPetTalentLevelNone
            || pet_conf.born_talent > kPetTalentLevelFive) {
            ERROR_TLOG("Pet %u born_talent:%u invalid[1-5]", pet_conf.id, pet_conf.born_talent);
            ERR_RT;
        }

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.is_hide, cur, "is_hide", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.abandon_flag, cur, "abandonable", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.waken_type, cur, "waken_type", 0);
        std::string tmp_str;
        decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "group_ids", "");
        std::vector<std::string> item_vec = split(tmp_str, ',');
        FOREACH(item_vec, iter) {
            pet_conf.group_ids.push_back(atoi((*iter).c_str()));
        }

        if (pet_conf.id < kMonPetIDStart) {//小于的都是精灵 其他是野怪
            /*
            if (pet_conf.egg_item && !g_item_conf_mgr.is_item_conf_exist(pet_conf.egg_item)) {
                ERROR_TLOG("Pet %u egg_item:%u not exist", 
                        pet_conf.id, pet_conf.egg_item);
                ERR_RT;
            }
            */
            if (!g_item_conf_mgr.is_item_conf_exist(pet_conf.talent_item)) {
                ERROR_TLOG("Pet %u talent_item:%u not exist", 
                        pet_conf.id, pet_conf.talent_item);
                ERR_RT;
            }
            /*
            if (pet_conf.evolve_item && !g_item_conf_mgr.is_item_conf_exist(pet_conf.evolve_item)) {
                ERROR_TLOG("Pet %u evolve_item:%u not exist", 
                        pet_conf.id, pet_conf.evolve_item);
                ERR_RT;
            }
            */
        }

        TRACE_TLOG("pet #%u: elem_type = %u, "
                " evolves_from = %u "
                " evolves_to = %u",
                pet_conf.id, pet_conf.elem_type,
                pet_conf.evolve_from,
                pet_conf.evolve_to);

        for (int i = 0; i < kMaxBattleValueTypeNum; i++) {
            TRACE_TLOG("pet #%u, battle type %u, value = %u",
                    pet_conf.id, i, pet_conf.basic_normal_battle_values[i]); 
        }
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.dup_pass_time_attr, cur, "dup_pass_time_attr", 0);

        tmp.add_pet_conf(pet_conf);
        cur = cur->next;
    }

    //检查进化依赖from与to精灵的存在性 以及防止循环进化
    FOREACH(tmp.const_pet_conf_map(), it) {
        const pet_conf_t *me = &(it->second);
        const pet_conf_t *from = tmp.find_pet_conf(me->evolve_from);
        const pet_conf_t *to = tmp.find_pet_conf(me->evolve_to);
        std::set<const pet_conf_t*> evolve_set;
        if (me->evolve_from && !from) {
            ERROR_TLOG("Pet %u evolve_from non-exist pet %u",
                    me->id, me->evolve_from);
            ERR_RT;
        }
        if (me->evolve_to && !to) {
            ERROR_TLOG("Pet %u evolve_to non-exist pet %u",
                    me->id, me->evolve_to);
            ERR_RT;
        }
        if (me->evolve_from && from->evolve_to != me->id) {
            ERROR_TLOG("Pet %u's evolve_from != from->evolve_to",
                    me->id);
            ERR_RT;
        }
        if (me->evolve_to && to->evolve_from != me->id) {
            ERROR_TLOG("Pet %u's evolve_to != to->evolve_from",
                    me->id);
            ERR_RT;
        }

        if (from && (/*from->growth_type != me->growth_type*/ from->elem_type != me->elem_type)) {
            ERROR_TLOG("Pet %u evolve_from pet of other or elem_type[%u->%u]",
                    me->id, from->elem_type, me->elem_type);
            ERR_RT;
        }
        if (to && (/*to->growth_type != me->growth_type*/ to->elem_type != me->elem_type)) {
            ERROR_TLOG("Pet %u evolve_to pet of other elem_type[%u->%u]",
                    me->id, to->elem_type, me->elem_type);
            ERR_RT;
        }
        if ((from && from == to) || from == me || to == me) {
            ERROR_TLOG("Pet %u evolve_from and evolve_to the same pet", me->id);
            ERR_RT;
        }
        const pet_conf_t *start = from ?from :me;
        evolve_set.clear();
        evolve_set.insert(start);
        while(start) {
            if (start->evolve_to) {
                to = tmp.find_pet_conf(start->evolve_to);
                if (!to) {
                    ERROR_TLOG("Pet %u evolve_to non-exist pet %u",
                            start->id, start->evolve_to);
                    ERR_RT;
                }
                if (evolve_set.count(to) > 0) {
                    ERROR_TLOG("Pet %u evolve chain has loop %u", start->id, to->id);
                    ERR_RT;
                }
                evolve_set.insert(to);
                start = to;
            } else {
                start = 0;
            }
        }
    }

    g_pet_conf_mgr.copy_from(tmp);
    
    return 0;
}
/*
int load_skill_hit_config(xmlNodePtr root, skill_conf_manager_t &mgr)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("hurt"))) {
            cur = cur->next;
            continue;
        }
        uint32_t skill_id;
        uint32_t hit_id;
        uint32_t hurt_rate;
        DECODE_XML_PROP_UINT32(skill_id, cur, "skill_id");
        DECODE_XML_PROP_UINT32(hit_id, cur, "id");
        DECODE_XML_PROP_UINT32(hurt_rate, cur, "hurt_rate");
        if (!mgr.skill_conf_exist(skill_id)) {
            ERROR_TLOG("Skill_id %u for skill_hurt.xml not exist in skill_parent.xml", skill_id);
            return -1;
        }
        skill_conf_t *skill = mgr.find_mutable_skill_conf(skill_id);
        skill->hurt_rate.push_back(hurt_rate);

        cur = cur->next;
    }
    return 0;
}
*/
int load_skill_level_config(xmlNodePtr root, skill_conf_manager_t &mgr)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("skill"))) {
            cur = cur->next;
            continue;
        }
        skill_conf_t skill;
        uint32_t skill_id;
        DECODE_XML_PROP_UINT32(skill_id, cur, "parent_skill_id");
        if (!mgr.skill_conf_exist(skill_id)) {
            ERROR_TLOG("Skill_id %u not exist for skill.xml in skill_parent.xml",
                    skill_id);
            return -1;
        }
        skill_conf_t *parent_skill = mgr.find_mutable_skill_conf(skill_id);

        DECODE_XML_PROP_UINT32(skill.skill_level, cur, "level");
        skill.skill_id = skill_id * 10 + skill.skill_level;
        if (mgr.skill_conf_exist(skill.skill_id)) {
            ERROR_TLOG("Duplicate skill_id_level in skill.xml[id %u, level %u]",
                    skill_id, skill.skill_level);
            return -1;
        }
        DECODE_XML_PROP_UINT32(skill.learn_level, cur, "learn_level");
        DECODE_XML_PROP_UINT32(skill.cd, cur, "cd");
        DECODE_XML_PROP_UINT32(skill.sp, cur, "tp");
        DECODE_XML_PROP_UINT32_DEFAULT(skill.normal_hurt_rate, cur, "normal_hurt", 100);
        DECODE_XML_PROP_UINT32_DEFAULT(skill.skill_hurt_rate, cur, "skill_hurt", 100);

        DECODE_XML_PROP_UINT32_DEFAULT(skill.hits, cur, "hits", 0);
        if (!skill.hits) {
            skill.hits = parent_skill->hits;
        }
        char hurt_rate_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(hurt_rate_str, cur, "hurt_rate", "");
        std::vector<std::string> hurt_rate_vec = split(hurt_rate_str, ',');

        if (skill.hits == 1) {
            skill.hurt_rate.push_back(100);
        } else if (skill.hits == parent_skill->hits) { //沿用父技能的hits
            if (hurt_rate_vec.size() == 0) {//使用父技能的伤害率
                skill.hurt_rate = parent_skill->hurt_rate;
            } else if (hurt_rate_vec.size() != skill.hits) {//使用自己的伤害率 但数量不对
                ERROR_TLOG("Skill %u hits != hurt_rate.size()", skill.skill_id);
                return -1;
            } else {
                FOREACH(hurt_rate_vec, it) {
                    int rate = atoi((*it).c_str());
                    skill.hurt_rate.push_back(rate);
                }
            }
        } else { //使用自己的hits
            if (hurt_rate_vec.size() != skill.hits) {//使用自己的伤害率 但数量不对
                ERROR_TLOG("Skill %u hits != hurt_rate.size()", skill.skill_id);
                return -1;
            } else {
                FOREACH(hurt_rate_vec, it) {
                    int rate = atoi((*it).c_str());
                    skill.hurt_rate.push_back(rate);
                }
            }
        }

        

        mgr.add_skill_conf(skill);
        cur = cur->next;
    }
    return 0;
}

int load_skill_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    skill_conf_manager_t tmp;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("skill"))) {
            cur = cur->next;
            continue;
        }
        skill_conf_t skill;
        DECODE_XML_PROP_UINT32(skill.skill_id, cur, "id");
        if (tmp.skill_conf_exist(skill.skill_id)) {
            ERROR_TLOG("Duplicate skill in skill.xml id=%u", skill.skill_id);
            ERR_RT;
        }
        DECODE_XML_PROP_UINT32_DEFAULT(skill.hits, cur, "hits", 1);
        if (skill.hits == 0) {
            skill.hits = 1;
        }
        char hurt_rate_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(hurt_rate_str, cur, "hurt_rate", "");
        std::vector<std::string> hurt_rate_vec = split(hurt_rate_str, ',');

        if (hurt_rate_vec.size() && skill.hits != hurt_rate_vec.size()) {
            ERROR_TLOG("Skill %u hits != hurt_rate.size()", skill.skill_id);
            ERR_RT;
        }
        if (skill.hits == 1) {
            skill.hurt_rate.push_back(100);
        } else if (hurt_rate_vec.size() == 0){
            uint32_t mod = 100 % skill.hits;
            uint32_t rate = 100 - mod;
            for (uint32_t i = 0; i < (skill.hits - 1); i++) {
                skill.hurt_rate.push_back(rate/skill.hits);
            }
            skill.hurt_rate.push_back(rate/skill.hits + mod);
        } else {
            FOREACH(hurt_rate_vec, it) {
                int rate = atoi((*it).c_str());
                skill.hurt_rate.push_back(rate);
            }
        }

        std::string tmp_str;
        decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "pet_from", "");
        std::vector<std::string> pet_from_list = split(tmp_str, ',');
        FOREACH(pet_from_list, iter) {
            skill.pet_from.push_back(atoi((*iter).c_str()));
        }

        DECODE_XML_PROP_INT_DEFAULT(skill.skill_type, cur, "skill_type", 0);

        skill.is_parent = 1;
        tmp.add_skill_conf(skill);
        cur = cur->next;
    }
    int ret = 0;
    tmp.show();

    const char *file = 0;
    xmlDocPtr doc = 0;
    xmlNodePtr head = 0;
    /*
    file = gen_full_path(g_server_config.conf_path, "skill_hurt.xml");
    xmlDocPtr doc = xmlParseFile(file);
    if (!doc) {
        ERROR_TLOG("Failed to Load %s", file);
        xmlFreeDoc(doc);
        ERR_RT;
    }
    xmlNodePtr head = xmlDocGetRootElement(doc); 
    if (!head) {
        ERROR_TLOG("xmlDocGetRootElement error when loading file '%s'", file);
        xmlFreeDoc(doc);
        ERR_RT;
    }
    ret = load_skill_hit_config(head, tmp);
    xmlFreeDoc(doc);
    if (ret) {
        ERR_RT;
    }
    FOREACH((tmp.mutable_skill_conf_map()), it) {
        skill_conf_t &skill = it->second;
        if (skill.hits != 0 && skill.hurt_rate.size() == 0) {
            for (uint32_t i = 0; i < skill.hits; i++) {
                skill.hurt_rate.push_back(100/skill.hits);
            }
        }
    }
    */
    file = gen_full_path(g_server_config.conf_path, "skill.xml");
    doc = xmlParseFile(file);
    if (!doc) {
        xmlErrorPtr xptr = xmlGetLastError();
        ERROR_TLOG("Failed to Load %s [line:%u msg:%s]", file, xptr->line, xptr->message);
        xmlFreeDoc(doc);
        ERR_RT;
    }
    head = xmlDocGetRootElement(doc); 
    if (!head) {
        ERROR_TLOG("xmlDocGetRootElement error when loading file '%s'", file);
        xmlFreeDoc(doc);
        ERR_RT;
    }
    ret = load_skill_level_config(head, tmp);
    xmlFreeDoc(doc);
    if (ret) {
        ERR_RT;
    }

    FOREACH_NOINCR_ITER((tmp.const_skill_conf_map()), it) {
        /*
        std::map<uint32_t, skill_conf_t>::const_iterator it1 = it;
        if (it1->second.is_parent) {
            it++;
            tmp.remove_skill_conf(it1->first);
            continue;
        }
        */
        //校验技能的字段
        skill_conf_t *skill = tmp.find_mutable_skill_conf(it->second.skill_id);
        if (skill->learn_level > kMaxPetLevel) {
            ERROR_TLOG("Skill %u learn level too large %u", 
                    skill->skill_id, skill->learn_level);
            ERR_RT;
        }
        if (skill->hits != skill->hurt_rate.size()) {
            ERROR_TLOG("Skill %u hits[%u] != hurt_rate_size[%u]",
                    skill->skill_id, skill->hits, skill->hurt_rate.size());
            ERR_RT;
        }
        uint32_t rate = 0;
        FOREACH(skill->hurt_rate, it2) {
            rate += *it2;
        }
        /*
        if (rate != 100) {
            ERROR_TLOG("Skill %u hits hurt_rate not equal 100", skill->skill_id);
            ERR_RT;
        }
        */
        it++;
    }
    g_skill_conf_mgr.copy_from(tmp);
    g_skill_conf_mgr.show();
    
    return 0;
}

int load_prize_element_config(xmlNodePtr root, prize_conf_manager_t &tmp)
{
    prize_elem_t elem;
    uint32_t prize_id = 0;
    uint32_t type = 0;
    xmlNodePtr child = root->xmlChildrenNode;
    while (child) {
        if (!xmlStrEqual(child->name, (const xmlChar *)"elem")) {
            child = child->next;
            continue;
        }
        elem.clear();
        DECODE_XML_PROP_UINT32(prize_id, child, "prize_id");
        prize_config_t *pz = tmp.get_mutable_prize_conf(prize_id);
        if (!pz) {
            ERROR_TLOG("Prize id %u for prize_element.xml not exist in prize.xml", prize_id);
            return -1;
        }
        DECODE_XML_PROP_UINT32(type, child, "type");

        uint32_t calc_type = 0;
        DECODE_XML_PROP_UINT32_DEFAULT(calc_type, child, "calc_type", 0);
        if (calc_type >= prize_incr_type_max) {
            ERROR_TLOG("Load Prize Config:[%u] incr_type:%u invalid", prize_id, calc_type);
            ERR_RT;
        }
        elem.calc_type = (prize_incr_type_t)calc_type;

        DECODE_XML_PROP_UINT32_DEFAULT(elem.adjust_type, child, "adjust_type", 0);
        if (elem.adjust_type && !TimeUtils::is_valid_time_conf_sub_key(TM_CONF_KEY_PRIZE_ADJUST, elem.adjust_type)) {
            ERROR_TLOG("Load Prize Config:[%u] ajust_type:%u invalid", prize_id, elem.adjust_type);
            ERR_RT;
        }
		//是否加入奖励榜
        decode_xml_prop_uint32_default(&elem.notice, child, "notice", 0);
    	//跑马灯
        decode_xml_prop_uint32_default(&elem.show, child, "show", 0);
        // 加载每日/每周/每月/永久上限
        decode_xml_prop_uint32_default(&elem.daily_limit_key, child, "daily_limit_key", 0);
        decode_xml_prop_uint32_default(&elem.daily_limit_max, child, "daily_limit_max", 0);
        decode_xml_prop_uint32_default(&elem.weekly_limit_key, child, "weekly_limit_key", 0);
        decode_xml_prop_uint32_default(&elem.weekly_limit_max, child, "weekly_limit_max", 0);
        decode_xml_prop_uint32_default(&elem.monthly_limit_key, child, "monthly_limit_key", 0);
        decode_xml_prop_uint32_default(&elem.monthly_limit_max, child, "monthly_limit_max", 0);
        decode_xml_prop_uint32_default(&elem.forever_limit_key, child, "forever_limit_key", 0);
        decode_xml_prop_uint32_default(&elem.forever_limit_max, child, "forever_limit_max", 0);
        //decode_xml_prop_uint32_default(&elem.first_dup_must_drop, child, "first_dup_must_drop", 0);

        if (type == 1) {
            //物品奖
            DECODE_XML_PROP_UINT32(elem.id, child, "target_id");
            DECODE_XML_PROP_UINT32(elem.count, child, "count");
            decode_xml_prop_uint32_default(&(elem.duration), child, "duration", 0);
            decode_xml_prop_uint32_default(&(elem.award_rate), child, "award_rate", 0);
            decode_xml_prop_uint32_default(&(elem.display_rate), child, "display_rate", 0);
            // decode_xml_prop_uint32_default(&(elem.price_rate), child, "price_rate", 0);
            decode_xml_prop_uint32_default(&(elem.price_type), child, "price_type", 0);
            decode_xml_prop_uint32_default(&(elem.price), child, "price", 0);
            //判断物品是否存在
            if (!g_item_conf_mgr.is_item_conf_exist(elem.id)) {
                ERROR_TLOG("Load Prize config[%d]: item[%d] not exist", pz->prize_id, elem.id);    
                ERR_RT;
            }
            pz->prize_items.push_back(elem);
        } else if (type == 2) {
            //精灵奖
            DECODE_XML_PROP_INT(elem.id, child, "target_id");
            DECODE_XML_PROP_INT(elem.talent_level, child, "talent_level");
            DECODE_XML_PROP_UINT32(elem.level, child, "level");
            decode_xml_prop_uint32_default(&(elem.award_rate), child, "award_rate", 0);
            decode_xml_prop_uint32_default(&(elem.display_rate), child, "display_rate", 0);
            // decode_xml_prop_uint32_default(&(elem.price_rate), child, "price_rate", 0);
            decode_xml_prop_uint32_default(&(elem.price_type), child, "price_type", 0);
            decode_xml_prop_uint32_default(&(elem.price), child, "price", 0);

            //判断精灵是否存在
            if (PetUtils::get_pet_conf(elem.id) == NULL) {
                ERROR_TLOG("Load Prize Config[%d]: pet[%d] not exist", pz->prize_id, elem.id);
                ERR_RT;
            }
            elem.count = 1;
            pz->prize_pets.push_back(elem);
        } else if (type == 3) {
            //属性奖
            DECODE_XML_PROP_UINT32(elem.id, child, "target_id");
            DECODE_XML_PROP_INT(elem.count, child, "count");
            decode_xml_prop_uint32_default(&(elem.award_rate), child, "award_rate", 0);
            decode_xml_prop_uint32_default(&(elem.display_rate), child, "display_rate", 0);
            // decode_xml_prop_uint32_default(&(elem.price_rate), child, "price_rate", 0);
            decode_xml_prop_uint32_default(&(elem.price_type), child, "price_type", 0);
            decode_xml_prop_uint32_default(&(elem.price), child, "price", 0);
            //判断属性是否存在
            if (g_attr_configs.find((attr_type_t)(elem.id))
                == g_attr_configs.end()) {
                ERROR_TLOG("Load Prize Config[%d]: attr[%d] not exist", pz->prize_id, elem.id);
                ERR_RT;
            }
            pz->prize_attrs.push_back(elem);
        } else if (type == 5) {
			DECODE_XML_PROP_UINT32(elem.id, child, "target_id");
            DECODE_XML_PROP_UINT32(elem.level, child, "level");
            decode_xml_prop_uint32_default(&(elem.award_rate), child, "award_rate", 0);
            decode_xml_prop_uint32_default(&(elem.display_rate), child, "display_rate", 0);
            // decode_xml_prop_uint32_default(&(elem.price_rate), child, "price_rate", 0);
            decode_xml_prop_uint32_default(&(elem.price_type), child, "price_type", 0);
            decode_xml_prop_uint32_default(&(elem.price), child, "price", 0);
			//判断符文是否存在
			if (!g_rune_conf_mgr.is_rune_conf_exist(elem.id)) {
				ERROR_TLOG("Load Prize Config[%d]: runeid[%d] not exist", pz->prize_id, elem.id);
				ERR_RT;
			}
			pz->prize_runes.push_back(elem);
		} else if (type == 4) {
			DECODE_XML_PROP_UINT32(elem.id, child, "target_id");
			decode_xml_prop_uint32_default(&(elem.award_rate), child, "award_rate", 0);
			decode_xml_prop_uint32_default(&(elem.display_rate), child, "display_rate", 0);
            // decode_xml_prop_uint32_default(&(elem.price_rate), child, "price_rate", 0);
            decode_xml_prop_uint32_default(&(elem.price_type), child, "price_type", 0);
            decode_xml_prop_uint32_default(&(elem.price), child, "price", 0);
			elem.count = 1;
			//TO kevin后期把称号解析放到前面，这里判断称号是否存在
			if (!g_title_conf_mgr.is_title_conf_exist(elem.id)) {
				ERROR_TLOG("Load Prize Config[%d]:titleid[%d] not exist", pz->prize_id, elem.id);
				ERR_RT;
			}
			pz->prize_titles.push_back(elem);
		}
		child = child->next;
	}//prize
	return 0;
}

int load_prize_config(xmlNodePtr root)
{
    prize_conf_manager_t tmp;
    tmp.clear_prize_conf();

    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar*)("prize"))) {
            cur = cur->next;
            continue;
        }
        prize_config_t  pz;
        pz.clear();
        DECODE_XML_PROP_UINT32(pz.prize_id, cur, "id");
        if (tmp.is_prize_exist(pz.prize_id)) {
            ERROR_TLOG("Duplicate Prize in prize.xml ID=%u", pz.prize_id);
            ERR_RT;
        }
        DECODE_XML_PROP_INT_DEFAULT(pz.rand_mode, cur, "rand_mode", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pz.show, cur, "show", 0);
        decode_xml_prop_uint32_default(&(pz.display_cnt), cur, "display_count", 0);
        DECODE_XML_PROP_STR_DEFAULT(pz.desc, cur, "des", "未知奖励");
 
        tmp.add_prize_conf(pz);
        cur = cur->next;
    }
    const char *file = gen_full_path(g_server_config.conf_path, "prize_element.xml");
    xmlDocPtr doc = xmlParseFile(file);
    if (!doc) {
        xmlErrorPtr xptr = xmlGetLastError();
        ERROR_TLOG("Failed to Load %s [line:%u msg:%s]", file, xptr->line, xptr->message);
        xmlFreeDoc(doc);
        ERR_RT;
    }
    xmlNodePtr head = xmlDocGetRootElement(doc); 
    if (!head) {
        ERROR_TLOG("xmlDocGetRootElement error when loading file '%s'", file);
        xmlFreeDoc(doc);
        ERR_RT;
    }
    int ret = load_prize_element_config(head, tmp);
    xmlFreeDoc(doc);
    if (ret) {
        ERR_RT;
    }
    FOREACH((tmp.get_const_prize_map()), it) {
        const prize_config_t &pz = it->second;
        uint32_t total_size = pz.prize_items.size()
            + pz.prize_attrs.size()
            + pz.prize_pets.size()
            + pz.prize_runes.size();

        if (pz.rand_mode != 0 && pz.rand_mode != SELF_RAND_MODE) {
            if (pz.rand_mode + pz.display_cnt > total_size) {
                ERROR_TLOG("Load Prize Conf[%d]: rand_mode[%d]+display_cnt[%d] > total_size[%d]",
                        pz.prize_id, pz.rand_mode, pz.display_cnt, total_size);
                ERR_RT;
            }
        }
    }
    
    g_prize_conf_mgr.copy_from(tmp);

    return 0;
}

int load_rune_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	rune_conf_mgr_t tmp;
	tmp.clear();
	
	
	while (cur) {
		/*
		   if (!xmlStrEqual(cur->name, (const xmlChar *)"Rune")) {
		   cur = cur->next;
		   continue;
		   }
		   */
		if (!xmlStrcmp(cur->name, (const xmlChar *)"rune")) {
			rune_conf_t rune;

			DECODE_XML_PROP_UINT32(rune.rune_id, cur, "id");
			DECODE_XML_PROP_UINT32(rune.rune_type, cur, "type");
			//DECODE_XML_PROP_UINT32(rune.fun_type, cur, "Fun");
			decode_xml_prop_uint32_default(&rune.fun_type, cur, "fun", 0);
			//DECODE_XML_PROP_UINT32(rune.fun_value, cur, "Funvar");
			decode_xml_prop_uint32_default(&rune.fun_value, cur, "funvar", 0);
			//DECODE_XML_PROP_UINT32(rune.arg_rate, cur, "ArgsRate");
			decode_xml_prop_uint32_default(&rune.arg_rate, cur, "argsrate", 0);
			//DECODE_XML_PROP_UINT32(rune.tran_add_exp, cur, "TransAddExp");
			decode_xml_prop_uint32_default(&rune.tran_add_exp, cur, "transaddexp", 0);
			tmp.add_rune_conf(rune);
		}
		cur = cur->next;
	}
	g_rune_conf_mgr.copy_from(tmp);
	g_rune_conf_mgr.print_rune_info();
	TRACE_TLOG("load rune.xml;id=[%u],");
	
	return 0;
}

int load_rune_exp_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	rune_exp_conf_mgr_t tmp;
	tmp.clear();
	while (cur) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)"rune")) {
			rune_exp_conf_t rune_exp;
			rune_exp.rune_type = 0;
			DECODE_XML_PROP_UINT32(rune_exp.rune_type, cur, "type");
			//uint32_t id;
			//DECODE_XML_PROP_UINT32(id, cur, "id");
			//decode_xml_prop_uint32_default(&id, cur, "id", 1);
			//decode_xml_prop_uint32_default(&(rune_exp.rune_type), cur, "type", 1);
			//DECODE_XML_PROP_UINT32(rune_exp.rune_type, cur, "type");
			//taomee::get_xml_prop(rune_exp.rune_type, cur, "Type");
			if (!(rune_exp.rune_type >= 1 && rune_exp.rune_type <= kMaxRuneExpType)) {
				ERROR_TLOG("invalid rune type %u", rune_exp.rune_type);
				ERR_RT;
			}    
			char buf[50];
			DECODE_XML_PROP_STR(buf, cur, "exp");
			std::vector<std::string> exp_list = split(buf, ',');
			if (exp_list.size() != kMaxRuneExpLevel) {
				ERROR_TLOG("invalid exp size %u",(uint32_t)exp_list.size());
				ERR_RT;
			}
			for (uint32_t i=0; i < exp_list.size(); i++) {
				int exp = boost::lexical_cast<int>(exp_list[i]);
				if (exp != 0) {
					rune_exp.exp_vec.push_back(exp);
				}
			}
			tmp.add_rune_exp_conf(rune_exp);
		}
		cur = cur->next;
	}
	g_rune_exp_conf_mgr.copy_from(tmp);
	g_rune_exp_conf_mgr.print_rune_exp_info();
		
	return 0;	
}

int load_rune_rate_config(xmlNodePtr root)
{
	rune_rate_conf_mgr_t tmp_rune_rate;
	tmp_rune_rate.clear();
	
    xmlNodePtr cur = root->xmlChildrenNode;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("call"))) {
			cur = cur->next;
			continue;
		}
		call_level_info tem01;
		std::vector<rune_rate_conf_t> tem_vec;
		DECODE_XML_PROP_UINT32(tem01.level, cur, "level");
		decode_xml_prop_uint32_default(&tem01.coin, cur, "coin", 0);
		decode_xml_prop_uint32_default(&tem01.callrate, cur, "callrate", 0);
		xmlNodePtr cur1 = cur->xmlChildrenNode;
		while (cur1) {
			if (!xmlStrEqual(cur1->name, (const xmlChar*)("rune"))) {
				cur1 = cur1->next;
				continue;
			}
			rune_rate_conf_t tem02;
			tem02.level = tem01.level;
			DECODE_XML_PROP_UINT32(tem02.conf_id, cur1, "id");
			if (!g_rune_conf_mgr.is_rune_conf_exist(tem02.conf_id)) {
				ERROR_TLOG("rune_rate_config,conf_id_err: id=[%u]", tem02.conf_id);
				ERR_RT;
			}
			DECODE_XML_PROP_UINT32(tem02.rate, cur1, "rate");
			tem_vec.push_back(tem02);
			cur1 = cur1->next;
		}
		tmp_rune_rate.add_rune_rate_conf(tem_vec, tem01.level);	
		tmp_rune_rate.add_call_level_conf(tem01, tem01.level);
		cur = cur->next;
	}
	g_rune_rate_conf_mgr.copy_from(tmp_rune_rate);	
	g_rune_rate_conf_mgr.print_rune_rate_info();
	
	return 0;
}

//加载任务信息 
int load_task_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    task_conf_mgr_t temp_task_conf_mgr;

    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("task"))) {
            cur = cur->next;
            continue;
        }
        uint32_t task_id = 0;
        DECODE_XML_PROP_UINT32(task_id, cur, "id");

        if (task_id < 0 || task_id >= kMaxTaskNum) {
            ERROR_TLOG("invalid task id %u, max = %u", task_id, kMaxTaskNum);
            ERR_RT;
        }

        TRACE_TLOG("task_xml_id=%u", task_id);
        task_conf_t task;
        task.task_id = task_id;

        if (temp_task_conf_mgr.is_task_conf_exist(task_id)) {
            ERROR_TLOG("duplicate task basic info %u", task_id); 
            ERR_RT;
        }
        task.task_id = task_id;

        DECODE_XML_PROP_STR_DEFAULT(task.task_name, cur, "name", "未知任务");
        DECODE_XML_PROP_UINT32(task.step_count, cur, "stepCnt");
        TRACE_TLOG("task.step_count=%u", task.step_count);
        if (!(task.step_count >= 1 && task.step_count <= 31)) {
            ERROR_TLOG("invalid task.step_count: %u", task.step_count); 
            ERR_RT;
        }

        DECODE_XML_PROP_UINT32_DEFAULT(task.type, cur, "tasktype",0);
        DECODE_XML_PROP_UINT32_DEFAULT(task.repeate, cur, "repeate",1);

        char buf[100];
        DECODE_XML_PROP_STR(buf, cur, "parentList");            
        std::vector<std::string> parent_str_list = split(buf, ',');
        for (uint32_t i=0; i < parent_str_list.size(); i++) {
            int parent = atoi(parent_str_list[i].c_str());
            if (parent != 0) {
                task.parent_list.push_back(parent);
            }
        }

        for (uint32_t i = 0; i < task.step_count; i++) {
            step_conf_t temp_step_conf;
            task.step_conf_vec.push_back(temp_step_conf);
        }

        // 解析跳过步骤信息
        std::string step_jump_str;
        decode_xml_prop_default<std::string, std::string>(step_jump_str, cur, "step_jump", "");
        std::vector<std::string> step_jump_list = split(step_jump_str, ';');
        uint32_t tmp_max = 0;
        FOREACH(step_jump_list,it) {
            std::vector<std::string> step_jump_vec = split(*it, ',');
            if(step_jump_vec.size() != 2) {
                ERROR_TLOG("step_jump config failed, task_id:%u", task_id);
                ERR_RT;
            }

            uint32_t src_step_id = atoi(step_jump_vec[0].c_str());
            uint32_t tar_step_id = atoi(step_jump_vec[1].c_str());

            if (src_step_id >= tar_step_id ||
                    src_step_id <= tmp_max ||
                    tar_step_id >  task.step_count) {
                ERROR_TLOG("step_jump config need ascending order or over max step limit, task_id:%u, src:%u, dst:%u", 
                        task_id, src_step_id, tar_step_id);
                ERR_RT;
            }

            tmp_max = tar_step_id;

            task.step_jump_map.insert(std::pair<uint32_t, uint32_t>(src_step_id, tar_step_id));
        }

        // 解析奖励
        DECODE_XML_PROP_STR(task.bonus_id, cur, "bonus");
		//Confirm kevin 检查奖励是否存在
		std::vector<std::string> bonus_vec = split(task.bonus_id, ',');
		FOREACH(bonus_vec, it) {
			uint32_t prize_id = atoi((*it).c_str());
			if (!g_prize_conf_mgr.is_prize_exist(prize_id)) {
				ERROR_TLOG("Task Config:Not Exist Prizeid:[%u][%u]", prize_id, task.task_id);
				ERR_RT;
			}
		}

        // 解析配表修改标志
        DECODE_XML_PROP_UINT32_DEFAULT(task.abandon_flag, cur, "abandon_flag", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(task.auto_finish, cur, "auto_finish", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(task.auto_prize, cur, "auto_prize", 0);

        if (task.auto_finish == 0 && task.auto_prize) {
            ERROR_TLOG("task config mode error, auto_finish:%u, auto_prize:%u", task.auto_finish, task.auto_prize);
        }

        DECODE_XML_PROP_UINT32_DEFAULT(task.daily_reset, cur, "daily_reset", 0);
        char time_str_a[100]={};
        DECODE_XML_PROP_STR_DEFAULT(time_str_a, cur, "start_time", "");
        if (strlen(time_str_a) > 0 && 
                TimeUtils::time_str_to_long(time_str_a, task.start_time) != 0) {
            ERROR_TLOG("Load task time failed, start time illegal, start:%s", time_str_a);    
            ERR_RT;
        }

        memset(time_str_a, 0, sizeof(time_str_a));
        DECODE_XML_PROP_STR_DEFAULT(time_str_a, cur, "end_time", "");
        if (strlen(time_str_a) > 0 && 
                TimeUtils::time_str_to_long(time_str_a, task.end_time) != 0) {
            ERROR_TLOG("Load task time failed, end time illegal, end:%s", time_str_a);    
            ERR_RT;
        }
        DECODE_XML_PROP_UINT32_DEFAULT(task.multi_condition, cur, "multi_condition", 0);

        TRACE_TLOG("task=%u bonus=%s,multi_condition=%u", task_id, task.bonus_id, task.multi_condition);

        xmlNodePtr taskChild = cur->xmlChildrenNode;
        while(taskChild) {
            if (!xmlStrcmp(taskChild->name, (const xmlChar *)"accept")) {
                xmlNodePtr taskGrandChild = taskChild->xmlChildrenNode;
                while(taskGrandChild) {
                    if(!xmlStrcmp(taskGrandChild->name, (const xmlChar *)"condition")) {
                        condition_conf_t con_conf;
                        DECODE_XML_PROP_UINT32(con_conf.id, taskGrandChild, "id");
                        DECODE_XML_PROP_STR(con_conf.params, taskGrandChild, "params");
                        task.accept_con_vec.push_back(con_conf);                        

                    } else if(!xmlStrcmp(taskGrandChild->name, (const xmlChar *)"combine")) {
                        char str_express[128];
                        DECODE_XML_PROP_STR(str_express, taskGrandChild, "express");

                        std::string combine_str(str_express);
                        std::stringstream ss;
                        ss.str("");
                        ss.clear();
                        for (uint32_t i = 0; i < combine_str.size(); i++) {
                            if(isdigit((int)combine_str.at(i))) {
                                ss << combine_str.c_str();
                            } else {
                                if(0 != ss.gcount()) {
                                    task.accept_con_exp_vec.push_back(ss.str());
                                }
                                ss.str("");
                                ss.clear();
                                task.accept_con_exp_vec.push_back(combine_str.c_str());
                            }

                            if(0 != ss.str().size()) {
                                task.accept_con_exp_vec.push_back(ss.str());
                                ss.str("");
                                ss.clear();
                            }                            
                        }

                        TRACE_TLOG("task xml : accpet task_id:%u, vec.size:%u, express:%s, combine_str:%s", 
                                task.task_id,
                                task.accept_con_exp_vec.size(),
                                ss.str().c_str(), combine_str.c_str());
                    }
                    taskGrandChild = taskGrandChild->next;
                }
            } else if(!xmlStrcmp(taskChild->name, (const xmlChar *)"complete")) {
                xmlNodePtr taskGrandChild = taskChild->xmlChildrenNode;
                uint32_t step = 0;
                DECODE_XML_PROP_UINT32(step, taskChild, "step");

                while(taskGrandChild) {
                    if(!xmlStrcmp(taskGrandChild->name, (const xmlChar *)"condition")) {
                        condition_conf_t con_conf;
                        uint32_t type;
                        DECODE_XML_PROP_UINT32(con_conf.id, taskGrandChild, "id");
                        DECODE_XML_PROP_UINT32(type, taskGrandChild, "type");
                        DECODE_XML_PROP_STR(con_conf.params, taskGrandChild, "params");
                        DECODE_XML_PROP_UINT32(con_conf.operate, taskGrandChild, "operate");
                        con_conf.type = (condition_type_t)type;
                        if (task.step_conf_vec.size() <= step - 1) {
                            ERROR_TLOG("step out of range of stepCnt, task_id: %u",
                                    task.task_id);
                            ERR_RT;
                        }
                        task.step_conf_vec[step - 1].complete_con_vec.push_back(con_conf);                        
                        TRACE_TLOG("task xml : task_id:%u, step:%u, con_conf.id:%u,vec.size:%u", 
                                task.task_id, step,
                                con_conf.id, task.step_conf_vec[step - 1].complete_con_vec.size());
                    } else if(!xmlStrcmp(taskGrandChild->name, (const xmlChar *)"combine")) {
                        char str_express[128];
                        DECODE_XML_PROP_STR(str_express, taskGrandChild, "express");

                        if (task.step_conf_vec.size() <= step - 1) {
                            ERROR_TLOG("step out of range of stepCnt, task_id: %u",
                                    task.task_id);
                            ERR_RT;
                        }
                        std::string combine_str(str_express);
                        std::stringstream ss;
                        ss.str("");
                        ss.clear();
                        for (uint32_t i = 0; i < combine_str.size(); i++) {
                            if(isdigit((int)combine_str.at(i))) {
                                ss << combine_str.c_str();
                            } else {
                                if(0 != ss.gcount()) {
                                    task.step_conf_vec[step - 1].complete_con_exp_vec.push_back(ss.str());
                                }
                                ss.str("");
                                ss.clear();
                                task.step_conf_vec[step - 1].complete_con_exp_vec.push_back(combine_str.c_str());
                            }

                            if(0 != ss.str().size()) {
                                task.step_conf_vec[step - 1].complete_con_exp_vec.push_back(ss.str());
                                ss.str("");
                                ss.clear();
                            }                            
                        }

                        TRACE_TLOG("task xml : task_id:%u, step:%u, vec.size:%u, express:%s, combine_str:%s", 
                                task.task_id, step,
                                task.step_conf_vec[step - 1].complete_con_exp_vec.size(),
                                ss.str().c_str(), combine_str.c_str());
                    }
                    taskGrandChild = taskGrandChild->next;
                }
            }            
            taskChild = taskChild->next;
        }
        temp_task_conf_mgr.add_task_conf(task);
        cur = cur->next;
    }

    g_task_conf_mgr.copy_from(temp_task_conf_mgr);

    init_task_cond_fun();
    return 0;
}

int init_task_cond_fun()
{
#define CONDITION_FUN(i) g_condition_fun[i] = TaskUtils::condition_fun_##i;
    CONDITION_FUN(1);
    CONDITION_FUN(2);
    CONDITION_FUN(3);
    CONDITION_FUN(5);
    CONDITION_FUN(6);

    CONDITION_FUN(10);
    CONDITION_FUN(11);
    CONDITION_FUN(13);
    CONDITION_FUN(14);
    CONDITION_FUN(15);
    CONDITION_FUN(17);
    CONDITION_FUN(18);
    CONDITION_FUN(19);
    CONDITION_FUN(20);
    CONDITION_FUN(21);
    CONDITION_FUN(22);

#undef CONDITION_FUN
 
    return 0;
}

int load_tran_card_config(xmlNodePtr root) 
{
	xmlNodePtr cur = root->xmlChildrenNode;
	tran_card_conf_mgr_t tmp_tran_card;
	tmp_tran_card.clear();
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("card"))) {
			cur = cur->next;
			continue;
		}
		tran_card_conf_t tran_card;
		DECODE_XML_PROP_UINT32(tran_card.conf_id, cur, "id");
		char buf[50];
		DECODE_XML_PROP_STR(buf, cur, "skill_ids");
		std::vector<std::string> skill_list = split(buf, ',');
		if (skill_list.size() > KMaxTranSkillNum) {
			ERROR_TLOG("invalid skill size %u", static_cast<uint32_t>(skill_list.size()));
			ERR_RT;
		}
		for (uint32_t i = 0; i < skill_list.size(); ++i) {
			uint32_t skill_id = boost::lexical_cast<uint32_t>(skill_list[i]);
			if (skill_id != 0) {
				tran_card.skill_ids.push_back(skill_id);
			}
		}
		char buf1[50];
		DECODE_XML_PROP_STR(buf1, cur, "evolve_item");
		std::vector<std::string> evolve_list = split(buf1, ',');
		for (uint32_t i = 0; i < evolve_list.size(); ++i) {
			uint32_t evolve_item = boost::lexical_cast<uint32_t>(evolve_list[i]);
			if (evolve_item != 0) {
				tran_card.evolve_items.push_back(evolve_item);
			}
		}
		tmp_tran_card.add_tran_card_conf(tran_card);
		cur = cur->next;
	}
	g_tran_card_conf_mgr.copy_from(tmp_tran_card);
	g_tran_card_conf_mgr.print_tran_card_info();
	return 0;
}


int load_builder_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    builder_conf_manager_t tmp;

    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)"builder")) {
            cur = cur->next;
            continue;
        }
        builder_conf_t builder_conf;

        DECODE_XML_PROP_UINT32(builder_conf.id, cur, "id");
        if (tmp.builder_conf_exist(builder_conf.id)) {
            ERROR_TLOG("Builder %u already exists", builder_conf.id);
            ERR_RT;  
        }
        DECODE_XML_PROP_INT_DEFAULT(builder_conf.points, cur, "points", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.hit_prize_id, cur, "hit_prize_id", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.hit_prize_count, cur, "hit_prize_count", 0);

        char builder_prize_id_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(builder_prize_id_str, cur, "prize_id_list", "");
        std::vector<std::string> builder_prize_list = split(builder_prize_id_str, ',');
        std::set<uint32_t> builder_prize_set;
        FOREACH(builder_prize_list, it) {
            uint32_t prize_id = atoi((*it).c_str());
            if (builder_prize_set.count(prize_id) > 0) {
                ERROR_TLOG("Builder prize %u repeate builder:%u", 
                        prize_id, builder_conf.id);
                ERR_RT;
            }
            builder_prize_set.insert(prize_id);
        }
        builder_conf.prize_id_list.clear();
        FOREACH(builder_prize_set, it) {
            builder_conf.prize_id_list.push_back(*it);
        }

        tmp.add_builder_conf(builder_conf);
        cur = cur->next;
    }

    g_builder_conf_mgr.copy_from(tmp);
    g_builder_conf_mgr.show();

    return 0;
}

// 加载exchange item配置
int load_exchange_item_config(xmlNodePtr cur, std::vector<exchange_item_config_t>& item_list)
{
    item_list.clear();
    while (cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"item")) {
            exchange_item_config_t item_config; 
            memset(&item_config, 0, sizeof(item_config));
            DECODE_XML_PROP_UINT32(item_config.id, cur, "id");
            if (!g_item_conf_mgr.is_item_conf_exist(item_config.id)) {
                ERROR_TLOG("Load exchange item config failed, item:%u not exist",
                        item_config.id);
                return -1;
            }
            DECODE_XML_PROP_UINT32(item_config.count, cur, "count");
            decode_xml_prop_uint32_default(&item_config.expire_time, cur, "expire_time", 0);
            decode_xml_prop_uint32_default(&item_config.rate, cur, "rate", 100);
            item_list.push_back(item_config);
        }
        cur = cur->next;
    }
    return 0;
}

// 加载exchange attr配置
int load_exchange_attr_config(xmlNodePtr cur, std::vector<exchange_attr_config_t>& attr_list)
{
    attr_list.clear();
    while (cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"attr")) {
            exchange_attr_config_t attr_config;
            memset(&attr_config, 0, sizeof(attr_config));
            DECODE_XML_PROP_UINT32(attr_config.id, cur, "id");
            if (g_attr_configs.count(attr_config.id) == 0) {
                ERROR_TLOG("Load exchange attr config failed, attr:%u not exist",
                        attr_config.id);
                return -1;
            }
            DECODE_XML_PROP_UINT32(attr_config.count, cur, "count");
            decode_xml_prop_uint32_default(&attr_config.rate, cur, "rate", 100);
            attr_list.push_back(attr_config);
        }
        cur = cur->next;
    }
    return 0;
}

// 加载exchange pet配置
int load_exchange_pet_config(xmlNodePtr cur, std::vector<exchange_pet_config_t>& pet_list)
{
    pet_list.clear();
    while (cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"pet")) {
            exchange_pet_config_t pet_config;
            DECODE_XML_PROP_UINT32(pet_config.id, cur, "id");
            if (!g_pet_conf_mgr.pet_conf_exist(pet_config.id)) {
                ERROR_TLOG("Load exchange pet conf failed, pet:%u not exist", pet_config.id);
                return -1;
            }
            DECODE_XML_PROP_UINT32(pet_config.level, cur, "level");
            if (pet_config.level > kMaxPetLevel) {
                ERROR_TLOG("Load exchange pet conf failed, pet_level:%u too large", pet_config.level);
                return -1;
            }
            decode_xml_prop_uint32_default(&pet_config.rate, cur, "rate", 100);
            pet_list.push_back(pet_config);
        }
        cur = cur->next;
    }
    return 0;
}

// 加载exchange In的配置
int load_exchange_in_config(xmlNodePtr cur, exchange_in_config_t* in_config)
{
    while (cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"in")) {
            in_config->item_list.clear();
            int ret = load_exchange_item_config(cur->xmlChildrenNode, in_config->item_list);
            if (ret != 0) {
                return -1;         
            }
            in_config->attr_list.clear();
            ret = load_exchange_attr_config(cur->xmlChildrenNode, in_config->attr_list);
            if (ret != 0) {
                return -1;
            }
        }
        cur = cur->next;
    }
    return 0;
}

// 加载exchange Out的配置
int load_exchange_out_config(xmlNodePtr cur, exchange_out_config_t* out_config)
{
    while (cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"out")) {
            out_config->item_list.clear();
            int ret = load_exchange_item_config(cur->xmlChildrenNode, out_config->item_list);
            if (ret != 0) {
                return -1;
            }
            out_config->attr_list.clear();
            ret = load_exchange_attr_config(cur->xmlChildrenNode, out_config->attr_list);
            if (ret != 0) {
                return -1;
            }
            out_config->pet_list.clear();
            ret = load_exchange_pet_config(cur->xmlChildrenNode, out_config->pet_list);
            if (ret != 0) {
                return -1;
            }
            break;
        }
        cur = cur->next;
    }
    return 0;
}

// 加载兑换配置
int load_exchange_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;

    exchange_conf_manager_t tmp;
    while (cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"exchange")) {
            exchange_config_t exchange_config;
            DECODE_XML_PROP_UINT32(exchange_config.id, cur, "id");
            if (tmp.exchg_exist(exchange_config.id)) {
                ERROR_TLOG("duplicate exchange config %u", exchange_config.id);
                ERR_RT;
            }
            decode_xml_prop_uint32_default(&exchange_config.daily_key, cur, "dailykey", 0);
            decode_xml_prop_uint32_default(&exchange_config.weekly_key, cur, "weeklykey", 0);
            decode_xml_prop_uint32_default(&exchange_config.monthly_key, cur, "monthlykey", 0);
            decode_xml_prop_uint32_default(&exchange_config.forever_key, cur, "foreverkey", 0);
            decode_xml_prop_uint32_default(&exchange_config.must_vip, cur, "must_vip", 0);
            decode_xml_prop_uint32_default(&exchange_config.addiction, cur, "addiction", 0);

            char msg_dir[128] = {0};
            DECODE_XML_PROP_STR_DEFAULT(msg_dir, cur, "msg_dir", "");
            exchange_config.msg_dir.clear();
            exchange_config.msg_dir.append(msg_dir);

            char msg_name[128] = {0};
            DECODE_XML_PROP_STR_DEFAULT(msg_name, cur, "msg_name", "");
            exchange_config.msg_name.clear();
            exchange_config.msg_name.append(msg_name);

            char msg_sub_name[128] = {0};
            DECODE_XML_PROP_STR_DEFAULT(msg_sub_name, cur, "msg_subname", "");
            exchange_config.msg_sub_name.clear();
            exchange_config.msg_sub_name.append(msg_sub_name);

            char desc[128] = {0};
            DECODE_XML_PROP_STR_DEFAULT(desc, cur, "desc", "未知兑换");
            exchange_config.msg_sub_name.clear();
            exchange_config.msg_sub_name.append(desc);

            decode_xml_prop_uint32_default(&exchange_config.rand_mode, cur, "rand", 0);

            int ret = load_exchange_in_config(cur->xmlChildrenNode, &exchange_config.in);
            if (ret != 0) {
                ERROR_TLOG("load exchange %u in config failed", exchange_config.id);
                ERR_RT;
            }
            ret = load_exchange_out_config(cur->xmlChildrenNode, &exchange_config.out);
            if (ret != 0) {
                ERROR_TLOG("load exchange %u out config failed", exchange_config.id);
                ERR_RT;
            }

            uint32_t total_out = exchange_config.out.item_list.size() + 
                exchange_config.out.attr_list.size() + exchange_config.out.pet_list.size();
            if (exchange_config.rand_mode > total_out) {
                ERROR_TLOG("load exchange %u rand_mode(%u) > total_out(%u) config",
                        exchange_config.id, exchange_config.rand_mode, total_out);
                ERR_RT;
            }
            tmp.add_exchg(exchange_config);
        }
        cur = cur->next;
    }
    g_exchg_conf_mgr.copy_from(tmp);
    g_exchg_conf_mgr.show();
    return 0;
}

int load_arena_streak_reward_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	arena_streak_reward_conf_mgr_t tmp_streak_reward;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("arena"))) {
			cur = cur->next;
			continue;
		}
		streak_info_t streak_info;
		decode_xml_prop_uint32_default(&streak_info.count, cur, "count", 0);
		decode_xml_prop_uint32_default(&streak_info.prize_id, cur, "prize_id", 0);
		if (!g_prize_conf_mgr.is_prize_exist(streak_info.prize_id)) {
            ERROR_TLOG("Arena prize id not exist in prize.xml ID=%u", streak_info.prize_id);
            ERR_RT;
		}
		tmp_streak_reward.add_streak_conf(streak_info);
		cur = cur->next;
	}
	g_arena_streak_reward_conf_mgr.copy_from(tmp_streak_reward);
	g_arena_streak_reward_conf_mgr.print_streak_info();
	return 0;
}

int load_arena_rank_reward_config(xmlNodePtr root) 
{
	xmlNodePtr cur = root->xmlChildrenNode;
	arena_rank_reward_mgr_t tmp_rank_reward;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("arena"))) {
			cur = cur->next;
			continue;
		}
		rank_reward_t reward_info;
		DECODE_XML_PROP_UINT32(reward_info.id, cur, "id");
		decode_xml_prop_uint32_default(&reward_info.start_rank, cur, "start_rank", 0);
		decode_xml_prop_uint32_default(&reward_info.end_rank, cur, "end_rank", 0);
		decode_xml_prop_uint32_default(&reward_info.bonus_id, cur, "bonus_id", 0);
		if (!g_prize_conf_mgr.is_prize_exist(reward_info.bonus_id)) {
            ERROR_TLOG("arenas.xml:Arena rank prize id not exist in prize.xml ID=%u", reward_info.bonus_id);
            ERR_RT;
		}
		decode_xml_prop_uint32_default(&reward_info.single_reward, cur, "single_reward", 0);
		if (!g_prize_conf_mgr.is_prize_exist(reward_info.single_reward)) {
            ERROR_TLOG("arenas.xml:Arena rank prize id not exist in prize.xml ID=%u", reward_info.single_reward);
            ERR_RT;
		}
		//decode_xml_prop_uint32_default(&reward_info.item_id, cur, "item_id", 0);
		//decode_xml_prop_uint32_default(&reward_info.item_count, cur, "item_count", 0);
		//decode_xml_prop_uint32_default(&reward_info.add_exp, cur, "add_exp", 0);
		//decode_xml_prop_uint32_default(&reward_info.coin, cur, "coin", 0);
		//decode_xml_prop_uint32_default(&reward_info.arenacoin, cur, "arenacoin", 0);
		tmp_rank_reward.add_rank_reward_conf(reward_info);
		cur = cur->next;
	}
	g_arena_rank_reward_conf_mgr.copy_from(tmp_rank_reward);
	g_arena_rank_reward_conf_mgr.print_rank_reward_info();
	return 0;
}

int load_trans_profession_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	trans_prof_conf_manager_t trans_prof_conf_manager;	
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("prof"))) {
			cur = cur->next;
			continue;
		}
		trans_prof_conf_t trans_prof_conf;
		DECODE_XML_PROP_UINT32(trans_prof_conf.prof_id, cur, "prof_id");
		DECODE_XML_PROP_UINT32(trans_prof_conf.stage, cur, "stage");
		DECODE_XML_PROP_UINT32(trans_prof_conf.hp, cur, "hp");
		DECODE_XML_PROP_UINT32(trans_prof_conf.normal_atk, cur, "normal_atk");
		DECODE_XML_PROP_UINT32(trans_prof_conf.normal_def, cur, "normal_def");
		DECODE_XML_PROP_UINT32(trans_prof_conf.skill_atk, cur, "skill_atk");
		DECODE_XML_PROP_UINT32(trans_prof_conf.skill_def, cur, "skill_def");
		DECODE_XML_PROP_UINT32(trans_prof_conf.crit, cur, "crit");
		DECODE_XML_PROP_UINT32(trans_prof_conf.anti_crit, cur, "anti_crit");
		DECODE_XML_PROP_UINT32(trans_prof_conf.hit, cur, "hit");
		DECODE_XML_PROP_UINT32(trans_prof_conf.dodge, cur, "dodge");
		DECODE_XML_PROP_UINT32(trans_prof_conf.block, cur, "block");
		DECODE_XML_PROP_UINT32(trans_prof_conf.break_block, cur, "break_block");
		DECODE_XML_PROP_UINT32(trans_prof_conf.level, cur, "level");
        DECODE_XML_PROP_UINT32_DEFAULT(trans_prof_conf.gold, cur, "gold", 0);

		char skill[128];
        DECODE_XML_PROP_STR(skill, cur, "skill");

        std::vector<std::string> parent_str_list = split(skill, ',');
        for (uint32_t i = 0; i < parent_str_list.size(); i++) {
            int parent = atoi(parent_str_list[i].c_str());
            if (parent != 0) {
                trans_prof_conf.skill_list.push_back(parent);
            }
        }
		char item[128];
        DECODE_XML_PROP_STR(item, cur, "item");
		parent_str_list.clear();
        parent_str_list = split(item, ';');

        for (uint32_t i = 0; i < parent_str_list.size(); i++) {
            std::vector<string> child_str_list = split(parent_str_list[i], ',');
            if (child_str_list.size() != 2) {
                ERROR_TLOG("load trans_profession:%u failed. item_format_invalid",
                        trans_prof_conf.prof_id);
                ERR_RT;
            }
            for (uint32_t j = 0; j < child_str_list.size(); j++) {
                uint32_t item_id = atoi(child_str_list[0].c_str());
                if (!g_item_conf_mgr.is_item_conf_exist(item_id)) {
                    ERROR_TLOG("load trans_profession:%u failed. item_id:%u not exist",
                            trans_prof_conf.prof_id, item_id);
                    ERR_RT;
                }
                uint32_t count = atoi(child_str_list[1].c_str());   
                trans_prof_conf.consume_item_map[item_id] = count;
            }
        }

		trans_prof_conf_manager.add_trans_prof_conf(trans_prof_conf);
		cur = cur->next;
	}

    g_trans_prof_conf_manager.copy_from(trans_prof_conf_manager);
	return 0;
}

int load_hm_gift_config(xmlNodePtr root) 
{
	xmlNodePtr cur = root->xmlChildrenNode;
	home_gift_conf_mgr_t tmp_hm_gift;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("gift"))) {
			cur = cur->next;
			continue;
		}
		home_gift_conf_t hm_gift;
		DECODE_XML_PROP_UINT32(hm_gift.id, cur, "fragment_id");
		decode_xml_prop_uint32_default(&hm_gift.rate, cur, "rate", 0);
		decode_xml_prop_uint32_default(&hm_gift.need_count, cur, "need_count", 0);
		decode_xml_prop_uint32_default(&hm_gift.to_item_id, cur, "to_item_id", 0);
		decode_xml_prop_uint32_default(&hm_gift.count, cur, "count", 0);
		if (!g_item_conf_mgr.is_item_conf_exist(hm_gift.to_item_id)) {
			ERROR_TLOG("Load Prize config[%d]: item[%d] not exist", hm_gift.to_item_id);    
			ERR_RT;
		}
		tmp_hm_gift.add_hm_gift_conf(hm_gift);	
		tmp_hm_gift.add_hm_gift_rate(hm_gift.rate);
		cur = cur->next;
	}
	g_hm_gift_mgr.copy_from(tmp_hm_gift);
	//g_hm_gift_mgr.print_hm_gift_info();
	//g_hm_gift_mgr.print_hm_gift_vec();
	return 0;
}

int load_name_pool_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
    std::map< uint32_t, rand_name_pool_t > tmp;
    while (cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"item")) {
            rand_name_pool_t name_vec;

            char tag[commonproto::MAX_RAND_NAME_TAG_LEN];
            uint32_t pos = 0;
            uint32_t tag_type = 0;
            DECODE_XML_PROP_STR(tag, cur, "tag");
            DECODE_XML_PROP_UINT32(pos, cur, "pos");
            DECODE_XML_PROP_UINT32(tag_type, cur, "tag_type");

            if (!(pos > 0 && pos <= commonproto::MAX_RAND_NAME_TAG_POS_TYPE )) {
        		ERROR_TLOG("Load rand_name_pool config[%d]: pos(%u) out of range", pos);    
			    ERR_RT;
            }

            if (tag_type == 0) {
            	ERROR_TLOG("Load rand_name_pool config[%d]: tag_type(%u) out of range", tag_type);    
			    ERR_RT;
            }

            std::map<uint32, rand_name_pool_t>::iterator iter = tmp.find(tag_type);
           if (iter == tmp.end()) {
                name_vec.name_pool[pos - 1].push_back(tag);
                tmp.insert(std::pair<uint32_t, rand_name_pool_t>(tag_type, name_vec));
            } else {
                iter->second.name_pool[pos - 1].push_back(tag);
            }

            TRACE_TLOG("rand_name_pool tag = '%s', pos = %d, tag_type = %d", tag, pos, tag_type);
        }
    
        cur = cur->next;
    }


    g_rand_name_pool = tmp;

    return 0;
}

int load_reward_task_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
    reward_task_conf_mgr_t tmp;
    while (cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"item")) {
            reward_task_conf_t reward_task;
		    decode_xml_prop_uint32_default(&(reward_task.id), cur, "id", 0);
		    decode_xml_prop_uint32_default(&(reward_task.task_id), cur, "task_id", 0);
			if (tmp.is_reward_task_conf_exist(reward_task.task_id)) {
				ERROR_TLOG("Load reward task config[%d]:task_id[%u] illegal", reward_task.id, reward_task.task_id);
				ERR_RT;
			}
            if (!TaskUtils::is_reward_task(reward_task.task_id)) {
                ERROR_TLOG("Load reward task config[%d]: item[%d] task_id[%u] illegal", reward_task.id, reward_task.task_id);    
                ERR_RT;
            }
		    decode_xml_prop_uint32_default(&reward_task.level, cur, "level", 0);
		    decode_xml_prop_uint32_default(&reward_task.attr_id, cur, "attr_id", 0);
		    decode_xml_prop_uint32_default(&reward_task.score, cur, "score", 0);

            if (!g_task_conf_mgr.is_task_conf_exist(reward_task.task_id)) {
                ERROR_TLOG("Load reward task config[%d]: task[%d] not exist", reward_task.id);    
                ERR_RT;
            }
            tmp.add_reward_task_conf(reward_task);
        }
        cur = cur->next;
    }

    g_reward_task_conf_mgr.copy_from(tmp);

	tmp.print_reward_task_info();
    return 0;
}

int load_time_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    std::map< uint32_t,TIME_CONFIG_LIMIT_T > tmp;
    tmp.clear();
    while(cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"time_config")) {
            TIME_CONFIG_LIMIT_T time_config;
            uint32_t id = 0;
            decode_xml_prop_uint32_default(&id, cur, "id", 0);
            if (id == 0){
                ERROR_TLOG("Load time config id[%u] illegal", id);    
                ERR_RT;
            }

            xmlNodePtr sub_node = cur->xmlChildrenNode;
            while(sub_node) {
                time_limit_t time_limit;
                if (!xmlStrcmp(sub_node->name, (const xmlChar *)"time_limit")) {
                    uint32_t tid = 0;
                    decode_xml_prop_uint32_default(&tid, sub_node, "tid", 0);
                    if (tid == 0){
                        ERROR_TLOG("Load time config tid[%u] illegal in id:%u", tid, id);    
                        ERR_RT;
                    }

                    DECODE_XML_PROP_UINT32_DEFAULT(time_limit.multi, sub_node, "multi", 1);

                    char time_str_a[100]={};
                    DECODE_XML_PROP_STR_DEFAULT(time_str_a, sub_node, "start", "");
                    if (strlen(time_str_a) > 0 && 
                            TimeUtils::time_str_to_long(time_str_a, time_limit.start_time) != 0) {
                        ERROR_TLOG("Load time config id[%u] failed, start time illegal, start:%s", id, time_str_a);    
                        ERR_RT;
                    }

                    memset(time_str_a, 0, sizeof(time_str_a));
                    DECODE_XML_PROP_STR_DEFAULT(time_str_a, sub_node, "end", "");
                    if (strlen(time_str_a) > 0 && 
                            TimeUtils::time_str_to_long(time_str_a, time_limit.end_time) != 0) {
                        ERROR_TLOG("Load time config id[%u] failed, end time illegal, end:%s", id, time_str_a);    
                        ERR_RT;
                    }

                    std::string time_str;
                    decode_xml_prop_default<std::string, std::string>(time_str, sub_node, "hour", "");
                    if (time_str.size() > 0) {
                        std::vector<std::string> hour_vec = split(time_str, ',');
                        if (hour_vec.size() != 2) {
                            ERROR_TLOG("Load time config id[%u] failed, hour illegal, hour:%s", id, time_str.c_str());    
                            ERR_RT;
                        }

                        std::vector<std::string> hour_time = split(hour_vec[0], ':');
                        if (hour_time.size() != 3) {
                            ERROR_TLOG("Load time config id[%u] failed, hour illegal, hour:%s", id, time_str.c_str());    
                            ERR_RT;
                        }
                        time_limit.start_hour = atoi(hour_time[0].c_str()); 
                        time_limit.start_min = atoi(hour_time[1].c_str()); 
                        time_limit.start_second = atoi(hour_time[2].c_str()); 

                        if (time_limit.start_hour > 23) {
                            ERROR_TLOG("Load time config id[%u] failed, start_hour illegal, hour:%u", id, time_limit.start_hour);    
                            ERR_RT;

                        }

                        if (time_limit.start_min > 59) {
                            ERROR_TLOG("Load time config id[%u] failed, start_min illegal, min:%u", id, time_limit.start_min);    
                            ERR_RT;

                        }

                        if (time_limit.start_second > 60) {
                            ERROR_TLOG("Load time config id[%u] failed, start_second illegal, hour:%u", id, time_limit.start_second);    
                            ERR_RT;

                        }

                        hour_time.clear();
                        hour_time = split(hour_vec[1], ':');
                        if (hour_time.size() != 3) {
                            ERROR_TLOG("Load time config id[%u] failed, hour illegal, hour:%s", id, time_str.c_str());    
                            ERR_RT;
                        }
                        time_limit.end_hour = atoi(hour_time[0].c_str()); 
                        time_limit.end_min = atoi(hour_time[1].c_str()); 
                        time_limit.end_second = atoi(hour_time[2].c_str());

                        if (time_limit.end_hour > 23) {
                            ERROR_TLOG("Load time config id[%u] failed, end_hour illegal, hour:%u", id, time_limit.end_hour);    
                            ERR_RT;

                        }

                        if (time_limit.end_min > 59) {
                            ERROR_TLOG("Load time config id[%u] failed, end_min illegal, min:%u", id, time_limit.end_min);    
                            ERR_RT;

                        }

                        if (time_limit.end_second > 60) {
                            ERROR_TLOG("Load time config id[%u] failed, end_second illegal, hour:%u", id, time_limit.end_second);    
                            ERR_RT;

                        }
                    }

                    time_str.clear();
                    decode_xml_prop_default<std::string, std::string>(time_str, sub_node, "weekday", "");
                    if (time_str.size() > 0) {
                        std::vector<string> weekday_vec = split(time_str, ',');
                        if (weekday_vec.size() > 7) {
                            ERROR_TLOG("Load time config id[%u] failed, weekday illegal,weekday:%s", id, time_str.c_str());    
                            ERR_RT;
                        }

                        if (weekday_vec.size() == 0) {
                            // 缺省一周7天有效
                            for (uint32_t i = 0; i < 7; i++) {
                                time_limit.weekdays.push_back(i + 1);
                            }
                        } else {
                            FOREACH(weekday_vec, it) {
                                time_limit.weekdays.push_back(atoi((*it).c_str()));
                            }
                        }
                        
						//周日期排序
						sort(time_limit.weekdays.begin(), time_limit.weekdays.end());
                    }

                    time_config.insert(std::pair<uint32_t, time_limit_t>(tid, time_limit));
                }

                sub_node = sub_node->next;
            }
            if (tmp.find(id) != tmp.end()) {
                ERROR_TLOG("Load time config id[%u] repeated", id);    
                ERR_RT;
            }

            tmp.insert(std::pair<uint32_t, TIME_CONFIG_LIMIT_T>(id, time_config));
        }
        cur = cur->next;
    }

    FOREACH(tmp, it) {
        uint32_t id = it->first;
        TRACE_TLOG("load time config:id:%u", id);
        FOREACH(it->second, iter) {
            uint32_t tid = iter->first;
            time_limit_t *time_limit = &(iter->second);
            TRACE_TLOG("load time config:tid:%u, start_time:%u, end_time:%u",
                    tid, time_limit->start_time, time_limit->end_time);
            TRACE_TLOG("start_hour:%u, end_hour:%u",
                    time_limit->start_hour, time_limit->end_hour);
            TRACE_TLOG("start_min:%u, end_min:%u",
                    time_limit->start_min, time_limit->end_min);
            TRACE_TLOG("start_second:%u, end_second:%u, weekdays_num:%u",
                    time_limit->start_second, time_limit->end_second, 
                    time_limit->weekdays.size());
        } 
    }

    g_time_config = tmp;
	//时间管理器
	int ret = g_srv_time_mgr.init();
	if(0 != ret){
		ERROR_TLOG("init g_srv_time_mgr err srv_id=%u time_key = %u", g_server_id, g_srv_time_mgr.get_time_key());    
		ERR_RT;
	}
	TRACE_TLOG("init g_srv_time_mgr suc srv_id=%u time_key = %u" , g_server_id,  g_srv_time_mgr.get_time_key());
    return 0;
}

int load_achieve_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	achieve_mgr_t tmp_achieve;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("achieve"))) {
			cur = cur->next;
			continue;
		}
		achieve_conf_t ach_info;
		DECODE_XML_PROP_UINT32(ach_info.id, cur, "id");
		if (tmp_achieve.is_achieve_info_conf_exist(ach_info.id)) {
			ERROR_TLOG("Achieve id invalid(exsit) in achieve.xml id=[%u]", ach_info.id);
			ERR_RT;
		}
		decode_xml_prop_uint32_default(&ach_info.achieve_type, cur, "achieve_type", 0);
		decode_xml_prop_uint32_default(&ach_info.track_target, cur, "track_target", 0);
		decode_xml_prop_uint32_default(&ach_info.target_2_value, cur, "track_target_2", 0);
		decode_xml_prop_uint32_default(&ach_info.point, cur, "point", 0);
		decode_xml_prop_uint32_default(&ach_info.prize_id, cur, "prize_id", 0);
		if (ach_info.prize_id) {
			if (!g_prize_conf_mgr.is_prize_exist(ach_info.prize_id)) {
				ERROR_TLOG("Achieve id=[%u],not exist prize id=[%u]", 
					ach_info.id, ach_info.prize_id);
				ERR_RT;
			}
		}
		char title_name_str[256] = {0};
		DECODE_XML_PROP_STR_DEFAULT(title_name_str, cur, "title", "");
		if (strlen(title_name_str)) {
			ach_info.title_id = ach_info.id;
		} else {
			ach_info.title_id = 0;
		}
		decode_xml_prop_uint32_default(&ach_info.title_during, cur, "title_during", 0);
		tmp_achieve.add_achieve_conf(ach_info);	
		tmp_achieve.insert_achieve_id(ach_info.achieve_type, ach_info.id);
		cur = cur->next;
	}
	g_achieve_mgr.copy_from(tmp_achieve);
	//g_achieve_mgr.print_achieve_info();
	//g_achieve_mgr.print_achieve_by_type();
	return 0;	
}

int load_server_buff_config(xmlNodePtr root) 
{
	xmlNodePtr cur = root->xmlChildrenNode;
    buff_conf_mgr_t tmp;
    while (cur) {
 		if (!xmlStrEqual(cur->name, (const xmlChar*)("buff"))) {
			cur = cur->next;
			continue;
		}
        buff_conf_t buff_conf;
        DECODE_XML_PROP_UINT32(buff_conf.buff_id, cur, "id");
        if (tmp.is_buff_conf_exist(buff_conf.buff_id)) {
            ERROR_TLOG("Buff %u already exist!", buff_conf.buff_id);
            ERR_RT;
        }

        DECODE_XML_PROP_UINT32_DEFAULT(buff_conf.over_type_id, cur, "over_type", 0);
        if (buff_conf.over_type_id == 0) {
            buff_conf.over_type_id = buff_conf.buff_id;
        }
        if (tmp.is_buff_conf_exist(buff_conf.buff_id)) {
            ERROR_TLOG("Buff %u already exist!", buff_conf.buff_id);
            ERR_RT;
        }

        if (buff_conf.over_type_id != buff_conf.buff_id
            && !tmp.is_buff_conf_exist(buff_conf.over_type_id)) {
            ERROR_TLOG("Buff %u over_type:%u not exist", buff_conf.buff_id, buff_conf.over_type_id);
            ERR_RT;
        }

        for (xmlAttrPtr attr = cur->properties; attr != NULL; attr = attr->next) {
            if (!tmp.is_server_effect_name((const char*)(attr->name))) {
                continue;
            }
            buff_effect_type_t effect_type = tmp.get_effect_type_by_name((const char*)(attr->name));
            if (!tmp.is_server_effect_type((uint32_t)effect_type)) {
                continue;
            }
            buff_effect_t effect;
            effect.effect_type = effect_type;
            char args[128];
            DECODE_XML_PROP_STR(args, cur, attr->name);
            std::vector<string> args_list = split(args, ',');
            FOREACH(args_list, it) {
                double dval = strtod((*it).c_str(), 0);
                int32_t ival = dval * 100;
                effect.args.push_back(ival);
            }
            buff_conf.effects.insert(make_pair((uint32_t)effect_type, effect));
        }
        tmp.add_buff_conf(buff_conf);
        cur = cur->next;
    }
    g_buff_conf_mgr.copy_from(tmp);
    return 0;
}

int load_nick_config(xmlNodePtr root)
{
    typeof(g_rand_nick_pos1[0]) tmp_rand_nick_pos1[2];
    typeof(g_rand_nick_pos2) tmp_rand_nick_pos2;
    typeof(g_rand_nick_pos3) tmp_rand_nick_pos3;

    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)"item")) {
            cur = cur->next;
            continue;
        }
        char tag[64];
        uint32_t pos = 0;
        uint32_t sex = 0;
        DECODE_XML_PROP_STR(tag, cur, "tag");
        DECODE_XML_PROP_UINT32(pos, cur, "pos");

        if (pos == 1) {
            DECODE_XML_PROP_UINT32(sex, cur, "sex");
            if (sex == 0) {
                tmp_rand_nick_pos1[0].push_back(tag); 
            } else if (sex == 1) {
                tmp_rand_nick_pos1[1].push_back(tag); 
            } else if (sex == 2) {
                tmp_rand_nick_pos1[0].push_back(tag); 
                tmp_rand_nick_pos1[1].push_back(tag); 
            }
        } else if (pos == 2) {
            tmp_rand_nick_pos2.push_back(tag); 
        } else {
            tmp_rand_nick_pos3.push_back(tag); 
        }
        TRACE_TLOG("tag = '%s', pos = %d, sex = %d", tag, pos, sex);
        cur = cur->next;
    }
    g_rand_nick_pos1[0] = tmp_rand_nick_pos1[0];
    g_rand_nick_pos1[1] = tmp_rand_nick_pos1[1];
    g_rand_nick_pos2 = tmp_rand_nick_pos2;
    g_rand_nick_pos3 = tmp_rand_nick_pos3;
    return 0;
}

int load_pet_quality_config(xmlNodePtr root)
{
    pet_quality_conf_manager_t tmp;
	xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"type")) {
            uint32_t waken_type = 0;
            DECODE_XML_PROP_UINT32_DEFAULT(waken_type, cur, "waken_type", 0);
            xmlNodePtr sub_cur = cur->xmlChildrenNode;
            while(sub_cur) {
                if (!xmlStrcmp(sub_cur->name, (const xmlChar *)"quality")) {
                    struct pet_quality_conf_t pet_quality_conf;
                    DECODE_XML_PROP_UINT32_DEFAULT(pet_quality_conf.id, sub_cur, "id", 0);
                    if (pet_quality_conf.id == 0) {
                        ERROR_TLOG("pet_quality_config id cannot be 0");
                        ERR_RT;
                    }
                    DECODE_XML_PROP_UINT32_DEFAULT(pet_quality_conf.next_id, sub_cur, "next_id", 0);
                    if (pet_quality_conf.next_id > commonproto::MAX_PET_QUALITY) {
                        ERROR_TLOG("pet_quality_config next_id over limit");
                        ERR_RT;
                    }

                    if (pet_quality_conf.id > commonproto::MAX_PET_QUALITY ||
                            pet_quality_conf.next_id > commonproto::MAX_PET_QUALITY) {
                        ERROR_TLOG("pet_quality_config id or next_id over limit, id:%u", pet_quality_conf.id);
                        ERR_RT;
                    }

                    DECODE_XML_PROP_UINT32_DEFAULT(pet_quality_conf.pet_level, sub_cur, "pet_level", 0);
                    DECODE_XML_PROP_UINT32_DEFAULT(pet_quality_conf.cost_gold, sub_cur, "cost_gold", 0);

                    std::string tmp_str;
                    decode_xml_prop_default<std::string, std::string>(tmp_str, sub_cur, "cost_item", "");
                    std::vector<std::string> item_str = split(tmp_str, ';');
                    FOREACH(item_str, iter) {
                        std::vector<std::string> item_info_str = split(*iter, ',');
                        reduce_item_info_t reduce_item;
                        reduce_item.item_id = atoi(item_info_str[0].c_str()); 
                        reduce_item.count = atoi(item_info_str[1].c_str()); 

                        if (!g_item_conf_mgr.is_item_conf_exist(reduce_item.item_id)) {
                            ERROR_TLOG("pet quality cost item not exist id:%u, item:%u",
                                    pet_quality_conf.id, reduce_item.item_id);
                            ERR_RT;
                        }
                        pet_quality_conf.cost_item.push_back(reduce_item);
                    }

                    bool ret = tmp.add_pet_quality_conf(
                            waken_type, pet_quality_conf.id, pet_quality_conf);
                    if (ret == false) {
                        ERROR_TLOG("pet_quality_config insert failed, repeated config, id:%u", pet_quality_conf.id);
                        ERR_RT;
                    }
                }
                sub_cur = sub_cur->next;
            }
        }
        cur = cur->next;
    }

    g_pet_quality_conf_mgr.copy_from(tmp);
    return 0;
}

int load_family_config(xmlNodePtr root) 
{
    xmlNodePtr cur = root->xmlChildrenNode;
    family_conf_manager_t tmp;
    while(cur) {
        // 解析家族副本boss属性配置
        if (!xmlStrcmp(cur->name, (const xmlChar *)"dup_boss")) {
            family_dup_boss_conf_t boss_conf;
            uint32_t stage_id = 0;
            uint32_t player_lv = 0;
            DECODE_XML_PROP_UINT32_DEFAULT(stage_id, cur, "stage_id", 0);
            if (stage_id == 0 || stage_id > commonproto::MAX_FAMILY_DUP_STAGE_ID) {
                ERROR_TLOG("family dup config stage_id error, stage_id:%u", stage_id);
                ERR_RT;
            }
            DECODE_XML_PROP_UINT32_DEFAULT(player_lv, cur, "player_lv", 0);
            if (player_lv != 0 && player_lv < commonproto::MIN_FAMILY_DUP_ENTER_LEVEL) {
                ERROR_TLOG("family dup config player_lv too low, player_lv:%u", player_lv);
                ERR_RT;
            }
            DECODE_XML_PROP_UINT32_DEFAULT(boss_conf.lv, cur, "boss_lv", 20);
            DECODE_XML_PROP_UINT32_DEFAULT(boss_conf.hp, cur, "boss_hp", 1);
            bool ret = tmp.add_lv_boss_info(stage_id, player_lv, boss_conf);
            if (ret == false) {
                ERROR_TLOG("family dup config insert failed, stage_id:%u, player_lv:%u", 
                        stage_id, player_lv);
                ERR_RT;
            }
        }

        // 解析家族杂项配置
        if (!xmlStrcmp(cur->name, (const xmlChar *)"config")) {
            uint32_t id = 0;
            DECODE_XML_PROP_UINT32(id, cur, "id");
            if (id == 0) {
                ERROR_TLOG("family common_config id error, id:%u", id);
                ERR_RT;
            }

            uint32_t value = 0;
            DECODE_XML_PROP_UINT32(value, cur, "value");

            bool ret = tmp.add_common_config(id,value);
            if (ret == false) {
                ERROR_TLOG("family common config insert failed, id:%u, value:%u", 
                        id, value);
                ERR_RT;
            }
        }

        // 解析家族等级配置
        if (!xmlStrcmp(cur->name, (const xmlChar *)"build")) {
            uint32_t level = 0;
            DECODE_XML_PROP_UINT32(level, cur, "level");
            if (level == 0) {
                ERROR_TLOG("family build config level error, level:%u", level);
                ERR_RT;
            }

            family_level_conf_t level_conf;
            DECODE_XML_PROP_UINT32(level_conf.need_construct_value, cur, "require");
            if (level > 1 && level_conf.need_construct_value == 0) {
                ERROR_TLOG("family build config construct_value error, value:%u", 
                        level_conf.need_construct_value);
                ERR_RT;
            }

            DECODE_XML_PROP_UINT32(level_conf.max_member_num, cur, "member_num");
            if (level_conf.max_member_num == 0) {
                ERROR_TLOG("family build config member_num error, value:%u", 
                        level_conf.max_member_num);
                ERR_RT;
            }

            DECODE_XML_PROP_UINT32(level_conf.max_vice_leader_num, cur, "dupty_num");
            if (level_conf.max_vice_leader_num == 0) {
                ERROR_TLOG("family build config max_vice_leader_num error, value:%u", 
                        level_conf.max_vice_leader_num);
                ERR_RT;
            }

            tmp.add_level_config(level, level_conf);
        }

        // 解析家族技能配置
        if (!xmlStrcmp(cur->name, (const xmlChar *)"technology")) {
            family_tech_conf_t tech_conf;
            DECODE_XML_PROP_UINT32(tech_conf.id, cur, "id");
            if (!(tech_conf.id >= 1 && tech_conf.id <= commonproto::MAX_FAMILY_TECH_ID)) {
                ERROR_TLOG("family tech conf id illegal ,id:%u", tech_conf.id);
                ERR_RT;
            }

            DECODE_XML_PROP_UINT32(tech_conf.attr_id, cur, "attr_id");
            if (!(tech_conf.attr_id >= kAttrHp && tech_conf.attr_id <= kAttrHpMax)) {
                ERROR_TLOG("family tech conf attr_id illegal ,attr_id:%u", tech_conf.id);
                ERR_RT;
            }

            DECODE_XML_PROP_UINT32(tech_conf.base_value, cur, "base_value");

            DECODE_XML_PROP_UINT32(tech_conf.lv_attr, cur, "lv_attr");
            // TODO toby check

            DECODE_XML_PROP_UINT32(tech_conf.coefficient, cur, "coefficient");
            if (tech_conf.coefficient == 0) {
                ERROR_TLOG("family tech conf coefficient illegal, coefficient:%u", 
                        tech_conf.coefficient);
                ERR_RT;
            }

            DECODE_XML_PROP_UINT32_DEFAULT(tech_conf.req, cur, "req", 0);
            tmp.add_tech_config(tech_conf.id, tech_conf);
        }

        // 解析家族贡献配置
        if (!xmlStrcmp(cur->name, (const xmlChar *)"contribute")) {
            family_contribute_conf_t contribute_conf;
            DECODE_XML_PROP_UINT32(contribute_conf.type_id, cur, "type");
            if (!(contribute_conf.type_id >= commonproto::FAMILY_CONSTUCT_TYPE_1 && 
                        contribute_conf.type_id <= commonproto::FAMILY_CONSTUCT_TYPE_3)) {
                ERROR_TLOG("family contribute conf type illegal ,type:%u", contribute_conf.type_id);
                ERR_RT;
            }

            DECODE_XML_PROP_UINT32(contribute_conf.req_add, cur, "req_add");
            DECODE_XML_PROP_UINT32(contribute_conf.contribution, cur, "contribution");
            DECODE_XML_PROP_UINT32(contribute_conf.construct_value, cur, "value");

            std::string tmp_str;
            decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "require", "");
            std::vector<std::string> item_vec = split(tmp_str, ',');
            if (item_vec.size() != 3) {
                ERROR_TLOG("family contribute require conf illegal ,size:%u, need:[type, ids, cnt]", item_vec.size());
                ERR_RT;
            }
            if (item_vec[0] == "item") {
                contribute_conf.cost_item = atoi(item_vec[1].c_str());
            }

            contribute_conf.base_cost_value = atoi(item_vec[2].c_str());

            tmp.add_contribute_config(contribute_conf.type_id, contribute_conf);
        }

        cur = cur->next;
    }

    g_family_conf_mgr.copy_from_stage_config(tmp);
    g_family_conf_mgr.copy_from_common_config(tmp);
    g_family_conf_mgr.copy_from_level_config(tmp);
    g_family_conf_mgr.copy_from_tech_config(tmp);
    g_family_conf_mgr.copy_from_contribute_config(tmp);

    return 0;
}

int load_pet_group_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    pet_group_manager_t tmp;
    while(cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"group")) {
            pet_group_info_t group_conf;
            uint32_t group_id = 0;
            DECODE_XML_PROP_UINT32(group_id, cur, "id");
            group_conf.group_id = group_id;

            uint32_t type = 0;
            DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "activate_type", 1);
            if (!PetUtils::is_valid_pet_group_active_type(type)) {
                ERROR_TLOG("pet group activate_type illegal, id:%u, activate_type:%u", 
                        group_id, group_conf.activate_type);
                ERR_RT;
            }
            group_conf.activate_type = (pet_group_active_type_t)type;
            std::string tmp_str;
            decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "group_pet_ids", "");
            std::vector<std::string> item_vec = split(tmp_str, ',');
            std::vector<uint32_t> pet_vec;
            FOREACH(item_vec, iter) {
                group_conf.pet_ids.push_back(atoi((*iter).c_str()));
            }

            if (group_conf.pet_ids.size() == 0) {
                ERROR_TLOG("pet group need pet ids, id:%u", group_id);
                ERR_RT;
            }

            tmp_str.clear();
            decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "group_effect", "");
            item_vec.clear();
            item_vec = split(tmp_str, ';');
            FOREACH(item_vec, iter) {
                std::vector<std::string> sub_vec = split(*iter, ',');
                if (sub_vec.size() != 3) {
                    ERROR_TLOG("pet group effect illegal ,lack config[id,value], id:%u", group_id);
                    ERR_RT;
                }
                group_add_attr_t add_attr;
                add_attr.type = atoi(sub_vec[0].c_str());
                add_attr.basic_value = atoi(sub_vec[1].c_str());
                add_attr.max_value = atoi(sub_vec[2].c_str());
                if (!(add_attr.type >= 1 && add_attr.type <= 11)) {
                    ERROR_TLOG("pet group effect type over range ,config[id:%u effct_type:%u", 
                            group_id, add_attr.type);
                    ERR_RT;
                }
#if 0
                if (add_attr.basic_value > add_attr.max_value) {
                    ERROR_TLOG("pet group effect basic > max_value config[id:%u, effect_type:%u]",
                            group_id, add_attr.type);
                }
#endif
                group_conf.effects.push_back(add_attr);
            }
            tmp.add_pet_group_conf(group_id, group_conf);
        }
        cur = cur->next;
    }

    g_pet_group_mgr.copy_from(tmp);

    return 0;
}
// 加载全服属性值配置
int load_global_attr_config(xmlNodePtr root)
{
    // g_global_attr_configs.clear();
	std::map<uint32_t, global_attr_config_t> tmp;
 
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("global_attr"))) {
            cur = cur->next;
            continue;
        }

        global_attr_config_t global_attr_config;
        DECODE_XML_PROP_UINT32(global_attr_config.id, cur, "id"); 
        DECODE_XML_PROP_UINT32(global_attr_config.time_id, cur, "time_id"); 
		DECODE_XML_PROP_UINT32_DEFAULT(global_attr_config.target_id, cur, "target_id", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(global_attr_config.max, cur, "max", 0);
		if (global_attr_config.id < 100000) {
			ERROR_TLOG("global_attr_config id illegal,id:%u",global_attr_config.id);
            ERR_RT;
		}

		if (global_attr_config.max < 0) {
			ERROR_TLOG("global_attr_config max illegal,id:%u",global_attr_config.max);
            ERR_RT;
		}

        tmp[global_attr_config.id] = global_attr_config;

        cur = cur->next;
    }
	g_global_attr_configs = tmp;
    return 0;
}

int  load_exped_info(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	exped_conf_mgr_t  tmp_exped;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("pet"))) {
			cur = cur->next;
			continue;
		}
		exped_conf_t  exped_info;
		DECODE_XML_PROP_UINT32(exped_info.card_id, cur, "id");
		if (tmp_exped.is_exped_conf_exist(exped_info.card_id)) {
			ERROR_TLOG("Exped id dupl,id=[%u]", exped_info.card_id);
			ERR_RT;
		}
		DECODE_XML_PROP_UINT32(exped_info.power_percent, cur, "value");
		DECODE_XML_PROP_UINT32_DEFAULT(exped_info.prize_id, cur, "prize_id", 0);
		if (!g_prize_conf_mgr.is_prize_exist(exped_info.prize_id)) {
			ERROR_TLOG("Exped Not Exist This Prize,prize_id=[%u]", 
					exped_info.prize_id);
			ERR_RT;
		}
		tmp_exped.add_exped_conf(exped_info);
		cur = cur->next;
	}
	g_exped_mgr.copy_from(tmp_exped);
	g_exped_mgr.print_exped_info();
	return 0;
}

int load_cultivate_equip_level_info(
        xmlNodePtr root, cultivate_equip_conf_t &cult_conf) 
{
	xmlNodePtr cur = root->xmlChildrenNode;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("equip"))) {
			cur = cur->next;
			continue;
		}

        cultivate_equip_level_info_t level_info;

		DECODE_XML_PROP_UINT32(level_info.level, cur, "level");
		DECODE_XML_PROP_UINT32_DEFAULT(
                level_info.need_cultivate_value, cur, "need_cultivate_value", 1);

        if (level_info.level > 1) {
            std::map<uint32_t, cultivate_equip_level_info_t>::iterator iter = 
                cult_conf.equip_info_map_.find(level_info.level - 1);
            if (iter != cult_conf.equip_info_map_.end()) {
                level_info.add_attrs_vec_ =  iter->second.add_attrs_vec_;
            }
        }

        std::string add_attr_str;
        decode_xml_prop_default<std::string, std::string>(add_attr_str, cur, "add_attr_val_list", "");
        std::vector<std::string> add_attr_list = split(add_attr_str, ';');
        FOREACH(add_attr_list,it) {
            std::vector<std::string> add_attr_vec = split(*it, ',');
            if(add_attr_vec.size() != 2) {
                ERROR_TLOG("add_attr config failed, type:%u", cult_conf.type);
                ERR_RT;
            }
            add_attr_value_t add_attr;
            add_attr.type       = atoi(add_attr_vec[0].c_str());
            add_attr.value = atoi(add_attr_vec[1].c_str());
            if (!(add_attr.type >= kAttrHp && add_attr.type <= kAttrHpMax)) {
                ERROR_TLOG("add_attr over limit, type:%u, attr_type:%u", cult_conf.type, add_attr.type);
                ERR_RT;
            }
            bool insert_flag = true;
            FOREACH(level_info.add_attrs_vec_, iter) {
                if ((*iter).type == add_attr.type) {
                    (*iter).value += add_attr.value;
                    insert_flag = false;
                    break;
                }
            }
            if (insert_flag) {
                level_info.add_attrs_vec_.push_back(add_attr);
            }
        }

        std::string step_prob_str;
        decode_xml_prop_default<std::string, std::string>(step_prob_str, cur, "step_prob", "");
        std::vector<std::string> step_prob_list = split(step_prob_str, ';');
        FOREACH(step_prob_list,it) {
            std::vector<std::string> step_prob_vec = split(*it, ',');
            if(step_prob_vec.size() != 2) {
                ERROR_TLOG("step_prob config faield, type:%u, level:%u", 
                        cult_conf.type, level_info.level);
                ERR_RT;
            }

            uint32_t percent = atoi(step_prob_vec[0].c_str());
            uint32_t prob = atoi(step_prob_vec[1].c_str());
            level_info.step_probs_map_.insert(
                    std::pair<uint32_t, uint32_t>(percent, prob));
        }

        std::string tmp_str;
        decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "req_item_id", "");
        std::vector<std::string> item_list = split(tmp_str, ';');

        tmp_str.clear();
        decode_xml_prop_default<std::string, std::string>(tmp_str, cur, "req_item_num", "");
        std::vector<std::string> item_num_list = split(tmp_str, ';');
        if (item_list.size() != item_num_list.size()) {
            ERROR_TLOG("cost item config illegal, id len:%u, num len:%u", 
                    item_list.size(), item_num_list.size());
            ERR_RT;
        }

        for (uint32_t i = 0; i < item_list.size();i++) {
            cost_item_t item;
            item.item_id = atoi(item_list[i].c_str());
            item.num = atoi(item_num_list[i].c_str());
            level_info.cost_item_vec_.push_back(item);
        }

        DECODE_XML_PROP_UINT32_DEFAULT(level_info.lv_reward_item, cur, "lv_reward_item", 0);
        //配了奖励才判断奖励物品是否存在
        if (level_info.lv_reward_item && !g_item_conf_mgr.is_item_conf_exist(level_info.lv_reward_item)) {
            ERROR_TLOG("cultivate type:%u level:%u, lv_reward_item not exist, item_id:%u", 
                    cult_conf.type, level_info.level, level_info.lv_reward_item);
            ERR_RT;
        }


        if(cult_conf.equip_info_map_.count(level_info.level) > 0) {
            ERROR_TLOG("repeated level config, type:%u, level:%u", 
                    cult_conf.type, level_info.level);
            ERR_RT;
        } else {
            cult_conf.equip_info_map_.insert(
                    std::pair<uint32_t, cultivate_equip_level_info_t>(
                        level_info.level, level_info));
        }
		cur = cur->next;
    }

    return 0;
}

int  load_cultivate_equip_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
    cultivate_equip_conf_manager_t tmp;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("cultivate_equip"))) {
			cur = cur->next;
			continue;
		}
		cultivate_equip_conf_t  cultivate_equip_info;
		DECODE_XML_PROP_UINT32(cultivate_equip_info.type, cur, "type");
		if (tmp.is_cultivate_equip_conf_exist(
                    cultivate_equip_info.type)) {
			ERROR_TLOG("repeated cultivate type, type:%u", cultivate_equip_info.type);
			ERR_RT;
		}

		DECODE_XML_PROP_UINT32_DEFAULT(cultivate_equip_info.init_item, cur, "init_item", 0);
        if (!(cultivate_equip_info.init_item == 0 ||
                    g_item_conf_mgr.is_item_conf_exist(cultivate_equip_info.init_item))) {
            ERROR_TLOG("init item id:%u not exist", cultivate_equip_info.init_item);
            ERR_RT;
        }

		DECODE_XML_PROP_UINT32_DEFAULT(cultivate_equip_info.init_level, cur, "init_level", 0);
		DECODE_XML_PROP_UINT32_DEFAULT(cultivate_equip_info.per_level_score, cur, "per_level_score", 0);
		DECODE_XML_PROP_UINT32_DEFAULT(cultivate_equip_info.default_buff_id, cur, "default_buff_id", 0);
        if (cultivate_equip_info.default_buff_id == 0) {
            ERROR_TLOG("cultivate item default_buff_id:%u not exist,  type:%u", 
                    cultivate_equip_info.default_buff_id, cultivate_equip_info.type);
			ERR_RT;
        }

		DECODE_XML_PROP_UINT32_DEFAULT(cultivate_equip_info.svip_trial_id, cur, "svip_trial_id", 0);
        
        int ret = load_cultivate_equip_level_info(cur, cultivate_equip_info);
        if (ret) {
            ERR_RT;
        }
        cultivate_equip_info.max_level = cultivate_equip_info.equip_info_map_.size();

	    if(!tmp.add_cultivate_equip_conf(cultivate_equip_info)) {
            ERROR_TLOG("add cultivate equip failed, type:%u", cultivate_equip_info.type);
			ERR_RT;
        }

		cur = cur->next;
	}

    g_cultivate_equip_mgr.copy_from(tmp);
    //g_cultivate_equip_mgr.print_cultivate_equip_info();
	return 0;
}

int  load_dup_area_prize_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
    dup_area_prize_conf_manager_t tmp;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("area_id"))) {
			cur = cur->next;
			continue;
		}
        
        uint32_t area_id = 0;
		DECODE_XML_PROP_UINT32(area_id, cur, "id");
        if (area_id == 0) {
            ERROR_TLOG("dup_area_prize area_id illegal, area_id:%u", area_id);
            ERR_RT;
        }

        uint32_t star_num = 0;
		DECODE_XML_PROP_UINT32_DEFAULT(star_num, cur, "star_num", 0);
        if (star_num == 0) {
            ERROR_TLOG("dup_area_prize star_num illegal, star_num:%u", star_num);
            ERR_RT;
        }

        uint32_t prize_id = 0;
		DECODE_XML_PROP_UINT32_DEFAULT(prize_id, cur, "prize_id", 0);
        if (prize_id == 0) {
            ERROR_TLOG("dup_area_prize prize_id illegal, prize_id:%u", prize_id);
            ERR_RT;
        }

        dup_area_prize_t *prize_info = 
            tmp.get_dup_area_prize_conf(area_id);
        if (prize_info == NULL){
            dup_area_prize_t tmp_info;
            tmp_info.insert(std::pair<uint32_t, uint32_t>(star_num, prize_id));
            tmp.add_dup_area_prize_conf(area_id, tmp_info);
        } else {
            if (prize_info->count(star_num) != 0) {
                ERROR_TLOG("dup_area_prize prize_id illegal, area_id:%u, prize_id:%u", 
                        area_id, prize_id);
                ERR_RT;
            }
            prize_info->insert(std::pair<uint32_t, uint32_t>(star_num, prize_id));
        }

		cur = cur->next;
	}

    g_dup_area_prize_conf_mgr.copy_from(tmp);
	return 0;
}

int load_topest_power_player_info_config(xmlNodePtr root)
{
	player_power_rank_conf_mgr_t tmp;
	xmlNodePtr cur = root->xmlChildrenNode;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("player_info"))) {
			cur = cur->next;
			continue;
		}
		player_power_rank_conf_t rank_info;
		DECODE_XML_PROP_UINT32(rank_info.userid, cur, "userid");
		if (!is_valid_uid(rank_info.userid)) {
			ERROR_TLOG("Uid:%u Not Valid Id", rank_info.userid);
			ERR_RT;
		}
		DECODE_XML_PROP_UINT32(rank_info.power_rank, cur, "power_rank");
		if (!(rank_info.power_rank >= 1 && rank_info.power_rank <= 10)) {
			ERROR_TLOG("Uid:%u, power_rank Not In Topest 10,Rank:%u",
					rank_info.userid, rank_info.power_rank);
			ERR_RT;
		}
		DECODE_XML_PROP_UINT32(rank_info.power_value, cur, "power_value");
		DECODE_XML_PROP_UINT32(rank_info.test_stage, cur, "test_stage");
		if (!(rank_info.test_stage >= 1 && rank_info.test_stage <= 2)) {
			ERROR_TLOG("Uid:%u, Test Stage Not Valid, state:%u",
				rank_info.userid, rank_info.test_stage);
			ERR_RT;
		}
		DECODE_XML_PROP_UINT32(rank_info.prize_id, cur, "prize_id");
		if (!g_prize_conf_mgr.is_prize_exist(rank_info.prize_id)) {
			ERROR_TLOG("Uid:%u, Not Exist This Prize,prize_id=[%u]", 
					rank_info.userid, rank_info.prize_id);
			ERR_RT;
		}
		tmp.add_power_rank_conf(rank_info);
		cur = cur->next;
	}
	g_ply_power_rank_conf_mgr.copy_from(tmp);
	g_ply_power_rank_conf_mgr.print_power_rank_info();
	return 0;
}

int load_first_test_uid_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	joined_test_userid_conf_mgr_t  tmp;
	tmp.clear_first_uids();

	//g_joined_test_uid_conf_mgr.clear_first_uids();
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("item"))) {
			cur = cur->next;
			continue;
		}
		uint32_t userid = 0;
		DECODE_XML_PROP_UINT32(userid, cur, "userid");
		tmp.add_first_test_uid(userid);

		//g_joined_test_uid_conf_mgr.add_first_test_uid(userid);
		cur = cur->next;
	}
	g_joined_test_uid_conf_mgr.copy_first_from(tmp);	
	g_joined_test_uid_conf_mgr.print_1st_test_uid_info();
	return 0;
}

int load_second_test_uid_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	joined_test_userid_conf_mgr_t tmp;
	tmp.clear_second_uids();

	//g_joined_test_uid_conf_mgr.clear_second_uids();
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("item"))) {
			cur = cur->next;
			continue;
		}
		uint32_t userid = 0;
		DECODE_XML_PROP_UINT32(userid, cur, "userid");

		tmp.add_second_test_uid(userid);

		//g_joined_test_uid_conf_mgr.add_second_test_uid(userid);
		cur = cur->next;
	}
	g_joined_test_uid_conf_mgr.copy_second_from(tmp);
	g_joined_test_uid_conf_mgr.print_2nd_test_uid_info();
	return 0;
}

int load_question_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	question_conf_manager_t tmp;
	tmp.clear();

	while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("question"))) {
            cur = cur->next;
            continue;
        }
		question_conf_t question_node;

		uint32_t id = 0;
		DECODE_XML_PROP_UINT32_DEFAULT(id, cur, "id", 0);
		if (id == 0){
			ERROR_TLOG("Load question config id[%u] illegal", id);    
			ERR_RT;
		}

		uint32_t count = 0;
		decode_xml_prop_uint32_default(&count, cur, "count", 0);
		if (count == 0){
			ERROR_TLOG("Load question config count[%u] illegal", count);    
			ERR_RT;
		}

		std::string correct_str;
		std::vector<uint32_t> vec_tmp;
		vec_tmp.clear();
		decode_xml_prop_default<std::string, std::string>(correct_str, cur, "correct", "");
		if (correct_str.size() > 0) {
			std::vector<std::string>correct_vec = split(correct_str, ',');
			if (correct_vec.size() < 1) {
				ERROR_TLOG("Load question config correct[%u] failed, correct illegal, correct:%s", id, correct_str.c_str());    
				ERR_RT;
			} 
			FOREACH(correct_vec, it){
				uint32_t tmp_id = 0;
				try{
					tmp_id = boost::lexical_cast<uint32_t> (*it);
				}
				catch(boost::bad_lexical_cast & e){
					ERROR_TLOG("Load question config id[%u] failed, correct illegal, correct:%u, err %s", id, tmp_id, e.what());    
					ERR_RT;
				}
				vec_tmp.push_back(tmp_id);
			}

		} else {
			ERROR_TLOG("Load question config correct[%u] failed, correct illegal, correct:%s", id, correct_str.c_str());    
			ERR_RT;
		}

		question_node.id = id;
		question_node.answer_cnt = count;
		question_node.correct_vec = vec_tmp;

		if (tmp.question_conf_exist(id)) {
			ERROR_TLOG("Load question config id[%u] repeated", id);    
			ERR_RT;
		}

		tmp.add_question_conf(question_node);

		cur = cur->next;
	}
	g_question_conf_mgr.copy_from(tmp);
	return 0;
}

int load_title_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	title_conf_mgr_t tmp;	
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("title"))) {
			cur = cur->next;
			continue;
		}
		title_conf_t title_conf;
		DECODE_XML_PROP_UINT32(title_conf.title_id, cur, "id");
		if (tmp.is_title_conf_exist(title_conf.title_id)) {
			ERROR_TLOG("title conf exist:title_id=[%u]", title_conf.title_id);
			ERR_RT;
		}
		/*
		char buf[255];
		DECODE_XML_PROP_STR(buf, cur, "add_attr");
		std::vector<std::string> attr_list = split(buf, ';');
		FOREACH(attr_list, it) {
			std::vector<std::string> single_attr = split(*it, ',');
			if (single_attr.size() != 2) {
				ERROR_TLOG("Parse Single attr Err;Cnt Must be 2:%u", single_attr.size());
				ERR_RT;
			}
			uint32_t attr_id = boost::lexical_cast<uint32_t>(single_attr[0]);
			uint32_t attr_val = boost::lexical_cast<uint32_t>(single_attr[1]);
			title_conf.add_attr_map.insert(make_pair(attr_id, attr_val));	
		}
		*/
		DECODE_XML_PROP_UINT32_DEFAULT(title_conf.end_type, cur, "title_end_type", 1);
		if (!(title_conf.end_type >= 1 && title_conf.end_type <= 4)) {
			ERROR_TLOG("Load title_id[%u] failed,end_type=[%u]",
					title_conf.title_id, title_conf.end_type);
			ERR_RT;
		}
		if (title_conf.end_type == 2) {
			char time_str[100]={};
			DECODE_XML_PROP_STR_DEFAULT(time_str, cur, "title_end_param", "");
			if (strlen(time_str) > 0 && 
					TimeUtils::time_str_to_long(time_str, title_conf.end_param) != 0) {
				ERROR_TLOG("Load title_id[%u] failed, end_param illegal, :%s", 
						title_conf.title_id, time_str);    
				ERR_RT;
			}
		} else if (title_conf.end_type == 3) {
			uint32_t duration_time = 0;
			DECODE_XML_PROP_UINT32_DEFAULT(duration_time, cur, "title_end_param", 0);
			title_conf.end_param = duration_time * DAY_SECS;
		}
		DECODE_XML_PROP_UINT32_DEFAULT(title_conf.end_type, cur, "title_end_type", 1);
		tmp.add_title_conf(title_conf);
		cur = cur->next;
	}
	g_title_conf_mgr.copy_from(tmp);
	g_title_conf_mgr.print_title_conf();
	return 0;
}

int load_bless_pet_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	bless_pet_conf_manager_t tmp;
	tmp.clear();

	bless_pet_conf_t bless_conf;

	while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("bless"))) {
            cur = cur->next;
            continue;
        }

		bless_conf.clear();
		uint32_t id = 0;
		DECODE_XML_PROP_UINT32_DEFAULT(id, cur, "id", 0);
		if (id == 0){
			ERROR_TLOG("Load bless pet config id[%u] illegal", id);    
			ERR_RT;
		}

		uint32_t petid = 0;
		DECODE_XML_PROP_UINT32_DEFAULT(petid, cur, "pet_id", 0);
		if (petid == 0){
			ERROR_TLOG("Load bless pet config count[%u] illegal", petid);    
			ERR_RT;
		}

		uint32_t dupid = 0;
		DECODE_XML_PROP_UINT32_DEFAULT(dupid, cur, "dup_id", 0);
		if (dupid == 0){
			ERROR_TLOG("Load bless pet config count[%u] illegal", dupid);    
			ERR_RT;
		}

		bless_conf.bless_id = id;
		bless_conf.pet_id   = petid;
		bless_conf.dup_id = dupid;

		if (tmp.bless_pet_conf_exist(id)) {
			ERROR_TLOG("Load bless pet config id[%u] repeated", id);    
			ERR_RT;
		}

		tmp.add_bless_pet_conf(bless_conf);

		cur = cur->next;
	}

    g_bless_pet_conf_mgr.copy_from(tmp);
    return 0;
}

int load_rpvp_reward_config(xmlNodePtr root)
{
    rpvp_reward_mgr_t tmp;
	xmlNodePtr cur = root->xmlChildrenNode;
	tmp.clear();

    rpvp_score_reward_conf_t score_conf;
    rpvp_rank_reward_conf_t rank_conf;
	while (cur) {

        if (xmlStrEqual(cur->name, (const xmlChar *)("reward"))) {
            DECODE_XML_PROP_UINT32_DEFAULT(score_conf.id, cur, "id", 0);
            if (tmp.is_rpvp_score_reward_conf_exist(score_conf.id)) {
                ERROR_TLOG("RPVP score reward conf:%u already exist", score_conf.id);
                ERR_RT;
            }
            DECODE_XML_PROP_UINT32(score_conf.min, cur, "credit_min");
            DECODE_XML_PROP_UINT32(score_conf.max, cur, "credit_max");
            DECODE_XML_PROP_UINT32(score_conf.prize_id, cur, "prize_id");
            if (score_conf.max <= score_conf.min) {
                ERROR_TLOG("RPVP score reward conf:%u max[%u] <= min[%u]", score_conf.id,
                        score_conf.max, score_conf.min);
                ERR_RT;
            }
            if (!g_prize_conf_mgr.is_prize_exist(score_conf.prize_id)) {
                ERROR_TLOG("RPVP score reward conf:%u prize_id:%u not exist", score_conf.id, score_conf.prize_id);
                ERR_RT;
            }
            tmp.add_rpvp_score_reward_conf(score_conf);

        } else if (xmlStrEqual(cur->name, (const xmlChar *)("pvpranking"))) {
            DECODE_XML_PROP_UINT32_DEFAULT(rank_conf.id, cur, "id", 0);
            if (tmp.is_rpvp_rank_reward_conf_exist(rank_conf.id)) {
                ERROR_TLOG("RPVP rank reward conf:%u already exist", rank_conf.id);
                ERR_RT;
            }
            DECODE_XML_PROP_UINT32(rank_conf.min, cur, "start_rank");
            DECODE_XML_PROP_UINT32(rank_conf.max, cur, "end_rank");
            DECODE_XML_PROP_UINT32(rank_conf.prize_id, cur, "prize_id");
            if (rank_conf.max < rank_conf.min) {
                ERROR_TLOG("RPVP rank reward conf:%u max[%u] < min[%u]", rank_conf.id,
                        rank_conf.max, rank_conf.min);
                ERR_RT;
            }
            if (!g_prize_conf_mgr.is_prize_exist(rank_conf.prize_id)) {
                ERROR_TLOG("RPVP rank reward conf:%u prize_id:%u not exist", rank_conf.id, rank_conf.prize_id);
                ERR_RT;
            }
            tmp.add_rpvp_rank_reward_conf(rank_conf);
        }

        cur = cur->next;
    }
    g_rpvp_reward_conf_mgr.copy_from(tmp);
    return 0;
}

int load_equip_buff_config(xmlNodePtr root)
{
    equip_buff_rand_manager_t tmp_mgr;
	xmlNodePtr cur = root->xmlChildrenNode;
    equip_buff_rand_group_t tmp_ebrg;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar*)("equip"))) {
            cur = cur->next;
            continue;
        }
        uint32_t group_id = 0;
        DECODE_XML_PROP_UINT32_DEFAULT(group_id, cur, "id", 0);
        if (group_id == 0) {
            ERROR_TLOG("Invalid EquipBuffRandGroupID: %u", group_id);
            ERR_RT;
        }

        bool new_group = false;
        equip_buff_rand_group_t *ebrg = tmp_mgr.find_mutable_equip_buff_rand_group(group_id);
        if (!ebrg) {
            ebrg = &tmp_ebrg;
            ebrg->group_id = group_id;
            ebrg->equip_buff_conf_vec.clear();
            new_group = true;
        }

        equip_buff_conf_t ebc;
        DECODE_XML_PROP_UINT32_DEFAULT(ebc.elem_id, cur, "elem_id", 0);

        uint32_t type;
        DECODE_XML_PROP_UINT32_DEFAULT(type, cur, "type", 0);
        if (type == 0 || type >= equip_buff_elem_type_end) {
            ERROR_TLOG("Invalid EquipBuffRandElemType:group_id:%u elem_id:%u type:%u",
                    ebrg->group_id, ebc.elem_id, type);
            ERR_RT;
        }
        ebc.type = (equip_buff_elem_type_t)type;

        switch(ebc.type) {
            case equip_buff_elem_type_attr:
                DECODE_XML_PROP_UINT32_DEFAULT(ebc.target_id, cur, "attr_id", 0);
                // if (g_attr_configs.count(ebc.target_id) == 0) {
				if (!item_conf_mgr_t::is_valid_equip_add_attr(ebc.target_id)) {
                    ERROR_TLOG("EquipBuff AttrID:%u not eixst, group_id:%u elem_id:%u",
                            ebc.target_id, ebrg->group_id, ebc.elem_id);
                    ERR_RT;
                }
                break;
            case equip_buff_elem_type_buff:
                DECODE_XML_PROP_UINT32_DEFAULT(ebc.target_id, cur, "buff_id", 0);
                if (g_client_buff_set.count(ebc.target_id) == 0) {
                    ERROR_TLOG("EquipBuff BuffID:%u not exist, group_id:%u elem_id:%u",
                            ebc.target_id, ebrg->group_id, ebc.elem_id);
                    ERR_RT;
                }
                break;
            default:
                break;
        }
        DECODE_XML_PROP_UINT32_DEFAULT(ebc.weight, cur, "award_rate", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(ebc.min, cur, "min", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(ebc.max, cur, "max", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(ebc.method, cur, "method", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(ebc.magic_power, cur, "magic_power", 1);

        if (ebc.max < ebc.min) {
            ERROR_TLOG("EquipBuffRandElem: Max < Min group_id:%u elem_id:%u",
                    ebrg->group_id, ebc.elem_id);
            ERR_RT;
        }
        ebrg->equip_buff_conf_vec.push_back(ebc);
        if (new_group) {
            tmp_mgr.add_equip_buff_rand_group(*ebrg);
        }

        cur = cur->next;
    }

    g_equip_buff_rand_mgr.copy_from(tmp_mgr);
    g_equip_buff_rand_mgr.show();
    return 0;   
}

int load_client_buff_config(xmlNodePtr root)
{
    std::set<uint32_t> tmp;
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar*)("buffdef"))) {
            cur = cur->next;
            continue;
        }
        uint32_t buff_id;
        DECODE_XML_PROP_UINT32_DEFAULT(buff_id, cur, "id", 0);
        tmp.insert(buff_id);
        cur = cur->next;
    }
    g_client_buff_set = tmp;
    return 0;
}

int load_pet_pass_dup_config(xmlNodePtr root)
{
	pet_pass_dup_conf_mgr_t tmp;
	xmlNodePtr cur = root->xmlChildrenNode;
	while (cur) {
		if (!xmlStrEqual(cur->name, (const xmlChar*)("pet"))) {
			cur = cur->next;
			continue;
		}
		pet_pass_dup_conf_t pet_dup_info;
		DECODE_XML_PROP_UINT32_DEFAULT(pet_dup_info.pet_id, cur, "pet_id", 0);
		if (tmp.is_pet_pass_dup_conf_exist(pet_dup_info.pet_id)) {
			ERROR_TLOG("Pet Pass Dup: Duplicate pet_id=%u", pet_dup_info.pet_id);
			ERR_RT;
		}
		if (!g_pet_conf_mgr.pet_conf_exist(pet_dup_info.pet_id)) {
			ERROR_TLOG("Pet Pass Dup: Can Not Found This Pet_id:%u In pet.xml",
					pet_dup_info.pet_id);
			ERR_RT;
		}
		xmlNodePtr sub_cur = cur->xmlChildrenNode;
		while (sub_cur) {
			if (!xmlStrEqual(sub_cur->name, (const xmlChar*)("pass_dup"))) {
				sub_cur = sub_cur->next;
				continue;
			}
			uint32_t activity_type;
			DECODE_XML_PROP_UINT32_DEFAULT(activity_type, sub_cur, "activity_type", 0);
			if (pet_dup_info.dup_shopid_map.count(activity_type) > 0) {
				ERROR_TLOG("Pet Pass Dup: Duplicate :pet_id=%u,activity_type=%u",
						pet_dup_info.pet_id, activity_type);
				ERR_RT;
			}
			pet_pass_dup_activity_type_t type = (pet_pass_dup_activity_type_t)activity_type;
			if (!(type > PET_PASS_DUP_ACTIVITY_TYPE_START &&
						type < PET_PASS_DUP_ACTIVITY_TYPE_END)) {
				ERROR_TLOG("Pet Pass Dup: activity type err:%u", activity_type);
				ERR_RT;
			}
			char pass_dups_str[256] = {0};
			DECODE_XML_PROP_STR_DEFAULT(pass_dups_str, sub_cur, "dup_shop_id_list", "");
			std::vector<std::string> pass_dups_list = split(pass_dups_str, ';');
			std::vector<dup_shop_id_t> dup_shopid_vec;
			FOREACH(pass_dups_list, it) {
				std::vector<std::string> tmp_vec = split((*it).c_str(), ',');
				if (tmp_vec.size() != 2) {
					ERROR_TLOG("Pet Pass Dup:dup_shop_id format err,pet_id:%u;"
							"type[%u],dup_shop_id[%s]",
							pet_dup_info.pet_id, (*it).c_str(), activity_type);
					ERR_RT;
				}
				uint32_t dup_id = atoi(tmp_vec[0].c_str());
				uint32_t shop_id = atoi(tmp_vec[1].c_str());
				if (!g_duplicate_conf_mgr.duplicate_exist(dup_id)) {
					ERROR_TLOG("Pet Pass Dup:dup not exist,pet_id:%u;"
							"type[%u],dup_id[%u]",
							pet_dup_info.pet_id, activity_type, dup_id);
					ERR_RT;
				}
				if (!g_product_mgr.is_product_exist(shop_id)) {
					ERROR_TLOG("Pet Pass Dup:Shop Id Not Exist,pet_id:%u;"
							"type[%u],shop_id[%u]",
							pet_dup_info.pet_id, activity_type, shop_id);
					ERR_RT;
				}
				dup_shop_id_t ds_info;
				ds_info.dup_id = dup_id;
				ds_info.shop_id = shop_id;
				dup_shopid_vec.push_back(ds_info);
			}
			pet_dup_info.dup_shopid_map.insert(make_pair(activity_type, dup_shopid_vec));

			uint32_t unlock_state = 0;
			DECODE_XML_PROP_UINT32_DEFAULT(unlock_state, sub_cur, "need_unlock", 0);
			pet_dup_info.unlock_state_map.insert(make_pair(activity_type, unlock_state));
			uint32_t base_shop_id = 0;
			DECODE_XML_PROP_UINT32_DEFAULT(base_shop_id, sub_cur, "base_shop_id", 0);
			pet_dup_info.base_shopid_map.insert(make_pair(activity_type, base_shop_id));
			sub_cur = sub_cur->next;
		}
		tmp.add_pet_pass_dup_conf(pet_dup_info);
        cur = cur->next;
	}
	g_pet_pass_dup_conf_mgr.copy_from(tmp);		
	g_pet_pass_dup_conf_mgr.print_pet_pass_dup_info();
	return 0;
}

int load_suit_config(xmlNodePtr root)
{
	xmlNodePtr cur = root->xmlChildrenNode;
	suit_conf_manager_t tmp;
	tmp.clear();

	while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("suit"))) {
            cur = cur->next;
            continue;
        }
		suit_conf_t suit_node;
		suit_node.clear();

		uint32_t id = 0;
		DECODE_XML_PROP_UINT32_DEFAULT(id, cur, "id", 0);
		if (id == 0){
			ERROR_TLOG("Load question config id[%u] illegal", id);    
			ERR_RT;
		}

		std::string equip_ids;
		// std::vector<uint32_t> vec_tmp;
		// vec_tmp.clear();
        decode_xml_prop_default<std::string, std::string>(equip_ids, cur, "part_ids", "");
		if (equip_ids.size() > 0) {
			std::vector<std::string>equip_vec = split(equip_ids, ',');
			if (equip_vec.size() < 1) {
				ERROR_TLOG("Load suit config part_ids[%u] failed, equip_vec illegal, equip_vec:%s", id, equip_vec.size());    
				ERR_RT;
			} 
			FOREACH(equip_vec, it){
				uint32_t tmp_id = 0;
				try{
					tmp_id = boost::lexical_cast<uint32_t> (*it);
					tmp.add_equip_suit_map(tmp_id, id);
				}
				catch(boost::bad_lexical_cast & e){
					ERROR_TLOG("Load suit config id[%u] failed,  equip_vec illegal, tmp_id:%u, err %s", id, tmp_id, e.what());    
					ERR_RT;
				}
				// vec_tmp.push_back(tmp_id);
			}

		} else {
			ERROR_TLOG("Load  suit config id [%u] failed, equip vec illegal, correct:%s", id, equip_ids.c_str());    
			ERR_RT;
		}

		std::string trigger_nums; 
		std::string buff_ids; 
		std::vector<uint32_t> vec_nums;
		std::vector<uint32_t> vec_ids;
		vec_nums.clear();
		vec_ids.clear();
        decode_xml_prop_default<std::string, std::string>(trigger_nums, cur, "trigger_nums", "");
        decode_xml_prop_default<std::string, std::string>(buff_ids, cur, "effect_ids", "");
		if(trigger_nums.size() < 0 || buff_ids.size() < 0){

			ERROR_TLOG("Load  suit config id [%u] failed, equip vec illegal, nums size:%d, ids size: %d", id, trigger_nums.size(), buff_ids.size());    
			ERR_RT;
		}

		std::vector<std::string> nums_tmp = split(trigger_nums, ',');
		std::vector<std::string> ids_tmp  = split(buff_ids, ',');
		if(nums_tmp.size() != ids_tmp.size()){
			ERROR_TLOG("Load  suit config id [%u] failed, equip vec illegal, nums size:%d, ids size: %d", id, nums_tmp.size(), ids_tmp.size());    
			ERR_RT;
		}
		uint32_t size = nums_tmp.size();
		uint32_t i;

		std::map<uint32_t, uint32_t> tmp_map;
		tmp_map.clear();
		// 合并vec 到map
		for(i = 0; i < size ; i++){
			uint32_t tmp_nums = 0;
			uint32_t tmp_ids  = 0;
			try{
				tmp_nums = boost::lexical_cast<uint32_t> (nums_tmp[i]);
				tmp_ids  = boost::lexical_cast<uint32_t> ( ids_tmp[i]);
				if(tmp_map.count(tmp_nums) == 0){
					tmp_map[tmp_nums] = tmp_ids;  
				} else {
					ERROR_TLOG("Load suit config id[%u] parts_id:[%u] repeated", id, tmp_nums);    
					ERR_RT;
				}
			}
			catch(boost::bad_lexical_cast & e){
				ERROR_TLOG("Load suit config id[%u] failed,  vec_nums illegal, tmp_nums:%u, tmp_ids:%u, err %s", id, tmp_nums, tmp_ids, e.what());    
				ERR_RT;
			}
		}

		suit_node.id = id;
		suit_node.trigger_buff_map = tmp_map;

		if (tmp.suit_conf_exist(id)) {
			ERROR_TLOG("Load suit config id[%u] repeated", id);    
			ERR_RT;
		}

		tmp.add_suit_conf(suit_node);

		cur = cur->next;
	}
	g_suit_conf_mgr.copy_from(tmp);
	return 0;
}
