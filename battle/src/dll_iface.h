
#ifndef SERVER_H
#define SERVER_H

#ifndef OL_REVISION
#define OL_REVISION 0
#endif

extern "C" {
#include <libtaomee/log.h>
}
#include <string>

struct server_config_t
{
    char conf_path[256];
};

std::string stack_trace();
const char *gen_full_path(const char *base_path, const char *file_name);

#endif
