extern "C" {
#include <libtaomee/tm_dirty/tm_dirty.h>
}
#include <libtaomee++/utils/strings.hpp>
#include "chat_processor.h"
#include "utils.h"
#include "attr.h"
#include "player.h"
#include "player_manager.h"
#include "proto/client/cli_errno.h"
#include "proto/client/common.pb.h"
#include "chat_processor.h"
#include "map_utils.h"
#include "global_data.h"
#include "service.h"
#include "pet_utils.h"
#include "data_proto_utils.h"
#include "item.h"
#include "proto/db/dbproto.chat.pb.h"
#include "player_utils.h"
#include "family_utils.h"
#include "rank_utils.h"
#include "prize.h"

//展示精灵尚未调试
int SayCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;

	onlineproto::say_type_t say_type = cli_in_.type();

	STRCPY_SAFE(content_, cli_in_.content().c_str());
	//检测脏词
	tm_dirty_replace(content_);
	//打包协议
	cli_out_.Clear();
	cli_out_.set_type(say_type);
	cli_out_.set_content(std::string(content_));
	cli_out_.set_userid(player->userid);
	cli_out_.set_create_tm(player->create_tm);
	cli_out_.set_name(player->nick);
	uint32_t sex = AttrUtils::get_attr_value(player, kAttrSex);
	cli_out_.set_sex(sex);

	//判断vip类型
	if (is_gold_vip(player)) {
		cli_out_.set_vip_type(commonproto::GOLD_VIP);
	} else if (is_silver_vip(player)) {
		cli_out_.set_vip_type(commonproto::SILVER_VIP);
	} else {
		cli_out_.set_vip_type(commonproto::NOT_VIP);
	}

	cli_out_.set_serverid(g_online_id);

    if (g_test_for_robot) {
        string ret_content;
		int ret = 0;
		ret = PlayerUtils::player_say_cmd(player, cli_in_.content(), ret_content);
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd, ret);
        } else if (!ret_content.empty()) {//cmd命令
            cli_out_.set_content(ret_content);
            return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
        }
    }
	switch (say_type) {
		//当前地图广播
		case onlineproto::SAY_TYPE_MAP: {
				return say_map(player);
				break;
			}
		//单服世界广播
		case onlineproto::SAY_TYPE_WORLD: {
				return say_world(player);
				break;
			}
		// 家族广播
		case onlineproto::SAY_TYPE_FAMILY: {
				return say_family(player);
				break;
			}
		default: {
			return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
		}
	}
}

int  SayCmdProcessor::say_world(player_t* player)
{
	temp_info_t* temp_info = &player->temp_info;
	uint32_t now = NOW();
	//检查CD时间
	if (temp_info->last_chat_time + kWorldChatCD >= now) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_say_too_fast);
	}

	temp_info->last_chat_time = now;
	//单服世界广播
	std::vector<player_t*> player_list;
	g_player_manager->get_player_list(player_list);

	for (uint32_t i = 0; i < player_list.size(); i++) {
		player_t* ply = player_list[i];
		send_msg_to_player(ply, player->cli_wait_cmd, cli_out_);
	}
	return 0;	
}

int SayCmdProcessor::say_map(player_t* player)
{
	temp_info_t* temp_info = &player->temp_info;
	uint32_t now = NOW();
	//检查CD时间
	if (temp_info->last_chat_time + kMapChatCD >= now) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_say_too_fast);
	}
	temp_info->last_chat_time = now;

	if (!MapUtils::is_player_in_map(player)) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_user_not_in_map);
	}
	return MapUtils::send_msg_to_map_users(player,
			player->cli_wait_cmd, cli_out_);
}

int SayCmdProcessor::say_family(player_t* player)
{
	uint32_t family_id = GET_A(kAttrFamilyId);
	if (!(FamilyUtils::is_valid_family_id(family_id))) {
		return send_err_to_player(
				player, player->cli_wait_cmd, cli_err_family_id_illegal);
	}

    return RankUtils::set_get_all_member(
            player, rankproto::SET_FAMILY_ONLINE_USERIDS, family_id);
}

