#ifndef ATTR_TABLE_H
#define ATTR_TABLE_H

#include <set>
extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute10x10.h>
#include "proto/db/dbproto.attr.pb.h"
#include "proto/db/dbproto.login.pb.h"
#include "proto/db/dbproto.item.pb.h"
#include "proto/client/common.pb.h"
#include "attr_type.h"
class AttrTable : public CtableRoute
{
public:
    AttrTable(mysql_interface* db);

    uint32_t set_attr(userid_t userid, uint32_t type, uint32_t value, uint32_t u_create_tm);
    uint32_t set_attrs(userid_t userid, const dbproto::cs_set_attr& cs_set_attr, uint32_t u_create_tm);

    uint32_t del_attr(userid_t userid, uint32_t type, uint32_t u_create_tm);
    uint32_t del_attrs(userid_t userid, const dbproto::cs_del_attr& cs_del_attr, uint32_t u_create_tm);
    
    uint32_t get_attr(userid_t userid, uint32_t type, uint32_t &value, uint32_t u_create_tm);
    uint32_t get_attrs(userid_t userid, const dbproto::cs_get_attr &cs_get_attr, 
            dbproto::sc_get_attr &sc_get_attr, uint32_t u_create_tm);
    uint32_t get_all_attrs(userid_t userid, commonproto::attr_data_list_t *attrs, uint32_t u_create_tm);
    uint32_t get_all_attrs(userid_t userid, std::map<uint32_t, uint32_t>& attr_map, uint32_t u_create_tm);
    uint32_t get_all_attrs(userid_t userid, dbproto::sc_get_login_info& sc_login_info, uint32_t u_create_tm);

    uint32_t get_ranged_attr(userid_t userid, uint32_t low, uint32_t high, 
            commonproto::attr_data_list_t* list, uint32_t u_create_tm);
    uint32_t ranged_clear(userid_t userid, uint32_t low, uint32_t high, uint32_t u_create_tm);

    uint32_t change_attr(userid_t userid, uint32_t type, int32_t change, uint32_t max_value, uint32_t u_create_tm);

    uint32_t set_attr_once(userid_t userid, uint32_t type, uint32_t value, uint32_t u_create_tm);

private:
};

extern AttrTable* g_attr_table;
 
#endif
