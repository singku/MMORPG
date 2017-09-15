#include "proto/client/pb0x06.pb.h"
#include "proto/db/dbproto.friend.pb.h"
#include "proto/cache/cacheproto.pb.h"
#include "friend_utils.h"
#include "friend.h"
#include "global_data.h"
#include "service.h"

//将从cache服务器中读到的信息加入到 前端与online的协议中
//后续如果前端如果需要拉取更多字段，皆可在此扩展
uint32_t FriendUtils::pack_friend_cache_data_to_online(
		const commonproto::player_base_info_t& base_info, 
		commonproto::friend_info_t* friend_info)
{

	friend_info->set_userid(base_info.user_id());
	friend_info->set_create_tm(base_info.create_tm());
	friend_info->set_nick(base_info.nick());

	uint32_t silver_vip_end_time = base_info.silver_vip_end_time();
	uint32_t gold_vip_end_time =  base_info.gold_vip_end_time();
	if (gold_vip_end_time > NOW()) {
		friend_info->set_vip_grade(2);
	} else if (silver_vip_end_time > NOW()){
		friend_info->set_vip_grade(1);
	} else {
		friend_info->set_vip_grade(0);
	}
	// if (base_info.vip_level() > 0) {
		// friend_info->set_is_vip(1);
		// friend_info->set_vip_grade(base_info.vip_level());
	// }
	// else {
		// friend_info->set_is_vip(0);
		// friend_info->set_vip_grade(0);
	// }
	friend_info->set_level(base_info.level());
	friend_info->set_power(base_info.power());
	friend_info->set_last_login_time(base_info.last_login_tm());
	friend_info->set_sex(base_info.sex());
	return 0;
}

//
uint32_t FriendUtils::pack_friend_data_to_player(
		const commonproto::player_base_info_t& base_info, 
		friend_t* friend_info)
{

	friend_info->id = base_info.user_id();
	friend_info->create_tm = base_info.create_tm();
	friend_info->nick = base_info.nick();

	uint32_t silver_vip_end_time = base_info.silver_vip_end_time();
	uint32_t gold_vip_end_time =  base_info.gold_vip_end_time();
	if (gold_vip_end_time > NOW()) {
		friend_info->is_vip = 1;
		friend_info->vip_grade = 2;
	} else if (silver_vip_end_time > NOW()){
		friend_info->vip_grade = 1;
		friend_info->is_vip = 1;
	} else {
		friend_info->vip_grade = 0;
		friend_info->is_vip = 0;
	}
	// if (base_info.vip_level() > 0) {
		// friend_info->is_vip = 1;
		// friend_info->vip_grade = base_info.vip_level();
	// }
	// else {
		// friend_info->is_vip = 0;
		// friend_info->vip_grade = 0;
	// }
	friend_info->level = base_info.level();
	friend_info->power = base_info.power();
	return 0;
}

uint32_t FriendUtils::pack_friend_cache_data_to_recommendation(
		const commonproto::player_base_info_t& base_info, 
		recommendation_t& recommendation)
{
	recommendation.userid = base_info.user_id();
	recommendation.create_tm= base_info.create_tm();
	recommendation.nick = base_info.nick();

	uint32_t silver_vip_end_time = base_info.silver_vip_end_time();
	uint32_t gold_vip_end_time =  base_info.gold_vip_end_time();
	if (gold_vip_end_time > NOW()) {
		recommendation.is_vip = 1;
		recommendation.vip_grade = 2;
	} else if (silver_vip_end_time > NOW()){
		recommendation.is_vip = 1;
		recommendation.vip_grade = 1;
	} else {
		recommendation.is_vip = 0;
		recommendation.vip_grade = 0;
	}
	// if (base_info.vip_level() > 0) {
		// recommendation.is_vip = 1;
		// recommendation.vip_grade = base_info.vip_level();
	// }
	// else {
		// recommendation.is_vip = 0;
		// recommendation.vip_grade = 0;
	// }

	recommendation.level = base_info.level();
	recommendation.power = base_info.power();
	recommendation.sex = base_info.sex();
	recommendation.last_login_time = base_info.last_login_tm();
	return 0;
}

