#include "attr_table.h"
#include "proto/client/attr_type.h"
#include "proto/db/db_errno.h"


AttrTable::AttrTable(mysql_interface* db) 
    : CtableRoute(db, "dplan_db", "attr_table", "userid")
{
}

uint32_t AttrTable::set_attr(userid_t userid, uint32_t type, uint32_t value, uint32_t u_create_tm)
{
    GEN_SQLSTR(this->sqlstr, 
            " INSERT INTO %s "
            " (userid, u_create_tm, type, value) "
            " VALUES(%u, %u, %u, %u) "
            " ON DUPLICATE KEY UPDATE type = %u, value = %u ",
            this->get_table_name(userid), userid, u_create_tm, type, value, type, value);
    return this->exec_insert_sql(this->sqlstr, db_err_attr_set); 
}

uint32_t AttrTable::set_attrs(userid_t userid,
            const dbproto::cs_set_attr& cs_set_attr, uint32_t u_create_tm)
{
    for (int i = 0; i < cs_set_attr.attrs_size(); i++) {
        uint32_t type = cs_set_attr.attrs(i).type();
        uint32_t value = cs_set_attr.attrs(i).value();
        int ret = set_attr(userid, type, value, u_create_tm);
        if (ret) return ret;
    }
    return 0;
}

uint32_t AttrTable::del_attr(userid_t userid, uint32_t type, uint32_t u_create_tm)
{
    GEN_SQLSTR(this->sqlstr,
            "DELETE FROM %s"
            "WHERE userid = %u AND u_create_tm = %u AND type = %u",
            this->get_table_name(userid), userid, u_create_tm, type);
    return this->exec_delete_sql(this->sqlstr, db_err_attr_del);
}

uint32_t AttrTable::del_attrs(userid_t userid,
            const dbproto::cs_del_attr &cs_del_attr, uint32_t u_create_tm)
{
    for(int i = 0; i < cs_del_attr.attr_types_size(); i++) {
        uint32_t type = cs_del_attr.attr_types(i);
        int ret = del_attr(userid, type, u_create_tm);
        if (ret) return ret;
    }
    return 0;
}

uint32_t AttrTable::get_attr(userid_t userid, uint32_t type, uint32_t &value, uint32_t u_create_tm)
{
    GEN_SQLSTR(this->sqlstr,
            "SELECT value FROM %s "
            "WHERE userid = %u AND u_create_tm = %u AND type = %u",
            this->get_table_name(userid), userid, u_create_tm, type);

    STD_QUERY_ONE_BEGIN(this->sqlstr, db_err_record_not_found)
        INT_CPY_NEXT_FIELD(value); 
    STD_QUERY_ONE_END()
}

uint32_t AttrTable::get_attrs(userid_t userid, const dbproto::cs_get_attr &cs_get_attr, 
        dbproto::sc_get_attr &sc_get_attr, uint32_t u_create_tm)
{
    sc_get_attr.Clear();
    for (int i = 0; i < cs_get_attr.type_list_size(); i++) {
        uint32_t type = cs_get_attr.type_list(i);
        uint32_t value = 0;
        int ret = get_attr(userid, type, value, u_create_tm);
        if (ret == db_err_record_not_found) {
            value = 0;
        }
        commonproto::attr_data_t *info = sc_get_attr.add_attrs();
        info->set_type(type);
        info->set_value(value);
    }
    return 0;
}

uint32_t AttrTable::get_all_attrs(userid_t userid, commonproto::attr_data_list_t *list, uint32_t u_create_tm)
{
    GEN_SQLSTR(this->sqlstr,
            "SELECT type, value FROM %s "
            "WHERE userid = %u AND u_create_tm = %u",
            this->get_table_name(userid), userid, u_create_tm);

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, list, attrs);
        PB_INT_CPY_NEXT_FIELD(item, type);
        PB_INT_CPY_NEXT_FIELD(item, value);
    PB_STD_QUERY_WHILE_END();
}

