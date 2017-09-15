
#ifndef GLOBAL_ATTR_H
#define GLOBAL_ATTR_H

#include <dbser/Ctable.h>
#include "proto/db/dbproto.data.pb.h"
#include "proto/db/dbproto.attr.pb.h"

class GlobalAttrTable : public Ctable
{
public:
    GlobalAttrTable(mysql_interface* db);

    int get_attr(const dbproto::global_attr_list_t& type_list,
            dbproto::global_attr_list_t* list, uint32_t server_id);

    int add_attr_with_limit(uint32_t type, uint32_t subtype, 
            uint32_t diff ,uint32_t limit, dbproto::sc_update_global_attr &ret,
            uint32_t server_id);

    int minus_attr(uint32_t type, uint32_t subtype, uint32_t diff, uint32_t server_id);

    int set_attr(uint32_t type, uint32_t subtype, uint32_t new_value, uint32_t server_id);
private:
	//调用存储过程proc_add_global_attr更新全服数据
	int exec_call_proc_sql(char * cmd, char * proc_ret, dbproto::sc_update_global_attr &ret, int nofind_err);

};

extern GlobalAttrTable* g_global_attr_table;

#endif
