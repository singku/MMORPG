#ifndef __MACRO_UTILS_H__
#define __MACRO_UTILS_H__

#define LOCK_SVR_MSG \
    g_svr_msg_locked = true;

#define UNLOCK_SVR_MSG \
    g_svr_msg_locked = false;

#define LOCK_SVR_CHECK \
    do { \
        if (g_svr_msg_locked) { \
            return 0; \
        } \
    } while (0)

#define LOCK_CLI_MSG \
    g_cli_msg_locked = true;

#define UNLOCK_CLI_MSG \
    g_cli_msg_locked = false;

#define LOCK_CLI_CHECK \
    do { \
        if (g_cli_msg_locked) { \
            return 0; \
        } \
    } while (0)

#define LOCK_MSG \
    LOCK_CLI_MSG;\
    LOCK_SVR_MSG;\

#define UNLOCK_MSG \
    UNLOCK_CLI_MSG;\
    UNLOCK_SVR_MSG;

#define LOCK_CHECK \
    LOCK_CLI_CHECK;\
    LOCK_SVR_CHECK;

#define GET_A(type) \
    AttrUtils::get_attr_value(player, type)

#define SET_A(type, value) \
    AttrUtils::set_single_attr_value(player, type, value)

#define ADD_A(type, value) \
    AttrUtils::add_attr_value(player, type, value)

#define SUB_A(type, value) \
    AttrUtils::sub_attr_value(player, type, value)

#define GET_A_MAX(type) \
    AttrUtils::get_attr_max_limit(player, type)

#define GET_A_INIT(type) \
    AttrUtils::get_attr_initial_value(player, type)

#define INCR_A_TO(player, type, value) do { if (GET_A(type) < value) { SET_A(type, value);}} while (0)

#define FOREACH(container, it) \
    for(typeof((container).begin()) it=(container).begin(); it!=(container).end(); ++it)

#define REVERSE_FOREACH(container, it) \
    for(typeof((container).rbegin()) it=(container).rbegin(); it!=(container).rend(); ++it)

#define FOREACH_NOINCR_ITER(container, it) \
       for(typeof((container).begin()) it=(container).begin(); it!=(container).end();) 

#define REVERSE_FOREACH_NOINCR_ITER(container, it) \
        for(typeof((container).rbegin()) it=(container).rbegin(); it!=(container).rend();)

//必须在setup_timer之后才能使用
#define NOW()   (get_now_tv()->tv_sec)

#define IF_ERR_THEN_RET(func) do { \
    int ret = func; \
    if (ret) { \
        return send_err_to_player(player, player->cli_wait_cmd, ret);\
    } \
} while (0)

#define RET_ERR(err) \
    do { \
        return send_err_to_player(player, player->cli_wait_cmd, err); \
    } while(0)

#define SEND_ERR(err) \
    do { \
        send_err_to_player(player, player->cli_wait_cmd, err); \
    } while(0)

#define RET_MSG \
    do { \
        return send_msg_to_player(player, player->cli_wait_cmd, cli_out_); \
    } while (0)

#endif
