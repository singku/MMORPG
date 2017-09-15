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
    
    int send_buf(server_t* svr, uint32_t userid, uint32_t u_create_tm, uint16_t cmd, 
            const char* body, int bodylen, bool wait_ret = WAIT_SRV_BACK);
    int send_msg(server_t* svr, uint32_t userid, uint32_t u_create_tm, uint16_t cmd, 
            const google::protobuf::Message& message, bool wait_ret = WAIT_SRV_BACK);
	
private:

    inline void set_fd(int fd) {
        fd_ = fd; 
    }

    int fd_;
    std::string service_name_;
    char send_buf_[1000000];
};

extern Service *g_dbproxy;

#endif
