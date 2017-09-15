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
#include <libtaomee/project/utilities.h>

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

#include "proto/common/svr_proto_header.h"
#include "proto/common/svr_common.pb.h"
#include "proto/client/common.pb.h"
#include "proto/battle/battle.pb.h"
#include "proto/battle/btl_cmd.h"

#include "proto/home_svr/home.pb.h"
#include "proto/home_svr/home_cmd.h"

#include "proto/cache/cache_cmd.h"
#include "proto/cache/cache_errno.h"
#include "proto/cache/cacheproto.pb.h"

#include "proto/client/pb0x00.pb.h"
#include "proto/client/pb0x01.pb.h"
#include "proto/client/pb0x02.pb.h"
#include "proto/client/pb0x03.pb.h"
#include "proto/client/pb0x06.pb.h"
#include "proto/client/pb0x07.pb.h"
#include "proto/client/pb0x08.pb.h"
#include "proto/client/header.pb.h"
#include "proto/client/cli_cmd.h"
#include "proto/client/cli_errno.h"
#include "proto/client/attr_type.h"
#include "proto/client/global_attr_type.h"

#include "proto/ranking/rank.pb.h"
#include "proto/ranking/rank_cmd.h"

#include "proto/db/db_cmd.h"
#include "proto/db/db_errno.h"
#include "proto/db/dbproto.attr.pb.h"
#include "proto/db/dbproto.base_info.pb.h"
#include "proto/db/dbproto.data.pb.h"
#include "proto/db/dbproto.item.pb.h"
#include "proto/db/dbproto.login.pb.h"
#include "proto/db/dbproto.nick.pb.h"
#include "proto/db/dbproto.pet.pb.h"
#include "proto/db/dbproto.transaction.pb.h"
#include "proto/db/dbproto.home.pb.h"
#include "proto/db/dbproto.family.pb.h"
#include "proto/db/dbproto.friend.pb.h"
#include "proto/db/dbproto.raw_data.pb.h"
#include "proto/db/dbproto.achieve.pb.h"
#include "proto/db/dbproto.mine.pb.h"

#include "proto/switch/switch_cmd.h"
#include "proto/switch/switch_errno.h"
#include "proto/switch/switch.pb.h"

#include "macro_utils.h"
#include "time_utils.h"

using namespace std;

struct player_t;

enum {
    NO_SYNC_DB      = 0,
    SYNC_DB         = 1,
    NO_NOTI_CLI     = 0,
    NOTI_CLI        = 1,
    NO_WAIT_SVR     = 0,
    WAIT_SVR        = 1,
    ADDICT_DETEC    = true,
    NO_ADDICT_DETEC = false,
    DO_IT_NOW       = true, //立即做
    DO_IT_DEPENDS   = false, //按情况做
    EXPAND_DATA     = true,
    NO_EXPAND_DATA  = false,
};

// 角色和精灵共有属性
// 战斗普通数值类型
enum battle_value_normal_type_t {
    kBattleValueNormalTypeHp = 0, //生命
    kBattleValueNormalTypeNormalAtk = 1, //普攻
    kBattleValueNormalTypeNormalDef = 2, //普防
    kBattleValueNormalTypeSkillAtk = 3, //技功
    kBattleValueNormalTypeSkillDef = 4, //技防
    kMaxBattleValueTypeNum,
};

// 隐藏战斗数值类型
enum battle_value_hide_type_t {
    kBattleValueHideTypeCrit = 0,//暴击值，
    kBattleValueHideTypeAntiCrit = 1,//防爆，同上
    kBattleValueHideTypeHit = 2,//命中值，同上
    kBattleValueHideTypeDodge = 3,//闪避值，同上
    kBattleValueHideTypeBlock = 4,//格挡值，同上
    kBattleValueHideTypeBreakBlock = 5,//破格值，同上
    kBattleValueHideTypeCritAffectRate = 6, //暴击加成率
    kBattleValueHideTypeBlockAffectRate = 7, //格挡加成率
    kBattleValueHideTypeAtkSpeed = 8, //攻击速度
    kMaxBattleValueHideTypeNum,
}; 

enum max_num_limit_type_t
{
    MAX_GET_FAMILY_MEMBER_LIST_PAGE_SIZE = 30,
    MAX_GET_RANK_INFO_USER_NUM          = 100,
    MAX_TRIAL_ATTR_PROGRESS             = 1000000,
    MAX_GET_FAMILY_EVENT_LIST_PAGE_SIZE = 50,
};

enum mail_type_t
{
	GET_TEST_PRIZE = 1,
	GET_CREATE_ROLE_PRIZE = 2,
	GET_MAYIN_BUCKET_PRIZE = 3,
	GET_MAYIN_FLOWER_PRIZE = 4,
	GET_MERGE_SVR_PRIZE = 5,
};

#endif
