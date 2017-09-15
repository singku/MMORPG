#include<time.h>
#include<stdio.h>
#include "time_utils.h"
extern "C" {
#include <libtaomee/log.h>
#include <libtaomee/timer.h>
}
#include "global_data.h"

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

    return 0;
}


int TimeUtils:: time_str_to_long(const char* time_str, uint32_t& time_value)
{
    struct tm* target_time;
    time_t rawtime;
    int year = 0,mon = 0,day = 0,hour = 0, min = 0,second = 0;

    sscanf(time_str, "%d-%d-%d %d:%d:%d", &year,&mon,&day,&hour,&min,&second);
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
    if (start <= end) {
        tm start_tm;
        localtime_r(&start, &start_tm);

        tm next_friday = {0};
        next_friday.tm_year = start_tm.tm_year;
        next_friday.tm_mon = start_tm.tm_mon;
        int32_t days_to_friday = (5 - start_tm.tm_wday + 7) % 7;
        if (start_tm.tm_wday == 5) {
            days_to_friday = 7;
        }
        next_friday.tm_mday = start_tm.tm_mday + days_to_friday;
        time_t next_friday_time = mktime(&next_friday);
        return end >= next_friday_time;
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

bool TimeUtils:: is_leap_year(const time_t time)
{
    const struct tm* tm = localtime(&time);	
   if (((tm->tm_year % 4 == 0) && (tm->tm_year % 100 != 0))
		   || (tm->tm_year % 400 == 0))
   {
	   return true;
   } else {
	   return false;
   }
}

uint32_t TimeUtils:: max_day_of_month(const time_t time)
{
	const struct tm* tm = localtime(&time);
	int mon = tm->tm_mon + 1;
	if (mon == 4 || mon == 6 || mon == 9 || mon ==11){
		return 30;
	} else if (mon == 2){
		return is_leap_year(time) ? 29 : 28;
	} else {
		return 31;
	}
}
uint32_t TimeUtils::get_now_hm() 
{
    const struct tm* tm = get_now_tm();
    return tm->tm_hour * 100 + tm->tm_min;
}

uint32_t TimeUtils::second_trans_hm(time_t time)
{
    struct tm tm;
    localtime_r(&time, &tm);

    return tm.tm_hour * 100 + tm.tm_min;
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
    if (last_update_time == now / 60) {
        return prev_firday_date; 
    }
    last_update_time = now;

    uint32_t last_friday_time = get_last_x_time(now, 5);
    prev_firday_date = TimeUtils::time_to_date(last_friday_time);
    return prev_firday_date;
}

/** 
 * @brief 取以begin_time开始的, 下周x0点的时间戳
 * 
 * @param begin_time 相对开始时间戳
 * @param x  0-6 表示周几
 * 
 * @return 
 */
uint32_t TimeUtils::get_next_x_time(time_t begin_time, int x)
{
    if (x < 0) {
        return 0;
    }

    tm start_tm;
    localtime_r(&begin_time, &start_tm);

    tm next_week_x = {0};
    next_week_x.tm_year = start_tm.tm_year;
    next_week_x.tm_mon = start_tm.tm_mon;
    int32_t days_to_next_x = (x - start_tm.tm_wday + 7) %7;
    if (x == start_tm.tm_wday) {
        days_to_next_x = 7;
    }
    next_week_x.tm_mday = start_tm.tm_mday + days_to_next_x;
    time_t next_week_x_time = mktime(&next_week_x);
    return next_week_x_time;
}

uint32_t TimeUtils::get_last_x_time(time_t begin_time, int x)
{
    if (x < 0) {
        return 0;
    }

    tm start_tm;
    localtime_r(&begin_time, &start_tm);

    tm last_week_x = {0};
    last_week_x.tm_year = start_tm.tm_year;
    last_week_x.tm_mon = start_tm.tm_mon;
    int32_t days_from_last_x = (start_tm.tm_wday - x + 7) % 7;
    /*
    if (start_tm.tm_wday == x) {
        days_from_last_x = 7;
    }
    */
    last_week_x.tm_mday = start_tm.tm_mday - days_from_last_x;
    time_t last_week_x_time = mktime(&last_week_x);
    return last_week_x_time;

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
//TODO vince
/**
 * @brief 得到time_config.xml里的时间
 * @param key   time_config.xml中的id 
 * @param sub_key time_config.xml中的tid
 * @param flag 1为开始，2为结束
 * @return time_t类型的时间表示
 */
uint32_t TimeUtils::get_time(uint32_t key, uint32_t sub_key, int flag)
{
	time_t t = 0;
	if(0 == key || 0 == sub_key){
		ERROR_TLOG("is_time_valid failed, not found time_id:key=%u,tid=%u",
				key, sub_key); 
		return t;
	}
    std::map<uint32_t, TIME_CONFIG_LIMIT_T >::iterator iter = g_time_config.find(key);
	if (iter != g_time_config.end()) {
		// 不配活动时间表示永久有效
		if (iter->second.size() == 0) {
			ERROR_TLOG("get_start_time failed, [code 1] not found time_id:%u, tid:%u",
				   	key, sub_key); 
			return t;
		}
		// 子项时间检查
        TIME_CONFIG_LIMIT_T::iterator t_iter = iter->second.find(sub_key);
		if (t_iter != iter->second.end()) {// 时间，日期，每周星期的交集
			time_limit_t time_limit = t_iter->second;
			if (flag == 1){
				//日期
				t = t_iter->second.start_time;
				//时间计算
				time_t tmp = day_align_low(t);
				tmp += t_iter->second.start_hour*60*60;
				tmp += t_iter->second.start_min * 60;
				tmp += t_iter->second.start_second;
				t = tmp > t ? tmp : t;
				// 周日期计算
				/*
				 * if (time_limit.weekdays.size() != 0) {
				 *     bool flag = false;//判断当前时间是否在周日期内
				 *     int big_wday = 0;//大于当前周日期的最小日期
				 *     struct tm in_time;
				 *     localtime_r(&t, &in_time);
				 *     FOREACH(time_limit.weekdays, w_iter) {
				 *         int wday = (*w_iter == 7) ? kSunday : (*w_iter);
				 *         if (in_time.tm_wday == wday) {
				 *             flag = true;
				 *             break;
				 *         } else if(in_time.tm_wday < wday){
				 *             big_wday = wday;
				 *         }
				 *     }
				 *     //当前时间不在周日期内,取大于它的最小日期
				 *     if(flag == false){
				 *         if (in_time.tm_wday == kSunday) {//日期为下周
				 *             uint32_t diff_time = get_next_x_time(t, big_wday);
				 *             t += diff_time; 
				 *         } else {
				 *             in_time.tm_wday = big_wday;
				 *         }
				 *     }
				 * }
				 */
			} else if (flag == 2){
				t = t_iter->second.end_time;
				time_t tmp = day_align_low(t);
				tmp += t_iter->second.end_hour*60*60;
				tmp += t_iter->second.end_min * 60;
				tmp += t_iter->second.start_second;
				t = tmp > t ? t : tmp;
				// 周日期计算
				/*
				 * if (time_limit.weekdays.size() != 0) {
				 *     bool flag = false;//判断当前时间是否在周日期内
				 *     int small_wday = 6;//小于当前周日期的最大日期
				 *     struct tm in_time;
				 *     localtime_r(&t, &in_time);
				 *     REVERSE_FOREACH(time_limit.weekdays, w_iter) {
				 *         int wday = (*w_iter == 7) ? kSunday : (*w_iter);
				 *         if (in_time.tm_wday == wday) {
				 *             flag = true;
				 *             break;
				 *         } else if(in_time.tm_wday > wday){
				 *             small_wday = wday;
				 *         }
				 *     }
				 *     //当前时间不在周日期内,取大于它的最小日期
				 *     if(flag == false){
				 *         if (in_time.tm_wday == kMonday) {//日期为上周
				 *             uint32_t diff_time = get_last_x_time(t, small_wday);
				 *             t += diff_time; 
				 *         } else {
				 *             in_time.tm_wday = small_wday;
				 *         }
				 *     }
				 * }
				 */
			}
			return t;
		} else {
			ERROR_TLOG("get_start_time failed, [code 3] not found time_id:%u, tid:%u",
				   	key, sub_key); 
			return t;
		}
	}
	ERROR_TLOG("get_start_time failed, [code 2] not found time_id:%u, tid:%u",
			key, sub_key); 
	return t;
}

/**
 * @brief 得到active_config_pool.xml里的开始时间
 * @param key   time_config.xml中的id 
 * @param sub_key time_config.xml中的tid
 * @return 成功为0，有问题非0
 */
uint32_t TimeUtils::get_start_time(uint32_t key, uint32_t sub_key)
{
	return get_time(key, sub_key, 1);
}

/**
 * @brief 得到active_config_pool.xml里的结束时间
 * @param key   time_config.xml中的id 
 * @param sub_key time_config.xml中的tid
 * @return 成功为0，有问题非0
 */
uint32_t TimeUtils::get_end_time(uint32_t key, uint32_t sub_key)
{
	return get_time(key, sub_key, 2);
}

/**
 * @brief 判断当前时间是否合法
 * @param key   time_config.xml中的id 
 * @param sub_key time_config.xml中的tid
 * @return 成功为true，有问题false
 */
bool TimeUtils::is_current_time_valid( uint32_t key, uint32_t sub_key)
{
	uint32_t timestamp = NOW();
	return is_time_valid(timestamp, key, sub_key);
}

uint32_t TimeUtils::get_current_time_prize_multi(uint32_t sub_key)
{
    if (!is_time_valid(NOW(), TM_CONF_KEY_PRIZE_ADJUST, sub_key)) {
        return 1;//不翻倍 就是当前倍数1倍
    }
    std::map<uint32_t, TIME_CONFIG_LIMIT_T >::iterator iter = 
        g_time_config.find(TM_CONF_KEY_PRIZE_ADJUST);

    if (iter == g_time_config.end()) {
        return 1;
    }

    if (iter->second.count(sub_key) == 0) {
        return 1;
    }
    const time_limit_t &tl = (iter->second.find(sub_key))->second;
    return tl.multi;
}

bool TimeUtils::is_valid_time_conf_key(uint32_t key)
{
    return (g_time_config.count(key) == 0 ?false :true);
}

bool TimeUtils::is_valid_time_conf_sub_key(uint32_t key, uint32_t sub_key)
{
    if (!is_valid_time_conf_key(key)) {
        return false;
    }
    return (((g_time_config.find(key))->second).count(sub_key) == 0 ?false :true);
}

/** 
 * @brief 判断timestamp是否在配置时间内
 * 
 * @param key   time_config.xml中的id 
 * @param sub_key time_config.xml中的tid
               0 表示检查所有子项配置 >0 检查对应子项配置，不存在时返回true
 * 
 * @return  true 在配置时间内 false 不在配置时间内
 */
bool TimeUtils::is_time_valid(uint32_t timestamp, uint32_t key, uint32_t sub_key)
{
    bool ret = false;
	if(0 == key){
		ERROR_TLOG("is_time_valid failed, not found time_id:key=%u", key); 
		return ret;
	}
    std::map<uint32_t, TIME_CONFIG_LIMIT_T >::iterator iter = g_time_config.find(key);
    // 没有对应配置项表示永久有效
    if(iter == g_time_config.end()) {
		ret = true;
		return ret;
    }
    // 不配子项表示永久有效
    if (iter->second.size() == 0) {
        return true;
    }
    if (sub_key == 0) {
        // 遍历子项检查 
        FOREACH(iter->second, t_iter) {
            ret = check_time_limit(t_iter->second, (time_t)timestamp);
            if (ret == true) {
                break;
            }
        }
    } else {
        // 检查特定子项
        TIME_CONFIG_LIMIT_T::iterator t_iter = iter->second.find(sub_key);
        if(t_iter == iter->second.end()) {
			ret = true;
			return ret;
        }

        ret = check_time_limit(t_iter->second, (time_t)timestamp);
    }

    return ret;
}

bool TimeUtils::check_time_limit(const time_limit_t &time_limit, time_t timestamp)
{
    uint32_t flag = 0;
    // 日期时间检查
    if (timestamp >= time_limit.start_time && 
            timestamp <= time_limit.end_time) {
        flag = taomee::set_bit_on(flag, 1);
    } else {
		return false;
	}

    // 小时时间检查
    struct tm tmp_start_time;
    struct tm tmp_end_time;
    localtime_r(&timestamp, &tmp_start_time);
    tmp_start_time.tm_hour = time_limit.start_hour;
    tmp_start_time.tm_min = time_limit.start_min;
    tmp_start_time.tm_sec = time_limit.start_second;
    uint32_t tmp_start = mktime(&tmp_start_time);

    localtime_r(&timestamp, &tmp_end_time);
    tmp_end_time.tm_hour = time_limit.end_hour;
    tmp_end_time.tm_min = time_limit.end_min;
    tmp_end_time.tm_sec = time_limit.end_second;
    uint32_t tmp_end = mktime(&tmp_end_time);

    if (timestamp >= tmp_start && timestamp <= tmp_end) {
        flag = taomee::set_bit_on(flag, 2);
    } else {
		return false;
	}

    // 周日期检查
    if (time_limit.weekdays.size() == 0) {
        flag = taomee::set_bit_on(flag, 3);
    } else {
        struct tm in_time;
        localtime_r(&timestamp, &in_time);
        FOREACH(time_limit.weekdays, w_iter) {
            int wday = (*w_iter == 7) ? kSunday : (*w_iter);
            if (in_time.tm_wday == wday) {
                flag = taomee::set_bit_on(flag, 3);
                break;
            }
        }
    }

    if (flag == 0x7) {
        return true;
    }

    return false;
}

const time_limit_t *TimeUtils::get_time_limit_config(uint32_t id, uint32_t tid)
{
    time_limit_t *time_limit = NULL;
    std::map<uint32_t, TIME_CONFIG_LIMIT_T >::iterator iter = g_time_config.find(id);
    if (iter != g_time_config.end()) {
        TIME_CONFIG_LIMIT_T::iterator t_iter = iter->second.find(tid);
        if (t_iter != iter->second.end()) {
            time_limit = &(t_iter->second);
        }
    }
    return time_limit;
}

uint32_t TimeUtils::get_sub_time_config_num(uint32_t id)
{
    std::map<uint32_t, TIME_CONFIG_LIMIT_T >::iterator iter = g_time_config.find(id);
    if (iter != g_time_config.end()) {
        return iter->second.size(); 
    }
    return 0;
}

uint32_t TimeUtils::get_n_x_date_between_timestamp(
		uint32_t x, std::vector<uint32_t>& date_vec, 
		uint32_t start_time, uint32_t end_time, uint32_t limit)
{
    if (start_time > end_time) {
        return 0;
    }
    uint32_t cnt = 0;
    date_vec.clear();
	while (end_time >= start_time) {
		end_time = TimeUtils::get_last_x_time(end_time, x);
		if (end_time < start_time) {
			break;
		}
		uint32_t date = TimeUtils::time_to_date(end_time);
		date_vec.push_back(date);
        end_time -= 60;
        if (limit && ++cnt >= limit) {
            break;
        }
	}
	return 0;
}

uint32_t TimeUtils::get_n_x_start_timestamp_between_timestamp(
		uint32_t x, std::vector<uint32_t>& timestamp_vec, 
		uint32_t start_time, uint32_t end_time, uint32_t limit)
{
    if (start_time > end_time) {
        return 0;
    }
    timestamp_vec.clear();
    uint32_t cnt = 0;
	while (end_time >= start_time) {
		end_time = TimeUtils::get_last_x_time(end_time, x);
		timestamp_vec.push_back(end_time);
        end_time -= 60;
        if (limit && ++cnt >= limit) {
            break;
        }
	}
	return 0;
}

bool TimeUtils::test_gived_time_exceed_tm_point(
		time_t give_time,
		const int32_t hour, 
		const int32_t minute, 
		const int32_t second)
{
	time_t CUR_TM = give_time;
	tm tm_point;
	localtime_r(&CUR_TM, &tm_point);
	if (tm_point.tm_hour > hour) {
		return true;
	} else if (tm_point.tm_hour == hour) {
		if (tm_point.tm_min > minute) {
			return true;
		} else if (tm_point.tm_min == minute) {
			if (tm_point.tm_sec >= second) {
				return true;
			} else {
				return false;
			}
		} else {
			return false;
		}
	} else {
		return false;
	}
}

time_t TimeUtils::cal_time_based_on_hms(
		uint8_t hour, uint8_t minute, 
		uint8_t second, const time_t give_time)
{
    time_t rawtime = give_time;
	struct tm timeinfo;
	localtime_r(&rawtime, &timeinfo);
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    rawtime = mktime(&timeinfo);
    return rawtime;
}

uint32_t TimeUtils::get_time_info_with_tm(
		struct tm& timeinfo, 
		const uint32_t give_time)
{
	time_t cur_time = give_time;
	localtime_r(&cur_time, &timeinfo);
	return 0;
}

bool TimeUtils::check_month_odd(const uint32_t give_time)
{
	struct tm timeinfo;
	get_time_info_with_tm(timeinfo, give_time);
	uint32_t cur_month = timeinfo.tm_mon + 1;
	if (cur_month % 2 == 0) {
		return false;
	}
	return true;
}

bool TimeUtils::is_even_number_month(const time_t time)
{
	struct tm *target_time;
	target_time = localtime(&time);
	int month = -1;
	month = target_time->tm_mon + 1;
	if(month < 1 || month > 12){
		ERROR_TLOG("month num error! mon=[%u]", month); 
		return -1;
	}

	if(0 == month % 2){
		return true;
	} else {
		return false;
	}

}

uint32_t TimeUtils::get_activity_rank_reward_time(uint32_t key_end,
			uint32_t subkey_end, uint32_t key_start, uint32_t subkey_start)
{
	uint32_t time = 0;
	uint32_t now_tm = NOW();
	//活动结束时间
	// uint32_t end_tm = get_end_time(key_end, subkey_end);
	//发奖开始时间
	uint32_t start_tm = get_start_time(key_start, subkey_start);
	//到了领奖的时间内
	if (is_current_time_valid(key_start, subkey_start)){
		time = NOW() + 10;
	} else if (now_tm < start_tm){
		time = start_tm + 1;
	}
	return time;
}
