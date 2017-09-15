#ifndef __TITLE_CONF_H__
#define __TITLE_CONF_H__

struct title_conf_t
{
	title_conf_t () :
		title_id(0),
		end_type(0),
		end_param(0) {}
	uint32_t title_id;
	uint32_t end_type;
	uint32_t end_param;
};

class title_conf_mgr_t
{
public:
	typedef std::map<uint32_t, title_conf_t> TitleConfMapMgr;
	title_conf_mgr_t() {
		clear();
	}
	~title_conf_mgr_t() {
		clear();
	}
	inline void clear() {
		title_map_conf_.clear();
	}
	inline const TitleConfMapMgr& const_title_conf_map() const {
		return title_map_conf_;
	}
	inline void copy_from(const title_conf_mgr_t &m) {
		title_map_conf_ = m.const_title_conf_map();
	}
	inline bool is_title_conf_exist(uint32_t title_id) {
		return title_map_conf_.count(title_id) > 0 ? true : false;
	}
	inline title_conf_t* get_title_conf(uint32_t title_id) {
		if (!is_title_conf_exist(title_id)) {
			return NULL;
		}
		return &(title_map_conf_.find(title_id)->second);
	}
	inline bool add_title_conf(const title_conf_t& title) {
		if (is_title_conf_exist(title.title_id)) {
			return false;
		}
		title_map_conf_.insert(TitleConfMapMgr::value_type(title.title_id, title));
		return true;
	}
	inline void print_title_conf() {
		FOREACH(title_map_conf_, it) {
			uint32_t title_id = it->second.title_id;
			uint32_t end_type = 0, end_params = 0;
			end_type = it->second.end_type;
			end_params = it->second.end_param;
			TRACE_TLOG("print title conf, title_id:%u,end_type:%u,end_params:%u", title_id, end_type, end_params);
		}
	}
private:
	TitleConfMapMgr title_map_conf_;
};
#endif
