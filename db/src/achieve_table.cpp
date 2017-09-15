#include "achieve_table.h"
#include "proto/db/db_errno.h"

AchieveTable::AchieveTable(mysql_interface* db)
		: CtableRoute(db, "dplan_db", "achieve_table", "userid") {}

uint32_t AchieveTable::get_all_achieve(userid_t userid, uint32_t u_create_tm,
		commonproto::achieve_info_list_t* achv_ptr)
{
	GEN_SQLSTR(this->sqlstr,
		" SELECT achieve_id, get_time"
		" FROM %s "
		" WHERE userid = %u and u_create_tm = %u ",
		this->get_table_name(userid), userid, u_create_tm);
	PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, achv_ptr, ach_info)
        PB_INT_CPY_NEXT_FIELD(item, achieve_id);
        PB_INT_CPY_NEXT_FIELD(item, get_time);
    PB_STD_QUERY_WHILE_END()
}

uint32_t AchieveTable::save_achieves(userid_t userid, uint32_t u_create_tm,
		const commonproto::achieve_info_list_t& achv_list_pb_inf)
{
	for (int i = 0; i < achv_list_pb_inf.ach_info_size(); ++i) {
		int ret = save_one_achieve(userid, u_create_tm, achv_list_pb_inf.ach_info(i));
		if (ret) {
			return ret;
		}
	}
	return 0;
}

uint32_t AchieveTable::save_one_achieve(userid_t userid, uint32_t u_create_tm,
		const commonproto::achieve_info_t& achv_pb_inf)
{
	GEN_SQLSTR(this->sqlstr,
			" INSERT INTO %s "
			" (userid, u_create_tm, achieve_id, get_time) "
			" VALUES (%u, %u, %u, %u) ",
			this->get_table_name(userid),
			userid, u_create_tm, achv_pb_inf.achieve_id(),
			achv_pb_inf.get_time());
	return this->exec_insert_sql(this->sqlstr, db_err_save_achieve_err); 
}
