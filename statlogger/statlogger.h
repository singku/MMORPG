#ifndef STAT_STATLOGGER_HPP_INCLUDED
#define STAT_STATLOGGER_HPP_INCLUDED

#include <stdint.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>



// 根据delim分解字符串
std::vector<std::string> stat_split(const std::string& s, char delim);
// 去除字符串s头尾的charlist字符
void stat_trim(std::string& s, const std::string& charlist);
// 判断给定的字符串是否utf8编码
bool stat_is_utf8(const std::string& s);

//整形转为字符串
std::string stat_itostr(int64_t para);
std::string stat_itostr(int32_t para);
std::string stat_itostr(uint64_t para);
std::string stat_itostr(uint32_t para);

inline bool size_between(const std::string& s, std::string::size_type min, std::string::size_type max)
{
	return ((s.size() >= min) && (s.size() <= max));
}

inline bool value_no_invalid_chars(const std::string& v)
{
	return (v.find_first_of("=|\t") == std::string::npos);
}


// 统计信息
// 本类非线程安全，如果要在多线程下使用，请为每个线程单独创建一个对象
class StatInfo {
public:

    /**
     * @brief  修改时需同步修改C接口中相应字段 
     */
	enum OpCode {
		op_begin,

		op_sum,    // 把某个字段某时间段内所有值相加
		op_max,    // 求某字段某时间段内最大值
	    op_set,    //直接取某字段最新的数值
        op_ucount, //对某个字段一段时间的值做去重处理

		op_item,      // 求某个大类下的各个item求人数人次
		op_item_sum,  // 对各个item的产出数量/售价等等求和
		op_item_max,  // 求出各个item的产出数量/售价等等的最大值
        op_item_set,  //求出每个item的最新数值

		op_sum_distr, // 对每个人的某字段求和，然后求出前面的“和”在各个区间下的人数
		op_max_distr, // 对每个人的某字段求最大值，然后求出前面的“最大值”在各个区间下的人数
		op_min_distr, //对每个人的某字段求最小值，然后根据前面的最小值在各个区间下做人数分布
        	op_set_distr, //取某个字段的最新值，做分布


		op_ip_distr,     // 根据ip字段求地区分布的人数人次


		op_end
	};

public:
	StatInfo()
	{
		m_has_op = false;
	}

	/**
	 * @brief 添加一项数据。最多只能增加30个kev-value对。
	 * @param key 键。不能以_开头或结束，StatInfo会自动过滤，比如_KEY_，会自动被修改成KEY；
	 *        不能够有“= : , ; . | \t”字符中的任何一个，否则程序运行时会崩溃
	 * @param value 值
	 */
	void add_info(std::string key, float value);
	/**
	 * @brief 添加一项数据。最多只能增加30个kev-value对。
	 * @param key 键。不能以_开头或结束，StatInfo会自动过滤，比如_KEY_，会自动被修改成KEY；
	 *        不能够有“= : , ; . | \t”字符中的任何一个，否则程序运行时会崩溃
	 * @param value 值。不能有“= | \t”字符中的任何一个，否则程序运行时会崩溃
	 */
	void add_info(std::string key, std::string value);
	/**
	 * @brief 添加一项操作。
	 * @param key1 键1。key1必须已经通过add_info方法添加好了
	 * @param key2 键2。key2必须已经通过add_info方法添加好了。仅当op的值为op_distr_use、
	 *             op_distr_sum、op_distr_max、op_distr_min之一时，key2的值才有意义
	 */
	void add_op(OpCode op, std::string key1, std::string key2 = "");
	/**
	 * @brief 清空已经添加了的所有数据
	 */
	void clear();

public:
	friend std::ostream& operator<<(std::ostream& out, const StatInfo& data);

private:
	typedef std::map<std::string, std::string> Kv;
	typedef std::set<std::string> OpKey;

private:
	static const uint32_t sc_key_maxsz   = 64;
	static const uint32_t sc_value_maxsz = 64;

private:
	void serialize(std::ostream& out) const;
	void serialize_op(std::ostream& out, const char *op,  const OpKey& keys) const;
	bool is_valid_key(const std::string& key) const
	{
		return (size_between(key, 1, sc_key_maxsz)
				&& (key.find_first_of("=:,;|\t") == std::string::npos)
				&& stat_is_utf8(key));
	}
	bool is_valid_value(const std::string& value) const
	{
		return (size_between(value, 1, sc_value_maxsz)
				&& value_no_invalid_chars(value)
				&& stat_is_utf8(value));
	}
	bool is_valid_op(OpCode op) const
	{
		return ((op > op_begin) && (op < op_end));
	}

private:
	Kv m_info;
	bool m_has_op;
	OpKey m_ops[op_end];
};

