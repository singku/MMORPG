#include "family_member_table.h"
#include "proto/client/attr_type.h"
#include "proto/db/db_errno.h"
#include "sql_utils.h"
#include "macro_utils.h"
#include "utils.h"

FamilyMemberTable::FamilyMemberTable(mysql_interface* db) 
    : CtableRoute100x10(
            db, "dplan_family_db", "family_member_table", "family_id")
{

}

int FamilyMemberTable::update_member_info(const dbproto::family_member_table_t &info, uint32_t flag)
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
        return db_err_gen_sql_from_proto_failed;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}

int FamilyMemberTable::change_family_member_info(
        const dbproto::family_member_change_data_t &info)
{
    std::string sql_str;
    SQLUtils::gen_change_sql_from_proto_any_key(
            this->get_table_name(info.family_id()),
            info, sql_str, &(this->db->handle)); 

    if (sql_str.size() == 0) {
        return db_err_gen_sql_from_proto_failed;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}

int FamilyMemberTable::get_member_info(
        uint32_t family_id, uint32_t userid, uint32_t u_create_tm,
        commonproto::family_member_info_t &family_member_info)
{
    GEN_SQLSTR(this->sqlstr,
            "select join_time, battle_value, left_construct_value,"
            "total_construct_value, title, last_login_time, last_logout_time "
            "from %s where family_id = %u and userid = %u and u_create_tm = %u",
            this->get_table_name(family_id), family_id, userid, u_create_tm);

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);

	if (ret) {
		return ret;
	}

    if  ((row = mysql_fetch_row(res)) != NULL) {
		family_member_info.set_join_time(atoi_safe(row[0]));
		family_member_info.set_battle_value(atoi_safe(row[1]));
		family_member_info.set_left_construct_value(atoi_safe(row[2]));
		family_member_info.set_total_construct_value(atoi_safe(row[3]));
		family_member_info.set_title(atoi_safe(row[4]));
		family_member_info.set_last_login_time(atoi_safe(row[5]));
        uint32_t last_login_time = atoi_safe(row[5]);
        uint32_t last_logout_time = atoi_safe(row[6]);
        bool is_online = (last_logout_time < last_login_time)? true : false;
		family_member_info.set_is_online(is_online);
        family_member_info.set_userid(userid);
        family_member_info.set_u_create_tm(u_create_tm);

	}
	mysql_free_result(res);
    return 0;
}

int FamilyMemberTable::get_member_info_by_title(
        uint32_t family_id, uint32_t title, 
        std::vector<commonproto::family_member_info_t> &member_info)
{
    GEN_SQLSTR(this->sqlstr,
            "select join_time, battle_value, left_construct_value,"
            "total_construct_value, userid, u_create_tm, last_login_time, last_logout_time "
            "from %s where family_id = %u and title = %u",
            this->get_table_name(family_id), family_id, title);

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);

	if (ret) {
		return ret;
	}

    while  ((row = mysql_fetch_row(res)) != NULL) {
        commonproto::family_member_info_t family_member_info;
		family_member_info.set_join_time(atoi_safe(row[0]));
		family_member_info.set_battle_value(atoi_safe(row[1]));
		family_member_info.set_left_construct_value(atoi_safe(row[2]));
		family_member_info.set_total_construct_value(atoi_safe(row[3]));
		family_member_info.set_userid(atoi_safe(row[4]));
		family_member_info.set_u_create_tm(atoi_safe(row[5]));
        uint32_t last_login_time = atoi_safe(row[6]);
        uint32_t last_logout_time = atoi_safe(row[7]);
		family_member_info.set_last_login_time(last_login_time);
        bool is_online = (last_logout_time < last_login_time)? true : false;
		family_member_info.set_is_online(is_online);
        member_info.push_back(family_member_info);
	}
	mysql_free_result(res);
    return 0;
}

int FamilyMemberTable::get_member_list(
        uint32_t family_id, uint32_t flag, uint32_t start , uint32_t page_size,
        dbproto::sc_family_get_member_list *member_list, uint32_t &num)
{
    if (flag == commonproto::FAMILY_MEMBER_LIST_ONLINE) {
        // 拉取在线玩家
        GEN_SQLSTR(this->sqlstr,
            "select userid, u_create_tm, join_time, battle_value, left_construct_value,"
            "total_construct_value, title, last_login_time, last_logout_time "
            "from %s where family_id = %u and last_login_time > last_logout_time "
            "order by battle_value desc limit %u, %u",
            this->get_table_name(family_id), family_id, start, page_size);
    } else {
        // 拉取所有玩家
        GEN_SQLSTR(this->sqlstr,
            "select userid, u_create_tm, join_time, battle_value, left_construct_value,"
            "total_construct_value, title, last_login_time, last_logout_time "
            "from %s where family_id = %u order by battle_value desc limit %u, %u",
            this->get_table_name(family_id), family_id, start, page_size);
    }

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);
	if (ret) {
		return ret;
	}

    num = 0;
    while  ((row = mysql_fetch_row(res)) != NULL) {
        commonproto::family_member_info_t *info = member_list->mutable_list_out()->add_member_info();
		info->set_userid(atoi_safe(row[0]));
		info->set_u_create_tm(atoi_safe(row[1]));
		info->set_join_time(atoi_safe(row[2]));
		info->set_battle_value(atoi_safe(row[3]));
		info->set_left_construct_value(atoi_safe(row[4]));
		info->set_total_construct_value(atoi_safe(row[5]));
		info->set_title(atoi_safe(row[6]));
        uint32_t last_login_time = atoi_safe(row[7]);
        uint32_t last_logout_time = atoi_safe(row[8]);
		info->set_last_login_time(last_login_time);
        bool is_online = (last_logout_time < last_login_time)? true : false;
		info->set_is_online(is_online);
        num++;
	}
	mysql_free_result(res);
    return 0;
}

