#include <boost/lexical_cast.hpp>
#include "common.h"
#include "proto/db/dbproto.mail.pb.h"
#include "proto/client/pb0x05.pb.h"
#include "mail_utils.h"
#include "service.h"
#include "global_data.h"
#include "attr_utils.h"
#include "pet_utils.h"
#include "time_utils.h"
#include "item.h"
#include "rune_utils.h"
#include "title.h"
#include "player_manager.h"
#include "player.h"

uint32_t MailUtils::add_player_new_mail(player_t* player, 
        const new_mail_t &new_mail, bool wait_db_ret)
{
	dbproto::cs_mail_new db_in;   
	commonproto::mail_data_t *mail_data = db_in.mutable_mail_data();
		                                                                 
	mail_data->set_mailid(Utils::gen_uuid());
    mail_data->set_status(commonproto::UNREAD);
    mail_data->set_send_time(NOW());
	mail_data->set_sender(new_mail.sender);
	mail_data->set_title(new_mail.title);                        
	mail_data->set_content(new_mail.content);
	if (!new_mail.attachment.empty()) {
		mail_data->set_attachment(new_mail.attachment);
	}

    if (wait_db_ret) {
        return g_dbproxy->send_msg(player, player->userid,player->create_tm,
			db_cmd_mail_new, db_in);
    }

    int ret = g_dbproxy->send_msg(0, player->userid,player->create_tm,
            db_cmd_mail_new, db_in);
    if (ret) {
		return cli_err_sys_err;
	}

	onlineproto::sc_0x0505_inform_new_mail sc_inform_new_mail;
	commonproto::mail_data_t *cli_mail_data = sc_inform_new_mail.add_mail();
    cli_mail_data->CopyFrom(*mail_data);

	//通知客户端
	return send_msg_to_player(player, cli_cmd_cs_0x0505_inform_new_mail, sc_inform_new_mail);
}

uint32_t MailUtils::get_mail_attach(player_t* player, const std::vector<attachment_t>& attach_vec)
{
    std::vector<add_item_info_t> add_item_vec;
    std::map<uint32_t, uint32_t> set_attr_map;
    std::map<uint32_t, uint32_t> add_pet_map;

	for (uint32_t i = 0; i < attach_vec.size(); i++) {
		if (attach_vec[i].type == kMailAttachItem) {
			add_item_info_t add_item;
			add_item.item_id = attach_vec[i].id;
			add_item.count = attach_vec[i].count;
			add_item_vec.push_back(add_item);
		}

		if (attach_vec[i].type == kMailAttachAttr) {
			uint32_t value = GET_A((attr_type_t)attach_vec[i].id);
			value += attach_vec[i].count;
            set_attr_map[attach_vec[i].id] = value;
		}

		if (attach_vec[i].type == kMailAttachTitle) {
			title_info_t title_info;
			title_info.title_id = attach_vec[i].id;
			title_info.get_time = NOW();
			uint32_t ret = player->title->add_one_title(player, title_info, true);
			if (ret) {
				ERROR_TLOG("Mail Attach,Add Title Failed,"
						"uid=%u,title_id=%u", player->userid,
						title_info.title_id);
			}
		}

		if (attach_vec[i].type == kMailAttachPet) {
            add_pet_map[attach_vec[i].id] = attach_vec[i].count;
		}

		if (attach_vec[i].type == kMailAttachRune) {
			uint32_t conf_id = attach_vec[i].id;
			uint32_t level = attach_vec[i].count;
			uint32_t ret = RuneUtils::add_rune(player, conf_id, level);
			if (ret) {
				ERROR_TLOG("Mail Attach,Add Rune Failed, "
						"uid=[%u],conf_id=[%u],level=[%u]", 
						player->userid, conf_id, level);
			}
		}
	}
    //先判定是否可以加物品
    std::vector<attachment_t> tmp_attach_vec;
    int ret = check_swap_item_by_item_id(player, 0, &add_item_vec, 
            NO_ADDICT_DETEC, &tmp_attach_vec);
    if (ret || tmp_attach_vec.size()) {
        return cli_err_item_exceed_max_own;
    }
    
    //如果物品可以加了 再判断是否可以加精灵
    FOREACH(add_pet_map, it) {
        uint32_t id = it->first;
        if (PetUtils::check_can_create_pet(player, id) == false) {
            return cli_err_already_own_pet;
        }
    }
    //再判断是否可以加称号

    //都可以加了
    //先加物品
    swap_item_by_item_id(player, 0, &add_item_vec, NO_WAIT_SVR,
            onlineproto::SYNC_REASON_NONE, NO_ADDICT_DETEC);
    //创建精灵
    FOREACH(add_pet_map, it) {
        uint32_t id = it->first;
        uint32_t lv = it->second;
        PetUtils::create_pet(player, id, lv, 0, 0);
    }
    //改属性
    AttrUtils::set_attr_value(player, set_attr_map);
	return 0;
}