inline std::ostream& operator<<(std::ostream& out, const StatInfo& data)
{
	data.serialize(out);
	return out;
}

// 本类非线程安全，如果要在多线程下使用，请为每个线程单独创建一个对象
// key/value中都不能有“= | \t”字符中的任何一个，若有，则程序直接崩溃
class StatLogger {
public:
	enum CurrencyType {
		ccy_begin	= 0,

		ccy_mibi	= 1, /*! 米币 */
		ccy_cny		= 2, /*! 人民币 */
		//ccy_usd		= 3, /*! 美元 */
		//ccy_eur		= 4, /*! 欧元 */

		ccy_end
	};


	enum PayReason {
		pay_begin	= 0,

		pay_vip		= 1, /*! VIP包月 */
		pay_buy		= 2, /*! 购买道具 */
        pay_charge = 3, //充值游戏金币
        pay_free = 4 , //赠送
		pay_end
	};

	enum TaskType {
		task_begin,

		task_newbie,     /*! 新手任务 */
		task_story,      /*! 主线任务 */
		task_supplement, /*! 支线任务 */
		//task_etc,        /*! 其他任务 */

		task_end
	};

    //用户退订VIP服务的渠道
    enum UnsubscribeChannel
    {
        uc_begin = 0,
        uc_duanxin = 1,    //短信
        uc_mibi = 2,        //米币
        uc_end
    };

    enum NewTransStep
    {
        nw_begin = 0,
        fGetRegSucc,
        fLoadRegSucc,
        fSendLoginReq,
        bGetLoginReq,
        bSendLoginReq,
        bGetLoginSucc,
        fGetLoginSucc,
        fLoadLoginSucc,
        fClickStartBtn,
        bGetNewroleReq,
        bSendNewroleSucc,
        fStartSrvlistReq,
        bStartGetSrvlist,
        bGetSrvlistSucc,
        fGetSrvlistSucc,
        fSendOnlineReq,
        fSend1001Req,
        bSendOnlineSucc,
        fOnlineSucc,
        fLoadInfoSucc,
        fInterGameSucc,
        nw_end
    };

public:

    /**
     * @brief StatLogger 构造函数
     *
     * @param game_id 应用ID 即游戏ID
     * @param zone_id 区ID
     * @param svr_id 服ID
     * @param site_id 平台ID 默认 -1表示该游戏不会拿出去放在不同的平台上运营  1：表示淘米平台
     * @param is_game 是否游戏后台络的数据
     */
	StatLogger(int game_id, int32_t zone_id = -1, int32_t svr_id = -1, int32_t site_id = -1, int isgame = 1);
	~StatLogger();

	StatLogger();
	void init(int game_id, int32_t zone_id = -1, int32_t svr_id = -1, int32_t site_id = -1, int isgame = 1);
	/**
	 * @brief 统计当前在线人数，每分钟调用一次
	 * @param cnt 某个区服当前的总在线人数
     * @param zone 电信和网通 默认参数表示总体在线 这里主要是针对单区服类游戏需要分电信网通来看在线
	 */
	void online_count(int cnt, std::string zone="");
    /**
     * @brief 在注册账户（米米号）时调用。
     */
	void reg_account(std::string acct_id, uint32_t cli_ip, std::string ads_id, std::string browser = "",
                     std::string device = "", std::string os = "", std::string resolution = "",
                     std::string network = "", std::string isp = "");
	// race: 职业  isp: 运营商
	/**
	 * @brief 用户在游戏中创建角色时调用
	 */
	void reg_role(std::string acct_id, std::string player_id, std::string race, uint32_t cli_ip,
				  std::string ads_id, std::string browser = "", std::string device = "", std::string os = "",
				  std::string resolution = "", std::string network = "", std::string isp = "");
     /**
	 * @brief 用户登录游戏时(验证用户名和密码)调用
	 */
	void verify_passwd(std::string acct_id, uint32_t cli_ip, std::string ads_id, std::string browser = "", std::string device = "",
				std::string os = "", std::string resolution = "", std::string network = "", std::string isp = "");
	/**
	 * @brief 用户登录游戏时(登录online)调用
	 * @param lv 取值范围1～5000
     * @param zone 为空表示总体，该字段主要时给公司老的单区服游戏使用，用于区分电信登录和网通登录
	 */
	void login_online(std::string acct_id, std::string player_id, std::string race, bool isvip, int lv,
				uint32_t cli_ip, std::string ads_id, std::string zone="", std::string browser = "", std::string device = "",
				std::string os = "", std::string resolution = "", std::string network = "", std::string isp = "");

