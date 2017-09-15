#ifndef ITEM_FUNC_PROCESSOR_H
#define ITEM_FUNC_PROCESSOR_H

#include "common.h"
#include "cmd_processor_interface.h"

//玩家经验值
class PlayerAddExpItemFuncProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player,const char*body, int bodylen);
private:
    onlineproto::cs_0x010C_use_item cli_in_;
    onlineproto::sc_0x010C_use_item cli_out_;
};

//玩家血瓶
class PlayerAddHpItemFuncProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player,const char*body, int bodylen);
private:
    onlineproto::cs_0x010C_use_item cli_in_;
    onlineproto::sc_0x010C_use_item cli_out_;
};

//玩家装备
class PlayerEquipArmItemFuncProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player,const char*body, int bodylen);
private:
	onlineproto::cs_0x010C_use_item cli_in_;
    onlineproto::sc_0x010C_use_item cli_out_;
};

//玩家一键装备
class PlayerOneKeyEquipArmItemFuncProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player,const char*body, int bodylen);
private:
	onlineproto::cs_0x010D_one_key_equip cli_in_;
    onlineproto::sc_0x010D_one_key_equip cli_out_;
};

//精灵学习力
class PetAddEffortItemFuncProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player,const char*body, int bodylen);
private:
	onlineproto::cs_0x010C_use_item cli_in_;
	onlineproto::sc_0x010C_use_item cli_out_;	
};

//精灵经验
class PetAddExpItemFuncProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player,const char*body, int bodylen);
private:
    onlineproto::cs_0x010C_use_item cli_in_;
    onlineproto::sc_0x010C_use_item cli_out_;    
};

//精灵血瓶
class PetAddHpItemFuncProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player,const char*body, int bodylen);
private:
    onlineproto::cs_0x010C_use_item cli_in_;
    onlineproto::sc_0x010C_use_item cli_out_;
};

//精灵天赋石升品级
class PetImproveTalentItemFuncProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player,const char*body, int bodylen);
private:
    onlineproto::cs_0x010C_use_item cli_in_;
    onlineproto::sc_0x010C_use_item cli_out_;
};

//增加体力道具
class PlayerAddVpItemFuncProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x010C_use_item cli_in_;
    onlineproto::sc_0x010C_use_item cli_out_;
};

//开宝箱
class PlayerOpenBoxItemFuncProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x010C_use_item cli_in_;
    onlineproto::sc_0x010C_use_item cli_out_;
};
#endif
