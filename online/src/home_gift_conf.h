#ifndef __HOME_GIFT_CONF_H__
#define __HOME_GIFT_CONF_H__
#include "home_data.h"


struct home_gift_conf_t {
	uint32_t id;
	uint32_t rate;
	uint32_t need_count;
	uint32_t to_item_id;
	uint32_t count;
};


class home_gift_conf_mgr_t {
public:
	typedef std::map<uint32_t, home_gift_conf_t> HomeGiftConfMap;
	home_gift_conf_mgr_t () {
		clear();
	}
	~home_gift_conf_mgr_t () {
		clear();
	}
	inline void clear() {
		hm_gift_conf_map_.clear();
	}
	inline const HomeGiftConfMap& const_home_gift_conf_map() const {
		return hm_gift_conf_map_;
	}
	home_gift_conf_t* get_hm_gift_info(uint32_t fragment_id) {
		if (!is_hm_gift_conf_exist(fragment_id)) {
			return NULL;
		}
		return &(hm_gift_conf_map_.find(fragment_id)->second);
	}
	inline const std::vector<uint32_t>&  get_home_gift_rate_vec() const{
		return hm_gift_rate_vec_;
	}
	inline void copy_from(const home_gift_conf_mgr_t& m) {
		hm_gift_conf_map_ = m.const_home_gift_conf_map();
		hm_gift_rate_vec_ = m.get_home_gift_rate_vec();
	}
	inline bool is_hm_gift_conf_exist(uint32_t id) {
		if (hm_gift_conf_map_.count(id) > 0) {
			return true;
		}
		return false;
	}
	inline bool add_hm_gift_conf(const home_gift_conf_t& hm_gift) {
		if (is_hm_gift_conf_exist(hm_gift.id)) {
			return false;
		}
		hm_gift_conf_map_.insert(HomeGiftConfMap::value_type(hm_gift.id, hm_gift));
		return true;
	}
	inline void add_hm_gift_rate(uint32_t rate) {
		hm_gift_rate_vec_.push_back(rate);
	}
	inline void print_hm_gift_info() {
		FOREACH(hm_gift_conf_map_, it) {
			uint32_t id = it->second.id;
			uint32_t rate = it->second.rate;
			uint32_t need_count = it->second.need_count;
			uint32_t to_item_id = it->second.to_item_id;
			uint32_t count = it->second.count;
			TRACE_TLOG("load hm gift:id=[%u],rate=[%u]need_count=[%u],to_item_id=[%u],count=[%u]",
					id, rate, need_count, to_item_id, count);
		}
	}
	inline void print_hm_gift_vec() {
		FOREACH(hm_gift_rate_vec_, it2) {
			TRACE_TLOG("load hm gift rate: rate=[%u]", *it2);
		}
	}
private:
	HomeGiftConfMap hm_gift_conf_map_;
	std::vector<uint32_t> hm_gift_rate_vec_;
};

#endif
