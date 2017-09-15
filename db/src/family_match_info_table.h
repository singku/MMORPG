#ifndef FAMILY_MATCH_INFO_TABLE_H
#define FAMILY_MATCH_INFO_TABLE_H

extern "C" {
#include <libtaomee/project/types.h>
}
#include	"proto/client/attr_type.h"
#include	"proto/db/db_errno.h"
#include	"sql_utils.h"
#include	"macro_utils.h"
#include	"utils.h"


class FamilyMatchInfoTable : public Ctable
{
public:
    FamilyMatchInfoTable(mysql_interface* db);

    int update_family_match_info(
            const dbproto::family_match_info_table_t &info, uint32_t type);

    int change_family_match_info(
            const dbproto::family_match_info_table_change_data_t &info);

    int get_rand_match_list(
        dbproto::cs_family_get_recommend_list &list_in, 
        dbproto::sc_family_get_recommend_list *list_out);

    int del_match_info(uint32_t family_id);
};

extern FamilyMatchInfoTable* g_family_match_info_table;

#endif //FAMILY_MATCH_INFO_TABLE_H
        
