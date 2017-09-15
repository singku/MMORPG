
#include <dbser/mysql_iface.h>
#include <libtaomee/hash_algo.h>
#include "nick_table.h"
#include "proto/db/db_errno.h"

NickTable::NickTable(mysql_interface* db) 
    : CtableRoute100(db, "dplan_other_db", "nick_table", "nick_hash")
{
}


uint32_t NickTable::get_user_by_nick(const std::string& nick, role_info_t &user)
{
    uint32_t table_no = get_table_no(nick);

    char nick_buf[64 + 1] = {0};
    char nick_mysql[2 * sizeof(nick_buf) + 1] = {0};

    strncpy(nick_buf, nick.c_str(), sizeof(nick_buf) - 1);
    set_mysql_string(nick_mysql, nick_buf, strlen(nick_buf));

    GEN_SQLSTR(this->sqlstr, 
            " SELECT userid, u_create_tm FROM %s "
            " WHERE nick = '%s' ",
            this->get_table_name(table_no), nick_mysql);

    MYSQL_RES* res = NULL;
    MYSQL_ROW row;
    this->db->exec_query_sql(this->sqlstr, &res);
    
    if (!res) {
        user.userid = 0;
        user.u_create_tm = 0;
        return 1;
    }

    row = mysql_fetch_row(res);
    if (row != NULL) {
        user.userid = atoi_safe(row[0]);
        user.u_create_tm = atoi_safe(row[1]);
    }
    mysql_free_result(res);
    return 0;
}

uint32_t NickTable::insert_nick_and_user(const std::string& nick, role_info_t &user)
{
    uint32_t table_no = get_table_no(nick);

    char nick_buf[64 + 1] = {0};
    char nick_mysql[2 * sizeof(nick_buf) + 1] = {0};

    strncpy(nick_buf, nick.c_str(), sizeof(nick_buf) - 1);
    set_mysql_string(nick_mysql, nick_buf, strlen(nick_buf));

    GEN_SQLSTR(this->sqlstr,
            " INSERT INTO %s (nick, userid, u_create_tm) "
            " VALUES('%s', %u, %u) ",
            this->get_table_name(table_no), nick_mysql, user.userid, user.u_create_tm);

    return this->exec_insert_sql(this->sqlstr, db_err_nick_already_exist);
}

uint32_t NickTable::delete_nick_and_user(const std::string& nick)
{
    uint32_t table_no = get_table_no(nick);

    char nick_buf[64 + 1] = {0};
    char nick_mysql[2 * sizeof(nick_buf) + 1] = {0};

    strncpy(nick_buf, nick.c_str(), sizeof(nick_buf) - 1);
    set_mysql_string(nick_mysql, nick_buf, strlen(nick_buf));


    GEN_SQLSTR(this->sqlstr,
            " DELETE FROM %s "
            " WHERE nick = '%s' ",
            this->get_table_name(table_no), nick_mysql);

    return this->exec_delete_sql(this->sqlstr, db_err_nick_not_exist);
}

uint32_t NickTable::get_hash(const std::string& s)
{
    return r5hash(s.c_str());
}


uint32_t NickTable::get_table_no(const std::string& s)
{
    uint32_t table_no = 0;
    if (s.size() == 0){
        return 0;
    }
    table_no = get_hash(s.c_str());
    return table_no;
}
