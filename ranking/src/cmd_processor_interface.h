
#ifndef CMD_PROCESSOR_INTERFACE_H
#define CMD_PROCESSOR_INTERFACE_H

#include <google/protobuf/message.h>
#include <libtaomee/project/types.h>

class CmdProcessorInterface {
public:
    virtual ~CmdProcessorInterface() { }

    virtual uint32_t process(
            userid_t userid,
            uint32_t u_create_tm,
            const std::string& req_body,
            std::string& ack_body) = 0;
};

#endif
