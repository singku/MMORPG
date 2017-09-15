#ifndef FRIEND_TABLE_H
#define FRIEND_TABLE_H 
extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute10x10.h>
#include "proto/db/dbproto.login.pb.h"
#include "proto/db/dbproto.task.pb.h"
#include <vector>

class FriendTable : public CtableRoute
{
public:
    FriendTable(mysql_interface* db);

    int get_friends(userid_t userid, uint32_t u_create_tm, 
             dbproto::sc_get_login_info& login_info);
	int save_friend(const commonproto::friend_data_t &finf);
	int remove_friend(const commonproto::friend_data_t &finf);
//	int update_gift_count(uint32_t userid, uint32_t friendid, uint32_t gift_count);
};

extern FriendTable* g_friend_table;

#endif
