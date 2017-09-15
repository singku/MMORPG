#include <ctime>
#include "mail_table.h"
#include "utils.h"
#include "sql_utils.h"
#include "proto/db/db_errno.h"
#include "attr_table.h"

MailTable::MailTable(mysql_interface* db)
	: CtableRoute(db, "dplan_db", "mail_table", "userid")
{

}

uint32_t MailTable::new_mail(userid_t userid, uint32_t u_create_tm, const commonproto::mail_data_t &mail_data)
{
    std::string safe_mail_id = this->get_mysql_str(const_cast<std::string &>(mail_data.mailid()));
    std::string safe_mail_sender = this->get_mysql_str(const_cast<std::string &>(mail_data.sender()));
    std::string safe_mail_title = this->get_mysql_str(const_cast<std::string &>(mail_data.title()));
    std::string safe_mail_content = this->get_mysql_str(const_cast<std::string &>(mail_data.content()));
    std::string safe_mail_attach = this->get_mysql_str(const_cast<std::string &>(mail_data.attachment()));

    GEN_SQLSTR(this->sqlstr, 
            "INSERT INTO %s (userid, u_create_tm, mailid, status, send_time, sender, title, content, attachment) "
            "VALUES (%u, %u, '%s', %u, %u, '%s', '%s', '%s', '%s')",
            this->get_table_name(userid), userid, u_create_tm, safe_mail_id.c_str(), (uint32_t)(mail_data.status()),
            mail_data.send_time(), safe_mail_sender.c_str(), safe_mail_title.c_str(),
            safe_mail_content.c_str(), safe_mail_attach.c_str());

	return this->exec_insert_sql(this->sqlstr, KEY_EXISTED_ERR);
}


uint32_t MailTable::get_all_mails(userid_t userid, uint32_t u_create_tm, dbproto::sc_mail_get_all &db_out)
{
	GEN_SQLSTR(this->sqlstr,
			"SELECT mailid, status, send_time, sender, title, content, attachment "
			"FROM %s "
			"WHERE userid = %u AND u_create_tm = %u limit 1000",
			this->get_table_name(userid), userid, u_create_tm);

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, (&db_out), mail_datas)
        PB_BIN_CPY_NEXT_FIELD(item, mailid);
        uint32_t status;
        INT_CPY_NEXT_FIELD(status);
        item.set_status((commonproto::mail_status_t)(status));
        PB_INT_CPY_NEXT_FIELD(item, send_time);
        PB_BIN_CPY_NEXT_FIELD(item, sender);
        PB_BIN_CPY_NEXT_FIELD(item, title);
        PB_BIN_CPY_NEXT_FIELD(item, content);
        PB_BIN_CPY_NEXT_FIELD(item, attachment);
    PB_STD_QUERY_WHILE_END()
}

uint32_t MailTable::del_mail_by_ids(userid_t userid, uint32_t u_create_tm, const dbproto::cs_mail_del_by_ids &db_in)
{
	int size = db_in.mailids_size();
	for (int i = 0; i < size; i++) {
        std::string safe_mail_id = this->get_mysql_str(const_cast<std::string &>(db_in.mailids(i)));
        GEN_SQLSTR(this->sqlstr,
                "DELETE FROM %s WHERE mailid = '%s'",
                this->get_table_name(userid), safe_mail_id.c_str());
        this->exec_delete_sql(sqlstr, 0);
	}
	return 0;
}

uint32_t MailTable::get_mail_by_ids(userid_t userid, uint32_t u_create_tm,
        const dbproto::cs_mail_get_by_ids &db_in,
		dbproto::sc_mail_get_by_ids &db_out)
{
	std::string str = "(mailid = '";
	if (db_in.mailids_size() != 0) {
        std::string safe_mail_id = this->get_mysql_str(const_cast<std::string &>(db_in.mailids(0)));
		str += safe_mail_id + "' ";
	}
	for (int i = 1; i < db_in.mailids_size(); i++) {
		str = str + "or mailid = '";
        std::string safe_mail_id = this->get_mysql_str(const_cast<std::string &>(db_in.mailids(i)));
        str += safe_mail_id + "' ";

	}
	str += ")";
	GEN_SQLSTR(this->sqlstr,
			" SELECT mailid, status, send_time, sender, title, content, attachment "
			" FROM %s "
			" WHERE userid = %u AND u_create_tm = %u AND %s",
			this->get_table_name(userid), userid, u_create_tm, str.c_str());

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, (&db_out), mail_datas)
        PB_BIN_CPY_NEXT_FIELD(item, mailid);
        uint32_t status;
        INT_CPY_NEXT_FIELD(status);
        item.set_status((commonproto::mail_status_t)(status));
        PB_INT_CPY_NEXT_FIELD(item, send_time);
        PB_BIN_CPY_NEXT_FIELD(item, sender);
        PB_BIN_CPY_NEXT_FIELD(item, title);
        PB_BIN_CPY_NEXT_FIELD(item, content);
        PB_BIN_CPY_NEXT_FIELD(item, attachment);
    PB_STD_QUERY_WHILE_END()
}

uint32_t MailTable::set_mail_status(userid_t userid, uint32_t u_create_tm, 
        std::string mailid, uint32_t status)
{
    std::string safe_mail_id = this->get_mysql_str(const_cast<std::string &>(mailid));

    GEN_SQLSTR(this->sqlstr,
            "UPDATE %s SET status = %u where mailid='%s' and userid=%u AND u_create_tm = %u ",
            this->get_table_name(userid), status, safe_mail_id.c_str(), userid, u_create_tm);

    return this->exec_update_sql(this->sqlstr, KEY_NOFIND_ERR);
}