int FamilyMemberTable::del_member(uint32_t family_id, uint32_t userid, uint32_t u_create_tm)
{
    if (userid > 0) {
        GEN_SQLSTR(this->sqlstr,
                "delete from %s where family_id = %u and userid = %u and u_create_tm = %u ",
                this->get_table_name(family_id), family_id, userid, u_create_tm);
    } else {
        GEN_SQLSTR(this->sqlstr,
                "delete from %s where family_id = %u",
                this->get_table_name(family_id), family_id);
    }

    int affected_rows = 0;
    return this->db->exec_update_sql(this->sqlstr, &affected_rows);
}

int FamilyMemberTable::get_total_battle_value(
        uint32_t family_id, uint32_t &total_battle_value)
{
    GEN_SQLSTR(this->sqlstr,
            "select sum(battle_value) from %s "
            "where family_id = %u",
            this->get_table_name(family_id), family_id);

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);

	if (ret) {
		return ret;
	}

    if  ((row = mysql_fetch_row(res)) != NULL) {
		total_battle_value = atoi_safe(row[0]);
	}
	mysql_free_result(res);
    return 0;
}

int FamilyMemberTable::get_total_member_num(
        uint32_t family_id, uint32_t flag, uint32_t &total_member_num)
{
    if (flag == commonproto::FAMILY_MEMBER_LIST_ONLINE) {
        // 拉取在线玩家数量
        GEN_SQLSTR(this->sqlstr,
                "select count(userid) from %s "
                "where family_id = %u and last_login_time > last_logout_time ",
                this->get_table_name(family_id), family_id);
    } else if (flag == commonproto::FAMILY_MEMBER_LIST_ALL) {
        // 拉取家族玩家总数
        GEN_SQLSTR(this->sqlstr,
                "select count(userid) from %s "
                "where family_id = %u",
                this->get_table_name(family_id), family_id);
    } else if (flag == commonproto::FAMILY_MEMBER_LIST_VICE_LEADER) {
        // 拉取家族副族长数
        GEN_SQLSTR(this->sqlstr,
                "select count(userid) from %s "
                "where family_id = %u and title = %u",
                this->get_table_name(
                    family_id), family_id, commonproto::FAMILY_TITLE_VICE_LEADER);
    }

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);

	if (ret) {
		return ret;
	}

    if  ((row = mysql_fetch_row(res)) != NULL) {
		total_member_num = atoi_safe(row[0]);
	}
	mysql_free_result(res);
    return 0;
}

int FamilyMemberTable::get_members(
    uint32_t family_id, uint32_t title, std::vector<role_info_t> &members)
{
    if (title == 0) {
        // 拉取所有玩家id
        GEN_SQLSTR(this->sqlstr,
            "select userid, u_create_tm from %s where family_id = %u",
            this->get_table_name(family_id), family_id);
    } else {
        // 拉取特定职位的玩家id
        GEN_SQLSTR(this->sqlstr,
            "select userid, u_create_tm from %s where family_id = %u and title = %u",
            this->get_table_name(family_id), family_id, title);
    }

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);
	if (ret) {
		return ret;
	}

    while  ((row = mysql_fetch_row(res)) != NULL) {
        role_info_t tmp;
        tmp.userid = atoi_safe(row[0]);
        tmp.u_create_tm = atoi_safe(row[1]);
        members.push_back(tmp);
	}
	mysql_free_result(res);
    return 0;
}

int FamilyMemberTable::get_top_construct_value_member(
        uint32_t family_id, uint32_t title, 
        role_info_t &leader, role_info_t &user)
{
    if (title == 0) {
        // 拉取所有玩家id
        GEN_SQLSTR(this->sqlstr,
            "select userid, u_create_tm from %s where family_id = %u and userid != %u and u_create_tm != %u "
            "order by total_construct_value desc limit 1",
            this->get_table_name(family_id), family_id, leader.userid, leader.u_create_tm);
    } else {
        // 拉取特定职位的玩家id
        GEN_SQLSTR(this->sqlstr,
            "select userid, u_create_tm from %s where family_id = %u and title = %u "
            "and userid != %u and u_create_tm != %u "
            "order by total_construct_value desc limit 1" ,
            this->get_table_name(family_id), family_id, title, leader.userid, leader.u_create_tm);
    }

    MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	int ret = this->db->exec_query_sql(sqlstr, &res);
	if (ret) {
		return ret;
	}

    if  ((row = mysql_fetch_row(res)) != NULL) {
        user.userid = atoi_safe(row[0]);
        user.u_create_tm = atoi_safe(row[1]);
	}
	mysql_free_result(res);
    return 0;
}
