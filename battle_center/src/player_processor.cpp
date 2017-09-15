#include "player_processor.h"
#include "player_manager.h"
#include "server_manager.h"

enum btl_type_t {
    DUP_BTL_TYPE_PPVE = 2,  //多人打野
    DUP_BTL_TYPE_RPVP = 5,  //实时PVP竞技场
};

int BtlEnterCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    if (parse_message(body, bodylen, &btl_in_)) {
        return player->send_err_to_player(btl_cmd_enter_duplicate, 
                cli_err_proto_format_err);
    }
    //保存对战信息
    btl_in_.player().SerializeToString(&(player->btl_info));
    //连接他对应的服务器
    string btl_name = btl_in_.btl_name();
    server_t *svr = SERVER_MGR.get_server_by_name(btl_name);
    if (!svr) {
        svr = SERVER_MGR.create_new_server(btl_name);
    }
    player->request_tm = NOW();
    player->score = btl_in_.player().rpvp_score();
    player->dup_id = btl_in_.dup_id();
    player->map_id = btl_in_.map_id();
    player->svr_id = btl_in_.svr_id();

    if (player->btl_start == false) {//新进战斗
        player->btl_name = btl_name;
        player->btl = svr;
        player->need_team_members = btl_in_.least_players();

        DEBUG_TLOG("create_new_player uid=%u add=%p idx=%u dup_id=%u map_id=%u svr_id=%u btl=%p score=%u", 
                player->uid, player, player->index, player->dup_id, player->map_id, player->svr_id,
                player->btl, player->score);
    }


    switch (btl_in_.btl_type()) {
    case DUP_BTL_TYPE_PPVE:
        PLAYER_MGR.add_player_to_ppve_match(player, player->svr_id, player->dup_id);
        break;
    case DUP_BTL_TYPE_RPVP:
        PLAYER_MGR.add_player_to_rpvp_match(player);
        break;
    default:
        return player->send_err_to_player(btl_cmd_enter_duplicate,
                cli_err_duplicate_btl_mode_err);        
    }
    return 0;
}

int BtlQuitMatchCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    if (parse_message(body, bodylen, &btl_in_)) {
        return player->send_err_to_player(btl_cmd_enter_duplicate, 
                cli_err_proto_format_err);
    }
    if (player->btl_start == true) {
        return player->send_err_to_player(btl_cmd_enter_duplicate,
                cli_err_battle_start);
    }
    //删除player
    PLAYER_MGR.destroy_player(player);
    return 0;
}

