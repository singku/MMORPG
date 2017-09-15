#ifndef __SERVER_H__
#define __SERVER_H__

#include "common.h"
#include "proto_queue.h"

#define TELCOM  (0)
#define NETCOM  (1)

#define MAX_CACHE_PROTO (10000)

class server_basic_info_t {
public:
    server_basic_info_t() {
        server_id_ = 0;
        online_id_ = 0;
        server_type_ = 0;
        listen_port_ = 0;
        fdsess_ = 0;
        waiting_cmd_ = 0;
        waiting_serv_cmd_ = 0;
        idc_zone_ = 0;
		serv_cmd_ = 0;
        name_.clear();
    }
    uint32_t server_id_;
    uint32_t online_id_;
    /*online or other*/
    uint32_t server_type_;
    /*服务器的监听端口*/
    uint16_t listen_port_;
    //服务器绑定的IP(fdsess中的ip可能是内网IP)
    uint32_t host_ip_;
    /*fd ip port*/
    fdsession_t *fdsess_;
    uint32_t waiting_cmd_;
    uint32_t waiting_serv_cmd_;
    uint32_t idc_zone_;
	//请求服务端的 serv_cmd
	uint32_t serv_cmd_;
    string name_;
};

class server_t {
public:
    server_t() {
        total_player_num_ = 0;
        vip_player_num_ = 0;
        proto_queue = new ProtoQueue(MAX_CACHE_PROTO);
    }
    ~server_t() {
        delete proto_queue;
    }
public: //inline funcs
    inline void clear_player_num() {
        total_player_num_ = 0;
        vip_player_num_ = 0;
    }
    inline void inc_player_num(bool is_vip) {
        total_player_num_ ++;
        if (is_vip) vip_player_num_ ++;
    }
    inline void dec_player_num(bool is_vip) {
        if (total_player_num_ == 0) return;
        total_player_num_ --;
        if (!is_vip || vip_player_num_ == 0) return;
        vip_player_num_ --;
    }
    inline uint32_t get_total_player_num() {
        return total_player_num_;
    }
    inline uint32_t get_total_vip_player_num() {
        return vip_player_num_;
    }

public://getters
    inline uint32_t server_id() { return basic_.server_id_; }
    inline uint32_t online_id() { return basic_.online_id_; }
    inline uint32_t server_type() { return basic_.server_type_; }
    inline uint16_t listen_port() { return basic_.listen_port_; }
    inline uint32_t host_ip() { return basic_.host_ip_; }
    inline fdsession_t *fdsess() { return basic_.fdsess_; }
    inline uint32_t waiting_cmd() { return basic_.waiting_cmd_; }
    inline uint32_t waiting_serv_cmd() { return basic_.waiting_serv_cmd_; }
    inline uint32_t idc_zone() { return basic_.idc_zone_; }
	inline uint32_t serv_cmd() { return basic_.serv_cmd_; }
    inline string name() { return basic_.name_; }

public://setters
    inline void set_server_id(uint32_t svrid) { basic_.server_id_ = svrid; }
    inline void set_online_id(uint32_t olid) { basic_.online_id_ = olid; }
    inline void set_server_type(uint32_t type) { basic_.server_type_ = type; }
    inline void set_listen_port(uint16_t port) { basic_.listen_port_ = port; }
    inline void set_host_ip(uint32_t ip) { basic_.host_ip_ = ip; }
    inline void set_fdsess(fdsession_t *fdsess) { basic_.fdsess_ = fdsess; }
    inline void set_waiting_cmd(uint32_t cmd) { basic_.waiting_cmd_ = cmd; }
    inline void set_waiting_serv_cmd(uint32_t cmd) { basic_.waiting_serv_cmd_ = cmd; }
    inline void set_idc_zone(uint32_t zone) { basic_.idc_zone_ = zone; }
    inline void set_name(string name) { basic_.name_ = name; }
    inline void clear_waiting_cmd() { basic_.waiting_cmd_ = 0; }
    inline void clear_waiting_serv_cmd() { basic_.waiting_serv_cmd_ = 0; }
	inline void set_serv_cmd(uint32_t cmd) {basic_.serv_cmd_ = cmd; }

public://处理的业务编号
    uint32_t process_seq;
    uint32_t process_uid;
    uint32_t process_u_create_tm;
    string cache_string;
    timer_struct_t *svr_timer;
    ProtoQueue* proto_queue;
private:
    /*基本信息*/
    server_basic_info_t basic_;
    /*总的玩家数*/
    uint32_t total_player_num_;
    /*VIP玩家数*/
    uint32_t vip_player_num_;
};

#endif
