#ifndef GIFT_CODE_TABLE_H
#define GIFT_CODE_TABLE_H

#include <set>
extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/Ctable.h>
#include "proto/db/dbproto.gift_code.pb.h"


class GiftCodeTable : public Ctable
{
public:
    GiftCodeTable(mysql_interface* db);
    //尝试使用兑换码
    //返回db_err_record_not_found表示没找到 返回0表示使用成功
    uint32_t use_gift_code(userid_t userid, uint32_t u_create_tm, uint32_t server_id, std::string code);
    //查询兑换码的信息
    uint32_t get_gift_code(std::string code, uint32_t &prize_id, uint32_t &status, uint32_t &userid);
};

extern GiftCodeTable *g_gift_code_table;

#endif
