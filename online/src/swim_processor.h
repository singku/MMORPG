#ifndef __SWIM_H__
#define __SWIM_H__
#include "cmd_processor_interface.h"
#include "proto/client/pb0x08.pb.h"


#define SWIM_GET_EXP_INTERVAL (5)	//游泳获得经验的时间间隔
#define SWIM_GET_EXP_FREQ_ALLOW (3)	//游泳获得经验的容差次数

class StartSwimCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0821_start_swim cli_in_;
	onlineproto::sc_0x0821_start_swim cli_out_;
};

class SwimGetExpCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0822_get_exp cli_in_;
	onlineproto::sc_0x0822_get_exp cli_out_;
};

class SwimPauseCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0823_swim_pause cli_in_;
	onlineproto::sc_0x0823_swim_pause cli_out_;
};

class UseChairCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0827_use_chair cli_in_;
	onlineproto::sc_0x0827_use_chair cli_out_;
};

class WithdrawChairCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0828_withdraw_chair cli_in_;
	onlineproto::sc_0x0828_withdraw_chair cli_out_;
};

class ApplyDiveCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x082A_apply_dive cli_in_;
	onlineproto::sc_0x082A_apply_dive cli_out_;
};

class PrepareDiveCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x082B_prepare_dive cli_in_;
	onlineproto::sc_0x082B_prepare_dive cli_out_;
};

class StartDiveCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x082C_start_dive cli_in_;
	onlineproto::sc_0x082C_start_dive cli_out_;
};

class InformDiveRankCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x082D_inform_dive_rank cli_in_;
	onlineproto::sc_0x082D_inform_dive_rank cli_out_;
};
void notify_start_swim_to_map(player_t* player);
uint32_t get_swim_left_time(player_t* player);
void notify_pause_swim_to_map(player_t* player);

#endif
