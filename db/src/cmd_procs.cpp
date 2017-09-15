#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>

extern "C" {
#include <libtaomee/crypt/qdes.h>
}
#include <libtaomee++/utils/strings.hpp>
#include <libtaomee++/utils/strings.hpp>
#include <libtaomee++/random/random.hpp> 
#include "cmd_procs.h"
#include "proto/db/db_errno.h"
#include "proto/client/common.pb.h"
#include "proto/client/attr_type.h"
#include "server.h"

#include "base_info_table.h"
#include "item_table.h"
#include "pet_table.h"
#include "nick_table.h"
#include "attr_table.h"
#include "task_info_table.h"
#include "user_action_log_table.h"
#include "rune_table.h"
#include "mail_table.h"
#include "transaction_table.h"
#include "friend_table.h"
#include "hm_visit_log_table.h"
#include "family_id_table.h"
#include "family_info_table.h"
#include "family_member_table.h"
#include "rawdata_table.h"
#include "family_event_table.h"
#include "family_log_table.h"
#include "family_match_info_table.h"
#include "global_attr_table.h"
#include "clisoft_table.h"
#include "gift_code_table.h"
#include "vip_op_trans_table.h"
#include "vip_user_info_table.h"
#include "achieve_table.h"
#include "mine_table.h"

uint32_t CreateRoleCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body) 
{
    cs_create_role_.Clear();    

    if (!cs_create_role_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }

    uint32_t ret = 0;
    u_create_tm = 0;
    ret = g_base_info_table->create_role(userid, cs_create_role_, u_create_tm);
    if (ret != db_err_succ) {
        return ret; 
    }
    sc_create_role_.Clear();
    g_base_info_table->get_info(userid, *(sc_create_role_.mutable_base_info()), u_create_tm);
    if (ret != db_err_succ) {
        return db_err_attr_set; 
    }
    sc_create_role_.SerializeToString(&ack_body);

    return db_err_succ; 
}

uint32_t GetBaseInfoCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body) 
{
    sc_get_base_info_.Clear(); 
    PARSE_DB_MSG(cs_get_base_info_);
    // 获取base_info
    uint32_t ret = g_base_info_table->get_infos(userid, cs_get_base_info_.server_id(),
            sc_get_base_info_.mutable_base_info());
    if (ret) {
        return ret; 
    }
    
    sc_get_base_info_.SerializeToString(&ack_body);
	Utils::print_message(sc_get_base_info_);
    return db_err_succ;
}

uint32_t GetLoginInfoCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body) 
{
    sc_get_login_info_.Clear(); 

    // 获取base_info
    commonproto::player_base_info_t* base_info = sc_get_login_info_.mutable_base_info();
    uint32_t ret = g_base_info_table->get_info(userid, *base_info, u_create_tm);
    if (ret) {
        return ret; 
    }

    //获取属性
    commonproto::attr_data_list_t list;
    list.Clear();
    ret = g_attr_table->get_all_attrs(userid, &list, u_create_tm);
    if (ret) {
        return ret; 
    }

    for(int i=0; i<list.attrs_size(); i++) {
      commonproto::attr_data_t* addr = sc_get_login_info_.mutable_attr_list()->add_attrs();
      addr->CopyFrom(list.attrs(i));
    }

    //获取任务
    ret = g_task_table->get_tasks(userid, u_create_tm, sc_get_login_info_);
    if (ret) {
        return ret; 
    }


    //获取物品
    ret = g_item_table->get_items(userid, u_create_tm, &sc_get_login_info_);
    if (ret) {
        return ret; 
    }

    //获取精灵
    commonproto::pet_list_t pet_list;
    ret = g_pet_table->get_pets(userid, u_create_tm, &pet_list);
    if (ret) {
        return ret; 
    }
    sc_get_login_info_.mutable_pet_list()->CopyFrom(pet_list);

    //获取符文
	ret = g_rune_table->get_runes(userid, u_create_tm, sc_get_login_info_);	
	if (ret) {
		return ret;
	}

    //获取好友
	ret = g_friend_table->get_friends(userid, u_create_tm, sc_get_login_info_);	
	if (ret) {
		return ret;
	}
	
    //获取buff
    char rawdata[BUFF_SIZE_MAX] = {0};
    uint32_t length = 0;
    ret = g_raw_data_table->get_user_raw_data(userid, u_create_tm, dbproto::BUFF_INFO, rawdata, length);
    if (ret && ret != db_err_record_not_found) {
        return ret;
    }
    if (length) {
       sc_get_login_info_.mutable_user_buff()->ParseFromArray(rawdata, length);
    }

    // 获取家族离线消息
    std::vector<std::string> family_msgs;
    ret = g_raw_data_table->get_raw_data_list(
            userid, u_create_tm, dbproto::FAMILY_OFFLINE_MESSAGE, family_msgs);
    if (ret) {
        return ret;
    }
    for (uint32_t i = 0; i < family_msgs.size();i++) {
       commonproto::family_msg_t *family_msg = sc_get_login_info_.add_family_msgs();
       family_msg->ParseFromString(family_msgs[i]);
    }

    // 获取申请的家族列表记录
    std::vector<std::string> family_apply_vec;
    ret = g_raw_data_table->get_raw_data_list(
            userid, u_create_tm, dbproto::FAMILY_APPLY_RECORD, family_apply_vec);
    if (ret) {
        return ret;
    }
    commonproto::family_apply_record_t *apply_record = sc_get_login_info_.mutable_family_apply_record();
    if (family_apply_vec.size() > 0) {
        apply_record->ParseFromString(family_apply_vec[0]);
    }

    // 获取元素挑战商店刷新物品
    get_shop_info(userid, u_create_tm, dbproto::ELEM_DUP_SHOP_PRODUCT, sc_get_login_info_.mutable_elem_dup_shop_info());
    // 获取竞技场商店刷新物品
    get_shop_info(userid, u_create_tm, dbproto::ARENA_SHOP_PRODUCT, sc_get_login_info_.mutable_arena_shop_info());
    // 获取家族商店刷新物品
    get_shop_info(userid, u_create_tm, dbproto::FAMILY_SHOP_PRODUCT, sc_get_login_info_.mutable_family_shop_info());
    // 伙伴激战商店(远征)
    get_shop_info(userid, u_create_tm, dbproto::EXPED_SHOP_PRODUCT, sc_get_login_info_.mutable_exped_shop_info());
	//夜袭商店
    get_shop_info(userid, u_create_tm, dbproto::NIGHT_RAID_SHOP_PRODUCT, sc_get_login_info_.mutable_night_raid_shop_info());
    // 每日商店
    get_shop_info(userid, u_create_tm, dbproto::DAILY_SHOP_PRODUCT, sc_get_login_info_.mutable_daily_shop_info());
	//熔炉普通商店
    get_shop_info(userid, u_create_tm, dbproto::SMELTER_MONEY, sc_get_login_info_.mutable_smelter_money_info());
	//熔炉特殊商店
    get_shop_info(userid, u_create_tm, dbproto::SMELTER_GOLD, sc_get_login_info_.mutable_smelter_gold_info());

	memset(rawdata, 0x0, sizeof(rawdata));
	length = 0;
    ret = g_raw_data_table->get_user_raw_data(userid, u_create_tm, dbproto::TITLE_INFO, rawdata, length);
    if (ret && ret != db_err_record_not_found) {
        return ret;
    }
    if (length) {
		sc_get_login_info_.mutable_title_list()->ParseFromArray(rawdata, length);
    }
	//拉取我的矿的id信息
	memset(rawdata, 0x0, sizeof(rawdata));
	length = 0;
	ret = g_raw_data_table->get_user_raw_data(userid, u_create_tm, dbproto::MINE_IDS, rawdata, length);
	if (ret && ret != db_err_record_not_found) {
		return ret;
	}
	if (length) {
		sc_get_login_info_.mutable_mine_id_list()->ParseFromArray(rawdata, length);
	}
	//拉取刷到新矿的数据
	memset(rawdata, 0x0, sizeof(rawdata));
	length = 0;
	ret = g_raw_data_table->get_user_raw_data(userid, u_create_tm, dbproto::NEW_MINE_INFO, rawdata, length);
	if (ret && ret != db_err_record_not_found) {
		return ret;
	}
	if (length) {
		sc_get_login_info_.mutable_mine_info_list()->ParseFromArray(rawdata, length);
	}
	//拉取对手矿的ids
	memset(rawdata, 0x0, sizeof(rawdata));
	length = 0;
	ret = g_raw_data_table->get_user_raw_data(userid, u_create_tm, dbproto::OPPONENT_MINE_IDS, rawdata, length);
	if (ret && ret != db_err_record_not_found) {
		return ret;
	}
	if (length) {
		sc_get_login_info_.mutable_opponent_mine_ids()->ParseFromArray(rawdata, length);
	}

    sc_get_login_info_.SerializeToString(&ack_body);
	Utils::print_message(sc_get_login_info_);	
    return db_err_succ;
}

