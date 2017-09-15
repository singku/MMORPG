#ifndef __SUIT_CONF_H__
#define __SUIT_CONF_H__

#include "common.h"

struct suit_conf_t {
	suit_conf_t(){
		clear();
	}
	void clear(){
		id = 0;
		trigger_buff_map.clear();
	}
	uint32_t id;
	std::map<uint32_t, uint32_t> trigger_buff_map;
};

class suit_conf_manager_t {
	public:
		suit_conf_manager_t(){
			clear();
		}
		~suit_conf_manager_t(){
			clear();
		}
	public:
		void clear(){
			suit_conf_map_.clear();
			equip_suit_map_.clear();
		}

		inline bool suit_conf_exist(uint32_t suit_id) {
			if (suit_conf_map_.count(suit_id) > 0) {
				return true;
			}
			return false;
		}

		inline bool equip_id_exist(uint32_t equip_id) {
			if (equip_suit_map_.count(equip_id) > 0) {
				return true;
			}
			return false;
		}

		inline bool add_suit_conf(suit_conf_t &suit_conf) {
			if (suit_conf_exist(suit_conf.id)) {
				return false;
			}
			suit_conf_map_[suit_conf.id] = suit_conf;
			return true;
		}

		inline bool add_equip_suit_map(uint32_t equip_id, uint32_t suit_id) {
			if (equip_id_exist(equip_id)) {
				return false;
			}
			equip_suit_map_[equip_id] = suit_id;
			return true;
		}

		const inline suit_conf_t *find_suit_conf(uint32_t suit_id) {
			if (suit_conf_map_.count(suit_id) == 0) {
				return NULL;
			}
			return &((suit_conf_map_.find(suit_id))->second);
		}

		const inline uint32_t find_suit_id(uint32_t equip_id) {
			if (equip_suit_map_.count(equip_id) == 0) {
				return NULL;
			}
			return (equip_suit_map_.find(equip_id))->second;
		}

		inline uint32_t suit_map_size(){ return suit_conf_map_.size();}
		inline uint32_t equip_map_size(){ return equip_suit_map_.size();}

		inline void copy_from(const suit_conf_manager_t &m) {
			suit_conf_map_ = m.const_suit_conf_map();
			 equip_suit_map_= m.const_equip_suit_map();
		}

		const inline std::map<uint32_t, suit_conf_t>& const_suit_conf_map() const{
			return suit_conf_map_;
		}

		const inline std::map<uint32_t, uint32_t>& const_equip_suit_map() const{
			return   equip_suit_map_;
		}

	private:
		std::map<uint32_t, suit_conf_t> suit_conf_map_;
		std::map<uint32_t, uint32_t>   equip_suit_map_;
};
#endif
