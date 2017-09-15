#ifndef FAMILY_LOG_TABLE_H
#define FAMILY_LOG_TABLE_H

extern "C" {
#include <libtaomee/project/types.h>
}
#include	"proto/client/attr_type.h"
#include	"proto/db/db_errno.h"
#include	"sql_utils.h"
#include	"macro_utils.h"
#include	"utils.h"


class FamilyLogTable : public CtableRoute100x100
{
public:
    FamilyLogTable(mysql_interface* db);

    int update_family_log(
            const dbproto::family_log_table_t &info, uint32_t type);

    int change_family_log(
            const dbproto::family_log_table_change_data_t &info);
};

extern FamilyLogTable* g_family_log_table;

#endif //FAMILY_LOG_TABLE_H
        