uint32_t GetUserByNickCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_get_user_by_nick_.Clear();
    if (!cs_get_user_by_nick_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }
    
    role_info_t user;
    user.userid = 0;
    user.u_create_tm = 0;
    int ret = g_nick_table->get_user_by_nick(cs_get_user_by_nick_.nick(), user);
    if (ret) {
        return db_err_nick_not_exist;
    }

    sc_get_user_by_nick_.set_userid(user.userid);
    sc_get_user_by_nick_.set_u_create_tm(user.u_create_tm);
    sc_get_user_by_nick_.SerializeToString(&ack_body);

    return 0;
}

uint32_t CheckUserExistCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body) 
{
    sc_check_user_exist_.Clear();

    if (!cs_check_user_exist_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }
    
    uint32_t ret = 0;  
    ret = g_base_info_table->user_exists(userid, 
            u_create_tm,
            cs_check_user_exist_.server_id(),
            cs_check_user_exist_.is_init_server());

    if (ret == db_err_user_not_find) {
        sc_check_user_exist_.set_exist(false); 
    } else if (ret == DB_SUCC) {
        sc_check_user_exist_.set_exist(true); 
    } else {
        return ret; 
    }
    
    sc_check_user_exist_.SerializeToString(&ack_body);

    return db_err_succ;
}

uint32_t InsertNickAndUserCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_insert_nick_and_user_.Clear();
    if (!cs_insert_nick_and_user_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }

    role_info_t user;
    user.userid = userid;
    user.u_create_tm = u_create_tm;
    return g_nick_table->insert_nick_and_user(
            cs_insert_nick_and_user_.nick(), user);
}

uint32_t DeleteNickAndUserCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_delete_nick_and_user_.Clear();
    if (!cs_delete_nick_and_user_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }

    return g_nick_table->delete_nick_and_user(
            cs_delete_nick_and_user_.nick());
}

uint32_t ChangeNickCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_change_nick_.Clear();
    if (!cs_change_nick_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
    g_base_info_table->change_nick(userid, cs_change_nick_.nick(), u_create_tm);
    sc_change_nick_.SerializeToString(&ack_body);
    return 0;
}

uint32_t SetAttrCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    cs_set_attr_.Clear();
    sc_set_attr_.Clear();
    if (!cs_set_attr_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
    for (int i = 0; i < cs_set_attr_.attrs_size(); i++) {
        uint32_t type = cs_set_attr_.attrs(i).type();
        uint32_t value = cs_set_attr_.attrs(i).value();
        g_attr_table->set_attr(userid, type, value, u_create_tm);   
    }
    ack_body.clear();
    return 0;
}

uint32_t SetAttrOnceCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    cs_set_attr_once_.Clear();
    sc_set_attr_once_.Clear();
    if (!cs_set_attr_once_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
    for (int i = 0; i < cs_set_attr_once_.attrs_size(); i++) {
        uint32_t type = cs_set_attr_once_.attrs(i).type();
        uint32_t value = cs_set_attr_once_.attrs(i).value();
        g_attr_table->set_attr_once(userid, type, value, u_create_tm);   
    }
    ack_body.clear();
    return 0;
}

uint32_t GetAttrCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_get_attr_.Clear();
    sc_get_attr_.Clear();

    if (!cs_get_attr_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
    g_attr_table->get_attrs(userid, cs_get_attr_, sc_get_attr_, u_create_tm);
    sc_get_attr_.SerializeToString(&ack_body);

    return 0;
}

uint32_t DelAttrCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    cs_del_attr_.Clear();
    sc_del_attr_.Clear();
    if (!cs_del_attr_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
    g_attr_table->del_attrs(userid, cs_del_attr_, u_create_tm);
    ack_body.clear();
    return 0;
}

uint32_t RangeGetAttrCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    cs_get_ranged_attr_.Clear();
    sc_get_ranged_attr_.Clear();
    if (!cs_get_ranged_attr_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
    g_attr_table->get_ranged_attr(userid, 
            cs_get_ranged_attr_.low(),
            cs_get_ranged_attr_.high(),
            sc_get_ranged_attr_.mutable_attr_list(),
            u_create_tm);
 
    sc_get_ranged_attr_.SerializeToString(&ack_body);
    return 0;
}

uint32_t RangeClearAttrCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    cs_clear_ranged_attr_.Clear();
    sc_clear_ranged_attr_.Clear();
    if (!cs_clear_ranged_attr_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
    g_attr_table->ranged_clear(userid, 
            cs_clear_ranged_attr_.low(),
            cs_clear_ranged_attr_.high(),
            u_create_tm);
    ack_body.clear();
    return 0;
}

uint32_t ChangeAttrCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    cs_change_attr_value_.Clear();
    sc_change_attr_value_.Clear();
    if (!cs_change_attr_value_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
    g_attr_table->change_attr(userid, 
            cs_change_attr_value_.type(),
            cs_change_attr_value_.change(),
            cs_change_attr_value_.max_value(),
            u_create_tm);
	if (cs_change_attr_value_.get_new_val() == true) {
		uint32_t new_value = 0;

		g_attr_table->get_attr(userid,
				cs_change_attr_value_.type(),
				new_value, u_create_tm);
		sc_change_attr_value_.set_new_value(new_value);
	}
    sc_change_attr_value_.SerializeToString(&ack_body);

    return 0;
}

uint32_t ItemChangeCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_change_items_.Clear();
    sc_change_items_.Clear();

    if (!cs_change_items_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
    
    int ret = 0;

    if(cs_change_items_.item_info_list_size() != 0){
        ret = g_item_table->update_items(userid, u_create_tm, cs_change_items_);
        if(ret != 0){
            return ret;
        }
    }

    // sc_item_change_.set_item_count(cs_item_change_.item_info_list_size());
    // sc_item_change_.SerializeToString(&ack_body);
    return ret;
}

uint32_t PetSaveCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body) 
{
    cs_pet_save_.Clear();
    sc_pet_save_.Clear();

    if (!cs_pet_save_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }

    return g_pet_table->save_pet(userid, u_create_tm,
            cs_pet_save_.pet_info());
}

