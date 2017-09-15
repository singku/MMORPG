#ifndef BUFF_TABLE_H
#define BUFF_TABLE_H

extern "C" {
#include <libtaomee/project/types.h>
}
#include	"proto/client/attr_type.h"
#include	"proto/db/db_errno.h"
#include	<dbser/CtableRoute10x10.h>
#include	"proto/db/dbproto.raw_data.pb.h"
#include	"proto/db/dbproto.table.pb.h"
#include	"sql_utils.h"
#include	"macro_utils.h"
#include	"utils.h"

#define BUFF_SIZE_MAX (4096)               //buff的最大size


class RawDataTable : public CtableRoute
{
public:
    RawDataTable(mysql_interface* db);
	int get_user_raw_data(
			uint32_t userid, uint32_t u_create_tm, uint32_t type,
			char buff[], uint32_t& length, 
			std::string buff_id="0");

    int get_raw_data_list(
            uint32_t userid, uint32_t u_create_tm, uint32_t type, 
            std::vector<std::string> &msgs);

    int update_raw_data(
            const dbproto::raw_data_table_t &info);

    int del_raw_data(
        uint32_t userid, uint32_t u_create_tm, uint32_t type, const std::string &buff_id);
};

extern RawDataTable* g_raw_data_table;

void get_shop_info(uint32_t userid, uint32_t u_create_tm, uint32_t type, commonproto::market_item_info_t *pb_shop_inf);

#endif //BUFF_TABLE_H
        
