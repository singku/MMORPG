#ifndef FAMILY_INFO_TABLE
#define FAMILY_INFO_TABLE

extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute100x10.h>
#include "proto/db/dbproto.family.pb.h"
#include "proto/client/common.pb.h"

class FamilyInfoTable : public CtableRoute100x10
{
public:
    FamilyInfoTable(mysql_interface* db);

    int get_family_info(
            uint32_t family_id, 
            commonproto::family_info_t &family_info);

    int update_family_info(
            const dbproto::family_info_table_t &info, uint32_t flag);

    int change_family_info(
        const dbproto::family_info_change_data_t &info);

    int del_family(uint32_t family_id);
};

extern FamilyInfoTable* g_family_info_table;

#endif
