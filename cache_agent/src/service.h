#ifndef __SERVICE_H__
#define __SERVICE_H__

#include "common.h"

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
    
    int send_buf(conn_info_t *conn, uint32_t userid, uint32_t u_create_tm, uint16_t cmd, uint32_t seq,
            const char* body, int bodylen);
    int send_msg(conn_info_t *conn, uint32_t userid, uint32_t u_create_tm, uint16_t cmd, uint32_t seq, 
            const google::protobuf::Message& message);
    int send_cmsg(conn_info_t *conn, uint32_t userid, uint32_t u_create_tm, uint16_t cmd, uint32_t seq,
            Cmessage *msg);
private:

    inline void set_fd(int fd) {
        fd_ = fd; 
    }

    int fd_;
    std::string service_name_;
    char send_buf_[10000];
};

extern Service *g_dbproxy;

#endif
