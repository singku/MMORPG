#ifndef __HOME_DATA_H__
#define __HOME_DATA_H__

#include "common.h"

//小屋送礼(送碎片)相关的公共数据
enum home_gift_relate_t {
	FRAGMENT_INDEX = commonproto::HM_FRAGMENT_INDEX,
	DAILY_SEND_GIFT_LIMIT = commonproto::HM_DAILY_SEND_GIFT_LIMIT,
	DAILY_RECV_GIFT_LIMIT = commonproto::HM_DAILY_RECV_GIFT_LIMIT,
	DAILY_HM_OPRA_COUNT = commonproto::HM_DAILY_HM_OPRA_COUNT,
	HM_SEND_FRANGMENT_LEVEL_LIMIT = commonproto::HM_SEND_FRANGMENT_LEVEL_LIMIT,
};

enum home_pet_exercise_t {
	HM_FOUND_BOX_RATE = 65,
};

/* 小屋访问日志结构体 */
struct visit_log_info_t {
    visit_log_info_t() {
        host_id_ = 0;
		h_create_tm_ = 0;
        guest_id_ = 0;
		g_create_tm_ = 0;
		sex_ = 0;
        guest_name_.clear();
        date_ = 0;
        log_type_ = 0;
        detail_.clear();
		fragment_id_ = 0;
    }

    /* 房主ID */
    uint32_t host_id_;
    /* 访客ID */
    uint32_t guest_id_;
    /* 性别 */
	uint32_t sex_;
    /* 访客名字 */
    string guest_name_;
    /* 访问时间 */
    int32_t date_;
    /* 动作类型 1表示啥也没干 6送碎片*/
    uint32_t log_type_;
    /* 留言内容 */
    string detail_;
	/* 若log_type类型为6，则记录碎片id，取属性表中的值*/
	uint32_t fragment_id_;
	uint32_t h_create_tm_;
	uint32_t g_create_tm_;
};


class home_data_t {
public:
	typedef std::map<int32_t, visit_log_info_t> visit_log_map_t;
	typedef std::map<int32_t, visit_log_info_t>::iterator visit_log_map_iter_t;
	home_data_t() {
		is_being_visited_ = false;
		at_whos_home_.userid = 0;
		at_whos_home_.u_create_tm = 0;
	}
	inline role_info_t at_whos_home() {
		return at_whos_home_;
	}
	inline void set_at_home(role_info_t host) {
		at_whos_home_ = host;
	}
	inline void set_is_being_visited(bool is_being_visited) {
		is_being_visited_ = is_being_visited;
	}
	uint32_t add_visit_log(const commonproto::visit_log_info_t &log_info); 
	uint32_t get_visit_log_map_info(visit_log_map_t& log_map);

	uint32_t notify_friend_ask_for_gift(std::vector<role_info_t>& user_id_vec, userid_t uid);

    uint32_t sync_data_when_send_frag(
            uint32_t player_id, uint32_t p_create_tm,
			uint32_t friend_id, uint32_t f_create_tm);

	static uint32_t clean_hm_fragment_info(player_t* player);

	std::string get_hm_frag() {
		return str_hm_frag_;
	}
	void set_hm_frag(const std::string& str) {
		str_hm_frag_ = str;
	}
private:
	uint32_t add_userid_to_hm_frag_info(uint64_t role_key);

	//如果玩家在某个人的小屋中 该字段记录对方的uid+create_tm组合成的key
	role_info_t at_whos_home_;
	//小屋信息是否在被其他人访问中
	bool is_being_visited_;
	//访客日志表
	visit_log_map_t vlog_map_;
	
	std::string str_hm_frag_;
	//玩家的好友，已经收到碎片的数量
	std::map<uint32_t, uint32_t> frag_map;
};

#endif
