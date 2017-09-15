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
#include <libtaomee/project/types.h>
#include <libtaomee/dataformatter/bin_str.h>
#include <libtaomee/log.h>
#include <libtaomee/timer.h>
#include <libtaomee/utils.h>
#include <libtaomee/tm_dirty/tm_dirty.h>
#include <libtaomee/random/random.h>

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
#include <bitset>
#include <list>
#include <memory>
#include <bitset>
#include <utility>

// libtaomee++ include files
#include <libtaomee++/conf_parser/xmlparser.hpp>
#include <libtaomee++/random/random.hpp>
#include <libtaomee++/inet/pdumanip.hpp>
#include <libtaomee++/utils/strings.hpp>
#include <libtaomee++/utils/strings.hpp>
#include <libtaomee++/proto/proto_base.h>
#include <libtaomee++/proto/proto_util.h>
#include <libtaomee++/bitmanip/bitmanip.hpp>

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

#include "proto/common/svr_common.pb.h"
#include "proto/client/common.pb.h"
#include "proto/battle/battle.pb.h"
#include "proto/battle/btl_cmd.h"
#include "proto/client/pb0x02.pb.h"
#include "proto/client/pb0x01.pb.h"
#include "proto/client/cli_cmd.h"
#include "proto/client/cli_errno.h"
#include "proto/client/attr_type.h"

#include "macro_utils.h"
#include "time_utils.h"
#include "utils.h"

using namespace std;

struct player_t;

#endif
