#ifndef RUNE_TABLE_H
#define RUNE_TABLE_H

extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute10x10.h>
#include "proto/db/dbproto.rune.pb.h"
#include "proto/db/dbproto.data.pb.h"
#include "proto/db/dbproto.login.pb.h"

class RuneTable : public CtableRoute
{
public:
	RuneTable(mysql_interface* db);
	uint32_t get_runes(userid_t userid, uint32_t u_create_tm,
			dbproto::sc_get_login_info& login_info);

    uint32_t get_rune_by_instance_id(userid_t userid, uint32_t u_create_tm,
            uint32_t instance_id,
            commonproto::rune_data_t &rune_info);

	uint32_t save_rune(userid_t userid, uint32_t u_create_tm,
			const commonproto::rune_data_t& rune_data);

	uint32_t del_rune(userid_t userid, uint32_t u_create_tm,
			const dbproto::cs_rune_del& rune_del);
};

extern RuneTable* g_rune_table;

#endif
