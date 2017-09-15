#ifndef FRIEND_INFO_H 
#define FRIEND_INFO_H
#include <libtaomee++/proto/proto_base.h>
#include <google/protobuf/message.h>
#include <libtaomee/project/types.h>
#include <stdio.h>
#include <stdint.h>
#include "proto.h"
#include "proto/db/dbproto.data.pb.h"
#include "proto/client/cli_errno.h"
#include <map>
#include <vector>
#include <list>
typedef uint64_t user_inf_t; 
struct friend_t {
	friend_t& operator=(const friend_t&);
	userid_t id;//好友id
	uint32_t create_tm;//创建时间
	std::string nick; //玩家昵称
	bool is_vip;//是否是vyp
	bool is_online; //是否在线
	uint32_t vip_grade; // vip等级
	uint32_t level; // vip等级
//	uint32_t gift_count;	//今日给该好友送礼的次数
	uint32_t power; // 战斗力
	uint32_t last_login_tm;	//最近一次的登录时间
};

struct recommendation_t {
	recommendation_t& operator=(const recommendation_t&);
	uint32_t userid; //用户id                                                                                            
    uint32_t create_tm; // 创建时间
	std::string nick;//用户名                                                                             
	bool is_vip;//是否vip                                                                            
	bool is_online;//是否在线                                                                        
	uint32_t vip_grade;//vip等级                                                                       
	uint32_t power;//战斗力                                                                            
	uint32_t team; //组别 0 好友， 1 黑名单 2 已删除 3 临时好友（仅用于上线提醒添加好友）              
	uint32_t level;//等级                                                                              
	uint32_t last_login_time;//上次登录时间                                                            
	uint32_t sex;//上次登录时间                                                            
};

struct black_t {
	userid_t id;//好友id
    uint32_t create_tm; // 创建时间
	std::string nick; //玩家昵称
	bool is_vip;//是否是vyp
	bool is_online; //是否在线
	uint32_t vip_grade; // vip等级
	uint32_t level; // vip等级
	//uint32_t power; // 战斗力
};

struct recent_t {
	bool operator ==(const recent_t& b);
	userid_t id;//好友id
    uint32_t create_tm; // 创建时间
	//std::string nick; //玩家昵称
	//bool is_vip;//是否是vyp
	//bool is_online; //是否在线
	//uint32_t vip_grade; // vip等级
	//uint32_t power; // 战斗力
};

struct temp_t {
	userid_t friendid; //好友申请者的userid
    uint32_t create_tm; // 创建时间
};
class FriendInfo
{
public:
	FriendInfo();
	~FriendInfo();
	// inline uint32_t add_friend(const friend_t& friend_data) {
		// m_friends[friend_data.id] = friend_data;
		// return 0;
	// }
	inline uint32_t add_friend(const friend_t& friend_data) {
		user_inf_t key = comp_u64(friend_data.id, friend_data.create_tm);		
		m_friends[key] = friend_data;
		return 0;
	}

	// inline uint32_t remove_friend(userid_t userid) {
		// m_friends.erase(userid);
		// return 0;
	// }

	inline uint32_t remove_friend(userid_t userid, uint32_t create_tm ) {
		user_inf_t key = comp_u64( userid, create_tm);		
		m_friends.erase(key);
		return 0;
	}

	// inline uint32_t remove_black(userid_t userid) {
		// m_blacklist.erase(userid);
		// return 0;
	// }

	inline uint32_t remove_black(userid_t userid, uint32_t create_tm) {
		user_inf_t key = comp_u64(userid, create_tm);		
		m_blacklist.erase(key);
		return 0;
	}

	// inline uint32_t add_black(const black_t& black_data) {
		// m_blacklist[black_data.id] = black_data;
		// return 0;
	// }
	inline uint32_t add_black(const black_t& black_data) {
		user_inf_t key = comp_u64( black_data.id, black_data.create_tm);		
		m_blacklist[key] = black_data;
		return 0;
	}
	inline uint32_t add_recent(const recent_t& recent_data) {
		l_recentlist.push_front(recent_data);
		return 0;
	}

	// inline uint32_t add_temp(const temp_t& temp_data) {
		// m_templist[temp_data.friendid] = temp_data;
		// return 0;
	// }
	inline uint32_t add_temp(const temp_t& temp_data) {
		user_inf_t key = comp_u64(temp_data.friendid, temp_data.create_tm);		
		m_templist[key] = temp_data;
		return 0;
	}

