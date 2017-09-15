#include "macro_utils.h"
#include "mine_table.h"
#include "proto/db/db_errno.h"
#include <boost/lexical_cast.hpp>

MineInfoTable::MineInfoTable(mysql_interface* db)
	: Ctable(db, "dplan_other_db", "mine_info_table")
{}

int MineInfoTable::save_mine_info(const commonproto::mine_info_t& inf,
		uint64_t &new_mine_id)
{
	GEN_SQLSTR(this->sqlstr,
			" insert into %s (mine_create_tm, mine_type, mine_size, "
			" mine_server_id, userid, u_create_tm, total_pet_power, "
			" duration_time, def_player_cnt, is_been_attacked, "
			" last_fight_time, elem_id) "
			" values (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u) ",
			this->get_table_name(),
			inf.mine_create_tm(), inf.type(), inf.size(),
			inf.server_id(), inf.user_id(), inf.u_create_tm(), inf.top_pet_power(),
			inf.duration_time(), inf.def_player_cnt(), inf.is_been_attacked(),
			inf.last_fight_time(), inf.elem_id());
	return this->exec_insert_sql_get_auto_increment_id(this->sqlstr, 0, &new_mine_id);
}

int MineInfoTable::update_mine_info(const commonproto::mine_info_t& inf)
{
	//uint64_t mine_id = boost::lexical_cast<uint64_t>(inf.mine_id());
	GEN_SQLSTR(this->sqlstr,
			" update %s set mine_create_tm=%u, mine_type=%u, mine_size=%u, "
			" mine_server_id=%u, userid=%u, u_create_tm=%u, total_pet_power=%u, "
			" duration_time=%u, def_player_cnt=%u, is_been_attacked=%u, "
			" last_fight_time=%u, elem_id=%u "
			" where mine_id=%lu ",
			this->get_table_name(),
			inf.mine_create_tm(), inf.type(), inf.size(),
			inf.server_id(), inf.user_id(), inf.u_create_tm(), inf.top_pet_power(),
			inf.duration_time(), inf.def_player_cnt(),
			inf.is_been_attacked(), inf.last_fight_time(), inf.elem_id(), inf.mine_id());
	return this->exec_update_sql(this->sqlstr, DB_SUCC);
}

int MineInfoTable::delete_mine_info(uint64_t mine_id)
{
	GEN_SQLSTR(this->sqlstr,
			" delete FROM %s "
			" where mine_id=%lu ",
			this->get_table_name(),
			mine_id);
	return this->exec_delete_sql(this->sqlstr, db_err_delete_mine);
}

int MineInfoTable::get_mine_info_by_power_range(const dbproto::cs_get_mine_info& inf,
		commonproto::mine_info_list_t* mine_ptr, uint32_t protect_duration)
{
	GEN_SQLSTR(this->sqlstr,
			" select mine_id, mine_create_tm, mine_type, "
			" mine_size, mine_server_id, userid, u_create_tm, "
			" total_pet_power, duration_time, def_player_cnt, is_been_attacked, "
			" last_fight_time, elem_id FROM %s "
			" where total_pet_power >= %u and total_pet_power <= %u "
			" and mine_server_id = %u and %u > (mine_create_tm + %u) "
			" and (mine_create_tm + duration_time) > %u limit 20 ",
			this->get_table_name(), inf.low_power(),
			inf.high_power(), inf.server_id(), inf.time_stamp(), protect_duration, inf.time_stamp());
	PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, mine_ptr, mine_info)
		PB_INT_CPY_NEXT_FIELD(item, mine_id);
		PB_INT_CPY_NEXT_FIELD(item, mine_create_tm);
		uint32_t value;
		INT_CPY_NEXT_FIELD(value);
		item.set_type((commonproto::mine_type_t)value);
		INT_CPY_NEXT_FIELD(value);
		item.set_size((commonproto::mine_size_t)value);
		PB_INT_CPY_NEXT_FIELD(item, server_id);
		PB_INT_CPY_NEXT_FIELD(item, user_id);
		PB_INT_CPY_NEXT_FIELD(item, u_create_tm);
		PB_INT_CPY_NEXT_FIELD(item, top_pet_power);
		PB_INT_CPY_NEXT_FIELD(item, duration_time);
		PB_INT_CPY_NEXT_FIELD(item, def_player_cnt);
		PB_INT_CPY_NEXT_FIELD(item, is_been_attacked);
		PB_INT_CPY_NEXT_FIELD(item, last_fight_time);
		PB_INT_CPY_NEXT_FIELD(item, elem_id);
	PB_STD_QUERY_WHILE_END()
}

