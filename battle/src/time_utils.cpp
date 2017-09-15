#include<time.h>
#include<stdio.h>
#include "time_utils.h"
extern "C" {
#include <libtaomee/log.h>
#include <libtaomee/timer.h>
}
#include "macro_utils.h"

int TimeUtils:: day_str_to_long(const char* time_str, uint32_t& time_value)
{
    struct tm* target_time;
    time_t rawtime;
    int year,mon,day,hour,min,second = 0;
    year = 0;
    mon = 0;
    day = 0;
    hour = 0;
    min = 0;
    second = 0;

    sscanf(time_str, "%d-%d-%d", &year,&mon,&day);
    //DEBUG_LOG("time_str=%s year=%d, mon=%d, day=%d, hour=%d, min=%d, second=%d", time_str,year, mon, day, hour, min, second);

    if (year < 1900 || mon < 1 || mon > 12 || day < 1 || day > 31 
            || hour < 0 || hour > 23 || min < 0 || min > 59 
            || second < 0 || second > 59) {
        return -1;
    }

    time(&rawtime);
    target_time = localtime (&rawtime); // 其它参数
    target_time->tm_year = year - 1900;
    target_time->tm_mon= mon - 1;     // 月 - 1
    target_time->tm_mday = day ;  // 日
    target_time->tm_hour = hour ;   // 时
    target_time->tm_min = min; // 分
    target_time->tm_sec = second;  // 秒
    time_value = mktime(target_time);

    //DEBUG_LOG("time_value:%u", time_value);

    return 0;
}


int TimeUtils:: time_str_to_long(const char* time_str, uint32_t& time_value)
{
    struct tm* target_time;
    time_t rawtime;
    int year,mon,day,hour,min,second = 0;

    sscanf(time_str, "%d-%d-%d %d:%d:%d", &year,&mon,&day,&hour,&min,&second);
    //DEBUG_LOG("time_str=%s year=%d, mon=%d, day=%d, hour=%d, min=%d, second=%d", time_str,year, mon, day, hour, min, second);

    if (year < 1900 || mon < 1 || mon > 12 || day < 1 || day > 31 
            || hour < 0 || hour > 23 || min < 0 || min > 59 
            || second < 0 || second > 59) {
        return -1;
    }

    time(&rawtime);
    target_time = localtime (&rawtime); // 其它参数
    target_time->tm_year = year - 1900;
    target_time->tm_mon= mon - 1;     // 月 - 1
    target_time->tm_mday = day ;  // 日
    target_time->tm_hour = hour ;   // 时
    target_time->tm_min = min; // 分
    target_time->tm_sec = second;  // 秒
    time_value = mktime(target_time);

    //DEBUG_LOG("time_value:%u", time_value);

    return 0;
}

int TimeUtils::second_at_day_start(int32_t day_offset) {
    static int today_start = 0;
    static uint32_t cur_date = 0; 

    struct tm now;
    time_t now_sec = NOW();
    localtime_r(&now_sec, &now);
    uint32_t now_date = time_to_date(now_sec);
    if (cur_date != now_date) {
        cur_date = now_date;
        now.tm_hour = 0;
        now.tm_min = 0;
        now.tm_sec = 0;
        today_start = mktime(&now); 
    }   

    return today_start + (day_offset * DAY_SECS);
}

int TimeUtils::second_between_hm(uint32_t from_hm, uint32_t to_hm)
{
    int f_h = from_hm / 100;
    int f_m = from_hm % 100;
    int t_h = to_hm / 100;
    int t_m = to_hm % 100;
    int h_diff = (t_h + 24 - f_h) % 24;
    int m_diff = t_m - f_m;
    uint32_t total_min = 0;
    if (h_diff == 0 && m_diff < 0) {
        total_min = 24 * 60 + m_diff;
    } else {
        total_min = h_diff * 60 + m_diff;
    }
    if (h_diff == 0 && m_diff == 0) {
        return 86400;
    }
    return total_min * 60;
}

int TimeUtils::second_to_hm(uint32_t hm)
{
    const tm *now = get_now_tm();
    int hour = hm / 100;
    int min = hm % 100;
    int h_diff = (hour + 24 - now->tm_hour) % 24;
    int min_diff = min - now->tm_min;
    uint32_t total_min = 0;
    if (h_diff == 0 && min_diff < 0) {
        total_min = 24 * 60 + min_diff;
    } else {
        total_min = h_diff * 60 + min_diff;
    }
    if (h_diff == 0 && min_diff == 0 && now->tm_sec == 0) {
        return 86400;//一整天
    }
    return total_min * 60 - now->tm_sec;
}

int TimeUtils::second_to_next_week()
{
    struct tm now;
    time_t now_sec = time(0);
    localtime_r(&now_sec, &now);
    uint32_t today = now.tm_wday; //一周的周几 0-6 对应周日到周六
    int32_t offset;
    if (today == 0) { //周日
        offset = 1; //到下周只有一天
    } else {
        offset = (7 - today + 1);
    }
    int secs_start = second_at_day_start(offset);

    return (secs_start - (int)now_sec + 1);
}