uint32_t PetDeleteCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body ,std::string& ack_body) 
{
    cs_pet_del_info_.Clear();

    if(!cs_pet_del_info_.ParseFromString(req_body)){
        return db_err_proto_format_err;
    }
    
    int ret = g_pet_table->delete_pet(userid, u_create_tm, cs_pet_del_info_);
    if(ret != 0){
        return ret;
    }

    return ret;
}

uint32_t PetListGetCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
	sc_pet_list_get_info_.Clear();
	int ret = g_pet_table->get_pets(userid, u_create_tm, sc_pet_list_get_info_.mutable_pet_list());
	if (ret) {
		return ret;
	}
	sc_pet_list_get_info_.SerializeToString(&ack_body);
	return 0;
}

uint32_t InsertActionLogCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body ,std::string& ack_body) 
{
    cs_insert_user_action_log_.Clear();
    if (!cs_insert_user_action_log_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
    for (int i = 0; i < cs_insert_user_action_log_.log_list_size(); i++) {
        const dbproto::user_action_log_t &info = 
            cs_insert_user_action_log_.log_list(i);
        g_user_action_log_table->insert_log(userid, u_create_tm, info);
    }
    ack_body.clear();
    return 0;
}

uint32_t GetActionLogCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body ,std::string& ack_body)
{
    cs_get_user_action_log_.Clear();
    sc_get_user_action_log_.Clear();
    if (!cs_get_user_action_log_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }
	if (cs_get_user_action_log_.has_target_id()) {
		g_user_action_log_table->get_action_log_with_tar_id(userid, u_create_tm,
				cs_get_user_action_log_.date(),
				cs_get_user_action_log_.start_idx(),
				cs_get_user_action_log_.num(),
				cs_get_user_action_log_.target_id(),
				&sc_get_user_action_log_);
		if (cs_get_user_action_log_.has_get_count_flag()) {
			g_user_action_log_table->get_user_log_total(userid, u_create_tm,
					cs_get_user_action_log_.date(),
					&sc_get_user_action_log_);
		}
	} else {
		g_user_action_log_table->get_action_log(userid, u_create_tm,
				cs_get_user_action_log_.date(),
				cs_get_user_action_log_.start_idx(),
				cs_get_user_action_log_.num(),
				&sc_get_user_action_log_);
		if (cs_get_user_action_log_.has_get_count_flag()) {
			g_user_action_log_table->get_user_log_total(userid, u_create_tm,
					cs_get_user_action_log_.date(),
					&sc_get_user_action_log_);
		}
	}
    ack_body.clear();
    sc_get_user_action_log_.SerializeToString(&ack_body);

	Utils::print_message(sc_get_user_action_log_);
    return 0;
}


uint32_t TaskSaveCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_task_save_.Clear();
    sc_task_save_.Clear();

    if (!cs_task_save_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }

    return g_task_table->save_tasks(userid, u_create_tm, cs_task_save_);
}

uint32_t TaskDelCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_task_del_.Clear();
    sc_task_del_.Clear();

    if (!cs_task_del_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }

    return g_task_table->del_task(userid, u_create_tm, cs_task_del_);
}

uint32_t RuneSaveCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
	cs_rune_save_.Clear();
	sc_rune_save_.Clear();
	if (!cs_rune_save_.ParseFromString(req_body)) {
		return db_err_proto_format_err;
	}
	std::string name = cs_rune_save_.GetTypeName();
	std::string debug_str = cs_rune_save_.Utf8DebugString();
	TRACE_TLOG("uid=[%u]PARSE MSG:'%s' ok\nMSG:\n[%s]", 
			userid, name.c_str(), debug_str.c_str());
	return g_rune_table->save_rune(userid, u_create_tm, cs_rune_save_.rune_data());
}

uint32_t RuneDelCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
	cs_rune_del_.Clear();
	sc_rune_del_.Clear();
	if (!cs_rune_del_.ParseFromString(req_body)) {
		return db_err_proto_format_err;
	}
	return g_rune_table->del_rune(userid, u_create_tm, cs_rune_del_);
}

uint32_t MailNewCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body) 
{
	cs_mail_new_.Clear();
	sc_mail_new_.Clear();

	if (!cs_mail_new_.ParseFromString(req_body)) {
		return db_err_proto_format_err;
	}
    int ret = g_mail_table->new_mail(userid, u_create_tm, cs_mail_new_.mail_data());
    if (ret) {
        return ret;
    }
    sc_mail_new_.mutable_mail_data()->CopyFrom(cs_mail_new_.mail_data());
	ack_body.clear();
	sc_mail_new_.SerializeToString(&ack_body);
	return 0;
}

uint32_t MailGetAllCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
	cs_mail_get_all_.Clear();
	sc_mail_get_all_.Clear();
	
	int ret = g_mail_table->get_all_mails(userid, u_create_tm, sc_mail_get_all_);
	if (ret != 0) {
		return ret;
	}
	sc_mail_get_all_.SerializeToString(&ack_body);
	return 0;
}

uint32_t MailDelByIdsCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
	cs_mail_del_by_ids_.Clear();
	sc_mail_del_by_ids_.Clear();

	if (!cs_mail_del_by_ids_.ParseFromString(req_body)) {
		return db_err_proto_format_err;
	}
	int ret = g_mail_table->del_mail_by_ids(userid, u_create_tm,
			cs_mail_del_by_ids_);
	if (ret != 0) {
		return ret;
	}
	sc_mail_del_by_ids_.SerializeToString(&ack_body);

	return 0;
}

uint32_t MailGetByIdsCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
	cs_mail_get_by_ids_.Clear();
	sc_mail_get_by_ids_.Clear();

	if (!cs_mail_get_by_ids_.ParseFromString(req_body)) {
		return db_err_proto_format_err;
	}

	int ret = g_mail_table->get_mail_by_ids(userid, u_create_tm,
            cs_mail_get_by_ids_,
            sc_mail_get_by_ids_);
	if (ret != 0) {
		return ret;
	}

	sc_mail_get_by_ids_.SerializeToString(&ack_body);
	return 0;
}

uint32_t MailSetStatusCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
	cs_mail_set_status_.Clear();
	sc_mail_set_status_.Clear();

	if (!cs_mail_set_status_.ParseFromString(req_body)) {
		return db_err_proto_format_err;
	}

	int ret = g_mail_table->set_mail_status(userid, u_create_tm,
            cs_mail_set_status_.mailid(),
            (uint32_t)cs_mail_set_status_.status());

	if (ret != 0) {
		return ret;
	}

	sc_mail_set_status_.SerializeToString(&ack_body);
	return 0;
}

uint32_t HomeGetInfoCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
	const uint32_t DEFAULT_HOME_TYPE = 15;
	cs_get_home_info_.Clear();
	sc_get_home_info_.Clear();

	if (!cs_get_home_info_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }
	commonproto::player_base_info_t base_info;
    uint32_t ret = g_base_info_table->get_info(userid, base_info, u_create_tm);
    if (ret) {
        return ret; 
    }
	uint32_t value = 0;
	ret = g_attr_table->get_attr(userid, kAttrHomeType, value, u_create_tm);
	if (ret == db_err_record_not_found) {
		g_attr_table->set_attr(userid, kAttrHomeType, DEFAULT_HOME_TYPE, u_create_tm);
	}
	sc_get_home_info_.set_host_id(userid);
    sc_get_home_info_.set_host_u_create_tm(u_create_tm);
	sc_get_home_info_.set_host_nick(base_info.nick());
	if (0 == value) {
		value = commonproto::HM_DEFAULT_TYPE;
	}
	sc_get_home_info_.set_home_type(value);
	sc_get_home_info_.set_getter_id(cs_get_home_info_.getter_id());
	sc_get_home_info_.SerializeToString(&ack_body);
	Utils::print_message(sc_get_home_info_);
	return db_err_succ;
}

