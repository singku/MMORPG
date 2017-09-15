#include "common.h"
#include "package.h"
#include "player.h"
#include "item.h"
#include "macro_utils.h"
#include "item_conf.h"

int check_item_in_pos(const player_t *p, uint32_t pos, uint32_t fun_type)
{
    const item_t *item = p->package->get_const_item_in_slot(pos);
    if (!item) {
        ERROR_TLOG("p:%u, INVALID ITEM INFO. POS=(%u)", 
                p->userid, pos);
        return cli_err_item_pos_not_exist;
    }
    if (!fun_type) {
        return 0;
    }
    const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(item->item_id);
    if (!item_conf || item_conf->item_id != item->item_id 
            || (uint32_t)item_conf->fun_type != fun_type) {
        ERROR_TLOG("p:%u, INVALID ITEM INFO [id:%u pos:%u(slot_id:%u) fun_type:%u]",
                p->userid, item->item_id, pos, item->slot_id,
                item_conf ?item_conf->fun_type :0);
        return cli_err_item_pos_not_wanted;
    }
    return 0;
}


Package::Package()
{
    min_free_slot_id_ = 1;
}

Package::~Package()
{

}

int Package::find_next_free_slot_id(uint32_t start_id)
{
    std::map<uint32_t, item_t>::iterator it = slots_.find(start_id);

    while (it != slots_.end() && it->first == start_id) {

        it++;
        start_id++;
    }

    return start_id;
}

int Package::put_item(const item_t* item)
{
    item_t* slot_item = get_item_in_slot(item->slot_id);

    // 已存放物品
    if (slot_item != NULL) {
        return -1; 
    }

    slots_[item->slot_id] = *item;
    put_item_slot(item->item_id, item->slot_id);

    if (item->slot_id == min_free_slot_id_) {
        min_free_slot_id_ = find_next_free_slot_id(min_free_slot_id_ + 1);
    }

    return 0;
}

int Package::add_item(const item_t* item, std::vector<item_t>& result_list)
{
    uint32_t remain = item->count;
    bool can_merge = true;
    const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(item->item_id);
    if (item_conf->slot_max == 1) {//一个格子只能放一个
        can_merge = false;
    }

    if (can_merge) {
        // 先放入之前可以堆叠的物品中
        std::vector<uint32_t> slot_id_list;
        find_all_slot_has_item(item->item_id, slot_id_list);

        if (slot_id_list.size()) {

            for (uint32_t i = 0; remain > 0 && i < slot_id_list.size(); i++) {

                uint32_t slot_id = slot_id_list[i];  
                item_t* slot_item = get_item_in_slot(slot_id);
                if (!slot_item || slot_item->count >= kMaxSlotVolume) {
                    continue; 
                }

                uint32_t left_volumn = kMaxSlotVolume - slot_item->count;
                if (remain > left_volumn) {
                    slot_item->count += left_volumn;
                    remain -= left_volumn;
                } else {
                    slot_item->count += remain;

                    remain = 0; 
                }
                result_list.push_back(*slot_item);
            }
        }
    }

    // 将剩余的放入新的格子里
    while (remain > 0) {
        uint32_t slot_id = min_free_slot_id_;
        item_t slot_item = *item;
		slot_item.slot_id = slot_id;

		if (can_merge) { //可合并
            if (remain > kMaxSlotVolume) {
                slot_item.count = kMaxSlotVolume;
                remain -= kMaxSlotVolume; 
            } else {
                slot_item.count = remain;
                remain = 0; 
            }
        } else {
            slot_item.count = 1;
            remain -= 1;
        }

        slots_[slot_id] = slot_item;
        put_item_slot(item->item_id, slot_id);

        min_free_slot_id_ = find_next_free_slot_id(min_free_slot_id_ + 1);

        result_list.push_back(slot_item);
    }

    return 0;
}

int Package::clear_slot_item(const item_t* slot_item)
{
    remove_item_slot(slot_item->item_id, slot_item->slot_id);
    remove_item_in_slot(slot_item->slot_id);

    if (slot_item->slot_id < min_free_slot_id_) {
        min_free_slot_id_ = slot_item->slot_id; 
    }

    return 0;
}