bool TimeUtils::is_same_day(int32_t time1, int32_t time2)
{
    if (time1 - time2 > DAY_SECS || time2 - time1 > DAY_SECS) {
        return false;
    }
    struct tm date1, date2;
    time_t tm1 = time1;
    time_t tm2 = time2;
    localtime_r(&tm1, &date1);
    localtime_r(&tm2, &date2);
    return (date1.tm_mday == date2.tm_mday);
}

uint32_t TimeUtils::time_to_date(time_t time)
{
    tm tm;
    localtime_r(&time, &tm);

    return (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
}

time_t TimeUtils::date_to_time(uint32_t date)
{
    struct tm       tm;

    memset(&tm, 0, sizeof(tm));

    tm.tm_year = date / 10000 - 1900;
    tm.tm_mon = (date / 100) % 100 - 1;
    tm.tm_mday = date % 100;

    if (tm.tm_year < 0 || tm.tm_mon < 0) {
        return 0;
    }

    return mktime(&tm);
}

/* 判断time是否是一天的开始 */
bool TimeUtils::is_day_point(time_t time)
{
    time_t          last_sec;
    struct tm       cur_tm;
    struct tm       last_tm;

    if (time == 0) {
        return false;
    }

    last_sec = time - 1;

    localtime_r(&time, &cur_tm);
    localtime_r(&last_sec, &last_tm);

    return cur_tm.tm_mday != last_tm.tm_mday;
}

/* 天对齐，找到一个是>=time且是一天的起点 */
time_t TimeUtils::day_align_high(time_t time)
{
    struct tm       cur_tm;
    struct tm       next_tm;
    time_t          next_day;

    if (is_day_point(time)) {
        return time;
    }

    next_day = time + 24 * 60 * 60;

    localtime_r(&next_day, &cur_tm);

    memset(&next_tm, 0, sizeof(next_tm));

    next_tm.tm_year = cur_tm.tm_year;
    next_tm.tm_mon = cur_tm.tm_mon;
    next_tm.tm_mday = cur_tm.tm_mday;

    return mktime(&next_tm);
}

/* 天对齐，找到一个是<=time且是一天的起点 */
time_t TimeUtils::day_align_low(time_t time)
{
    struct tm       cur_tm;
    struct tm       next_tm;
    time_t          next_day;

    if (is_day_point(time)) {
        return time;
    }

    next_day = time;

    localtime_r(&next_day, &cur_tm);

    memset(&next_tm, 0, sizeof(next_tm));

    next_tm.tm_year = cur_tm.tm_year;
    next_tm.tm_mon = cur_tm.tm_mon;
    next_tm.tm_mday = cur_tm.tm_mday;

    return mktime(&next_tm);
}

/* 获取两个时间的天数间隔 */
int TimeUtils::get_days_between(time_t start, time_t end)
{
    if (start > end) {
        return TimeUtils::get_days_between(end, start);       
    }

    return (day_align_low(end) - day_align_low(start)) / 24 / 3600;
}

bool TimeUtils::check_is_week_past(time_t start, time_t end)
{
    TRACE_TLOG("CheckIsWeekPast:start_time:%u;end_time:%u", start, end);
    if (start <= end) {
        tm start_tm;
        localtime_r(&start, &start_tm);

        /*
        tm next_week_monday = {0};
        next_week_monday.tm_year = start_tm.tm_year;
        next_week_monday.tm_mon = start_tm.tm_mon;
        next_week_monday.tm_mday = start_tm.tm_mday + 
            (start_tm.tm_wday == 0?1:8 - start_tm.tm_wday);
        time_t next_week_monday_time = mktime(&next_week_monday);
        */
        tm next_week_monday = {0};
        next_week_monday.tm_year = start_tm.tm_year;
        next_week_monday.tm_mon = start_tm.tm_mon;
        next_week_monday.tm_mday = start_tm.tm_mday + 
            (start_tm.tm_wday == 0?1:8 - start_tm.tm_wday);
        time_t next_week_monday_time = mktime(&next_week_monday);
    TRACE_TLOG("CheckIsWeekPast:clean_time:%u;end_time:%u", next_week_monday_time, end);
        return end >= next_week_monday_time;
    } else {
        return false;
    }
}
    
bool TimeUtils::check_is_month_past(time_t start, time_t end)
{
    if (start <= end) {
        tm start_tm;
        localtime_r(&start, &start_tm);

        tm next_month = {0};
        next_month.tm_year = start_tm.tm_year;
        next_month.tm_mon = start_tm.tm_mon + 1;
        next_month.tm_mday = 1;

        time_t next_month_first = mktime(&next_month);

        return end >= next_month_first;
    } else {
        return false;
    }
}
    
uint32_t TimeUtils::get_cur_month_first_time(time_t time) {
    tm cur_tm;
    localtime_r(&time, &cur_tm);

    tm cur_month = {0};
    cur_month.tm_year = cur_tm.tm_year;
    cur_month.tm_mon = cur_tm.tm_mon;
    cur_month.tm_mday = 1;

    time_t cur_month_first = mktime(&cur_month);

    return cur_month_first;
}

/*
int TimeUtils::calc_days_of_month(int month, int year)
{
    switch (month) {
        case 0:
        case 2:
        case 4:
        case 6:
        case 7:
        case 9:
        case 11:
            return 31;
        case 1:
            return year%4 == 0?29:28;
        case 3:
        case 5:
        case 8:
        case 10:
            return 30;
        default:
            return 0;

    }
}
*/

uint32_t TimeUtils::get_today_date()
{
    const struct tm* tm = get_now_tm();

    return (tm->tm_year + 1900) * 10000 + (tm->tm_mon + 1) * 100 + tm->tm_mday;
}

time_t TimeUtils::minute_align_low(time_t time)
{
    struct tm tm;
    localtime_r(&time, &tm);

    tm.tm_sec = 0;

    return mktime(&tm); 
}

time_t TimeUtils::minute_align_high(time_t time)
{
    time_t time_low = TimeUtils::minute_align_low(time);

    if (time_low < time) {
        time_low += 60; 
    }

    return time_low;
}

uint32_t TimeUtils::parse_hour_min_str(const std::string& time_str)
{
    int hour = 0, minute = 0;
    sscanf(time_str.c_str(), "%d:%d", &hour, &minute);

    return hour * 100 + minute;
}

uint32_t TimeUtils::get_prev_friday_date()
{
    static uint32_t last_update_time = 0;
    static uint32_t prev_firday_date = 0;

    time_t now = get_now_tv()->tv_sec;

    // 一分钟更新一次
    if (last_update_time == now) {
        return prev_firday_date; 
    }

    last_update_time = now;

    for (int i = 0; i <= 7; i++) {
        now -= i * 24 * 3600; 
        struct tm tm;
        localtime_r(&now, &tm);

        if (tm.tm_wday == kFriday) {
            prev_firday_date = TimeUtils::time_to_date(now);

            return prev_firday_date;
        }
    }

    return prev_firday_date;
}

uint32_t TimeUtils::get_next_x_time(time_t begin_time, int x)
{
    if (x < 0) {
        return 0;
    }

    TRACE_TLOG("get_next_x_time:begin_time:%u;x:%d", begin_time, x);
    tm start_tm;
    localtime_r(&begin_time, &start_tm);

    /*
    tm next_week_monday = {0};
    next_week_monday.tm_year = start_tm.tm_year;
    next_week_monday.tm_mon = start_tm.tm_mon;
    next_week_monday.tm_mday = start_tm.tm_mday + 
        (start_tm.tm_wday == 0?1:8 - start_tm.tm_wday);
    time_t next_week_monday_time = mktime(&next_week_monday);
    */
    tm next_week_x = {0};
    next_week_x.tm_year = start_tm.tm_year;
    next_week_x.tm_mon = start_tm.tm_mon;
    next_week_x.tm_mday = start_tm.tm_mday + 
        (start_tm.tm_wday < (int)x?x - start_tm.tm_wday:7 - start_tm.tm_wday + x);
    time_t next_week_x_time = mktime(&next_week_x);
    return next_week_x_time;

}

uint32_t TimeUtils::get_last_x_time(time_t begin_time, int x)
{
    if (x < 0) {
        return 0;
    }

    TRACE_TLOG("get_last_x_time:begin_time:%u;x:%d", begin_time, x);
    tm start_tm;
    localtime_r(&begin_time, &start_tm);

    /*
    tm next_week_monday = {0};
    next_week_monday.tm_year = start_tm.tm_year;
    next_week_monday.tm_mon = start_tm.tm_mon;
    next_week_monday.tm_mday = start_tm.tm_mday + 
        (start_tm.tm_wday == 0?1:8 - start_tm.tm_wday);
    time_t next_week_monday_time = mktime(&next_week_monday);
    */
    tm next_week_x = {0};
    next_week_x.tm_year = start_tm.tm_year;
    next_week_x.tm_mon = start_tm.tm_mon;
    next_week_x.tm_mday = start_tm.tm_mday - 
        (start_tm.tm_wday >= (int)x?start_tm.tm_wday - x:7 + start_tm.tm_wday - x);
    time_t next_week_x_time = mktime(&next_week_x);
    return next_week_x_time;

}

std::string TimeUtils::time_to_string(time_t time)
{
    tm tm;  

    localtime_r(&time, &tm);

    char str_buf[256] = {0};

    snprintf(str_buf, sizeof(str_buf), 
            "%04d-%04d-%04d %02d:%02d:%02d",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour, 
            tm.tm_min,
            tm.tm_sec);

    return std::string(str_buf);
}
