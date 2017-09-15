
#ifndef TASK_INFO_TABLE_H
#define TASK_INFO_TABLE_H 
extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute10x10.h>
#include "proto/db/dbproto.login.pb.h"
#include "proto/db/dbproto.task.pb.h"


class TaskInfoTable : public CtableRoute
{
public:
    TaskInfoTable(mysql_interface* db);

    uint32_t get_tasks(userid_t userid, uint32_t u_create_tm,
             dbproto::sc_get_login_info& login_info);

    uint32_t save_tasks(userid_t userid, uint32_t u_create_tm,
             const dbproto::cs_task_save& cs_task_save);

    uint32_t del_task(userid_t userid, uint32_t u_create_tm,
             const dbproto::cs_task_del& cs_task_del);
};

extern TaskInfoTable* g_task_table;

#endif
