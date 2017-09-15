#ifndef __PET_PASS_DUP_CONF_H__
#define __PET_PASS_DUP_CONF_H__
#include "common.h"

enum pet_pass_dup_activity_type_t
{
	PET_PASS_DUP_ACTIVITY_TYPE_START = commonproto::PET_PASS_DUP_ACTIVITY_TYPE_START,
	PET_PASS_DUP_DIJU_AWAKE = commonproto::PET_PASS_DUP_DIJU_AWAKE,
	PET_PASS_DUP_ACTIVITY_TYPE_END = commonproto::PET_PASS_DUP_ACTIVITY_TYPE_END,
};

struct dup_shop_id_t
{
	uint32_t dup_id;
	uint32_t shop_id;
};

bool order_by_dup_id_asc(dup_shop_id_t dup_shopid_1, dup_shop_id_t dup_shopid_2);
bool order_by_dup_id_des(dup_shop_id_t dup_shopid_1, dup_shop_id_t dup_shopid_2);

struct pet_pass_dup_conf_t 
{
	uint32_t pet_id;
	std::map<uint32_t, std::vector<dup_shop_id_t> > dup_shopid_map;
	std::map<uint32_t, uint32_t> unlock_state_map;
	std::map<uint32_t, uint32_t> base_shopid_map;
};

class pet_pass_dup_conf_mgr_t
{
public:
	typedef std::map<uint32_t, pet_pass_dup_conf_t> PetPassDupConfMgr;
	pet_pass_dup_conf_mgr_t() {
		clear();
	}
	~pet_pass_dup_conf_mgr_t() {
		clear();
	}
	inline void clear() {
		dup_ids_conf_map_.clear();
	}
	inline const PetPassDupConfMgr& const_pet_pass_dup_conf_map() const {
		return dup_ids_conf_map_;
	}
	inline void copy_from(const pet_pass_dup_conf_mgr_t &m) {
		dup_ids_conf_map_ = m.const_pet_pass_dup_conf_map();
	}
	inline bool is_pet_pass_dup_conf_exist(uint32_t pet_id) {
		return dup_ids_conf_map_.count(pet_id) ? true : false;
	}
	pet_pass_dup_conf_t* get_pet_pass_dup_info_conf(uint32_t pet_id) {
		if (dup_ids_conf_map_.count(pet_id) == 0) {
			return NULL;
		}
		return &((dup_ids_conf_map_.find(pet_id))->second);
	}
	inline bool add_pet_pass_dup_conf(const pet_pass_dup_conf_t& pet_conf) {
		if (is_pet_pass_dup_conf_exist(pet_conf.pet_id)) {
			return false;
		}
		dup_ids_conf_map_.insert(PetPassDupConfMgr::value_type(pet_conf.pet_id, pet_conf));
		return true;
	}
	inline uint32_t get_shop_id(uint32_t pet_id,
			uint32_t activity_type, uint32_t dup_id, uint32_t &shop_id) {
		if (dup_ids_conf_map_.count(pet_id) && 
				dup_ids_conf_map_[pet_id].dup_shopid_map.count(activity_type)) {

			std::vector<dup_shop_id_t>& dup_shopid_vec = 
				dup_ids_conf_map_[pet_id].dup_shopid_map[activity_type];
			FOREACH(dup_shopid_vec, it) {
				if (it->dup_id == dup_id) {
					shop_id = it->shop_id;
					return 0;
				}
			}
		}
		return 0;
	}
	inline void print_pet_pass_dup_info() {
		FOREACH(dup_ids_conf_map_, it) {
			uint32_t pet_id = it->first;
			FOREACH(it->second.dup_shopid_map, iter) {
				uint32_t activity_type = iter->first;
				FOREACH(iter->second, iter02) {
					uint32_t dup_id = iter02->dup_id;
					uint32_t shop_id = iter02->shop_id;
					TRACE_TLOG("load config pet pass dup info:pet_id[%u],type[%u],dup_id[%u],shop_id[%u]",
							pet_id, activity_type, dup_id, shop_id);
				}
			}
		}
	}
private:
	PetPassDupConfMgr dup_ids_conf_map_;
};

#endif
