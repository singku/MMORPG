#ifndef FAMILY_MEMBER_TABLE
#define FAMILY_MEMBER_TABLE

extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute100x10.h>
#include "proto/db/dbproto.family.pb.h"
#include "proto/client/common.pb.h"
#include "proto/common/svr_proto_header.h"

class FamilyMemberTable : public CtableRoute100x10
{
public:
    FamilyMemberTable(mysql_interface* db);

    int del_member(uint32_t family_id, uint32_t userid, uint32_t u_create_tm);
    int get_member_info(
            uint32_t family_id, uint32_t userid, uint32_t u_create_tm,
            commonproto::family_member_info_t &family_member_info);
    int get_member_info_by_title(
        uint32_t family_id, uint32_t title, 
        std::vector<commonproto::family_member_info_t> &member_info);

    int get_member_list(
        uint32_t family_id, uint32_t flag, uint32_t start , uint32_t page_size,
        dbproto::sc_family_get_member_list *member_list, uint32_t &num);

    int get_total_member_num(
        uint32_t family_id, uint32_t flag, uint32_t &total_member_num);

    int get_total_battle_value(
            uint32_t family_id, uint32_t &total_battle_value);


    int update_member_info(
            const dbproto::family_member_table_t &info, uint32_t flag);
    int change_family_member_info(
        const dbproto::family_member_change_data_t &info);

    int get_members(
        uint32_t family_id, uint32_t title, std::vector<role_info_t> &members);

    int get_top_construct_value_member(
        uint32_t family_id, uint32_t title, 
        role_info_t &leader, role_info_t &user);
};

extern FamilyMemberTable* g_family_member_table;

#endif
