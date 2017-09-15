/**
 * @file data_proto_utils.h
 * @brief 打包data.proto公用协议结构体工具
 * @author lemon lemon@taomee.com
 * @version 1.0.0
 * @date 2013-07-03
 */

#ifndef DATA_PROTO_UTILS_H
#define DATA_PROTO_UTILS_H

#include "common.h"
#include "player.h"
#include "duplicate_entity.h"

struct player_t;
class Pet;

class DataProtoUtils
{
public:
    static int pack_player_base_info(player_t* player,
            commonproto::player_base_info_t* base_info); 
    static int unpack_player_base_info(player_t *player, 
            const commonproto::player_base_info_t &base_info);

    static int pack_player_battle_info(player_t *player,
            commonproto::battle_info_t *battle_info);
    static int unpack_player_battle_info(player_t *player,
            const commonproto::battle_info_t &battle_info);

    static int pack_player_equip_info(player_t *player,
            commonproto::equip_list_t *equp_list);
    static int unpack_player_equip_info(player_t *player,
            const commonproto::equip_list_t &equp_list);

    static int pack_player_chisel_pet_info(player_t *player,
            commonproto::battle_pet_list_t *chisel_pet_list);
    static int unpack_player_chisel_pet_info(player_t *player,
            const commonproto::battle_pet_list_t &chisel_pet_list);
   
    static int pack_pet_rune_info(Pet *pet,
            google::protobuf::RepeatedPtrField <commonproto::rune_data_t> *rune_info);
    static int unpack_pet_rune_info(Pet *pet,
            const google::protobuf::RepeatedPtrField <commonproto::rune_data_t> &rune_list);

    static int pack_pet_info(Pet *pet, 
            commonproto::pet_info_t *pet_info);
    static int unpack_pet_info(Pet *pet,
            const commonproto::pet_info_t &pet_info);

    static int pack_player_battle_pet_info(player_t *player,
            commonproto::battle_pet_list_t *pet_list);
    static int unpack_player_battle_pet_info(player_t *player,
            const commonproto::battle_pet_list_t &pet_list);

    static int pack_player_switch_pet_info(player_t *player,
            commonproto::battle_pet_list_t *pet_list);
    static int unpack_player_switch_pet_info(player_t *player,
            const commonproto::battle_pet_list_t &pet_list);

    static int pack_duplicate_pet_info(duplicate_map_pet_t &dup_pet,
            commonproto::battle_pet_data_t *btl_pet);

    static int pack_duplicate_map_born_pet_info(std::vector<duplicate_map_pet_t> &pet_vec,
            google::protobuf::RepeatedPtrField<commonproto::battle_pet_data_t> *pet_list);

    static int pack_duplicate_map_monster_info(duplicate_entity_t *entity,
            google::protobuf::RepeatedPtrField<commonproto::battle_pet_data_t> *pet_list);

    static int pack_duplicate_map_builder_info(duplicate_entity_t *entity,
            google::protobuf::RepeatedPtrField<commonproto::battle_pet_data_t> *pet_list);

    static int pack_duplicate_player_info(player_t *player, duplicate_entity_t *entity,
            google::protobuf::RepeatedPtrField <commonproto::battle_player_data_t> *players);

    static int pack_duplicate_all_object(player_t *player, duplicate_entity_t *entity,
            battleproto::sc_battle_duplicate_enter_map &btl_msg);

    static int pack_player_battle_all_info(player_t *player,
            commonproto::battle_player_data_t *battle_all_info);

    static int unpack_player_battle_all_info(player_t *player,
            const commonproto::battle_player_data_t &battle_all_info);
	
	static int pack_duplicate_pet_info_when_born(duplicate_map_pet_t &dup_pet,
			commonproto::battle_pet_data_t *btl_pet);
};

#endif
