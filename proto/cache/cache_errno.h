#ifndef __CACHE_ERRNO_H__
#define __CACHE_ERRNO_H__

enum cache_errno_t {
    cache_err_sys_err               = 40001,
    cache_err_cmd_not_finished      = 40002,
    cache_err_msg_format_err        = 40003,
    cache_err_cmd_not_found         = 40004,
    cache_err_user_not_found        = 40005,
    cache_err_req_time_up           = 40006, //请求超时 2s
    cache_err_cache_not_found       = 40007, //缓存数据未找到
};

#endif
