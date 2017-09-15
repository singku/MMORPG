#ifndef EQUIP_PROCESSOR_H
#define EQUIP_PROCESSOR_H

#include "proto/client/pb0x01.pb.h"
#include "cmd_processor_interface.h"

class EquipLevelUpCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
    onlineproto::cs_0x0117_equip_level_up cli_in_;
    onlineproto::sc_0x0117_equip_level_up cli_out_;
};

class HideFashionCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
    onlineproto::cs_0x013F_hide_fashion cli_in_;
    onlineproto::sc_0x013F_hide_fashion cli_out_;
};

class CultivateEquipCmdProcessor: public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int cultivate_equip(player_t *player, uint32_t type, uint32_t auto_flag);
private:
    onlineproto::cs_0x0146_cultivate_equip cli_in_;
    onlineproto::sc_0x0146_cultivate_equip cli_out_;
};

class EquipQuenchCmdProcessor: public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0147_equip_quench cli_in_;
    onlineproto::sc_0x0147_equip_quench cli_out_;
};

class SetEquipShowStatusCmdProcessor: public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0149_set_equip_show_status cli_in_;
    onlineproto::sc_0x0149_set_equip_show_status cli_out_;
};

class GetMountBattleValueCmdProcessor: public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x014C_get_mount_battle_value cli_in_;
    onlineproto::sc_0x014C_get_mount_battle_value cli_out_;
};
#endif
