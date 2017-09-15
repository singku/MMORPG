#include "vip_user_info_table.h"
#include "proto/db/db_errno.h"

VipUserInfoTable::VipUserInfoTable(mysql_interface* db)
	: Ctable(db, "dplan_other_db", "vip_user_info_table")
{}

int32_t VipUserInfoTable::new_vip_table_info(userid_t userid,
	const dbproto::vip_user_info_t& inf)
{
	GEN_SQLSTR(this->sqlstr, " insert into %s (user_id, u_create_tm, server_num, "
		" begin_time, end_time, time_flag, fee_flag, "
		" curr_time, vip_type, ct_vip_type) values (%u, %u, %u, "
		" FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), %u, %u, "
		" FROM_UNIXTIME(%u), %u, %u) "
		" ON DUPLICATE KEY UPDATE "
		" server_num=%u, begin_time=FROM_UNIXTIME(%u), "
		" end_time=FROM_UNIXTIME(%u), time_flag=%u, fee_flag=%u, "
		" curr_time=FROM_UNIXTIME(%u), vip_type=%u ", 
			get_table_name(),
			userid,
			inf.u_create_tm(),
			inf.server_no(),

			inf.begin_time(),
			inf.end_time(),
			inf.time_flag(),
			inf.fee_flag(),
			inf.curr_time(),
			inf.vip_type(),
			inf.ct_vip_type(),

			inf.server_no(),
			inf.begin_time(),
			inf.end_time(),
			inf.time_flag(),
			inf.fee_flag(),
			inf.curr_time(),
			inf.vip_type()
			);
	return this->exec_insert_sql(this->sqlstr, DB_SUCC);
}
