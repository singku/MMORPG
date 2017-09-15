#include "common.h"
#include "player.h"
#include "player_manager.h"
#include "proto_queue.h"
#include "package.h"
#include "global_data.h"
#include "rune.h"
#include "task_info.h"
#include "rune.h"
#include "duplicate_utils.h"
#include "tran_card.h"
#include "home_data.h"
#include "friend.h"
#include "achieve.h"
#include "exped.h"
#include "title.h"

int PlayerManager::init_player(player_t *player, fdsession_t *fdsess, uint32_t userid)
{
    player->fdsess = fdsess;
    player->userid = userid;

    // 初始化地图链表
    INIT_LIST_HEAD(&player->timer_list);

    player->is_login = false;

    // 初始化精灵信息
    player->pets = new std::map<uint32_t, Pet>();
    player->bag_pets = new std::map<uint32_t, Pet*>();
    player->store_pets = new std::map<uint32_t, Pet*>();
    player->elite_pets = new std::map<uint32_t, Pet*>();
    player->room_pets = new std::map<uint32_t, Pet*>();
    player->pet_id_pets = new std::map<uint32_t, std::vector<Pet*> >();
    player->family_lock_sets = new std::vector<std::string>();
    player->family_apply_record = new std::set<uint32_t>();
    player->elem_dup_shop_items = new std::map<uint32_t, ol_market_item_t>();
    player->arena_shop_items = new std::map<uint32_t, ol_market_item_t>();
	player->exped_shop_items = new std::map<uint32_t, ol_market_item_t>();
	player->night_raid_shop_items = new std::map<uint32_t, ol_market_item_t>();
    player->family_shop_items = new std::map<uint32_t, ol_market_item_t>();
    player->daily_shop_items = new std::map<uint32_t, ol_market_item_t>();
	player->smelter_money_shop_items = new std::map<uint32_t, ol_market_item_t>();
	player->smelter_gold_shop_items = new std::map<uint32_t, ol_market_item_t>();
    player->daily_charge_diamond_draw_cards_info = new std::vector<cache_prize_elem_t>;

    for (int i = 0; i < MAX_FIGHT_POS; i++) {
        player->fight_pet[i] = NULL;
    }

    // 初始化attr信息
    player->attrs = new Attr();
    // 背包
    player->package = new Package();

	//任务
	player->task_info = new TaskInfo();    

    // 初始化缓存协议队列
    player->proto_queue = new ProtoQueue(kMaxProtoQueueSize);

    // 初始化buff列表
    player->buff_id_map = new std::map<uint32_t,uint32_t>();
	//套装buf列表
    player->suit_buff_map = new std::map<uint32_t,uint32_t>();

    // 定时器初始化
    INIT_LIST_HEAD(&player->timer_list);
    player->daily_op_timer = NULL;
    player->check_money_return = NULL;
    player->sync_client_time_timer = NULL;
    player->svr_request_timer = NULL;
    player->clear_daily_attr_timer = NULL;
    player->clean_expired_items_timer = NULL;
    player->vp_flush_timer = NULL;
    player->shop_flush_timer = NULL;
    player->dive_timer = NULL;
    player->vp_add_timer = NULL;
    player->weekly_arena_reward_timer = NULL;
	player->exercise_pets_add_exp = NULL;
	player->open_srv_power_reward_timer = NULL;
	player->open_srv_gold_consume_reward_timer = NULL;
	player->weekly_activity_rank_reward_timer = NULL;

    // 初始化临时信息
    memset(&(player->temp_info), 0, sizeof(player->temp_info));
    player->temp_info.state_buf = new std::string();
    player->temp_info.rank_info = new string();
	player->temp_info.ai_nick = new string();
	player->temp_info.tmp_btl_info = new string();
	player->temp_info.bless_team_info = new string();
	player->temp_info.cache_string = new string();
	player->temp_info.cache_out_string = new string();
    player->temp_info.bit_vec = new bitset<1024>();
    player->temp_info.bit_vec->reset();
    player->temp_info.login_steps = new bitset<64>();
    player->temp_info.login_steps->reset();
    player->temp_info.cache_dup_drop_prize = new std::map<uint32_t, cache_prize_elem_t>();
	player->temp_info.m_arena_index_info = new std::map<uint32_t, uint64_t>();
	player->temp_info.atk_no_map = new std::map<uint32_t, uint32_t>();
	player->temp_info.cache_prize_vec = new std::vector<cache_prize_elem_t>();
	player->temp_info.cache_tmp_vec = new std::vector<cache_prize_elem_t>();
	player->temp_info.cache_question_vec = new std::vector<uint32_t>();
	player->temp_info.cache_prize_id = new std::vector<uint32_t>();
	player->temp_info.mon_cris_pets = new std::map<uint32_t, int>();
	player->temp_info.night_raid_pets = new std::map<uint32_t, int>();
	player->temp_info.night_raid_op_pets = new std::map<uint32_t, int>();
	player->temp_info.dirty_users= new std::set<uint64_t>();
	player->temp_info.bless_team_member_info = new std::vector<std::string>();

	//初始化符文馆信息
	player->rune_meseum = NULL;
	//TODO temp comment
	player->rune_meseum = new RuneMeseum(player);
	player->m_tran_card = NULL;
	player->m_tran_card = new TranCard();
	player->home_data = NULL;
	player->home_data = new home_data_t();
	player->achieve = NULL;
	player->achieve = new Achieve();
	player->expedtion = NULL;
	player->expedtion = new Expedition();
	player->nightraid = NULL;
	player->nightraid = new NightRaid();
	//初始化好友信息
	player->friend_info = NULL;
	player->friend_info = new FriendInfo();
	
	player->title = NULL;
	player->title = new Title();

	player->mine_info = NULL;
	player->mine_info = new MineInfo();

    return 0;
}

