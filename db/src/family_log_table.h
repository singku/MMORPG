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


class FamilyLogTable : public CtableRoute
{
public:
    FamilyLogTable(mysql_interface* db);

    int update_family_log(
            const dbproto::family_log_table_t &info, uint32_t flag);

    int get_log_list(
        uint32_t family_id, uint32_t type, 
        uint32_t start , uint32_t page_size,
        dbproto::sc_family_get_log_list *log_list, uint32_t &num);

    int get_total_log_num(
        uint32_t family_id, uint32_t type, uint32_t &total_log_num);

    int del_log(uint32_t family_id, uint32_t num);
};

extern FamilyLogTable* g_family_log_table;

#endif //FAMILY_LOG_TABLE_H
        
