#ifndef MAIL_PROCESSOR_H
#define MAIL_PROCESSOR_H 
#include "cmd_processor_interface.h"

#include "proto/client/pb0x05.pb.h"
#include "proto/db/dbproto.mail.pb.h"

class GetMailListCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);

private:

    onlineproto::cs_0x0501_get_mail_list cli_in_;
    onlineproto::sc_0x0501_get_mail_list cli_out_;
    dbproto::cs_mail_get_all db_in_;
    dbproto::sc_mail_get_all db_out_;
};

class DelMailListCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:

    onlineproto::cs_0x0502_del_mail_list cli_in_;
    onlineproto::sc_0x0502_del_mail_list cli_out_;
    dbproto::cs_mail_del_by_ids db_in_;
};

class ReadMailCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);

private:

    onlineproto::cs_0x0504_read_mail cli_in_;
    onlineproto::sc_0x0504_read_mail cli_out_;
    dbproto::cs_mail_set_status db_in_;
};

class GetMailAttachmentCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);

private:

    struct mail_attach_session_t
    {
        uint32_t mail_id;
    };

    onlineproto::cs_0x0503_get_mail_attachment cli_in_;
    onlineproto::sc_0x0503_get_mail_attachment cli_out_;

    dbproto::cs_mail_get_by_ids db_in_;
    dbproto::sc_mail_get_by_ids db_out_;
    dbproto::cs_mail_set_status db_set_status_;
};

class AddMailCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);

private:

    onlineproto::cs_0x0506_add_mail cli_in_;
    onlineproto::sc_0x0506_add_mail cli_out_;
    dbproto::cs_mail_new db_in_;
};
#endif
