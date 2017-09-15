#ifndef TASK_PROCESSOR_H
#define TASK_PROCESSOR_H

#include "cmd_processor_interface.h"

#include "proto/client/pb0x04.pb.h"
#include "proto/db/dbproto.task.pb.h"


class AcceptTaskCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);

private:

    onlineproto::cs_0x0401_accept_task cli_in_;
    onlineproto::sc_0x0401_accept_task cli_out_;
};

class AbandonTaskCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);

private:

    onlineproto::cs_0x0402_abandon_task cli_in_;
    onlineproto::sc_0x0402_abandon_task cli_out_;
};


class TaskCompleteCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);

private:

    onlineproto::cs_0x0403_complete_task cli_in_;
    onlineproto::sc_0x0403_complete_task cli_out_;
};

class CompleteEvilKnifeLegendCmdProcessor : public CmdProcessorInterface
{
    public:
        int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    private:
        onlineproto::cs_0x0405_complete_evil_knife_legend cli_in_;
        onlineproto::sc_0x0405_complete_evil_knife_legend cli_out_;
};


#endif
