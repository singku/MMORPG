#ifndef __PROTO_PROCESSOR_H__
#define __PROTO_PROCESSOR_H__

#include "common.h"
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
    
    int proc_pkg_from_client(void* data, int len, fdsession_t* fdsess);
    
    void proc_pkg_from_serv(int fd, void* data, int len);

private:

    std::map<uint32_t, CmdProcessorInterface*> cmd_processors_;
};

extern ProtoProcessor* g_proto_processor;
extern uint32_t max_incoming_packets_len;

#endif
