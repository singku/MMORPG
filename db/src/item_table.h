
#ifndef ITEM_TABLE_H
#define ITEM_TABLE_H

extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute.h>
#include "proto/db/dbproto.login.pb.h"
#include "proto/db/dbproto.item.pb.h"

class ItemTable : public CtableRoute
{
public:
    ItemTable(mysql_interface* db);

    uint32_t get_items(userid_t userid, uint32_t u_create_tm,
            dbproto::sc_get_login_info *login_info);

    uint32_t update_items(userid_t userid, uint32_t u_create_tm,
            const dbproto::cs_change_items &change_items);
    
    uint32_t get_item_by_slot_id(userid_t userid, uint32_t u_create_tm,
            uint32_t slot_id, commonproto::item_info_t *info);
};

extern ItemTable* g_item_table;
 
#endif
