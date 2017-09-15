#include "item_table.h"
#include "proto/db/db_errno.h"
#include "libtaomee++/utils/strings.hpp"

enum {
    ITEM_OPTIONAL_ATTR_BUF_MAX_SIZE = 1024
};
    
ItemTable::ItemTable(mysql_interface* db) 
    : CtableRoute(db, "dplan_db", "item_table", "userid")
{
}

uint32_t ItemTable::get_items(userid_t userid, uint32_t u_create_tm,
        dbproto::sc_get_login_info* login_info)
{
    GEN_SQLSTR(this->sqlstr,
            " SELECT item_id, slot_id, count, using_count, "
            " UNIX_TIMESTAMP(expire_time), "
            " item_optional_attr "   
            " FROM %s WHERE userid = %u AND u_create_tm = %u ",
            get_table_name(userid), userid, u_create_tm);

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, login_info->mutable_item_list(), item_list);
    PB_INT_CPY_NEXT_FIELD(item, item_id);
    PB_INT_CPY_NEXT_FIELD(item, slot_id);
    PB_INT_CPY_NEXT_FIELD(item, count);
    PB_INT_CPY_NEXT_FIELD(item, using_count);
    PB_INT_CPY_NEXT_FIELD(item, expire_time);
    PB_OBJ_CPY_NEXT_FIELD(item, item_optional_attr);
    PB_STD_QUERY_WHILE_END()
    
}

uint32_t ItemTable::get_item_by_slot_id(userid_t userid, uint32_t u_create_tm, 
        uint32_t slot_id, commonproto::item_info_t *info)
{
    GEN_SQLSTR(this->sqlstr,
            " SELECT item_id, slot_id, count, using_count, "
            " UNIX_TIMESTAMP(expire_time), "
            " item_optional_attr "   
            " FROM %s WHERE userid = %u and slot_id = %u and u_create_tm = %u ",
            get_table_name(userid), userid, slot_id, u_create_tm);

    info->Clear();
    commonproto::item_list_t item_list;
    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, (&item_list), item_list);
    PB_INT_CPY_NEXT_FIELD(item, item_id);
    PB_INT_CPY_NEXT_FIELD(item, slot_id);
    PB_INT_CPY_NEXT_FIELD(item, count);
    PB_INT_CPY_NEXT_FIELD(item, using_count);
    PB_INT_CPY_NEXT_FIELD(item, expire_time);
    PB_OBJ_CPY_NEXT_FIELD(item, item_optional_attr);
    info->CopyFrom(item);
    PB_STD_QUERY_WHILE_END()
}

uint32_t ItemTable::update_items(userid_t userid, uint32_t u_create_tm, 
        const dbproto::cs_change_items &change_items)
{
    int count = change_items.item_info_list_size();
    if (count == 0) {
        return 0; 
    }

    static char item_optional_attr_buf[ITEM_OPTIONAL_ATTR_BUF_MAX_SIZE] = {0};
    static char item_optional_attr_buf_safe[ITEM_OPTIONAL_ATTR_BUF_MAX_SIZE*2] = {0};

    for (int i = 0; i < count; i++) {
        const commonproto::item_info_t& item_info = 
            change_items.item_info_list(i);

        memset(item_optional_attr_buf, 0, sizeof(item_optional_attr_buf));
        memset(item_optional_attr_buf_safe, 0, sizeof(item_optional_attr_buf_safe));

        if (0 == item_info.count()) {
            GEN_SQLSTR(this->sqlstr,
                    " DELETE FROM %s "
                    " WHERE userid = %u AND u_create_tm = %u AND slot_id = %u ",
                    this->get_table_name(userid),
                    userid, u_create_tm, item_info.slot_id()); 

        } else if (item_info.has_item_optional_attr()) {
            item_info.item_optional_attr().SerializeToArray(
                    item_optional_attr_buf, sizeof(item_optional_attr_buf));
            set_mysql_string(item_optional_attr_buf_safe, 
                    item_optional_attr_buf, item_info.item_optional_attr().ByteSize());

            GEN_SQLSTR(this->sqlstr, 
                    " INSERT INTO %s "
                    " (userid, u_create_tm, item_id, slot_id, count, using_count, " 
                    " expire_time, "
                    " item_optional_attr)"
                    " VALUES (%u, %u, %u, %u, %u, %u, "
                    " FROM_UNIXTIME(%u), "
                    " '%s' ) "
                    " ON DUPLICATE KEY UPDATE "
                    " item_id = %u, count = %u, using_count = %u, "
                    " expire_time = from_unixtime(%u), item_optional_attr = '%s'",                       
                    this->get_table_name(userid),
                    userid, u_create_tm, item_info.item_id(), item_info.slot_id(), 
                    item_info.count(), item_info.using_count(), item_info.expire_time(),
                    item_optional_attr_buf_safe,
                    item_info.item_id(),
                    item_info.count(),
                    item_info.using_count(),                       
                    item_info.expire_time(),
                    item_optional_attr_buf_safe);

        } else {
            GEN_SQLSTR(this->sqlstr, 
                    " INSERT INTO %s "
                    " (userid, u_create_tm, item_id, slot_id, count, using_count, " 
                    " expire_time)  "
                    " VALUES (%u, %u, %u, %u, %u, %u, "
                    " FROM_UNIXTIME(%u) ) "
                    " ON DUPLICATE KEY UPDATE "
                    " item_id = %u, count = %u, using_count = %u, "
                    " expire_time = from_unixtime(%u)",                                   
                    this->get_table_name(userid),
                    userid, u_create_tm, item_info.item_id(), 
                    item_info.slot_id(), item_info.count(),item_info.using_count(),                                               
                    item_info.expire_time(),
                    item_info.item_id(),
                    item_info.count(),
                    item_info.using_count(),                       
                    item_info.expire_time());
        }
        int ret = this->exec_insert_sql(this->sqlstr, KEY_EXISTED_ERR);
        if (ret != 0) {
            ERROR_LOG("exec sql err: %s", this->sqlstr);
            return ret;
        }
    }

    return 0;
}
