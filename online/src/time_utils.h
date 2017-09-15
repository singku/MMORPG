#ifndef __TIME_UTILS_H__
#define __TIME_UTILS_H__

extern "C" {
#include <libtaomee/timer.h>
#include <libtaomee/log.h>
}
#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include "macro_utils.h"
extern uint32_t g_server_id;

//定义一天有多少秒 正常是86400
#define DAY_SECS (86400)
#define DAY_ADJ (57600)

enum {
    TM_CONF_KEY_ELEM_CHLG               = 1,        //元素挑战开放时间控制KEY
    TM_CONF_KEY_WEEKLY_LOGIN_PRIZE      = 2,        //周登陆奖励开放时间控制
    TM_CONF_KEY_WORLD_BOSS              = 3,        //世界boss开放时间控制
    TM_CONF_KEY_BUCKET                  = 4,        //一桶天下
    TM_CONF_KEY_CONSUME_DIAMOND          = 5,        //钻石消费礼
	TM_CONF_KEY_TEST_GIFT				= 6,		//数次邀请封测的奖励发放
    TM_CONF_KEY_PRIZE_ADJUST            = 8,        //奖励限时翻倍
    TM_CONF_KEY_CHARGE_DIAMOND_DRAW     = 9,        //充钻抽卡
	TM_CONF_KEY_CREATE_ROLE_PRIZE		= 10,		//创建角色奖励
	TM_CONF_KEY_ADDICTION_CLOSE		    = 11,		//防沉迷关闭时间
	TM_CONF_KEY_LOTTERY_FAVORABLE		= 12,		//抽卡优惠活动
	TM_CONF_KEY_MARLY_ANSWER_QUESTION	= 13,		//玛茵答题活动
	TM_CONF_KEY_RANKING_TIME_LIMIT   	= 14,		//限时排行榜
    TM_CONF_KEY_RANKING_GIFT_TIME_LIMIT = 15,   //限时排行榜领奖时间
    TM_CONF_KEY_DUP_OPEN                = 16,       //副本开放时间限定
	TM_CONF_KEY_MAYIN_FLOWER			= 17,		//玛音送花排行奖励
    TM_CONF_KEY_ACTIVITY_MARKET         = 18,       //活动商店开放时间限定
	TM_CONF_KEY_ACTIVITY_OPEN_TIME		= 20,		//活动开放时间
	TM_CONF_KEY_PET_BLESS		        = 23,		//伙伴祈福
	TM_CONF_KEY_KUROME_SEND_DESSERT     = 24,		//黑瞳送点心
    TM_CONF_KEY_RPVP_OPEN_TM            = 26,       //手动竞技场开放时间
    TM_CONF_KEY_10Y_GOLD_VIP_TM         = 27,       //10元勋章购买限制时间
    TM_CONF_KEY_CHARGE_DIAMOND_GET_GIFT = 28,       //冲钻送豪礼
	TM_CONF_KEY_SUMMER_WEEKLY_SIGN		= 29,		//暑假周签到

    TM_CONF_KEY_TEST_GLOABL_ATTR        = 100000,   //测试全服属性
};

enum {
    kSunday = 0, 
    kMonday = 1, 
    kTuesday = 2, 
    kWednesday = 3, 
    kThursday = 4, 
    kFriday = 5, 
    kSaturday = 6,
};

struct time_limit_t{
    time_limit_t() {
        tid = 0;
        start_time = 0;
        end_time = 0xFFFFFFFF;

        start_hour = 0;
        start_min = 0;
        start_second = 0;
        end_hour = 23;
        end_min = 59;
        end_second = 59;
        multi = 0;
    }
    uint32_t tid;
    uint32_t start_time;
    uint32_t end_time;

    uint32_t start_hour;
    uint32_t start_min;
    uint32_t start_second;
    uint32_t end_hour;
    uint32_t end_min;
    uint32_t end_second;

    uint32_t multi;
    std::vector<uint32_t> weekdays;
    /*uint32_t weekdays[7]; */
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