uint32_t AttrTable::get_all_attrs(userid_t userid, std::map<uint32_t, uint32_t>& attr_map, uint32_t u_create_tm)
{
    attr_map.clear();
    commonproto::attr_data_list_t list;
    int ret = get_all_attrs(userid, &list, u_create_tm);
    if (ret) return ret;

    for (int i = 0; i < list.attrs_size(); i++) {
        attr_map[list.attrs(i).type()] = list.attrs(i).value();
    }
    return 0;
}

uint32_t AttrTable::get_all_attrs(userid_t userid,
             dbproto::sc_get_login_info& login_info, uint32_t u_create_tm)
{
    commonproto::attr_data_list_t list;
    int ret = get_all_attrs(userid, &list, u_create_tm);
    if (ret) return ret;

    login_info.mutable_attr_list()->CopyFrom(list);
    return 0;
}

uint32_t AttrTable::get_ranged_attr(userid_t userid, uint32_t low, uint32_t high, 
        commonproto::attr_data_list_t* list, uint32_t u_create_tm)
{
    GEN_SQLSTR(this->sqlstr,
            " SELECT type, value FROM %s "
            " WHERE userid = %u AND u_create_tm = %u "
            " AND type >= %u AND type <= %u",
            this->get_table_name(userid),
            userid, u_create_tm, low, high);

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, list, attrs);
        PB_INT_CPY_NEXT_FIELD(item, type);
        PB_INT_CPY_NEXT_FIELD(item, value);
    PB_STD_QUERY_WHILE_END();

    return 0;
}

uint32_t AttrTable::ranged_clear(userid_t userid, uint32_t low, uint32_t high, uint32_t u_create_tm)
{
    GEN_SQLSTR(this->sqlstr, 
            " DELETE FROM %s "
            " WHERE userid = %u AND u_create_tm = %u "
            " AND type >= %u "
            " AND type <= %u ",
            this->get_table_name(userid),
            userid, u_create_tm, low, high); 

    return exec_delete_sql(this->sqlstr, DB_SUCC);
}

uint32_t AttrTable::change_attr(userid_t userid, uint32_t type, 
        int32_t change, uint32_t max_value, uint32_t u_create_tm)
{
    if (change > 0) {
        if ((uint32_t)change > max_value) {
            change = max_value;
        }
        GEN_SQLSTR(this->sqlstr,
                " INSERT INTO %s "
                " (userid, u_create_tm, type, value) "
                " VALUES(%u, %u, %u, %d) "
                " ON DUPLICATE KEY UPDATE "
                " value = IF(value + %d > %u, %u, value + %d) ",
                this->get_table_name(userid),
                userid, u_create_tm, type, change,
                change, max_value, max_value, change);

    } else if (change < 0){
        GEN_SQLSTR(this->sqlstr,
                " INSERT INTO %s "
                " (userid, u_create_tm, type, value) "
                " VALUES(%u, %u, %u, 0) "
                " ON DUPLICATE KEY UPDATE "
                " value = IF(value +  (%d) < 0, 0, value + (%d)) ",
                this->get_table_name(userid),
                userid, u_create_tm, type, change, change);
    }
    return this->exec_update_sql(this->sqlstr, DB_SUCC);
}

uint32_t AttrTable::set_attr_once(userid_t userid, uint32_t type, uint32_t value, uint32_t u_create_tm)
{
    GEN_SQLSTR(this->sqlstr,
            " INSERT INTO %s "
            " (userid, u_create_tm, type, value) "
            " VALUES(%u, %u, %u, %u) "
            " ON DUPLICATE KEY UPDATE "
            " value = IF(value > 0, value, %u) ",
            this->get_table_name(userid),
            userid, u_create_tm, type, value, value);

    return this->exec_update_sql(this->sqlstr, DB_SUCC);
}
