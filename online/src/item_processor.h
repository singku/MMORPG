#ifndef __ITEM_PROCESSOR_H__
#define __ITEM_PROCESSOR_H__

#include "common.h"
#include "cmd_processor_interface.h"
#include "interface_manager.h"

//物品使用的限定类型
enum restrict_func_type_t {
    //kItemRestrictPetList = 1,
};

struct item_slot_info_t;

class ItemRestrictInterface
{
public:
    /**
     * @brief  check_restrict 检查使用物品是否受限制
     *
     * @param res_args 限制的参数
     * @param cs_use_item 使用物品的协议
     *
     * @return 错误码 0 表示可以使用该物品
     */
    virtual uint32_t check_restrict(player_t* player, 
            const std::vector<int>& res_args, 
            onlineproto::cs_0x010C_use_item cs_use_item) = 0;
	virtual ~ItemRestrictInterface()
	{
	}
};

typedef std::map<uint32_t, CmdProcessorInterface *> item_funcs_map;
typedef std::map<uint32_t, ItemRestrictInterface *> item_restrict_funcs_map;

class ItemUseCmdProcessor : public CmdProcessorInterface
{
public:
    ~ItemUseCmdProcessor() {
        FOREACH(restrict_funcs_, it) {
            delete it->second; 
        }
        restrict_funcs_.clear();
    }
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
    void init();
private:
    onlineproto::cs_0x010C_use_item cli_in_;
    onlineproto::sc_0x010C_use_item cli_out_;
    InterfaceManager<CmdProcessorInterface> item_funcs_;    
    item_restrict_funcs_map restrict_funcs_;
    ItemRestrictInterface* get_restrict_processor(uint32_t res_type);
    void register_restrict_funcs(
            uint32_t res_type, ItemRestrictInterface* item_restrict_processor);
};

class ItemSmelterCmdProcessor : public CmdProcessorInterface {
public:
	ItemSmelterCmdProcessor();
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_when_smelter(player_t *player, uint32_t type,
			std::vector<item_slot_info_t>& items);
private:
	static int equip_smelter(player_t *player,
			std::vector<item_slot_info_t>& items);
	static int pet_fragment_smelter(player_t* player,
			std::vector<item_slot_info_t>& items);

	onlineproto::cs_0x016C_item_smelter cli_in_;
	onlineproto::sc_0x016C_item_smelter cli_out_;
	typedef int (*item_smelter_fun) (player_t* player,
			std::vector<item_slot_info_t>& items);
	std::map<uint32_t, item_smelter_fun> item_smelter_func_map_;
};

class ItemReInitEquipAttrCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x016D_item_reinit_equip_attr cli_in_;
	onlineproto::sc_0x016D_item_reinit_equip_attr cli_out_;
};

#endif
