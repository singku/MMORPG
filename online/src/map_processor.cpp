#include <libtaomee++/utils/strings.hpp>

#include "player.h"
#include "map_processor.h"
#include "map_utils.h"
#include "proto/client/cli_errno.h"
#include "proto/client/cli_cmd.h"

#include "data_proto_utils.h"
#include "service.h"
#include "attr_utils.h"
#include "utils.h"
#include "global_data.h"
#include "pet_utils.h"
#include "player_manager.h"
#include "home_data.h"
#include "family_utils.h"
#include "player_utils.h"
#include "task_utils.h"

enum {
    PLAYER_TYPE = 1,
    PET_TYPE = 2,
    MONSTER_TYPE = 3
};

int EnterMapCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;

    // TODO toby register func
    uint32_t family_id = GET_A(kAttrFamilyId);
    if (cli_in_.mapid() == FAMILY_MAP_ID){
        // 进入家族大厅
        const map_conf_t *map = MapUtils::get_map_conf(cli_in_.mapid());
        if (map == NULL) {
            RET_ERR(cli_err_map_not_exist); 
        }

        // 没加入家族，不能进入家族专属大厅
        if (!FamilyUtils::is_valid_family_id(family_id)) {
            return send_err_to_player(
                    player, player->cli_wait_cmd, cli_err_family_id_illegal);
        }

        if (player->cur_map_id == FAMILY_MAP_ID) {
            // 已经在家族大厅中 离开家族大厅
            homeproto::cs_leave_family_hall leave_info;
		    g_dbproxy->send_msg(
                    0, player->userid, player->create_tm, home_cmd_leave_family_hall, leave_info);
        }

        // 离开之前的地图
        if(player->cur_map_id > 0 && player->cur_map_id != FAMILY_MAP_ID) {
            MapUtils::leave_map(player);
        }

        // 离开小屋
		PlayerUtils::leave_current_home(player);

        // 进入前被踢出家族??
        homeproto::cs_enter_family_hall enter_info;
        enter_info.set_map_id(cli_in_.mapid());
        enter_info.set_x_pos(cli_in_.x_pos());
        enter_info.set_y_pos(cli_in_.y_pos());
        enter_info.set_heading(cli_in_.heading());
        commonproto::map_player_data_t *map_player = enter_info.mutable_player();
        DataProtoUtils::pack_map_player_info(player, map_player);
		return g_dbproxy->send_msg(
                player, player->userid, player->create_tm, home_cmd_enter_family_hall, enter_info);
    } else if (cli_in_.mapid() == MEDAL_HALL_MAP_ID) {
		TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_MEDAL_HALL, 1);
	} else if (cli_in_.mapid() == BUSINESS_STREET) {
		TaskUtils::update_task_condition_attr(player, REWARD_TASK_BUSINESS_STREET, 1);
	}
    
    // 进入普通地图
    cli_out_.Clear();
	PlayerUtils::leave_current_home(player);

    int err = MapUtils::enter_map(
            player,cli_in_.mapid(), cli_in_.x_pos(), cli_in_.y_pos(), cli_in_.heading());
    if (err) {
        return send_err_to_player(player, player->cli_wait_cmd, err);
    }

    player->cur_map_id = cli_in_.mapid();
    
    cli_out_.set_x_pos(cli_in_.x_pos());
    cli_out_.set_y_pos(cli_in_.y_pos());
    cli_out_.set_heading(cli_in_.heading());
    
    const map_conf_t *map = MapUtils::get_map_conf(cli_in_.mapid());
    if (!map->is_private) {
        std::set<const player_t*> players_in_same_line;
        g_map_user_mgr.get_players_in_the_same_map_line(player, players_in_same_line);
        FOREACH(players_in_same_line, it) {
            if (*it == player) {
                continue;
            }
            commonproto::map_player_data_t* map_player = cli_out_.add_players();
            DataProtoUtils::pack_map_player_info((player_t*)(*it), map_player);            
        }
    }

    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);

    return 0;
}

int EnterMapCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case home_cmd_enter_family_hall:
            return proc_pkg_from_serv_after_enter_family_hall(player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int EnterMapCmdProcessor::proc_pkg_from_serv_after_enter_family_hall(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(enter_family_hall_out_);

    const map_conf_t *map = MapUtils::get_map_conf(cli_in_.mapid());
    if (map == NULL) {
        RET_ERR(cli_err_map_not_exist); 
    }

    player->cur_map_id = cli_in_.mapid();
    player->family_hall_line_id = enter_family_hall_out_.line_id();
    player->map_x = cli_in_.x_pos();
    player->map_y = cli_in_.y_pos();
    if (player->map_x == 0) {
        player->map_x = map->init_pos[0].x;
        player->map_y = map->init_pos[0].y;
    }

    player->heading = cli_in_.heading();
    if (player->heading > 360) {
        player->heading = 0;
    }

    cli_out_.Clear();
    cli_out_.set_x_pos(cli_in_.x_pos());
    cli_out_.set_y_pos(cli_in_.y_pos());
    cli_out_.set_heading(cli_in_.heading());
    cli_out_.mutable_players()->CopyFrom(enter_family_hall_out_.players());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int LeaveMapCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    return MapUtils::leave_map(player);
}

int PlayerChangeStateCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;

    if (cli_in_.type() == PLAYER_TYPE) {
        player->temp_info.state_buf->assign(cli_in_.state_bytes());
#if 0   //record road_map
        if (player->userid == 6031662) {
            FILE *fp = fopen("/home/singku/dplan/road", "ab");
            if (fp) {
                string str;
                cli_in_.SerializeToString(&str);
                uint32_t size = str.size();
                fseek(fp, 0, SEEK_END);
                fwrite(&size, sizeof(uint32_t), 1, fp);
                fwrite(str.c_str(), 1, size, fp);
                fclose(fp);
            }
        }

#endif
    } else if (cli_in_.type() == PET_TYPE){
        Pet *pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm());
        if (!pet) {
            return send_err_to_player(player, player->cli_wait_cmd, cli_err_pet_not_exist);
        }
        pet->set_state(cli_in_.state_bytes());
    } else {
        //return 0;
    }

    MapUtils::sync_player_change_state(player, cli_in_);
	return 0;
}
