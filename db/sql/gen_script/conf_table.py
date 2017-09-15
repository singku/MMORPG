#!/usr/bin/env python
# -*- coding: UTF-8 -*-

#/////////////////////////////////////////////////////config_start/////////////////////////////////////////////////////////////
#####基本定义
#库名表名关键词，"_db,_table"不用在这里加
db_prefix="dplan_family"
table_prefix="family_event"

#分表分库配置
table_type="100x10"

#####表结构定义
#               0           1        2        3                 4               5                       6      
#           字段名      字段类型    长度    缺省值              注释    是否主键(0普通1key2主键) 是否自增(1是0否)
table_arch=(
        ("family_id",   "uint32",   "11",   "DEFAULT 0",        "家族id",       "2",                    "0"),
        ("userid",      "uint32",   "11",   "DEFAULT 0",  "事件目标用户id",       "2",                    "0"),
        ("event_type",  "uint32",   "11",   "DEFAULT 0",        "事件类型",     "2",                    "0"),
        ("event_status",  "uint32", "11",   "DEFAULT 0",        "事件处理结果", "0",                    "0"),
        ("event_time",  "uint32",   "11",   "DEFAULT 0",        "事件发生时间戳", "0",                  "0"),
        )

#####头文件列表
#.h头文件
include_h_files=(
        "\"proto/client/attr_type.h\"",
        "\"proto/db/db_errno.h\"",
        "\"sql_utils.h\"", 
        "\"macro_utils.h\"",
        "\"utils.h\"",
        )

#.cpp头文件
include_cpp_files=(
        "<dbser/mysql_iface.h>",
        "<dbser/CtableRoute100x10.h>",
        "\"proto/db/dbproto.family.pb.h\"",
        "\"proto/client/common.pb.h\"",
        "\"sql_utils.h\"",
        )

#分表分库id
route_key_name="family_id"
#route_key_name="userid"

#生成文件保存路径
file_path="./" + table_prefix + "_table/"

#/////////////////////////////////////////////////////config_end///////////////////////////////////////////////////////////////