uint32_t NewTransactionCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_new_transaction_.Clear();
    if (!cs_new_transaction_.ParseFromString(req_body)) {
        return db_err_proto_format_err;
    }

    uint64_t auto_incr_id = 0;
    bool is_duplicate_trans = false;
    int ret = g_transaction_table->new_transaction_rd(userid, cs_new_transaction_.info(), auto_incr_id, is_duplicate_trans);
    if (ret) return ret;

    sc_new_transaction_.Clear();
    sc_new_transaction_.set_is_duplicate_trans(is_duplicate_trans);
    sc_new_transaction_.mutable_info()->CopyFrom(cs_new_transaction_.info());
    sc_new_transaction_.set_transaction_id(auto_incr_id);
    
    sc_new_transaction_.SerializeToString(&ack_body);
    return db_err_succ;
}

uint32_t VipOpTransCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string& req_body,
		std::string& ack_body)
{
	cs_new_vip_op_trans_.Clear();
	if (!cs_new_vip_op_trans_.ParseFromString(req_body)) {
		return db_err_proto_format_err;
	}
	uint32_t auto_incr_id = 0;
	int ret = g_vip_op_trans_table->new_vip_op_trans_rd(userid,
			cs_new_vip_op_trans_.info(), auto_incr_id);
	if (ret) return ret;
    return db_err_succ;
}

uint32_t VipUserInfoCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string& req_body,
		std::string& ack_body)
{
	cs_new_vip_user_info_.Clear();
	if (!cs_new_vip_user_info_.ParseFromString(req_body)) {
		return db_err_proto_format_err;
	}
	int ret = g_vip_user_info_table->new_vip_table_info(userid, cs_new_vip_user_info_.info());
	if (ret) return ret;
	return db_err_succ;
}

uint32_t GetBuyPdTransListCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string& req_body,
		std::string& ack_body)
{
	cs_get_buy_pd_trans_list_.Clear();
	if (!cs_get_buy_pd_trans_list_.ParseFromString(req_body)) {
		return db_err_proto_format_err;
	}

	uint32_t start_time = cs_get_buy_pd_trans_list_.start_time();
	uint32_t end_time = cs_get_buy_pd_trans_list_.end_time();
	uint32_t product_id = 0;
	if (cs_get_buy_pd_trans_list_.has_product_id()) {
		product_id = cs_get_buy_pd_trans_list_.product_id();
	}
	sc_get_buy_pd_trans_list_.Clear();
	dbproto::transaction_list_t* list = sc_get_buy_pd_trans_list_.mutable_tran_list();
	if (product_id) {
		uint32_t ret = g_transaction_table->get_buy_pd_trans_list(userid, u_create_tm,
				start_time, end_time, product_id, list);
		if (ret) {
			return ret;
		}
	} else {
		return db_err_must_give_pd_id;
	}
	sc_get_buy_pd_trans_list_.SerializeToString(&ack_body);
	return 0;
}
	

uint32_t GetTransactionListCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_get_transaction_list_.Clear();
    if (!cs_get_transaction_list_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }

    uint32_t start_time = cs_get_transaction_list_.start_time();
    uint32_t end_time = cs_get_transaction_list_.end_time();

    sc_get_transaction_list_.Clear();
    dbproto::transaction_list_t* list = sc_get_transaction_list_.mutable_list();
    uint32_t ret = g_transaction_table->get_transaction_list(userid, u_create_tm, 
            start_time, end_time, list);
    if (ret) {
        return ret; 
    }

    sc_get_transaction_list_.SerializeToString(&ack_body);

    return 0;
}

uint32_t SaveFriendCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body) 
{
    cs_save_friend_.Clear();
    if (!cs_save_friend_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }

    return g_friend_table->save_friend(cs_save_friend_.finf());
}

uint32_t RemoveFriendCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body) 
{
    cs_remove_friend_.Clear();
    if (!cs_remove_friend_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }
    return g_friend_table->remove_friend(cs_remove_friend_.finf());
}

uint32_t GetCacheInfoCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    cs_get_cache_info_.Clear();
    sc_get_cache_info_.Clear();

    commonproto::battle_player_data_t *player_info = sc_get_cache_info_.mutable_cache_info();

    //基本信息
    commonproto::player_base_info_t *base_info = player_info->mutable_base_info();
    int ret = g_base_info_table->get_info(userid, *base_info, u_create_tm);
    if (ret) {
        return ret;
    }
	uint32_t tmp_value;
	g_attr_table->get_attr(userid, kAttrLastLoginTm, tmp_value, u_create_tm);
	base_info->set_last_login_tm(tmp_value);

    //战斗属性
    commonproto::attr_data_list_t list;
    g_attr_table->get_ranged_attr(userid, kAttrCurVp, kAttrSkill6, &list, u_create_tm);
    std::map<uint32_t, uint32_t> attr_map;
    for (int i = 0; i < list.attrs_size(); i++) {
        attr_map[list.attrs(i).type()] = list.attrs(i).value();
    }
