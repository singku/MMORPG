#include "pet_table.h"
#include "rune_table.h"
#include "utils.h"
#include "sql_utils.h"
#include "proto/db/db_errno.h"
#include <boost/lexical_cast.hpp>

enum {
	PET_OPTIONAL_ATTR_BUF_MAX_SIZE = 4096
};

PetTable::PetTable(mysql_interface* db)
    : CtableRoute(db, "dplan_db", "pet_table", "userid") {

}

uint32_t PetTable::save_pet(userid_t userid, uint32_t u_create_tm, const commonproto::pet_info_t& pet_info) 
{
    const commonproto::pet_base_info_t &b = pet_info.base_info();
    uint32_t talent_level  = pet_info.talent_level();
    const commonproto::pet_effort_value_t &e = pet_info.effort();
    const commonproto::anti_value_t &a = pet_info.battle_info().anti();
    const commonproto::battle_info_t &be = pet_info.battle_info();
	const commonproto::pet_effort_lv_t& e_lv = pet_info.effort_lv();
    uint32_t rune_ids[6] = {0,0,0,0,0,0};
    for (int i = 0; i < pet_info.rune_info_size() && i < 6; i++) {
		uint32_t index = pet_info.rune_info(i).grid_id();
		if (index >= 1 && index <= 4) {
			rune_ids[index - 1] = pet_info.rune_info(i).runeid();
		}
    }

	uint32_t rune_specified_pos_flag = pet_info.rune_specified_pos_flag();
	uint32_t exercise_tm = pet_info.exercise_tm();
	uint32_t last_add_exp_tm = pet_info.last_add_exp_tm();
	uint64_t mine_flag = boost::lexical_cast<uint64_t>(pet_info.mine_flag());
	uint64_t def_mine_id = boost::lexical_cast<uint64_t>(pet_info.defend_mine_id());

	static char pet_optional_attr_buf[PET_OPTIONAL_ATTR_BUF_MAX_SIZE] = {0};
	static char pet_optional_attr_buf_safe[PET_OPTIONAL_ATTR_BUF_MAX_SIZE*2] = {0}; 

	memset(pet_optional_attr_buf, 0, sizeof(pet_optional_attr_buf));
	memset(pet_optional_attr_buf_safe, 0, sizeof(pet_optional_attr_buf_safe));
	if (pet_info.has_pet_optional_attr()) {
		pet_info.pet_optional_attr().SerializeToArray(
			pet_optional_attr_buf, sizeof(pet_optional_attr_buf));
		set_mysql_string(pet_optional_attr_buf_safe,
				pet_optional_attr_buf, pet_info.pet_optional_attr().ByteSize());
	}
    
    GEN_SQLSTR(this->sqlstr, "INSERT INTO %s (userid, u_create_tm, pet_id, create_tm, level, exp,"
            "fight_pos, is_excercise, talent_level, "
            "effort_hp, effort_normal_atk, effort_normal_def, effort_skill_atk, effort_skill_def, "
            "anti_water, anti_fire, anti_grass, anti_light, anti_dark, anti_ground, anti_force, "
            "crit, anti_crit, hit, dodge, block, break_block, atk_speed, crit_affect_rate, block_affect_rate, "
            "hp, max_hp, normal_atk, normal_def, skill_atk, skill_def, power, "
            "rune_1_id, rune_2_id, rune_3_id, rune_4_id, rune_5_id, rune_6_id, chisel_pos, loc, rune_3_unlock_flag, quality, "
			"exercise_tm, last_add_exp_tm, pet_expedition_hp, exped_flag, mon_cris_hp, "
			"mon_night_raid_hp, effort_hp_lv, effort_normal_atk_lv, effort_normal_def_lv, effort_skill_atk_lv, effort_skill_def_lv, "
            "elem_type, elem_damage_percent, mine_fight_hp, mine_fight_flag, defend_mine_id, pet_optional_attr) "
            "VALUES (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, "
            "%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, "
            "%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, "
			"%u, %u, %u, %u, %u, %u, %u, %u, %lu, %lu, '%s') ON DUPLICATE KEY UPDATE pet_id = %u, level = %u, exp = %u, "
            "fight_pos = %u, is_excercise = %u, talent_level = %u, "
            "effort_hp = %u, effort_normal_atk = %u, effort_normal_def = %u, effort_skill_atk = %u, effort_skill_def = %u,"
            "anti_water = %u, anti_fire = %u, anti_grass = %u, anti_light = %u, anti_dark = %u, anti_ground = %u, anti_force = %u,"
            "crit = %u, anti_crit = %u, hit = %u, dodge = %u, block = %u, break_block = %u, atk_speed = %u, crit_affect_rate = %u,"
            "block_affect_rate = %u, hp = %u, max_hp = %u, normal_atk = %u, normal_def = %u, skill_atk = %u, skill_def = %u, power = %u, "
            "rune_1_id = %u, rune_2_id = %u, rune_3_id = %u, rune_4_id = %u, rune_5_id = %u, rune_6_id = %u, chisel_pos = %u,"
            "loc = %u, rune_3_unlock_flag=%u, quality=%u, exercise_tm=%u, last_add_exp_tm=%u, pet_expedition_hp = %u, exped_flag = %u, "
			"mon_cris_hp = %u, mon_night_raid_hp = %u, effort_hp_lv=%u, effort_normal_atk_lv=%u, effort_normal_def_lv=%u, "
            "effort_skill_atk_lv=%u, effort_skill_def_lv=%u, elem_type=%u, elem_damage_percent=%u, mine_fight_hp=%u, mine_fight_flag=%lu, "
			"defend_mine_id=%lu, pet_optional_attr='%s' ",
            this->get_table_name(userid), userid, u_create_tm, b.pet_id(), b.create_tm(), b.level(), b.exp(), 
            b.fight_pos(), b.exercise_pos(), talent_level, 
            e.hp(), e.normal_atk(), e.normal_def(), e.skill_atk(), e.skill_def(),
            a.water(), a.fire(), a.grass(), a.light(), a.dark(), a.ground(), a.force(),
            be.crit(), be.anti_crit(), be.hit(), be.dodge(), be.block(), be.break_block(), be.atk_speed(),
            be.crit_affect_rate(), be.block_affect_rate(), be.cur_hp(), be.max_hp(), be.normal_atk(), be.normal_def(),
            be.skill_atk(), be.skill_def(), b.power(),
            rune_ids[0], rune_ids[1], rune_ids[2], rune_ids[3], rune_ids[4], rune_ids[5], b.chisel_pos(), b.loc(), rune_specified_pos_flag, pet_info.quality(),
			exercise_tm, last_add_exp_tm, pet_info.exped_hp(), pet_info.exped_flag(), pet_info.mon_cris_hp(), pet_info.mon_night_raid_hp(),
            //rune_ids[0], rune_ids[1], rune_ids[2], rune_ids[3], rune_ids[4], rune_ids[5], b.chisel_pos(), b.loc(), rune_specified_pos_flag, 1,
			e_lv.effort_hp_lv(), e_lv.effort_normal_atk_lv(), e_lv.effort_normal_def_lv(), e_lv.effort_skill_atk_lv(), e_lv.effort_skill_def_lv(),
            be.elem_type(), be.elem_damage_percent(), pet_info.mine_fight_hp(), mine_flag, def_mine_id, pet_optional_attr_buf_safe,

            b.pet_id(), b.level(), b.exp(), 
            b.fight_pos(), b.exercise_pos(), talent_level,
            e.hp(), e.normal_atk(), e.normal_def(), e.skill_atk(), e.skill_def(),
            a.water(), a.fire(), a.grass(), a.light(), a.dark(), a.ground(), a.force(),
            be.crit(), be.anti_crit(), be.hit(), be.dodge(), be.block(), be.break_block(), be.atk_speed(),
            be.crit_affect_rate(), be.block_affect_rate(), be.cur_hp(), be.max_hp(), be.normal_atk(), be.normal_def(),
            be.skill_atk(), be.skill_def(), b.power(),
            rune_ids[0], rune_ids[1], rune_ids[2], rune_ids[3], rune_ids[4], rune_ids[5], b.chisel_pos(), b.loc(), rune_specified_pos_flag, pet_info.quality(),
            //rune_ids[0], rune_ids[1], rune_ids[2], rune_ids[3], rune_ids[4], rune_ids[5], b.chisel_pos(), b.loc(), rune_3_unlock_flag, 1);
			exercise_tm, last_add_exp_tm, pet_info.exped_hp(), pet_info.exped_flag(), pet_info.mon_cris_hp(), pet_info.mon_night_raid_hp(),
			e_lv.effort_hp_lv(), e_lv.effort_normal_atk_lv(), e_lv.effort_normal_def_lv(), e_lv.effort_skill_atk_lv(), e_lv.effort_skill_def_lv(),
            be.elem_type(), be.elem_damage_percent(), pet_info.mine_fight_hp(), mine_flag, def_mine_id, pet_optional_attr_buf_safe);
    
    return this->exec_update_list_sql(this->sqlstr, db_err_user_not_find);
}

