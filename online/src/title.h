#ifndef __TITLE_H__
#define __TITLE_H__

#include "common.h"
#include "cmd_processor_interface.h"

struct player_t;

struct title_info_t
{
	title_info_t() {
		title_id = 0;
		get_time = 0;
	}
	uint32_t title_id;
	uint32_t get_time;
};

class Title
{
public:
	typedef std::map<uint32_t, title_info_t> TitleMap;

	uint32_t add_one_title(player_t* player, 
			title_info_t& title_info, 
			bool sync_cli = true);

	uint32_t insert_one_title_to_memory(title_info_t& title_info);
	uint32_t get_all_titles(TitleMap&  titlemap);
	uint32_t get_title_info(uint32_t title_id, title_info_t& title_info);
	uint32_t pack_all_titles(commonproto::title_info_list_t* titles_ptr);
	uint32_t sync_all_titles(player_t* player, bool sync_cli = true);
	//检查称号是否过期
	bool check_title_expire(uint32_t title_id);
	bool check_player_has_this_title(uint32_t title_id);

	uint32_t erase_expire_titles(player_t* player);
private:
	TitleMap title_map_;
};

class GetTitleCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0615_get_titles cli_in_;
	onlineproto::sc_0x0615_get_titles cli_out_;
	rankproto::sc_get_user_multi_rank rank_out_;
};

class EquipTitleCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0617_equip_title cli_in_;
	onlineproto::sc_0x0617_equip_title cli_out_;
};

class UnloadTitleCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0619_unload_title cli_in_;
	onlineproto::sc_0x0619_unload_title cli_out_;
};

#endif
