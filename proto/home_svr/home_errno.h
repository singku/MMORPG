#ifndef __HOME_ERRNO_H__
#define __HOME_ERRNO_H__

enum home_errno_t {
    home_err_sys_err                = 30001,
    home_err_cmd_not_finished       = 30002,
    home_err_msg_format_err         = 30003,
    home_err_cmd_not_found          = 30004,
    home_err_cmd_overtime           = 30005,
	home_err_must_in_host_hm_ope	= 30006, //必须在屋主家才能进行操作
	home_err_host_not_exsit 		= 30007, //小屋屋主不存在
};

#endif
