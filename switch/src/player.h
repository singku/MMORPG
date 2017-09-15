#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "common.h"

/*
 * player结构太大(甚至有可能改成class),
 * 因此专用一个结构体来保存用户登录时的注册信息, 为 add_player 提供参数
 */
class player_basic_t {
public:
    /* 角色ID */
	uint32_t			uid_;
    /* 角色创建时间 */
	int32_t				create_tm_;
    /* 是否VIP */
    uint8_t             is_vip_;
};

/**
 * @brief player_t 管理一个玩家的所有数据
 * 注意: 所有数据按模块划分, 不要直接在 player_t 里加子模块内的字段
 */
class player_t {
public: 
    player_t() {};
    ~player_t() {}; 

public: //getters
    inline uint32_t uid() { return basic_.uid_; }
    inline int32_t create_tm() { return basic_.create_tm_; }
    inline uint8_t is_vip() { return basic_.is_vip_; }
    inline uint32_t server_id() { return server_id_; }
    inline uint32_t online_id() { return online_id_; }


public: //setters
    inline void set_server_id(uint32_t svrid) { server_id_ = svrid; }
    inline void set_online_id(uint32_t onlineid) { online_id_ = onlineid; }
    inline void set_uid(uint32_t uid) { basic_.uid_ = uid; }
    inline void set_create_tm(int32_t create_tm) { basic_.create_tm_ = create_tm; }
    inline void set_is_vip(uint8_t is_vip) { basic_.is_vip_ = is_vip; }

private:
	/*! 基础数据 */
	player_basic_t		basic_;
    /* 目前位于的服务器ID*/
    uint32_t            server_id_;
    /* */
    uint32_t            online_id_;
};

#endif //__PLAYER_H__