uint32_t PetTable::delete_pet(userid_t userid, uint32_t u_create_tm, const dbproto::cs_pet_del_info& cs_pet_del_info) 
{
    GEN_SQLSTR(this->sqlstr, "DELETE FROM %s WHERE userid=%u AND u_create_tm = %u AND create_tm=%u",
            this->get_table_name(userid),userid, u_create_tm, cs_pet_del_info.create_tm());
    
    return this->exec_delete_sql(this->sqlstr, db_err_user_not_own_pet);
}

#define PACK_PET_FROM_SQL \
    do { \
        PB_STD_QUERY_WHILE_BEGIN(this->sqlstr, pet_list, pet_list) \
        commonproto::pet_base_info_t &b = *(item.mutable_base_info());\
        commonproto::pet_effort_value_t &e = *(item.mutable_effort());\
        commonproto::anti_value_t &a = *(item.mutable_battle_info()->mutable_anti());\
        commonproto::battle_info_t &be = *(item.mutable_battle_info());\
		commonproto::pet_effort_lv_t &e_lv = *(item.mutable_effort_lv()); \
        PB_INT_CPY_NEXT_FIELD(b, pet_id); PB_INT_CPY_NEXT_FIELD(b, create_tm); PB_INT_CPY_NEXT_FIELD(b, level);\
        PB_INT_CPY_NEXT_FIELD(b, exp); PB_INT_CPY_NEXT_FIELD(b, fight_pos); PB_INT_CPY_NEXT_FIELD(b, exercise_pos);\
        PB_INT_CPY_NEXT_FIELD(item, talent_level);\
        PB_INT_CPY_NEXT_FIELD(e, hp); PB_INT_CPY_NEXT_FIELD(e, normal_atk); PB_INT_CPY_NEXT_FIELD(e, normal_def);\
        PB_INT_CPY_NEXT_FIELD(e, skill_atk); PB_INT_CPY_NEXT_FIELD(e, skill_def); PB_INT_CPY_NEXT_FIELD(a, water);\
        PB_INT_CPY_NEXT_FIELD(a, fire); PB_INT_CPY_NEXT_FIELD(a, grass); PB_INT_CPY_NEXT_FIELD(a, light);\
        PB_INT_CPY_NEXT_FIELD(a, dark); PB_INT_CPY_NEXT_FIELD(a, ground); PB_INT_CPY_NEXT_FIELD(a, force);\
        PB_INT_CPY_NEXT_FIELD(be, crit); PB_INT_CPY_NEXT_FIELD(be, anti_crit); PB_INT_CPY_NEXT_FIELD(be, hit); \
        PB_INT_CPY_NEXT_FIELD(be, dodge); PB_INT_CPY_NEXT_FIELD(be, block); PB_INT_CPY_NEXT_FIELD(be, break_block); \
        PB_INT_CPY_NEXT_FIELD(be, atk_speed); PB_INT_CPY_NEXT_FIELD(be, crit_affect_rate); \
        PB_INT_CPY_NEXT_FIELD(be, block_affect_rate); PB_INT_CPY_NEXT_FIELD(be, cur_hp); PB_INT_CPY_NEXT_FIELD(be, max_hp);\
        PB_INT_CPY_NEXT_FIELD(be, normal_atk); PB_INT_CPY_NEXT_FIELD(be, normal_def);\
        PB_INT_CPY_NEXT_FIELD(be, skill_atk); PB_INT_CPY_NEXT_FIELD(be, skill_def); \
		PB_INT_CPY_NEXT_FIELD(b, power); \
        int tmp_val; INT_CPY_NEXT_FIELD(tmp_val); be.set_elem_type((commonproto::equip_elem_type_t)tmp_val); \
        PB_INT_CPY_NEXT_FIELD(be, elem_damage_percent);\
        uint32_t value; \
        commonproto::rune_data_t rune_info; \
        for (int i = 0; i < 6; i++) { \
            INT_CPY_NEXT_FIELD(value); \
            if (value == 0) { \
                continue; \
            } \
            rune_info.Clear(); \
            if (g_rune_table->get_rune_by_instance_id(userid, u_create_tm, value, rune_info)) {\
                continue;\
            } \
            rune_info.set_grid_id(i+1); \
            commonproto::rune_data_t *inf = item.add_rune_info();\
            inf->CopyFrom(rune_info); \
        }\
        PB_INT_CPY_NEXT_FIELD(b, chisel_pos);\
        INT_CPY_NEXT_FIELD(value);\
        b.set_loc((commonproto::pet_loc_type_t)value);\
        PB_INT_CPY_NEXT_FIELD(item, rune_specified_pos_flag);\
        PB_INT_CPY_NEXT_FIELD(item, quality);\
        PB_INT_CPY_NEXT_FIELD(item, exercise_tm);\
        PB_INT_CPY_NEXT_FIELD(item, last_add_exp_tm);\
        PB_INT_CPY_NEXT_FIELD(item, exped_hp);\
        INT_CPY_NEXT_FIELD(value);\
		item.set_exped_flag((commonproto::pet_exped_flag_t)value); \
        PB_INT_CPY_NEXT_FIELD(item, mon_cris_hp);\
        PB_INT_CPY_NEXT_FIELD(item, mon_night_raid_hp); \
		PB_INT_CPY_NEXT_FIELD(e_lv, effort_hp_lv); PB_INT_CPY_NEXT_FIELD(e_lv, effort_normal_atk_lv); \
		PB_INT_CPY_NEXT_FIELD(e_lv, effort_normal_def_lv); PB_INT_CPY_NEXT_FIELD(e_lv, effort_skill_atk_lv); \
		PB_INT_CPY_NEXT_FIELD(e_lv, effort_skill_def_lv); \
		PB_INT_CPY_NEXT_FIELD(item, mine_fight_hp); \
		INT_CPY_NEXT_FIELD(value);\
		std::string str_mine_flag = boost::lexical_cast<std::string>(value);\
		item.set_mine_flag(str_mine_flag); \
		INT_CPY_NEXT_FIELD(value);\
		std::string str_def_mine_id = boost::lexical_cast<std::string>(value);\
		item.set_defend_mine_id(str_def_mine_id);\
		PB_OBJ_CPY_NEXT_FIELD(item, pet_optional_attr); \
        PB_STD_QUERY_WHILE_END() \
    } while (0)

