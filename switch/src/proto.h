#ifndef PROTO_H
#define PROTO_H

#include "proto/common/svr_proto_header.h"

struct platform_recharge_diamond_req_t {
    uint32_t dest_user; //充值目标用户米米号
    uint32_t dest_server_id; //区服号
    uint32_t type;   //消费类型
    uint32_t diamond_cnt;  //充值增加的钻石数量
    uint32_t trans_id; //充值交易号
    uint32_t dest_create_tm; // 用户的create_tm 
    uint32_t add_data_2; // 附加信息2 
} __attribute__((packed));

struct platform_recharge_diamond_ack_t {
	uint32_t trans_id; //充值交易号(原样返回)
};

//平台请求的包体
struct platform_get_role_info_req_t {
    uint32_t dest_user; //用户的米米号
    uint32_t time; //请求的时间（如果没有配置key的该字段为空）
    char sign[32];
    uint32_t server_id; //指定的区服(合服前玩家所在的服务器)
} __attribute__((packed));


//返回包体。目前是一服一角色
struct platform_get_role_info_ack_t {
	uint32_t level;
	char name[150];
} __attribute__((packed));


struct plat_role_info_t {
    uint32_t u_create_tm;   //角色创建时间戳
    uint32_t init_server_id; //初始服务器ID
    uint32_t cur_server_id; //当前服务器ID
    uint32_t level; //等级
    char role_name[64]; //名字
} __attribute__((packed));

//平台拉取米米号对应的角色信息请求包体
struct platform_get_role_info_req_ex_t {
    //包头中有米米号
    uint32_t init_server_id; //初始服ID 传0则拉取米米号在各服的所有角色
} __attribute__((packed));

struct platform_get_role_info_rsp_t {
    uint32_t count; //角色的数量
    //后面跟着count个plat_role_info_t
} __attribute__((packed));

//平台请求角色在某时间段是否登录过
struct platform_if_role_login_req_t {
    uint32_t from_tm; //从tm unix_timestamp
    uint32_t to_tm; //至tm unix_timestamp
} __attribute__((packed));

struct platform_if_role_login_rsp_t {
    uint8_t result;//0 否 1是
} __attribute__((packed));

#endif
