#include <string>
#include <stdlib.h>
#include <time.h>
#include <google/protobuf/dynamic_message.h>
#ifndef UTILS_H
#define UTILS_H
inline int atoi_safe(const char* str)
{
    if (str == NULL) {
        return 0; 
    } else {
        return atoll(str); 
    }
}

inline uint32_t time_to_date(time_t time)
{
     tm tm;
     localtime_r(&time, &tm);
 
     return (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
}

inline bool is_same_day(int32_t time1, int32_t time2)
{
    if (time1 - time2 > 86400 || time2 - time1 > 86400) {
        return false;
    }
    struct tm date1, date2;
    time_t tm1 = time1;
    time_t tm2 = time2;
    localtime_r(&tm1, &date1);
    localtime_r(&tm2, &date2);
    return (date1.tm_mday== date2.tm_mday); 
}

class Utils
{
public:
	static std::string to_string(uint32_t n);
	static std::string to_string(int n);
	static void print_message(const google::protobuf::Message& message);
};

#define STD_INSERT_GET_ID( sqlstr,existed_err, id )  \
    { int dbret;\
        int acount; \
        if ((dbret=this->db->exec_update_sql(sqlstr,&acount ))==db_err_succ){\
            id=mysql_insert_id(&(this->db->handle));\
            return db_err_succ;\
        }else {\
            if (dbret==ER_DUP_ENTRY)\
                return  existed_err;\
            else return db_err_sys_err;\
        }\
    }
#endif
