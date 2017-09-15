
#include <map>
#include <vector>
#include <google/protobuf/descriptor.h>
#include <mysql/mysql.h>

extern "C" {
#include <libtaomee/log.h>
}

#include "sql_utils.h"

const std::string& to_str(uint32_t value, std::string& str)
{
    char num_buf[20]; 

    sprintf(num_buf, "%u", value);

    str.clear();
    str.assign(num_buf);

    return str;
}

const std::string& int_to_str(int value, std::string& str)
{
    char num_buf[20]; 

    sprintf(num_buf, "%d", value);

    str.clear();
    str.assign(num_buf);

    return str;
}

std::string SQLUtils::to_escape_string(MYSQL* mysql, const std::string& src)
{
    // 这里限制最大转义字串要小于32k
    static char dest[65536];

    if (sizeof(dest) < src.size() * 2 + 1) {
        return ""; 
    }

    int dest_len = mysql_real_escape_string(mysql, dest, src.c_str(), src.size());

    return std::string(dest, dest_len);
}

int SQLUtils::gen_replace_sql_from_proto(
        userid_t userid,
        uint32_t u_create_tm,
        const char* table_name,
        const google::protobuf::Message& message,
        std::string& sql, 
        MYSQL* mysql)
{
    sql.clear();

    const google::protobuf::Reflection * msg_reflect = message.GetReflection();
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    msg_reflect->ListFields(message, &fields);

    if (fields.size() == 0) {
        return 0;
    }

    std::string uid_str;
    to_str(userid, uid_str);

    std::string crtm_str;
    to_str(u_create_tm, crtm_str);

    sql.append("INSERT INTO "); 
    sql.append(table_name);
    sql.append(" (userid, u_create_tm");

    std::vector<std::string> keys;
    std::vector<std::string> vals;

    std::vector<const google::protobuf::FieldDescriptor *>::iterator it;

    for (it = fields.begin(); it != fields.end(); it++) {

        const std::string& key = (*it)->name();
        std::string value;
        uint32_t n = 0;

        switch ((*it)->type()) {
            case google::protobuf::FieldDescriptor::TYPE_INT32:
                n = msg_reflect->GetInt32(message, *it);
                to_str(n, value);
                break;
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                n = msg_reflect->GetUInt32(message, *it);
                to_str(n, value);
                break;	
            case google::protobuf::FieldDescriptor::TYPE_STRING:
                value.append("'");
                value.append(to_escape_string(mysql, msg_reflect->GetString(message, *it)));
                value.append("'");
                break;
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                n = msg_reflect->GetBool(message, *it);
                to_str(n, value);
                break;
            default:
                ERROR_LOG("unsupport field type %u",
                        (*it)->type());
                return -1;
        }

        keys.push_back(key);
        vals.push_back(value);
    }

    for (uint32_t i = 0; i < keys.size(); i++) {

        sql.append(" , ");
        sql.append(keys[i]);
    }
    sql.append(") VALUES (");
    sql.append(uid_str);
    sql.append(", ");
    sql.append(crtm_str);

    for (uint32_t i = 0; i < vals.size(); i++) {
    
        sql.append(" , ");
        sql.append(vals[i]);
    }
    sql.append(") ON DUPLICATE KEY UPDATE ");

    for (uint32_t i = 0; i < keys.size(); i++) {

        if (keys[i] == "userid" || keys[i] == "u_create_tm") {
            continue; 
        }

        if (i != 0) {
            sql.append(" , "); 
        }
        sql.append(keys[i] + " = " + vals[i]); 
    }

    return 0;
}

