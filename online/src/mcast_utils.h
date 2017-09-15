#ifndef MCAST_UTILS_H
#define MCAST_UTILS_H

#include "common.h"

class McastUtils {
public:

    static uint32_t reload_configs(const std::string& conf_name);

    static uint32_t notify(const std::string& msg);
};


#endif
