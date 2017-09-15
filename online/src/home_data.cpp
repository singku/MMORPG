#include <boost/lexical_cast.hpp>
#include "home_data.h"
#include "service.h"
#include "global_data.h"
#include "player.h"
#include "player_manager.h"
#include "player_utils.h"

uint32_t home_data_t::add_visit_log(const commonproto::visit_log_info_t &log_info)
{
	struct visit_log_info_t  tmp_log;
	tmp_log.host_id_ = log_info.hostid();
	tmp_log.h_create_tm_ = log_info.h_create_tm();
	tmp_log.guest_id_ = log_info.guestid();
	tmp_log.g_create_tm_ = log_info.g_create_tm();
	tmp_log.sex_ = log_info.sex();
	tmp_log.guest_name_.assign(log_info.guestname());
	tmp_log.log_type_ = log_info.logtype();
	tmp_log.date_ = log_info.date();
	tmp_log.detail_.assign(log_info.detail());
	tmp_log.fragment_id_ = log_info.gift_id();
	vlog_map_.insert(make_pair(tmp_log.date_, tmp_log));
	return 0;
}

uint32_t home_data_t::get_visit_log_map_info(visit_log_map_t& log_map)
{
	log_map = vlog_map_;
	return 0;
}

uint32_t home_data_t::notify_friend_ask_for_gift(
	std::vector<role_info_t>& role_vec, userid_t uid)
{
	string tmp_content1 = "您的好友"; 
	string tmp_content2 = "向您索要礼物";
	string total_content;
	total_content = tmp_content1 + boost::lexical_cast<string>(uid) + tmp_content2;

	switchproto::cs_sw_transmit_only sw_in;
	FOREACH(role_vec, it) {
		player_t *player = g_player_manager->get_player_by_userid(it->userid);
		if (player) {
			onlineproto::sc_0x013C_notify_friend_ask_for_gift  cli_msg;
			cli_msg.set_content(total_content);
			send_msg_to_player(player, 
					cli_cmd_cs_0x013C_notify_friend_ask_for_gift, 
					cli_msg);
			return 0;
		}
		switchproto::sw_player_basic_info_t *base_info_ptr = sw_in.add_receivers();
		base_info_ptr->set_userid(it->userid);
		base_info_ptr->set_create_tm(it->u_create_tm);
	}
	sw_in.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
	sw_in.set_cmd(cli_cmd_cs_0x013C_notify_friend_ask_for_gift);
	onlineproto::sc_0x013C_notify_friend_ask_for_gift  cli_msg;
	cli_msg.set_content(total_content);
	string pkg;
	cli_msg.SerializeToString(&pkg);
	sw_in.set_pkg(pkg);
	g_switch->send_msg(
			NULL, g_online_id, 0,
			sw_cmd_sw_transmit_only, sw_in);	
	return 0;	
}

uint32_t home_data_t::add_userid_to_hm_frag_info(uint64_t role_key)
{
	std::set<uint64_t> role_s;
	commonproto::hm_fragment_info msg;
	msg.ParseFromString(str_hm_frag_);
	uint32_t size = msg.friend_role_info_size();
	for (uint32_t i = 0; i < size; ++i) {
		uint64_t role = ROLE_KEY(ROLE(msg.friend_role_info(i).userid(), msg.friend_role_info(i).u_create_tm()));
		role_s.insert(role);
	}
	if (role_s.count(role_key)) {
		return cli_err_hm_gift_has_sent_today;
	}
	str_hm_frag_.clear();
	role_info_t role_info = KEY_ROLE(role_key);
	commonproto::role_info_t* pb_ptr = msg.add_friend_role_info();
	pb_ptr->set_userid(role_info.userid);
	pb_ptr->set_u_create_tm(role_info.u_create_tm);
	msg.SerializeToString(&str_hm_frag_);
	return 0;
}

uint32_t home_data_t::sync_data_when_send_frag(
        uint32_t player_id, uint32_t p_create_tm,
		uint32_t friend_id, uint32_t f_create_tm)
{
	uint64_t f_role_key = ROLE_KEY(ROLE(friend_id, f_create_tm));
	uint32_t ret = add_userid_to_hm_frag_info(f_role_key);
	if (ret) {
		return ret;
	}
	commonproto::hm_fragment_info msg;
	msg.ParseFromString(str_hm_frag_);
	PlayerUtils::update_user_raw_data(
			player_id, p_create_tm, dbproto::SEND_GIFT_FRIEND_ID, 
			msg, "0");
	return 0;
}

uint32_t home_data_t::clean_hm_fragment_info(player_t* player) 
{
	if (player == NULL) {
		return 0;
	}
	commonproto::hm_fragment_info tmp_info;
	if (!TimeUtils::is_same_day(GET_A(kAttrLastLoginTm), NOW())) {
		tmp_info.Clear();
		PlayerUtils::update_user_raw_data(
                player->userid, player->create_tm, dbproto::SEND_GIFT_FRIEND_ID, tmp_info, "0");
	}
	return 0;
}
