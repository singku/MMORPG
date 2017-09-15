
#ifndef CMD_PROCESSOR_INTERFACE_H
#define CMD_PROCESSOR_INTERFACE_H

extern "C" {
#include <libtaomee/log.h>    
}
#include <libtaomee++/proto/proto_base.h>
#include <google/protobuf/message.h>
#include <libtaomee/project/types.h>
#include "client.h"

struct client_info_t;

class CmdProcessorInterface
{
public:

    virtual ~CmdProcessorInterface() {}

    /**
     * @brief  proc_pkg_from_client 响应client请求包
     *
     * @param client 客户端缓存
     * @param body 请求包体
     * @param bodylen 请求包体长度
     *
     * @return 0 处理成功 -1 处理失败
     */
    virtual int proc_pkg_from_client(
            client_info_t* client,
            const char* body,
            int bodylen) = 0;

    /**
     * @brief  proc_pkg_from_serv 响应dbserver返回包
     *
     * @param client 客户端缓存
     * @param ack_uid 返回的userid
     * @param body 返回包体
     * @param bodylen 返回包体长度
     *
     * @return 0 处理成功 -1 处理失败
     */
    virtual int proc_pkg_from_serv(
            client_info_t* client,
            userid_t ack_uid,
            const char* body,
            int bodylen) {
        return 0;
    };

    /**
     * @brief  proc_errno_from_serv 处理dbserver返回的错误码
     *
     * @param client 客户端缓存
     * @param ret dbserver返回的错误码
     *
     * @return 0 处理成功 -1 处理失败
     */
    virtual int proc_errno_from_serv(
            client_info_t* client,
            int ret) {
        return send_err_to_client(client, ret);
    };
};

inline int parse_message(
        const char* body, int bodylen, 
        google::protobuf::Message* message)
{
    message->Clear();

    std::string name = message->GetTypeName();
    if (!message->ParseFromArray(body, bodylen)) {
        std::string errstr = message->InitializationErrorString();
        ERROR_LOG("PARSE MSG '%s' failed, err = '%s'", 
                name.c_str(), errstr.c_str());
        return -1; 
    }

    std::string debug_str = message->Utf8DebugString();
    TRACE_TLOG("PARSE MSG:'%s' ok\nMSG:\n[%s]", 
            name.c_str(), debug_str.c_str());

    return 0;
}

inline int parse_cmessage(
        const char* body, int bodylen,
        Cmessage* cmessage)
{
    //还原对象
    cmessage->init();
    byte_array_t in_ba (body, bodylen);
    //失败
    if (!cmessage->read_from_buf(in_ba)) {
        DEBUG_LOG("sw 还原对象失败");
        return -1;	
    }

    //客户端多上传报文
    if (!in_ba.is_end()) {
        DEBUG_LOG("sw re 过多报文");
        return -1;
    }

    return 0;
}

#endif
