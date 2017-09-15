#ifndef __ACHIEVE_CONF_H__
#define __ACHIEVE_CONF_H__

enum {
	ACH_01_INC_POWER = 1,
	ACH_02_INC_LEVEL = 2,
	ACH_03_GET_EQUIP = 3,
	ACH_05_GET_PET = 5,
	ACH_06_GET_TELENT_PET = 6,
	ACH_07_GET_QUALITY_PET = 7,
	ACH_08_INC_PET_EFFORT = 8,
	ACH_14_EXPED_PASS_CARD = 14,
};

struct achieve_conf_t
{
	achieve_conf_t() : 
		id(0),
		achieve_type(0),
		track_target(0),
		target_2_value(0),
		point(0),
		prize_id(0),
		title_id(0) {}
	uint32_t id;
	uint32_t achieve_type;
	uint32_t track_target;
	uint32_t target_2_value;
	uint32_t point;
	uint32_t prize_id;
	uint32_t title_id;
	uint32_t title_during;
};

class achieve_mgr_t
{
public:
	typedef std::map<uint32_t, achieve_conf_t> AchieveInfoConfMgr;
	typedef std::vector<uint32_t> AchieveIdVec;
	typedef std::map<uint32_t, AchieveIdVec> AchieveTypeMap;
	achieve_mgr_t() {
		clear();
	}
	~achieve_mgr_t() {
		clear();
	}
	inline void clear() {
		achieve_map_conf_.clear();
	}
	inline const AchieveInfoConfMgr& const_achieve_info_conf_map() const {
		return achieve_map_conf_;
	}
	inline const AchieveTypeMap& const_achieve_type_map() const {
		return achieve_type_map_;
	}
	inline void copy_from(const achieve_mgr_t &m) {
		achieve_map_conf_ = m.const_achieve_info_conf_map();
		achieve_type_map_ = m.const_achieve_type_map();
	}
	inline bool is_achieve_info_conf_exist(uint32_t id) {
		return achieve_map_conf_.count(id) > 0 ? true : false;
	}
	inline achieve_conf_t* get_achieve_info_conf(uint32_t id) {
		if (!is_achieve_info_conf_exist(id)) {
			return NULL;
		}
		return  &(achieve_map_conf_.find(id)->second);
	}
	inline const AchieveIdVec* get_achieve_id_vec(uint32_t type) {
		if (achieve_type_map_.count(type) == 0) {
			return NULL;
		}
		return &(achieve_type_map_.find(type)->second);
	}
	inline bool add_achieve_conf(const achieve_conf_t &achieve_info) {
		if (is_achieve_info_conf_exist(achieve_info.id)) {
			return false;
		}
		achieve_map_conf_.insert(AchieveInfoConfMgr::value_type(achieve_info.id, achieve_info));
		return true;
	}
	inline void print_achieve_info() {
		FOREACH(achieve_map_conf_, it) {
			uint32_t id = it->second.id;
			uint32_t achieve_type = it->second.achieve_type;
			uint32_t track_target = it->second.track_target;
			uint32_t prize_id = it->second.prize_id;
			uint32_t title_id = it->second.title_id;
			uint32_t title_during = it->second.title_during;
			TRACE_TLOG("print achieve_map_conf:id=[%u],type=[%u],target=[%u],prize_id=[%u],title_id=[%u],title_during=[%u]",
					id, achieve_type, track_target, prize_id, title_id, title_during);
		}
	}
	inline void print_achieve_by_type() {
		FOREACH(achieve_type_map_, it) {
			uint32_t type = it->first;
			TRACE_TLOG("zjun_achieve_1020:type=[%u]", type);
			FOREACH(it->second, it02) {
				uint32_t ach_id = *it02;
				TRACE_TLOG("print_achieve_by_type:type=[%u],ach_id=[%u]",
						type, ach_id);
			}
		}
	}
	inline void insert_achieve_id(uint32_t achieve_type, uint32_t achieve_id) {
		if (achieve_type) {
			achieve_type_map_[achieve_type].push_back(achieve_id);
		}
	}
private:
	AchieveInfoConfMgr achieve_map_conf_;
	AchieveTypeMap achieve_type_map_;
};

#endif
