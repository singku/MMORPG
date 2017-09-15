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
#include "proto/client/common.pb.h"
#include "attr.h"
#include "proto/client/pb0x01.pb.h"
#include "player.h"


struct player_t;
struct item_t;
struct rune_t;
struct task_t;
/* struct rune_t; */
/* struct achieve_t; */
class Pet;
/* class Tactic; */


class DataProtoUtils
{
public:
    static int pack_player_base_info(player_t* player,
            commonproto::player_base_info_t* base_info); 

    static int pack_player_battle_base(player_t *player,
            commonproto::battle_info_t *battle_info, bool expand_data = false);

    static int pack_player_equip_info_by_attr(player_t *player,
            commonproto::item_info_t *item_info,
            uint32_t attr_id);

    static int pack_player_equip_info_by_attr(player_t *player,
            commonproto::equip_list_t *equp_list,
            uint32_t attr_id);

    static int pack_player_equip_info(player_t *player,
            commonproto::equip_list_t *equp_list);

    static int pack_player_battle_pet_info(player_t *player,
            commonproto::battle_pet_list_t *pet_list, bool expand_data = false);

    //打包参加远征的精灵信息(包括出战)
    static int pack_player_battle_exped_joined_pet_info(player_t* player,
            commonproto::battle_pet_list_t *pet_list);

    //打包远征中出战的精灵信息
    static int pack_player_battle_exped_fight_pet_info(player_t* player,
            commonproto::battle_pet_list_t *pet_list);

    //ps: 不要在进入远征场景的协议中使用
    static int pack_player_battle_exped_n_topest_pet_info(player_t* player,
            commonproto::battle_pet_list_t *pet_list);

    static int pack_player_battle_exped_n_topest_pet_info(player_t* player,
            onlineproto::exped_pet_info_list* pet_list);

    static int pack_exped_unit_pet_info(Pet& pet, 
            onlineproto::exped_pet_info* pb_pet_ptr);

    static int pack_player_chisel_pet_info(player_t *player,
            commonproto::battle_pet_list_t *pet_list);

    //打包背包中未出战的精灵信息
    static int pack_player_non_fight_bag_pet_info(player_t *player,
            commonproto::battle_pet_list_t *pet_list, bool expand_data = false);

    static int pack_player_item_info(player_t *player,
            commonproto::item_list_t *item_list);

    static int pack_pet_info(Pet *pet, 
            commonproto::pet_info_t *pet_info);

    static int pack_player_pet_info(player_t *player, Pet *pet,
            commonproto::pet_info_t *pet_info, bool expand_data = false);

    static int pack_player_pet_list(player_t *player,
            commonproto::pet_list_t *pet_list, 
            pet_location_type_t loc = PET_LOC_UNDEF, 
            bool expand_data = false);

    //pet_info的数据 解包到pet中
    static int unpack_pet_info(Pet *pet,
            const commonproto::pet_info_t &pet_info);
    //pet_list中的pet数据解包到玩家的精灵信息结构体里
    static int unpack_player_pet_info(player_t *player,
            const commonproto::pet_list_t &pet_list);

    static int pack_player_attr_info(player_t *player,
            commonproto::attr_data_list_t *attr_list);

    static int pack_player_battle_all_info(player_t *player,
            commonproto::battle_player_data_t *battle_all_info, bool expand_data = false);
    static int pack_map_pet_info(Pet *pet,
            commonproto::map_pet_info_t *pet_info);
    static int pack_map_player_info(player_t *player,
            commonproto::map_player_data_t *map_player_info);

    static int unpack_player_item_info(player_t *player, commonproto::item_list_t *item_list);

    static int unpack_item_info(const commonproto::item_info_t& item_info, item_t* item, bool &item_change);

    static int pack_item_info(const item_t* item, commonproto::item_info_t* item_info);

    static int unpack_base_info(player_t* player, const commonproto::player_base_info_t& base_info);

    static int pack_player_attrs(player_t* player, commonproto::attr_data_list_t* attr_list);

    static int unpack_player_attrs(player_t* player, const commonproto::attr_data_list_t& attr_list); 

