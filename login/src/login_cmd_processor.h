
#ifndef LOGIN_CMD_PROCESSOR_H
#define LOGIN_CMD_PROCESSOR_H

#include "cmd_processor_interface.h"
#include "proto/client/common.pb.h"
#include "proto/common/svr_common.pb.h"
#include "proto/client/pb0x00.pb.h"
#include "proto/db/dbproto.base_info.pb.h"
#include "proto/db/dbproto.nick.pb.h"
#include "proto/db/dbproto.attr.pb.h"
#include "proto.h"
#include "proto/db/db_cmd.h"
#include "proto/db/db_errno.h"
#include "proto/switch/switch_cmd.h"
#include "proto/switch/switch_errno.h"
#include "proto/switch/switch.pb.h"
#include "proto/client/cli_cmd.h"
#include "proto/client/cli_errno.h"

#include "cmd_processor_interface.h"

/*登录流程*/
class LoginCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(client_info_t* client, 
            const char* body, int bodylen);
    int proc_pkg_from_serv(client_info_t* client, 
            userid_t ack_uid, const char* body, int bodylen);
    int proc_errno_from_serv(client_info_t* client, int ret);

private:
    struct login_session_t {
        bool is_active;
        userid_t userid;
        int login_buflen;
        char login_buf[8000]; 
    };

    struct login_session_check_pwd_t {
        char tad[128]; 
        act_login_with_verify_code_req_t req_body;
    };

    int proc_pkg_from_serv_aft_check_passwd(client_info_t* client, 
            userid_t ack_uid, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_userid(client_info_t* client, 
            const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_is_active( client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_base_info(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);

    int response_to_client(client_info_t* client, 
            google::protobuf::RepeatedPtrField<commonproto::player_base_info_t> *base_info);

    onlineproto::cs_0x0001_login cli_in_;
    onlineproto::sc_0x0001_login cli_out_;
    dbproto::cs_check_user_exist db_cue_in_;
    dbproto::sc_check_user_exist db_cue_out_;
    dbproto::cs_get_base_info db_gbi_in_;
    dbproto::sc_get_base_info db_gbi_out_;
    dbproto::cs_get_user_by_nick db_gubn_in_;
    dbproto::sc_get_user_by_nick db_gubn_out_;
};

/*统计*/
class StatCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(client_info_t* client,
            const char* body, int bodylen);

private:
    onlineproto::cs_0x0009_cli_stat cli_in_;
};

/*角色创建*/
class CreateRoleCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(client_info_t* client,
            const char* body, int bodylen);
    int proc_pkg_from_serv(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_errno_from_serv(client_info_t* client, int ret);

private:
    int proc_pkg_from_serv_aft_check_session(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_create_role(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_insert_nick(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_add_gameflag(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_check_exist(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);

    onlineproto::cs_0x0003_create_role cli_in_;
    onlineproto::sc_0x0003_create_role cli_out_;
    char m_send_buf[4096];

    struct create_role_session_t {
        char nick[64];
        int server_id; 
        int sex;
        int prof;
        char tad[256];
    };  

    dbproto::cs_create_role db_cr_in_;
    dbproto::sc_create_role db_cr_out_;
    dbproto::cs_insert_nick_and_user db_inau_in_;
    dbproto::cs_check_user_exist db_cue_in_;
    dbproto::sc_check_user_exist db_cue_out_;
};

/*随机昵称*/
class RandomNickCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(client_info_t* client,
            const char* body, int bodylen);
    int proc_pkg_from_serv(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    std::string get_random_nick(uint32_t sex);

private:
    struct random_nick_session_t{
        uint32_t sex; 
        char nick[64];
    };

    onlineproto::cs_0x0002_require_random_nick cli_in_;
    onlineproto::sc_0x0002_require_random_nick cli_out_;
    dbproto::cs_get_user_by_nick db_in_;
    dbproto::sc_get_user_by_nick db_out_;
};

/*获取服务器列表*/
class GetSvrListCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(client_info_t* client,
            const char* body, int bodylen);
    int proc_pkg_from_serv(client_info_t* client, 
            userid_t ack_uid, const char* body, int bodylen);
    int proc_errno_from_serv(client_info_t* client, int ret);

private:
    int proc_pkg_from_serv_aft_check_session(client_info_t* client,
            userid_t ack_uid, const char* body,int bodylen);
    int proc_pkg_from_serv_aft_get_pre_online(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_server_list(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);

    onlineproto::cs_0x0004_get_svr_list cli_in_;
    onlineproto::sc_0x0004_get_svr_list cli_out_;
    dbproto::cs_get_attr db_in_;
    dbproto::sc_get_attr db_out_;
    char m_send_buf[4096];
};

/*激活码激活账户*/
class ActiveUserCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(client_info_t* client,
            const char* body,int bodylen);
    int proc_pkg_from_serv(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_errno_from_serv(client_info_t* client, int ret);

private:
    onlineproto::cs_0x0007_active_user cli_in_;
    onlineproto::sc_0x0007_active_user cli_out_;
    char send_buf_[4096];
};

/*获取验证码图片*/
class GetVerifyImageCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(client_info_t* client,
            const char* body, int bodylen);
    int proc_pkg_from_serv(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_errno_from_serv(client_info_t* client, int ret);

private:
    onlineproto::sc_0x0005_get_verify_image cli_out_;
};

/*session登录*/
class SessionLoginCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(client_info_t* client,
            const char* body, int bodylen);
    int proc_pkg_from_serv(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_errno_from_serv(client_info_t* client, int ret);

private:
    int proc_pkg_from_serv_aft_check_session(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_add_session(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int proc_pkg_from_serv_aft_get_base_info(client_info_t* client,
            userid_t ack_uid, const char* body, int bodylen);
    int response_to_client(client_info_t* client, 
            google::protobuf::RepeatedPtrField<commonproto::player_base_info_t> *base_info);

    struct session_login_session_t {
        bool is_active; 
        char session[16];
        uint32_t from_gameid;
    };

    onlineproto::cs_0x0006_session_login cli_in_;
    onlineproto::sc_0x0006_session_login cli_out_;
    dbproto::cs_get_base_info db_in_;
    dbproto::sc_get_base_info db_out_;
    char send_buf_[4096];
};

class ChooseServerCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(client_info_t *client, const char *body, int bodylen);
    int proc_pkg_from_serv(client_info_t *client, userid_t ack_uid, const char *body, int bodylen);
private:
    onlineproto::cs_0x000B_cli_choose_server cli_in_;
    onlineproto::sc_0x000B_cli_choose_server cli_out_;
    dbproto::cs_get_base_info db_gbi_in_;
    dbproto::sc_get_base_info db_gbi_out_;
};
#endif
