#ifndef __CACHE_CMD_H__
#define __CACHE_CMD_H__

enum cache_cmd_t {
    /* 缓存服务器命令 获取用户缓存信息 */
    //online 对cache_agent的命令
    cache_cmd_ol_req_users_info    = 0x8A00,

    //cache agent对cache_svr的命令
    cache_cmd_ag_req_users_info    = 0x8C00,

    //online 对cache_svr的命令
    cache_cmd_set_cache             = 0x8C01,
    cache_cmd_get_cache             = 0x8C02,

    cache_cmd_set_user_info_outdate = 0x8C03,
};

#endif
