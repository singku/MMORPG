#ifndef VIP_OP_TRANS_TABLE_H
#define VIP_OP_TRANS_TABLE_H

#include <dbser/mysql_iface.h>
#include <dbser/Ctable.h>
#include "proto/db/dbproto.transaction.pb.h"
class VipOpTransTable : public Ctable
{
public:
	VipOpTransTable(mysql_interface* db);
	int32_t new_vip_op_trans_rd(userid_t userid,
			const dbproto::vip_op_trans_info_t& inf,
			uint32_t &auto_incr_id);
};

extern VipOpTransTable* g_vip_op_trans_table;

#endif