int Package::reduce_item(item_t* item, uint32_t n)
{
    if (item == NULL) {
        return -1; 
    }

    // 物品数量不足
    uint32_t usable_count = item->usable_count();
    if (usable_count < n) {
        return -1; 
    }

    if(0 != item->reduce_n(n)) {
        return -1;
    }

    if (item->count <= 0) {
        return clear_slot_item(item);
    }

    return 0;
}

int Package::reduce_item_by_slot_id(uint32_t slot_id, uint32_t n, item_t* result)
{
    item_t* slot_item = get_item_in_slot(slot_id);
    *result = *slot_item; 

    int ret = reduce_item(slot_item, n);
    if (ret != 0) {
        return -1; 
    } else {
        result->count -= n;
        return 0;
    }
}

int Package::dump(FILE* file)
{
    slots_ptr_t it = slots_.begin();

    while (it != slots_.end()) {

        const item_t* item = &it->second;

        fprintf(file, "slot #%u: item %u, count %u, expire_time %u\n",
                it->first, item->item_id, item->count, item->expire_time);

        it++;
    }

    item_slot_set_ptr_t ptr = item_slot_set_.begin();

    while (ptr != item_slot_set_.end()) {
        fprintf(file, "item_slot: %lu\n", *ptr); 
        ptr++;
    }

    return 0;
}

int Package::get_all_items(std::vector<item_t>& results)
{
    slots_ptr_t ptr = slots_.begin();

    while (ptr != slots_.end()) {

        results.push_back(ptr->second);

        ptr++;
    }

    return 0;
}

uint32_t Package::get_equip_count_by_quality(player_t* player,
		equip_quality_t quality, uint32_t& count)
{
	count = 0;
	slots_ptr_t ptr = slots_.begin();
	while (ptr != slots_.end()) {
        ptr++;
		item_t& item = ptr->second;
		if (!g_item_conf_mgr.is_equip(item.item_id)) {
			continue;
		}
		commonproto::item_optional_attr_t pb_item_attr;
		pb_item_attr.ParseFromString(item.opt_attr);
		if (pb_item_attr.level() != (uint32_t)quality) {
			continue;
		}
		++count;
	}
	return 0;
}

int Package::get_fun_type_items(std::vector<item_t>& results, uint32_t fun_type)
{
    slots_ptr_t ptr = slots_.begin();

    while (ptr != slots_.end()) {
        item_t &item = ptr->second;
        const item_conf_t *item_conf = 
            g_item_conf_mgr.find_item_conf(item.item_id);
        if (item_conf && (uint32_t)item_conf->fun_type == fun_type) {
            results.push_back(ptr->second);
        }

        ptr++;
    }

    return 0;
}

int Package::reduce_item_by_item_id(uint32_t item_id, uint32_t n, std::vector<item_t>& result)
{
    std::vector<item_t*> item_list;
    uint32_t usable_total_num = 0;
    get_all_usable_items(item_id, item_list, &usable_total_num);

    if (usable_total_num < n) {
        return -1; 
    }

    item_t temp;
    for (uint32_t i = 0; n && i < item_list.size(); i++) {
        item_t* item = item_list[i]; 
        temp = *item;
        uint32_t usable_count = item->usable_count();
        if (usable_count > n) {
            reduce_item(item, n);
            temp.count -= n;
            n = 0;
        } else {
            reduce_item(item, usable_count);
            temp.count -= usable_count;
            n -= usable_count;
        }
        
        result.push_back(temp);
    }

    return 0;
}

bool Package::has_item(uint32_t item_id, bool must_not_expire)
{
    if (must_not_expire == true) {
        int err;
        item_t *item = find_usable_item(item_id, err, 1);
        return (item != NULL);
    }

    //过期与否没关系
    uint32_t slot_id = 0;
    bool find = find_min_slot_has_item(item_id, &slot_id);
    uint32_t another_item_id = g_item_conf_mgr.get_another_item_id(item_id);
    find = find ?find :find_min_slot_has_item(another_item_id, &slot_id);
    return find;
}