int SQLUtils::gen_replace_sql_from_proto_any_key(
        const char* table_name,
        const google::protobuf::Message& message,
        std::string& sql, 
        MYSQL* mysql)
{
    sql.clear();

    const google::protobuf::Reflection * msg_reflect = message.GetReflection();
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    msg_reflect->ListFields(message, &fields);

    if (fields.size() == 0) {
        return 0;
    }

    sql.append("INSERT INTO "); 
    sql.append(table_name);
    sql.append("(");

    // 所有列集合
    std::vector<std::string> keys;
    std::vector<std::string> vals;

    // 不包含主键的列集合
    std::vector<std::string> para_keys;
    std::vector<std::string> para_vals;

    std::vector<const google::protobuf::FieldDescriptor *>::iterator it;

    for (it = fields.begin(); it != fields.end(); it++) {

        const std::string& key = (*it)->name();
        std::string value;
        uint32_t n = 0;
        int32_t  m = 0;

        switch ((*it)->type()) {
            case google::protobuf::FieldDescriptor::TYPE_INT32:
                m = msg_reflect->GetInt32(message, *it);
                int_to_str(m, value);
                break;
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                n = msg_reflect->GetUInt32(message, *it);
                to_str(n, value);
                break;	
            case google::protobuf::FieldDescriptor::TYPE_STRING:
                value.append("'");
                value.append(to_escape_string(mysql, msg_reflect->GetString(message, *it)));
                value.append("'");
                break;
            case google::protobuf::FieldDescriptor::TYPE_BYTES:
                value.append("'");
                value.append(to_escape_string(mysql, msg_reflect->GetString(message, *it)));
                value.append("'");
                break;
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                n = msg_reflect->GetBool(message, *it);
                to_str(n, value);
                break;
            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                {
                    const google::protobuf::EnumValueDescriptor *des = msg_reflect->GetEnum(message, *it);
                    m = des->number();
                    int_to_str(m, value);
                    break;
                }
            default:
                ERROR_LOG("unsupport field type %u",
                        (*it)->type());
                return -1;
        }

        if ((*it)->label() != google::protobuf::FieldDescriptor::LABEL_REQUIRED) {
            para_keys.push_back(key);
            para_vals.push_back(value);
        }

        keys.push_back(key);
        vals.push_back(value);
    }

    for (uint32_t i = 0; i < keys.size(); i++) {
        sql.append(keys[i]);
        sql.append(" , ");
    }
    sql = sql.substr(0, sql.length() - 2);
    sql.append(") VALUES (");

    for (uint32_t i = 0; i < vals.size(); i++) {
        sql.append(vals[i]);
        sql.append(" , ");
    }
    sql = sql.substr(0, sql.length() - 2);
    sql.append(")");

    if (para_keys.size() > 0) {
        sql.append(" ON DUPLICATE KEY UPDATE ");


        for (uint32_t i = 0; i < para_keys.size(); i++) {
            sql.append(para_keys[i] + " = " + para_vals[i]); 
            sql.append(", ");
        }
        sql = sql.substr(0, sql.length() - 2);

    }
    return 0;
}

int SQLUtils::gen_change_sql_from_proto_any_key(
        const char* table_name,
        const google::protobuf::Message& message,
        std::string& sql, 
        MYSQL* mysql)
{
    sql.clear();

    const google::protobuf::Reflection * msg_reflect = message.GetReflection();
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    msg_reflect->ListFields(message, &fields);

    if (fields.size() == 0) {
        return 0;
    }

    sql.append("INSERT INTO "); 
    sql.append(table_name);
    sql.append(" (");

    // 所有列集合
    std::vector<std::string> keys;
    std::vector<std::string> vals;

    // 数值类型的列集合
    std::vector<std::string> para_keys;
    std::vector<std::string> para_vals;

    std::vector<const google::protobuf::FieldDescriptor *>::iterator it;

    for (it = fields.begin(); it != fields.end(); it++) {

        const std::string& key = (*it)->name();
        std::string value;
        uint32_t n = 0;
        int32_t m = 0;

        bool push_flag = false;
        // TODO toby all data type
        switch ((*it)->type()) {
            case google::protobuf::FieldDescriptor::TYPE_INT32:
                m = msg_reflect->GetInt32(message, *it);
                int_to_str(m, value);
                push_flag = true;
                break;
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                n = msg_reflect->GetUInt32(message, *it);
                to_str(n, value);
                push_flag = true;
                break;	
            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                {
                    const google::protobuf::EnumValueDescriptor *des = msg_reflect->GetEnum(message, *it);
                    m = des->number();
                    int_to_str(m, value);
                    break;
                }
                break;
            default:
                break;
        }

        if ((*it)->label() != google::protobuf::FieldDescriptor::LABEL_REQUIRED 
                && push_flag == true){
            para_keys.push_back(key);
            para_vals.push_back(value);
        }

        if ((*it)->label() == google::protobuf::FieldDescriptor::LABEL_REQUIRED 
                || push_flag == true) {
            keys.push_back(key);
            vals.push_back(value);
        }
        
    }

    for (uint32_t i = 0; i < keys.size(); i++) {
        sql.append(keys[i]);
        sql.append(" , ");
    }
    sql = sql.substr(0, sql.length() - 2);
    sql.append(") VALUES (");

    for (uint32_t i = 0; i < vals.size(); i++) {
        sql.append(vals[i]);
        sql.append(" , ");
    }
    sql = sql.substr(0, sql.length() - 2);
    sql.append(")");

    if (para_keys.size() > 0) {
        sql.append(" ON DUPLICATE KEY UPDATE ");
        for (uint32_t i = 0; i < para_keys.size(); i++) {
            sql.append(para_keys[i] + " = IF( " + para_keys[i] + "+(" + para_vals[i] + ") > " + para_keys[i] + " && (" + para_vals[i] + ") < 0, 0, " + para_keys[i] + " + (" + para_vals[i] + "))");

            sql.append(", ");
        }
        sql = sql.substr(0, sql.length() - 2);

    }
    return 0;
}

