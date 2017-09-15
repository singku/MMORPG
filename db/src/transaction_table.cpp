#include "transaction_table.h"
#include "proto/db/db_errno.h"

TransactionTable::TransactionTable(mysql_interface* db) 
    : Ctable(db, "dplan_other_db", "transaction_table")
{
}

uint64_t TransactionTable::if_transaction_exist(uint32_t channel_id, uint32_t pay_gate_trans_id)
{
    GEN_SQLSTR(this->sqlstr, "select transaction_id from %s where channel_id = %u and "
            " pay_gate_trans_id = %u limit 1", get_table_name(), 
            channel_id, pay_gate_trans_id);
    int ret;
    uint64_t tran_id = 0;
    MYSQL_RES *res;
    MYSQL_ROW row;
    int rowcount;
    this->db->id=this->id;
    ret =this->db->exec_query_sql(sqlstr,&res);
    if (ret==DB_SUCC){
        rowcount=mysql_num_rows(res);
        if (rowcount!=1) {
            mysql_free_result(res);
            return 0;
        }else {
            row= mysql_fetch_row(res);
            tran_id = atoi_safe(row[0]);
            mysql_free_result(res);
            return tran_id;
        }
    }else {
        return 0;
    }
}

int32_t TransactionTable::new_transaction_rd(userid_t userid, const dbproto::transaction_info_t &inf, 
                            uint64_t &auto_incr_id, bool &is_duplicate_trans)
{
    is_duplicate_trans = false;
    if (inf.channel_id() == dbproto::CHANNEL_TYPE_DEPOSIT) { //货币充值
        uint64_t trans_id = this->if_transaction_exist(inf.channel_id(), inf.pay_gate_trans_id());
        if (trans_id != 0) { //已经存在
            auto_incr_id = trans_id;
            is_duplicate_trans = true;
            return 0;
        }
    }

    GEN_SQLSTR(this->sqlstr, "insert into %s (transaction_time, server_no, account_id, s_create_tm, "
                            " dest_account_id, d_create_tm, channel_id, pay_gate_trans_id, product_id, product_type, "
                            " product_duration, product_count, money_num, result, server_num) values "
                            " (FROM_UNIXTIME(%u), %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %d, %u, %u) ",
                            get_table_name(), 
                            inf.transaction_time(),
                            inf.server_no(),
                            inf.account_id(),
                            inf.s_create_tm(),
                            inf.dest_account_id(),
                            inf.d_create_tm(),
                            inf.channel_id(),
                            inf.pay_gate_trans_id(),
                            inf.product_id(),
                            inf.product_type(),
                            inf.product_duration(),
                            inf.product_count(),
                            inf.money_num(),
                            inf.result(),
                            inf.server_no());
    return this->exec_insert_sql_get_auto_increment_id(this->sqlstr, 0, &auto_incr_id);
}

uint32_t TransactionTable::get_transaction_list(userid_t userid, uint32_t u_create_tm,
        uint32_t start_time, uint32_t end_time, dbproto::transaction_list_t* list)
{
    GEN_SQLSTR(this->sqlstr, 
            " SELECT UNIX_TIMESTAMP(transaction_time), server_no, account_id, s_create_tm, "
            " dest_account_id, d_create_tm, channel_id, pay_gate_trans_id, "
            " product_id, product_type, product_duration, "
            " product_count, money_num, result "
            " FROM %s "
            " WHERE account_id = %u and s_create_tm = %u "
            " AND transaction_time >= FROM_UNIXTIME(%u) "
            " AND transaction_time <= FROM_UNIXTIME(%u) ",
            this->get_table_name(), userid, u_create_tm,
            start_time, end_time);

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, list, trans_list);
        PB_INT_CPY_NEXT_FIELD(item, transaction_time);
        PB_INT_CPY_NEXT_FIELD(item, server_no);
        PB_INT_CPY_NEXT_FIELD(item, account_id);
        PB_INT_CPY_NEXT_FIELD(item, s_create_tm);
        PB_INT_CPY_NEXT_FIELD(item, dest_account_id);
        PB_INT_CPY_NEXT_FIELD(item, d_create_tm);
        item.set_channel_id((dbproto::channel_type_t)(atoi_safe(NEXT_FIELD)));
        PB_INT_CPY_NEXT_FIELD(item, pay_gate_trans_id);
        PB_INT_CPY_NEXT_FIELD(item, product_id);
        item.set_product_type((dbproto::product_type_t)(atoi_safe(NEXT_FIELD)));
        PB_INT_CPY_NEXT_FIELD(item, product_duration);
        PB_INT_CPY_NEXT_FIELD(item, product_count);
        PB_INT_CPY_NEXT_FIELD(item, money_num);
        PB_INT_CPY_NEXT_FIELD(item, result);
	PB_STD_QUERY_WHILE_END();
    return 0;
}

uint32_t TransactionTable::get_buy_pd_trans_list(userid_t userid, uint32_t u_create_tm,
		uint32_t start_time, uint32_t end_time, uint32_t product_id,
		dbproto::transaction_list_t* list)
{
    GEN_SQLSTR(this->sqlstr, 
            " SELECT UNIX_TIMESTAMP(transaction_time), server_no, account_id, s_create_tm, "
            " dest_account_id, d_create_tm, channel_id, pay_gate_trans_id, "
            " product_id, product_type, product_duration, "
            " product_count, money_num, result "
            " FROM %s "
            " WHERE account_id = %u and s_create_tm = %u "
            " AND transaction_time >= FROM_UNIXTIME(%u) "
            " AND transaction_time <= FROM_UNIXTIME(%u) "
			" AND product_id = %u ",
            this->get_table_name(), userid, u_create_tm,
            start_time, end_time, product_id);

    PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, list, trans_list);
        PB_INT_CPY_NEXT_FIELD(item, transaction_time);
        PB_INT_CPY_NEXT_FIELD(item, server_no);
        PB_INT_CPY_NEXT_FIELD(item, account_id);
        PB_INT_CPY_NEXT_FIELD(item, s_create_tm);
        PB_INT_CPY_NEXT_FIELD(item, dest_account_id);
        PB_INT_CPY_NEXT_FIELD(item, d_create_tm);
        item.set_channel_id((dbproto::channel_type_t)(atoi_safe(NEXT_FIELD)));
        PB_INT_CPY_NEXT_FIELD(item, pay_gate_trans_id);
        PB_INT_CPY_NEXT_FIELD(item, product_id);
        item.set_product_type((dbproto::product_type_t)(atoi_safe(NEXT_FIELD)));
        PB_INT_CPY_NEXT_FIELD(item, product_duration);
        PB_INT_CPY_NEXT_FIELD(item, product_count);
        PB_INT_CPY_NEXT_FIELD(item, money_num);
        PB_INT_CPY_NEXT_FIELD(item, result);
    PB_STD_QUERY_WHILE_END();
}
