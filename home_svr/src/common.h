#ifndef __COMMON_H__
#define __COMMON_H__

extern "C" {
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <glib.h>
#include <errno.h>

//libtaomee head files
#include <libtaomee/conf_parser/config.h>
#include <libtaomee/project/stat_agent/msglog.h>
#include <libtaomee/dataformatter/bin_str.h>
#include <libtaomee/log.h>
#include <libtaomee/timer.h>
#include <libtaomee/utils.h>
#include <libtaomee/tm_dirty/tm_dirty.h>

//async_server head files
#include <async_serv/dll.h>
#include <async_serv/net_if.h>
}

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <set>
#include <list>
#include <memory>
#include <bitset>
#include <utility>

// libtaomee++ include files
#include <libtaomee++/conf_parser/xmlparser.hpp>
#include <libtaomee++/random/random.hpp>
#include <libtaomee++/inet/pdumanip.hpp>
#include <libtaomee++/utils/strings.hpp>
#include <libtaomee++/proto/proto_util.h>

// protobuf common include files
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/repeated_field.h>

#include "proto/home_svr/home_cmd.h"
#include "proto/home_svr/home_errno.h"
#include "proto/home_svr/home.pb.h"
#include "proto/client/common.pb.h"
#include "proto/client/cli_errno.h"
#include "proto/client/cli_cmd.h"
#include "proto/db/db_cmd.h"
#include "proto/db/db_errno.h"
#include "proto/db/dbproto.data.pb.h"
#include "proto/db/dbproto.home.pb.h"
#include "proto/db/dbproto.family.pb.h"
#include "proto/switch/switch_cmd.h"
#include "proto/switch/switch_errno.h"
#include "proto/switch/switch.pb.h"
#include "proto/ranking/rank.pb.h"
#include "proto/ranking/rank_cmd.h"
#include "proto/cache/cache_cmd.h"
#include "proto/cache/cache_errno.h"
#include "proto/cache/cacheproto.pb.h"
#include "proto/common/svr_proto_header.h"

class player_t;

#include "macro_utils.h"

enum cmd_finished_t {
    CLEAR_CMD           = 1,
    DONT_CLEAR_CMD      = 0,
};

enum {
	DEFAULT_HOME_TYPE = commonproto::HM_DEFAULT_TYPE,
};

using namespace std;

inline int atoi_safe(const char* str)
{
    if (str == NULL) {
        return 0;
    } else {
        return atoll(str);
    }
}

#endif
