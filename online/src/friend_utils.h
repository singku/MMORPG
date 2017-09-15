#ifndef FRIEND_UTILS_H
#define FRIEND_UTILS_H

#include <stdint.h>
#include <libtaomee/project/types.h>
#include "player.h"
#include "friend.h"

class FriendUtils
{
public:
	static uint32_t pack_friend_cache_data_to_online(
			const commonproto::player_base_info_t& base_info,
			commonproto::friend_info_t* friend_info);

	static uint32_t pack_friend_data_to_player(
			const commonproto::player_base_info_t& base_info,
			friend_t* friend_info);
	static uint32_t pack_blacklist_data_to_player(
			const commonproto::player_base_info_t& base_info,
			black_t* black_info);

	static uint32_t proc_recent_list(player_t* player,
			uint32_t recent_id, uint32_t create_tm);
	static uint32_t pack_friend_cache_data_to_recommendation(
			const commonproto::player_base_info_t& base_info,
			recommendation_t& recommendation);
};

#endif
