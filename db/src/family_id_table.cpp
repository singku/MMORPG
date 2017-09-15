#include "family_id_table.h"
#include "proto/client/attr_type.h"
#include "proto/db/db_errno.h"
#include "sql_utils.h"
#include "macro_utils.h"
#include "utils.h"

FamilyIdTable::FamilyIdTable(mysql_interface* db) 
    : Ctable(db, "dplan_other_db", "family_id_table")
{

}

int FamilyIdTable::create_family(uint32_t &family_id, uint32_t server_id)
{
    GEN_SQLSTR(this->sqlstr,
        "insert into %s (server_id) values(%u)",this->get_table_name(), server_id);
    STD_INSERT_GET_ID(this->sqlstr, db_err_sys_err, family_id);
}
