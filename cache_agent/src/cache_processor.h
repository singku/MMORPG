#ifndef __CACHE_PROCESSOR_H__
#define __CACHE_PROCESSOR_H__

#include "common.h"
#include "cmd_processor_interface.h"

class GetPlayerBaseInfoCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(conn_info_t *conn, const char* body, int bodylen);
    int proc_pkg_from_serv(conn_info_t *conn, const char* body, int bodylen, svr_proto_header_t *header) {
        return 0;
    }
private:
    cacheproto::cs_batch_get_users_info in_;
};

class GetPlayerBaseCacheCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(conn_info_t *conn, const char* body, int bodylen) {
        return 0;
    }
    int proc_pkg_from_serv(conn_info_t *conn, const char* body, int bodylen, svr_proto_header_t *header);
private:
    cacheproto::sc_batch_get_users_info svr_out_;   //cache_svr的回包
};

#endif