uint32_t PetTable::get_pets(userid_t userid, uint32_t u_create_tm, commonproto::pet_list_t *pet_list)
{
    GEN_SQLSTR(this->sqlstr, "SELECT pet_id, create_tm, level, exp,"
            "fight_pos, is_excercise, talent_level, "
            "effort_hp, effort_normal_atk, effort_normal_def, effort_skill_atk, effort_skill_def,"
            "anti_water, anti_fire, anti_grass, anti_light, anti_dark, anti_ground, anti_force,"
            "crit, anti_crit, hit, dodge, block, break_block, atk_speed, crit_affect_rate, block_affect_rate,"
            "hp, max_hp, normal_atk, normal_def, skill_atk, skill_def, power, elem_type, elem_damage_percent,"
            "rune_1_id, rune_2_id, rune_3_id, rune_4_id, rune_5_id, rune_6_id, chisel_pos, loc, rune_3_unlock_flag, quality, "
			"exercise_tm, last_add_exp_tm, pet_expedition_hp, exped_flag, mon_cris_hp, mon_night_raid_hp, "
			"effort_hp_lv, effort_normal_atk_lv, effort_normal_def_lv, effort_skill_atk_lv, effort_skill_def_lv, mine_fight_hp, "
			"mine_fight_flag, defend_mine_id, pet_optional_attr "
            "FROM %s where userid = %u and u_create_tm = %u ", this->get_table_name(userid), userid, u_create_tm);

    PACK_PET_FROM_SQL;
}

