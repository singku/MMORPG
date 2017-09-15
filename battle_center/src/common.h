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
#include <uuid.h>

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


#include "proto/client/common.pb.h"
#include "proto/common/svr_common.pb.h"
#include "proto/battle/battle.pb.h"
#include "proto/battle/btl_cmd.h"
#include "proto/client/pb0x02.pb.h"
#include "proto/client/cli_cmd.h"
#include "proto/client/cli_errno.h"


class player_t;
class server_t;

#include "macro_utils.h"
#include "proto.h"

enum cmd_finished_t {
    DONT_CLEAR_CMD      = 0,
    CLEAR_CMD           = 1,
    NO_WAIT_SRV_BACK    = 0,
    WAIT_SRV_BACK       = 1,
};

static inline std::string gen_uuid(void)
{
    std::string ret;
    uuid_t uu;
    static const int kUUID_STR_LEN	= 36;
    char uu_str[kUUID_STR_LEN+1]; // 36 + '\0'
    uuid_generate(uu);
    uuid_unparse_lower(uu, uu_str);
    ret.assign(uu_str);
    return ret;
}

using namespace std;
using google::protobuf::Message;
extern char g_send_buf[1000000];

#endif