    //b03启动设备
    void start_device(std::string device_id);

	/**
	 * @brief 用于统计用户登出人次和每次登录的游戏时长，只需在用户退出游戏的时候调用一次。
	 * @param oltime 本次登录游戏时长
	 */
	void logout(std::string acct_id, bool isvip, int lv, int oltime);

	/**
	 * @brief 玩家每次等级提升时调用
	 */
	void level_up(std::string acct_id, std::string race, int lv);

	/**
	 * @brief 玩家获得精灵时调用
	 */
    void obtain_spirit(std::string acct_id, bool isvip, int lv, std::string spirit);

	/**
	 * @brief 玩家失去精灵时调用
	 */
    void lose_spirit(std::string acct_id, bool isvip, int lv, std::string spirit);

    /**
     * @brief 商品系统统计米币购买道具时调用，游戏不需要调用
     * @param outcome 付费获得的道具名称
     */
    void pay_item(std::string acct_id, bool isvip, float pay_amount, CurrencyType currency, std::string outcome, int outcnt);

    /**
     * @brief 玩家每次在游戏内使用米币购买道具时调用
     * @param outcome 付费获得的道具名称。如果pay_reason选择的是pay_buy，则outcome赋值为相应的“道具名字”；
     *                如果pay_reason的值不是pay_buy，则本参数可以赋空字符串。
     * @param pay_type 支付类型，如米币卡、米币帐户、支付宝、苹果官方、网银、手机付费等等，米币渠道传"1"。
     */
    void pay(std::string acct_id, bool isvip, float pay_amount, CurrencyType currency, PayReason pay_reason, std::string outcome, int outcnt,
                std::string pay_channel = "_mibiaccount_");

    /**
     * @brief unsubscribe 退订VIP服务
     *
     * @param std::acct_id  用户账户
     * @param channel   退订渠道(目前只有米币和短信两个渠道)
     */
    void unsubscribe(std::string acct_id, UnsubscribeChannel uc);

    /**
     * @brief cancel_acct 销户
     *
     * @param acct_id   用户帐户
     * @param channel   销户渠道
     */
    void cancel_acct(std::string acct_id, std::string channel);

	/**
	 * @brief 玩家每次获得游戏金币时调用 也就是游戏币产出。注意：玩家通过付费充值方式获得金币时，系统会自动调用该函数。
	 * @param reason 获得的金币数量。1~1000000。 系统赠送游戏内一级货币时调用
	 */
	void obtain_golds(std::string acct_id,  int amt);
	/**
	 * @brief 玩家每次使用游戏金币时调用 也就是说游戏币消耗。
	 * @param reason 使用游戏内一级货币的原因，如获得特权、复活等等, 购买道具时无需调用。
	 * @param reason 使用的数量。1~100000。
	 */
	void use_golds(std::string acct_id, bool isvip, std::string reason, float amt, int lv);
	/**
	 * @brief 玩家每次花米币或者游戏金币购买道具时调用
	 * @param pay_type 购买道具类型(一级游戏币购买or 二级游戏币购买)
	 * @param pay_amount 付出的米币/游戏金币数量
	 * @param outcome 购买的道具名称
	 * @param outcnt 道具数量
     * 通过一级游戏币购买道具时调用
	 */
	void buy_item(std::string acct_id,  bool isvip, int lv, float pay_amount, std::string outcome, int outcnt);
	/**
	 * @brief 接受任务。任务预设新手、主线、支线和其他。其他类任务可以通过页面配置具体任务类型。
	 */
	void accept_task(TaskType type, std::string acct_id, std::string task_name, int lv);
	/**
	 * @brief 完成任务。
	 */
	void finish_task(TaskType type, std::string acct_id, std::string task_name, int lv);
	/**
	 * @brief 放弃任务。
	 */
	void abort_task(TaskType type, std::string acct_id, std::string task_name, int lv);
	/**
	 * @brief 新增注册转化
	 */
	void new_trans(NewTransStep step, std::string acct_id);
	/**
	 * @brief 自定义统计项
	 * @param stat_name 主统计名称，不能以_开头或结束，StatLogger会自动把头尾的_去掉。
	 * @param sub_stat_name 子主统计名称，一个主统计名称下可以有多个子统计项。
	 *                      不能以_开头或结束，StatLogger会自动把头尾的_去掉。
	 */
	void log(std::string stat_name, std::string sub_stat_name, std::string acct_id,
				std::string player_id, const StatInfo& info = StatInfo());

private:
    void use_golds_buyitem(std::string acct_id, bool is_vip, float amt, int lv);
	void do_obtain_golds(const std::string& acct_id, const std::string& reason, int amt);
    void do_buy_vip(const std::string& acct_id, float pay_amount, int amt);
	void do_buy_item(const std::string& acct_id,  bool isvip, int lv, float pay_amount, const std::string& pay_type, const std::string& outcome, int outcnt);
	const char* logout_time_interval(int tm) const;
	void set_basic_info(std::ostringstream& oss, const std::string& statname, const std::string& sub_statname,
						time_t ts, const std::string& acct_id, const std::string& player_id = "") const;
	void set_device_info(std::ostringstream& oss, std::string& op, const std::string& ads_id, const std::string& browser,
							const std::string& device, const std::string& os, const std::string& resolution,
							const std::string& network, const std::string& isp) const;
	void write_basic_log(const std::string& s, time_t ts);
	void write_custom_log(const std::string& s, time_t ts);

