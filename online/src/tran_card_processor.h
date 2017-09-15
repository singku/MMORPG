#ifndef TRAN_CARD_PROCESSOR_H
#define	TRAN_CARD_PROCESSOR_H
#include "common.h"
#include "cmd_processor_interface.h"

class UptarLvCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0119_update_star_level	cli_in_;
	onlineproto::sc_0x0119_update_star_level	cli_out_;
};

class ChooseRoleCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0120_choose_role	cli_in_;
	onlineproto::sc_0x0120_choose_role	cli_out_;
};

#endif
