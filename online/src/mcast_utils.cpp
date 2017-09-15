#include "mcast_utils.h"
#include "global_data.h"
#include "xml_configs.h"
#include "player.h"
#include "player_manager.h"
#include "dll_iface.h"

uint32_t McastUtils::reload_configs(const std::string& conf_name)
{
    int ret = 0;
    if (conf_name.compare("attribute.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "attribute.xml"), load_attr_config);
    } else if (conf_name.compare("time_config.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "time_config.xml"), load_time_config);
    } else if (conf_name.compare("buff.xml") == 0) {
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "buff.xml"), load_client_buff_config);
    } else if (conf_name.compare("server_buff.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "server_buff.xml"), load_server_buff_config);
    } else if (conf_name.compare("equip_buff.xml") == 0) {
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "equip_buff.xml"), load_equip_buff_config);
    } else if (conf_name.compare("item.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "item.xml"), load_item_config);
    } else if (conf_name.compare("skill_parent.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "skill_parent.xml"), load_skill_config);
    } else if (conf_name.compare("pet.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "pet.xml"), load_pet_config);
    } else if (conf_name.compare("rune.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "rune.xml"), load_rune_config);
    } else if (conf_name.compare("rune_exp.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "rune_exp.xml"), load_rune_exp_config);
    } else if (conf_name.compare("rune_rate.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "rune_rate.xml"), load_rune_rate_config);
    } else if (conf_name.compare("map.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "map.xml"), load_map_config);
    } else if (conf_name.compare("duplicate.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "duplicate.xml"), load_duplicate_config);
    } else if (conf_name.compare("title_info.xml")) {
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "title_info.xml"), load_title_config);
    } else if (conf_name.compare("prize.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "prize.xml"), load_prize_config);
    } else if (conf_name.compare("player.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "player.xml"), load_player_config);
    } else if (conf_name.compare("tasks.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "tasks.xml"), load_task_config);
    } else if (conf_name.compare("reward_task.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "reward_task.xml"), load_reward_task_config);
    } else if (conf_name.compare("sys_ctrl.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "sys_ctrl.xml"), load_sys_ctrl_config);
    } else if (conf_name.compare("shop.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "shop.xml"), load_shop_config);
    } else if (conf_name.compare("market.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "market.xml"), load_market_config);
    } else if (conf_name.compare("builder.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "builder.xml"), load_builder_config);
    } else if (conf_name.compare("exchanges.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "exchanges.xml"), load_exchange_config);
    } else if (conf_name.compare("arena_str_reward.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "arena_str_reward.xml"), load_arena_streak_reward_config);
    } else if (conf_name.compare("arenas.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "arenas.xml"), load_arena_rank_reward_config);
    } else if (conf_name.compare("trans_profession.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "trans_profession.xml"), load_trans_profession_config);
    } else if (conf_name.compare("home_gift.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "home_gift.xml"), load_hm_gift_config);
    } else if (conf_name.compare("name_pool.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "name_pool.xml"), load_name_pool_config);
    } else if (conf_name.compare("achieve.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "achieve.xml"), load_achieve_config);
    } else if (conf_name.compare("nick.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "nick.xml"), load_nick_config);
    } else if (conf_name.compare("pet_quality.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "pet_quality.xml"), load_pet_quality_config);
    } else if (conf_name.compare("family_templates.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "family_templates.xml"), load_family_config);
    } else if (conf_name.compare("global_attr.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "global_attr.xml"), load_global_attr_config);
    } else if (conf_name.compare("pet_group.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "pet_group.xml"), load_pet_group_config);
    } else if (conf_name.compare("expedition.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "expedition.xml"), load_exped_info);
    } else if (conf_name.compare("duplicate_area_prize.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "duplicate_area_prize.xml"), load_dup_area_prize_config);
    } else if (conf_name.compare("cultivate_equip.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "cultivate_equip.xml"), load_cultivate_equip_config);
    } else if (conf_name.compare("topest_power_users.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "topest_power_users.xml"), load_topest_power_player_info_config);
    } else if (conf_name.compare("test1_joined_user.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "test1_joined_user.xml"), load_first_test_uid_config);
    } else if (conf_name.compare("test2_joined_user.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "test2_joined_user.xml"), load_second_test_uid_config);
    } else if (conf_name.compare("question.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "question.xml"), load_question_config);
    } else if (conf_name.compare("suit.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "suit.xml"), load_suit_config);
    } else if (conf_name.compare("bless_pet.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "bless_pet.xml"), load_bless_pet_config);
    } else if (conf_name.compare("rpvp_reward.xml") == 0){
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "rpvp_reward.xml"), load_rpvp_reward_config);
    } else if (conf_name.compare("pet_pass_dup.xml") == 0){
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "pet_pass_dup.xml"), load_pet_pass_dup_config);
	} else {
        ret = -1;
    }

    return ret;
}

uint32_t McastUtils::notify(const std::string& msg)
{
    onlineproto::sc_0x012A_system_notice sc_notice;  

    sc_notice.set_type(0);
    sc_notice.set_content(msg);

    std::vector<player_t*> player_list;
    g_player_manager->get_player_list(player_list);
    for (uint32_t i = 0; i < player_list.size(); i++) {
        player_t* player = player_list[i]; 
        send_msg_to_player(player, cli_cmd_cs_0x012A_system_notice, sc_notice);
    }
    return 0;
}
