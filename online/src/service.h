
#ifndef SERVICE_H
#define SERVICE_H

#include <libtaomee++/proto/proto_base.h>
#include <libtaomee/project/types.h>
#include <google/protobuf/message.h>

struct player_t;

class Service
{
public:  
    Service(const std::string& service_name);
    //connect by ip-port
    Service(const std::string& service_name, const std::string& ipaddr, in_addr_t port);
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

    int send_to_act(player_t* player, userid_t userid, uint16_t cmd, 
            const char* body, int bodylen, uint32_t ret = 0);   
    int send_buf(player_t* player, userid_t userid, uint32_t u_create_tm, uint16_t cmd, 
            const char* body, int bodylen, uint32_t ret = 0);
    int send_msg(player_t* player, userid_t userid, uint32_t u_create_tm, uint16_t cmd, 
            const google::protobuf::Message& message, uint32_t ret = 0);
    int send_cmsg(player_t* p, userid_t userid, uint32_t u_create_tm, uint16_t cmd, 
            Cmessage *msg, uint32_t ret = 0);
private:

    inline void set_fd(int fd) 
    {
        fd_ = fd; 
    }

    int fd_;
    std::string service_name_;
    std::string ipaddr_;
    in_addr_t port_; 
    bool is_connect_by_name_;
    char send_buf_[65536];
};

#endif
