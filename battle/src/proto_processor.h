#ifndef PROTO_PROCESSOR_H
#define PROTO_PROCESSOR_H

#include "common.h"

#include "player.h"
#include "cmd_processor_interface.h"


enum {
    PROTO_FROM_CLIENT = 0,
    PROTO_FROM_SERV = 1,
};


/**
 * @brief  ProtoProcessor 协议注册执行类
 */
class ProtoProcessor
{
public:
    ProtoProcessor();

    ~ProtoProcessor();

    int register_command(uint32_t cmd,
            CmdProcessorInterface* processor);

    int get_pkg_len(int fd, const void* avail_data, 
           int avail_len, int from);
    
    int proc_pkg_from_client(void* data, int len, fdsession_t* fdsess, bool from_queue);
    
    void proc_pkg_from_serv(int fd, void* data, int len);

    CmdProcessorInterface* get_processor(uint32_t cmd);
private:

    std::map<uint32_t, CmdProcessorInterface*> cmd_processors_;
};

#endif
