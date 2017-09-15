#include "exped.h"
#include "player_utils.h"
#include "player.h"
#include "data_proto_utils.h"
#include "exped_utils.h"
#include "service.h"
#include "global_data.h"

uint32_t Expedition::init_all_cards_exped_info(
		const onlineproto::expedition_users_pet_info_list& exped_list)
{
	for (int i = 0; i < exped_list.list_info_size(); ++i) {
		const onlineproto::expedition_users_pet_info &pb_inf = exped_list.list_info(i);
		exped_info_t tmp;
		tmp.card_id = pb_inf.card_id();
		tmp.user_id = pb_inf.users();
		tmp.create_tm = pb_inf.create_tm();
		tmp.kill_flag = pb_inf.kill_flag();
		tmp.sex = pb_inf.sex();
		strncpy(tmp.nick, pb_inf.nick().c_str(), sizeof(tmp.nick));
		tmp.level = pb_inf.level();
		tmp.power = pb_inf.power();
		(pb_inf.exped_pet_list()).SerializeToString(&tmp.proto_btl_pets);
		exped_info_map_[tmp.card_id] = tmp;	
	}
	cur_card_id_ = exped_info_map_.size();
	return 0;
}

uint32_t Expedition::add_new_card_exped_info(
		const commonproto::battle_player_data_t& player_info)
{
	exped_info_t  exped_info;
	assert (cur_card_id_ < EXPED_HIGHEST_CARD_ID);
	exped_info.card_id = cur_card_id_ + 1;
	exped_info.user_id = player_info.base_info().user_id();	
	exped_info.create_tm = player_info.base_info().create_tm();
	exped_info.kill_flag = false;
	exped_info.sex = player_info.base_info().sex();
	strncpy(exped_info.nick, player_info.base_info().nick().c_str(), sizeof(exped_info.nick));
	exped_info.level = player_info.base_info().level();
	exped_info.power = player_info.base_info().power();

	onlineproto::exped_pet_info_list  pb_exped_list;
	ExpedUtils::pack_battle_exped_n_topest_pet_info(
			&pb_exped_list, player_info.power_pet_list());

	pb_exped_list.SerializeToString(&exped_info.proto_btl_pets);
	exped_info_map_.insert(std::pair<uint32_t, exped_info_t>(exped_info.card_id, exped_info));	
	set_cur_card_id(exped_info.card_id);
	return 0;
}

uint32_t Expedition::pack_one_ai_exped_info_to_pbmsg(
		const exped_info_t& exped_info,
		onlineproto::expedition_users_pet_info* pet_ptr)
{
	pet_ptr->set_card_id(exped_info.card_id);
	pet_ptr->set_users(exped_info.user_id);
	pet_ptr->set_create_tm(exped_info.create_tm);
	pet_ptr->set_kill_flag(exped_info.kill_flag);
	pet_ptr->set_sex(exped_info.sex);
	pet_ptr->set_nick(exped_info.nick);
	pet_ptr->set_level(exped_info.level);
	pet_ptr->set_power(exped_info.power);
	pet_ptr->mutable_exped_pet_list()->ParseFromString(exped_info.proto_btl_pets);
	return 0;
}

exped_info_t* Expedition::get_exped_info_by_card_id(uint32_t card_id)
{
	if (exped_info_map_.count(card_id) == 0) {
		return NULL;
	}
	ExpedInfoMap::iterator it = exped_info_map_.find(card_id);
	return &it->second;
}

uint32_t Expedition::change_ai_pets_hp(player_t* player,
		const onlineproto::expedition_pet_cur_hp_list& pb_pets_hp)
{
	onlineproto::exped_pet_info_list pb_exped_pets;
	pb_exped_pets.ParseFromString(exped_info_map_[cur_card_id_].proto_btl_pets);
	for (int idx = 0; idx < pb_exped_pets.exped_pets_size(); ++idx) {
		uint32_t create_tm = pb_exped_pets.exped_pets(idx).create_tm();
		uint32_t old_exped_hp = pb_exped_pets.exped_pets(idx).exped_hp();
		for (int j = 0; j < pb_pets_hp.cur_hp_size(); ++j) {
			const onlineproto::expedition_pet_cur_hp&  pb_inf = pb_pets_hp.cur_hp(j);
			if (pb_inf.create_tm() == create_tm) {
				if (pb_inf.exped_cur_hp() / (EXPED_HP_COE * 1.0) > old_exped_hp) {
					ERROR_TLOG("Change Ai Pets Hp Err, cur_hp=[%u],client_hp=[%u]",
							old_exped_hp, pb_inf.exped_cur_hp());
					return cli_err_ai_pets_hp_sent_client_err;
				}
				onlineproto::exped_pet_info *pb_ptr = pb_exped_pets.mutable_exped_pets(idx);
				pb_ptr->set_exped_hp(pb_inf.exped_cur_hp() / (EXPED_HP_COE * 1.0));
			}
		}
	}
	pb_exped_pets.SerializeToString(&(exped_info_map_[cur_card_id_].proto_btl_pets));

	//修改血量到btl_pet_info_，以及rawdata中
	commonproto::battle_pet_list_t pb_btl_inf;
	pb_btl_inf.ParseFromString(btl_pet_info_);
	ExpedUtils::exped_modify_ai_pet_hp(pb_btl_inf, pb_exped_pets);
	btl_pet_info_.clear();
	pb_btl_inf.SerializeToString(&btl_pet_info_);
	PlayerUtils::update_user_raw_data(
			player->userid, player->create_tm, dbproto::EXPED_CUR_CARD_PETS,
			pb_btl_inf, "0");
	return 0;
}

