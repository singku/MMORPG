#ifndef __CMD_MSG_H__
#define __CMD_MSG_H__

#include <stdint.h>

struct cmd_msg_t
{
    char cs_msg[256];
    char sc_msg[256];
    uint32_t cmd;
};

extern cmd_msg_t cmd_msg_arr[];
extern uint32_t cmd_msg_arr_size;

#endif
