#ifndef NICK_TABLE_h
#define NICK_TABLE_h

#include <dbser/CtableRoute100.h>
extern "C" {
#include <libtaomee/project/types.h>
}
#include "proto/common/svr_proto_header.h"

class NickTable : public CtableRoute100
{
public:
    NickTable(mysql_interface* db);

    uint32_t get_user_by_nick(const std::string& nick, role_info_t &user);  

    uint32_t insert_nick_and_user(const std::string& nick, role_info_t &user);

    uint32_t delete_nick_and_user(const std::string& nick);
private:
    uint32_t get_hash(const std::string& s);

    
    uint32_t get_table_no(const std::string& s);
};


extern NickTable* g_nick_table;
#endif