int PlayerManager::uninit_player(player_t *player)
{
    player->pets->clear();
    delete player->pets;
    player->pets = NULL;

    player->bag_pets->clear();
    delete player->bag_pets;
    player->bag_pets = NULL;

    player->store_pets->clear();
    delete player->store_pets;
    player->store_pets = NULL;

    player->elite_pets->clear();
    delete player->elite_pets;
    player->elite_pets = NULL;

    player->room_pets->clear();
    delete player->room_pets;
    player->room_pets = NULL;
    
    player->pet_id_pets->clear();
    delete player->pet_id_pets;
    player->pet_id_pets = NULL;

    player->family_lock_sets->clear();
    delete player->family_lock_sets;
    player->family_lock_sets = NULL; 

    player->family_apply_record->clear();
    delete player->family_apply_record;
    player->family_apply_record = NULL;

    player->elem_dup_shop_items->clear();
    delete player->elem_dup_shop_items;
    player->elem_dup_shop_items = NULL;

    player->arena_shop_items->clear();
    delete player->arena_shop_items;
    player->arena_shop_items = NULL;

	player->exped_shop_items->clear();
	delete player->exped_shop_items;
	player->exped_shop_items = NULL;

	player->night_raid_shop_items->clear();
	delete player->night_raid_shop_items;
	player->night_raid_shop_items = NULL;

	player->daily_shop_items->clear();
	delete player->daily_shop_items;
	player->daily_shop_items = NULL;

    player->family_shop_items->clear();
    delete player->family_shop_items;
    player->family_shop_items = NULL;

    player->daily_charge_diamond_draw_cards_info->clear();
    delete player->daily_charge_diamond_draw_cards_info;
    player->daily_charge_diamond_draw_cards_info = NULL;

	player->smelter_gold_shop_items->clear();
	delete player->smelter_gold_shop_items;
	player->smelter_gold_shop_items = NULL;

	player->smelter_money_shop_items->clear();
	delete player->smelter_money_shop_items;
	player->smelter_money_shop_items = NULL;

    for (int i = 0; i < MAX_FIGHT_POS; i++) {
        player->fight_pet[i] = NULL;
    }

    player->buff_id_map->clear();
    delete player->buff_id_map;
    player->buff_id_map = NULL;

    player->suit_buff_map->clear();
    delete player->suit_buff_map;
    player->suit_buff_map = NULL;

    delete player->attrs;
    player->attrs = NULL;

    delete player->proto_queue;
    player->proto_queue = NULL;  
    
    delete player->temp_info.rank_info;
    player->temp_info.rank_info = NULL;

	delete player->temp_info.ai_nick;
	player->temp_info.ai_nick = NULL;

    delete player->temp_info.state_buf;
    player->temp_info.state_buf = NULL;

    delete player->temp_info.bit_vec;
    player->temp_info.bit_vec = NULL;

    delete player->temp_info.login_steps;
    player->temp_info.login_steps = NULL;

    delete player->package;
    player->package = NULL;

    delete player->temp_info.cache_dup_drop_prize;
    player->temp_info.cache_dup_drop_prize = NULL;

    delete player->temp_info.cache_prize_id;
    player->temp_info.cache_prize_id= NULL;

    delete player->temp_info.cache_prize_vec;
    player->temp_info.cache_prize_vec= NULL;

    delete player->temp_info.cache_question_vec;
    player->temp_info.cache_question_vec = NULL;

    delete player->temp_info.cache_tmp_vec;
    player->temp_info.cache_tmp_vec= NULL;

    delete player->temp_info.bless_team_member_info;
    player->temp_info.bless_team_member_info = NULL;

	delete player->temp_info.m_arena_index_info;
	player->temp_info.m_arena_index_info = NULL;

	delete player->temp_info.atk_no_map;
	player->temp_info.atk_no_map = NULL;

	delete player->temp_info.tmp_btl_info;
	player->temp_info.tmp_btl_info = NULL;

	delete player->temp_info.bless_team_info;
	player->temp_info.bless_team_info = NULL;

	delete player->temp_info.cache_string;
	player->temp_info.cache_string = NULL;

	delete player->temp_info.cache_out_string;
	player->temp_info.cache_out_string = NULL;

	delete player->temp_info.mon_cris_pets;
	player->temp_info.mon_cris_pets = NULL;

	delete player->temp_info.night_raid_pets;
	player->temp_info.night_raid_pets = NULL;

	delete player->temp_info.dirty_users;
	player->temp_info.dirty_users = NULL;

	delete player->temp_info.night_raid_op_pets;
	player->temp_info.night_raid_op_pets = NULL;

	delete player->task_info;
	player->task_info = NULL;

	delete player->rune_meseum;
	player->rune_meseum = NULL;

	delete player->m_tran_card;
	player->m_tran_card = NULL;

	delete player->home_data;
	player->home_data = NULL;

	delete player->achieve;
	player->achieve = NULL;

	delete player->expedtion;
	player->expedtion = NULL;

	delete player->nightraid;
	player->nightraid = NULL;

	delete player->friend_info;
	player->friend_info = NULL;

	delete player->title;
	player->title = NULL;

	delete player->mine_info;
	player->mine_info = NULL;

    return 0;

}