uint32_t PetTable::get_pet_by_catch_time(userid_t userid, uint32_t u_create_tm, uint32_t create_time, commonproto::pet_list_t *pet_list)
{
    GEN_SQLSTR(this->sqlstr, "SELECT pet_id, create_tm, level, exp,"
            "fight_pos, is_excercise, talent_level, "
            "effort_hp, effort_normal_atk, effort_normal_def, effort_skill_atk, effort_skill_def,"
            "anti_water, anti_fire, anti_grass, anti_light, anti_dark, anti_ground, anti_force,"
            "crit, anti_crit, hit, dodge, block, break_block, atk_speed, crit_affect_rate, block_affect_rate,"
            "hp, max_hp, normal_atk, normal_def, skill_atk, skill_def, power, elem_type, elem_damage_percent,"
            "rune_1_id, rune_2_id, rune_3_id, rune_4_id, rune_5_id, rune_6_id, chisel_pos, loc, rune_3_unlock_flag, quality, "
			"exercise_tm, last_add_exp_tm, pet_expedition_hp, exped_flag, mon_cris_hp, mon_night_raid_hp, "
			"effort_hp_lv, effort_normal_atk_lv, effort_normal_def_lv, effort_skill_atk_lv, effort_skill_def_lv, mine_fight_hp, "
			"mine_fight_flag, defend_mine_id, pet_optional_attr "
            "FROM %s where userid = %u and u_create_tm = %u and create_tm = %u", 
            this->get_table_name(userid), userid, u_create_tm, create_time);

    PACK_PET_FROM_SQL;
}

