
#ifndef CMD_PROCESSOR_INTERFACE_H
#define CMD_PROCESSOR_INTERFACE_H

#include "common.h"

class CmdProcessorInterface
{
public:
    virtual ~CmdProcessorInterface() { }

public:

    /**
     * @brief  proc_pkg_from_client 响应client请求包
     *
     * @param player 客户端缓存
     * @param body 请求包体
     * @param bodylen 请求包体长度
     *
     * @return 0 处理成功 -1 处理失败
     */
    virtual int proc_pkg_from_client(
            player_t* player,
            const char* body,
            int bodylen) {
        return 0;
    }

    /**
     * @brief  proc_pkg_from_serv 响应dbserver返回包
     *
     * @param player 客户端缓存
     * @param body 返回包体
     * @param bodylen 返回包体长度
     *
     * @return 0 处理成功 -1 处理失败
     */
    virtual int proc_pkg_from_serv(
            player_t* player,
            const char* body,
            int bodylen) {
        return 0;
    }

    /**
     * @brief  proc_errno_from_serv 处理dbserver返回的错误码
     *
     * @param client 客户端缓存
     * @param ret dbserver返回的错误码
     *
     * @return 返回online的错误码
     */
    virtual uint32_t proc_errno_from_serv(
            player_t* player, uint32_t ret) {
        return cli_err_sys_err; 
    }
};

inline int parse_message(
        const char* body, int bodylen, 
        google::protobuf::Message* message)
{
    message->Clear();

    std::string name = message->GetTypeName();
    if (!message->ParseFromArray(body, bodylen)) {
        std::string errstr = message->InitializationErrorString();
        ERROR_TLOG("PARSE MSG '%s' failed, err = '%s'", 
                name.c_str(), errstr.c_str());
        return -1; 
    }

    std::string debug_str = message->Utf8DebugString();
    if (name != "onlineproto.cs_0x0106_player_change_state") {
        TRACE_TLOG("PARSE MSG:'%s' ok\nMSG:\n[%s]", 
                name.c_str(), debug_str.c_str());
    }

    return 0;
}

inline int parse_cmessage(
        const char* body, int bodylen,
        Cmessage* cmessage)
{
    cmessage->init();

    byte_array_t in_ba (body, bodylen);
    //失败
    if (!cmessage->read_from_buf(in_ba)) {
        ERROR_TLOG("还原对象失败");
        return -1;	
    }

    //客户端多上传报文
    if (!in_ba.is_end()) {
        ERROR_TLOG("过多报文");
        return -1;
    }

    return 0;
}

#define PARSE_MSG \
    do { \
        cli_in_.Clear();\
        if (parse_message(body, bodylen, &cli_in_)) {\
            return send_err_to_player(player, \
                    player->wait_cmd, cli_err_proto_format_err);\
        }\
    } while (0)

#endif
