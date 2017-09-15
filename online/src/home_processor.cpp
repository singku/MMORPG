#include "home_processor.h"
#include "player.h"
#include "service.h"
#include "data_proto_utils.h"
#include "global_data.h"
#include "map_utils.h"
#include "home_data.h"
#include "attr_utils.h"
#include "player_manager.h"
#include "attr.h"
#include "friend.h"
#include "home_gift_conf.h"
#include "item.h"
#include "player_utils.h"
#include <algorithm>

int AccessHomeCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	//如果已经在小屋，则先通知home,先离开	
	role_info_t orig_host = player->home_data->at_whos_home();
	if (orig_host.userid && orig_host.u_create_tm) {
		return PlayerUtils::leave_current_home(player, true);
	}


	homeproto::cs_enter_home home_msg;
	home_msg.set_host_id(cli_in_.hostid());
	DataProtoUtils::pack_map_player_info(player, home_msg.mutable_player_info());
	home_msg.set_u_create_tm(cli_in_.h_create_tm());

	return g_dbproxy->send_msg(
			player, cli_in_.hostid(), cli_in_.h_create_tm(), 
			home_cmd_enter_home, home_msg, 
			player->userid);
}

int AccessHomeCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen) 
{
	switch(player->serv_cmd) {
		case home_cmd_enter_home:
			return proc_pkg_from_serv_aft_get_home_info(player, body, bodylen);
		case db_cmd_pet_list_get:
			return proc_pkg_from_serv_aft_get_host_pet_info(player, body, bodylen);
		case home_cmd_exit_home:
			return proc_pkg_from_serv_aft_exit_home(player, body, bodylen);
		default:
			return 0;
	}
}

int AccessHomeCmdProcessor::proc_pkg_from_serv_aft_get_home_info(
	player_t* player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(home_out_);

	//离开现在的公共地图
	MapUtils::leave_map(player);
	MapUtils::enter_map(player, home_out_.hometype(), home_out_.x_pos(), home_out_.x_pos());
	cli_out_.Clear();
	cli_out_.set_hostid(home_out_.hostid());
	cli_out_.set_h_create_tm(home_out_.u_create_tm());
	cli_out_.set_hostname(home_out_.name());
	cli_out_.set_hometype(home_out_.hometype());
	cli_out_.mutable_player_data_list()->CopyFrom(home_out_.players());

	player->home_data->set_at_home(ROLE(home_out_.hostid(), home_out_.u_create_tm()));
	//return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);

	//去DB拉取房屋主人的精灵信息
	dbproto::cs_pet_list_get_info db_in_;
	db_in_.Clear();
	return g_dbproxy->send_msg(
            player, home_out_.hostid(), home_out_.u_create_tm(), 
			db_cmd_pet_list_get, db_in_);
}

