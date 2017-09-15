#include <dbser/mysql_iface.h>
#include <dbser/db_macro.h>
#include "base_info_table.h"
#include "proto/db/db_errno.h"
// #include "utils.h"
#include "sql_utils.h"
#include "proto/client/attr_type.h"
#include "attr_table.h"



BaseInfoTable::BaseInfoTable(mysql_interface* db) 
    : CtableRoute(db, "dplan_db", "base_info_table", "userid") {
}

uint32_t BaseInfoTable::create_role( userid_t userid, const dbproto::cs_create_role& cs_create_role, uint32_t &u_create_tm)
{
    char nick[32 + 1] = {0};
    char nick_mysql[2 * sizeof(nick) + 1] = {0};

    strncpy(nick, cs_create_role.nick().c_str(), sizeof(nick) - 1);
    set_mysql_string(nick_mysql, nick, strlen(nick));
    u_create_tm = time(0);

    uint32_t cur_server_id = cs_create_role.cur_server_id();
    uint32_t init_server_id = cs_create_role.init_server_id();
    if (cur_server_id == 0) {
        cur_server_id = 1;
    }
    if (init_server_id == 0) {
        init_server_id = cur_server_id;
    }
    GEN_SQLSTR(this->sqlstr,
            " INSERT INTO %s "
            " (userid, u_create_tm, init_server_id, server_id, nick) "
            " VALUES (%u, %u, %u, %u, '%s')",
            this->get_table_name(userid),
            userid, u_create_tm, init_server_id, cur_server_id,
            nick_mysql );

    int ret = this->exec_insert_sql(this->sqlstr, db_err_role_already_exist);
    if(DB_SUCC != ret) {
        return ret;
    }

    //职业及出生时间
    g_attr_table->set_attr(userid, kAttrCreateTm, u_create_tm, u_create_tm);
    g_attr_table->set_attr(userid, kAttrCurProf, cs_create_role.cur_prof(), u_create_tm);
    g_attr_table->set_attr(userid, kAttrSex, cs_create_role.sex(), u_create_tm);

    return ret;
}

uint32_t BaseInfoTable::user_exists(userid_t userid, uint32_t u_create_tm, uint32_t server_id, bool is_init_server) 
{
    if (server_id == 0) { // 没有指定服务器ID 则判断角色是否存在
        GEN_SQLSTR(this->sqlstr,
                " SELECT userid FROM %s "
                " WHERE userid = %u AND u_create_tm = %u",
                this->get_table_name(userid),
                userid, u_create_tm); 

    } else if (is_init_server) {//init_server_id上一个userid只可能有一个记录
        GEN_SQLSTR(this->sqlstr,
                " SELECT userid FROM %s "
                " WHERE userid = %u AND init_server_id = %u",
                this->get_table_name(userid),
                userid, server_id); 

    } else {//server_id上 由于存在合服 可能存在多个记录
        if (u_create_tm) {//指定了角色创建时间
            GEN_SQLSTR(this->sqlstr,
                    " SELECT userid FROM %s "
                    " WHERE userid = %u AND server_id = %u AND u_create_tm = %u limit 1",
                    this->get_table_name(userid),
                    userid, server_id, u_create_tm); 
        } else {
            GEN_SQLSTR(this->sqlstr,
                    " SELECT userid FROM %s "
                    " WHERE userid = %u AND server_id = %u limit 1",
                    this->get_table_name(userid),
                    userid, server_id); 
        }

    }

    STD_QUERY_ONE_BEGIN(this->sqlstr, db_err_user_not_find);
    STD_QUERY_ONE_END();
}

