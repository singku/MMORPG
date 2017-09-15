#include "gift_code_table.h"
#include "proto/db/db_errno.h"

GiftCodeTable::GiftCodeTable(mysql_interface* db) 
    : Ctable(db, "dplan_other_db", "gift_code_table")
{
}

uint32_t GiftCodeTable::use_gift_code(userid_t userid, uint32_t u_create_tm, uint32_t server_id, std::string code)
{
    uint32_t now = (uint32_t)time(0);

    GEN_SQLSTR(this->sqlstr,
            " UPDATE %s SET userid = %u, u_create_tm = %u, on_server_id = %u, used_tm = from_unixtime(%u) "
            " WHERE code = '%s' AND status = 0 AND userid = 0 AND unix_timestamp(start) <= %u AND unix_timestamp(end) >= %u",
            this->get_table_name(), userid, u_create_tm, server_id, now, code.c_str(), now, now);

    return this->exec_update_sql(this->sqlstr, db_err_record_not_found);
}

uint32_t GiftCodeTable::get_gift_code(std::string code, uint32_t &prize_id, uint32_t &status, uint32_t &userid)
{
    GEN_SQLSTR(this->sqlstr,
            "SELECT prize_id, userid, status from %s WHERE code = '%s'",
            this->get_table_name(), code.c_str());
    STD_QUERY_ONE_BEGIN(this->sqlstr, db_err_record_not_found)
        prize_id = atoi_safe(row[0]);
        userid = atoi_safe(row[1]);
        status = atoi_safe(row[2]);
    STD_QUERY_ONE_END()
}