int SQLUtils::gen_update_sql_from_proto(
        userid_t userid,
        uint32_t u_create_tm,
        const char* table_name, 
        const google::protobuf::Message& message,
        std::string& sql,
        MYSQL* mysql)
{
    sql.clear();

    const google::protobuf::Reflection * msg_reflect = message.GetReflection();
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    msg_reflect->ListFields(message, &fields);

    if (fields.size() == 0) {
        return 0;
    }

    std::string uid_str;
    to_str(userid, uid_str);

    sql.append("UPDATE "); 
    sql.append(table_name);
    sql.append(" SET ");

    std::vector<const google::protobuf::FieldDescriptor *>::iterator it;

    int count = 0;
    for (it = fields.begin(); it != fields.end(); it++, count++) {

        const std::string& key = (*it)->name();
        std::string value;
        uint32_t n = 0;

        if (count != 0) {
            sql.append(" , ");  
        }

        switch ((*it)->type()) {
            case google::protobuf::FieldDescriptor::TYPE_INT32:
                n = msg_reflect->GetInt32(message, *it);
                to_str(n, value);
                break;
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                n = msg_reflect->GetUInt32(message, *it);
                to_str(n, value);
                break;	
            case google::protobuf::FieldDescriptor::TYPE_STRING:
                value.append("'");
                value.append(to_escape_string(mysql, msg_reflect->GetString(message, *it)));
                value.append("'");
                break;
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                n = msg_reflect->GetBool(message, *it);
                to_str(n, value);
                break;
            default:
                ERROR_LOG("unsupport field type %u",
                        (*it)->type());
                return -1;
        }

        sql.append(key + " = " + value);
    }

    sql.append(" WHERE userid = ");
    sql.append(uid_str);
    sql.append(" AND u_create_tm = ");
    string create_tm_str;
    to_str(u_create_tm, create_tm_str);
    sql.append(create_tm_str);

    return 0;
}