#define FIND_ATTR(key) (attr_map.count((key)) ?attr_map[(key)] :0)
    commonproto::battle_info_t *battle_info = player_info->mutable_battle_info();
    battle_info->set_max_hp(FIND_ATTR(kAttrHpMax));
    battle_info->set_cur_hp(FIND_ATTR(kAttrHpMax));
    battle_info->set_normal_atk(FIND_ATTR(kAttrNormalAtk));
    battle_info->set_normal_def(FIND_ATTR(kAttrNormalDef));
    battle_info->set_skill_atk(FIND_ATTR(kAttrSkillAtk));
    battle_info->set_skill_def(FIND_ATTR(kAttrSkillDef));
    battle_info->set_crit(FIND_ATTR(kAttrCrit));
    battle_info->set_anti_crit(FIND_ATTR(kAttrAntiCrit));
    battle_info->set_hit(FIND_ATTR(kAttrHit));
    battle_info->set_dodge(FIND_ATTR(kAttrDodge));
    battle_info->set_block(FIND_ATTR(kAttrBlock));
    battle_info->set_break_block(FIND_ATTR(kAttrBreakBlock));
    battle_info->set_crit_affect_rate(FIND_ATTR(kAttrCritDamageRate));
    battle_info->set_block_affect_rate(FIND_ATTR(kAttrBlockDamageRate));
    battle_info->set_atk_speed(FIND_ATTR(kAttrAtkSpeed));
	battle_info->set_elem_type((commonproto::equip_elem_type_t)FIND_ATTR(kAttrPlayerElemType));
	battle_info->set_elem_damage_percent(FIND_ATTR(kAttrPlayerElemDamageRate));
    battle_info->set_req_power(base_info->power());
    battle_info->mutable_anti()->set_water(FIND_ATTR(kAttrAntiWater));
    battle_info->mutable_anti()->set_fire(FIND_ATTR(kAttrAntiFire));
    battle_info->mutable_anti()->set_grass(FIND_ATTR(kAttrAntiGrass));
    battle_info->mutable_anti()->set_light(FIND_ATTR(kAttrAntiLight));
    battle_info->mutable_anti()->set_dark(FIND_ATTR(kAttrAntiLight));
    battle_info->mutable_anti()->set_ground(FIND_ATTR(kAttrAntiGround));
    battle_info->mutable_anti()->set_force(FIND_ATTR(kAttrAntiForce));

    //装备信息(包括时装)
    commonproto::equip_list_t *equip_list = player_info->mutable_equip_list();
    equip_list->Clear();
    commonproto::item_info_t item_info;
    for (int i = kAttrHead; i <= kAttrPlayerElemType; i++) {
        uint32_t slot_id = FIND_ATTR(i);
        if (slot_id) {
            item_info.Clear();
            ret = g_item_table->get_item_by_slot_id(userid, u_create_tm, slot_id, &item_info);
            if (ret || item_info.slot_id() == 0) {
                continue;
            }
            commonproto::item_info_t *inf = equip_list->add_equips();
            inf->MergeFrom(item_info);
        }
    }

    //跟随精灵的信息
    commonproto::battle_pet_list_t *battle_pet_list = player_info->mutable_pet_list();
    commonproto::pet_list_t pet_list;
    g_pet_table->get_fight_pets(userid, u_create_tm, &pet_list);
    for (int i = 0; i < pet_list.pet_list_size(); i++) {
        const commonproto::pet_info_t &inf = pet_list.pet_list(i);
        commonproto::battle_pet_data_t *btl_inf = battle_pet_list->add_pet_list();
        btl_inf->mutable_pet_info()->CopyFrom(inf);
        btl_inf->mutable_pet_info()->mutable_battle_info()->set_req_power(base_info->power());
    }
    
    //刻印精灵的信息
    battle_pet_list = player_info->mutable_chisel_pet_list();
    pet_list.Clear();
    g_pet_table->get_chisel_pets(userid, u_create_tm, &pet_list);
    for (int i = 0; i < pet_list.pet_list_size(); i++) {
        const commonproto::pet_info_t &inf = pet_list.pet_list(i);
        commonproto::battle_pet_data_t *btl_inf = battle_pet_list->add_pet_list();
        btl_inf->mutable_pet_info()->CopyFrom(inf);
        btl_inf->mutable_pet_info()->mutable_battle_info()->set_req_power(base_info->power());
    }

	//战斗力最高的五个精灵
	battle_pet_list = player_info->mutable_power_pet_list();
	pet_list.Clear();
	g_pet_table->get_n_topest_power_pets(userid, u_create_tm, &pet_list, 5);
	for (int i = 0; i < pet_list.pet_list_size(); ++i) {
		const commonproto::pet_info_t& inf = pet_list.pet_list(i);
		commonproto::battle_pet_data_t* btl_inf = battle_pet_list->add_pet_list();
		btl_inf->mutable_pet_info()->CopyFrom(inf);
        btl_inf->mutable_pet_info()->mutable_battle_info()->set_req_power(base_info->power());
	}
	for (uint32_t i = 0; i < commonproto::MAX_SKILL_NUM; ++i) {
		player_info->add_skills(FIND_ATTR(kAttrSkill1 + i));
	}
    sc_get_cache_info_.SerializeToString(&ack_body);
	//Utils::print_message(sc_get_cache_info_);
    return 0;
}

uint32_t ShowItemCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{

	cs_show_item_.Clear();
    if (!cs_show_item_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }
	uint32_t slot_id = cs_show_item_.slot_id(); 
	sc_show_item_.Clear();
	commonproto::item_info_t temp_info;
	uint32_t ret = g_item_table->get_item_by_slot_id(userid, u_create_tm, slot_id, &temp_info);
	if (ret) {
		return ret;
	}
	if (temp_info.has_item_id()) {
		commonproto::item_info_t* item_info = sc_show_item_.mutable_item_data();
		item_info->CopyFrom(temp_info);
	}
	sc_show_item_.SerializeToString(&ack_body);
	return 0;
}

uint32_t ShowPetCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{

    cs_show_pet_.Clear();
    if (!cs_show_pet_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }
	uint32_t pet_id = cs_show_pet_.pet_id(); 
	sc_show_pet_.Clear();
    commonproto::pet_list_t pet_list;
    uint32_t ret = g_pet_table->get_pet_by_catch_time(userid, u_create_tm, pet_id, &pet_list);
    if (ret) {
        return ret; 
    }
	if (pet_list.pet_list_size() == 1) {
		commonproto::pet_info_t* pet_info = sc_show_pet_.mutable_pet_data();
		*pet_info = pet_list.pet_list(0);
	}
	sc_show_pet_.SerializeToString(&ack_body);
	return 0;
}

uint32_t HmAddVisitCmdProcessor::process(
	userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
	cs_add_visit_log_.Clear();
    if (!cs_add_visit_log_.ParseFromString(req_body)) {
        return db_err_proto_format_err; 
    }
	
	const commonproto::visit_log_info_t &log_info = cs_add_visit_log_.log_info();
	std::string str_detail = log_info.detail();
	g_hm_visit_table->add_visit_log(userid, log_info);
	sc_add_visit_log_.Clear();
	sc_add_visit_log_.SerializeToString(&ack_body);
	return 0;
}

/*
uint32_t UpdateGiftCountCmdProcessor::process(
	userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
	cs_update_gift_count_.Clear();
	if (!cs_update_gift_count_.ParseFromString(req_body)) {
		return db_err_proto_format_err;
	}
	uint32_t friendid = cs_update_gift_count_.friendid();
	uint32_t gift_count = cs_update_gift_count_.gift_count();
	g_friend_table->update_gift_count(userid, friendid, gift_count);
	return 0;
}
*/

uint32_t GetHmVisitLogCmdProcessor::process(
	userid_t userid, uint32_t u_create_tm, 
	const std::string& req_body, std::string& ack_body)
{
	sc_get_visit_log_.Clear();
	uint32_t ret = g_hm_visit_table->get_visit_log(userid, u_create_tm, sc_get_visit_log_.mutable_log_list());
	if (ret) {
		return ret;
	}
	ret = g_hm_visit_table->get_hm_gift_log(userid, u_create_tm, sc_get_visit_log_.mutable_log_list());
	if (ret) {
		return ret;
	}
	char rawdata[BUFF_SIZE_MAX] = { 0 };
	uint32_t length = 0;
	ret = g_raw_data_table->get_user_raw_data(userid, u_create_tm, dbproto::SEND_GIFT_FRIEND_ID, rawdata, length);
	if (ret && ret != db_err_record_not_found) {
		return ret;
	}
	if (length) {
		sc_get_visit_log_.set_buff(std::string(rawdata, length));
	}
	sc_get_visit_log_.SerializeToString(&ack_body);
	return 0;
}

uint32_t SaveAchievesCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
	PARSE_DB_MSG(cs_save_achieves_);
	uint32_t ret = g_achieve_table->save_achieves(userid,
			u_create_tm, cs_save_achieves_.achv_info());	
	if (ret) {
		return ret;
	}
	sc_save_achieves_.Clear();
	sc_save_achieves_.SerializeToString(&ack_body);
	return 0;
}


uint32_t FamilyCreateCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_create_);

    uint32_t family_id = 0;
    int ret = g_family_id_table->create_family(family_id, cs_family_create_.server_id());
    if (ret) {
        return ret; 
    }

    sc_family_create_.Clear();
    sc_family_create_.set_family_id(family_id);
	sc_family_create_.SerializeToString(&ack_body);
	return 0;
}

