#include "friend.h"
FriendInfo::FriendInfo() {

}

FriendInfo::~FriendInfo() {

}
friend_t& friend_t::operator=(const friend_t& other) {
	if (this == &other) {
		return *this;
	}
	id = other.id;
	create_tm = other.create_tm;
	nick = other.nick;
	is_vip = other.is_vip;
	is_online = other.is_online;
	vip_grade = other.vip_grade;
	//gift_count = other.gift_count;
	last_login_tm = other.last_login_tm;
	
	//power = other.power;
	return *this;

}

recommendation_t& recommendation_t::operator=(const recommendation_t& other) {
	if (this == &other) {
		return *this;
	}
	userid = other.userid;
	create_tm = other.create_tm;
	nick = other.nick;
	is_vip = other.is_vip;
	is_online = other.is_online;
	vip_grade = other.vip_grade;
	power = other.power;
	team = other.team;	
	level = other.level;
	last_login_time = other.last_login_time;
	sex = other.sex;
	return *this;
}

bool recent_t::operator==(const recent_t& other) {
	if (this->id == other.id && this->create_tm == other.create_tm) {
		return true;
	}
	else {
		return false;
	}
}
uint32_t FriendInfo::get_all_friends(std::vector<friend_t>& friends) {
	for (std::map< user_inf_t, friend_t>::iterator it = m_friends.begin(); it != m_friends.end(); it++){
		friends.push_back(it->second);
	}
	return 0;
}


uint32_t FriendInfo::get_all_recent(std::vector<recent_t>& recentlist) {
	for (std::list<recent_t>::iterator it = l_recentlist.begin(); it != l_recentlist.end(); it++){
		recentlist.push_back(*it);
	}
	return 0;
}

uint32_t FriendInfo::get_all_black(std::vector<black_t>& blacklist) {
	for (std::map< user_inf_t, black_t>::iterator it = m_blacklist.begin(); it != m_blacklist.end(); it++){
		blacklist.push_back(it->second);
	}
	return 0;
}

uint32_t FriendInfo::get_all_temps(std::vector<temp_t>& templist) {
	for (std::map< user_inf_t, temp_t>::iterator it = m_templist.begin(); it != m_templist.end(); it++){
		templist.push_back(it->second);
	}
	return 0;
}

bool FriendInfo::has_recent(recent_t& recent_data) {
	for (std::list<recent_t>::iterator it = l_recentlist.begin(); it != l_recentlist.end(); it++){
		if (recent_data == *it) {
			return true;
		}
	}
	return false;
}

bool FriendInfo::order_by_last_login_time(friend_t f1, friend_t f2)
{
	return f1.last_login_tm > f2.last_login_tm;
}
