#include	<dbser/mysql_iface.h>
#include	<dbser/CtableRoute100x10.h>
#include	"proto/db/dbproto.family.pb.h"
#include	"proto/client/common.pb.h"
#include	"sql_utils.h"
#include	"family_event_table.h"

FamilyEventTable::FamilyEventTable(mysql_interface* db) 
    : CtableRoute100x10(db, "dplan_family_db", "family_event_table", "family_id")
{

}

int FamilyEventTable::update_family_event(
        const dbproto::family_event_table_t &info, uint32_t flag)
{
    uint32_t total_event_num = 0;
    this->get_total_event_num(info.family_id(), 0, 0, info.event_type(), total_event_num);
    if (total_event_num >= 200) {
        this->del_oldest_event(info.family_id(), info.event_type());
    }
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

int FamilyEventTable::change_family_event(
        const dbproto::family_event_table_change_data_t &info)
{
    std::string sql_str;
    SQLUtils::gen_change_sql_from_proto_any_key(
            this->get_table_name(info.family_id()),
            info, sql_str, &(this->db->handle)); 

    if (sql_str.size() == 0) {
        return db_err_proto_format_err;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}

int FamilyEventTable::del_oldest_event(uint32_t family_id, uint32_t type)
{
    GEN_SQLSTR(this->sqlstr,
            "delete from %s where family_id = %u and event_type = %u order by event_time asc limit 1",
            this->get_table_name(family_id), family_id, type);

    int affected_rows = 0;
    return this->db->exec_update_sql(this->sqlstr, &affected_rows);
}

int FamilyEventTable::del_event(
        uint32_t family_id, uint32_t userid, uint32_t u_create_tm, uint32_t type)
{
    if (userid == 0) {
        GEN_SQLSTR(this->sqlstr,
                "delete from %s where family_id = %u",
                this->get_table_name(family_id), family_id);
    } else if (userid > 0 && type == 0){
        GEN_SQLSTR(this->sqlstr,
                "delete from %s where family_id = %u and userid = %u and u_create_tm = %u ",
                this->get_table_name(family_id), family_id, userid, u_create_tm);
    } else {
        GEN_SQLSTR(this->sqlstr,
                "delete from %s where family_id = %u and userid = %u and u_create_tm = %u and event_type = %u ",
                this->get_table_name(family_id), family_id, userid, u_create_tm, type);
    }

    int affected_rows = 0;
    return this->db->exec_update_sql(this->sqlstr, &affected_rows);
}

int FamilyEventTable::get_event_info(
        uint32_t family_id, uint32_t userid, uint32_t u_create_tm,
        uint32_t type, commonproto::family_event_t &event)
{
    GEN_SQLSTR(this->sqlstr,
            "select event_status, event_time "
            "from %s where family_id = %u and userid = %u and u_create_tm = %u and event_type = %u",
            this->get_table_name(family_id), family_id, userid, u_create_tm, type);

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);

	if (ret) {
		return ret;
	}

    if  ((row = mysql_fetch_row(res)) != NULL) {
        event.set_event_status(atoi_safe(row[0]));
        event.set_event_time(atoi_safe(row[1]));
        event.set_family_id(family_id);
        event.mutable_role()->set_userid(userid);
        event.mutable_role()->set_u_create_tm(u_create_tm);
        event.set_event_type(type);
    }
	mysql_free_result(res);
    return 0;
}

int FamilyEventTable::get_event_list(
        uint32_t family_id, uint32_t userid, uint32_t u_create_tm,
        uint32_t type, uint32_t start , uint32_t page_size,
        dbproto::sc_family_get_event_list *event_list, uint32_t &num)
{
    if (userid == 0) {
        // 拉取所有玩家的特定类型事件
        GEN_SQLSTR(this->sqlstr,
            "select userid, u_create_tm, event_status, event_time "
            "from %s where family_id = %u and event_type = %u "
            "limit %u, %u",
            this->get_table_name(family_id), family_id, type, start, page_size);
    } else {
        // 拉取指定玩家的特定类型事件
        GEN_SQLSTR(this->sqlstr,
            "select userid, u_create_tm, event_status, event_time "
            "from %s where family_id = %u and userid = %u and u_create_tm = %u and event_type = %u "
            "limit %u, %u",
            this->get_table_name(family_id), family_id, userid, u_create_tm, type, start, page_size);
    }

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);
	if (ret) {
		return ret;
	}

    num = 0;
    while  ((row = mysql_fetch_row(res)) != NULL) {
        commonproto::family_event_t *info = event_list->add_event_infos();
		info->set_family_id(family_id);
		info->mutable_role()->set_userid(atoi_safe(row[0]));
        info->mutable_role()->set_u_create_tm(atoi_safe(row[1]));
		info->set_event_status(atoi_safe(row[2]));
		info->set_event_time(atoi_safe(row[3]));
		info->set_event_type(type);
        num++;
	}
	mysql_free_result(res);
    return 0;
}

int FamilyEventTable::get_total_event_num(
        uint32_t family_id, uint32_t userid, uint32_t u_create_tm,
        uint32_t type, uint32_t &total_event_num)
{
    if (userid > 0) {
        // 拉取指定玩家的特定类型事件总数
        GEN_SQLSTR(this->sqlstr,
                "select count(userid) from %s "
                "where family_id = %u and userid = %u and u_create_tm = %u and event_type = %u ",
                this->get_table_name(family_id), family_id, userid, u_create_tm, type);
    } else {
        // 拉取所有玩家的特定类型事件总数
        GEN_SQLSTR(this->sqlstr,
                "select count(userid) from %s "
                "where family_id = %u and event_type = %u",
                this->get_table_name(family_id), family_id, type);
    } 

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);

	if (ret) {
		return ret;
	}

    if  ((row = mysql_fetch_row(res)) != NULL) {
		total_event_num = atoi_safe(row[0]);
	}
	mysql_free_result(res);
    return 0;
}
