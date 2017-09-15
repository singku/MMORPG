#include	<dbser/mysql_iface.h>
#include	<dbser/CtableRoute.h>
#include	"proto/db/dbproto.family.pb.h"
#include	"proto/client/common.pb.h"
#include	"sql_utils.h"
#include	"family_log_table.h"

FamilyLogTable::FamilyLogTable(mysql_interface* db) 
    : CtableRoute(db, "dplan_family_db", "family_log_table", "family_id")
{

}

int FamilyLogTable::update_family_log(
        const dbproto::family_log_table_t &info, uint32_t flag)
{
    std::string sql_str;
    if (flag == (uint32_t)dbproto::DB_UPDATE_AND_INESRT) {
        SQLUtils::gen_replace_sql_from_proto_any_key(
                this->get_table_name(info.family_id()),
                info, sql_str, &(this->db->handle)); 
    } else {
        SQLUtils::gen_update_sql_from_proto_any_key(
                this->get_table_name(info.family_id()),
                info, sql_str, &(this->db->handle));
    }

    if (sql_str.size() == 0) {
        return db_err_proto_format_err;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}

int FamilyLogTable::get_log_list(
        uint32_t family_id, uint32_t type, 
        uint32_t start , uint32_t page_size,
        dbproto::sc_family_get_log_list *log_list, uint32_t &num)
{
    if (type == 0) {
        // 拉取所有家族日志
        GEN_SQLSTR(this->sqlstr,
            "select log_type, log_msg, log_time "
            "from %s where family_id = %u order by log_time desc "
            "limit %u, %u",
            this->get_table_name(family_id), family_id, start, page_size);
    } else {
        // 拉取指定类型的家族日志
        GEN_SQLSTR(this->sqlstr,
            "select log_type, log_msg, log_time "
            "from %s where family_id = %u and log_type = %u order by log_time desc "
            "limit %u, %u",
            this->get_table_name(family_id), family_id, type, start, page_size);
    }

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);
	if (ret) {
		return ret;
	}

    num = 0;
    while  ((row = mysql_fetch_row(res)) != NULL) {
        commonproto::family_log_t *info = log_list->add_log_infos();
		info->set_log_type(atoi_safe(row[0]));
		info->set_log_str(row[1]);
		info->set_log_time(atoi_safe(row[2]));
        num++;
	}
	mysql_free_result(res);
    return 0;
}

int FamilyLogTable::get_total_log_num(
        uint32_t family_id, uint32_t type, uint32_t &total_log_num)
{
    if (type == 0) {
        // 拉取所有家族日志总数
        GEN_SQLSTR(this->sqlstr,
                "select count(family_id) from %s "
                "where family_id = %u",
                this->get_table_name(family_id), family_id);
    } else {
        // 拉取特定类型的家族日志总数
        GEN_SQLSTR(this->sqlstr,
                "select count(family_id) from %s "
                "where family_id = %u and log_type = %u",
                this->get_table_name(family_id), family_id, type);
    } 

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);

	if (ret) {
		return ret;
	}

    if  ((row = mysql_fetch_row(res)) != NULL) {
		total_log_num = atoi_safe(row[0]);
	}
	mysql_free_result(res);
    return 0;
}

int FamilyLogTable::del_log(uint32_t family_id, uint32_t num)
{
    if (num == 0) {
        GEN_SQLSTR(this->sqlstr,
                "delete from %s where family_id = %u",
                this->get_table_name(family_id), family_id);
    } else {
        GEN_SQLSTR(this->sqlstr,
                "delete from %s where family_id = %u limit %u",
                this->get_table_name(family_id), family_id, num);
    }

    int affected_rows = 0;
    return this->db->exec_update_sql(this->sqlstr, &affected_rows);
}
