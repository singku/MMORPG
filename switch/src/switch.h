#ifndef __SWITCH_H__
#define __SWITCH_H__

#include "common.h"
#include "server.h"
#include "player.h"

enum {
    DONT_CLEAR_WAITING_CMD     = 0,
    CLEAR_WAITING_CMD           = 1,

    DONT_SKIP_SELF              = 0,
    SKIP_SELF                   = 1,

    MAX_CACHED_PKG             = 50,
};

//---------------------------------------------------------------------- 
// helper functions
//---------------------------------------------------------------------- 
void prepare_player_basic(const switchproto::sw_player_basic_info_t &msg, 
        player_basic_t &basic);
void prepare_server_basic(const switchproto::cs_register_server &msg, 
        fdsession_t *fdsess, server_basic_info_t &basic);
server_t *prepare_dup_reg(server_t *svr, fdsession_t *fdsess, 
        switchproto::cs_register_server &msg, int &mode);

//---------------------------------------------------------------------- 
// functions
//---------------------------------------------------------------------- 

int send_err_to_fdsess(fdsession_t *fdsess, uint32_t cmd, 
        int32_t err, uint32_t seq = 0, uint32_t svr_id = 0, 
        uint32_t uid = 0, uint32_t u_create_tm = 0);
int send_err_to_server(server_t *svr, uint32_t cmd, int32_t err, 
        uint32_t uid = 0, uint32_t u_create_tm = 0);
int send_err_to_act(fdsession_t *fdsess, uint32_t cmd,
		int32_t err, uint32_t seq = 0, uint32_t svr_id = 0,
		uint32_t uid = 0);
int send_msg_to_server(server_t *svr, uint32_t cmd, 
        Message &msg, bool clear_waiting_cmd = true);
int send_msg_to_act(server_t *svr, uint32_t cmd, const char* body,
		int bodylen, int ret = 0);

#endif // __SWITCH_H__
