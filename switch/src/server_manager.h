#ifndef __SERVER_MANAGER_H__
#define __SERVER_MANAGER_H__

#include "common.h"
#include "singleton.h"
#include "server.h"
#include "statlogger/statlogger.h"

extern const uint32_t TEMP_SVR_TYPE;

class server_manager_t {
public:
	typedef std::map<uint32_t, server_t*> svrid_to_server_map_t;
	typedef svrid_to_server_map_t::iterator svrid_to_server_map_iter_t;
    typedef svrid_to_server_map_t::const_iterator svrid_to_server_map_const_iter_t;

    typedef std::map<int, server_t*> fd_to_server_map_t;
    typedef std::map<int, server_t*>::iterator fd_to_server_map_iter_t;

	server_t *create_new_server(server_basic_info_t *info);
    server_t *create_temp_server(fdsession_t *fdsess);

	void add_server(server_t *svr);
    void add_temp_server(server_t *svr);
	void del_server(server_t *svr);

	//根据给定的服号，给该服里所有的线发送命令号为cmd的message协议
	uint32_t transmit_msg_to_dest_svr(uint32_t svr_id, uint16_t cmd,
			google::protobuf::Message& message);

public: //inline funcs
    inline server_t *get_server_by_olid(uint32_t olid) {
        svrid_to_server_map_iter_t iter = svrid_to_server_map_.find(olid);
        if (iter == svrid_to_server_map_.end()) { return 0; }
        return iter->second;
    }
    inline server_t *get_server_by_fd(int fd) {
        fd_to_server_map_iter_t iter = fd_to_server_map_.find(fd);
        if (iter == fd_to_server_map_.end()) { return 0; }
        return iter->second;
    }
    inline std::set<server_t*> get_onlines_by_svrid(uint32_t svr_id) {
        std::set<server_t*> svrs;
        FOREACH(svrid_to_server_map_, it) {
            server_t *svr = it->second;
            if (svr->server_id() == svr_id) {
                svrs.insert(svr);
            }
        }
        return svrs;
    }
	inline void add_svrid_to_svridset(uint32_t svr_id) {
		if (svrid_set_.count(svr_id) == 0) {
			svrid_set_.insert(svr_id);
		}
	}
public:
    const svrid_to_server_map_t &all_server_map() {return svrid_to_server_map_;}
	const std::set<uint32_t>& all_svrid_set() { return svrid_set_;}
private:
	//其实应该是olid_to_server_map_t : key==> online_id; value==> server*
	svrid_to_server_map_t svrid_to_server_map_;
    fd_to_server_map_t fd_to_server_map_;
	//当前所拥有的服的集合:一个服有N个online_id
	std::set<uint32_t> svrid_set_;
};

typedef singleton_default<server_manager_t> server_manager_singleton_t;
#define SERVER_MGR (server_manager_singleton_t::instance())

extern std::map<int, server_t*> g_pending_proto_svrs;
extern std::map<uint32_t, StatLogger*> g_stat_logger_map;
StatLogger *get_stat_logger(uint32_t svr_id);

#endif // __SERVER_MANAGER_H__
