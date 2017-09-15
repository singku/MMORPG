#include "task_info_table.h"
#include "utils.h"
#include "sql_utils.h"
#include "proto/db/db_errno.h"
#include "macro_utils.h"
#include <libtaomee++/utils/strings.hpp>
#include <boost/lexical_cast.hpp>
#include <libtaomee++/bitmanip/bitmanip.hpp>
#include <libtaomee/timer.h>


TaskInfoTable::TaskInfoTable(mysql_interface* db)
    : CtableRoute(db, "dplan_db", "task_table", "userid")
{

}

uint32_t TaskInfoTable::get_tasks(userid_t userid, uint32_t u_create_tm,
      dbproto::sc_get_login_info& login_info)
{
    GEN_SQLSTR(this->sqlstr,
            " SELECT task_id, status, done_times, cond_status, bonus_status, task_fin_tm"
            " FROM %s "
            " WHERE userid = %u AND u_create_tm = %u ",
            this->get_table_name(userid), userid, u_create_tm);

    MYSQL_RES* res = NULL;
    MYSQL_ROW  row;
    int ret = this->db->exec_query_sql(sqlstr, &res);

    if (ret) {
        return ret; 
    }
	//
    while ((row = mysql_fetch_row(res)) != NULL) {
    
        commonproto::task_data_t* task_info = login_info.mutable_task_list()->add_task_list();

        task_info->set_task_id(atoi_safe(row[0]));
        task_info->set_status(atoi_safe(row[1]));
        task_info->set_done_times(atoi_safe(row[2]));

		std::vector<std::string> args_list = split(row[3], ',');
		FOREACH(args_list, it) {
			task_info->add_cond_status(atoi_safe((*it).c_str()));
		}
        task_info->set_bonus_status(atoi_safe(row[4]));
		task_info->set_fin_statue_tm(atoi_safe(row[5]));
    }

    mysql_free_result(res);

    return 0;
}

uint32_t TaskInfoTable:: save_tasks(userid_t userid, uint32_t u_create_tm,
        const dbproto::cs_task_save& cs_task_save) 
{
    int count = cs_task_save.task_list().task_list_size();

    if (count == 0) {
        return 0; 
    }

    for (int i = 0; i < count; i++) {
        const commonproto::task_data_t& task_info = 
			cs_task_save.task_list().task_list(i);

		std::string cond_status;
		for (int j = 0; j < task_info.cond_status().size(); j++) {
			cond_status = cond_status + Utils::to_string(task_info.cond_status(j));
			if (j  < task_info.cond_status().size() - 1) {
				cond_status = cond_status + ",";
			}
		}
		
		GEN_SQLSTR(this->sqlstr, 
                " INSERT INTO %s "
                " (userid, u_create_tm, task_id, status, done_times, cond_status, bonus_status, task_fin_tm) "
                " VALUES (%u, %u, %u, %u, %u, '%s', %u, %u) "
                " ON DUPLICATE KEY UPDATE "
                " status= %u, done_times='%u', cond_status = '%s', bonus_status = '%u', task_fin_tm = '%u'",
                this->get_table_name(userid),
				userid, u_create_tm, task_info.task_id(), task_info.status(), task_info.done_times(),
				cond_status.c_str(), task_info.bonus_status(), task_info.fin_statue_tm(),
                task_info.status(), task_info.done_times(), cond_status.c_str(), task_info.bonus_status(), task_info.fin_statue_tm());
                
        int affected_rows = 0;
        int ret = this->db->exec_update_sql(this->sqlstr, &affected_rows);

        if (ret != 0) {
            ERROR_LOG("exec sql failed: %s", this->sqlstr);
            return ret;
        }

    }

    return 0;
}

uint32_t TaskInfoTable:: del_task(userid_t userid, uint32_t u_create_tm, 
        const dbproto::cs_task_del& cs_task_del)
{
    GEN_SQLSTR(this->sqlstr,
        " DELETE FROM %s "
        " WHERE userid = %u AND u_create_tm = %u AND task_id = %u",
        this->get_table_name(userid), userid, u_create_tm,
        cs_task_del.task_id());

    return this->exec_delete_sql(this->sqlstr, DB_SUCC);
}
