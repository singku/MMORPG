#include "night_raid.h"
#include "player_utils.h"
#include "player.h"
#include "data_proto_utils.h"
#include "night_raid.h"
#include "service.h"
#include "global_data.h"
#include "player_manager.h"
#include "utils.h"
#include "cmd_processor_interface.h"

uint32_t  NightRaid::get_opponent_uids(std::set<uint64_t>& uids)
{
	FOREACH(night_raid_info_map_, it) {
		uids.insert(ROLE_KEY(ROLE(it->second.user_id, it->second.create_tm)));
	}
	return 0;
}
uint32_t  NightRaid::get_cur_opponent_uid()
{
   	night_raid_info_t *cur_info = get_night_raid_info_by_card_id(cur_card_id);
	if(NULL == cur_info){
		ERROR_TLOG("night_raid_info_map_:not found card_id:%u", cur_card_id);
		return cli_err_night_raid_cur_card_info_err;
	}
	return cur_info->user_id;
}
uint32_t NightRaid::add_new_card_night_raid_info(
		const commonproto::battle_player_data_t& player_info)
{
	night_raid_info_t night_raid_info;
	// assert (cur_card_id =< NIGHT_RAID_HIGHEST_CARD_ID);
	// night_raid_info.card_id = cur_card_id + 1;
	night_raid_info.card_id = cur_card_id ;
	night_raid_info.user_id = player_info.base_info().user_id();	
	night_raid_info.create_tm = player_info.base_info().create_tm();	
	night_raid_info.sex = player_info.base_info().sex();
    //size调用不包含0结尾的字节
    size_t nick_len = player_info.base_info().nick().size();
    size_t max_cpy_len = sizeof(night_raid_info.nick) - 1;
    memset(night_raid_info.nick, 0, sizeof(night_raid_info.nick));
    nick_len = nick_len > max_cpy_len ?max_cpy_len :nick_len;
	strncpy(night_raid_info.nick, player_info.base_info().nick().c_str(), nick_len);
	night_raid_info.cur_hp = player_info.battle_info().cur_hp();
	night_raid_info.max_hp = player_info.battle_info().max_hp();
	night_raid_info.fight_value = player_info.base_info().power();

	const commonproto::battle_pet_list_t& pet_info = player_info.pet_list();
	night_raid_info.pet_ids[0] = pet_info.pet_list_size();
	for (int i = 0; i < pet_info.pet_list_size(); ++i) {
		night_raid_info.pet_ids[i + 1] = pet_info.pet_list(i).pet_info().base_info().pet_id();

	}
	// night_raid_info_map_.insert(std::pair<uint32_t, night_raid_info_t >( night_raid_info.card_id, night_raid_info));	
	night_raid_info_map_[night_raid_info.card_id] = night_raid_info;	
	// set_cur_card_id(night_raid_info.card_id);
	return 0;
}

uint32_t  NightRaid::pack_night_raid_info_to_msg(
		const night_raid_info_t& night_raid_info,
		commonproto::pvep_match_data_t* op_ptr)
{
	op_ptr->set_card_id( night_raid_info.card_id);
	op_ptr->set_userid( night_raid_info.user_id);
	op_ptr->set_create_tm( night_raid_info.create_tm);
	op_ptr->set_sex( night_raid_info.sex);
	op_ptr->set_nick(std::string(night_raid_info.nick));
	op_ptr->set_fight_value(night_raid_info.fight_value);
	op_ptr->set_cur_hp(night_raid_info.cur_hp);
	op_ptr->set_max_hp(night_raid_info.max_hp);
	for(uint32_t i = 1; i <= night_raid_info.pet_ids[0]; i++){
		op_ptr->add_pet_id(night_raid_info.pet_ids[i]);
	}
	return 0;
}
uint32_t  NightRaid::unpack_msg_list_to_night_raid_info_map(
	   	commonproto::pvep_opponent_list& op_ptr)
{
	night_raid_info_map_.clear();
	for(int i = 0; i < op_ptr.opponent_data().size(); i++){
		const commonproto::pvep_match_data_t& op_inf = op_ptr.opponent_data(i);
		night_raid_info_t tmp;
		unpack_msg_to_night_raid_info(tmp, op_inf);
		night_raid_info_map_[tmp.card_id] = tmp;
	}
	return 0;
}
uint32_t  NightRaid::unpack_msg_to_night_raid_info(
		night_raid_info_t& night_raid_info,
		const commonproto::pvep_match_data_t& op_ptr)
{
	night_raid_info.card_id = op_ptr.card_id();
	night_raid_info.user_id = op_ptr.userid();
	night_raid_info.create_tm = op_ptr.create_tm();
	night_raid_info.sex= op_ptr.sex();

	std::string str = op_ptr.nick();
    size_t nick_len = str.size();
    size_t max_cpy_len = sizeof(night_raid_info.nick) - 1;
    nick_len = nick_len > max_cpy_len ?max_cpy_len : nick_len;
    memset(night_raid_info.nick, 0, sizeof(night_raid_info.nick));
	strncpy(night_raid_info.nick, str.c_str(), nick_len);

	night_raid_info.fight_value= op_ptr.fight_value();
	night_raid_info.cur_hp = op_ptr.cur_hp();
	night_raid_info.max_hp= op_ptr.max_hp();
	night_raid_info.pet_ids[0] = op_ptr.pet_id().size();
	for(int i = 0; i < op_ptr.pet_id().size(); i++){
		night_raid_info.pet_ids[i + 1] = op_ptr.pet_id(i);
	}
	return 0;
}
uint32_t  NightRaid::pack_cur_card_night_raid_info(
		commonproto::pvep_match_data_t* op_ptr)
{
	if (night_raid_info_map_.count(cur_card_id) == 0) {
		ERROR_TLOG("night_raid_info_map_:not found card_id:%u", cur_card_id);
		return cli_err_night_raid_cur_card_info_err;
	}

	NightRaidInfoMap::iterator it = night_raid_info_map_.find(cur_card_id);
	pack_night_raid_info_to_msg(it->second, op_ptr);
	return 0;
}