    //判断是否过去了一周时间(每周五的00:00作为一周的开始)
    static bool check_is_week_past(time_t start, time_t end);
    
    //判断是否过去了一月时间
    static bool check_is_month_past(time_t start, time_t end);
    
    /*
    //计算一个月有多长时间
    static int calc_days_of_month(int month, int year);
*/
    static uint32_t get_today_date();

    static uint32_t get_now_hm();

	//判断闰年
	static bool is_leap_year(const time_t time);
	//一个月最大天数	
    static uint32_t max_day_of_month(const time_t time);

    //时间转化为时分格式
    static uint32_t second_trans_hm(time_t time);

    static time_t minute_align_low(time_t time);

    static time_t minute_align_high(time_t time);

    // 解析xx:xx类型的字符串
    static uint32_t parse_hour_min_str(const std::string& time_str);

    // 获取上一个周五(而不是上周五)0点的日期
    // exp:今天周六 则上一个周五0点是昨天0点, 今天是周五,上一个周五0点就是今天0点)
    // 今天是周五, 上周五表示上一周的周五
    static uint32_t get_prev_friday_date();

    // 获取下一个周x的unix-time
    // from sunday(0~6)
    static uint32_t get_next_x_time(time_t begin_time, int x);

    // 获取上一个周x的unix-tim
    // from sunday(0~6)
    // 可能会获得当前天
    static uint32_t get_last_x_time(time_t begin_time, int x);

    // 获取当月第一天的起始时间
    static uint32_t get_cur_month_first_time(time_t time);

    // 获取一个时间的字符串形式 xxxx-xx-xx xx:xx:xx
    static std::string time_to_string(time_t time);

	/*********************************************
	 * time_config.xml配表相关
	 *********************************************/

	/**
	 * 注:weekday设置的时候 时间区间非连续的
	 * @brief 得到time_config.xml里的时间
     * @param key   time_config.xml中的id 
     * @param sub_key time_config.xml中的tid
	 * @param flag 1为开始，2为结束
	 * @return time_t类型的时间表示
	 */
	static uint32_t get_time(uint32_t key, uint32_t sub_key, int flag);
	/**
	 * @brief 得到active_config_pool.xml里的开始时间
     * @param key   time_config.xml中的id 
     * @param sub_key time_config.xml中的tid
	 * @return 成功为0，有问题非0
	 */
	static uint32_t get_start_time(uint32_t key, uint32_t sub_key);

	/**
	 * @brief 得到active_config_pool.xml里的结束时间
     * @param key   time_config.xml中的id 
     * @param sub_key time_config.xml中的tid
	 * @return 成功为0，有问题非0
	 */
	static uint32_t get_end_time(uint32_t key, uint32_t sub_key);

	/**
	 * @brief 判断当前时间是否合法
	 * @param key   time_config.xml中的id 
	 * @param sub_key time_config.xml中的tid
	 * @return 成功为0，有问题非0
	 */
	static bool is_current_time_valid( uint32_t key, uint32_t sub_key);

    /**
     * @brief获取当前时段的翻倍倍数 如果不翻倍则返回1 表示1倍
     */ 
    static uint32_t get_current_time_prize_multi(uint32_t sub_key);

    /**
     * @brief 判断time key是否存在
     */
    static bool is_valid_time_conf_key(uint32_t key);
    static bool is_valid_time_conf_sub_key(uint32_t key, uint32_t sub_key);

	/* 判断月份是偶数月 */
	static bool is_even_number_month(const time_t time);
    /** 
     * @brief 判断timestamp是否在配置时间内
     * 
     * @param key   time_config.xml中的id 
     * @param sub_key time_config.xml中的tid
                   0 表示检查所有子项配置 >0 检查对应子项配置，不存在时返回true
     * 
     * @return  true 在配置时间内 false 不在配置时间内
     */
    static bool is_time_valid(uint32_t timestamp, uint32_t key, uint32_t sub_key);

    static bool check_time_limit(const time_limit_t &time_limit, time_t timestamp);

