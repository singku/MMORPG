
#ifndef PROTO_PROCESSOR_H
#define PROTO_PROCESSOR_H

#include <stdint.h>
#include <map>

#include "cmd_processor_interface.h"

/**
 * @brief  ProtoProcessor 协议注册执行类
 */
class ProtoProcessor
{
public:
    ProtoProcessor();

    ~ProtoProcessor();

    int register_cmd(
            uint32_t cmd,
            CmdProcessorInterface* processor);

    int get_pkg_len(
            const void* avail_data, 
            int avail_len); 

    int process(
            const char* recv_buf, 
            int recv_len,
            char** send_buf,
            int* send_len);

private:
    std::map<uint32_t, CmdProcessorInterface*> m_cmd_processors;
    char m_send_buf[2097152];
    std::string m_req_body;
    std::string m_ack_body;
};

#endif
