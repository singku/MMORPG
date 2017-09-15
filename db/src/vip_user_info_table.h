
#ifndef VIP_USER_INFO_TABLE_H
#define VIP_USER_INFO_TABLE_H

#include <dbser/mysql_iface.h>
#include <dbser/Ctable.h>
#include "proto/db/dbproto.transaction.pb.h"
class VipUserInfoTable : public Ctable
{
public:
	VipUserInfoTable(mysql_interface* db);
	int32_t new_vip_table_info(userid_t userid, const dbproto::vip_user_info_t& inf);
};
extern VipUserInfoTable* g_vip_user_info_table;
#endif