	void init_errlog(); // 错误日志模块。不用现成的DEBUG_LOG是为了避免可能存在的代码冲突。
	void errlog(const std::string& msg); // 记录日志
	void fini_errlog();
	int  calc_checksum() const;
	bool verify_checksum() const;
	bool is_valid_appid(const std::string& appid) const;
	bool is_valid_acctid(const std::string& acctid) const
	{
		return size_between(acctid, 3, sc_param_maxsz) && value_no_invalid_chars(acctid);
	}
	bool is_valid_playerid(const std::string& playerid) const
	{
		return size_between(playerid, 0, sc_param_maxsz) && value_no_invalid_chars(playerid);
	}
	bool is_valid_race(const std::string& race) const
	{
		return (size_between(race, 1, sc_param_maxsz) && value_no_invalid_chars(race) && stat_is_utf8(race));
	}
	bool is_valid_ip(const std::string& ip) const
	{
		return size_between(ip, 7, sc_param_maxsz) && value_no_invalid_chars(ip);
	}
	bool is_valid_lv(int lv) const
	{
		return ((lv >= 0) && (lv <= 5000));
	}
	// 判断在线时长合法性
	bool is_valid_oltm(int oltm) const
	{
		return ((oltm >=0) && (oltm <= 864000));
	}
	bool is_valid_currency(CurrencyType ccy) const
	{
		return ((ccy > ccy_begin) && (ccy < ccy_end));
	}
	bool is_valid_payreason(PayReason r) const
	{
		return ((r > pay_begin) && (r < pay_end));
	}
	bool is_valid_tasktype(TaskType type) const
	{
		return ((type > task_begin) && (type < task_end));
	}
	bool is_valid_newtransstep(NewTransStep step) const
	{
		return ((step > nw_begin) && (step < nw_end));
	}
	bool is_valid_common_parm(const std::string& parm, std::string::size_type min_sz = 1,
								std::string::size_type max_sz = sc_param_maxsz) const
	{
		return (size_between(parm, min_sz, max_sz) && value_no_invalid_chars(parm));
	}
	bool is_valid_common_utf8_parm(const std::string& parm, std::string::size_type min_sz = 1,
									std::string::size_type max_sz = sc_param_maxsz) const
	{
		return (size_between(parm, min_sz, max_sz) && value_no_invalid_chars(parm) && stat_is_utf8(parm));
	}
	const char* get_task_stid(TaskType type, int stage) const
	{
		// stage: 0接受任务、1完成任务、2放弃任务
		// 不用static是为了防止被越界写
		const char* stids[][3] = {
			{ "", "", "" },
			{ "_getnbtsk_", "_donenbtsk_", "_abrtnbtsk_" },
			{ "_getmaintsk_", "_donemaintsk_", "_abrtmaintsk_" },
			{ "_getauxtsk_", "_doneauxtsk_", "_abrtauxtsk_" },
			{ "_getetctsk_", "_doneetctsk_", "_abrtetctsk_" }
		};
		return stids[type][stage];
	}
    const char* get_new_trans_step(NewTransStep step) const
    {
        const char* new_trans_step_sstid[] = {
            "",
            "fGetRegSucc",
            "fLoadRegSucc",
            "fSendLoginReq",
            "bGetLoginReq",
            "bSendLoginReq",
            "bGetLoginSucc",
            "fGetLoginSucc",
            "fLoadLoginSucc",
            "fClickStartBtn",
            "bGetNewroleReq",
            "bSendNewroleSucc",
            "fStartSrvlistReq",
            "bStartGetSrvlist",
            "bGetSrvlistSucc",
            "fGetSrvlistSucc",
            "fSendOnlineReq",
            "fSend1001Req",
            "bSendOnlineSucc",
            "fOnlineSucc",
            "fLoadInfoSucc",
            "fInterGameSucc",
        };
        return new_trans_step_sstid[step];
    }

