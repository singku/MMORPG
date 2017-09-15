#include "mine.h"
#include "player.h"
#include "rank_utils.h"
#include "mine_utils.h"
#include <boost/lexical_cast.hpp>

uint32_t MineInfo::add_mine_tmp_info_map(struct mine_info_t& mine_info)
{
	uint32_t size = mine_tmp_info_map_.size();
	mine_info.mine_id = size + 1;
	mine_tmp_info_map_[mine_info.mine_id] = mine_info;
	return 0;
}

uint32_t MineInfo::pack_match_mine_ids(
		commonproto::mine_id_list_t& pb_ids)
{
	FOREACH(match_mine_ids_, it) {
		pb_ids.add_mine_id(*it);
	}
	return 0;
}

uint32_t MineInfo::save_mine_tmp_info_to_memory(uint64_t mine_id,
		struct mine_info_t& mine_info) {
	mine_tmp_info_map_[mine_id] = mine_info;
	return 0;
}

uint32_t MineInfo::get_mine_tmp_info(uint64_t index, struct mine_info_t& mine_info)
{
	if (mine_tmp_info_map_.count(index) == 0) {
		return cli_err_not_found_this_mine;
	}
	mine_info = mine_tmp_info_map_[index];
	return 0;
}

uint32_t MineInfo::erase_single_mine_from_mine_tmp_info(uint64_t mine_id)
{
	MineTmpInfoMap::iterator it = mine_tmp_info_map_.find(mine_id);
	if (it == mine_tmp_info_map_.end()) {
		return cli_err_not_found_this_mine;
	}
	mine_tmp_info_map_.erase(mine_id);
	return 0;
}

uint32_t MineInfo::get_mine_info_from_memory(uint64_t mine_id,
		struct mine_info_t& mine_info)
{
	if (mine_info_map_.count(mine_id) == 0) {
		return cli_err_not_found_this_mine;
	}
	mine_info = mine_info_map_[mine_id];
	return 0;
}

bool MineInfo::check_mine_id_exist_in_mine_info_map(uint64_t mine_id)
{
	return mine_info_map_.count(mine_id) ? true : false;
}

uint32_t MineInfo::get_match_mine_info_from_memory(uint64_t mine_id,
		struct mine_info_t& mine_info)
{
	if (match_mine_info_.count(mine_id) == 0) {
		return cli_err_match_mine_not_found;
	}
	mine_info = match_mine_info_[mine_id];
	return 0;
}

uint32_t MineInfo::save_matched_mine_info_to_memory(struct mine_info_t& mine_info)
{
	/*
	uint64_t mine_id = mine_inf.mine_id();
	if (match_mine_info_.count(mine_id)) {
		return cli_err_refresh_match_mine_info_err;
	}
	mine_info_t mine_info;
	MineUtils::unpack_mine_info(mine_inf, mine_info);
	*/
	if (match_mine_info_.count(mine_info.mine_id)) {
		return cli_err_refresh_match_mine_info_err;
	}
	match_mine_info_.insert(make_pair(mine_info.mine_id, mine_info));
	return 0;
}

uint32_t MineInfo::insert_player_mine_info_to_memory(uint64_t mine_id,
		const struct mine_info_t& mine_info)
{
	if (mine_info_map_.count(mine_id)) {
		return cli_err_sys_busy;
	}
	mine_info_map_.insert(make_pair(mine_id, mine_info));
	return 0;
}

uint32_t MineInfo::get_mine_ids_from_memory(std::vector<uint64_t>& mine_ids)
{
	mine_ids = mine_id_vec_;
	return 0;
}

uint32_t MineInfo::delete_pet_info_in_memory(uint64_t mine_id)
{
	if (pet_info_map_.count(mine_id) == 0) {
		return cli_err_this_mine_not_pet_defend;
	}
	pet_info_map_.erase(mine_id);
	return 0;
}

uint32_t MineInfo::get_pet_info_from_memory(uint64_t mine_id, std::string& pet_info_pkg)
{
	if (pet_info_map_.count(mine_id) == 0) {
		return cli_err_this_mine_not_pet_defend;
	}
	pet_info_pkg = pet_info_map_[mine_id];
	return 0;
}

uint32_t MineInfo::pack_mine_ids(commonproto::mine_id_list_t& pb_ids)
{
	FOREACH(mine_id_vec_, it) {
		pb_ids.add_mine_id(*it);
	}
	return 0;
}

uint32_t MineInfo::insert_my_mine_ids_to_memory(uint64_t mine_id)
{
	std::vector<uint64_t>::iterator it;
	it = std::find(mine_id_vec_.begin(), mine_id_vec_.end(), mine_id);
	if (it != mine_id_vec_.end()) {
		return cli_err_save_mine_id_err;
	}
	mine_id_vec_.push_back(mine_id);
	return 0;
}

uint32_t MineInfo::delete_my_mine_id_from_memory(uint64_t mine_id)
{
	std::vector<uint64_t>::iterator it;
	it = std::find(mine_id_vec_.begin(), mine_id_vec_.end(), mine_id);
	if (it == mine_id_vec_.end()) {
		return cli_err_can_not_del_the_no_exist_mine;
	}
	mine_id_vec_.erase(it);
	return 0;
}