int SQLUtils::gen_update_sql_from_proto_any_key(
        const char* table_name,
        const google::protobuf::Message& message,
        std::string& sql, 
        MYSQL* mysql)
{
    sql.clear();

    const google::protobuf::Reflection * msg_reflect = message.GetReflection();
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    msg_reflect->ListFields(message, &fields);

    if (fields.size() == 0) {
        return 0;
    }

    sql.append("UPDATE "); 
    sql.append(table_name);
    sql.append(" SET ");

    // 主键列集合
    std::vector<std::string> keys;
    std::vector<std::string> vals;

    // 不包含主键的列集合
    std::vector<std::string> para_keys;
    std::vector<std::string> para_vals;

    std::vector<const google::protobuf::FieldDescriptor *>::iterator it;

    for (it = fields.begin(); it != fields.end(); it++) {

        const std::string& key = (*it)->name();
        std::string value;
        uint32_t n = 0;
        int32_t  m = 0;

        switch ((*it)->type()) {
            case google::protobuf::FieldDescriptor::TYPE_INT32:
                m = msg_reflect->GetInt32(message, *it);
                int_to_str(m, value);
                break;
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                n = msg_reflect->GetUInt32(message, *it);
                to_str(n, value);
                break;	
            case google::protobuf::FieldDescriptor::TYPE_STRING:
                value.append("'");
                value.append(to_escape_string(mysql, msg_reflect->GetString(message, *it)));
                value.append("'");
                break;
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                n = msg_reflect->GetBool(message, *it);
                to_str(n, value);
                break;
            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                {
                    const google::protobuf::EnumValueDescriptor *des = msg_reflect->GetEnum(message, *it);
                    m = des->number();
                    int_to_str(m, value);
                    break;
                }
            default:
                ERROR_LOG("unsupport field type %u",
                        (*it)->type());
                return -1;
        }

        if ((*it)->label() != google::protobuf::FieldDescriptor::LABEL_REQUIRED) {
            para_keys.push_back(key);
            para_vals.push_back(value);
        } else {
            keys.push_back(key);
            vals.push_back(value);
        }
    }

    for (uint32_t i = 0; i < para_keys.size(); i++) {
        sql.append(para_keys[i] + " = " + para_vals[i]); 
        sql.append(", ");
    }
    sql = sql.substr(0, sql.length() - 2);
    sql = sql.append(" where ");

    for (uint32_t i = 0; i < keys.size(); i++) {
        sql.append(keys[i] + " = " + vals[i]); 
        sql.append(" and ");
    }
    sql = sql.substr(0, sql.length() - 4);

    return 0;
}

int SQLUtils::gen_insert_sql_from_proto(
        userid_t userid,
        uint32_t u_create_tm,
        const char* table_name,
        const google::protobuf::Message& message,
        std::string& sql,
        MYSQL* mysql)
{
    sql.clear();

    const google::protobuf::Reflection * msg_reflect = message.GetReflection();
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    msg_reflect->ListFields(message, &fields);

    if (fields.size() == 0) {
        return 0;
    }

    std::string uid_str;
    to_str(userid, uid_str);

    sql.append("INSERT INTO "); 
    sql.append(table_name);
    sql.append(" (userid, u_create_tm");

    std::vector<std::string> keys;
    std::vector<std::string> vals;

    std::vector<const google::protobuf::FieldDescriptor *>::iterator it;

    for (it = fields.begin(); it != fields.end(); it++) {

        const std::string& key = (*it)->name();
        std::string value;
        uint32_t n = 0;

        switch ((*it)->type()) {
            case google::protobuf::FieldDescriptor::TYPE_INT32:
                n = msg_reflect->GetInt32(message, *it);
                to_str(n, value);
                break;
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                n = msg_reflect->GetUInt32(message, *it);
                to_str(n, value);
                break;	
            case google::protobuf::FieldDescriptor::TYPE_STRING:
                value.append("'");
                value.append(to_escape_string(mysql, msg_reflect->GetString(message, *it)));
                value.append("'");
                break;
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                value = msg_reflect->GetBool(message, *it);
                to_str(n, value);
                break;
            default:
                ERROR_LOG("unsupport field type %u",
                        (*it)->type());
                return -1;
        }

        keys.push_back(key);
        vals.push_back(value);
    }

    for (uint32_t i = 0; i < keys.size(); i++) {

        sql.append(" , ");
        sql.append(keys[i]);
    }
    sql.append(") VALUES (");
    sql.append(uid_str);
    sql.append(" , ");
    string create_tm_str;
    to_str(u_create_tm, create_tm_str);
    sql.append(create_tm_str);

    for (uint32_t i = 0; i < vals.size(); i++) {
    
        sql.append(" , ");
        sql.append(vals[i]);
    }
    sql.append(")");

    return 0;
}
