#!/usr/bin/env python
# -*- coding: UTF-8 -*-

#//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#测试版
#
#使用方法：
#配置conf_table.py, 执行./gen_table.py，会在当前目录生成表名目录，其中包含对应表操作脚本和程序
#
#生成的mysql库表操作脚本,包含基本功能:
#1  建立库表
#2  删除表
#3  删除库
#4  数据清空(TODO)
#5  生成表对应配置文件备份
#
#生成表关联的.cpp .h源码(依赖protobuf库支持), 缺省包含
#1  对表任意字段的update操作
#2  对表数值字段的change操作
#
#注意：主键是自增id时生成的cpp代码不可用,sql脚本可以用
#//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

import sys
import os
from conf_table import *

#####下面的不需要配置，扩展功能时需要修改

conf_file_name="conf_table"

#生成完整库名，表名前缀
db_name=db_prefix + "_db"
table_name=table_prefix + "_table"

#####生成文件名
create_name="create_%s.sh"%(table_name)
del_table_name="del_%s.sh"%(table_name)
del_db_name="drop_%s.sh"%(db_name)
clear_name="clear_%s.sh"%(table_name)
cpp_name="%s.cpp"%(table_name)
h_name="%s.h"%(table_name)
table_pb_name="%s.proto"%(table_name)
common_name="%s.common"%(table_name)

#print create_name
#print del_table_name
#print del_db_name
#print clear_name
#print cpp_name
#print h_name
#print common_name

#####生成各种变量名
class_name_upper=table_name.upper()
func_suffix_name=table_prefix

def trans_class_name(underline_name):
    s_name=underline_name.split('_')
    c_name=""
    for n in s_name:
        f=n[0].upper()
        f=f+n[1:]
        c_name=c_name+f
    return c_name

class_name=trans_class_name(table_name)
#print class_name 

#####公用映射关系
s_table_type=table_type.split('x')

#格式化后缀
suffix_format_map={
    "1":("","", ""),
    "10":("_%01d", "9", "_x"),
    "100":("_%02d", "99", "_xx"),
}

#变量类型映射
type_map={
    "uint32":("INT", "UNSIGNED"),
    "int32":("INT", ""),
    "char":("VARCHAR",""),
}

#自增标志
auto_increment={
    "1":"AUTO_INCREMENT",
    "0":"",
}

#//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#####生成sql
max_db_no=s_table_type[0]
max_table_no=s_table_type[1]

#print max_db_no
#print max_table_no
#print suffix_format_map[max_db_no][1]
#print suffix_format_map[max_db_no][0]
#print suffix_format_map[max_table_no][1]
#print suffix_format_map[max_table_no][0]

#生成key
pkey_str=""
ikey_str=""
for cln in table_arch:
    if cln[5] == "2":
        pkey_str=pkey_str + cln[0] + ","
    elif cln[5] == "1":
        ikey_str=ikey_str + ",INDEX(" +  cln[0] + ")"

pkey_str=pkey_str[:-1]
#ikey_str=ikey_str[:-1]

def save_file(filename, str):
    fp=open(file_path + filename, "w")
    fp.write(str)
    fp.close()



#####生成sql
def gen_del_table_sql():
    """
        删除表NxN
    """
    db_sql_str=""
    table_sql_str='''mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop table %s${db_no}.%s${table_no}"'''%(db_name, table_name)

    sql='''
for i in {0..%s}
do
    db_no=`printf '%s' $i`
    %s
    for j in {0..%s}
    do
        table_no=`printf '%s' $j`
        %s
        echo %s$db_no.%s$table_no ok
    done
done
    '''%(suffix_format_map[max_db_no][1], 
            suffix_format_map[max_db_no][0],
            db_sql_str, 
            suffix_format_map[max_table_no][1], 
            suffix_format_map[max_table_no][0],
            table_sql_str, 
            db_name, table_name)

    sh_head='''#!/bin/sh'''
    sql = sh_head + sql

#print sql
    print "gen del table sql done!"
    save_file(del_table_name, sql)

def gen_del_db_sql():
    """
        删除库NxN
    """
    sql='''
for i in {0..%s}
do
    db_no=`printf '%s' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop database %s${db_no}"
    echo "drop database %s${db_no}"
done'''%(suffix_format_map[max_db_no][1], suffix_format_map[max_db_no][0], db_name, db_name)

    sh_head='''#!/bin/sh'''
    sql = sh_head + sql
    print "gen drop db sql done!"
    save_file(del_db_name, sql)



