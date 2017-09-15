
#ifndef PROTO_PROCESSOR_H
#define PROTO_PROCESSOR_H

#include <stdint.h>
#include <map>

#include "cmd_processor_interface.h"

enum {
    PROTO_FROM_CLIENT = 0,
    PROTO_FROM_SERV = 1,
};

extern std::map<int, client_info_t*> g_pending_proto_clients; 

/**
 * @brief  ProtoProcessor 协议注册执行类
 */
class ProtoProcessor
{
public:
    ProtoProcessor();
    ~ProtoProcessor();

    int register_cmd(uint32_t cmd, CmdProcessorInterface* processor);
    int get_pkg_len(int fd, const void* avail_data, int avail_len, int from = PROTO_FROM_CLIENT);
    int proc_pkg_from_client(void* data, int len, fdsession_t* fdsess, bool from_queue=false); 
    void proc_pkg_from_serv(int fd, void* data, int len);

private:
    std::map<uint32_t, CmdProcessorInterface*> m_cmd_processors;
};

#endif
