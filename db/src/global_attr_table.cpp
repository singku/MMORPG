
#include "global_attr_table.h"
#include "proto/db/db_errno.h"

GlobalAttrTable::GlobalAttrTable(mysql_interface* db)
    : Ctable(db, "dplan_other_db", "global_attr_table")
{
}

int GlobalAttrTable::get_attr(
        const dbproto::global_attr_list_t& type_list,
        dbproto::global_attr_list_t* list,
        uint32_t server_id)
{
    int pos = 0;
    pos += snprintf(this->sqlstr + pos, sizeof(this->sqlstr) - pos, 
            " SELECT type, subtype, value FROM %s WHERE server_id = %u AND ( ", 
            this->get_table_name(), server_id);

    for (int i = 0; i < type_list.attr_list_size(); i++) {
        const dbproto::global_attr_info_t& attr_info = type_list.attr_list(i);

        if (i != 0) {
            pos += snprintf(this->sqlstr + pos, sizeof(this->sqlstr) - pos,
                    " OR "); 
        }

        pos += snprintf(this->sqlstr + pos, sizeof(this->sqlstr) - pos, 
                " (type = %u AND subtype = %u) ", 
                attr_info.type(), attr_info.subtype());         
    }
    pos += snprintf(this->sqlstr + pos, sizeof(this->sqlstr) - pos, " ) ");

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, list, attr_list);
        PB_INT_CPY_NEXT_FIELD(item, type);
        PB_INT_CPY_NEXT_FIELD(item, subtype);
        PB_INT_CPY_NEXT_FIELD(item, value);
    PB_STD_QUERY_WHILE_END();

    return 0;
}

int GlobalAttrTable::add_attr_with_limit(uint32_t type, uint32_t subtype, 
        uint32_t diff, uint32_t limit, dbproto::sc_update_global_attr &ret,
        uint32_t server_id)
{
	char proc_ret[50];
	strcpy(proc_ret,"@out_ret");
	this->db->select_db(this->db_name);
    GEN_SQLSTR(this->sqlstr, 
            " CALL proc_add_global_attr"
            "(%u, %u, %u, %u, %u, %s)",
             server_id, type, subtype, diff, limit, proc_ret);

    return exec_call_proc_sql(this->sqlstr, proc_ret, ret, DB_SUCC);
}

int GlobalAttrTable::exec_call_proc_sql(char * cmd, char * proc_ret, dbproto::sc_update_global_attr &ret, int nofind_err )
{
	int acount = -1; 
	int dbret = -1; 
	this->db->id=this->id;
	if ((dbret=this->db->exec_update_sql(cmd, &acount))==DB_SUCC)
	{
		char out_req[50];
		int out_ret  = 0;
		MYSQL_RES *results;
		MYSQL_ROW record;
		snprintf(out_req, sizeof(out_req),"select %s",proc_ret);
		if ((dbret=this->db->exec_query_sql(out_req, &results)) == DB_SUCC){
			while((record = mysql_fetch_row(results))){
				out_ret = atoi(record[0]);
			}
		}
		//获取返回结果
		ret.set_is_succ(out_ret);

		if(acount == 1 ){
			return DB_SUCC;	
		}else{
			return nofind_err; 
		}
	}else {
		return DB_ERR;
	} 
}	
int GlobalAttrTable::minus_attr(uint32_t type, uint32_t subtype, uint32_t diff, uint32_t server_id)
{
    GEN_SQLSTR(this->sqlstr, 
            " UPDATE %s "
            " SET value = value - %u "
            " WHERE type = %u AND subtype = %u AND value >= %u AND server_id = %u ",
            this->get_table_name(), 
            diff, type, subtype, diff, server_id);

    return exec_update_sql(this->sqlstr, db_err_global_attr_not_enough);
}

int GlobalAttrTable::set_attr(uint32_t type, uint32_t subtype, uint32_t new_value, uint32_t server_id)
{
    GEN_SQLSTR(this->sqlstr, 
            " REPLACE INTO %s "
            " (server_id, type, subtype, value) "
            " VALUES (%u, %u, %u, %u) ",
            this->get_table_name(), 
            server_id, type, subtype, new_value);

    return exec_update_sql(this->sqlstr, DB_SUCC);
}
