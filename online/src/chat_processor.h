#ifndef CHAT_PROCESSOR_H
#define CHAT_PROCESSOR_H
#include "common.h"
#include "cmd_processor_interface.h"
#include "proto/client/pb0x06.pb.h"
#include "prize_conf.h"
#include "player.h"

const int kWorldChatCD = 10;// 世界聊天cd
const int kMapChatCD = 1; // 附近聊天cd
const int kBroadcastCD = 5; // 喇叭cd

const uint32_t BroadcastID = 31000; //喇叭道具ID

class SayCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
	int say_map(player_t* player);
	int say_world(player_t* player);
	int say_family(player_t* player);
    int proc_pkg_from_serv_aft_get_family_olmembers(
        player_t *player, const char *body, int bodylen);
	onlineproto::cs_0x060A_say cli_in_;
	onlineproto::sc_0x060A_say cli_out_;
	char content_[4096];
};

class ShowItemCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
	onlineproto::cs_0x060B_show_item cli_in_;
	onlineproto::sc_0x060B_show_item cli_out_;
};

class ShowPetCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
	onlineproto::cs_0x060D_show_pet cli_in_;
	onlineproto::sc_0x060D_show_pet cli_out_;
};


class BroadcastCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
	int broadcast(player_t* player);
	onlineproto::cs_0x060E_broadcast cli_in_;
	onlineproto::sc_0x060E_broadcast cli_out_;
	char content_[4096];
};

class SystemNotify {
public:
	// 某玩家获得高级奖励，系统将奖励数据发给客户端，客户端根据奖励原因，重新封包
	static int SystemNotifyForPrize(player_t* player, 
            const google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> *award_elems,
            commonproto::system_noti_reason_t reason);

	//通知缓存奖励
	static int SystemNotifyForCachePrize(player_t* player, 
			const cache_prize_elem_t &data, commonproto::system_noti_reason_t reason);


    static int SystemNotifyNormal(std::string &info, uint32_t mcast_type = 0, uint32_t map_id = 0);
};

class SendHyperLinkMsgCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
	onlineproto::cs_0x0623_send_hyperlink_msg cli_in_;
	onlineproto::sc_0x0623_send_hyperlink_msg cli_out_;
};
#endif
