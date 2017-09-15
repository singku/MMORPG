#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "common.h"
#include "pet.h"
#include "duplicate_entity.h"

struct player_t {
    player_t() {
        clear();
    }
    void clear() {
        uid  = 0;
        create_tm = 0;
        wait_cmd = 0;
        cur_seq = 0;
        fdsess = 0;
        cur_hp = 0;
        max_hp = 0;
        cur_tp = 0;
        max_tp = 0;
        x_pos = 0;
        y_pos = 0;
        heading = 0;
        player_dup_state = PLAYER_NOTHING;
        side = SIDE0;
        dup_entity = 0;
        line_id = 0;
        is_artifacial = false;
        is_captain = false;
		level = 0;
        rpvp_score = 0;
    }
    uint32_t uid;
    uint32_t create_tm;
    uint32_t wait_cmd;
    uint32_t cur_seq;
    uint32_t is_captain;
    fdsession_t *fdsess;
    std::map<uint32_t, Pet> fight_pets; //出战精灵
    std::map<uint32_t, Pet> switch_pets; //可切换的精灵(和fight_pets不相交)
    std::map<uint32_t, Pet> chisel_pets; //刻印的精灵
    std::vector<uint32_t> skills; // 玩家装备的技能

    string proto_equip_info; //序列化的玩家proto equip_info
    string proto_base_info; //序列化的玩家proto base_info;
    string proto_battle_info; //序列化的玩家proto battle_info;
    string proto_chisel_pet_info; //序列化的玩家proto chisel_pet_info;

	uint32_t tran_card_level; 	//携带进副本的变身卡牌星级
    uint32_t family_dup_boss_lv;    // 家族副本boss等级
    uint32_t family_dup_boss_hp;    // 家族副本boss当前血量
    uint32_t family_dup_boss_maxhp;    // 家族副本boss最大血量
	uint32_t card_id;
    uint32_t max_hp;
    uint32_t cur_hp; //当前血量在battle_info中有 但会实时改变
    uint32_t max_tp; //最大能量值
    uint32_t cur_tp;
    uint32_t level; //player的等级
    uint32_t rpvp_score;

    //玩家在地图的状态
    uint32_t x_pos;
    uint32_t y_pos;
    uint32_t heading;
    string state_bytes;

    //玩家在副本的状态
    player_duplicate_status_t player_dup_state;
    //玩家在副本的边
    player_duplicate_side_t side;
    duplicate_entity_t *dup_entity; //玩家在副本的实例
    
    uint32_t line_id;       // 玩家在的副本分线id

    uint32_t team; // 玩家team

    bool is_artifacial; //是否人造player 不是一个真实的player 默认为false pvep为true
};

int send_msg_to_player(player_t* player, uint32_t cmd, const google::protobuf::Message& msg);
int send_err_to_player(player_t* player, uint32_t cmd, int err);
int send_err_to_fdsess(fdsession_t *fdsess, uint32_t cmd, uint32_t uid, uint32_t seq, int err);
int relay_msg_to_player(player_t *player, uint32_t cmd, const google::protobuf::Message &msg);
int relay_notify_msg_to_player(player_t *player, uint32_t cmd, const google::protobuf::Message &msg);

#endif //__PLAYER_H__
