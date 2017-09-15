#include "proto/db/db_errno.h"
#include "user_action_log_table.h"
#include "utils.h"

UserActionLogTable::UserActionLogTable(mysql_interface* db)
    : CtableDate(db, "dplan_other_db", "user_action_log_table", "id")
{
}

uint32_t UserActionLogTable::insert_log(userid_t userid, uint32_t u_create_tm, 
        const dbproto::user_action_log_t& insert_msg)
{
    GEN_SQLSTR(this->sqlstr, 
            " INSERT INTO %s "
            " (userid, u_create_tm, type, target_id, value, extra1, extra2, extra3, insert_time) "
            " VALUES (%u, %u, %u, %u, %d, %u, %u, %u, FROM_UNIXTIME(%u)) ",
            this->get_table_name(insert_msg.insert_time()),
            userid, u_create_tm, insert_msg.type(), insert_msg.target_id(), insert_msg.value(),
            insert_msg.extra1(), insert_msg.extra2(), insert_msg.extra3(), insert_msg.insert_time());

    int ret = exec_insert_sql(sqlstr, DB_ERR);

    if (ret) {
        char create_sql[1024] = {0};
        snprintf(create_sql, sizeof(create_sql), "\
                CREATE TABLE IF NOT EXISTS %s \
                (\
                 id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,\
                 userid INT UNSIGNED NOT NULL,\
                 u_create_tm INT UNSIGNED NOT NULL DEFAULT 0, \
                 type TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '消息类型',\
                 target_id INT NOT NULL,\
                 value INT NOT NULL,\
                 extra1 INT UNSIGNED NOT NULL DEFAULT 0,\
                 extra2 INT UNSIGNED NOT NULL DEFAULT 0,\
                 extra3 INT UNSIGNED NOT NULL DEFAULT 0,\
                 insert_time TIMESTAMP NOT NULL DEFAULT 0 COMMENT '插入时间',\
                 INDEX (userid) \
                ) ENGINE = INNODB CHARSET = UTF8;", 
                this->get_table_name(insert_msg.insert_time())); 

        ret = exec_update_sql(create_sql, DB_SUCC);
        if (ret == 0) {
            ret = exec_insert_sql(sqlstr, DB_ERR);
            if (ret) {
                return ret; 
            }
        } else {
            return ret; 
        }
    }
    return 0;
}
    
uint32_t UserActionLogTable::get_action_log(userid_t userid, uint32_t u_create_tm, uint32_t date, 
        uint32_t start_index, uint32_t num, dbproto::sc_get_user_action_log* log_list)
{
    log_list->Clear();
    GEN_SQLSTR(this->sqlstr,
            " SELECT type, target_id, value, extra1, extra2, extra3, UNIX_TIMESTAMP(insert_time)"
            " FROM dplan_other_db.user_action_log_table_%u "
            " WHERE userid = %u AND u_create_tm = %u limit %u, %u",
            date, userid, u_create_tm, start_index, num); 

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, log_list->mutable_log_list(), logs);
        dbproto::action_log_type_t type = (dbproto::action_log_type_t)(atoi_safe(NEXT_FIELD));
        item.set_type(type);
        PB_INT_CPY_NEXT_FIELD(item, target_id);
        PB_INT_CPY_NEXT_FIELD(item, value);
        PB_INT_CPY_NEXT_FIELD(item, extra1);
        PB_INT_CPY_NEXT_FIELD(item, extra2);
        PB_INT_CPY_NEXT_FIELD(item, extra3);
        PB_INT_CPY_NEXT_FIELD(item, insert_time);
    PB_STD_QUERY_WHILE_END();
}

