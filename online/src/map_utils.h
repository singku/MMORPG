
#ifndef MAP_UTILS_H
#define MAP_UTILS_H

#include "common.h"

#include "map_conf.h"
#include "map_user_manager.h"
#include "proto/client/common.pb.h"
#include "proto/client/pb0x01.pb.h"
//struct map_conf_t;


const uint32_t kMapIDChaosShiKong = 1037;
const uint32_t kMapIDChaosFenSe = 1039;
const uint32_t kMapIDChaosLvye = 1043;
const uint32_t kMapIDChaosZhange = 1038;
const uint32_t kMapIDChaosHuangmeng = 1040;
const uint32_t kMapIDGoldenDesertMoLian = 1009;
const uint32_t kMapIDGoldenDesertLiujin = 1007;
const uint32_t kMapIDGoldenDesertChuangchen = 1008;
const uint32_t kMapIDCateVolcanoShuiGuo = 1011;
const uint32_t kMapIDCateVolcanoMeiShi = 1010;
const uint32_t kMapIDCateVolcanoDangao = 1012;
const uint32_t kMapIDEverNightLiuyin = 1014;
const uint32_t kMapIDEverNightYeDong = 1015;
const uint32_t kMapIDToyIslandJimu = 1017;
const uint32_t kMapIDToyIslandWanju = 1018;
const uint32_t kMapIDDestroyGod = 1052; //神之暗灭
class MapUtils
{
public:
    static bool is_valid_map_id(uint32_t map_id);

    static bool is_mon_crisis_map_id(uint32_t map_id);

    static const map_conf_t* get_map_conf(uint32_t map_id);

    static bool is_player_in_map(player_t *player);

    static int enter_map(player_t* p, uint32_t map_id, uint32_t x = 0, uint32_t y = 0, uint32_t heading = 0);

    static int leave_map(player_t* p);

    static int send_msg_to_map_users(player_t *player, uint32_t cmd, 
            const google::protobuf::Message& message);

    static int send_msg_to_map_users_except_self(player_t *player, uint32_t cmd,
            const google::protobuf::Message& message);

    static int send_msg_to_all_map_users(uint32_t map_id, uint32_t cmd,
        const google::protobuf::Message& message);

    static int sync_player_change_state(player_t* player, onlineproto::cs_0x0106_player_change_state &player_change_state);

    static int sync_map_player_info(player_t *player, commonproto::map_player_change_reason_t reason);
};

#endif
