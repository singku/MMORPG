#ifndef TRANS_PROF_PROCESSOR_H
#define TRANS_PROF_PROCESSOR_H
#include "cmd_processor_interface.h"
#include "proto/client/pb0x01.pb.h"

class TransProfCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
	onlineproto::cs_0x0139_trans_prof cli_in_;
	onlineproto::sc_0x0139_trans_prof cli_out_;
};

#endif

