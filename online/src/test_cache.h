#ifndef __TEST_CACHE_H__
#define __TEST_CACHE_H__

#include "common.h"
#include "cmd_processor_interface.h"

class GetCacheInfoCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen) {
        return 0;
    }
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
    cacheproto::sc_batch_get_users_info cache_out_;
};


void test_get_cache(player_t *p);

#endif
