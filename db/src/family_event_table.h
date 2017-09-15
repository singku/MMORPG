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
            const dbproto::family_event_table_t &info, uint32_t flag);

    int change_family_event(
            const dbproto::family_event_table_change_data_t &info);

    int del_oldest_event(uint32_t family_id, uint32_t type);

    int del_event(uint32_t family_id, uint32_t userid, uint32_t u_create_tm, uint32_t type);

    int get_event_info(
        uint32_t family_id, uint32_t userid, uint32_t u_create_tm,
        uint32_t type, commonproto::family_event_t &event);

    int get_event_list(
        uint32_t family_id, uint32_t userid, uint32_t u_create_tm,
        uint32_t type, uint32_t start , uint32_t page_size,
        dbproto::sc_family_get_event_list *event_list, uint32_t &num);

    int get_total_event_num(
        uint32_t family_id, uint32_t userid, uint32_t u_create_tm,
        uint32_t type, uint32_t &total_event_num);

    int get_rand_match_list(
        dbproto::cs_family_get_recommend_list &list_in, 
        dbproto::sc_family_get_recommend_list *list_out);
};

extern FamilyEventTable* g_family_event_table;

#endif //FAMILY_EVENT_TABLE_H
        