uint32_t FamilyUpdateInfoCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, 
        const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_update_info_);
	
    // 插入家族信息
    int ret = g_family_info_table->update_family_info(
            cs_family_update_info_.family_info(), 
            cs_family_update_info_.flag());
    if (ret) {
        return ret; 
    }

	sc_family_update_info_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyChangeInfoCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, 
        const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_change_info_);
	
    int ret = g_family_info_table->change_family_info(cs_family_change_info_.change_info());
    if (ret) {
        return ret; 
    }

    sc_family_change_info_.Clear();
	sc_family_change_info_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyGetInfoCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, 
        const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_get_info_);
    commonproto::family_info_t family_info;
    int ret = g_family_info_table->get_family_info(family_id, family_info);

    //family_info.set_family_id(family_id);

    // 拉取族长id
    std::vector<commonproto::family_member_info_t> member_info;
    ret = g_family_member_table->get_member_info_by_title(
            family_id, commonproto::FAMILY_TITLE_LEADER, member_info);
    if (!member_info.empty()) {
        family_info.set_leader_id(member_info[0].userid());
        family_info.set_leader_create_tm(member_info[0].u_create_tm());
    }

    // 计算总战力
    uint32_t total_battle_value = 0;
    ret = g_family_member_table->get_total_battle_value(
            family_id, total_battle_value);
    family_info.set_total_battle_value(total_battle_value);

    // 计算总成员数
    uint32_t total_member_num = 0;
    ret = g_family_member_table->get_total_member_num(family_id, 
            commonproto::FAMILY_MEMBER_LIST_ALL, total_member_num);
    family_info.set_member_num(total_member_num);

    // 计算副族长数
    uint32_t vice_leader_num = 0;
    ret = g_family_member_table->get_total_member_num(family_id, 
            commonproto::FAMILY_MEMBER_LIST_VICE_LEADER, vice_leader_num);
    family_info.set_vice_leader_num(vice_leader_num);

    // 拉取成员列表
    std::vector<role_info_t> members;
    if (cs_family_get_info_.flag() == dbproto::DB_FAMILY_GET_INFO_WITH_MEMBER_LIST) {
        ret = g_family_member_table->get_members(family_id, 0, members);
    }

    sc_family_get_info_.Clear();
    sc_family_get_info_.mutable_family_info()->CopyFrom(family_info);
    FOREACH(members, it) {
        commonproto::role_info_t *inf = sc_family_get_info_.add_members();
        inf->set_userid(it->userid);
        inf->set_u_create_tm(it->u_create_tm);
    }
    sc_family_get_info_.SerializeToString(&ack_body);
	return 0;
}

uint32_t FamilyGetMemberInfoCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_get_member_info_);

    sc_family_get_member_info_.Clear();
    for(int i = 0; i < cs_family_get_member_info_.users_size(); i++) {
        uint32_t userid = cs_family_get_member_info_.users(i).userid();
        u_create_tm = cs_family_get_member_info_.users(i).u_create_tm();
        commonproto::family_member_info_t *member_info = 
            sc_family_get_member_info_.add_member_infos();

        int ret = g_family_member_table->get_member_info(
                family_id, userid, u_create_tm, *member_info);
        if (ret) {
            return ret; 
        }
    }

    sc_family_get_member_info_.SerializeToString(&ack_body);
	return 0;
}

uint32_t FamilyDismissFamilyCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    // 删除所有家族成员
    int ret = g_family_member_table->del_member(family_id, 0, 0);
    if (ret) {
        return ret; 
    }

    // 删除家族信息
    ret = g_family_info_table->del_family(family_id);
    if (ret) {
        return ret; 
    }

    // 删除家族事件
    ret = g_family_event_table->del_event(family_id, 0, 0, 0);
    if (ret) {
        return ret; 
    }

    // 删除家族日志
 
	return 0;
}

uint32_t FamilyUpdateMemberInfoCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_update_member_info_);
    int ret = g_family_member_table->update_member_info(
            cs_family_update_member_info_.member_info(), 
            cs_family_update_member_info_.flag());
    if (ret) {
        return ret; 
    }

	return 0;
}

uint32_t FamilyChangeMemberInfoCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_change_member_info_);
    int ret = g_family_member_table->change_family_member_info(
            cs_family_change_member_info_.change_member_info());
    if (ret) {
        return ret; 
    }

    sc_family_change_member_info_.Clear();
    sc_family_change_member_info_.SerializeToString(&ack_body);
    return 0;

	
    return 0;
}

uint32_t FamilyGetMemberListCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_get_member_list_);

    sc_family_get_member_list_.Clear();
    uint32_t page_size = cs_family_get_member_list_.list_in().page_size();
    uint32_t start = (cs_family_get_member_list_.list_in().page_no()  - 1) * page_size;
    uint32_t flag = cs_family_get_member_list_.list_in().flag();

    uint32_t total_num = 0;
    int ret = g_family_member_table->get_total_member_num(family_id, flag, total_num);
    if (ret) {
        return ret; 
    }

    uint32_t page_num = 0;
    ret = g_family_member_table->get_member_list(
            family_id, flag, start, page_size, 
            &sc_family_get_member_list_, page_num);
    if (ret) {
        return ret; 
    }

    sc_family_get_member_list_.mutable_list_out()->set_total_member_num(total_num);
    sc_family_get_member_list_.mutable_list_out()->set_page_member_num(page_num);
    sc_family_get_member_list_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyQuitCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_quit_);

    // 删除成员记录
    uint32_t userid = cs_family_quit_.userid();
    u_create_tm = cs_family_quit_.u_create_tm();
    int ret = g_family_member_table->del_member(family_id, userid, u_create_tm);
    if (ret) {
        return ret; 
    }

	return 0;
}

uint32_t UserRawDataGetCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    PARSE_DB_MSG(cs_user_raw_data_get_);
    dbproto::user_raw_data_type_t type = cs_user_raw_data_get_.type();
    string buff_id = cs_user_raw_data_get_.buff_id();
    char rawdata[BUFF_SIZE_MAX] = { 0 }; 

    uint32_t length = 0; 
    g_raw_data_table->get_user_raw_data(userid, u_create_tm, type, rawdata, length, buff_id);

    sc_user_raw_data_get_.Clear();
    sc_user_raw_data_get_.set_type(type);
    sc_user_raw_data_get_.set_raw_data(std::string(rawdata, length));
    // sc_user_raw_data_get_.set_type(type);

    sc_user_raw_data_get_.SerializeToString(&ack_body);

    return 0;
}

uint32_t UserRawDataUpdateCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    PARSE_DB_MSG(cs_user_raw_data_update_);
	int ret = g_raw_data_table->update_raw_data(cs_user_raw_data_update_.raw_data());
	if (ret) {
		return ret;
	}
	return 0;
}

uint32_t UserRawDataDelCmdProcessor::process(
		userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    PARSE_DB_MSG(cs_user_raw_data_del_);
	int ret = g_raw_data_table->del_raw_data(
            userid, u_create_tm, (uint32_t)cs_user_raw_data_del_.type(), 
            cs_user_raw_data_del_.buff_id());
	if (ret) {
		return ret;
	}
	return 0;
}

uint32_t FamilyUpdateEventCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_update_event_);
	
    // 更新家族事件
    int ret = g_family_event_table->update_family_event(
            cs_family_update_event_.family_event(), dbproto::DB_UPDATE_AND_INESRT);
    if (ret) {
        return ret; 
    }

	sc_family_update_event_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyDelEventCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_del_event_);
	
    int ret = g_family_event_table->del_event(
            family_id, cs_family_del_event_.userid(), cs_family_del_event_.u_create_tm(),
            cs_family_del_event_.type());
    if (ret) {
        return ret; 
    }

	sc_family_del_event_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyGetEventInfoCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_get_event_info_);

    sc_family_get_event_info_.Clear();
    commonproto::family_event_t *event = sc_family_get_event_info_.mutable_family_event();
    int ret = g_family_event_table->get_event_info(
            family_id, cs_family_get_event_info_.userid(), 
            cs_family_get_event_info_.u_create_tm(), 
            cs_family_get_event_info_.type(),
            *event);
    if (ret) {
        return ret; 
    }

    sc_family_get_event_info_.SerializeToString(&ack_body);
	return 0;
}

