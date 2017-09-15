
#ifndef MAP_PROCESSOR_H
#define MAP_PROCESSOR_H


#include "cmd_processor_interface.h"


class EnterMapCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
    int proc_pkg_from_serv_after_enter_family_hall(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0102_enter_map cli_in_;
    onlineproto::sc_0x0102_enter_map cli_out_;
    homeproto::cs_enter_family_hall enter_family_hall_in_;
    homeproto::sc_enter_family_hall enter_family_hall_out_;
};

class LeaveMapCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
    onlineproto::cs_0x0104_leave_map cli_in_;
    onlineproto::sc_0x0104_leave_map cli_out_;
};


class PlayerChangeStateCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
    onlineproto::cs_0x0106_player_change_state cli_in_;
    onlineproto::sc_0x0106_player_change_state cli_out_;
};

#endif
