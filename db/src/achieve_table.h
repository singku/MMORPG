#ifndef ACHIEVE_TABLE_H
#define ACHIEVE_TABLE_H
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute10x10.h>
#include "proto/client/common.pb.h"

class AchieveTable : public CtableRoute
{
public:
	AchieveTable(mysql_interface* db);
	uint32_t get_all_achieve(userid_t userid, uint32_t u_create_tm,
		commonproto::achieve_info_list_t* achv_ptr);

	uint32_t save_achieves(userid_t userid, uint32_t u_create_tm,
			const commonproto::achieve_info_list_t& achv_list_pb_inf);

	uint32_t save_one_achieve(userid_t userid, uint32_t u_create_tm,
			const commonproto::achieve_info_t& achv_pb_inf);
};

extern AchieveTable* g_achieve_table;
#endif