uint32_t FamilyGetEventListCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_get_event_list_);

    sc_family_get_event_list_.Clear();
    uint32_t page_size = cs_family_get_event_list_.page_size();
    uint32_t start = (cs_family_get_event_list_.page_no()  - 1) * page_size;
    uint32_t userid = cs_family_get_event_list_.userid();
    u_create_tm = cs_family_get_event_list_.u_create_tm();
    uint32_t type = cs_family_get_event_list_.type();

    uint32_t total_num = 0;
    int ret = g_family_event_table->get_total_event_num(
            family_id, userid, u_create_tm, type,  total_num);
    if (ret) {
        return ret; 
    }

    uint32_t page_num = 0;
    ret = g_family_event_table->get_event_list(
            family_id, userid, u_create_tm, type, 
            start, page_size,
            &sc_family_get_event_list_, page_num);
    if (ret) {
        return ret; 
    }

    sc_family_get_event_list_.set_total_num(total_num);
    sc_family_get_event_list_.set_page_num(page_num);
    sc_family_get_event_list_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyUpdateLogCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_update_log_);
	
    // 更新家族日志
    int ret = g_family_log_table->update_family_log(
            cs_family_update_log_.family_log(), dbproto::DB_UPDATE_AND_INESRT);
    if (ret) {
        return ret; 
    }

	sc_family_update_log_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyGetLogListCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_get_log_list_);

    sc_family_get_log_list_.Clear();
    uint32_t page_size = cs_family_get_log_list_.page_size();
    uint32_t start = (cs_family_get_log_list_.page_no()  - 1) * page_size;
    uint32_t type = cs_family_get_log_list_.type();

    uint32_t total_num = 0;
    int ret = g_family_log_table->get_total_log_num(
            family_id, type,  total_num);
    if (ret) {
        return ret; 
    }

    if (total_num > commonproto::MAX_FAMILY_LOG_NUM) {
        int ret = g_family_log_table->del_log(
                family_id, total_num - (uint32_t)commonproto::MAX_FAMILY_LOG_NUM);
        if (ret) {
            return ret; 
        }
    }

    uint32_t page_num = 0;
    ret = g_family_log_table->get_log_list(
            family_id, type, 
            start, page_size,
            &sc_family_get_log_list_, page_num);
    if (ret) {
        return ret; 
    }

    sc_family_get_log_list_.set_total_num(total_num);
    sc_family_get_log_list_.set_page_num(page_num);
    sc_family_get_log_list_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyGetNextLeaderCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_get_next_leader_);
    role_info_t cur_leader;
    role_info_t next_leader;
    cur_leader.userid = cs_family_get_next_leader_.leader_id();
    cur_leader.u_create_tm = cs_family_get_next_leader_.leader_create_tm();

    int ret = g_family_member_table->get_top_construct_value_member(
            family_id, (uint32_t)commonproto::FAMILY_TITLE_VICE_LEADER, 
            cur_leader, next_leader);
    if (ret) {
        return ret; 
    }

    if (next_leader.userid == 0) {
        ret = g_family_member_table->get_top_construct_value_member(
                family_id, (uint32_t)commonproto::FAMILY_TITLE_MEMBER,
                cur_leader, next_leader);
        if (ret) {
            return ret; 
        }
    }

    sc_family_get_next_leader_.Clear();
    sc_family_get_next_leader_.set_next_leader_id(next_leader.userid);
    sc_family_get_next_leader_.set_next_leader_create_tm(next_leader.u_create_tm);
    sc_family_get_next_leader_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyGetRecommendListCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_get_recommend_list_);
    sc_family_get_recommend_list_.Clear();
    g_family_match_info_table->get_rand_match_list(
            cs_family_get_recommend_list_, 
            &sc_family_get_recommend_list_);
    sc_family_get_recommend_list_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyUpdateMatchInfoCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_update_match_info_);
	
    // 更新家族推荐库信息
    int ret = g_family_match_info_table->update_family_match_info(
            cs_family_update_match_info_.match_info(), 
            dbproto::DB_UPDATE_AND_INESRT);
    if (ret) {
        return ret; 
    }

	sc_family_update_match_info_.SerializeToString(&ack_body);
    return 0;
}


uint32_t FamilyChangeMatchInfoCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_change_match_info_);
    int ret = g_family_match_info_table->change_family_match_info(
            cs_family_change_match_info_.change_match_info());
    if (ret) {
        return ret; 
    }

    sc_family_change_match_info_.Clear();
    sc_family_change_match_info_.SerializeToString(&ack_body);
    return 0;
}

uint32_t FamilyDelMatchInfoCmdProcessor::process(
        uint32_t family_id, uint32_t u_create_tm, const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_family_del_match_info_);
    int ret = g_family_match_info_table->del_match_info(
            cs_family_del_match_info_.family_id());
    if (ret) {
        return ret; 
    }

    sc_family_del_match_info_.Clear();
    sc_family_del_match_info_.SerializeToString(&ack_body);
    return 0;
}

//global_attr TODO vince
uint32_t GetGlobalAttrCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    PARSE_DB_MSG(cs_get_global_attr_);

    const dbproto::global_attr_list_t& type_list = 
        cs_get_global_attr_.type_list();

    sc_get_global_attr_.Clear();
    dbproto::global_attr_list_t* list = sc_get_global_attr_.mutable_list();

    uint32_t err = g_global_attr_table->get_attr(type_list, list, cs_get_global_attr_.server_id());
    if (err) {
        return err; 
    }

    sc_get_global_attr_.SerializeToString(&ack_body);

    return 0;
}

uint32_t UpdateGlobalAttrCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    PARSE_DB_MSG(cs_update_global_attr_);

    const dbproto::global_attr_op_list_t& op_list = 
        cs_update_global_attr_.list();
    int op_list_size = op_list.op_list_size();

    for (int i = 0; i < op_list_size; i++) {
        const dbproto::global_attr_op_info_t& op_info = 
            op_list.op_list(i);
        uint32_t type = op_info.type();
        uint32_t subtype = op_info.subtype();
        uint32_t value = op_info.value();
        uint32_t limit = op_info.limit();
        uint32_t err = 0;
        if (op_info.op() == dbproto::GLOBAL_ATTR_OP_ADD) {
			sc_update_global_attr_.Clear();
            err = g_global_attr_table->add_attr_with_limit(type, subtype, value, 
                    limit, sc_update_global_attr_, cs_update_global_attr_.server_id());
			if (err) {
				return err; 
			}
			sc_update_global_attr_.SerializeToString(&ack_body);

        } else if (op_info.op() == dbproto::GLOBAL_ATTR_OP_MINUS) {
            err = g_global_attr_table->minus_attr(type, subtype, value, cs_update_global_attr_.server_id());
        } else {
            err = g_global_attr_table->set_attr(type, subtype, value, cs_update_global_attr_.server_id()); 
        }

        if (err) {
            return err;
        }
    }

    return 0;
}

uint32_t SetCliSoftvCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    PARSE_DB_MSG(cs_set_client_soft_);
    int ret = g_clisoftv_table->set_clisoft_v(userid, cs_set_client_soft_, u_create_tm);
    if (ret) {
        return ret;
    }
    sc_set_client_soft_.Clear();
    sc_set_client_soft_.SerializeToString(&ack_body);
    return 0;
}

