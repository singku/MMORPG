#ifndef USER_ACTION_LOG_TABLE_H
#define USER_ACTION_LOG_TABLE_H
extern "C" {
#include <libtaomee/project/types.h>
}

#include <set>
#include <dbser/mysql_iface.h>
#include <dbser/CtableDate.h>
#include "proto/db/dbproto.user_action_log.pb.h"

// 生成全局唯一id的表
class UserActionLogTable : public CtableDate
{
public: 
    UserActionLogTable(mysql_interface* db);
    uint32_t insert_log(userid_t userid, uint32_t u_create_tm,
            const dbproto::user_action_log_t& insert_msg);

    uint32_t get_action_log(userid_t userid, uint32_t u_create_tm, uint32_t date,
            uint32_t start_index, uint32_t num, dbproto::sc_get_user_action_log* log_list);

	uint32_t get_action_log_with_tar_id(
			userid_t userid, uint32_t u_create_tm, uint32_t date,
			uint32_t start_index, uint32_t num, uint32_t tar_id,
			dbproto::sc_get_user_action_log* log_list);

    uint32_t get_login_tm_info_ex(userid_t userid, uint32_t u_create_tm, uint32_t date, uint32_t from_tm, uint32_t to_tm,
            std::set<uint32_t> &login_tm_vec);

    uint32_t get_login_tm_info(userid_t userid, uint32_t u_create_tm, uint32_t from_tm, uint32_t to_tm,
            std::set<uint32_t> &login_tm_vec);

	uint32_t get_user_log_total(
			userid_t userid, uint32_t u_create_tm,
			uint32_t date, dbproto::sc_get_user_action_log* log_list);
};

extern UserActionLogTable* g_user_action_log_table;

#endif
