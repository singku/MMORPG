#include	<dbser/mysql_iface.h>
#include	<dbser/CtableRoute.h>
#include	"proto/db/dbproto.family.pb.h"
#include	"proto/client/common.pb.h"
#include	"sql_utils.h"
#include	"family_match_info_table.h"
#include    <set>

FamilyMatchInfoTable::FamilyMatchInfoTable(mysql_interface* db) 
    : Ctable(db, "dplan_other_db", "family_match_info_table")
{

}

int FamilyMatchInfoTable::update_family_match_info(
        const dbproto::family_match_info_table_t &info, uint32_t flag)
{
    std::string sql_str;
    if (flag == (uint32_t)dbproto::DB_UPDATE_AND_INESRT) {
        SQLUtils::gen_replace_sql_from_proto_any_key(
                get_table_name(),
                info, sql_str, &(this->db->handle)); 
    } else {
        SQLUtils::gen_update_sql_from_proto_any_key(
                get_table_name(),
                info, sql_str, &(this->db->handle));
    }

    if (sql_str.size() == 0) {
        return db_err_proto_format_err;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}

int FamilyMatchInfoTable::change_family_match_info(
        const dbproto::family_match_info_table_change_data_t &info)
{
    std::string sql_str;
    SQLUtils::gen_change_sql_from_proto_any_key(
            get_table_name(),
            info, sql_str, &(this->db->handle)); 

    if (sql_str.size() == 0) {
        return db_err_proto_format_err;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}

int FamilyMatchInfoTable::get_rand_match_list(
        dbproto::cs_family_get_recommend_list &list_in, 
        dbproto::sc_family_get_recommend_list *list_out)
{
    uint32_t total_match_num = 0;
    uint32_t max_start_pos = 0;
    uint32_t start_pos = 0;
    std::set<uint32_t> family_ids;
    uint32_t recommend_num = list_in.recommend_num();
    uint32_t server_id = list_in.server_id();
    // 推荐规则:
    // 家族平均战力与玩家战力在预定相近范围内  
    // 家族可以自动加入并且玩家达到最低战力
    // 新创建家族优先
    // 家族未满员
    GEN_SQLSTR(this->sqlstr,
            "select count(family_id) from %s where server_id = %u and base_join_value  <= %u and is_full = 0 "
            "and (total_battle_value / member_num)  >= %u and (total_battle_value / member_num) <= %u",
            get_table_name(), server_id, list_in.battle_value(), 
            list_in.low_percent() * list_in.battle_value() / 100,
            list_in.high_percent() * list_in.battle_value() / 100);
    MYSQL_RES* res = NULL;
    MYSQL_ROW row;
    int ret = this->db->exec_query_sql(sqlstr, &res);
    if (ret) {
        return ret;
    }

    if  ((row = mysql_fetch_row(res)) != NULL) {
        total_match_num = atoi_safe(row[0]);
    }
    mysql_free_result(res);

    if (total_match_num > recommend_num) {
        max_start_pos = total_match_num - recommend_num;
        start_pos = rand() % max_start_pos;
    }

    GEN_SQLSTR(this->sqlstr,
            "select family_id, family_name, member_num, total_battle_value, family_level,join_type, base_join_value "
            "from %s where server_id = %u and base_join_value  <= %u and is_full = 0 and family_id in "
            "(select family_id from %s where (total_battle_value / member_num)  >= %u and "
            "(total_battle_value / member_num) <= %u) order by create_time desc limit %u, %u",
            get_table_name(), server_id, list_in.battle_value(), 
            get_table_name(), 
            list_in.low_percent() * list_in.battle_value() / 100,
            list_in.high_percent() * list_in.battle_value() / 100,
            start_pos, recommend_num);
    res = NULL;
    ret = this->db->exec_query_sql(sqlstr, &res);

    if (ret) {
        return ret;
    }

    std::string ids_str;
    while  ((row = mysql_fetch_row(res)) != NULL) {
        commonproto::family_rank_info_t *info = list_out->add_family_list();
        info->set_rank(0);
        uint32_t family_id = atoi_safe(row[0]);
        info->set_family_id(family_id);
        info->set_family_name(row[1]);
        info->set_member_num(atoi_safe(row[2]));
        info->set_total_battle_value(atoi_safe(row[3]));
        info->set_family_level(atoi_safe(row[4]));
        info->set_join_type(atoi_safe(row[5]));
        info->set_base_join_value(atoi_safe(row[6]));

        family_ids.insert(family_id);
    }
    mysql_free_result(res);

    // 如果推荐数量不够，从高平均战力区间补充
    if ((uint32_t)list_out->family_list_size() < recommend_num) {
        uint32_t left_num = recommend_num - list_out->family_list_size();
        uint32_t total_high_num = 0;
        GEN_SQLSTR(this->sqlstr,
                "select count(family_id) from %s where server_id = %u and base_join_value  <= %u and is_full = 0 "
                "and (total_battle_value / member_num)  > %u",
                get_table_name(), server_id, list_in.battle_value(), 
                list_in.high_percent() * list_in.battle_value() / 100);
        MYSQL_RES* res = NULL;
        MYSQL_ROW row;
        int ret = this->db->exec_query_sql(sqlstr, &res);
        if (ret) {
            return ret;
        }

        if  ((row = mysql_fetch_row(res)) != NULL) {
            total_high_num = atoi_safe(row[0]);
        }
        mysql_free_result(res);

        start_pos = 0;
        if (total_high_num > left_num) {
            max_start_pos = total_high_num - left_num;
            start_pos = rand() % max_start_pos;
        }

        GEN_SQLSTR(this->sqlstr,
                "select family_id, family_name, member_num, total_battle_value, family_level,join_type, base_join_value from %s "
                "where server_id = %u and base_join_value  <= %u and is_full = 0 and (total_battle_value / member_num)  > %u limit %u, %u",
                get_table_name(), server_id, list_in.battle_value(), 
                list_in.high_percent() * list_in.battle_value() / 100,
                start_pos, recommend_num);
        res = NULL;
        ret = this->db->exec_query_sql(sqlstr, &res);

        if (ret) {
            return ret;
        }

        std::string ids_str;
        while  ((row = mysql_fetch_row(res)) != NULL && 
                 (uint32_t)list_out->family_list_size() < recommend_num) {
            uint32_t family_id = atoi_safe(row[0]);
            if (family_ids.count(family_id) == 0) {
                commonproto::family_rank_info_t *info = list_out->add_family_list();
                info->set_rank(0);
                info->set_family_id(family_id);
                info->set_family_name(row[1]);
                info->set_member_num(atoi_safe(row[2]));
                info->set_total_battle_value(atoi_safe(row[3]));
                info->set_family_level(atoi_safe(row[4]));
                info->set_join_type(atoi_safe(row[5]));
                info->set_base_join_value(atoi_safe(row[6]));

                family_ids.insert(family_id);
            }
        }
        mysql_free_result(res);
    }

    // 推荐数量还是不够，从低平均战力区间补充
    if ((uint32_t)list_out->family_list_size() < recommend_num) {
        uint32_t left_num = recommend_num - list_out->family_list_size();
        uint32_t total_low_num = 0;
        GEN_SQLSTR(this->sqlstr,
                "select count(family_id) from %s where server_id = %u and base_join_value  <= %u and is_full = 0 "
                "and (total_battle_value / member_num)  < %u",
                get_table_name(), server_id, list_in.battle_value(), 
                list_in.high_percent() * list_in.battle_value() / 100);
        MYSQL_RES* res = NULL;
        MYSQL_ROW row;
        int ret = this->db->exec_query_sql(sqlstr, &res);
        if (ret) {
            return ret;
        }

        if  ((row = mysql_fetch_row(res)) != NULL) {
            total_low_num = atoi_safe(row[0]);
        }
        mysql_free_result(res);

        start_pos = 0;
        if (total_low_num > left_num) {
            max_start_pos = total_low_num - left_num;
            start_pos = rand() % max_start_pos;
        }

        GEN_SQLSTR(this->sqlstr,
                "select family_id, family_name, member_num, total_battle_value, family_level,join_type, base_join_value from %s "
                "where server_id = %u and base_join_value  <= %u and is_full = 0 and (total_battle_value / member_num)  < %u limit %u, %u",
                get_table_name(), server_id, list_in.battle_value(), 
                list_in.low_percent() * list_in.battle_value() / 100,
                start_pos, recommend_num);
        res = NULL;
        ret = this->db->exec_query_sql(sqlstr, &res);

        if (ret) {
            return ret;
        }

        std::string ids_str;
        while  ((row = mysql_fetch_row(res)) != NULL) {
            uint32_t family_id = atoi_safe(row[0]);
            if (family_ids.count(family_id) == 0) {
                commonproto::family_rank_info_t *info = list_out->add_family_list();
                info->set_rank(0);
                info->set_family_id(family_id);
                info->set_family_name(row[1]);
                info->set_member_num(atoi_safe(row[2]));
                info->set_total_battle_value(atoi_safe(row[3]));
                info->set_family_level(atoi_safe(row[4]));
                info->set_join_type(atoi_safe(row[5]));
                info->set_base_join_value(atoi_safe(row[6]));

                family_ids.insert(family_id);
            }
        }
        mysql_free_result(res);
    }

    return 0;
}

int FamilyMatchInfoTable::del_match_info(uint32_t family_id)
{
    GEN_SQLSTR(this->sqlstr,
            "delete from %s where family_id = %u",get_table_name(), family_id);

    int affected_rows = 0;
    return this->db->exec_update_sql(this->sqlstr, &affected_rows);
}