//因为小屋锻炼案子不要了，所以该协议处理暂时不用
int AccessHomeCmdProcessor::proc_pkg_from_serv_aft_get_host_pet_info(
	player_t* player, const char* body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	const commonproto::pet_list_t& pet_inf = db_out_.pet_list();
	uint32_t pet_list_size = pet_inf.pet_list_size();
	for (uint32_t i = 0; i < pet_list_size; ++i) {
		uint32_t exerciese_pos = pet_inf.pet_list(i).base_info().exercise_pos();
		uint16_t type = 0, pos = 0;
		Utils::decomp_u16(exerciese_pos, type, pos);
		if (type != HM_PET_EXERCISE) {
			continue;
		}
		commonproto::pet_info_t* pet_ptr = cli_out_.mutable_pet_list()->add_pet_list();
		pet_ptr->CopyFrom(pet_inf.pet_list(i));
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int AccessHomeCmdProcessor::proc_pkg_from_serv_aft_exit_home(
	player_t* player, const char* body, int bodylen)
{
	homeproto::cs_enter_home home_msg;
	home_msg.set_host_id(cli_in_.hostid());
	DataProtoUtils::pack_map_player_info(player, home_msg.mutable_player_info());
	home_msg.set_x_pos(player->map_x);
	home_msg.set_y_pos(player->map_y);
	home_msg.set_u_create_tm(cli_in_.h_create_tm());

	return g_dbproxy->send_msg(
			player, cli_in_.hostid(), cli_in_.h_create_tm(), 
			home_cmd_enter_home, home_msg, 
			player->userid);
}

int HmAddVisitLogCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	//服务端设置访问时间
	cli_out_.Clear();
	send_msg_to_player(player, cli_cmd_cs_0x0133_add_visit_log, cli_out_);
	cli_in_.mutable_visit_log_info()->set_date(NOW());
	const commonproto::visit_log_info_t &log_info = cli_in_.visit_log_info();
	uint32_t host_id = log_info.hostid();
	uint32_t h_create_tm = log_info.h_create_tm();
	if (log_info.logtype() == commonproto::SHOW_FLOWER ||
		log_info.logtype() == commonproto::THROW_EGG) {
		if (GET_A(kDailyHmOpraCount) >= DAILY_HM_OPRA_COUNT) {
			return send_err_to_player(player,
					player->cli_wait_cmd, 
					cli_err_hm_opra_count_get_limit);
		}
	}

	char content[4096];
	STRCPY_SAFE(content, cli_in_.visit_log_info().detail().c_str());
	tm_dirty_replace(content);
	cli_in_.mutable_visit_log_info()->set_detail(std::string(content));

	if (log_info.logtype() == commonproto::FRAGMENT) {
		//目前每日送礼最多20次
		if (GET_A(kDailyHmSendGift) >= DAILY_SEND_GIFT_LIMIT) {
			return send_err_to_player(
					player, player->cli_wait_cmd, 
					cli_err_daily_hm_send_gift_num_ex_limit);
		}
		friend_t tmp_friend;
		//判定是否关注了对方
		uint32_t ret = player->friend_info->get_friend_by_id(host_id, h_create_tm, tmp_friend);
		if (ret) {
			return send_err_to_player(player, player->cli_wait_cmd, ret);
		}
		dbproto::cs_get_attr db_msg;
		db_msg.add_type_list(kDailyHmRecvGift);
		uint32_t begin = kAttrHmGiftTrial1;
		uint32_t end = kAttrHmGiftTrial10;
		for (uint32_t index = begin; index <= end; ++index) {
			db_msg.add_type_list(index);
		}
		return g_dbproxy->send_msg(
				player, host_id, h_create_tm,
				db_cmd_get_attr, db_msg);
	}
	onlineproto::sc_0x0134_inform_new_visit inform_msg;
	inform_msg.mutable_visit_log_info()->CopyFrom(cli_in_.visit_log_info());	
	player_t *host_p = g_player_manager->get_player_by_userid(host_id);	
	if (host_p) {
		host_p->home_data->add_visit_log(log_info);
		send_msg_to_player(
				host_p, cli_cmd_cs_0x0134_inform_new_visit, inform_msg);
	} else {
		homeproto::cs_gen_home_visit_log gen_log_msg;
		std::string pkg;
		inform_msg.SerializeToString(&pkg);		
		gen_log_msg.set_pkg(pkg);
		gen_log_msg.set_userid(player->userid);
		gen_log_msg.set_create_tm(player->create_tm);
		gen_log_msg.set_dest_uid(host_id);
		gen_log_msg.set_dest_create_tm(h_create_tm);
		g_dbproxy->send_msg(
				0, host_id, h_create_tm,  home_cmd_gen_visit_log, 
				gen_log_msg, player->userid);
	}
	dbproto::cs_add_visit_log db_msg;
	db_msg.mutable_log_info()->CopyFrom(cli_in_.visit_log_info());
	g_dbproxy->send_msg(0, host_id, h_create_tm, db_cmd_hm_add_visit_log, db_msg);	

	cli_out_.Clear();
	cli_out_.set_type(log_info.logtype());
	if (log_info.logtype() == commonproto::SHOW_FLOWER ||
		log_info.logtype() == commonproto::THROW_EGG) {
		ADD_A(kDailyHmOpraCount, 1);
	}
	send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

int HmAddVisitLogCmdProcessor :: proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	const commonproto::visit_log_info_t &log_info = cli_in_.visit_log_info();
	if (log_info.logtype() != commonproto::FRAGMENT) {
		ERROR_TLOG("recv frag count:but visit log type not fragment"
				"uid=[%u]type=[%u]", player->userid, (uint32_t)log_info.logtype());
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_sys_err);
	}
	uint32_t host_id = log_info.hostid();
	uint32_t h_create_tm = log_info.h_create_tm();
	uint32_t recv_count = 0;
	std::map<uint32_t, uint32_t> frag_cnt_map;
	for (int i = 0; i < db_out_.attrs_size(); ++i) {
		const commonproto::attr_data_t &inf = db_out_.attrs(i);
		if (inf.type() == kDailyHmRecvGift) {
			recv_count = inf.value();
			continue;
		}
		frag_cnt_map.insert(make_pair(inf.type(), inf.value()));
	}
	if (recv_count >= DAILY_RECV_GIFT_LIMIT) {
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_friend_recv_gift_get_daily_limit);
	}
	//添加好友到已赠送的名单里
	uint32_t ret = player->home_data->sync_data_when_send_frag(
			player->userid, player->create_tm, host_id, h_create_tm);
	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd, ret);
	}
	//随机出物品碎片
	std::set<uint32_t> display_hit_idx_set;
	std::vector<uint32_t> hm_gift_vec;
	hm_gift_vec = g_hm_gift_mgr.get_home_gift_rate_vec();
    std::map<uint32_t, uint32_t> award_rate_map;
	uint32_t idx = 1;
	FOREACH(hm_gift_vec, it) {
		award_rate_map[idx] = *it;
		++idx;
	}
	Utils::rand_select_uniq_m_from_n_with_r(award_rate_map, display_hit_idx_set, 1);
	std::set<uint32_t>::iterator it = display_hit_idx_set.begin();
	uint32_t fragment_id = *it;
	home_gift_conf_t* hm_info_ptr = g_hm_gift_mgr.get_hm_gift_info(fragment_id);
	if (hm_info_ptr == NULL) {
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_hm_fragment_id_err);
    }
	uint32_t attr_frag_id = FRAGMENT_INDEX + fragment_id;
	if (hm_info_ptr->need_count > frag_cnt_map[attr_frag_id]) {
		//好友的fragment_id碎片数量 + 1;这里不走物品系统，只是属性加1
		AttrUtils::change_other_attr_value_pub(
				host_id, h_create_tm,
				attr_frag_id, 1, false);
		//好友的收礼数量 + 1
		AttrUtils::change_other_attr_value_pub(
				host_id, h_create_tm,
				kDailyHmRecvGift, 1, false);
	}

	//自己送礼次数 + 1
	ADD_A(kDailyHmSendGift, 1);

	//访客日志：若是好友送碎片，则添加碎片在属性表中的id
	cli_in_.mutable_visit_log_info()->set_gift_id(attr_frag_id);
	onlineproto::sc_0x0134_inform_new_visit inform_msg;
	inform_msg.mutable_visit_log_info()->CopyFrom(cli_in_.visit_log_info());	
	player_t *host_p = g_player_manager->get_player_by_userid(host_id);	
	if (host_p) {
		host_p->home_data->add_visit_log(log_info);
		send_msg_to_player(
				host_p, cli_cmd_cs_0x0134_inform_new_visit, inform_msg);
	} else {
		homeproto::cs_gen_home_visit_log gen_log_msg;
		std::string pkg;
		inform_msg.SerializeToString(&pkg);		
		gen_log_msg.set_userid(player->userid);
		gen_log_msg.set_create_tm(player->create_tm);
		gen_log_msg.set_dest_uid(host_id);
		gen_log_msg.set_dest_create_tm(h_create_tm);
		gen_log_msg.set_pkg(pkg);
		g_dbproxy->send_msg(
				0, host_id, h_create_tm, home_cmd_gen_visit_log, 
				gen_log_msg, player->userid);
	}
	dbproto::cs_add_visit_log db_msg;
	db_msg.mutable_log_info()->CopyFrom(cli_in_.visit_log_info());
	g_dbproxy->send_msg(0, host_id, 0, db_cmd_hm_add_visit_log, db_msg);	

	cli_out_.Clear();
	cli_out_.set_type(log_info.logtype());
	std::string str = player->home_data->get_hm_frag();
	cli_out_.mutable_list()->ParseFromString(str);
	send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	return 0;
}

int HmGetVisitLogCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	cli_out_.Clear();
	dbproto::cs_get_visit_log db_msg;
	g_dbproxy->send_msg(
			player, player->userid, player->create_tm,
			db_cmd_get_hm_visit_log, db_msg);

	return 0;	
}

int HmGetVisitLogCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(db_out_);
	DataProtoUtils::unpack_hm_visit_log(player, db_out_.log_list());
	DataProtoUtils::unpack_hm_fragment_info(player, db_out_.buff());

	cli_out_.Clear();
	DataProtoUtils::pack_hm_visit_log(player, cli_out_.mutable_visit_log_list());
	std::string str = player->home_data->get_hm_frag();
	cli_out_.mutable_list()->ParseFromString(str);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int HmAskForGiftCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	cli_out_.Clear();
	std::vector<role_info_t> role_vec;

	std::vector<friend_t> friend_vec;
	player->friend_info->get_all_friends(friend_vec);
	std::sort(friend_vec.begin(), friend_vec.end(), 
			player->friend_info->order_by_last_login_time);
	std::vector<friend_t>::iterator it = friend_vec.begin();
	for (; it != friend_vec.end(); ++it) {
		onlineproto::ask_gift_info* gift_ptr = cli_out_.add_ask_list();	
		if (it->is_online) {
			gift_ptr->set_flag(onlineproto::ASK_FOR_GIFT_SUCESS);
			role_info_t role_info;
			role_info.userid= it->id;
			role_info.u_create_tm = it->create_tm;
			role_vec.push_back(role_info);
		} else {
			gift_ptr->set_flag(onlineproto::ASK_FOR_GIFT_FAIL);
		}
		gift_ptr->set_friend_id(it->id);
		gift_ptr->set_friend_c_tm(it->create_tm);
	}
	player->home_data->notify_friend_ask_for_gift(role_vec, player->userid);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int HmGiftExchgCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	cli_out_.Clear();
	uint32_t fragment_id = cli_in_.fragment_id();
	home_gift_conf_t* hm_info_ptr = g_hm_gift_mgr.get_hm_gift_info(fragment_id);
	if (hm_info_ptr == NULL) {
		return send_err_to_player(
				player, player->cli_wait_cmd, 
				cli_err_hm_fragment_id_err);
	}
	uint32_t own_frag_count = GET_A((attr_type_t)(FRAGMENT_INDEX + fragment_id));
	if (own_frag_count < hm_info_ptr->need_count) {
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_frag_count_not_enough);
	}
	
	int ret = add_single_item(player, hm_info_ptr->to_item_id, hm_info_ptr->count);
	if (ret) {
		return send_err_to_player(
				player, player->cli_wait_cmd,
				cli_err_hm_gift_exchg_item_err);
	}
	SET_A((attr_type_t)(FRAGMENT_INDEX + fragment_id), 0);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
