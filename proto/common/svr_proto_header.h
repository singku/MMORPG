#ifndef SVR_PROTO_HEADER_H
#define SVR_PROTO_HEADER_H

struct comm_proto_header_t
{
    uint32_t len;
    uint32_t seq;
    uint16_t cmd;
    uint32_t ret;
    uint32_t uid;
} __attribute__((packed));

struct svr_proto_header_t
{
    uint32_t len;
    uint32_t seq;
    uint16_t cmd;
    uint32_t ret;
    uint32_t uid;           //请求的uid
    uint32_t u_create_tm;   //请求的uid的create_tm
    uint32_t cb_uid;        //回调需要的uid//主要在小屋中用
    uint32_t cb_u_create_tm;//回调需要的uid的create_tm//主要在小屋服务器用
} __attribute__((packed));

struct act_proto_header_t
{
    uint32_t len;
    uint32_t seq;
    uint16_t cmd;
    uint32_t ret;
    uint32_t uid;
} __attribute__((packed));

//角色信息结构
struct role_info_t {
    uint32_t userid;
    uint32_t u_create_tm;
};

inline uint64_t comp_u64(uint32_t high, uint32_t low) 
{
    uint64_t a = high;
    a = a << 32;
    a = a + low;
    return a;
}

inline void decomp_u64(uint64_t val, uint32_t &high, uint32_t &low) 
{
    high = val >> 32;
    low = val;
}

inline role_info_t role_key_to_role(uint64_t key)
{
    role_info_t tmp;
    decomp_u64(key, tmp.userid, tmp.u_create_tm);
    return tmp;
}

inline role_info_t get_role(uint32_t uid, uint32_t u_create_tm)
{
    role_info_t tmp;
    tmp.userid = uid;
    tmp.u_create_tm = u_create_tm;
    return tmp;
}

#define ROLE_KEY(role) comp_u64(role.userid, role.u_create_tm)
#define ROLE_KEY_PROTO(role) comp_u64(role.userid(), role.u_create_tm())
#define KEY_ROLE(key) role_key_to_role(key)
#define ROLE(uid, create_tm) get_role(uid, create_tm)
#endif