    static int pack_task_data(const task_t* task, commonproto::task_data_t* task_info);
    static int unpack_task_data(const commonproto::task_data_t* task_info, task_t* task);
    static int pack_player_task(player_t* player, commonproto::task_list_t* task_list);
    static int unpack_player_task(player_t* player, const commonproto::task_list_t& task_list);

    static int pack_rune_data(player_t* player, commonproto::rune_list_cli_t* rune_list);
    static int unpack_rune_data(player_t* player, const commonproto::rune_list_t& rune_list);
	static int pack_all_achieves_info(player_t* player, commonproto::achieve_info_list_t* achv_ptr); 
	static int unpack_achieves_info(player_t* player, const commonproto::achieve_info_list_t& achieve_list);

    static int pack_single_rune_data(rune_t& rune, commonproto::rune_data_t* rune_data);

    static int pack_tran_card_data(player_t* player, onlineproto::sc_0x0101_enter_svr& cli_out);
    static int unpack_tran_data(player_t* player);
    static uint32_t pack_player_map_pet_info(player_t* player, commonproto::map_pet_list_t *pet_list);
    static int pack_pet_base_info(Pet *pet, commonproto::pet_base_info_t* pet_base_info);

    static int unpack_friend_data(player_t* player, const commonproto::friend_list_t& friend_list);
    static int pack_friend_data(player_t* player, commonproto::friend_info_list_t* friend_list);

    //static int unpack_hm_visit_log(player_t* player, const dbproto::hm_visit_log_list_t& log_list);
    static int unpack_hm_visit_log(player_t* player, const commonproto::visit_log_list_t& log_list);
    static int pack_hm_visit_log(player_t* player, commonproto::visit_log_list_t* log_list);

    static int unpack_hm_fragment_info(player_t* player, const std::string& buff_str);

    //打包自动pvp双方的战斗信息，并保存战报(需要两个玩家同服在线，才能调用此函数)
    static int pack_pk_players_btl_info_include_tick(
            player_t* player, player_t* ai_player_ptr, 
            uint32_t atk_tick, uint32_t def_tick, 
            commonproto::battle_data_include_tick_t* btl_inf,
            commonproto::btl_ground_type_t type = commonproto::GROUND_UNDEFINE);

    static int pack_player_buff(player_t *player, commonproto::user_buffs_t *user_buffs);
    static int unpack_player_buff(player_t *player, const commonproto::user_buffs_t &user_buffs);

    static int unpack_player_apply_family_ids(
            player_t *player, const commonproto::family_apply_record_t &apply_record);

#if 0
    static int unpack_elem_dup_shop_items(
            player_t *player, const commonproto::market_item_info_t &shop_info);

    static int unpack_arena_shop_items(
            player_t *player, const commonproto::market_item_info_t &shop_info);

    static int unpack_family_shop_items(
            player_t *player, const commonproto::market_item_info_t &shop_info);

    static int unpack_exped_shop_items(
            player_t *player, const commonproto::market_item_info_t &shop_info);
#endif
    static int unpack_shop_items(player_t *player, uint32_t market_type, 
            const commonproto::market_item_info_t &shop_info);

	static int unpack_all_titles_info(player_t* player,
			const commonproto::title_info_list_t & titles_list);

	static int pack_all_titles_info(player_t* player,
			commonproto::title_info_list_t* titles_ptr);

	static int unpack_mine_ids(player_t* player,
			const commonproto::mine_id_list_t& mine_ids_inf);

	static int unpack_mine_tmp_info(player_t* player,
			const commonproto::mine_info_list_t& mine_info_inf);

	static int unpack_mine_match_ids(player_t* player,
			const commonproto::mine_id_list_t& match_ids_inf);

	static int pack_pet_diju_extra_attr(
			commonproto::battle_player_data_t& battle_all_info);

	static int pack_player_suit_buff_info(player_t *player,
			commonproto::suit_buff_list_t *suit_buf_list);

	static int trim_battle_player_info(player_t *player,
		   	const commonproto::battle_player_data_t &player_info,
		   	commonproto::battle_player_data_t &cache_player_info);

};

#endif