void Package::get_all_usable_items(uint32_t item_id, 
        std::vector<item_t*>& item_list, uint32_t* usable_total_num)
{
    item_list.clear();

    //如果是免费的道具ID 则优先找收费的道具
    if (item_id <= ITEM_FC_DIFF) {
        item_id = g_item_conf_mgr.get_another_item_id(item_id);
    }

    item_slot_t min_item_slot = get_item_min_item_slot(item_id); 
    item_slot_t max_item_slot = get_item_max_item_slot(item_id);
    item_slot_set_ptr_t low_ptr = item_slot_set_.lower_bound(min_item_slot);
    item_slot_set_ptr_t up_ptr = item_slot_set_.upper_bound(max_item_slot);
    item_slot_set_ptr_t ptr = low_ptr;

    *usable_total_num = 0;

    for (; ptr != up_ptr; ptr++) {
        uint32_t item_id = 0;
        uint32_t slot_id = 0;
        parse_item_slot(*ptr, &item_id, &slot_id);
        item_t* item = get_item_in_slot(slot_id);
        if (item && item->using_count < item->count) {
            item_list.push_back(item);
            *usable_total_num += item->usable_count();
        }
    }

    uint32_t another_item_id = g_item_conf_mgr.get_another_item_id(item_id);
    min_item_slot = get_item_min_item_slot(another_item_id); 
    max_item_slot = get_item_max_item_slot(another_item_id);
    low_ptr = item_slot_set_.lower_bound(min_item_slot);
    up_ptr = item_slot_set_.upper_bound(max_item_slot);
    ptr = low_ptr;

    for (; ptr != up_ptr; ptr++) {
        uint32_t item_id = 0;
        uint32_t slot_id = 0;
        parse_item_slot(*ptr, &item_id, &slot_id);
        item_t* item = get_item_in_slot(slot_id);
        if (item && item->using_count < item->count) {
            item_list.push_back(item);
            *usable_total_num += item->usable_count();
        }
    }

}

item_t *Package::find_usable_item(uint32_t item_id, int &err, uint32_t n) 
{
    //如果是免费的道具ID 则优先找收费的道具
    if (item_id <= ITEM_FC_DIFF) {
        item_id = g_item_conf_mgr.get_another_item_id(item_id);
    }

    uint32_t another_item_id = g_item_conf_mgr.get_another_item_id(item_id);

    item_slot_t min_item_slot = get_item_min_item_slot(item_id); 
    item_slot_t max_item_slot = get_item_max_item_slot(item_id);
    item_slot_set_ptr_t ptr = item_slot_set_.lower_bound(min_item_slot);

    item_slot_t a_min_item_slot = get_item_min_item_slot(another_item_id); 
    item_slot_t a_max_item_slot = get_item_max_item_slot(another_item_id);
    item_slot_set_ptr_t a_ptr = item_slot_set_.lower_bound(a_min_item_slot);

    err = 0;
    if (ptr == item_slot_set_.end() && a_ptr == item_slot_set_.end()) {
        err = cli_err_item_not_exist; 
        return 0;
    }

    item_t *item = 0;
    uint32_t slot_id;
    for (; ptr != item_slot_set_.end() && (*ptr) <= max_item_slot; ptr++) {
        parse_item_slot(*ptr, &item_id, &slot_id);
        item = get_mutable_item_in_slot(slot_id);
        if (item->usable_count() >= n) {
            return item;
        }
        item = 0;
    }

    for (; a_ptr != item_slot_set_.end() && (*a_ptr) <= a_max_item_slot; a_ptr++) {
        parse_item_slot(*a_ptr, &another_item_id, &slot_id);
        item = get_mutable_item_in_slot(slot_id);
        if (item->usable_count() >= n) {
            return item;
        }
        item = 0;
    }

    err = cli_err_lack_usable_item;
    return 0;
}