uint32_t UserActionLogTable::get_action_log_with_tar_id(
		userid_t userid, uint32_t u_create_tm, uint32_t date,
		uint32_t start_index, uint32_t num, uint32_t tar_id,
		dbproto::sc_get_user_action_log* log_list)
{
    log_list->Clear();
    GEN_SQLSTR(this->sqlstr,
            " SELECT type, target_id, value, extra1, extra2, extra3, UNIX_TIMESTAMP(insert_time)"
            " FROM dplan_other_db.user_action_log_table_%u "
            " WHERE userid = %u AND u_create_tm = %u AND target_id = %u limit %u, %u",
            date, userid, u_create_tm, tar_id, start_index, num); 

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, log_list->mutable_log_list(), logs);
        dbproto::action_log_type_t type = (dbproto::action_log_type_t)(atoi_safe(NEXT_FIELD));
        item.set_type(type);
        PB_INT_CPY_NEXT_FIELD(item, target_id);
        PB_INT_CPY_NEXT_FIELD(item, value);
        PB_INT_CPY_NEXT_FIELD(item, extra1);
        PB_INT_CPY_NEXT_FIELD(item, extra2);
        PB_INT_CPY_NEXT_FIELD(item, extra3);
        PB_INT_CPY_NEXT_FIELD(item, insert_time);
    PB_STD_QUERY_WHILE_END();
}

uint32_t UserActionLogTable::get_login_tm_info_ex(userid_t userid, uint32_t u_create_tm, 
        uint32_t date, uint32_t from_tm, uint32_t to_tm, std::set<uint32_t> &login_tm_vec)
{
    if (u_create_tm) { //如果没指定u_create_tm则查询所有角色
        GEN_SQLSTR(this->sqlstr,
                " SELECT UNIX_TIMESTAMP(insert_time)"
                " FROM dplan_other_db.user_action_log_table_%u "
                " WHERE userid = %u AND u_create_tm = %u and type = %u "
                " UNIX_TIMESTAMP(insert_time) >= %u and UNIX_TIMESTAMP(insert_time) <= %u limit 100",
                date, userid, u_create_tm, (uint32_t)(dbproto::ActionTypeLogin), from_tm, to_tm); 

    } else {
        GEN_SQLSTR(this->sqlstr,
                " SELECT UNIX_TIMESTAMP(insert_time)"
                " FROM dplan_other_db.user_action_log_table_%u "
                " WHERE userid = %u AND type = %u "
                " AND UNIX_TIMESTAMP(insert_time) >= %u and UNIX_TIMESTAMP(insert_time) <= %u limit 100",
                date, userid, (uint32_t)(dbproto::ActionTypeLogin), from_tm, to_tm); 
    }

    std::vector<uint32_t> tm_vec_tmp;
    STD_QUERY_WHILE_BEGIN_NEW(this->sqlstr, tm_vec_tmp);
        uint32_t tm = 0;
        INT_CPY_NEXT_FIELD(tm);
        login_tm_vec.insert(tm);
    STD_QUERY_WHILE_END_NEW();
}

uint32_t UserActionLogTable::get_user_log_total(
		userid_t userid, uint32_t u_create_tm,
		uint32_t date, dbproto::sc_get_user_action_log* log_list)
{
	GEN_SQLSTR(this->sqlstr,
			" SELECT count(*) FROM dplan_other_db.user_action_log_table_%u "
			" WHERE userid = %u AND u_create_tm = %u ",
			date, userid, u_create_tm);	
    STD_QUERY_ONE_BEGIN(this->sqlstr, db_err_record_not_found)
		uint32_t total_count = 0;
		INT_CPY_NEXT_FIELD(total_count);log_list->set_log_list_count(total_count);
    STD_QUERY_ONE_END()
}

uint32_t UserActionLogTable::get_login_tm_info(userid_t userid, uint32_t u_create_tm, uint32_t from_tm, uint32_t to_tm,
        std::set<uint32_t> &login_tm_vec)
{
    if (from_tm > to_tm) {
        uint32_t tmp = to_tm;
        to_tm = from_tm;
        from_tm = tmp;
    }
    uint32_t now = time(0);
    if (from_tm > now) {
        from_tm = now;
    }
    if (to_tm > now) {
        to_tm = now;
    }
    uint32_t from_date = time_to_date(from_tm);
    uint32_t to_date = time_to_date(to_date);
    if (to_date - from_date >= 10) {
        to_date = from_date + 10;
    }
    for (uint32_t i = from_date; i <= to_date; i++) {
        get_login_tm_info_ex(userid, u_create_tm, i, from_tm, to_tm, login_tm_vec);
    }
    return 0;
}
