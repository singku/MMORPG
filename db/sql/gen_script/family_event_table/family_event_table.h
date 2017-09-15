#ifndef FAMILY_EVENT_TABLE_H
#define FAMILY_EVENT_TABLE_H

extern "C" {
#include <libtaomee/project/types.h>
}
#include	"proto/client/attr_type.h"
#include	"proto/db/db_errno.h"
#include	"sql_utils.h"
#include	"macro_utils.h"
#include	"utils.h"


class FamilyEventTable : public CtableRoute100x10
{
public:
    FamilyEventTable(mysql_interface* db);

    int update_family_event(
            const dbproto::family_event_table_t &info);

    int change_family_event(
            const dbproto::family_event_table_change_data_t &info);
};

extern FamilyEventTable* g_family_event_table;

#endif //FAMILY_EVENT_TABLE_H
        