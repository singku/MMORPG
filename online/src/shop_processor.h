#ifndef __SHOP_PROCESSOR_H__
#define __SHOP_PROCESSOR_H__

#include "common.h"
#include "cmd_processor_interface.h"

class BuyProductCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x012E_buy_product cli_in_;
    onlineproto::sc_0x012E_buy_product cli_out_;
};

class SellProductCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x012F_sell_items cli_in_;
    onlineproto::sc_0x012F_sell_items cli_out_;
};

class ShopRefreshCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0131_shop_refresh cli_in_;
    onlineproto::sc_0x0131_shop_refresh cli_out_;
};

class ExchangeCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
    onlineproto::cs_0x0136_exchange cli_in_;
    onlineproto::sc_0x0136_exchange cli_out_;
};

class ShopGetProductInfoCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0144_shop_get_product_info cli_in_;
    onlineproto::sc_0x0144_shop_get_product_info cli_out_;
};
#endif