uint32_t UseGiftCodeCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    PARSE_DB_MSG(cs_use_gift_code_);
    sc_use_gift_code_.Clear();
    uint32_t prize_id = 0;
    uint32_t status = 0;
    uint32_t used_userid = 0;

    int ret1 = g_gift_code_table->use_gift_code(userid, u_create_tm, 
            cs_use_gift_code_.svr_id(), cs_use_gift_code_.code());

    if (ret1 == 0) {//可用
        sc_use_gift_code_.set_status(0);
    }
    int ret2 = g_gift_code_table->get_gift_code(cs_use_gift_code_.code(), prize_id, status, used_userid);

    if (ret1 == 0) {//正常用掉
        if (ret2 == db_err_record_not_found) {//没查到 奇怪了
            ERROR_TLOG("GIFT_CODE_EXCEPTION code:%s USED BY User:%u", 
                    cs_use_gift_code_.code().c_str(), userid);
            sc_use_gift_code_.set_status(2);//不存在

        } else {//查到信息
            sc_use_gift_code_.set_status(0);
            sc_use_gift_code_.set_prize_id(prize_id);
        }

    } else {//不能用
        if (ret2 == db_err_record_not_found) {//不存在
            sc_use_gift_code_.set_status(2);

        } else {//存在不能用(过期或被删除或被使用)
            if (used_userid) { //被用
                sc_use_gift_code_.set_status(1);
            } else if (status != 0) {
                sc_use_gift_code_.set_status(3);
            } else {
                sc_use_gift_code_.set_status(4);
            }
        }
    }
    sc_use_gift_code_.set_prize_id(prize_id);
    sc_use_gift_code_.SerializeToString(&ack_body);
    return 0;
}

uint32_t GetLoginTmInfoCmdProcessor::process(
        userid_t userid, uint32_t u_create_tm, const std::string &req_body, std::string &ack_body)
{
    PARSE_DB_MSG(cs_get_login_tm_info_);
    sc_get_login_tm_info_.Clear();

    std::set<uint32_t> login_tm_vec;
    g_user_action_log_table->get_login_tm_info(userid, u_create_tm, 
            cs_get_login_tm_info_.from_tm(), cs_get_login_tm_info_.to_tm(), login_tm_vec);

    FOREACH(login_tm_vec, it) {
        sc_get_login_tm_info_.add_login_tm(*it);
    }
    sc_get_login_tm_info_.SerializeToString(&ack_body);
    return 0;
}

uint32_t SearchOccupyedMineCmdProcessor::process(userid_t userid, uint32_t u_create_tm,
		const std::string &req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_get_mine_info_);
	std::vector<uint64_t> my_ids_vec;
	for (int i = 0; i < cs_get_mine_info_.my_mine_id_size(); ++i) {
		my_ids_vec.push_back(cs_get_mine_info_.my_mine_id(i));
	}
	sc_get_mine_info_.Clear();
	commonproto::mine_info_list_t  pb_list;
	uint32_t ret = g_mine_info_table->get_mine_info_by_power_range(cs_get_mine_info_,
			&pb_list, cs_get_mine_info_.protect_duration());
	if (ret) {
		return ret;
	}
	if (!my_ids_vec.empty()) {
		commonproto::mine_info_list_t* pb_ptr = sc_get_mine_info_.mutable_list();
		for (int i = 0; i < pb_list.mine_info_size(); ++i) {
			uint64_t mine_id = pb_list.mine_info(i).mine_id();
			std::vector<uint64_t>::iterator it;
			it = std::find(my_ids_vec.begin(), my_ids_vec.end(), mine_id);
			if (it != my_ids_vec.end()) {
				continue;
			}
			pb_ptr->add_mine_info()->CopyFrom(pb_list.mine_info(i));
		}
	} else {
		sc_get_mine_info_.mutable_list()->CopyFrom(pb_list);
	}
	sc_get_mine_info_.SerializeToString(&ack_body);
	Utils::print_message(sc_get_mine_info_);
	return 0;
}

uint32_t SaveNewMineCmdProcessor::process(userid_t userid, uint32_t u_create_tm,
		const std::string &req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_save_mine_info_);
	sc_save_mine_info_.Clear();

	uint64_t new_mine_id = 0;
	int ret = g_mine_info_table->save_mine_info(cs_save_mine_info_.mine_info(), new_mine_id);
	if (ret) { return ret; }

	sc_save_mine_info_.set_mine_id(new_mine_id);
	sc_save_mine_info_.SerializeToString(&ack_body);
	Utils::print_message(sc_save_mine_info_);
	return 0;
}

uint32_t GetPlayerMineInfoCmdProcessor::process(userid_t userid, uint32_t u_create_tm,
		const std::string &req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_get_player_mine_info_);
	sc_get_player_mine_info_.Clear();
	for (int i = 0; i < cs_get_player_mine_info_.mine_id_size(); ++i) {
		uint64_t mine_id = cs_get_player_mine_info_.mine_id(i);
		int ret =  g_mine_info_table->get_player_mine_info(mine_id,
				sc_get_player_mine_info_.mutable_list());
		if (ret == db_err_record_not_found) {
			sc_get_player_mine_info_.add_expire_mine_ids(mine_id);
			continue;
		}
	}
	sc_get_player_mine_info_.SerializeToString(&ack_body);
	Utils::print_message(sc_get_player_mine_info_);
	return 0;
}

uint32_t UpdateMineCmdProcessor::process(userid_t userid, uint32_t u_create_tm,
		const std::string& req_body, std::string& ack_body)
{
    PARSE_DB_MSG(cs_update_mine_info_);
	int ret  = g_mine_info_table->update_mine_info(cs_update_mine_info_.mine_info());
	if (ret) { return ret;}
	sc_update_mine_info_.SerializeToString(&ack_body);
	return 0;
}

uint32_t DelMineInfoCmdProcessor::process(userid_t userid, uint32_t u_create_tm,
		const std::string& req_body, std::string& ack_body)
{
	PARSE_DB_MSG(cs_del_mine_info_);
	for (int i = 0; i < cs_del_mine_info_.mine_id_size(); ++i) {
		int ret = g_mine_info_table->delete_mine_info(cs_del_mine_info_.mine_id(i));
		if (ret == db_err_delete_mine) {
			continue;
		} else if (ret && ret != db_err_delete_mine) {
			return ret;
		} 
		sc_del_mine_info_.add_mine_id(cs_del_mine_info_.mine_id(i));
	}
	sc_del_mine_info_.SerializeToString(&ack_body);
	Utils::print_message(sc_del_mine_info_);
	return 0;
}

uint32_t IncreDefCntCmdProcessor::process(userid_t userid, uint32_t u_create_tm,
		const std::string& req_body, std::string& ack_body)
{
	PARSE_DB_MSG(cs_increment_defender_cnt_);
	uint64_t mine_id = cs_increment_defender_cnt_.mine_id();

	uint32_t is_been_attacked = 0;
	int ret = g_mine_info_table->get_attack_state(mine_id, is_been_attacked);
	if (ret && ret != db_err_record_not_found) {
		return ret;
	} 
	bool operate_state = true;
	if (is_been_attacked) {
		operate_state = false;
		sc_increment_defender_cnt_.set_operate_state(operate_state);
		sc_increment_defender_cnt_.set_mine_id(mine_id);
		sc_increment_defender_cnt_.SerializeToString(&ack_body);
		Utils::print_message(sc_increment_defender_cnt_);
		return 0;
	}

	ret = g_mine_info_table->increment_def_cnt(mine_id);
	if (ret == db_err_exceed_mine_def_cnt_limit) {
		operate_state = false;
	} else if (ret && ret != db_err_exceed_mine_def_cnt_limit) {
		return ret;
	}
	sc_increment_defender_cnt_.set_operate_state(operate_state);
	sc_increment_defender_cnt_.set_mine_id(mine_id);
	sc_increment_defender_cnt_.SerializeToString(&ack_body);
	Utils::print_message(sc_increment_defender_cnt_);
	return 0;
}
