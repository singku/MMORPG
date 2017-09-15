#include	<dbser/mysql_iface.h>
#include	"sql_utils.h"
#include	"rawdata_table.h"

RawDataTable::RawDataTable(mysql_interface* db) 
    : CtableRoute(db, "dplan_db", "buff_table", "userid")
{

}

/*
int RawDataTable::update_buff(
        const dbproto::buff_table_t &info)
{
    std::string sql_str;
    SQLUtils::gen_replace_sql_from_proto_any_key(
            this->get_table_name(info.userid()),
            info, sql_str, &(this->db->handle)); 

    if (sql_str.size() == 0) {
        return db_err_proto_format_err;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}
*/

int RawDataTable::update_raw_data(
		const dbproto::raw_data_table_t &info)
{
	char safe_buff[mysql_str_len(BUFF_SIZE_MAX)] = { 0 };
	const std::string buff_str = info.rawdata();
	set_mysql_string(safe_buff, buff_str.c_str(), buff_str.size());

	char safe_id_buff[mysql_str_len(BUFF_SIZE_MAX)] = { 0 };
	const std::string buff_id_str = info.rawdata_id();
	set_mysql_string(safe_id_buff, buff_id_str.c_str(), buff_id_str.size());
	GEN_SQLSTR(this->sqlstr,
				" INSERT INTO %s "
				" (userid, u_create_tm, buff_type, buff_id, buff_data) "
				" VALUES (%u, %u, %u, '%s', '%s') "
				" ON DUPLICATE KEY UPDATE "
				" buff_data='%s'", 
				this->get_table_name(info.userid()),
				info.userid(), info.u_create_tm(), 
                info.rawdata_type(), safe_id_buff, safe_buff,
				safe_buff);
	return this->exec_insert_sql(this->sqlstr, KEY_EXISTED_ERR);
}

int RawDataTable::get_user_raw_data(uint32_t userid, uint32_t u_create_tm, 
        uint32_t type, char raw_data[], uint32_t& length, std::string buff_id)
{
    // 调用之前保证type合法
    memset(raw_data, 0, BUFF_SIZE_MAX);
    GEN_SQLSTR(this->sqlstr, "select buff_data from %s where userid=%u and u_create_tm = %u"
            " and buff_type = %u and buff_id='%s' ",
            this->get_table_name(userid), userid, u_create_tm, type, buff_id.c_str());  

    STD_QUERY_ONE_BEGIN(this->sqlstr, db_err_record_not_found);
        mysql_fetch_lengths(res);
        length = res->lengths[0];
        res->lengths[0] < BUFF_SIZE_MAX? \
        memcpy(raw_data, row[0], res->lengths[0]): memcpy(raw_data, row[0], BUFF_SIZE_MAX);
    STD_QUERY_ONE_END()

    return 0;
}

int RawDataTable::get_raw_data_list(
        uint32_t userid, uint32_t u_create_tm, uint32_t type, std::vector<std::string> &msgs)
{
    GEN_SQLSTR(this->sqlstr, "select buff_data from %s where userid=%u and u_create_tm = %u and buff_type = %u",
            this->get_table_name(userid), userid, u_create_tm, type);  

    MYSQL_RES* res = NULL;
    MYSQL_ROW row;

    int ret = this->db->exec_query_sql(sqlstr, &res);
    if (ret) {
        return ret;
    }

    std::string tmp_str;
    while  ((row = mysql_fetch_row(res)) != NULL) {
        mysql_fetch_lengths(res);
        tmp_str = std::string(row[0], res->lengths[0]);
        msgs.push_back(tmp_str);
    }
    mysql_free_result(res);
    return 0;
}

int RawDataTable::del_raw_data(
        uint32_t userid, uint32_t u_create_tm, uint32_t type, const std::string &buff_id)
{
    GEN_SQLSTR(this->sqlstr,
            "delete from %s where userid = %u and u_create_tm = %u and buff_type = %u and buff_id='%s'",
            this->get_table_name(userid), userid, u_create_tm, type, buff_id.c_str());

    int affected_rows = 0;
    return this->db->exec_update_sql(this->sqlstr, &affected_rows);
}

void get_shop_info(uint32_t userid, uint32_t u_create_tm, uint32_t type, commonproto::market_item_info_t *pb_shop_inf)
{
    pb_shop_inf->Clear();
    std::vector<std::string> inf_vec;
    int ret = g_raw_data_table->get_raw_data_list(userid, u_create_tm, type, inf_vec);
    if (ret) {
        return;
    }
    if (inf_vec.size() == 0) {
        return;
    }
    pb_shop_inf->ParseFromString(inf_vec[0]);
}

