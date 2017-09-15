#include	<dbser/mysql_iface.h>
#include	<dbser/CtableRoute100x10.h>
#include	"proto/db/dbproto.family.pb.h"
#include	"proto/client/common.pb.h"
#include	"sql_utils.h"
#include	"family_event_table.h"

FamilyEventTable::FamilyEventTable(mysql_interface* db) 
    : CtableRoute100x10(db, "dplan_family_db", "family_event_table", "family_id")
{

}

int FamilyEventTable::update_family_event(
        const dbproto::family_event_table_t &info, uint32_t flag)
{
    std::string sql_str;
    if (flag == (uint32_t)dbproto::DB_UPDATE_AND_INESRT) {
        SQLUtils::gen_replace_sql_from_proto_any_key(
                this->get_table_name(info.family_id()),
                info, sql_str, &(this->db->handle)); 
    } else {
        SQLUtils::gen_update_sql_from_proto_any_key(
                this->get_table_name(info.family_id()),
                info, sql_str, &(this->db->handle));
    }

    if (sql_str.size() == 0) {
        return db_err_proto_format_err;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}

int FamilyEventTable::change_family_event(
        const dbproto::family_event_table_change_data_t &info)
{
    std::string sql_str;
    SQLUtils::gen_change_sql_from_proto_any_key(
            this->get_table_name(info.family_id()),
            info, sql_str, &(this->db->handle)); 

    if (sql_str.size() == 0) {
        return db_err_proto_format_err;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}
