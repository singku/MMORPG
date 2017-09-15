
#ifndef MONSTER_TABLE_H
#define MONSTER_TABLE_H

extern "C" {
#include <libtaomee/project/types.h>
}
#include <dbser/mysql_iface.h>
#include <dbser/CtableRoute10x10.h>
#include "proto/db/dbproto.login.pb.h"
#include "proto/db/dbproto.pet.pb.h"
#include "proto/client/common.pb.h"

class PetTable : public CtableRoute
{
public:
    PetTable(mysql_interface* db);

    uint32_t get_pets(userid_t userid, uint32_t u_create_tm, commonproto::pet_list_t *pet_list);

    uint32_t get_fight_pets(userid_t userid, uint32_t u_create_tm, commonproto::pet_list_t *pet_list);

    uint32_t get_chisel_pets(userid_t userid, uint32_t u_create_tm, commonproto::pet_list_t *pet_list);

    uint32_t save_pet(userid_t userid, uint32_t u_create_tm,
            const commonproto::pet_info_t& pet_info);

    uint32_t delete_pet(userid_t userid, uint32_t u_create_tm,
            const dbproto::cs_pet_del_info& cs_pet_del_info_);

	uint32_t get_pet_by_catch_time(userid_t userid, uint32_t u_create_tm, uint32_t catch_time, 
			commonproto::pet_list_t *pet_list);

	uint32_t get_n_topest_power_pets(userid_t userid, uint32_t u_create_tm, 
			commonproto::pet_list_t* pet_list, const uint32_t need_num);

};

extern PetTable* g_pet_table;

#endif
