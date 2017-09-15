#ifndef HM_VISIT_LOG_TABLE_H
#define HM_VISIT_LOG_TABLE_H

extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute10x10.h>
#include "proto/db/dbproto.home.pb.h"
#include "proto/client/common.pb.h"

class HmVisitLogTable : public CtableRoute
{
public:
	HmVisitLogTable(mysql_interface* db);
	uint32_t add_visit_log(userid_t userid,
			const commonproto::visit_log_info_t& log_info);
	uint32_t get_visit_log(userid_t userid, uint32_t u_create_tm, 
			commonproto::visit_log_list_t *list);
	uint32_t get_hm_gift_log(userid_t userid, uint32_t u_create_tm, 
			commonproto::visit_log_list_t *list);
};

extern HmVisitLogTable *g_hm_visit_table;

#endif