    /** 
     * @brief 取time_config.xml里面的子项配置
     * 
     * @param id 
     * @param tid 
     * 
     * @return 
     */
    static const time_limit_t *get_time_limit_config(uint32_t id, uint32_t tid);


    /** 
     * @brief 取子配置项总数
     * 
     * @param id 
     * 
     * @return 
     */
    static uint32_t get_sub_time_config_num(uint32_t id);
	
    /** 
     * @brief : 获得两个时间点中的n个星期x的日期 
     * @param x: 
     * @param start_time: 
     * @param end_time: 
     * @param limit 最多取limit个
     * @return 
     */
	static uint32_t get_n_x_date_between_timestamp(
			uint32_t x, std::vector<uint32_t>& date_vec,
			uint32_t start_time, uint32_t end_time, uint32_t limit = 10);
    /** 
     * @brief : 获得两个时间点中的n个星期x的起始时间戳
     * @param x: 
     * @param start_time: 
     * @param end_time: 
     * @param limit: 最多取limit个
     * @return 
     */
	static uint32_t get_n_x_start_timestamp_between_timestamp(
			uint32_t x, std::vector<uint32_t>& date_vec,
			uint32_t start_time, uint32_t end_time, uint32_t limit = 10);
    /** 
     * @brief : 判断give_time指定的时间，是否超过give_time所在的当日的某个时刻
	 *			时刻 由 ：hour, minute, second 来指定
     * @param give_time: 
     * @param hour: 
     * @param minute: 
     * @param second: 
     * @return 
     */
	static bool test_gived_time_exceed_tm_point(
			time_t give_time,
			const int32_t hour, 
			const int32_t minute = 0, 
			const int32_t second = 0);

    /** 
     * @brief : 给定 时，分，秒 生成当日的对应的时间戳
     * @param hour: 
     * @param minute: 
     * @param second: 
     * @param give_time: 
     * @return 
     */
	static time_t cal_time_based_on_hms(
			uint8_t hour,  uint8_t minute = 0, 
			uint8_t second = 0, const time_t give_time = NOW());

	static uint32_t get_time_info_with_tm(struct tm& timeinfo,
			const uint32_t give_time);

    /** 
     * @brief : 检查给定时间所在的月的奇偶性；
     * @return : true:奇数；false:偶数
     */
	static bool check_month_odd(const uint32_t give_time);

    /** 
     * @brief : 主要登录定时器用
	 * key_end 排行活动结束时间, key_start 领取排行开始时间
	 * 如果当前时间在活动结束之后，领奖时间之前，则返回领奖开始时间
	 * 如果当前时间在领奖时间内，则登录发奖
	 * 其他情况不加定时器
     * @return : time
     */
	static uint32_t get_activity_rank_reward_time(uint32_t key_end,
			uint32_t subkey_end, uint32_t key_start, 
			uint32_t subkey_start);
};

//开服排行榜时间sub key
enum srv_time_subkey {
	TM_SUBKEY_NONE = 0,
	//活动时间
	TM_SUBKEY_DIAMOND_RECHARGE = 1, //冲值榜
	//领奖时间
	TM_SUBKEY_DIAMOND_RECHARGE_REWARD = 2, //充值榜发奖

	TM_SUBKEY_POWER = 3, //战力榜
	TM_SUBKEY_POWER_REWARD = 4, //战力榜发奖

	TM_SUBKEY_GOLD_CONSUME = 5,//金币消费榜
	TM_SUBKEY_GOLD_CONSUME_REWARD = 6,//金币消费榜发奖
};

class srv_time_manager_t{
	//开服排行榜时间key 每个服对应
	enum srv_time_key {
		TM_KEY_SRV_NONE = 0,
		TM_KEY_SRV_1 = 101,
		TM_KEY_SRV_2 = 102,
		TM_KEY_SRV_3 = 103,
		TM_KEY_SRV_4 = 104,
	};

	public:
	 srv_time_manager_t(){
		time_key = TM_KEY_SRV_NONE;
		time_subkey = TM_SUBKEY_NONE; 
	}

