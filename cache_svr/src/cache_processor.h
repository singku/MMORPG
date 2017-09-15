#ifndef __CACHE_PROCESSOR_H__
#define __CACHE_PROCESSOR_H__

#include "common.h"
#include "cmd_processor_interface.h"

class CliGetPlayerCacheInfoCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(conn_info_t *conn, const char* body, int bodylen);
private:
    cacheproto::cs_batch_get_users_info in_;
};

class DbGetPlayerCacheInfoCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_serv(conn_info_t *conn, const char* body, int bodylen);
private:
    dbproto::sc_get_cache_info out_;
};

class SetCacheCmdProcessor: public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(conn_info_t *conn, const char *body, int bodylen);
private:
    cacheproto::cs_set_cache in_;
    cacheproto::sc_set_cache out_;
};

class GetCacheCmdProcessor: public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(conn_info_t *conn, const char *body, int bodylen);
private:
    cacheproto::cs_get_cache in_;
    cacheproto::sc_get_cache out_;
};

class SetUserInfoOutDateCmdProcessor: public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(conn_info_t *conn, const char *body, int bodylen);
private:
    cacheproto::sc_set_user_info_outdate out_;
};

#endif
