#ifndef XML_CONFIGS_H
#define XML_CONFIGS_H

#include "xmlutils.h"
#include "global_data.h"

int load_sys_ctrl_config(xmlNodePtr root);
int load_player_config(xmlNodePtr root);
int load_attr_config(xmlNodePtr root);
int load_map_config(xmlNodePtr root);
int load_item_config(xmlNodePtr root);
int load_duplicate_config(xmlNodePtr root);
int load_skill_config(xmlNodePtr root);
int load_pet_config(xmlNodePtr root);
int load_talent_improve_config(xmlNodePtr root);
int load_prize_config(xmlNodePtr root);
int load_rune_config(xmlNodePtr root);
int load_rune_exp_config(xmlNodePtr root);
int load_rune_rate_config(xmlNodePtr root);
int load_task_config(xmlNodePtr root);
int load_condition_config(xmlNodePtr root);
int load_tran_card_config(xmlNodePtr root);
int load_shop_config(xmlNodePtr root);
int load_market_config(xmlNodePtr root);
int load_builder_config(xmlNodePtr root);
int load_exchange_config(xmlNodePtr root);
int load_arena_streak_reward_config(xmlNodePtr root);
int load_arena_rank_reward_config(xmlNodePtr root);
int load_trans_profession_config(xmlNodePtr root);
int load_hm_gift_config(xmlNodePtr root);
int load_name_pool_config(xmlNodePtr root);
int load_reward_task_config(xmlNodePtr root);
int load_achieve_config(xmlNodePtr root);
int load_time_config(xmlNodePtr root);
int load_server_buff_config(xmlNodePtr root);
int load_nick_config(xmlNodePtr root);
int load_pet_quality_config(xmlNodePtr root);
int load_family_config(xmlNodePtr root);
int load_pet_group_config(xmlNodePtr root);
int load_global_attr_config(xmlNodePtr root);
int load_exped_info(xmlNodePtr root);
int load_cultivate_equip_config(xmlNodePtr root);
int load_dup_area_prize_config(xmlNodePtr root);
int load_topest_power_player_info_config(xmlNodePtr root);
int load_first_test_uid_config(xmlNodePtr root);
int load_second_test_uid_config(xmlNodePtr root);
int load_question_config(xmlNodePtr root);
int init_task_cond_fun();
int load_title_config(xmlNodePtr root);
int load_bless_pet_config(xmlNodePtr root);
int load_rpvp_reward_config(xmlNodePtr root);
int load_equip_buff_config(xmlNodePtr root);
int load_client_buff_config(xmlNodePtr root);
int load_pet_pass_dup_config(xmlNodePtr root);
int load_suit_config(xmlNodePtr root);

#endif
