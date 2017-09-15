
#ifndef PROTO_H
#define PROTO_H

#include "proto/common/svr_proto_header.h"

extern bool cli_proto_encrypt;

struct cli_proto_header_t
{
    uint32_t total_len;
    uint16_t head_len;
    uint16_t cmd;
    uint32_t check_sum;
} __attribute__((packed));

/*
struct act_proto_header_t
{
    uint32_t len;   
    uint32_t seq;
    uint16_t cmd;
    uint32_t ret;
    uint32_t uid;
} __attribute__((packed));
*/

//账号平台account命令集合
enum {
    act_cmd_login_with_verify_code = 0xA039,
    act_cmd_check_session = 0xA03A,
    act_cmd_is_active = 0xA029,
    act_cmd_add_session = 0xA122,
    act_cmd_user_logout = 0xA125,
    act_err_check_session_failed = 4431,
    act_cmd_get_boss_info = 0x1A01,
    act_cmd_get_gameflag = 0x0007,
};

struct act_is_active_req_t
{
    uint8_t gameid;
} __attribute__((packed));

struct act_is_active_ack_t
{
    uint8_t is_active;
} __attribute__((packed));

struct act_add_session_req_t
{
    uint32_t gameid; 
    uint32_t ip;
} __attribute__((packed));

struct act_add_session_ack_t
{
    char session[16];
} __attribute__((packed));

struct act_check_session_req_t
{
    uint32_t from_game;
    char session[16];
    uint32_t del_flag;
    uint32_t to_game;
    uint32_t ip;
    uint16_t region;
    uint8_t enter_game;
    char tad[128];
} __attribute__((packed));

struct act_user_logout_req_t
{
    uint32_t gameid; 
    uint32_t login_time;
    uint32_t logout_time;
} __attribute__((packed));

struct act_login_with_verify_code_req_t
{
    char email[64];
    char passwd_md5_two[16];
    uint16_t verifyid;
    uint16_t region;
    uint16_t gameid;
    uint32_t ip;
    char verify_session[16];
    char verify_code[6];
    uint16_t login_channel;
    char login_promot_tag[128];
} __attribute__((packed));

#define ACT_LOGIN_ACK_FLAG_SUCC 0
#define ACT_LOGIN_ACK_FLAG_WRONG_PASSWD_TOO_MUCH 1
#define ACT_LOGIN_ACK_FLAG_LOGIN_IN_DIFF_CITY 2
#define ACT_LOGIN_ACK_FLAG_LOGIN_IN_DIFF_CITY_TOO_MUCH 3
#define ACT_LOGIN_ACK_FLAG_WRONG_VERIFY_CODE 4
#define ACT_LOGIN_ACK_FLAG_ACCOUNT_UNSAFE 5

struct act_login_with_verify_code_ack_t
{
    uint32_t flag; 
} __attribute__((packed));

struct act_get_boss_info_req_t
{
    uint8_t gameid; 
    uint32_t query_count;
    uint32_t start_time;
    uint32_t end_time;
} __attribute__((packed));

struct act_get_boss_info_ack_t
{
    uint32_t query_count; 
    uint32_t consume_num;
} __attribute__((packed));


struct act_get_gameflag_ack_t
{
    uint32_t gameflag;
} __attribute__((packed));

#endif
