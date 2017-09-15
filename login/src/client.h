
#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

extern "C" {
#include <libtaomee/list.h>
#include <libtaomee/project/types.h>
#include <libtaomee/timer.h>
}
#include <async_serv/service.h>
#include "proto_queue.h"

#include <map>
using namespace std;

struct client_info_t
{
    userid_t userid;
    bool has_uid;
    int fd;
    uint32_t cli_seq;
    uint32_t pkg_idx;
    uint32_t cli_cmd;
    fdsession_t* fdsession;
    uint32_t wait_serv_cmd;
    uint32_t cur_serv_cmd;
    timer_struct_t* db_request_timer;
    list_head_t timer_list;
    char session[4096 * 3];        
    ProtoQueue* proto_queue;
    uint32_t on_server_id;
    uint32_t on_server_create_tm;
    //svr,create_tm
    std::map<uint32_t, uint32_t> *svr_create_tm_map;
    char login_session[33];
    char nick[64];//缓存用户传过来的昵称
    bool has_nick;
    char email[64];//缓存用户传过来的email
    bool has_email;
    string *cache_msg;
};

client_info_t* get_client(int fd);
client_info_t* get_or_alloc_client(int fd);
void destroy_client(int fd);

int send_to_client(client_info_t* client, uint32_t cmd, 
        const char* body, int bodylen, int32_t ret = 0, uint32_t seq = 0);
int send_msg_to_client(client_info_t* client, uint32_t cmd,
        const google::protobuf::Message& message);
int send_to_fdsess(fdsession_t *fdsess, uint32_t cmd, 
        const char *body, int bodylen, uint32_t uid,
        uint32_t seq, int32_t ret = 0);
int send_err_to_client(client_info_t *client, int32_t err, uint32_t seq = 0);
int send_err_to_fdsess(fdsession_t *fdsess, uint32_t cmd, int32_t err, uint32_t seq = 0);

/**
 * @brief  write_msglog_new 新统计平台人数/人次
 *
 * @param userid 米米号
 * @param dir 统计平台的分类目录
 * @param name 统计项分类(活动名称)
 * @param subname 子统计项(活动中的统计项名称)
 */
void write_msglog_new(userid_t userid, uint32_t svr_id,
        const std::string& dir, const std::string& name, const std::string& subname, uint32_t count = 1);
void write_msglog_new(const std::string &user, uint32_t svr_id,
        const std::string& dir, const std::string& name, const std::string& subname, uint32_t count = 1);
void write_msglog_new(const char *user, uint32_t svr_id,
        const std::string& dir, const std::string& name, const std::string& subname, uint32_t count = 1);

std::string to_string(uint32_t n);

#endif
