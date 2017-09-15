
#ifndef BASE_INFO_TABLE_H
#define BASE_INFO_TABLE_H

extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute10x10.h>
#include "proto/db/dbproto.base_info.pb.h"
#include "proto/db/dbproto.login.pb.h"
#include "proto/client/common.pb.h"

class BaseInfoTable : public CtableRoute {
public:
    BaseInfoTable(mysql_interface* db);

    uint32_t create_role(userid_t userid, const dbproto::cs_create_role& cs_create_role, uint32_t &u_create_tm);

    uint32_t user_exists(userid_t userid, uint32_t u_create_tm, uint32_t server_id, bool is_init_server);

    uint32_t get_info(userid_t userid, 
            commonproto::player_base_info_t& base_info, uint32_t u_create_tm);

    uint32_t get_infos(userid_t userid, uint32_t server_id,
            google::protobuf::RepeatedPtrField<commonproto::player_base_info_t> *base_infos);
   
    uint32_t change_nick(userid_t userid, std::string new_nick, uint32_t u_create_tm);
};

extern BaseInfoTable* g_base_info_table;

#endif

