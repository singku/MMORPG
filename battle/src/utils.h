#ifndef UTILS_H
#define UTILS_H

#include "common.h"

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
            asynsvr_send_warning_msg("AssertErr", get_server_id(), 0, 0,  \
                    get_server_name()); \
        } \
    } while (0);

class Utils
{
public:
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
     * 随机从n个数中选择m个不相同的出来, n个数各有自己的概率r
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

    static inline uint32_t get_date()
    {
        const tm* tm = get_now_tm(); 

        return ((tm->tm_year + 1900) * 10000 + ((tm->tm_mon + 1) * 100) + tm->tm_mday);
    }

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
     * @brief 找到整形32位数中第一个为0的比特位的下标 从右往左 0-31
     */
    static bool find_first_zero(uint32_t var, uint32_t &idx);
    static bool find_first_one(uint32_t var, uint32_t &idx);


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
};

#endif