uint32_t BaseInfoTable::get_info(userid_t userid, 
        commonproto::player_base_info_t& base_info, uint32_t u_create_tm)
{
    base_info.set_user_id(userid);
    GEN_SQLSTR(this->sqlstr,
            " SELECT nick, server_id, init_server_id "
            " FROM %s "
            " WHERE userid = %u AND u_create_tm = %u ",
            this->get_table_name(userid),
            userid, u_create_tm);

    STD_QUERY_ONE_BEGIN(this->sqlstr, db_err_user_not_find)
        base_info.set_nick(row[0]);
        base_info.set_server_id(atoi_safe(row[1]));
        base_info.set_init_server_id(atoi_safe(row[2]));
        //base_info的attr是从1-100固定的
        commonproto::attr_data_list_t list;
        list.Clear();
        g_attr_table->get_ranged_attr(userid, kBaseInfoAttrStart, kBaseInfoAttrEnd, &list, u_create_tm);
        std::map<uint32_t, uint32_t>attr_map;
        attr_map.clear();
        for (int i = 0; i < list.attrs_size(); i++) {
            attr_map[list.attrs(i).type()] = list.attrs(i).value();
        }
		uint32_t value = 0;
		g_attr_table->get_attr(userid, kAttrCurBattleValue, value, u_create_tm);
		attr_map[kAttrCurBattleValue] = value;
        value = 0;
		g_attr_table->get_attr(userid, kAttrFamilyId, value, u_create_tm);
		attr_map[kAttrFamilyId] = value;
        value = 0;
        g_attr_table->get_attr(userid, kAttrLastLoginTm, value, u_create_tm);
        attr_map[kAttrLastLoginTm] = value;	
        value = 0;
        g_attr_table->get_attr(userid, kAttrPlayerElemType, value, u_create_tm);
        attr_map[kAttrPlayerElemType] = value;
        value = 0;
        g_attr_table->get_attr(userid, kAttrPlayerElemDamageRate, value, u_create_tm);
        attr_map[kAttrPlayerElemDamageRate] = value;

        value = 0;
		g_attr_table->get_attr(userid, kAttrMountShowFlag, value, u_create_tm);
		attr_map[kAttrMountShowFlag] = value;
        value = 0;
		g_attr_table->get_attr(userid, kAttrWingShowFlag, value, u_create_tm);
		attr_map[kAttrWingShowFlag] = value;

		value = 0;
		g_attr_table->get_attr(userid, kAttrEquipTitleId, value, u_create_tm);
		attr_map[kAttrEquipTitleId] = value;

#define FIND_ATTR(key) (attr_map.count((key)) ?attr_map[(key)] :0)

        base_info.set_create_tm(FIND_ATTR(kAttrCreateTm));
        base_info.set_cur_prof(FIND_ATTR(kAttrCurProf));
        base_info.set_sex(FIND_ATTR(kAttrSex));
        base_info.set_level(FIND_ATTR(kAttrLv));
        base_info.set_exp(FIND_ATTR(kAttrExp));
		//暂时不用
		// base_info.set_vip_begin_time(FIND_ATTR(kAttrVipBeginTime));
		// base_info.set_vip_end_time(FIND_ATTR(kAttrVipEndTime));
		// base_info.set_vip_level(FIND_ATTR(kAttrVipLevel));
		// base_info.set_is_yearly_vip(FIND_ATTR(kAttrIsYearlyVip));
        base_info.set_frozen_end_time(FIND_ATTR(kAttrFrozenEndTime));
        base_info.set_frozen_reason(FIND_ATTR(kAttrFrozenReason));
		base_info.set_power(FIND_ATTR(kAttrCurBattleValue));
		base_info.set_family_id(FIND_ATTR(kAttrFamilyId));
        base_info.set_is_newbe(FIND_ATTR(kAttrGuideFinished) ?false :true);
        base_info.set_last_login_tm(FIND_ATTR(kAttrLastLoginTm));
		//vip相关
		base_info.set_silver_vip_end_time(FIND_ATTR(kAttrSilverVipEndTime));
		base_info.set_gold_vip_end_time(FIND_ATTR(kAttrGoldVipEndTime));
		base_info.set_mount_show_flag(FIND_ATTR(kAttrMountShowFlag));
		base_info.set_wing_show_flag(FIND_ATTR(kAttrWingShowFlag));
		base_info.set_equip_title_id(FIND_ATTR(kAttrEquipTitleId));

    STD_QUERY_ONE_END();
}

uint32_t BaseInfoTable::get_infos(userid_t userid, uint32_t server_id,
        google::protobuf::RepeatedPtrField<commonproto::player_base_info_t> *base_infos)
{
    base_infos->Clear();

    if (server_id) {
        GEN_SQLSTR(this->sqlstr,
                " SELECT u_create_tm "
                " FROM %s "
                " WHERE userid = %u AND init_server_id = %u ",
                this->get_table_name(userid), userid, server_id);
    } else {
        GEN_SQLSTR(this->sqlstr,
                " SELECT u_create_tm "
                " FROM %s "
                " WHERE userid = %u ",
                this->get_table_name(userid), userid);
    }

    uint32_t u_create_tm = 0;
    std::vector<uint32_t> create_tm_vec;
    STD_QUERY_WHILE_BEGIN_NEW(this->sqlstr, create_tm_vec)
        INT_CPY_NEXT_FIELD(u_create_tm);
        commonproto::player_base_info_t *inf = base_infos->Add();
        get_info(userid, *inf, u_create_tm);
    STD_QUERY_WHILE_END_NEW();
}

uint32_t BaseInfoTable::change_nick(userid_t userid, std::string new_nick, uint32_t u_create_tm)
{
    char nick[32 + 1] = {0};
    char nick_mysql[2 * sizeof(nick) + 1] = {0};

    strncpy(nick, new_nick.c_str(), sizeof(nick) - 1);
    set_mysql_string(nick_mysql, nick, strlen(nick));

    GEN_SQLSTR(this->sqlstr,
            "UPDATE %s SET nick = '%s' WHERE userid = %u and u_create_tm = %u",
            this->get_table_name(userid), nick_mysql, userid, u_create_tm);

    return  this->exec_update_sql(this->sqlstr, 0);
}