int SayCmdProcessor::proc_pkg_from_serv(                             
		player_t* player, const char* body, int bodylen)             
{
    switch (player->serv_cmd) {
		case ranking_cmd_set_get_all_member:
			return proc_pkg_from_serv_aft_get_family_olmembers(player, body, bodylen);
        default:
            return 0;
    }
	return 0;                                                        
}

int SayCmdProcessor::proc_pkg_from_serv_aft_get_family_olmembers(                             
		player_t* player, const char* body, int bodylen)             
{
    rankproto::sc_set_get_all_member     rank_member_out_;
    PARSE_SVR_MSG(rank_member_out_);

	//打包给switch并向全服发广播
	switchproto::cs_sw_transmit_only cs_sw_transmit_only_;	

    for (int i = 0; i < rank_member_out_.members_size();i++) {
        //uint32_t userid = atoi(rank_member_out_.members(i).c_str());
		uint64_t role_key = atol(rank_member_out_.members(i).c_str());
        uint32_t userid = KEY_ROLE(role_key).userid;
        uint32_t u_create_tm = KEY_ROLE(role_key).u_create_tm;
		//判断是否是同服在线
		player_t* ply = g_player_manager->get_player_by_userid(userid);
		if (ply != NULL){
			send_msg_to_player(ply, player->cli_wait_cmd, cli_out_);
			continue;
		}
		switchproto::sw_player_basic_info_t* sw_player_basic_info = 
        cs_sw_transmit_only_.add_receivers();
		sw_player_basic_info->set_userid(userid);
		sw_player_basic_info->set_create_tm(u_create_tm);
    }
	//需要跨服发消息
	if (0 != cs_sw_transmit_only_.receivers_size()){
		cs_sw_transmit_only_.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
		cs_sw_transmit_only_.set_cmd(player->cli_wait_cmd);
		std::string pkg;
		cli_out_.SerializeToString(&pkg);                           
		cs_sw_transmit_only_.set_pkg(pkg);                                    

		return	g_switch->send_msg(NULL, g_online_id, 0, sw_cmd_sw_transmit_only,                      
				cs_sw_transmit_only_);    
	} 
	return 0;
}
int ShowItemCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen) 
{
    PARSE_MSG;
	//根据slot_id和userid展示物品,向db发包拉取信息
	uint32_t slot_id = cli_in_.slot_id();
	uint32_t other_uid = cli_in_.userid();
	uint32_t other_create_tm = cli_in_.create_tm();
	
	dbproto::cs_show_item cs_show_item_;
	cs_show_item_.set_slot_id(slot_id);
	return g_dbproxy->send_msg(player, other_uid, other_create_tm, db_cmd_show_item, cs_show_item_);
}


int ShowItemCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen) 
{
	dbproto::sc_show_item sc_show_item_;
    PARSE_SVR_MSG(sc_show_item_);

	if (sc_show_item_.has_item_data() == false) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_item_no_longer_exist);

	}
	//将拉取到的信息返回给客户端
	commonproto::item_info_t* item_data = sc_show_item_.mutable_item_data();
	cli_out_.Clear();
	cli_out_.mutable_item_data()->CopyFrom(*item_data);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}


//全服小喇叭的广播
int BroadcastCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;

	STRCPY_SAFE(content_, cli_in_.content().c_str());
	//检测脏词
	tm_dirty_replace(content_);
	
	
	//打包协议信息
	uint32_t vip_level = AttrUtils::get_attr_value(
			player, kAttrVipLevel);

	cli_out_.Clear();
	cli_out_.set_content(std::string(content_));
	cli_out_.set_userid(player->userid);
	cli_out_.set_name(player->nick);
	uint32_t sex = AttrUtils::get_attr_value(player, kAttrSex);
	cli_out_.set_sex(sex);

	if (is_vip(player)) {
		cli_out_.set_vip(vip_level);
	} 
	else {
		cli_out_.set_vip(0);
	}
	cli_out_.set_serverid(g_online_id);

	return broadcast(player);
}



int BroadcastCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
	return 0;
}

