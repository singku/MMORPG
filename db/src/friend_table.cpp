#include "friend_table.h"
#include "utils.h"
#include "sql_utils.h"
#include "proto/db/db_errno.h"
#include "macro_utils.h"
#include <libtaomee++/utils/strings.hpp>
#include <boost/lexical_cast.hpp>
#include <libtaomee++/bitmanip/bitmanip.hpp>
#include <libtaomee/timer.h>

FriendTable::FriendTable(mysql_interface* db)
    : CtableRoute(db, "dplan_db", "friend_table", "userid")
{

}
int FriendTable::get_friends(userid_t userid, uint32_t u_create_tm,
		dbproto::sc_get_login_info& login_info)
{
	GEN_SQLSTR(this->sqlstr,
			" SELECT friendid, f_create_tm, is_friend, is_black, is_recent, is_temp "
			" FROM %s "
			" WHERE userid = %u AND u_create_tm = %u",
			this->get_table_name(userid), userid, u_create_tm);

	MYSQL_RES* res = NULL;
	MYSQL_ROW  row;
	int ret = this->db->exec_query_sql(sqlstr, &res);

	if (ret) {
		return ret; 
	}

	while ((row = mysql_fetch_row(res)) != NULL) {

		commonproto::friend_data_t* friend_data = login_info.mutable_friend_list()->add_friend_list();

		friend_data->set_userid(userid);
		friend_data->set_u_create_tm(u_create_tm);
		friend_data->set_friendid(atoi_safe(row[0]));
		friend_data->set_f_create_tm(atoi_safe(row[1]));
		friend_data->set_is_friend(atoi_safe(row[2]));
		friend_data->set_is_blacklist(atoi_safe(row[3]));
		friend_data->set_is_recent(atoi_safe(row[4]));
		friend_data->set_is_temp(atoi_safe(row[5]));
	}

	mysql_free_result(res);

	return 0;
}


int FriendTable::save_friend(const commonproto::friend_data_t &finf)
{
	GEN_SQLSTR(this->sqlstr,
			" REPLACE INTO %s (userid, u_create_tm, friendid, f_create_tm, is_friend, is_recent, is_black, is_temp) "
			" VALUES (%u, %u, %u, %u, %u, %u, %u, %u) ",
			this->get_table_name(finf.userid()), 
			finf.userid(), finf.u_create_tm(), finf.friendid(), finf.f_create_tm(),
            finf.is_friend(), finf.is_recent(), finf.is_blacklist(), finf.is_temp());

	int ret = this->exec_update_list_sql(this->sqlstr, db_err_friend_set);
	return ret;
}

int FriendTable::remove_friend(const commonproto::friend_data_t &finf) {
	GEN_SQLSTR(this->sqlstr,
			" DELETE FROM %s"
			" WHERE userid = %u AND u_create_tm = %u AND friendid = %u AND f_create_tm = %u",
			this->get_table_name(finf.userid()), 
			finf.userid(), finf.u_create_tm(), finf.friendid(), finf.f_create_tm());
	return this->exec_delete_sql(this->sqlstr, DB_SUCC);
}

/*
int FriendTable::update_gift_count(
		uint32_t userid, uint32_t friendid, uint32_t gift_count) {
	GEN_SQLSTR(this->sqlstr, 
			" UPDATE %s" 
			" SET gift_count = %u" 
			" WHERE userid = %u AND friendid = %u",
			this->get_table_name(userid), gift_count,
			userid, friendid);
	int ret = this->exec_update_sql(this->sqlstr, db_err_update_gift_count);
	return ret;
}
*/
