#ifndef __DUPLICATE_PROCESSOR_H__
#define __DUPLICATE_PROCESSOR_H__

#include "common.h"
#include "cmd_processor_interface.h"

class DuplicateRelayCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

public:
    int proc_duplicate_leave_map(player_t *player, const string &pkg);
    int proc_duplicate_battle_ready(player_t *player, const string &pkg);
    int proc_duplicate_exit(player_t *player, const string &pkg);
    int proc_duplicate_hit_character(player_t *player, const string &pkg);
    int proc_duplicate_to_next_phase(player_t *player, const string &pkg);
    int proc_duplicate_change_state(player_t *player, const string &pkg);
    int proc_duplicate_switch_fight_pet(player_t *player, const string &pkg);
    int proc_duplicate_front_mon_flush_req(player_t *player, const string &pkg);
    int proc_duplicate_skill_affect(player_t *player, const string &pkg);

private:
    battleproto::cs_battle_relay cli_in_;
    battleproto::sc_battle_relay cli_out_;
};

class EnterDuplicateCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    battleproto::cs_battle_duplicate_enter_map cli_in_;
    battleproto::sc_battle_duplicate_enter_map cli_out_;
};

class RevivalDuplicateCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    battleproto::cs_battle_duplicate_revival cli_in_;
    battleproto::sc_battle_duplicate_revival cli_out_;
};


class DuplicateTrigCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    battleproto::cs_battle_duplicate_trig cli_in_;
    battleproto::sc_battle_duplicate_trig cli_out_;
};
#endif