int BroadcastCmdProcessor::broadcast(player_t* player)
{
	temp_info_t* temp_info = &player->temp_info;
	//检查CD，防止过快向switch广播信息
	uint32_t now = NOW();
	if (temp_info->last_chat_time + kBroadcastCD >= now) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_say_too_fast);
	}
	//减全服广播的道具喇叭
	int ret = 0;
	const uint32_t item_cnt = 1;
	ret = reduce_single_item(player, BroadcastID, item_cnt);
	//道具喇叭物品不足,扣除10钻石
	if(cli_err_no_enough_item_num == ret){
        attr_type_t attr_type = kServiceBuyBroadcastDiamond;	
        const uint32_t product_id = 90013;	
        ret = buy_attr_and_use(player, attr_type, product_id, item_cnt);
	}

	if (ret) {
		return send_err_to_player(player, player->cli_wait_cmd,
					ret);
	}
	//记录本次广播的时间 
	temp_info->last_chat_time = now;
	//打包给switch并向全服发广播
	switchproto::cs_sw_transmit_only cs_sw_transmit_only_;	
	cs_sw_transmit_only_.set_transmit_type(switchproto::SWITCH_TRANSMIT_WORLD);
	cs_sw_transmit_only_.set_cmd(player->cli_wait_cmd);
	std::string pkg;                                                      
	cli_out_.SerializeToString(&pkg);                           
	cs_sw_transmit_only_.set_pkg(pkg);                                    

	g_switch->send_msg(NULL, g_online_id, 0, sw_cmd_sw_transmit_only,                      
			cs_sw_transmit_only_);    
	return 0;
}

int ShowPetCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen) 
{
	PARSE_MSG;
	uint32_t pet_id = cli_in_.pet_id();
	uint32_t other_userid = cli_in_.userid();
	uint32_t other_create_tm = cli_in_.create_tm();
	//根据userid和pet_id向db拉取精灵信息	
	dbproto::cs_show_pet cs_show_pet_;
	cs_show_pet_.set_pet_id(pet_id);
	return g_dbproxy->send_msg(
			player, other_userid, other_create_tm, 
			db_cmd_show_pet, cs_show_pet_);
}


int ShowPetCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen) 
{
    dbproto::sc_show_pet sc_show_pet_;
    PARSE_SVR_MSG(sc_show_pet_);

	if (sc_show_pet_.has_pet_data() == false) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_bag_pet_not_exist);

	}
	//将db拉取到的精灵信息返回给客户端
	commonproto::pet_info_t* pet_data = sc_show_pet_.mutable_pet_data();
	cli_out_.Clear();
	cli_out_.mutable_pet_data()->CopyFrom(*pet_data);
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int SystemNotify::SystemNotifyForPrize(player_t* player,
        const google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> *award_elems,
        commonproto::system_noti_reason_t reason)
{
	onlineproto::sc_0x0611_system_noti noti;
	noti.set_userid(player->userid);
	noti.set_create_tm(player->create_tm);
	noti.set_name(player->nick);
	noti.set_reason(reason);
    noti.mutable_award_elems()->CopyFrom(*award_elems);
    noti.set_mon_id(player->temp_info.dup_kill_mon_id);

	switchproto::cs_sw_transmit_only cs_sw_transmit_only_;	
	cs_sw_transmit_only_.set_transmit_type(switchproto::SWITCH_TRANSMIT_WORLD);
	cs_sw_transmit_only_.set_cmd(cli_cmd_cs_0x0611_system_noti);
	std::string pkg;                                                      
	noti.SerializeToString(&pkg);                           
	cs_sw_transmit_only_.set_pkg(pkg);                                    

	g_switch->send_msg(NULL, g_online_id, 0, sw_cmd_sw_transmit_only,                      
			cs_sw_transmit_only_);    
	return 0;
}

