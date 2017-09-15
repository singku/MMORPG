#ifndef __RUNE_PROCESSOR_H__
#define __RUNE_PROCESSOR_H__
#include "../../proto/client/pb0x03.pb.h"
#include "cmd_processor_interface.h"


//使用经验瓶中的经验来升级符文
class UpRuneByExpCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x031B_uprune_by_exp cli_in_;
	onlineproto::sc_0x031B_uprune_by_exp cli_out_;
};

//符文吞噬
class SwallowRuneCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x031C_swallow_rune cli_in_;
	onlineproto::sc_0x031C_swallow_rune cli_out_;
};

//装备符文
class EquipRuneCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x031D_equip_rune cli_in_;
	onlineproto::sc_0x031D_equip_rune cli_out_;
};

/*
//卸下符文
class UnEquipRuneCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x032E_unquip_rune cli_in_;
	onlineproto::sc_0x032E_unquip_rune cli_out_;
};
*/

//背包之间的转换(收藏背包 与 转换背包)
class ChgRunePackCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x031E_chgrune_pack cli_in_;
	onlineproto::sc_0x031E_chgrune_pack cli_out_;
};

/*
//转化背包 -> 收藏背包
class CollectRuneCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0330_col_rune cli_in_;
	onlineproto::cs_0x0330_col_rune cli_out_;
};
*/

//符文转化为经验瓶中经验
class RunetoExpBottleCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x031F_rune_to_bottleexp cli_in_;
	onlineproto::sc_0x031F_rune_to_bottleexp cli_out_;
};

//符文召唤
class RuneCallCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0320_rune_call cli_in_;
	onlineproto::sc_0x0320_rune_call cli_out_;
};

//一键拾取符文
class GetRuneFromRpToTpCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0321_get_rune_from_runepack_to_tranpack cli_in_;
	onlineproto::sc_0x0321_get_rune_from_runepack_to_tranpack cli_out_;
};

//卖出符文碎片
class SellGrayRuneCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0322_sell_gray_rune cli_in_;
	onlineproto::sc_0x0322_sell_gray_rune cli_out_;
};

//符文一键转化为经验瓶中经验值
class OneKeyToBottleCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0324_rune_one_key_to_bottleexp cli_in_;
	onlineproto::sc_0x0324_rune_one_key_to_bottleexp cli_out_;
};

class UnlockPetRunePosCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0327_unlock_pet_rune_pos cli_in_;
	onlineproto::sc_0x0327_unlock_pet_rune_pos cli_out_;
};

class UnlockRuneCollectBagCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0329_unlock_rune_collect_bag cli_in_;
	onlineproto::sc_0x0329_unlock_rune_collect_bag cli_out_;
};

class BuyRuneTransPackPageCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0337_buy_rune_trans_pack_page cli_in_;
	onlineproto::sc_0x0337_buy_rune_trans_pack_page cli_out_;
};

#endif
