#ifndef UTILS_H
#define UTILS_H

#include "common.h"
#include <uuid.h>
#include "dll_iface.h"
#include "statlogger/statlogger.h"
#include <math.h>

#define STRCPY_SAFE(buf, str) \
    do { \
        int buflen = sizeof(buf); \
        buf[buflen - 1] = '\0';\
        strncpy(buf, str, buflen - 1); \
    } while (0);

#define HASH_RANDOM_MASK 1463735687
#define HASH_RANDOM_MASK2 1653893711  

#define MY_ASSERT(exp) \
    do { \
        if (!(exp)) { \
            ERROR_LOG("assert '%s' failed, stack = %s", #exp, stack_trace().c_str()); \
            asynsvr_send_warning_msg("AssertErr", g_online_id, 0, 0,  \
                    get_server_name()); \
        } \
    } while (0);

#define CHECK_MSG_DATA_IN_CLOSED_INTERVAL(type, val, min, max, errno) \
    do { \
        if (!Utils::is_in_closed_interval<type>(val, min, max)) { \
            return send_err_to_player( \
                player, player->cli_wait_cmd, errno); \
        } \
    } while (0) \

#define CONDITION_CHECK(condition) \
	do{ \
		if(Utils::is_true(condition)) \
		; \
		else \
		ERROR_TLOG("conditon" #condition "failed !"); \
	}while(0)\


extern server_config_t g_server_config;
extern StatLogger *g_stat_logger;

struct rand_name_pool_t {
    std::vector<std::string> name_pool[commonproto::MAX_RAND_NAME_TAG_POS_TYPE];
};


class Utils
{
public:
	inline static bool is_true (bool condition){return condition;}
    /**
     * @brief  is_cool_down 是否达到冷却时间
     *
     * @param start_time 起始时间
     * @param cd 冷却时间间隔
     *
     * @return true已经冷却 false未冷却
     */
    inline static bool is_cool_down(uint32_t start_time, uint32_t cd)
    {
        uint32_t now = get_now_tv()->tv_sec;

        if (now < start_time) {
            // 时间倒退
            return false; 
        } else if (now >= start_time && now < start_time + cd - 5) {
            // 5秒内误差
            return false; 
        } else {
            return true; 
        }
    }

    static std::string bin2hex(const std::string& bin);

    /**
     * @brief  rand_select_k 从[low,high]中随机挑选出k个数
     *
     * @param low 起始数
     * @param high 终止数
     * @param k 选出k个
     * @param list 保存选出的数，从小到大有序排列
     *
     * @return 0 成功 -1失败
     */
    inline static int rand_select_k(int low, int high, int k, std::vector<uint32_t>& list)
    {
        if (high - low + 1 < k) {
            return -1; 
        }

        for (int i = 1; i <= k; i++) {
            uint32_t n = taomee::ranged_random(low + k - i, high); 
            list.push_back(n);
            high = n - 1;
        }

        return 0;
    }
    
    static void select_n_from_m(uint32_t begin, uint32_t end, uint32_t n, std::vector<uint32_t>& list);
    //从保存概率的一个vector里面找出一个来。
    //返回的值从0开始。
    static uint32_t select_from_rate_list(std::vector<uint32_t>& list);

    /*
     * 随机选择 nr概率表, first为概率的标识, second为概率, 概率基数为10000
     * idx为返回的命中列表, 为first的集合
     */
    static bool rand_select_from_set(std::map<uint32_t, uint32_t> nr, std::set<uint32_t> &idx) 
    {
        idx.clear();
        FOREACH(nr, it) {
            uint32_t value = ranged_random(0, 10000);
            if (value <= it->second) {
                idx.insert(it->first);
            }
        }
        return true;
    }

    /*
     * 随机从n个数中选择m个不相同的出来, n个数各有自己的权重r
     * 返回值为false表示选择失败 为true表示成功 选择到的数值的下标放在idx set里面 从0开始
    */
    static bool rand_select_uniq_m_from_n_with_r(std::map<uint32_t, uint32_t> nr, 
            std::set<uint32_t> &idx, uint32_t m)
    {
        if (nr.size() < m) {
            idx.clear();
            return false;
        }
        if (nr.size() == m) {
            idx.clear();
            FOREACH(nr, it) {
                idx.insert(it->first);
            }
            return true;
        }
        
        idx.clear();
        std::map<uint32_t, uint32_t> nr_map;
        nr_map.clear();
        FOREACH(nr, it) {
            nr_map[it->first] = it->second;
        }

        std::map<uint32_t, uint32_t> rate_map;
        uint32_t sum;
        std::map<uint32_t, uint32_t>::iterator it;
        for (uint32_t i = 0; i < m; i++) {
            //重算总和
            sum = 0;
            rate_map.clear();
            FOREACH(nr_map, it2) {
                sum += it2->second;
                rate_map[sum] = it2->first;
            }
            int val = ranged_random(0, sum);
            it = rate_map.lower_bound(val);
            if (it == rate_map.end()) {
                it --;
            }
            idx.insert(it->second);
            nr_map.erase(it->second);
        }
        return true;
    }

    static std::string to_string(uint32_t n);

    static std::string to_string(int n);

    static std::string to_string(uint64_t n);

    static inline uint32_t get_date()
    {
        const tm* tm = get_now_tm(); 

        return ((tm->tm_year + 1900) * 10000 + ((tm->tm_mon + 1) * 100) + tm->tm_mday);
    }
#if 0
    /**
     * @brief  write_msglog 写人数/人次统计项
     *
     * @param userid 米米号
     * @param msgid 统计id
     * @param level 等级,默认1表示只统计人数人次, 否则是等级分布
     */
    static inline void write_msglog(uint32_t userid, uint32_t msgid, uint32_t level = 1)
    {
        static uint32_t msgbuf[2] = {0};

        uint32_t now = get_now_tv()->tv_sec;

        msgbuf[0] = userid;
        msgbuf[1] = level;

        msglog(g_server_config.statistic_file, 
                msgid, now, msgbuf, sizeof(msgbuf));
    }

    /**
     * @brief  write_msglog_count 写数量统计项
     *
     * @param count 数量
     * @param msgid 统计id
     */
    static inline void write_msglog_count(uint32_t count, uint32_t msgid)
    {
        uint32_t now = get_now_tv()->tv_sec;

        msglog(g_server_config.statistic_file, 
                msgid, now, &count, sizeof(count));
    }

    /**
     * @brief  write_msglog_input 统计消耗
     *
     * @param count 消耗数量
     * @param msgid 消息id
     */
    static inline void write_msglog_input(uint32_t count, uint32_t msgid)
    {
        static uint32_t msgbuf[2] = {0};
        uint32_t now = get_now_tv()->tv_sec;

        msgbuf[0] = 0;
        msgbuf[1] = count;
    
        msglog(g_server_config.statistic_file, 
                msgid, now, msgbuf, sizeof(msgbuf));
    }

    /**
     * @brief  write_msglog_output 统计产出
     *
     * @param count 产出数量
     * @param msgid 消息id
     */
    static inline void write_msglog_output(uint32_t count, uint32_t msgid)
    {
        static uint32_t msgbuf[2] = {0};
        uint32_t now = get_now_tv()->tv_sec;

        msgbuf[0] = count;
        msgbuf[1] = 0;
    
        msglog(g_server_config.statistic_file, 
                msgid, now, msgbuf, sizeof(msgbuf));
    }
#endif

    static inline uint32_t hex2dec(const char* str)
    {
        char* end_ptr; 
        long int val = strtol(str, &end_ptr, 16);

        if (*end_ptr != '\0') {
            return 0; 
        } else {
            return val; 
        }
    }

    // from innodb 
    static inline uint32_t fold_uint_pair(uint32_t n1, uint32_t n2)                                       
    {
        return (((((n1 ^ n2 ^ HASH_RANDOM_MASK2) << 8) + n1)
                    ^ HASH_RANDOM_MASK) + n2);
    }   

    // from innodb buffer pool checksum
    static inline uint32_t fold_binary(const uint8_t* str, int len)
    {
        const uint8_t* str_end = str + len;
        uint32_t fold = 0;

        while (str < str_end) {

            fold = fold_uint_pair(fold, (uint32_t)(*str));

            str++;
        }

        return (fold);
    }

    /**
     * @brief  gen_verify_question 生成验证图片
     */
    static inline void gen_verify_question(
            std::string& question, int* answer)
    {
        // 生成 0 ~ 20 以内加减法          
        bool add_op = rand() % 2; // 是否加法运算
        int low = 0;
        int high = 20;
        int num1 = ranged_random(low, high);
        int num2 = low;
        char question_buf[20];

        if (add_op) {
            num2 = ranged_random(low, high - num1);  
            snprintf(question_buf, sizeof(question_buf), "%d+%d", num1, num2);
            *answer = num1 + num2;
        } else {
            num2 = ranged_random(low, high);
            if (num1 < num2) {
                std::swap(num1, num2);
            }
            snprintf(question_buf, sizeof(question_buf), "%d-%d", num1, num2);
            assert(num1 >= num2);
            *answer = num1 - num2;
        }

        assert(*answer >= low && *answer <= high);
        question = std::string(question_buf);
    }

    /**
     * @brief 找到整形32位数中第一个为0的比特位的下标 从右往左 0-31
     */
    static bool find_first_zero(uint32_t var, uint32_t &idx);
    static bool find_first_one(uint32_t var, uint32_t &idx);

    /**
     * @brief  write_msglog_new 新统计平台人数/人次
     *
     * @param userid 米米号
     * @param dir 统计平台的分类目录
     * @param name 统计项分类(活动名称)
     * @param subname 子统计项(活动中的统计项名称)
     */
    static void write_msglog_new(userid_t userid, 
            const std::string& dir, const std::string& name, const std::string& subname);

    static void write_msglog_count(userid_t userid, const std::string &dir, 
            const std::string &name, const std::string &subname, uint32_t count);

    // 对list随机排序
    template<class T>
    static void rand_sort(std::vector<T>& list)
    {
        if (list.size() == 0) {
            return;
        }
        for (uint32_t i = 0; i < list.size() - 1; i++) {
            uint32_t swap_idx = taomee::ranged_random(i + 1, list.size() - 1);
            std::swap(list[i], list[swap_idx]);
        }
    }

    template<typename T>
    static bool is_in_closed_interval(T value, T min, T max)
    {
        if (value >= min && value <= max) {
            return true;
        }
        return false;
    }

    // 从name_pool.xml配置生成一个随机名字
    static std::string get_rand_name(uint32_t type);

    // 检查活动是否开启
    // type，活动类型，sys-ctrl里面配置
    // time，检查活动的时间，比如NOW()
    // 判断依据，首先判断open是否为1，无配置的话默认为1
    // 然后判断是否在start-time和end-tiggme的时间段内
    static bool is_activity_open(uint32_t type, time_t time);

    // 过滤msg的非法字符
    static std::string clean_msgname(const std::string& name)
    {
        std::string name_new = name;

        for (uint32_t i = 0; i < name_new.size(); i++) {

            if (name_new[i] == '='
                    || name_new[i] == ':'
                    || name_new[i] == ','
                    || name_new[i] == ';'
                    || name_new[i] == '.'
                    || name_new[i] == '/'
                    || name_new[i] == '['
                    || name_new[i] == ']'
                    || name_new[i] == '|'
					|| name_new[i] == '\t') {
                name_new[i] = '_';
            }
        }

        return name_new;
    }


    static inline void gen_uuid(std::string *uuid)
    {
        uuid_t uu;
        static const int kUUID_STR_LEN	= 36;
        char uu_str[kUUID_STR_LEN+1]; // 36 + '\0'
        uuid_generate(uu);
        uuid_unparse_lower(uu, uu_str);
        uuid->assign(uu_str);
    }

    static inline std::string gen_uuid(void)
    {
        std::string ret;
        gen_uuid(&ret);
        return ret;
    }


    static int check_dirty_name(const std::string &name, bool flag = true);

    /** 
     * @brief 跨服通知
     * 
     * @param tran_type: 玩家，服务器，全世界，系统通知
     * @param cmd: 传递给客户端协议的命令号
	 * @param message: 传递给客户端协议
	 * @param recv_id: 如果tran_type为switch::SWITCH_TRANSMIT_USERS
	 * 				   则reccv_id填写被通知到的玩家的米米号
	 * @param svr_id:  如果tran_type为switch::SWITCH_TRANSMIT_SERVERS
					   则svr_id填写被通知到的进程服id号
     * @return 0 成功， -1 失败
     */
	static int switch_transmit_msg(
			switchproto::switch_transmit_type_t tran_type,
			uint16_t cmd, const google::protobuf::Message& message,
			const std::vector<uint32_t> *recv_id = 0,
			const std::vector<uint32_t> *svr_id = 0);

    /** 
     * @brief 判断字符串是否纯数字
     * 
     * @param str 字符串
     * 
     * @return true 是 false 否
     */
    static bool is_number(const string &str) {
        bool ret = true;
        for(uint32_t i = 0; i < str.size();i++) {
            if (!(str[i] >= '0' && str[i] <= '9')) {
                ret = false;
                break;
            }
        }
        return ret;
    }

    //获得以左下角为坐标原点的平面上某个点为圆心半径为R上的圆弧上角度为A的点的坐标
    /**
     * @params x y 坐标 r半径 a角度[0-360), t_x, t_y得到的坐标
     */
    static void get_x_y(int x, int y, int r, int a, int &t_x, int &t_y) {
        a = a%360;
        double d_x;
        double d_y;
        #define PIE (3.1415926)
        double sp = sin(a*PIE/180);
        double cp = cos(a*PIE/180);
        d_y = sp * r;
        d_x = cp * r;
        t_x = x + d_x + 0.5; //+0.5以便整数四舍五入
        t_y = y + d_y + 0.5;
        return;
    }

	static uint32_t comp_u16(uint16_t high, uint16_t low) {
		uint32_t a = high;
		a = a << 16; 
		a = a + low;
		return a;
	}

	static void decomp_u16(uint32_t val, uint16_t &high, uint16_t &low) {
		high = val >> 16; 
		low = val;
	}
};

inline int atoi_safe(const char* str)
{
	if (str == NULL) {
		return 0;
	} else {
		return atoll(str);
	}
}

#endif
