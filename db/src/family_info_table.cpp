#include "family_info_table.h"
#include "proto/client/attr_type.h"
#include "proto/db/db_errno.h"
#include "sql_utils.h"
#include "macro_utils.h"
#include "utils.h"

FamilyInfoTable::FamilyInfoTable(mysql_interface* db) 
    : CtableRoute100x10(db, "dplan_family_db", "family_info_table", "family_id")
{

}

int FamilyInfoTable::update_family_info(
        const dbproto::family_info_table_t &info, uint32_t flag)
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
        return db_err_gen_sql_from_proto_failed;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}

int FamilyInfoTable::change_family_info(
        const dbproto::family_info_change_data_t &info)
{
    std::string sql_str;
    SQLUtils::gen_change_sql_from_proto_any_key(
            this->get_table_name(info.family_id()),
            info, sql_str, &(this->db->handle)); 

    if (sql_str.size() == 0) {
        return db_err_gen_sql_from_proto_failed;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}


int FamilyInfoTable::get_family_info(
        uint32_t family_id, commonproto::family_info_t &family_info)
{
    GEN_SQLSTR(this->sqlstr,
            "select family_id, family_name,  level, pet_name, pet_level, member_num,"
            "construct_value, board_msg, creator_id, u_create_tm, create_time, join_type,"
            "base_join_value, last_member_login_time, server_id "
            "from %s where family_id = %u",
            this->get_table_name(family_id), family_id);

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);

	if (ret) {
		return ret;
	}

    if ((row = mysql_fetch_row(res)) != NULL) {
		family_info.set_family_id(atoi_safe(row[0]));
		family_info.set_family_name(row[1]);
		family_info.set_level(atoi_safe(row[2]));
		family_info.set_pet_name(row[3]);
		family_info.set_pet_level(atoi_safe(row[4]));

		family_info.set_member_num(atoi_safe(row[5]));
		family_info.set_construct_value(atoi_safe(row[6]));
		family_info.set_board_msg(row[7]);

		family_info.mutable_role()->set_userid(atoi_safe(row[8]));
		family_info.mutable_role()->set_u_create_tm(9);
		family_info.set_create_time(atoi_safe(row[10]));
		family_info.set_join_type(atoi_safe(row[11]));
		family_info.set_base_join_value(atoi_safe(row[12]));
		family_info.set_last_member_login_time(atoi_safe(row[13]));
		family_info.set_server_id(atoi_safe(row[14]));
	}
	mysql_free_result(res);
    return 0;
}

int FamilyInfoTable::del_family(uint32_t family_id)
{
    GEN_SQLSTR(this->sqlstr,
         "delete from %s where family_id = %u",
         this->get_table_name(family_id), family_id);
    int affected_rows = 0;
    return this->db->exec_update_sql(this->sqlstr, &affected_rows);
}