uint32_t Package::get_total_item_count(uint32_t item_id)
{
    item_slot_t min_item_slot = get_item_min_item_slot(item_id); 
    item_slot_t max_item_slot = get_item_max_item_slot(item_id);
    item_slot_set_ptr_t low_ptr = item_slot_set_.lower_bound(min_item_slot);
    item_slot_set_ptr_t up_ptr = item_slot_set_.upper_bound(max_item_slot);
    item_slot_set_ptr_t ptr = low_ptr;

    uint32_t total_num = 0;

    for (; ptr != up_ptr; ptr++) {
        uint32_t item_id = 0;
        uint32_t slot_id = 0;
        parse_item_slot(*ptr, &item_id, &slot_id);
        item_t* item = get_item_in_slot(slot_id);
        if (item) {
            total_num += item->count;
        }
    }

    item_id = g_item_conf_mgr.get_another_item_id(item_id);
    min_item_slot = get_item_min_item_slot(item_id); 
    max_item_slot = get_item_max_item_slot(item_id);
    low_ptr = item_slot_set_.lower_bound(min_item_slot);
    up_ptr = item_slot_set_.upper_bound(max_item_slot);
    ptr = low_ptr;

    for (; ptr != up_ptr; ptr++) {
        uint32_t item_id = 0;
        uint32_t slot_id = 0;
        parse_item_slot(*ptr, &item_id, &slot_id);
        item_t* item = get_item_in_slot(slot_id);
        if (item) {
            total_num += item->count;
        }
    }

    return total_num;
}

uint32_t Package::get_total_usable_item_count(uint32_t item_id)
{
    std::vector<item_t*> item_list;
    uint32_t total_num = 0;

    get_all_usable_items(item_id, item_list, &total_num);

    return total_num;
}

void Package::clean_expired_items(std::vector<item_t>& result_list)
{
    slots_ptr_t ptr = slots_.begin();
    uint32_t now = get_now_tv()->tv_sec;

    while (ptr != slots_.end()) {

        const item_t* slot_item = &ptr->second;
        ptr++;

        if (slot_item->expire_time != 0 && slot_item->expire_time < now) {
            result_list.push_back(*slot_item);
            clear_slot_item(slot_item);
        }
    }
}

void Package::find_all_slot_has_item(uint32_t item_id, std::vector<uint32_t>& slot_id_list)
{
    slot_id_list.clear();

    item_slot_t min_item_slot = get_item_min_item_slot(item_id); 
    item_slot_t max_item_slot = get_item_max_item_slot(item_id);
    item_slot_set_ptr_t ptr = item_slot_set_.lower_bound(min_item_slot);

    for (; ptr != item_slot_set_.end() && 
            (*ptr) >= min_item_slot && (*ptr) <= max_item_slot; ptr++) {
        uint32_t slot_id = 0;
        parse_item_slot(*(ptr), &item_id, &slot_id);
        slot_id_list.push_back(slot_id);
    }
}

int Package::use_item_by_item_id(uint32_t item_id, item_t* result)
{
    std::vector<item_t*> item_list;
    uint32_t usable_total_num = 0;
    get_all_usable_items(item_id, item_list, &usable_total_num);

    if (usable_total_num == 0) {
        return -1; 
    }

    for (uint32_t i = 0; i < item_list.size(); i++) {
        item_t* item = item_list[i]; 
        if (item->usable_count() >= 1) {
            item->using_count++; 
            *result = *item;
            return 0;
        }
    }

    return -1;
}

int Package::unuse_item_by_item_id(uint32_t item_id, item_t* result)
{
    std::vector<uint32_t> slot_id_list;
    find_all_slot_has_item(item_id, slot_id_list);

    if (slot_id_list.size() == 0) {
        return -1; 
    }

    for (uint32_t i = 0; i < slot_id_list.size(); i++) {

        uint32_t slot_id = slot_id_list[i];
        item_t* item = get_item_in_slot(slot_id);

        if (item && item->using_count > 0) {
            item->using_count--; 
            *result = *item;
            return 0;
        }
    }

    return -1;
}

void Package::update_item_id(item_t *item, uint32_t new_item_id)
{
    remove_item_slot(item->item_id, item->slot_id);
    item->item_id = new_item_id;
    put_item_slot(item->item_id, item->slot_id);
}