int SystemNotify::SystemNotifyForCachePrize(player_t* player, 
	   	const cache_prize_elem_t &data, commonproto::system_noti_reason_t reason)
{
	onlineproto::sc_0x0611_system_noti noti;
	noti.set_userid(player->userid);
	noti.set_create_tm(player->create_tm);
	noti.set_name(player->nick);
	noti.set_reason(reason);
	commonproto::prize_elem_t *inf = noti.add_award_elems();

	commonproto::prize_elem_type_t type = (commonproto::prize_elem_type_t)data.type;
	inf->set_type(type);
	inf->set_id(data.id);
	//如果需要其他数据可以添加
	if(type == commonproto::PRIZE_ELEM_TYPE_ITEM
			|| type == commonproto::PRIZE_ELEM_TYPE_ATTR){

		inf->set_count(data.count);
	} else if (type == commonproto::PRIZE_ELEM_TYPE_PET){
		inf->set_level(data.level);
		inf->set_talent_level(data.talent_level);
		inf->set_count(1);
	} else if (type == commonproto::PRIZE_ELEM_TYPE_TITLE){

	} else if (type == commonproto::PRIZE_ELEM_TYPE_RUNE){
		inf->set_level(data.level);
	}
	switchproto::cs_sw_transmit_only cs_sw_transmit_only_;	
	cs_sw_transmit_only_.set_transmit_type(switchproto::SWITCH_TRANSMIT_WORLD);
	cs_sw_transmit_only_.set_cmd(cli_cmd_cs_0x0611_system_noti);
	std::string pkg;                                                      
	noti.SerializeToString(&pkg);                           
	cs_sw_transmit_only_.set_pkg(pkg);                                    

	g_switch->send_msg(NULL, g_online_id, 0, sw_cmd_sw_transmit_only,                      
			cs_sw_transmit_only_);    
	return 0;
}

/** 
 * @brief 跑马灯通知
 * 
 * @param player 
 * @param info 
 * @param mcast_type 0 单服 1全服 2 指定地图
 * 
 * @return 
 */
int SystemNotify::SystemNotifyNormal(
        std::string &info, uint32_t mcast_type, uint32_t map_id)
{
	onlineproto::sc_0x0611_system_noti noti;
	noti.set_info(info);
	noti.set_reason(commonproto::SYSTEM_NOTI_NORMAL);

    if (mcast_type == 0) {
        g_player_manager->send_msg_to_all_player(cli_cmd_cs_0x0611_system_noti, noti);
    } else if (mcast_type == 1){
        switchproto::cs_sw_transmit_only cs_sw_transmit_only_;	
        cs_sw_transmit_only_.set_transmit_type(switchproto::SWITCH_TRANSMIT_WORLD);
        cs_sw_transmit_only_.set_cmd(cli_cmd_cs_0x0611_system_noti);
        std::string pkg;                                                      
        noti.SerializeToString(&pkg);                           
        cs_sw_transmit_only_.set_pkg(pkg);                                    

        g_switch->send_msg(NULL, g_online_id, 0, sw_cmd_sw_transmit_only,                      
                cs_sw_transmit_only_);
    } else if (mcast_type == 2) {
        MapUtils::send_msg_to_all_map_users(
                map_id, cli_cmd_cs_0x0611_system_noti, noti);
    }

	return 0;
}

int SendHyperLinkMsgCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
	onlineproto::sc_0x0624_notify_hyperlink_msg noti_msg;
	noti_msg.set_user_id(player->userid);
	noti_msg.set_create_tm(player->create_tm);
	noti_msg.set_nick(player->nick);
	noti_msg.set_type(cli_in_.type());
	noti_msg.set_content(cli_in_.content());
	std::vector<uint32_t> recv_id;
	for (int i = 0; i < cli_in_.role_size(); ++i) {
		uint32_t userid = cli_in_.role(i).userid();
		player_t* ply = g_player_manager->get_player_by_userid(userid);
		if (ply == NULL) {
			recv_id.push_back(userid);
			continue;
		}
		//同线的玩家直接通知，不走switch
		send_msg_to_player(ply, cli_cmd_cs_0x0624_notify_hyperlink_msg, noti_msg);
		
	}
	if (!recv_id.empty()) {
		Utils::switch_transmit_msg(switchproto::SWITCH_TRANSMIT_USERS,
				cli_cmd_cs_0x0624_notify_hyperlink_msg,
				noti_msg, &recv_id);
	}
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
