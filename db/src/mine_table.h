#ifndef MINE_TABLE_H
#define MINE_TABLE_H

#include <dbser/mysql_iface.h>
#include <dbser/Ctable.h>
#include "proto/db/dbproto.mine.pb.h"
class MineInfoTable : public Ctable
{
public:
	MineInfoTable(mysql_interface* db);
	int save_mine_info(const commonproto::mine_info_t& inf, uint64_t& new_mine_id);
	int get_mine_info_by_power_range(const dbproto::cs_get_mine_info& inf,
			commonproto::mine_info_list_t* mine_ptr, uint32_t protect_duration);
	int update_mine_info(const commonproto::mine_info_t& inf);

	int get_player_mine_info(uint64_t mine_id,
			commonproto::mine_info_list_t* pb_mine_ptr);

	int delete_mine_info(uint64_t mine_id);

	int increment_def_cnt(uint64_t mine_id);
	int get_mine_def_cnt(uint64_t mine_id, uint32_t& def_cnt, uint32_t& mine_size);
	int get_attack_state(uint64_t mine_id, uint32_t& is_been_attacked);
};

extern MineInfoTable* g_mine_info_table;

#endif
