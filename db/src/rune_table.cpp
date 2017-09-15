#include "rune_table.h"
#include "utils.h"
#include "sql_utils.h"
#include "proto/db/db_errno.h"

RuneTable::RuneTable(mysql_interface* db)
	: CtableRoute(db, "dplan_db", "rune_table", "userid")
{}

uint32_t RuneTable::get_runes(userid_t userid, uint32_t u_create_tm,
		dbproto::sc_get_login_info& login_info)
{
	GEN_SQLSTR(this->sqlstr,
			" SELECT runeid, rune_type, level, exp, pack_type, pet_catch_time, grid_id"
			" FROM %s "
			" WHERE userid = %u and u_create_tm = %u ",
			this->get_table_name(userid), userid, u_create_tm);

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, (login_info.mutable_rune_list()), rune_data)
        PB_INT_CPY_NEXT_FIELD(item, runeid);
        PB_INT_CPY_NEXT_FIELD(item, index);
        PB_INT_CPY_NEXT_FIELD(item, level);
        PB_INT_CPY_NEXT_FIELD(item, exp);
        PB_INT_CPY_NEXT_FIELD(item, pack_type);
        PB_INT_CPY_NEXT_FIELD(item, pet_catch_time);
        PB_INT_CPY_NEXT_FIELD(item, grid_id);
    PB_STD_QUERY_WHILE_END()
}

uint32_t RuneTable::get_rune_by_instance_id(userid_t userid, 
        uint32_t u_create_tm, uint32_t instance_id,
        commonproto::rune_data_t &rune_info)
{
	GEN_SQLSTR(this->sqlstr,
			" SELECT runeid, rune_type, level, exp, pack_type, pet_catch_time, grid_id"
			" FROM %s "
			" WHERE userid = %u and u_create_tm = %u and runeid = %u",
			this->get_table_name(userid), userid, u_create_tm, instance_id);
    STD_QUERY_ONE_BEGIN(this->sqlstr, db_err_record_not_found)
        uint32_t value;
        INT_CPY_NEXT_FIELD(value); rune_info.set_runeid(value);
        INT_CPY_NEXT_FIELD(value); rune_info.set_index(value);
        INT_CPY_NEXT_FIELD(value); rune_info.set_level(value);
        INT_CPY_NEXT_FIELD(value); rune_info.set_exp(value);
        INT_CPY_NEXT_FIELD(value); rune_info.set_pack_type(value);
        INT_CPY_NEXT_FIELD(value); rune_info.set_pet_catch_time(value);
        INT_CPY_NEXT_FIELD(value); rune_info.set_grid_id(value);
    STD_QUERY_ONE_END()
}

uint32_t RuneTable::save_rune(userid_t userid, uint32_t u_create_tm,
        const commonproto::rune_data_t& rune_data) 
{
	GEN_SQLSTR(this->sqlstr, 
			" INSERT INTO %s "
			" (userid, u_create_tm, runeid, rune_type, level, exp, pack_type, pet_catch_time, grid_id) "
			" VALUES (%u, %u, %u, %u, %u, %u, %u, %u, %u) "
			" ON DUPLICATE KEY UPDATE " 
			" rune_type=%u, level=%u, exp=%u, pack_type=%u, pet_catch_time=%u, grid_id=%u",
			this->get_table_name(userid),
			userid, u_create_tm, rune_data.runeid(), rune_data.index(), rune_data.level(), 
            rune_data.exp(), rune_data.pack_type(), rune_data.pet_catch_time(), rune_data.grid_id(), 
			rune_data.index(), rune_data.level(), rune_data.exp(), rune_data.pack_type(), 
			rune_data.pet_catch_time(), rune_data.grid_id());
	return this->exec_insert_sql(this->sqlstr, db_err_rune_save);
}

uint32_t RuneTable::del_rune(userid_t userid, uint32_t u_create_tm, const dbproto::cs_rune_del& rune_del)
{
	GEN_SQLSTR(this->sqlstr, "DELETE FROM %s WHERE userid=%u AND u_create_tm = %u AND runeid=%u",
		this->get_table_name(userid),userid, u_create_tm, rune_del.runeid());
	return this->exec_delete_sql(this->sqlstr, db_err_user_not_own_rune);
}
