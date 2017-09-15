
#ifndef PROTO_H
#define PROTO_H

#include <libtaomee++/inet/pdumanip.hpp>
#include "proto/common/svr_proto_header.h"

#define SESSION_LEN (16)

extern bool cli_proto_encrypt;
/**
 * @brief  cli_proto_header_t 客户端请求的包头
 */
struct cli_proto_header_t
{
    uint32_t len;
    uint16_t head_len;
    uint16_t cmd;
    uint32_t check_sum;
} __attribute__((packed));

/**
 * @brief  svr_proto_header_t 请求后端系统的包头
struct svr_proto_header_t
{
    uint32_t len;
    uint32_t seq;
    uint16_t cmd;
    uint32_t ret;
    uint32_t uid;
} __attribute__((packed));

struct act_proto_header_t
{
    uint32_t len;
    uint32_t seq;
    uint16_t cmd;
    uint32_t ret;
    uint32_t uid;
} __attribute__((packed));
*/

/*账号平台命令号*/
enum 
{
    act_cmd_login_with_verify_code = 0xA039,
    act_cmd_check_session = 0xA03A,
    act_cmd_add_gameflag = 0xA13B,
    act_cmd_is_active = 0xA029,
    act_cmd_active_user = 0xA128,
    act_cmd_get_verify_image = 0xA031,
    act_cmd_add_session = 0xA122,

    act_err_active_code_err = 3001,
    act_err_check_session_failed = 4331,
};

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

struct act_login_succ_ack_t
{
    char session[16];
    uint32_t gameflag; 
} __attribute__((packed));

struct act_login_need_verify_image_ack_t
{
    char verify_image_session[16];
    uint32_t image_size;
    char image[0]; 
} __attribute__((packed));

struct act_login_in_diff_city_ack_t
{
    char session[16];
    uint32_t gameflag;
    uint32_t last_login_ip;
    uint32_t last_login_time;
    char last_login_city[64];
    char current_login_city[64];
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

struct act_add_gameflag_req_t
{
    uint32_t idc_zone; 
    uint32_t gameid;
    uint16_t channel;
    char tad[128];
} __attribute__((packed));

struct act_is_active_req_t
{
    uint8_t gameid;
} __attribute__((packed));

struct act_is_active_ack_t
{
    uint8_t is_active;
} __attribute__((packed));

struct act_active_user_req_t
{
    uint8_t gameid;  
    char active_code[10];
    uint32_t ip;
    char verify_session[16];
    char verify_code[6];
} __attribute__((packed));

#define ACTIVE_ACK_FLAG_OK 0 // 登录成功
#define ACTIVE_ACK_FLAG_ACCOUNT_UNSAFE 1 // 账号存在异常，需要输入验证码
#define ACTIVE_ACK_FLAG_WRONG_VERIFY_CODE 4 // 验证码错误
#define ACTIVE_ACK_FLAG_WRONG_ACTIVE_CODE_TOO_MUCH 5 // 密码尝试次数过多

struct act_active_user_ack_t
{
    uint32_t flag;
} __attribute__((packed));

struct act_active_need_verify_image_ack_t
{
    char verify_image_session[16];
    uint32_t image_size;
    char image[0]; 
} __attribute__((packed));

struct act_get_verify_image_req_t
{
    uint16_t verify_id; 
    uint32_t ip;
    char verify_session[16];
} __attribute__((packed));

struct act_get_verify_image_ack_t
{
    uint32_t flag; 
    char verify_image_session[16];
    uint32_t image_size;
    char image[0];
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


#endif