	bool time_check(){
		return TimeUtils::is_valid_time_conf_sub_key(time_key, TM_SUBKEY_DIAMOND_RECHARGE)
			&& TimeUtils::is_valid_time_conf_sub_key(time_key, TM_SUBKEY_DIAMOND_RECHARGE_REWARD)
			&& TimeUtils::is_valid_time_conf_sub_key(time_key, TM_SUBKEY_POWER)
			&& TimeUtils::is_valid_time_conf_sub_key(time_key, TM_SUBKEY_POWER_REWARD)
			&& TimeUtils::is_valid_time_conf_sub_key(time_key, TM_SUBKEY_GOLD_CONSUME)
			&& TimeUtils::is_valid_time_conf_sub_key(time_key, TM_SUBKEY_GOLD_CONSUME_REWARD);
	}

	int init(){
		bool result = false;
		switch(g_server_id){
			case 1:
				time_key = TM_KEY_SRV_1;
				break;
			case 2:
				time_key = TM_KEY_SRV_2;
				break;
			case 3:
				time_key = TM_KEY_SRV_3;
				break;
			case 4:
				time_key = TM_KEY_SRV_4;
				break;
			default:
				time_key = TM_KEY_SRV_NONE;
				// KERROR_LOG(time_key, "srv time manager err!");
				ERROR_TLOG("srv time manager err! srv_id=%u key=%u ", g_server_id, time_key);    
				return -1;
		}
		result = time_check();
		if(!result){
			return -1;
		}
		return 0;
	}
	uint32_t get_time_key(){return time_key;}
	uint32_t get_time_subkey(){return time_subkey;}

	inline bool is_now_time_valid(srv_time_subkey subkey){
		time_subkey = subkey;
		if(time_subkey == TM_SUBKEY_NONE){
			return false;
		}
		return TimeUtils::is_current_time_valid(time_key, time_subkey);
	}

	inline bool is_time_valid(srv_time_subkey subkey, uint32_t timestamp){
		time_subkey = subkey;
		if(time_subkey == TM_SUBKEY_NONE){
			return false;
		}
		return TimeUtils::is_time_valid(timestamp, time_key, time_subkey);
	}

	inline uint32_t get_start_time(srv_time_subkey subkey){
		time_subkey = subkey;
		if(time_subkey == TM_SUBKEY_NONE){
			return 0;
		}
		return  TimeUtils::get_start_time(time_key, time_subkey);
	}

	inline uint32_t get_end_time(srv_time_subkey subkey){
		time_subkey = subkey;
		if(time_subkey == TM_SUBKEY_NONE){
			return 0;
		}
		return  TimeUtils::get_end_time(time_key, time_subkey);
	}

	// //发奖开始时间
	// start_tm = g_srv_time_mgr.get_start_time(TM_SUBKEY_POWER_REWARD);
	// //活动结束时间
	// end_tm =  g_srv_time_mgr.get_end_time(TM_SUBKEY_POWER);
	// //到了领奖的时间内
	// now_tm = NOW();
	// if (g_srv_time_mgr.is_now_time_valid(TM_SUBKEY_POWER_REWARD)){
		// time = NOW() + 10;
	// } else if (now_tm  > end_tm && now_tm < start_tm){
		// time = start_tm;
	// }
	inline uint32_t get_send_time(srv_time_subkey activ_subkey,
			srv_time_subkey reward_subkey){

		uint32_t time = 0;
		uint32_t now_tm = NOW();
		//活动结束时间
		uint32_t end_tm =  get_end_time(activ_subkey);
		//发奖开始时间
		uint32_t start_tm = get_start_time(reward_subkey);
		//到了领奖的时间内
		if (is_now_time_valid(reward_subkey)){
			time = NOW() + 10;
		} else if (now_tm  > end_tm && now_tm < start_tm){
			time = start_tm + 1;
		}
		return time;
	}
	private:
	srv_time_key time_key;
	srv_time_subkey time_subkey; 
};

#endif
