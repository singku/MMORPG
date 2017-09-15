#ifndef __BLESS_PET_UTILS_H__
#define __BLESS_PET_UTILS_H__

#include <boost/lexical_cast.hpp>
#include "player.h"
#include "global_data.h"
#include "bless_pet_conf.h"
#include "pet_utils.h"
#include "common.h"

class BlessPetUtils {
	public:
		static bool check_has_bless_pet(player_t *player, const uint32_t bless_id);
		static bool check_other_has_bless_pet(const commonproto::pet_list_t &pet_info, const uint32_t bless_id);
		static int update_bless_team_info_to_redis(player_t *player, const commonproto::bless_pet_team_info_t &team_info);
		static int delete_bless_team_info_to_redis(player_t *player);

		//保存要拉取的队伍信息
		static int push_bless_team_list(player_t *player);
		static int get_bless_team_list(player_t *player);

		inline static int parse_team_info_from_string(commonproto::bless_pet_team_info_t &team_info, const std::string &pkg){

			std::string name = team_info.GetTypeName();
			if(!team_info.ParseFromString(pkg)){
				std::string errstr = team_info.InitializationErrorString();
				ERROR_TLOG("PARSE MSG '%s' failed, err = '%s'", 
						name.c_str(), errstr.c_str());
				return cli_err_bless_team_info_parse_err;
			}
			return 0;
		}

		inline static int parse_battle_info_from_string(commonproto::battle_player_data_t* btl_info, const std::string &pkg){

			std::string name = btl_info->GetTypeName();
			if(!btl_info->ParseFromString(pkg)){
				std::string errstr = btl_info->InitializationErrorString();
				ERROR_TLOG("PARSE MSG '%s' failed, err = '%s'", 
						name.c_str(), errstr.c_str());
				return cli_err_bless_pet_battle_info_parse_err;
			}
			return 0;
		}

};
#endif
