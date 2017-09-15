#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "common.h"
#include "pb_master.h"
#include "robot.pb.h"

using google::protobuf::Message;

struct UniqRequest;

extern std::string config_full_path;

// <request_uniq_name, const pbcfg::Body*>
typedef std::map<std::string, const UniqRequest*> UniqNameMap;
typedef UniqNameMap::iterator UniqNameMapIter;

extern pbcfg::CfgRoot cfg_root;
extern int kMaxTotalClientNum;
extern UniqNameMap uniq_name_map;

extern std::map<uint32_t, std::string> cs_cmd_msg_map;
extern std::map<std::string, uint32_t> cs_msg_cmd_map;
extern std::map<uint32_t, std::string> sc_cmd_msg_map;
extern std::map<std::string, uint32_t> sc_msg_cmd_map;

extern std::vector<std::string> walk_vec;

bool ValidationRobotConfigs(const pbcfg::CfgRoot &cfg);
bool CollectConfigInfos(const pbcfg::CfgRoot &cfg);
bool init_robot_config();

inline uint32_t get_cmd_by_msg_name(string msg_name)
{
    if (cs_msg_cmd_map.count(msg_name) > 0) {
        return (cs_msg_cmd_map.find(msg_name))->second;
    } else if (sc_msg_cmd_map.count(msg_name) > 0) {
        return (sc_msg_cmd_map.find(msg_name))->second;
    } else if (msg_name == "dbproto.cs_create_role") {
        return 0x8201;
    }
    return 0;
}

inline string get_cs_msg_name_by_cmd(uint32_t cmd)
{
    if (cs_cmd_msg_map.count(cmd) > 0) {
        return (cs_cmd_msg_map.find(cmd))->second;
    } else if (cmd == 0x8201) {
        return "dbproto.cs_create_role";
    }

    return "";
}

inline string get_sc_msg_name_by_cmd(uint32_t cmd)
{
    if (sc_cmd_msg_map.count(cmd) > 0) {
        return (sc_cmd_msg_map.find(cmd))->second;
    } else if (cmd == 0x8201) {
        return "dbproto.sc_create_role";
    }

    return "";
}


// UniqRequest 针对每一个 uniq_name 记录请求数据信息
struct UniqRequest {
	~UniqRequest() { delete bodymsg; }
	UniqRequest(const pbcfg::Body *bdcfg, const Message *bdmsg)
		: bodycfg(bdcfg), bodymsg(bdmsg) { }

	const pbcfg::Body *bodycfg;
	const Message *bodymsg;
};


#endif // __CONFIG_H__
