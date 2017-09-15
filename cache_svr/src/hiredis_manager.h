#ifndef  __HIREDIS_MANAGER_H__
#define  __HIREDIS_MANAGER_H__

#include "common.h"
#include "hiredis/hiredis.h"

#define CONN_REDIS_TIMEOUT 2  // 连接redis超时时间(单位:秒)

class hiredis_manager_t {
public:
	hiredis_manager_t();
	~hiredis_manager_t();

public:
    bool init(void);
    bool fini(void);
    
    bool get_player_info(uint32_t uid, uint32_t u_create_tm, commonproto::battle_player_data_t *info);
    void set_player_info(uint32_t uid, uint32_t u_create_tm, std::string &val); 
    void del_player_info(uint32_t uid, uint32_t u_create_tm);

    void set_cache(const string &key, const string &value, uint32_t ttl);
    bool get_cache(const string &key, string &value);
private:
    redisContext* get_redis(void);   
private:
	redisContext *redis_context_;
};

typedef singleton_default<hiredis_manager_t> hiredis_manager_singleton_t;
#define HIREDIS_MGR (hiredis_manager_singleton_t::instance())

#endif  /*__HIREDIS_MANAGER_H__*/
