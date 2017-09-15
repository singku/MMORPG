#ifndef MAIL_TABLE_H
#define MAIL_TABLE_H
extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute10x10.h>
#include "proto/db/dbproto.mail.pb.h"
#include "proto/client/common.pb.h"

class MailTable : public CtableRoute
{
public:
	MailTable(mysql_interface* db);

	uint32_t new_mail(userid_t userid, uint32_t u_create_tm, const commonproto::mail_data_t &mail_data);

	uint32_t get_all_mails(userid_t userid, uint32_t u_create_tm, dbproto::sc_mail_get_all &db_out);

	uint32_t del_mail_by_ids(userid_t userid, uint32_t u_create_tm, const dbproto::cs_mail_del_by_ids &db_in);

	uint32_t get_mail_by_ids(userid_t userid, uint32_t u_create_tm,
            const dbproto::cs_mail_get_by_ids &db_in,
			dbproto::sc_mail_get_by_ids &db_out);

    uint32_t set_mail_status(userid_t userid, uint32_t u_create_tm, std::string mailid, uint32_t status);
};

extern MailTable* g_mail_table;

#endif
