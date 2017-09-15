#ifndef __MAIL_UTILS_H__
#define __MAIL_UTILS_H__

#include "common.h"
#include "mail.h"
#include "player.h"

class MailUtils {
public:

	// 给玩家发新邮件到db，并同时推送给客户端
	static uint32_t add_player_new_mail(player_t* player, 
            const new_mail_t& mail, bool wait_db_ret=NO_WAIT_SVR);

	// 获取邮件附件
	static uint32_t get_mail_attach(player_t* player, 
            const std::vector<attachment_t>& attach_vec);

	static uint32_t serialize_prize_to_attach_string(
			const std::vector<cache_prize_elem_t> &prize_vec, 
			std::string &attachment);

    static uint32_t serialize_attach_to_attach_string(
            const std::vector<attachment_t> &attach_vec, 
            std::string &attachment);

    static uint32_t parse_attach_from_string(const string &attach_str, 
            std::vector<attachment_t> &attach_vec);

	static uint32_t pack_prize_to_mail_attach(
			const std::vector<cache_prize_elem_t>& prize_vec,
			std::vector<attachment_t>& attaches);

	//给别人发邮件
	static uint32_t send_mail_to_player(player_t* player, 
		uint32_t r_userid, uint32_t r_create_tm, 
		const new_mail_t &new_mail, bool wait_cb);
};
#endif