	/**
	 * @brief 可忽略
	 * 用于统计用户每天的总在线时长，可以在用户退出游戏的时候统一调用一次，也可以定期调用（如每隔5分钟调用一次）。
	 *        由于出现程序崩溃重启的情况会无法调用该函数统计用户在线时长，故建议定期调用，以保证数据准确性。
	 *        注意：统计平台对每次调用该函数时传递的oltime进行累加，故每次调用该函数后，游戏后台需要重新计算在线时间！
	 */
	void accumulate_online_time(std::string acct_id, std::string race, int oltime);

private:
	static const uint32_t sc_appid_minsz = 6;
	static const uint32_t sc_appid_sz    = 9;
	static const uint32_t sc_ip_minsz    = 6;
	static const uint32_t sc_ip_sz       = 64;
	static const uint32_t sc_path_sz     = 256;
	static const uint32_t sc_param_maxsz = 128;

private:
	int m_chksum1;        // 校验和，和m_chksum2的值一样，用于判断内存是否被越界写乱

	// 为了保证类内成员遭到越界写时可以被检测出来，所有的成员变量都必须使用基本类型。
	// 如果使用了复合类型，而复合类型有动态分配的内存，则无法检测到动态分配的内存是否被越界写。

	// 注意：不要在m_siteid前加任何变量，也不要移动m_siteid的位置
	int32_t m_siteid;     // 应用/游戏的运营平台ID，由游戏自定义，在统计平台可以设定ID对应的名称
	//char m_appid[sc_appid_sz];  // 统计平台为每个接入统计的应用/游戏而生成的唯一ID
    	int m_appid;
	int m_isgame;
	int m_magic_num1;     // 写死的神奇数字，用于判断内存是否被越界写乱
	int32_t m_zoneid;     // 应用/游戏的大区ID，由游戏自定义，在统计平台可以设定ID对应的名称
	int32_t m_svrid;      // 应用/游戏的服务器ID，由游戏自定义，在统计平台可以设定ID对应的名称
	char m_hostip[sc_ip_sz]; // 本机IP
	int m_magic_num2;     // 写死的神奇数字，用于判断内存是否被越界写乱
	char m_path[sc_path_sz]; // 统计相关的主目录，该目录下还有inbox、outbox、sent、log等子目录
	int m_basic_fd;       // 用于写入基础统计项的fd，5秒钟切换一个文件
	int m_custom_fd;      // 用于写入自定义统计项的fd，5分钟切换一个文件
	int m_magic_num3;     // 写死的神奇数字，用于判断内存是否被越界写乱
	time_t m_basic_ts;    // 记录基础统计项文件的创建时间，用于判断5秒钟切换一个文件
	time_t m_custom_ts;   // 记录自定义统计项文件的创建时间，用于判断5分钟切换一个文件
	std::ofstream m_of;   // 记录异常错误日志。因为这个日志和统计数据无关，就算出错也无所谓，所以可以用复合类型
	int m_inited;  //是否初始化


	int m_chksum2;       // 校验和，和m_chksum1的值一样，用于判断内存是否被越界写乱
	// 注意：chksum2必须是最后一个成员变量，绝对不能在m_chksum2后面新增变量
    //

private:
	static const int sc_magic1 = 0x01234567; // 这三个常量一开始就赋值给上面的3个magic_num
	static const int sc_magic2 = 0x89ABCDEF;
	static const int sc_magic3 = 0x13579BDF;

};

#endif // STAT_STATLOGGER_HPP_INCLUDED
