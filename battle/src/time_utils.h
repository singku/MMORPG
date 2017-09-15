#ifndef __TIME_UTILS_H__
#define __TIME_UTILS_H__

extern "C" {
#include <libtaomee/timer.h>
}
#include <stdint.h>
#include <map>
#include <string>

//定义一天有多少秒 正常是86400
#define DAY_SECS (86400)
#define DAY_ADJ (57600)

enum {
    kSunday = 0, 
    kMonday = 1, 
    kTuesday = 2, 
    kWednesday = 3, 
    kThursday = 4, 
    kFriday = 5, 
    kSaturday = 6,
};

class TimeUtils {
public:

    //将字符串时间格式转化为整数
    static int time_str_to_long(const char* time_str, uint32_t& time_value); 
    static int day_str_to_long(const char* time_str, uint32_t& time_value); 
    static bool is_same_day(int32_t time1, int32_t time2);
    static int second_at_day_start(int32_t day_offset = 0);
    static int second_to_next_week();
    //现在到下一个时间分钟之间的间隔
    static int second_to_hm(uint32_t hm);
    static int second_between_hm(uint32_t from_hm, uint32_t to_hm);
    static inline bool is_weekend() {
        const struct tm* tm = get_now_tm(); 
        if (tm->tm_wday == 0 || tm->tm_wday == 6 || tm->tm_wday == 5) {
            return true; 
        } else {
            return false; 
        }
    }

    static uint32_t time_to_date(time_t time);
    static time_t date_to_time(uint32_t date);
    /* 判断time是否是一天的开始 */
    static bool is_day_point(time_t time);
    /* 天对齐，找到一个是>=time且是一天的起点 */
    static time_t day_align_high(time_t time);
    /* 天对齐，找到一个是<=time且是一天的起点 */
    static time_t day_align_low(time_t time);
    /* 获取两个时间的天数间隔 */
    static int get_days_between(time_t start, time_t end);

    //判断是否过去了一周时间
    static bool check_is_week_past(time_t start, time_t end);
    
    //判断是否过去了一月时间
    static bool check_is_month_past(time_t start, time_t end);
    
    /*
    //计算一个月有多长时间
    static int calc_days_of_month(int month, int year);
*/
    static uint32_t get_today_date();

    static time_t minute_align_low(time_t time);

    static time_t minute_align_high(time_t time);

    // 解析xx:xx类型的字符串
    static uint32_t parse_hour_min_str(const std::string& time_str);

    // 获取上周五0点的日期
    static uint32_t get_prev_friday_date();

    // 获取下个周x的unix-time
    // from sunday(0~6)
    static uint32_t get_next_x_time(time_t begin_time, int x);

    // 获取上个周x的unix-time
    // from sunday(0~6)
    // 可能会获得当前天
    static uint32_t get_last_x_time(time_t begin_time, int x);

    // 获取当月第一天的起始时间
    static uint32_t get_cur_month_first_time(time_t time);

    // 获取一个时间的字符串形式 xxxx-xx-xx xx:xx:xx
    static std::string time_to_string(time_t time);
};

#endif
