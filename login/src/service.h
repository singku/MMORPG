
#ifndef SERVICE_H
#define SERVICE_H

#include <libtaomee++/proto/proto_base.h>
#include <libtaomee/project/types.h>
#include <google/protobuf/message.h>

struct client_info_t;

class Service
{
public:  
    Service(const std::string& service_name);
    ~Service();

    inline const std::string& service_name() const
    {
        return service_name_; 
    }

    inline int fd() const
    {
        return fd_;
    }

    int connect();

    int close();

    static void on_connect_callback(int fd, void* args);

    int send_to_act(client_info_t* client, userid_t userid, uint16_t cmd, 
            const char* body, int bodylen, uint32_t ret = 0);   
    int send_buf(client_info_t* client, userid_t userid, uint32_t u_create_tm, uint16_t cmd, 
            const char* body, int bodylen, uint32_t ret = 0);
    int send_msg(client_info_t* client, userid_t userid, uint32_t u_create_tm, uint16_t cmd, 
            const google::protobuf::Message& message, uint32_t ret = 0);
    int send_cmsg(client_info_t* client, userid_t userid, uint32_t u_create_tm, uint16_t cmd, 
            Cmessage *msg, uint32_t = 0);
private:

    inline void set_fd(int fd) 
    {
        fd_ = fd; 
    }

    int fd_;
    std::string service_name_;
    char send_buf_[10000];
};

#endif
