
#ifndef SQL_UTILS_H
#define SQL_UTILS_H

#include <string>
#include <google/protobuf/message.h>
#include <libtaomee/project/types.h>

using namespace std;

class SQLUtils
{
public:
    static int gen_replace_sql_from_proto(
            userid_t userid,
            uint32_t u_create_tm,
            const char* table_name,
            const google::protobuf::Message& message,
            std::string& sql,
            MYSQL* mysql);

    static int gen_update_sql_from_proto(
            userid_t userid,
            uint32_t u_create_tm,
            const char* table_name, 
            const google::protobuf::Message& message,
            std::string& sql,
            MYSQL* mysql);

    static int gen_insert_sql_from_proto(
            userid_t userid,
            uint32_t u_create_tm,
            const char* table_name,
            const google::protobuf::Message& message,
            std::string& sql,
            MYSQL* mysql);

    static std::string to_escape_string(MYSQL* mysql, const std::string& src);

    static int gen_replace_sql_from_proto_any_key(
            const char* table_name,
            const google::protobuf::Message& message,
            std::string& sql, 
            MYSQL* mysql);

    static int gen_change_sql_from_proto_any_key(
            const char* table_name,
            const google::protobuf::Message& message,
            std::string& sql, 
            MYSQL* mysql);

    static int gen_update_sql_from_proto_any_key(
            const char* table_name,
            const google::protobuf::Message& message,
            std::string& sql, 
            MYSQL* mysql);
};



#endif
