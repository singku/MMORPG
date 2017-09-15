#ifndef TRANSACTION_TABLE_H
#define TRANSACTION_TABLE_H

#include <set>
extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/Ctable.h>
#include "proto/db/dbproto.transaction.pb.h"


class TransactionTable : public Ctable
{
public:
    TransactionTable(mysql_interface* db);

    //交易记录是否存在 存在则返回transaction_id 不存在则返回0
    uint64_t if_transaction_exist(uint32_t channel_id, uint32_t pay_gate_trans_id);
    //新增一条交易记录 返回非0表示失败。
    int32_t new_transaction_rd(userid_t userid, const dbproto::transaction_info_t &inf, 
            uint64_t &auto_incr_id, bool &is_duplicate_trans);

    uint32_t get_transaction_list(userid_t userid, uint32_t u_create_tm,
            uint32_t start_time, uint32_t end_time, dbproto::transaction_list_t* list);

	uint32_t get_buy_pd_trans_list(userid_t userid, uint32_t u_create_tm,
			uint32_t start_time, uint32_t end_time, uint32_t product_id,
			dbproto::transaction_list_t* list);
};

extern TransactionTable* g_transaction_table;
 
#endif
