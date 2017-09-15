#ifndef __BLESS_PET_CONF_H__
#define __BLESS_PET_CONF_H__
#include "common.h"

struct bless_pet_conf_t {
	bless_pet_conf_t(){
		clear();
	} 
	void clear(){
		bless_id = 0;
		pet_id   = 0;
		dup_id   = 0;
	}
	uint32_t bless_id;
	uint32_t pet_id;
	uint32_t dup_id;
};

class bless_pet_conf_manager_t {
	public:
		bless_pet_conf_manager_t(){
			clear();
		}
		~bless_pet_conf_manager_t(){
			clear();
		}
	public:
		void clear(){
			bless_pet_conf_map_.clear();
		}
		inline bool bless_pet_conf_exist(uint32_t bless_id) {
			if (bless_pet_conf_map_.count(bless_id) > 0) {
				return true;
			}
			return false;
		}
		inline bool add_bless_pet_conf(bless_pet_conf_t &bless_pet_conf) {
			if (bless_pet_conf_exist(bless_pet_conf.bless_id)) {
				return false;
			}
			bless_pet_conf_map_[bless_pet_conf.bless_id] = bless_pet_conf;
			return true;
		}
		const inline bless_pet_conf_t *find_bless_pet_conf(uint32_t bless_id) {
			if (bless_pet_conf_map_.count(bless_id) == 0) {
				return NULL;
			}
			return &((bless_pet_conf_map_.find(bless_id))->second);
		}
		inline uint32_t map_size(){ return bless_pet_conf_map_.size();}
		inline void copy_from(const bless_pet_conf_manager_t &m) {
			bless_pet_conf_map_ = m.const_bless_pet_conf_map();
		}
		const inline std::map<uint32_t, bless_pet_conf_t>& const_bless_pet_conf_map() const{
			return bless_pet_conf_map_;
		}
	private:
		std::map<uint32_t, bless_pet_conf_t> bless_pet_conf_map_;
};

#endif