uint32_t  NightRaid::pack_all_card_night_raid_info(
		commonproto::pvep_opponent_list& op_list)
{
	FOREACH(night_raid_info_map_, it) {
		pack_night_raid_info_to_msg(it->second, op_list.add_opponent_data());
	}
	return 0;
}

night_raid_info_t*  NightRaid::get_night_raid_info_by_card_id(uint32_t card_id)
{
	if (night_raid_info_map_.count(card_id) == 0) {
		ERROR_TLOG("night_raid_info_map_:not found card_id:%u", cur_card_id);
		return NULL;
	}
	NightRaidInfoMap::iterator it = night_raid_info_map_.find(card_id);
	return &it->second;
}

uint32_t  NightRaid::get_cur_night_raid_battle_info(
		commonproto::battle_player_data_t* btl_info)
{

	if(btl_player_info_.size() >= 4096){
		DEBUG_TLOG("raw_data over limit size=[%u]", btl_player_info_.size());
		ERROR_TLOG("raw_data over limit size = %u", btl_player_info_.size());
	}

	std::string name = btl_info->GetTypeName();
	// if(!btl_info->ParseFromString(btl_player_info_)){
	if(!btl_info->ParsePartialFromString(btl_player_info_)){
		std::string errstr = btl_info->InitializationErrorString();
		ERROR_TLOG("PARSE MSG '%s' failed, err = '%s'", 
				name.c_str(), errstr.c_str());
		return cli_err_night_raid_battle_info_parse_err;
	}
	return 0;
}

bool NightRaid::is_frist_time_play_night_raid()
{
    if (cur_card_id == 0 && 0 == cur_win_id) {//第一次玩
		return true;
    } else{
		return false;
	}
}

bool NightRaid::is_cur_phases_win_night_raid()
{
    if (cur_card_id == cur_win_id) {//第一次玩
		return true;
    } else{
		return false;
	}
}
uint32_t NightRaid::get_cur_power_percent()
{
	if(cur_card_id > 10 || cur_card_id <= 0 ){
		ERROR_TLOG("night_raid_ match power percent err cardid :%u", cur_card_id);
		return cli_err_night_raid_cur_card_info_err;
	}
	//匹配算数据法
	uint32_t data[][2] = {
		{1, 650},
		{2, 700},
		{3, 750},
		{4, 850},
		{5, 900},
		{6, 900},
		{7, 1000},
		{8, 1000},
		{9, 1100},
		{10, 1200}
	};
	return data[cur_card_id - 1][1];
}
void NightRaid::get_opponent_match_args(match_info_t& data, uint32_t cur_power)
{
	//匹配算法
	uint32_t power_percent = 0;
	power_percent = get_cur_power_percent();
	uint32_t percent = power_percent / 1000.0 * 100;
	uint32_t low_percent = 0, high_percent = 0;
	if (percent <= 1) {
		low_percent = percent;
	} else {
		low_percent = percent - 1;
	}
	high_percent = percent + 1;
	data.low_score = low_percent / 100.0 * cur_power;
	data.high_score = high_percent / 100.0 * cur_power;
	data.unit_val= ceil(cur_power / 100.0);
}
uint32_t NightRaid::get_cur_id_from_attr_data(uint32_t data) 
{
	if(data == 0){
		return 0;
	}
	uint32_t count = 0;
	for(uint32_t i = 1;i <= 32; i++){
		if(taomee::test_bit_on(data, i)){
			count ++;
		} else{
			break;
		}
	}
	return count;
}