	// inline uint32_t remove_temp(userid_t userid) {
		// m_templist.erase(userid);
		// return 0;
	// }
	inline uint32_t remove_temp(userid_t userid, uint32_t create_tm) {
		user_inf_t key = comp_u64(userid, create_tm);		
		m_templist.erase(key);
		return 0;
	}
	// inline uint32_t get_friend_by_id(userid_t friendid, friend_t& friend_data) {
		// std::map<userid_t, friend_t>::iterator it = m_friends.find(friendid);
		// if (it == m_friends.end()) {
			// return cli_err_friend_id_not_exist;
		// }
		// else {
			// friend_data = m_friends[friendid];
		// }
		// return 0;
	// }
	inline uint32_t get_friend_by_id(userid_t friendid, uint32_t create_tm, friend_t& friend_data) {
		// std::map<userid_t, friend_t>::iterator it = m_friends.find(friendid);
		user_inf_t key = comp_u64(friendid, create_tm);		
		std::map<user_inf_t, friend_t>::iterator it = m_friends.find(key);
		if (it == m_friends.end()) {
			return cli_err_friend_id_not_exist;
		}
		else {
			friend_data = m_friends[key];
		}
		return 0;
	}
	// inline uint32_t get_black_by_id(userid_t friendid, black_t& black_data) {
		// std::map<userid_t, black_t>::iterator it = m_blacklist.find(friendid);
		// if (it == m_blacklist.end()) {
			// return cli_err_friend_id_not_exist;
		// }
		// else {
			// black_data = m_blacklist[friendid];
		// }
		// return 0;
	// }
	inline uint32_t get_black_by_id(userid_t friendid, uint32_t create_tm, black_t& black_data) {
		// std::map<userid_t, black_t>::iterator it = m_blacklist.find(friendid);
		user_inf_t key = comp_u64(friendid, create_tm);		
		std::map<user_inf_t, black_t>::iterator it = m_blacklist.find(key);
		if (it == m_blacklist.end()) {
			return cli_err_friend_id_not_exist;
		}
		else {
			black_data = m_blacklist[key];
		}
		return 0;
	}
	// inline bool has_friend(userid_t friendid) {
		// std::map<userid_t, friend_t>::iterator it = m_friends.find(friendid);
		// if (it == m_friends.end()) {
			// return false;
		// }
		// else {
			// return true;
		// }
	// }
	
	inline bool has_friend(userid_t friendid, uint32_t create_tm) {
		// std::map<userid_t, friend_t>::iterator it = m_friends.find(friendid);
		user_inf_t key = comp_u64(friendid, create_tm);		
		std::map<user_inf_t, friend_t>::iterator it = m_friends.find(key);
		if (it == m_friends.end()) {
			return false;
		} else {
			return true;
		}
	}
	// inline bool has_black(userid_t blackid) {
		// std::map<userid_t, black_t>::iterator it = m_blacklist.find(blackid);
		// if (it == m_blacklist.end()) {
			// return false;
		// }
		// else {
			// return true;
		// }
	// }

	inline bool has_black(userid_t blackid, uint32_t create_tm) {
		// std::map<userid_t, black_t>::iterator it = m_blacklist.find(blackid);
		user_inf_t key = comp_u64( blackid, create_tm);		
		std::map<user_inf_t, black_t>::iterator it = m_blacklist.find(key);
		if (it == m_blacklist.end()) {
			return false;
		}
		else {
			return true;
		}
	}
	// inline bool has_temp(userid_t tempid) {
		// std::map<userid_t, temp_t>::iterator it = m_templist.find(tempid);
		// if (it == m_templist.end()) {
			// return false;
		// }
		// else {
			// return true;
		// }
	// }
	inline bool has_temp(userid_t tempid, uint32_t create_tm) {
		user_inf_t key = comp_u64(tempid, create_tm);		
		std::map<user_inf_t, temp_t>::iterator it = m_templist.find(key);
		if (it == m_templist.end()) {
			return false;
		}
		else {
			return true;
		}
	}
	inline uint32_t get_recent_size() {
		return l_recentlist.size();
	}

	inline uint32_t get_recent_back(recent_t& recent) {
		if (!l_recentlist.empty()) {
			recent.id = l_recentlist.back().id;
			recent.create_tm= l_recentlist.back().create_tm;
			return 0;
		}
		return cli_err_list_is_empty;
	}

	inline uint32_t get_recent_front(recent_t& recent) {
		if (!l_recentlist.empty()) {
			recent.id = l_recentlist.front().id;
			recent.create_tm= l_recentlist.back().create_tm;
			return 0;
		}
		return cli_err_list_is_empty;
	}
	inline uint32_t remove_recent_back() {
		if (l_recentlist.empty()) {
			return cli_err_list_is_empty;
		}
		l_recentlist.pop_back();
		return 0;
	}

	inline uint32_t remove_recent_front() {
		if (l_recentlist.empty()) {
			return cli_err_list_is_empty;
		}
		l_recentlist.pop_front();
		return 0;
	}

	inline uint32_t remove_recent_element(recent_t& recent_data ) {
		l_recentlist.remove(recent_data);
		return 0;
	}
	inline uint32_t add_recent_back(const recent_t& recent_data) {
		l_recentlist.push_back(recent_data);
		return 0;
	}

	inline uint32_t add_recent_front(const recent_t& recent_data) {
		l_recentlist.push_front(recent_data);
		return 0;
	}
	inline uint32_t get_friends_size() {
		return m_friends.size();
	}

	inline uint32_t get_blacks_size() {
		return m_blacklist.size();
	}
	uint32_t get_all_friends(std::vector<friend_t>& friends);
	uint32_t get_all_recent(std::vector<recent_t>& recentlist);
	uint32_t get_all_black(std::vector<black_t>& blacklist);
	uint32_t get_all_temps(std::vector<temp_t>& templist);
	bool has_recent(recent_t& recent_data);

	static bool order_by_last_login_time(friend_t f1, friend_t f2); 

private:
	// std::map<userid_t, friend_t> m_friends;             
	// std::map<userid_t, black_t> m_blacklist;             
	// std::map<userid_t, temp_t> m_templist;             
	// std::list<recent_t> l_recentlist;             
	std::map< user_inf_t, friend_t> m_friends;             
	std::map< user_inf_t, black_t> m_blacklist;             
	std::map< user_inf_t, temp_t> m_templist;             
	std::list<recent_t> l_recentlist;             
};

#endif
