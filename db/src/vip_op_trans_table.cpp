#include "vip_op_trans_table.h"
#include "proto/db/db_errno.h"

VipOpTransTable::VipOpTransTable(mysql_interface* db)
	: Ctable(db, "dplan_other_db", "vip_op_trans_table")
{}

int32_t VipOpTransTable::new_vip_op_trans_rd(userid_t userid,
		const dbproto::vip_op_trans_info_t& inf,
		uint32_t &auto_incr_id)
{
	GEN_SQLSTR(this->sqlstr, " insert into %s (user_id, u_create_tm, server_num, "
			" cmd_id, trade_id, apply_time, begin_time, end_time, "
			" time_flag, fee_flag, action_type, time_length, "
			" vip_type, ct_vip_type) values "
			" (%u, %u, %u, "
			" %u, %u, FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), "
			" %u, %u, %u, %u, "
			" %u, %u) ",
			get_table_name(),
			inf.user_id(),
			inf.u_create_tm(),
			inf.server_no(),

			inf.cmd_id(),
			inf.trade_id(),
			inf.apply_time(),
			inf.begin_time(),
			inf.end_time(),

			inf.time_flag(),
			inf.fee_flag(),
			inf.action_type(),
			inf.time_length(),
			inf.vip_type(),
			inf.ct_vip_type());
	return this->exec_insert_sql_get_auto_increment_id(this->sqlstr, 0, &auto_incr_id);
}
