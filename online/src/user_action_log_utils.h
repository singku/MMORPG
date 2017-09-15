#ifndef USER_ACTION_LOG_UTILS_H
#define USER_ACTION_LOG_UTILS_H


#include "proto/db/dbproto.user_action_log.pb.h"

struct item_change_log_info_t
{
    uint32_t item_id; //target_id
    int chg_val; //value
    uint32_t orig_val; //extra1
    uint32_t chg_reason; //extra2
    uint32_t slot_id; //extra3
};

struct attr_change_log_info_t
{
    uint32_t attr_type; //target_id
    int chg_val; //value
    uint32_t orig_val; //extra1
    uint32_t chg_reason; //extra2
};


class UserActionLogUtils
{
public:
    //原生写日志
    static void write_db_log(player_t *player, 
            dbproto::action_log_type_t type,
            uint32_t target_id,
            int32_t chg_val,
            uint32_t extra1,
            uint32_t extra2,
            uint32_t extra3);

    //物品增减改变日志
    static void log_item_num_change(player_t* player, 
            std::vector<item_change_log_info_t>& log_info_list);
    //属性改变日志
    static void log_attr_change(player_t* player,
            std::vector<attr_change_log_info_t>& log_info_list);
private:
};



#endif
