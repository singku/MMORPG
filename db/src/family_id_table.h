#ifndef FAMILY_ID_TABLE
#define FAMILY_ID_TABLE

extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute100x10.h>
#include "proto/db/dbproto.family.pb.h"
#include "proto/client/common.pb.h"

class FamilyIdTable : public Ctable
{
public:
    FamilyIdTable(mysql_interface* db);

    int create_family(uint32_t &family_id, uint32_t server_id);
};

extern FamilyIdTable* g_family_id_table;

#endif
