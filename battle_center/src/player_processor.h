#ifndef __PLAYER_PROCESSOR_H__
#define __PLAYER_PROCESSOR_H__

#include "cmd_processor_interface.h"

class BtlEnterCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    battleproto::cs_battle_duplicate_enter_map btl_in_;
};

class BtlQuitMatchCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    battleproto::cs_battle_give_up_match btl_in_;
};

#endif
