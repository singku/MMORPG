#include "player.h"
#include "global_data.h"
#include "service.h"
#include "test_cache.h"

void test_get_cache(player_t *p)
{
    cacheproto::cs_batch_get_users_info cache_req;
	/*
    cache_req.add_uids(100022);
    cache_req.add_uids(100021);
    cache_req.add_uids(6031662);
    cache_req.add_uids(123456);
	*/
    
    g_dbproxy->send_msg(0, p->userid, p->create_tm, cache_cmd_ol_req_users_info, cache_req, 0);
}

int GetCacheInfoCmdProcessor::proc_pkg_from_serv(
        player_t* player, const char* body, int bodylen)
{
    cache_out_.Clear();
    if (parse_message(body, bodylen, &cache_out_)) {
        return 0;
    }

    return 0;
}