//序列化奖励 到 附件
uint32_t MailUtils::serialize_prize_to_attach_string(
		const std::vector<cache_prize_elem_t>& prize_vec, 
		std::string &attachment)
{
	attachment.clear();
	uint32_t type;
	std::string tmp_str;
	FOREACH (prize_vec, it) {
		type = (uint32_t)it->type;
		tmp_str = boost::lexical_cast<string>(type) + "," 
		+ boost::lexical_cast<string>(it->id) + ","
		+ boost::lexical_cast<string>(it->count) + ";"; 
		attachment += tmp_str;
	}
	return 0;
}

uint32_t MailUtils::serialize_attach_to_attach_string(
		const std::vector<attachment_t> &attach_vec, 
		std::string &attachment)
{
	attachment.clear();
	uint32_t type;
	std::string tmp_str;
	FOREACH (attach_vec, it) {
		type = (uint32_t)it->type;
		tmp_str = boost::lexical_cast<string>(type) + "," 
		+ boost::lexical_cast<string>(it->id) + ","
		+ boost::lexical_cast<string>(it->count) + ";"; 
		attachment += tmp_str;
	}
	return 0;
}

uint32_t MailUtils::parse_attach_from_string(const string &attach_str,
        std::vector<attachment_t> &attach_vec)
{
    std::vector<std::string> attach_str_list = split(attach_str, ';');
    for (uint32_t k = 0; k < attach_str_list.size(); k++){
        attachment_t attachment; 
        std::vector<std::string> attach_pair = split(attach_str_list[k].c_str(), ',');
        if (attach_pair.size() == 3) {
            attachment.type = mail_attach_type_t(atoi(attach_pair[0].c_str()));
            attachment.id = atoi(attach_pair[1].c_str());
            attachment.count = atoi(attach_pair[2].c_str());
            attach_vec.push_back(attachment);
        }
    }
    return 0;
}

uint32_t MailUtils::pack_prize_to_mail_attach(
		const std::vector<cache_prize_elem_t>& prize_vec,
		std::vector<attachment_t>& attaches)
{
	FOREACH(prize_vec, it) {
		struct attachment_t tmp;
		tmp.type = (mail_attach_type_t)it->type;
		tmp.id = it->id;
		tmp.count = it->count;
		attaches.push_back(tmp);
	}
	return 0;
}


uint32_t MailUtils::send_mail_to_player(player_t* player, 
		uint32_t r_userid, uint32_t r_create_tm, 
		const new_mail_t &new_mail, bool wait_cb)
{
	//判断是否是同服且在线
	player_t* ply = g_player_manager->get_player_by_userid(r_userid);
	if(ply != NULL){//直接发,不回调
		add_player_new_mail(ply, new_mail, 0);
	} else {//通过switch转发
		switchproto::cs_sw_gm_new_mail  sw_in_; 
		commonproto::mail_data_t *mail_data = sw_in_.mutable_mail_data();
		sw_in_.set_userid(r_userid);
		sw_in_.set_u_create_tm(r_create_tm);
		sw_in_.set_wait_cmd(wait_cb);
		std::string tmp_mailid="0";
		mail_data->set_mailid(tmp_mailid);
		mail_data->set_status(commonproto::UNREAD);
		mail_data->set_send_time(NOW());
		mail_data->set_sender(new_mail.sender);
		mail_data->set_title(new_mail.title);                        
		mail_data->set_content(new_mail.content);
		mail_data->set_attachment(new_mail.attachment);

		if (wait_cb) {
			return g_switch->send_msg(player, r_userid, r_create_tm,
					sw_cmd_sw_gm_new_mail, sw_in_);
		}

		return g_switch->send_msg(0, r_userid, r_create_tm,
				sw_cmd_sw_gm_new_mail, sw_in_);
	}
	return 0;
}