def gen_create_sql():
    """
        建立库表NxN
    """
    db_sql_str='''mysql -udplan -pdplan@0601 -h10.1.1.154 -e "CREATE DATABASE IF NOT EXISTS %s${db_no}"'''%(db_name)
    table_sql_str='''mysql -udplan -pdplan@0601 -h10.1.1.154 -e "'''

    sql_line='''
        SET NAMES UTF8;
        CREATE TABLE IF NOT EXISTS %s${db_no}.%s${table_no}
        (
    '''%(db_name, table_name)

    for cln in table_arch:
        line = '''
        \t%s %s(%s) %s %s %s %s %s '%s',\n'''%(cln[0],type_map[cln[1]][0], cln[2], type_map[cln[1]][1], " NOT NULL ",cln[3],auto_increment[cln[6]]," COMMENT ",cln[4]),
        sql_line=sql_line + line[0]

    sql_end='''
        \tPRIMARY KEY (%s)%s
        ) ENGINE = INNODB CHARSET = UTF8;
        "
    '''%(pkey_str, ikey_str)

    table_sql_str=table_sql_str + sql_line + sql_end

    sql='''
for i in {0..%s}
do
    db_no=`printf '%s' $i`
    %s
    for j in {0..%s}
    do
        table_no=`printf '%s' $j`
        %s
        echo %s$db_no.%s$table_no ok
    done
done
    '''%(suffix_format_map[max_db_no][1], 
            suffix_format_map[max_db_no][0],
            db_sql_str, 
            suffix_format_map[max_table_no][1], 
            suffix_format_map[max_table_no][0],
            table_sql_str, 
            db_name, table_name)

    sh_head='''#!/bin/sh'''
    sql = sh_head + sql
    sql=sql+"\n"
#print sql
    print "gen create table sql done!"
    save_file(create_name, sql)

#TODO 
#清空

#####生成表关联源码
#包含功能update,change
def gen_cpp():
    """
        生成表类cpp文件
    """
    
    cpp_str='''
%s::%s(mysql_interface* db) 
    : CtableRoute%s(db, "%s", "%s", "%s")
{

}

int %s::update_%s(
        const dbproto::%s_table_t &info, uint32_t flag)
{
    std::string sql_str;
    if (flag == (uint32_t)dbproto::DB_UPDATE_AND_INESRT) {
        SQLUtils::gen_replace_sql_from_proto_any_key(
                this->get_table_name(info.%s()),
                info, sql_str, &(this->db->handle)); 
    } else {
        SQLUtils::gen_update_sql_from_proto_any_key(
                this->get_table_name(info.family_id()),
                info, sql_str, &(this->db->handle));
    }

    if (sql_str.size() == 0) {
        return db_err_proto_format_err;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}

int %s::change_%s(
        const dbproto::%s_table_change_data_t &info)
{
    std::string sql_str;
    SQLUtils::gen_change_sql_from_proto_any_key(
            this->get_table_name(info.%s()),
            info, sql_str, &(this->db->handle)); 

    if (sql_str.size() == 0) {
        return db_err_proto_format_err;
    }
    int affected_rows = 0;
    return this->db->exec_update_sql(sql_str.c_str(), &affected_rows);
}
'''%(class_name, class_name, table_type, db_name, table_name, route_key_name,
        class_name, func_suffix_name,func_suffix_name,
        route_key_name,
        class_name, func_suffix_name,func_suffix_name,
        route_key_name,
        )
    include_str=""
    for s in include_cpp_files:
        include_str=include_str+"#include\t"+s+"\n"

    include_str=include_str+"#include\t"+"\""+table_name+".h\"\n"
        
    cpp_str=include_str+cpp_str
#print cpp_str
    print "gen table .cpp done!"
    save_file(cpp_name, cpp_str)

def gen_h():
    """
        生成表类头文件
    """
    include_str=""
    for s in include_h_files:
        include_str=include_str+"#include\t"+s+"\n"
 
    h_str='''#ifndef %s_H
#define %s_H

extern "C" {
#include <libtaomee/project/types.h>
}
%s

class %s : public CtableRoute%s
{
public:
    %s(mysql_interface* db);

    int update_%s(
            const dbproto::%s_t &info, uint32_t type);

    int change_%s(
            const dbproto::%s_change_data_t &info);
};

extern %s* g_%s_table;

#endif //%s_H
        '''%(class_name_upper,class_name_upper,
                include_str, class_name, table_type,
                class_name, 
                table_prefix,table_name,
                table_prefix,table_name,
                class_name, table_prefix,
                class_name_upper,
                )

#print h_str
    print "gen table .h done!"
    save_file(h_name, h_str)

def gen_conf_bak():
    """
        生成配置文件备份
    """
    bak_file = conf_file_name + "_" + table_prefix + ".bak"
    bak_cmd = "cp %s.py %s"%(conf_file_name, file_path + bak_file)
    if os.system(bak_cmd) != 0:
        print "backup conf file failed"

    print "gen conf bak done!"

#TODO 
#####生成protobuf定义文件
def gen_table_def():
    """
        生成表对应的protobuf定义文件
    """
    pb_str='''
// %s%s.%s%s
message %s_t                                                       
{   
}                                                                                                                                  

message %s_change_data_t                                                                                      
{   
}
'''%(db_name, suffix_format_map[max_db_no][2], 
        table_name, suffix_format_map[max_table_no][2], 
        table_name, table_name)
    print "gen table .proto done!"
    save_file(table_pb_name, pb_str)

#TODO 
#####生成调用代码

#TODO 
#####生成db协议代码

if __name__ == "__main__":
    mkpath_cmd = "mkdir -p %s"%(file_path)
    if os.system(mkpath_cmd) == 0:
        gen_create_sql()
        gen_del_table_sql()
        gen_del_db_sql()
        gen_cpp()
        gen_h()
        gen_table_def()
        gen_conf_bak()
    else:
        print "mkdir dir failed"
        sys.exit(0)