uint32_t PetTable::get_fight_pets(userid_t userid, uint32_t u_create_tm, commonproto::pet_list_t *pet_list)
{
    GEN_SQLSTR(this->sqlstr, "SELECT pet_id, create_tm, level, exp,"
            "fight_pos, is_excercise, talent_level, "
            "effort_hp, effort_normal_atk, effort_normal_def, effort_skill_atk, effort_skill_def,"
            "anti_water, anti_fire, anti_grass, anti_light, anti_dark, anti_ground, anti_force,"
            "crit, anti_crit, hit, dodge, block, break_block, atk_speed, crit_affect_rate, block_affect_rate,"
            "hp, max_hp, normal_atk, normal_def, skill_atk, skill_def, power, elem_type, elem_damage_percent, "
            "rune_1_id, rune_2_id, rune_3_id, rune_4_id, rune_5_id, rune_6_id, chisel_pos, loc, rune_3_unlock_flag ,quality, "
			"exercise_tm, last_add_exp_tm, pet_expedition_hp, exped_flag, mon_cris_hp, mon_night_raid_hp, "
			"effort_hp_lv, effort_normal_atk_lv, effort_normal_def_lv, effort_skill_atk_lv, effort_skill_def_lv, mine_fight_hp, "
			"mine_fight_flag, defend_mine_id, pet_optional_attr "
            "FROM %s where userid = %u and u_create_tm = %u and fight_pos != 0", 
            this->get_table_name(userid), userid, u_create_tm);

    PACK_PET_FROM_SQL;
}

