#ifndef CLISOFT_TABLE_H
#define CLISOFT_TABLE_H

#include <set>
extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/Ctable.h>
#include "proto/db/dbproto.clisoft.pb.h"


class CliSoftTable : public Ctable
{
public:
    CliSoftTable(mysql_interface* db);
    uint32_t set_clisoft_v(userid_t userid, const dbproto::cs_set_client_soft &msg, uint32_t u_create_tm);
};

extern CliSoftTable *g_clisoftv_table;

#endif
