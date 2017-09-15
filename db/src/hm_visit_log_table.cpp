#include "hm_visit_log_table.h"
#include "utils.h"
#include "sql_utils.h"

HmVisitLogTable::HmVisitLogTable(mysql_interface* db) 
	: CtableRoute(db, "dplan_db", "home_visit_log_table", "userid")
{}

uint32_t HmVisitLogTable::get_visit_log(userid_t userid, uint32_t u_create_tm, 
        commonproto::visit_log_list_t *list)
{
    GEN_SQLSTR(this->sqlstr,
            " SELECT userid, u_create_tm, guestid, g_create_tm, guestname, UNIX_TIMESTAMP(date), "
			" action_type, detail_info, gift_id, guestsex "
            " FROM %s WHERE userid = %u and u_create_tm = %u and action_type < 6 order by date desc limit 100",
            get_table_name(userid), userid, u_create_tm);
    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, list, visit)
        PB_INT_CPY_NEXT_FIELD(item, hostid);
        PB_INT_CPY_NEXT_FIELD(item, h_create_tm);
        PB_INT_CPY_NEXT_FIELD(item, guestid);
        PB_INT_CPY_NEXT_FIELD(item, g_create_tm);
        PB_BIN_CPY_NEXT_FIELD(item, guestname);
        PB_INT_CPY_NEXT_FIELD(item, date);
		uint32_t log_type;
		INT_CPY_NEXT_FIELD(log_type);
		item.set_logtype((commonproto::log_type_t)log_type);
        PB_BIN_CPY_NEXT_FIELD(item, detail);
        PB_INT_CPY_NEXT_FIELD(item, gift_id);
        PB_INT_CPY_NEXT_FIELD(item, sex);
    PB_STD_QUERY_WHILE_END()
}

uint32_t HmVisitLogTable::get_hm_gift_log(userid_t userid, uint32_t u_create_tm, 
        commonproto::visit_log_list_t *list)
{
	GEN_SQLSTR(this->sqlstr,
			" SELECT userid, u_create_tm, guestid, g_create_tm, guestname, UNIX_TIMESTAMP(date), "
			" action_type, detail_info, gift_id"
			" FROM %s WHERE userid = %u AND u_create_tm = %u AND action_type = 6 order by date desc" ,
			get_table_name(userid), userid, u_create_tm);
    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, list, visit)
        PB_INT_CPY_NEXT_FIELD(item, hostid);
        PB_INT_CPY_NEXT_FIELD(item, h_create_tm);
        PB_INT_CPY_NEXT_FIELD(item, guestid);
        PB_INT_CPY_NEXT_FIELD(item, g_create_tm);
        PB_BIN_CPY_NEXT_FIELD(item, guestname);
        PB_INT_CPY_NEXT_FIELD(item, date);
        //PB_INT_CPY_NEXT_FIELD(item, logtype);
		uint32_t log_type;
		INT_CPY_NEXT_FIELD(log_type);
		item.set_logtype((commonproto::log_type_t)log_type);
        PB_BIN_CPY_NEXT_FIELD(item, detail);
        PB_INT_CPY_NEXT_FIELD(item, gift_id);
    PB_STD_QUERY_WHILE_END()
}


uint32_t HmVisitLogTable::add_visit_log(userid_t userid,
		const commonproto::visit_log_info_t& log_info)
{
	uint32_t sql_len = sprintf(this->sqlstr, 
            "insert into %s (userid, u_create_tm, guestid, g_create_tm, guestname, "
            " date, action_type, detail_info, gift_id, guestsex) values ",
            get_table_name(userid));

    char safe_nick[commonproto::USER_NICK_NAME_MAX_LEN * 2];
    size_t len = strlen(log_info.guestname().c_str());
    memset(safe_nick, 0, sizeof(safe_nick));
    size_t w_len = (len > commonproto::USER_NICK_NAME_MAX_LEN) ?commonproto::USER_NICK_NAME_MAX_LEN :len;
    set_mysql_string(safe_nick, log_info.guestname().c_str(), w_len);

    char safe_detail[commonproto::VISIT_LOG_DETAIL_MAX_LEN * 2];
    len = strlen(log_info.detail().c_str());
    memset(safe_detail, 0, sizeof(safe_detail));
    w_len = (len > commonproto::VISIT_LOG_DETAIL_MAX_LEN) ?commonproto::VISIT_LOG_DETAIL_MAX_LEN :len;
    set_mysql_string(safe_detail, log_info.detail().c_str(), w_len);

    sql_len += sprintf(this->sqlstr + sql_len, "(%u, %u, %u, %u, '%s', FROM_UNIXTIME(%u), %u, '%s', %u, %u)",
                        log_info.hostid(), 
                        log_info.h_create_tm(),
                        log_info.guestid(), 
                        log_info.g_create_tm(),
                        safe_nick,
                        log_info.date(),
                        log_info.logtype(),
                        safe_detail,
						log_info.gift_id(),
						log_info.sex());

    return exec_insert_sql(sqlstr, DB_SUCC);
}
