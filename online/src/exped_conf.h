#ifndef __EXPED_CONF_H__
#define __EXPED_CONF_H__

struct exped_conf_t
{
	exped_conf_t() :
			card_id(0),
			power_percent(0),
			prize_id(0) {}
	uint32_t card_id;
	uint32_t power_percent;
	uint32_t prize_id;
};

class exped_conf_mgr_t {
public:
	typedef std::map<uint32_t, exped_conf_t> ExpedConfMgr;
	exped_conf_mgr_t() { clear();}
	~exped_conf_mgr_t() { clear();}
	inline void clear() {
		exped_conf_map_.clear();
	}
	inline const ExpedConfMgr& const_exped_conf_map() const {
		return exped_conf_map_;
	}
	inline void copy_from(const exped_conf_mgr_t& m) {
		exped_conf_map_ = m.const_exped_conf_map();
	}
	inline bool is_exped_conf_exist(uint32_t card_id) {
		return exped_conf_map_.count(card_id) > 0 ? true : false;
	}
	exped_conf_t* get_exped_conf_info(uint32_t card_id) {
		if (!is_exped_conf_exist(card_id)) {
			return NULL;
		}
		return &(exped_conf_map_.find(card_id)->second);
	}
	inline bool add_exped_conf(const exped_conf_t& exped) {
		if (is_exped_conf_exist(exped.card_id)) {
			return false;
		}
		exped_conf_map_.insert(ExpedConfMgr::value_type(exped.card_id, exped));
		return true;
	}
	inline void print_exped_info() {
		FOREACH(exped_conf_map_, it) {
			uint32_t card_id = it->second.card_id;
			uint32_t power_per = it->second.power_percent;
			uint32_t prize_id = it->second.prize_id;
			TRACE_TLOG("load config exped:[%u][%u][%u]", card_id, power_per, prize_id);
		}
	}
private:
	ExpedConfMgr  exped_conf_map_;	
};
#endif