int MineInfoTable::get_player_mine_info(uint64_t mine_id,
			commonproto::mine_info_list_t* pb_mine_ptr)
{
		GEN_SQLSTR(this->sqlstr,
			" select mine_id, mine_create_tm, mine_type, "
			" mine_size, mine_server_id, userid, u_create_tm, "
			" total_pet_power, duration_time, def_player_cnt, "
			" is_been_attacked, last_fight_time, elem_id  FROM %s "
			" where mine_id = %lu",
			this->get_table_name(), mine_id);
		STD_QUERY_ONE_BEGIN(this->sqlstr, db_err_record_not_found)
			commonproto::mine_info_t* pb_ptr = pb_mine_ptr->add_mine_info();
			uint64_t value;
			INT_CPY_NEXT_FIELD(value);
			//uint32_t mine_id = boost::lexical_cast<uint32_t>(value);
			pb_ptr->set_mine_id(value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_mine_create_tm(value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_type((commonproto::mine_type_t)value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_size((commonproto::mine_size_t)value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_server_id(value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_user_id(value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_u_create_tm(value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_top_pet_power(value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_duration_time(value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_def_player_cnt(value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_is_been_attacked(value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_last_fight_time(value);
			INT_CPY_NEXT_FIELD(value);pb_ptr->set_elem_id(value);
     	STD_QUERY_ONE_END()
	return 0;
}

int MineInfoTable::get_attack_state(uint64_t mine_id,
		uint32_t& is_been_attacked)
{
	GEN_SQLSTR(this->sqlstr,
			" select is_been_attacked  FROM %s "
			" where mine_id = %lu",
			this->get_table_name(), mine_id);
	STD_QUERY_ONE_BEGIN(this->sqlstr, db_err_record_not_found)
		uint32_t value;
		INT_CPY_NEXT_FIELD(value);is_been_attacked = value;
	STD_QUERY_ONE_END()
	return 0;
}

int MineInfoTable::get_mine_def_cnt(uint64_t mine_id,
		uint32_t& def_cnt, uint32_t& mine_size)
{
	GEN_SQLSTR(this->sqlstr,
			" select def_player_cnt, mine_size from %s "
			" where mine_id = %lu ",
			this->get_table_name(), mine_id);
		STD_QUERY_ONE_BEGIN(this->sqlstr, db_err_record_not_found)
			uint32_t value;
			INT_CPY_NEXT_FIELD(value); def_cnt = value;
			INT_CPY_NEXT_FIELD(value); mine_size = value;
		STD_QUERY_ONE_END()
	return 0;
}

int MineInfoTable::increment_def_cnt(uint64_t mine_id)
{
	uint32_t def_cnt = 1;
	uint32_t mine_size = 1;
	get_mine_def_cnt(mine_id, def_cnt, mine_size);
	uint32_t cnt_limit = commonproto::MINE_DEFENDER_CNT_LIMIT;

	if (mine_size == commonproto::SMALL_MINE) {
		cnt_limit = commonproto::MINE_SMALL_DEFENDER_CNT_LIMIT;
	} else if (mine_size == commonproto::MID_MINE) {
		cnt_limit = commonproto::MINE_MID_DEFENDER_CNT_LIMIT;
	}

	if (def_cnt + 1 > cnt_limit) {
		return db_err_exceed_mine_def_cnt_limit;
	}
	++def_cnt;
	DEBUG_TLOG("the_increm,def_cnt=%u", def_cnt);
	GEN_SQLSTR(this->sqlstr,
			" update %s set def_player_cnt=%u"
			" where mine_id=%lu and def_player_cnt < %u ",
			this->get_table_name(), def_cnt, 
			mine_id, cnt_limit);
	return this->exec_update_sql(this->sqlstr, db_err_exceed_mine_def_cnt_limit);
}
