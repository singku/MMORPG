#include "hiredis_manager.h"

hiredis_manager_t::hiredis_manager_t()
{
}

hiredis_manager_t::~hiredis_manager_t()
{
    fini();
}

bool hiredis_manager_t::init(void) 
{
    struct timeval timeout = {CONN_REDIS_TIMEOUT, 0}; 
    redis_context_ = redisConnectWithTimeout(
                config_get_strval("redis_svr_ip"),
                config_get_intval("redis_svr_port", 6379),
                timeout);
    if (redis_context_ == NULL || redis_context_->err) {
        if (redis_context_) {
            ERROR_TLOG("Connection redis svr error: %s !", redis_context_->errstr);
            redisFree(redis_context_);
        } 
        redis_context_ = 0;
        return false;
    } 
    DEBUG_TLOG("Redis Connect ok![%s:%d]", 
            config_get_strval("redis_svr_ip"), config_get_intval("redis_svr_port", 6379));
    return true;    
}

bool hiredis_manager_t::fini(void) 
{
    if (redis_context_) {
        redisFree(redis_context_);
    }
    redis_context_ = 0;
    return true;
}

redisContext* hiredis_manager_t::get_redis(void)
{
    struct timeval timeout = {CONN_REDIS_TIMEOUT, 0}; 
    if (!redis_context_) {
        redis_context_ = redisConnectWithTimeout(
                config_get_strval("redis_svr_ip"),
                config_get_intval("redis_svr_port", 6379),
                timeout);
        if (!redis_context_ || redis_context_->err) {
            if (redis_context_) {
                ERROR_TLOG("Redis: Get Redis Failed %s", redis_context_->errstr);
                redisFree(redis_context_);
            }
            ERROR_TLOG("Redis: Connect failed[%s:%d]",
                    config_get_strval("redis_svr_ip"), config_get_intval("redis_svr_port", 6379));
            redis_context_ = 0;
            return 0;
        }
        DEBUG_TLOG("Redis Connect ok![%s:%d]", 
                config_get_strval("redis_svr_ip"), config_get_intval("redis_svr_port", 6379));
        return redis_context_;
    } else if (redis_context_->err) {
        redis_context_ = redisConnectWithTimeout(
                config_get_strval("redis_svr_ip"),
                config_get_intval("redis_svr_port", 6379),
                timeout);
        if (!redis_context_ || redis_context_->err) {
            if (redis_context_) {
                ERROR_TLOG("Redis: Get Redis Failed %s", redis_context_->errstr);
                redisFree(redis_context_);
            }
            ERROR_TLOG("Redis: Connect failed[%s:%d]",
                    config_get_strval("redis_svr_ip"), config_get_intval("redis_svr_port", 6379));
            redis_context_ = 0;
            return 0;
        }
        DEBUG_TLOG("Redis Connect ok![%s:%d]", 
                config_get_strval("redis_svr_ip"), config_get_intval("redis_svr_port", 6379));
        return redis_context_;
    } 

    return redis_context_;
}

bool hiredis_manager_t::get_player_info(uint32_t uid, uint32_t u_create_tm,
        commonproto::battle_player_data_t *info)
{
AGAIN:
    redisContext *rc = get_redis();
    if (!rc) return false;
    redisReply *reply = (redisReply *)redisCommand(rc, "GET %s%u_%u", "dplan-player-cache-u:", uid, u_create_tm);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            ERROR_TLOG("Redis: Execute Command Faild %s", reply->str);
            freeReplyObject(reply);
        }
        if (rc->err) {
            ERROR_TLOG("Redis: Execute Command Faild %s", rc->errstr);
            goto AGAIN;
        }
        return false;
    }

    if (reply->type == REDIS_REPLY_STRING) {
        if (!info->ParsePartialFromArray(reply->str, reply->len)) {
            ERROR_TLOG("get_player_info ParsePartialFromArray fail");
            freeReplyObject(reply);
            return false;
        }
        freeReplyObject(reply);
        /* 由于数据会更新到DB 为了防止一直命中拉不到新数据 强制到时就过期
        //命中后重设超时时间
        reply = (redisReply *)redisCommand(rc, "EXPIRE %s%u %u", "dplan-player-cache-u:", 
                uid, config_get_intval("cache_expire_tm", 10800));
        if (reply) {
            freeReplyObject(reply);
        }
        */
        return true;
    }
    freeReplyObject(reply);
    return false;
}

