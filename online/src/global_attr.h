#ifndef GLOBAL_ATTR_H
#define GLOBAL_ATTR_H

#include "proto/db/dbproto.attr.pb.h"
#include "proto/db/dbproto.data.pb.h"
#include "cmd_processor_interface.h"
#include "service.h"
#include "global_data.h"
#include "player.h"

// enum global_attr_type_t
// {
    // kGlobalAttrlottery = 0, //抽卡物品
// };

struct global_attr_config_t
{
    global_attr_config_t() {
        id = 0;
		time_id = 0;
        max = 0;
		target_id = 0;
		type = 0;
    }
    uint32_t id;
	uint32_t time_id;
	uint32_t type;
	uint32_t target_id;
    uint32_t max;
};

struct global_attr_info_t
{
	uint32_t type; 
	uint32_t subtype;
	uint32_t value;
};

struct global_attr_chg_info_t
{
	uint32_t type; 
	uint32_t subtype;
	uint32_t diff;
	uint32_t limit;
};

class GlobalAttrSet
{
public:

    inline uint32_t get(uint32_t type, uint32_t subtype)
    {
        uint64_t key = gen_key(type, subtype);

        return attrs_[key];
    }

    inline void set(uint32_t type, uint32_t subtype, uint32_t value)
    {
        uint64_t key = gen_key(type, subtype); 

        attrs_[key] = value;
    }

    inline void clear()
    {
        attrs_.clear();  
    }

    inline void get_all(std::vector<global_attr_info_t>& attrs)
    {
        std::map<uint64_t, uint32_t>::iterator ptr = 
            attrs_.begin();

        for (; ptr != attrs_.end(); ptr++) {
            global_attr_info_t info;
            uint64_t key = ptr->first; 
            uint32_t value = ptr->second;

            parse_key(key, info.type, info.subtype);
            info.value = value;

            attrs.push_back(info);
        }
    }

private:

    inline uint64_t gen_key(uint32_t type, uint32_t subtype)
    {
        return ((uint64_t)type << 32) + (uint64_t)subtype;
    }

    inline void parse_key(uint64_t key, uint32_t& type, uint32_t& subtype)
    {
        type = key >> 32;
        subtype = key & (uint64_t)((1ULL << 32) - 1);
    }

    std::map<uint64_t, uint32_t> attrs_;
};

class GlobalAttrUtils
{
public:
	// 增加全局属性，type 为属性同时对应time_config id，subtype为time_config tid，limit为上限值
    static inline int add_attr_with_limit(player_t* player, const uint32_t type, const uint32_t subtype, const uint32_t value, const uint32_t limit)
    {
		if(!TimeUtils::is_current_time_valid(type, subtype)){
			return cli_err_activity_time_invalid; 
		}
        dbproto::cs_update_global_attr cs_update_global_attr;
        dbproto::global_attr_op_list_t* op_list = 
            cs_update_global_attr.mutable_list();
        dbproto::global_attr_op_info_t* op_info = 
            op_list->add_op_list();

        op_info->set_op(dbproto::GLOBAL_ATTR_OP_ADD);
        op_info->set_type(type);
        op_info->set_subtype(subtype);
        op_info->set_value(value);
        op_info->set_limit(limit);

        return g_dbproxy->send_msg(player, player->userid, player->create_tm,
                db_cmd_update_global_attr, cs_update_global_attr);
    }

	static int parse_get_attr_ack(const char* body, int bodylen,
			GlobalAttrSet& attrs)
	{
		dbproto::sc_get_global_attr sc_get_global_attr;

		if (parse_message(body, bodylen, &sc_get_global_attr)) {
			return cli_err_sys_err;  
		}

		const dbproto::global_attr_list_t& list = sc_get_global_attr.list();

		int attr_list_size = list.attr_list_size(); 

		attrs.clear();
		for (int i = 0; i < attr_list_size; i++) {
			const dbproto::global_attr_info_t& attr_info = 
				list.attr_list(i);
			attrs.set(attr_info.type(), attr_info.subtype(), attr_info.value());
		}

		return 0;
	}

    static inline int get_attr_wait(player_t* player, 
            uint32_t type, uint32_t subtype)
    {
        dbproto::cs_get_global_attr cs_get_global_attr; 
        dbproto::global_attr_list_t* list = cs_get_global_attr.mutable_type_list();
        dbproto::global_attr_info_t* attr_info = list->add_attr_list();

        attr_info->set_type(type);
        attr_info->set_subtype(subtype);
        attr_info->set_value(0);
		cs_get_global_attr.set_server_id(g_server_id);

        return g_dbproxy->send_msg(player, player->wait_svr_cmd, player->create_tm,
                db_cmd_get_global_attr, cs_get_global_attr);
    }

    static int get_attrs_wait(player_t* player,
            const std::vector<uint32_t>& type_list,
            const std::vector<uint32_t>& subtype_list)
    {
        dbproto::cs_get_global_attr cs_get_global_attr; 
        dbproto::global_attr_list_t* list = cs_get_global_attr.mutable_type_list();

        if (type_list.size() != subtype_list.size()) {
            return -1; 
        }

        for (uint32_t i = 0; i < type_list.size(); i++) {

            dbproto::global_attr_info_t* attr_info = list->add_attr_list();
            uint32_t type = type_list[i]; 
            uint32_t subtype = subtype_list[i];
            
            attr_info->set_type(type);
            attr_info->set_subtype(subtype);
            attr_info->set_value(0);
        }

        return g_dbproxy->send_msg(player, player->wait_svr_cmd, player->create_tm,
                db_cmd_get_global_attr, cs_get_global_attr);
    }

	static const global_attr_config_t* get_global_attr_config(global_attr_type_t attr_type)
	{
		std::map<uint32_t, global_attr_config_t>::iterator it = g_global_attr_configs.find(attr_type);

		if (it == g_global_attr_configs.end()) {
			return NULL; 
		} else {
			return &it->second;    
		}

		return NULL;
	}
};

#endif