uint32_t PetTable::get_chisel_pets(userid_t userid, uint32_t u_create_tm, commonproto::pet_list_t *pet_list)
{
    GEN_SQLSTR(this->sqlstr, "SELECT pet_id, create_tm, level, exp,"
            "fight_pos, is_excercise, talent_level, "
            "effort_hp, effort_normal_atk, effort_normal_def, effort_skill_atk, effort_skill_def,"
            "anti_water, anti_fire, anti_grass, anti_light, anti_dark, anti_ground, anti_force,"
            "crit, anti_crit, hit, dodge, block, break_block, atk_speed, crit_affect_rate, block_affect_rate,"
            "hp, max_hp, normal_atk, normal_def, skill_atk, skill_def, power, elem_type, elem_damage_percent,"
            "rune_1_id, rune_2_id, rune_3_id, rune_4_id, rune_5_id, rune_6_id, chisel_pos, loc, rune_3_unlock_flag, quality, "
			"exercise_tm, last_add_exp_tm, pet_expedition_hp, exped_flag, mon_cris_hp, mon_night_raid_hp, "
			"effort_hp_lv, effort_normal_atk_lv, effort_normal_def_lv, effort_skill_atk_lv, effort_skill_def_lv, mine_fight_hp, "
			"mine_fight_flag, defend_mine_id, pet_optional_attr "
            "FROM %s where userid = %u and u_create_tm = %u and chisel_pos != 0", 
            this->get_table_name(userid), userid, u_create_tm);

    PACK_PET_FROM_SQL;
}

uint32_t PetTable::get_n_topest_power_pets(userid_t userid, uint32_t u_create_tm,
		commonproto::pet_list_t *pet_list, const uint32_t need_num)
{
    GEN_SQLSTR(this->sqlstr, "SELECT pet_id, create_tm, level, exp,"
            "fight_pos, is_excercise, talent_level, "
            "effort_hp, effort_normal_atk, effort_normal_def, effort_skill_atk, effort_skill_def,"
            "anti_water, anti_fire, anti_grass, anti_light, anti_dark, anti_ground, anti_force,"
            "crit, anti_crit, hit, dodge, block, break_block, atk_speed, crit_affect_rate, block_affect_rate,"
            "hp, max_hp, normal_atk, normal_def, skill_atk, skill_def, power, elem_type, elem_damage_percent,"
            "rune_1_id, rune_2_id, rune_3_id, rune_4_id, rune_5_id, rune_6_id, chisel_pos, loc, rune_3_unlock_flag, quality, "
			"exercise_tm, last_add_exp_tm, pet_expedition_hp, exped_flag, mon_cris_hp, mon_night_raid_hp, "
			"effort_hp_lv, effort_normal_atk_lv, effort_normal_def_lv, effort_skill_atk_lv, effort_skill_def_lv, mine_fight_hp, "
			"mine_fight_flag, defend_mine_id, pet_optional_attr "
            "FROM %s where userid = %u and u_create_tm = %u order by power desc limit %u ",
            this->get_table_name(userid), userid, u_create_tm, need_num);

    PACK_PET_FROM_SQL;
}