uint32_t Expedition::clear_ai_pets_hp(player_t* player)
{
	onlineproto::exped_pet_info_list pb_exped_pets;
	pb_exped_pets.ParseFromString(exped_info_map_[cur_card_id_].proto_btl_pets);
	for (int i = 0; i < pb_exped_pets.exped_pets_size(); ++i) {
		onlineproto::exped_pet_info* pb_ptr = pb_exped_pets.mutable_exped_pets(i);
		pb_ptr->set_exped_hp(0);
	}
	pb_exped_pets.SerializeToString(&(exped_info_map_[cur_card_id_].proto_btl_pets));
	commonproto::battle_pet_list_t pb_btl_inf;
	pb_btl_inf.ParseFromString(btl_pet_info_);
	ExpedUtils::exped_modify_ai_pet_hp(pb_btl_inf, pb_exped_pets);
	btl_pet_info_.clear();
	pb_btl_inf.SerializeToString(&btl_pet_info_);
	PlayerUtils::update_user_raw_data(
			player->userid, player->create_tm, dbproto::EXPED_CUR_CARD_PETS,
			pb_btl_inf, "0");
	return 0;
}

bool Expedition::check_cur_card_kill_state ()
{
	if (exped_info_map_.count(cur_card_id_) == 0) {
		return false;
	}
	return ((exped_info_map_.find(cur_card_id_))->second).kill_flag;
}

bool Expedition::check_kill_state_by_card(uint32_t card_id)
{
	if (exped_info_map_.count(card_id) == 0) {
		return false;
	}
	return ((exped_info_map_.find(card_id))->second).kill_flag;
}

uint32_t Expedition::pack_cur_card_exped_info(
		onlineproto::expedition_users_pet_info* pet_ptr)
{
	if (exped_info_map_.count(cur_card_id_) == 0) {
		ERROR_TLOG("exped_info_map_:not found card_id:%u", cur_card_id_);
		return cli_err_exped_cur_card_info_err;
	}
	ExpedInfoMap::iterator it = exped_info_map_.find(cur_card_id_);
	pack_one_ai_exped_info_to_pbmsg(it->second, pet_ptr);
	pet_ptr->mutable_power_pet_list()->ParseFromString(btl_pet_info_);
	return 0;
}

uint32_t Expedition::clear_exped_info(player_t* player)
{
	exped_info_map_.clear();
	cur_card_id_ = 0;
	dbproto::cs_user_raw_data_del db_in_;
	db_in_.set_type(dbproto::EXPED_PETS_INFO);
	db_in_.set_buff_id("0");
	g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
			db_cmd_user_raw_data_del, db_in_);
	db_in_.Clear();
	db_in_.set_type(dbproto::EXPED_CUR_CARD_PETS);
	db_in_.set_buff_id("0");
	g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
			db_cmd_user_raw_data_del, db_in_);
	db_in_.Clear();
	db_in_.set_type(dbproto::EXPED_TOTAL_PRIZE);
	db_in_.set_buff_id("0");
	g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
			db_cmd_user_raw_data_del, db_in_);
	return 0;
}

uint32_t Expedition::get_exped_ai_uids(std::set<uint64_t>& uids)
{
	FOREACH(exped_info_map_, it) {
		uids.insert(ROLE_KEY(ROLE(it->second.user_id, it->second.create_tm)));
	}
	return 0;
}

uint32_t Expedition::pack_all_exped_info_to_pbmsg(
	onlineproto::expedition_users_pet_info_list* pb_exped_ptr)
{
	FOREACH(exped_info_map_, it) {
		pack_one_ai_exped_info_to_pbmsg(it->second, pb_exped_ptr->add_list_info());
	}
	return 0;
}
