#include "clisoft_table.h"
#include "sql_utils.h"
#include "proto/db/db_errno.h"

CliSoftTable::CliSoftTable(mysql_interface* db) 
    : Ctable(db, "dplan_other_db", "client_soft_version_table")
{
}

uint32_t CliSoftTable::set_clisoft_v(userid_t userid, const dbproto::cs_set_client_soft &msg, uint32_t u_create_tm)
{
    std::string sql_str;
    SQLUtils::gen_replace_sql_from_proto(userid, u_create_tm,
            this->get_table_name(), msg, sql_str, &(this->db->handle));
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}