void PlayerManager::send_msg_to_all_player(uint32_t cmd, const google::protobuf::Message& message)
{
    std::vector<player_t*> player_list;

    get_player_list(player_list);

    for (uint32_t i = 0; i < player_list.size(); i++) {
        player_t* player = player_list[i]; 
        send_msg_to_player(player, cmd, message);
    }
}

void PlayerManager::send_err_to_all_player(int err)
{
    std::vector<player_t*> player_list;

    get_player_list(player_list);

    for (uint32_t i = 0; i < player_list.size(); i++) {
        player_t* player = player_list[i]; 
        send_err_to_player(player, player->cli_wait_cmd, err);
    }
}

void PlayerManager::send_msg_to_player_in_special_map(uint32_t cmd, uint32_t map_id, google::protobuf::Message& message)
{
    std::vector<player_t*> player_list;

    get_player_list(player_list);

    for (uint32_t i = 0; i < player_list.size(); i++) {
        player_t* player = player_list[i]; 
        if (player->cur_map_id == map_id) {
            send_msg_to_player(player, cmd, message);
        }
    }
}

void PlayerManager::on_battle_close()
{
    FOREACH(fd_users_, it) {
        player_t *player = it->second;
        if (!player->temp_info.dup_id) {
            continue;
        }
        //如果玩家在副本中给玩家发副本结束的消息
        player->temp_info.dup_state = PLAYER_DUP_END_LOSE;
        onlineproto::sc_0x020E_duplicate_notify_end noti_msg;
        send_msg_to_player(player, cli_cmd_cs_0x020E_duplicate_notify_end, noti_msg);
    }
}

void PlayerManager::on_battle_center_close()
{
    FOREACH(fd_users_, it) {
        player_t *player = it->second;
        if (!player->temp_info.dup_id) {
            continue;
        }
        duplicate_battle_type_t type = DupUtils::get_player_duplicate_battle_type(player);
        if (type == DUP_BTL_TYPE_PPVE
            || type == DUP_BTL_TYPE_RPVP) {
            //如果玩家在多人副本中给玩家发副本结束的消息
            player->temp_info.dup_state = PLAYER_DUP_END_LOSE;
            onlineproto::sc_0x020E_duplicate_notify_end noti_msg;
            send_msg_to_player(player, cli_cmd_cs_0x020E_duplicate_notify_end, noti_msg);
        }
    }
}

bool PlayerManager::is_player_has_old_machine(uint32_t userid)
{
    player_t *p = get_player_by_userid(userid);
    if (p && p->temp_info.is_old_machine) {
        return true;
    }
    return false;
}


