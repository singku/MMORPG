#ifndef __TRAN_CARD_CONF_H__
#define __TRAN_CARD_CONF_H__

struct tran_card_conf_t {
	uint32_t conf_id;
	std::vector<uint32_t> skill_ids;
	std::vector<uint32_t> evolve_items;
};

class tran_card_conf_mgr_t {
public:
	typedef std::map<uint32_t, tran_card_conf_t> TranCardConfMap; 
	tran_card_conf_mgr_t() {
		clear();
	}
	~tran_card_conf_mgr_t (){
		clear();
	}
	inline void clear() {
		tran_card_conf_map_.clear();
	}
	inline const TranCardConfMap& const_tran_card_conf_map() const {
		return tran_card_conf_map_;
	}
	inline void copy_from(const tran_card_conf_mgr_t& m) {
		tran_card_conf_map_ = m.const_tran_card_conf_map();
	}
	inline bool is_tran_card_conf_exist(uint32_t card_id) {
		if (tran_card_conf_map_.count(card_id) > 0) { 
			return true;
		}
		return false;
	}

	tran_card_conf_t* get_tran_card_conf_info(uint32_t card_id) {
		TranCardConfMap::iterator it = tran_card_conf_map_.find(card_id);
		if (it == tran_card_conf_map_.end()) {
			return NULL;
		}
		return &(it->second);
	}

	inline bool add_tran_card_conf(const tran_card_conf_t& tran_card) {
		if (is_tran_card_conf_exist(tran_card.conf_id)) {
			return false;
		}
		tran_card_conf_map_.insert(TranCardConfMap::value_type(tran_card.conf_id, tran_card));
		return true;
	}

	inline void print_tran_card_info() {
		FOREACH(tran_card_conf_map_, it) {
			uint32_t card_id = it->first;
			std::vector<uint32_t> tem_skill_vec(it->second.skill_ids);
			FOREACH(tem_skill_vec, iter01) {
				uint32_t skill_id = *iter01;
				TRACE_TLOG("load_config_tran_card1:[%u][%u]", card_id, skill_id);
			}
			std::vector<uint32_t> tem_evolve_vec(it->second.evolve_items);
			FOREACH(tem_evolve_vec, iter02) {
				uint32_t evolve_id = *iter02;
				TRACE_TLOG("load_config_tran_card2:[%u][%u]", card_id, evolve_id);
			}
		}
	}
	
private:
	TranCardConfMap tran_card_conf_map_;
};
#endif