uint32_t FriendUtils::pack_blacklist_data_to_player(
		const commonproto::player_base_info_t& base_info, 
		black_t* black_info)
{

	black_info->id= base_info.user_id();
	black_info->create_tm= base_info.create_tm();
	black_info->nick = base_info.nick();

	uint32_t silver_vip_end_time = base_info.silver_vip_end_time();
	uint32_t gold_vip_end_time =  base_info.gold_vip_end_time();
	if (gold_vip_end_time > NOW()) {
		black_info->is_vip = 1;
		black_info->vip_grade = 2;
	} else if (silver_vip_end_time > NOW()){
		black_info->is_vip = 1;
		black_info->vip_grade = 1;
	} else {
		black_info->is_vip = 0;
		black_info->vip_grade = 0;
	}
	// if (base_info.vip_level() > 0) {
		// black_info->is_vip = 1;
		// black_info->vip_grade = base_info.vip_level();
	// }
	// else {
		// black_info->is_vip = 0;
		// black_info->vip_grade = 0;
	// }
	black_info->level = base_info.level();	
	return 0;
}
//当recent列表满时
//1 当前要提到recent列表最前边的用户在列表中：直接将该用户删除，然后再加入到列表最前边
//2 当前要提到recent列表最前边的用户不在列表中：删除排在最后的用户，添加新用户到最前端
//当recent列表未满时
//直接添加或者移动该用户到最前边
//修改对应的数据库
//注意:因为最后将id列表存入与客户端同步的协议中时，是逆序的，所以上述描述过程与代码前后相反
uint32_t FriendUtils::proc_recent_list(player_t* player,
		uint32_t recent_id, uint32_t create_tm)
{
	uint32_t size = player->friend_info->get_recent_size();	
	recent_t recent_front;
	if (size >= 10) {
		//修改内存中数据
		recent_t temp_recent;
		temp_recent.id = recent_id;
		temp_recent.create_tm= create_tm;

		bool is_in_recent = player->friend_info->has_recent(temp_recent);
		if (is_in_recent == false) {
			uint32_t ret = player->friend_info->get_recent_front(recent_front);	
			if (ret) {
				send_err_to_player(player, player->cli_wait_cmd, ret);
			}
			player->friend_info->remove_recent_front();	
		}
		else {
			player->friend_info->remove_recent_element(temp_recent);
		}
		player->friend_info->add_recent_back(temp_recent);
		
		//与客户端 同步
		onlineproto::sc_0x0602_sync_friend_data sc_0x0602_sync_friend_data_;	
		
		std::vector<recent_t> recentlist;
		player->friend_info->get_all_recent(recentlist);

		for (int i = recentlist.size() - 1; i >= 0; i--) {
			// sc_0x0602_sync_friend_data_.add_recent_list(recentlist[i].id);
			commonproto::role_info_t *info = sc_0x0602_sync_friend_data_.add_recent_list();
			info->set_userid(recentlist[i].id);
			info->set_u_create_tm(recentlist[i].create_tm);
		}
		send_msg_to_player(player, cli_cmd_cs_0x0602_sync_friend_data, sc_0x0602_sync_friend_data_);
		
		if (is_in_recent == false) {
			//写数据库
			// dbproto::cs_save_friend cs_save_friend1_;
			// cs_save_friend1_.set_userid(player->userid);
			// cs_save_friend1_.set_friendid(recent_id);
			// cs_save_friend1_.set_is_friend(1);
			// cs_save_friend1_.set_is_recent(1);
			// cs_save_friend1_.set_is_black(0);
			// cs_save_friend1_.set_is_temp(0);

			dbproto::cs_save_friend cs_save_friend1_;
			commonproto::friend_data_t finf1;

			finf1.set_userid(player->userid);
			finf1.set_u_create_tm(player->create_tm);
			finf1.set_friendid(recent_id);
			finf1.set_f_create_tm(create_tm);
			finf1.set_is_friend(1);
			finf1.set_is_recent(1);
			finf1.set_is_blacklist(0);
			finf1.set_is_temp(0);

			cs_save_friend1_.mutable_finf()->CopyFrom(finf1);

			g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
					db_cmd_save_friend , cs_save_friend1_);  

			// dbproto::cs_save_friend cs_save_friend2_;
			// cs_save_friend2_.set_userid(player->userid);
			// cs_save_friend2_.set_friendid(recent_front.id);
			// cs_save_friend2_.set_is_friend(1);
			// cs_save_friend2_.set_is_recent(0);
			// cs_save_friend2_.set_is_black(0);
			// cs_save_friend2_.set_is_temp(0);

			dbproto::cs_save_friend cs_save_friend2_;
			commonproto::friend_data_t finf2;

			finf2.set_userid(player->userid);
			finf2.set_u_create_tm(player->create_tm);
			finf2.set_friendid(recent_id);
			finf2.set_f_create_tm(create_tm);
			finf2.set_is_friend(1);
			finf2.set_is_recent(0);
			finf2.set_is_blacklist(0);
			finf2.set_is_temp(0);

			cs_save_friend2_.mutable_finf()->CopyFrom(finf2);
			g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
					db_cmd_save_friend , cs_save_friend2_);  
		}
	}
	else {
		//修改内存数据
		recent_t temp_recent;
		temp_recent.id = recent_id;
		temp_recent.create_tm = create_tm;

		bool is_in_recent = player->friend_info->has_recent(temp_recent);

		if (is_in_recent == true) {
			player->friend_info->remove_recent_element(temp_recent);
		}

		player->friend_info->add_recent_back(temp_recent);

		//与客户端 同步
		onlineproto::sc_0x0602_sync_friend_data sc_0x0602_sync_friend_data_;	

		std::vector<recent_t> recentlist;
		player->friend_info->get_all_recent(recentlist);
		for (int i = recentlist.size() - 1; i >= 0; i--) {
			// sc_0x0602_sync_friend_data_.add_recent_list(recentlist[i].id);
			commonproto::role_info_t *info = sc_0x0602_sync_friend_data_.add_recent_list();
			info->set_userid(recentlist[i].id);
			info->set_u_create_tm(recentlist[i].create_tm);
		}
		send_msg_to_player(player, cli_cmd_cs_0x0602_sync_friend_data, sc_0x0602_sync_friend_data_);

		if (is_in_recent == false) {
			//写数据库
			// dbproto::cs_save_friend cs_save_friend1_;
			// cs_save_friend1_.set_userid(player->userid);
			// cs_save_friend1_.set_friendid(recent_id);
			// cs_save_friend1_.set_is_friend(1);
			// cs_save_friend1_.set_is_recent(1);
			// cs_save_friend1_.set_is_black(0);
			// cs_save_friend1_.set_is_temp(0);

			dbproto::cs_save_friend cs_save_friend1_;
			commonproto::friend_data_t finf;

			finf.set_userid(player->userid);
			finf.set_u_create_tm(player->create_tm);
			finf.set_friendid(recent_id);
			finf.set_f_create_tm(create_tm);
			finf.set_is_friend(1);
			finf.set_is_recent(0);
			finf.set_is_blacklist(0);
			finf.set_is_temp(0);

			cs_save_friend1_.mutable_finf()->CopyFrom(finf);
			g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
					db_cmd_save_friend , cs_save_friend1_);  
		}
	}
	return 0;
}