void hiredis_manager_t::set_player_info(uint32_t uid, uint32_t u_create_tm, std::string &val)
{
AGAIN:
    redisContext *rc = get_redis();
    if (!rc) return;
    redisReply *reply = (redisReply *)redisCommand(rc, "SET %s%u_%u %b", "dplan-player-cache-u:", uid, u_create_tm,
            val.c_str(), val.size());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            ERROR_TLOG("Redis: Execute Command Faild %s", reply->str);
            freeReplyObject(reply);
        }
        if (rc->err) {
            ERROR_TLOG("Redis: Execute Command Faild %s", rc->errstr);
            goto AGAIN;
        }
        return;
    }
    freeReplyObject(reply);
    //设置后重设超时时间
    reply = (redisReply *)redisCommand(rc, "EXPIRE %s%u_%u %u", "dplan-player-cache-u:", 
            uid, u_create_tm, config_get_intval("cache_expire_tm", 10800));
    if (reply) {
        freeReplyObject(reply);
    }
    return;
}

void hiredis_manager_t::del_player_info(uint32_t uid, uint32_t u_create_tm)
{
AGAIN:
    redisContext *rc = get_redis();
    if (!rc) return;
    redisReply *reply = (redisReply *)redisCommand(rc, "DEL %s%u_%u", "dplan-player-cache-u:", uid, u_create_tm);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            ERROR_TLOG("Redis: Execute Command Faild %s", reply->str);
            freeReplyObject(reply);
        }
        if (rc->err) {
            ERROR_TLOG("Redis: Execute Command Faild %s", rc->errstr);
            goto AGAIN;
        }
        return;
    }
    freeReplyObject(reply);
    return;
}

bool hiredis_manager_t::get_cache(const string &key, string &value)
{
AGAIN:
    redisContext *rc = get_redis();
    if (!rc) return false;
    redisReply *reply = (redisReply *)redisCommand(rc, "GET %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            ERROR_TLOG("Redis: Execute Command Faild %s", reply->str);
            freeReplyObject(reply);
        }
        if (rc->err) {
            ERROR_TLOG("Redis: Execute Command Faild %s", rc->errstr);
            goto AGAIN;
        }
        return false;
    }

    if (reply->type == REDIS_REPLY_STRING) {
        value.assign(reply->str, reply->len);
        freeReplyObject(reply);
        return true;
    }
    freeReplyObject(reply);
    return false;
}

void hiredis_manager_t::set_cache(const string &key, const string &value, uint32_t ttl)
{
AGAIN:
    redisContext *rc = get_redis();
    if (!rc) return;
    redisReply *reply = (redisReply *)redisCommand(rc, "SET %s %b", key.c_str(), 
            value.c_str(), value.size());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            ERROR_TLOG("Redis: Execute Command Faild %s", reply->str);
            freeReplyObject(reply);
        }
        if (rc->err) {
            ERROR_TLOG("Redis: Execute Command Faild %s", rc->errstr);
            goto AGAIN;
        }
        return;
    }
    freeReplyObject(reply);
    //设置后重设超时时间
    if (ttl == 0) {
        ttl = 0xFFFFFFFF;
    }
    reply = (redisReply *)redisCommand(rc, "EXPIRE %s %u", key.c_str(), ttl);
    if (reply) {
        freeReplyObject(reply);
    }
    return;
}
