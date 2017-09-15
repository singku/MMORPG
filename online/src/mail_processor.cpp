
extern "C" {
#include <libtaomee/utils.h>
}

#include "mail_processor.h"
#include "attr_utils.h"
#include "proto/db/db_cmd.h"
#include "mail_utils.h"
#include "proto/client/cli_cmd.h"

#include "player.h"
#include "global_data.h"
#include "service.h"
#include "utils.h"
#include <string>


int GetMailListCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
	//向DB发协议拉取数据
    return g_dbproxy->send_msg(player, player->userid,player->create_tm, 
            db_cmd_mail_get_all, db_in_);
}

int GetMailListCmdProcessor::proc_pkg_from_serv(
        player_t* player, const char* body, int bodylen)
{ 
    PARSE_SVR_MSG(db_out_);
    cli_out_.Clear();
    cli_out_.mutable_mail_list()->CopyFrom(db_out_.mail_datas());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int DelMailListCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    db_in_.Clear();
 
    db_in_.mutable_mailids()->CopyFrom(cli_in_.mailid_list());
    g_dbproxy->send_msg(0, player->userid, player->create_tm,
            db_cmd_mail_del_by_ids, db_in_);

    cli_out_.mutable_mailid_list()->CopyFrom(cli_in_.mailid_list());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int ReadMailCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;

    cli_out_.Clear();
    db_in_.Clear();

    db_in_.set_mailid(cli_in_.mailid());
    db_in_.set_status(commonproto::UNGETATTACH);

	//向DB发协议改邮件状态
    g_dbproxy->send_msg(0, player->userid, player->create_tm,
            db_cmd_mail_set_status, db_in_);

	//回包给客户端
    cli_out_.set_mailid(cli_in_.mailid());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int GetMailAttachmentCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
    db_in_.Clear();
     
    db_in_.mutable_mailids()->CopyFrom(cli_in_.mailid_list());
    return g_dbproxy->send_msg(player, player->userid,player->create_tm, 
            db_cmd_mail_get_by_ids, db_in_);
}

int GetMailAttachmentCmdProcessor::proc_pkg_from_serv(
		player_t* player, const char* body, int bodylen)
{
    PARSE_SVR_MSG(db_out_);
	cli_out_.Clear();

	//取db读的邮件信息
	for (int i = 0; i < db_out_.mail_datas_size(); i++) {
        if (db_out_.mail_datas(i).status() == commonproto::GOTATTACH) {
            continue;
        }

		std::string attach_str = db_out_.mail_datas(i).attachment();
        std::vector<attachment_t> attach_vec;
        MailUtils::parse_attach_from_string(attach_str, attach_vec);
        int ret = MailUtils::get_mail_attach(player, attach_vec);
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd, cli_err_mail_attach_partly_got);
        }

		//标记领取附件
        db_set_status_.Clear();
        db_set_status_.set_mailid(db_out_.mail_datas(i).mailid());
        db_set_status_.set_status(commonproto::GOTATTACH);
		g_dbproxy->send_msg(NULL, player->userid,player->create_tm, 
				db_cmd_mail_set_status, db_set_status_);

		cli_out_.add_mailid_list(db_out_.mail_datas(i).mailid());
	}

	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

//此协议仅供测试db代码使用，废非业务逻辑部分
int AddMailCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	PARSE_MSG;
    new_mail_t new_mail;
    new_mail.sender = cli_in_.sender();
    new_mail.title = cli_in_.title();
    new_mail.content = cli_in_.content();
	new_mail.attachment = cli_in_.attachment();
    MailUtils::add_player_new_mail(player, new_mail);

	return 0;
}
