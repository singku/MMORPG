#ifndef RUNE_H
#define RUNE_H

#include <map>
#include <stdint.h>
#include <common.h>

//临时背包大小
extern const uint32_t kMaxRuneCallPack;
//符文最高等级
extern const uint32_t kMaxRuneLv;
//转化背包大小
extern const uint32_t kMaxRuneTransferPack;
//当前最大召唤阵等级(共四级)
extern const uint32_t KMaxCallLevel;
//符文碎片的价格
extern const uint32_t KGrayRunePrice;
extern const uint32_t OPEN_RUNE_POS_PRICE;
//碎片的conf_id
extern const uint32_t RUNE_FRAGMENT_CONF_ID;

//符文类型
enum rune_attr_type_t
{
	kRuneNotDef = 0,	//未定义
    kRuneGreen = 1, //符文绿色
    kRuneBlue = 2, //符文蓝色 
    kRunePurple = 3, //符文紫色 
	kRuneOrange = 4, //符文橙色
    kRuneGray = 10, //碎片
    kRuneRed = 11, //符文能量 
};

//符文背包类型
enum rune_pack_type_t
{
   kRunePack = 0, //临时符文背包(符文馆)
   kTransferPack = 1, //转化背包
   kCollectPack = 2, //收藏背包
   kPetPack = 3,  //精灵身上
};

enum rune_pet_pos
{
	RUNE_PET_POS1 = 1,	//精灵身上第一个位置
	RUNE_PET_POS2 = 2,	//第二个位置
	RUNE_PET_POS3 = 3,  //第三个位置
	RUNE_PET_POS4 = 4,  //第四个位置
};

enum rune_equip_need_pet_lv
{
	RUNE_EQUIP_POS1_NEED_LV = commonproto::RUNE_EQUIP_POS_1_NEED_LV,	//第一个格子需要的等级
	RUNE_EQUIP_POS2_NEED_LV = commonproto::RUNE_EQUIP_POS_2_NEED_LV,	//第二个格子需要的等级
	RUNE_EQUIP_POS3_NEED_LV = commonproto::RUNE_EQUIP_POS_3_NEED_LV,	//第三个格子需要的等级
	RUNE_EQUIP_POS4_NEED_LV = commonproto::RUNE_EQUIP_POS_4_NEED_LV,	//第四个格子需要的等级
};

enum get_rune_channel_t
{
	GET_RUNE_IN_CALL_LEVEL = 0,	//时空门召唤获得
	GET_RUNE_SYSTEM_SEND = 1,	//系统赠送
};

struct rune_conf_t {
	uint32_t rune_id;	//配表中的id(与 rune_t结构体中的id不同) ps: 后期维护最好改成 conf_id
	uint32_t rune_type; //符文品级1.绿色； 2.蓝色； 3.紫色；4.橙色；10.灰色；11.能量
	uint32_t fun_type;	//0-11类；(功能类)
	uint32_t fun_value;	//属性值；默认为符文第一级的值
	uint32_t arg_rate;	//等级对应的属性值增加
	uint32_t tran_add_exp;	//符文初始携带的经验值（专用于吞噬或升级）
};

struct battle_value_t {
    uint32_t type; //战斗值种类
    uint32_t value; //战斗值
};

class rune_conf_mgr_t {
public:
	typedef std::map<uint32_t/*conf_id*/, rune_conf_t> RuneConfMgr;
	rune_conf_mgr_t() {
		clear();
	}
	~rune_conf_mgr_t() {
		clear();
	}
	inline void clear() {
		rune_conf_rune_.clear();
	}
	inline const RuneConfMgr& const_rune_conf_map() const {
		return rune_conf_rune_;
	}
	inline void copy_from(const rune_conf_mgr_t &m) {
		rune_conf_rune_ = m.const_rune_conf_map();
	}
	inline bool is_rune_conf_exist(uint32_t conf_id) {
		return rune_conf_rune_.count(conf_id) > 0 ? true : false;
	}
	rune_conf_t* get_rune_conf_t_info(uint32_t conf_id) {
		RuneConfMgr::iterator it = rune_conf_rune_.find(conf_id);
		if (it != rune_conf_rune_.end()) {
			return &it->second;
		}
		return NULL;
	}
	inline bool add_rune_conf(const rune_conf_t &rune) {
		if (is_rune_conf_exist(rune.rune_id/*conf_id*/)) {
			return false;
		}
		rune_conf_rune_.insert(RuneConfMgr::value_type(rune.rune_id/*conf_id*/, rune));
		return true;
	}
	inline int get_conf_id(uint32_t rune_type, uint32_t fun_type, uint32_t& conf_id) {
		uint32_t flag = 0;
		FOREACH(rune_conf_rune_, it) {
			if (it->second.rune_type == rune_type && it->second.fun_type == fun_type) {
				conf_id = it->second.rune_id;
				flag = 1;
				break;
			}
		}
		return flag;
	}
	inline void print_rune_info() {
		//RuneConfMgr::iterator it = rune_conf_rune_.begin();
		FOREACH(rune_conf_rune_, it) {
			uint32_t id = it->second.rune_id;
			uint32_t rune_type = it->second.rune_type;
			uint32_t fun_type = it->second.fun_type;
			uint32_t fun_value = it->second.fun_value;
			uint32_t arg_rate = it->second.arg_rate;
			uint32_t tran_add_exp = it->second.tran_add_exp;
			TRACE_TLOG("load config rune:[%u],[%u],[%u],[%u],[%u],[%u]", 
					id, rune_type, fun_type, fun_value, arg_rate, tran_add_exp);
		}
		
	}

private:
	RuneConfMgr rune_conf_rune_;
	//std::map<uint32_t/*rune id*/, rune_conf_t> rune_conf_rune_;
};

struct rune_exp_conf_t {
	uint32_t rune_type;
	std::vector<uint32_t> exp_vec;
};

class rune_exp_conf_mgr_t {
public:
	typedef std::map<uint32_t/*rune_type*/, rune_exp_conf_t> RuneExpConMgr;
	rune_exp_conf_mgr_t() {
		clear();
	}
	~rune_exp_conf_mgr_t() {
		clear();
	}
	inline void clear() {
		rune_exp_conf_rune_.clear();
	}
	inline const RuneExpConMgr& const_rune_exp_conf_rune() const {
		return rune_exp_conf_rune_;
	}
	inline void copy_from(const rune_exp_conf_mgr_t &m) {
		rune_exp_conf_rune_ = m.const_rune_exp_conf_rune();
	}
	inline bool is_rune_exp_conf_exist(uint32_t rune_type) {
		return rune_exp_conf_rune_.count(rune_type) > 0 ? true : false;
	};
	inline bool add_rune_exp_conf(const rune_exp_conf_t& rune_exp_conf) {
		if (is_rune_exp_conf_exist(rune_exp_conf.rune_type)) {
			return false;
		}
		rune_exp_conf_rune_.insert(RuneExpConMgr::value_type(rune_exp_conf.rune_type, rune_exp_conf));
		return true;
	}

	rune_exp_conf_t* get_rune_exp_conf_t_info(const uint32_t rune_type) {
		RuneExpConMgr::iterator it = rune_exp_conf_rune_.find(rune_type);
		if (it != rune_exp_conf_rune_.end()) {
			return &it->second;
		}
		return NULL;
	}

	/* @brief 获得符文等级对应的经验值
		rune_type :符文品级
		level 符文目标等级
	 */
	uint32_t get_exp_by_level_from_rune_exp_conf(
			uint32_t rune_type, uint32_t level, uint32_t& exp) {
		rune_exp_conf_t* rune_exp_ptr = get_rune_exp_conf_t_info(rune_type);
		if (rune_exp_ptr == NULL) {
			ERROR_TLOG("rune_type err:[%u]", rune_type);
			return cli_err_rune_type_invaild;
		}
		if (!rune_exp_ptr->exp_vec.empty()) {
			uint32_t len = rune_exp_ptr->exp_vec.size();
			//level最高值是6；len 最高值是5
			if (level < 2 || level - 2 >= len) {
				ERROR_TLOG("rune_level err:[%u],[%u]", level, len);
				return cli_err_rune_level_value_err;
			}
			exp = rune_exp_ptr->exp_vec[level - 2];
		}
		return 0;
	}

	inline void print_rune_exp_info() {
		FOREACH(rune_exp_conf_rune_, it) {
			uint32_t rune_type = it->second.rune_type;
			std::vector<uint32_t> tmp_vec(it->second.exp_vec);
			FOREACH(tmp_vec, tmp_iter) {
				uint32_t tmp_value = *tmp_iter;
				TRACE_TLOG("load_config_rune_exp:[%u],[%u]", rune_type, tmp_value);
			}
		}
	}
private:
	RuneExpConMgr rune_exp_conf_rune_;
};

struct rune_rate_conf_t {
	uint32_t level;			//产出该符文所在的召唤馆等级
	uint32_t conf_id;		//对应rune.xml中符文id
	uint32_t rate;          //该类型该属性的符文被召唤到的概率
};

struct call_level_info {
	uint32_t level;
	uint32_t coin;          //召唤该类型属性的符文需要消耗的金币
	uint32_t callrate;      //开启下一阶的概率
};

class rune_rate_conf_mgr_t {
public:
	typedef std::map<uint32_t/*召唤阵等级*/, std::vector<rune_rate_conf_t> > RuneRateConMgr;
	typedef std::map<uint32_t/*召唤阵等级*/, call_level_info> CallLevelMgr;
	rune_rate_conf_mgr_t() {
		clear();
	}
	~rune_rate_conf_mgr_t() {
		clear();
	}
	inline void clear() {
		rune_rate_conf_.clear();
        call_level_conf_.clear();
	}
	inline const RuneRateConMgr& const_rune_rate_conf_rune() const {
		return rune_rate_conf_;
	}
	inline const CallLevelMgr& const_call_level_conf_rune() const {
		return call_level_conf_;
	}
	inline void copy_from(const rune_rate_conf_mgr_t &m) {
		rune_rate_conf_ = m.const_rune_rate_conf_rune();
		call_level_conf_ = m.const_call_level_conf_rune();
	}
	inline bool is_rune_rate_conf_exist(uint32_t level) {
		return rune_rate_conf_.count(level) > 0 ? true : false;
	}
	inline bool is_call_level_conf_exist(uint32_t level) {
		return call_level_conf_.count(level) > 0 ? true : false;
	}

	inline uint32_t get_call_info_by_level(
			uint32_t level, uint32_t& callrate, 
			uint32_t& coin) {
		CallLevelMgr::iterator it = call_level_conf_.find(level);
		if (it == call_level_conf_.end()) {
			return cli_err_call_level_err;
		}
		coin = it->second.coin;
		callrate = it->second.callrate;
		return 0;
	}

	inline bool add_rune_rate_conf(
			const std::vector<rune_rate_conf_t>& tem, 
			const uint32_t level) {
		if (is_rune_rate_conf_exist(level)) {
			return false;
		}
		rune_rate_conf_.insert(RuneRateConMgr::value_type(level, tem));
		return true;
	}

	inline bool add_call_level_conf(
			const call_level_info& call_level, 
			const uint32_t level) {
		if (is_call_level_conf_exist(level)) {
			return false;
		}
		call_level_conf_.insert(CallLevelMgr::value_type(level, call_level));
		return true;
	}

	inline uint32_t get_rune_rate_conf_info(
			uint32_t level, std::vector<rune_rate_conf_t>& vec_item) {
		RuneRateConMgr::iterator it = rune_rate_conf_.find(level);
		if (it == rune_rate_conf_.end()) {
			return cli_err_call_level_err;
		}
		vec_item = it->second;
		return 0;
	}
	void print_rune_rate_info() {
		FOREACH(rune_rate_conf_, it) {
			uint32_t level = it->first;
			FOREACH(it->second, it2) {
				uint32_t conf_id = it2->conf_id;
				uint32_t rate = it2->rate;
				TRACE_TLOG("rune_rate_conf:conf_id=[%u],rate=[%u],level=[%u]", 
						conf_id, rate, level);
			}
		}
		FOREACH(call_level_conf_, it) {
			uint32_t level = it->first;
			uint32_t coin = it->second.coin;
			uint32_t callrate = it->second.callrate;
			TRACE_TLOG("rune_rate_conf:level=[%u],coin=[%u],callrate=[%u]", level, coin, callrate);
		}
	}
private:
	RuneRateConMgr rune_rate_conf_;
	CallLevelMgr call_level_conf_;
};

struct rune_t {
	uint32_t id;		//前端传来的id(即使conf_id相同，此id也是唯一)
	uint32_t conf_id;	//配置表中的rune_id
	uint32_t exp;		//符文当前经验值
	uint32_t level;		//符文等级(初始等级为1，之后靠减少经验瓶中的经验来升级)
	uint32_t pack_type;		//符文所在的背包类型
	uint32_t pet_catch_time;	//符文装配在的精灵的id
	uint32_t grid_id;	//若果装在精灵身上表示符文所在精灵符文的格子id(1-6) 否则为0
};

struct rune_cli_t {
	struct rune_t rune;
	uint32_t flag;
};

class RuneMeseum {
public:
	RuneMeseum(player_t* player) {
		m_rune_map.clear();
		m_call_level.clear();
		m_call_level_set.clear();
		//get_call_level_set_from_DB(player);
	}
	~RuneMeseum() {}
	typedef std::map<uint32_t/*rune_id*/, rune_t> RuneMap;
	/*@brief 判断符文是否存在
	 */
	//int is_rune_id_exsit(uint32_t rune_id);
	/*@brief 获得符文信息
	 */
	bool has_rune(const uint32_t rune_id);
	uint32_t get_rune(const uint32_t rune_id, rune_t& rune);
	uint32_t save_rune(rune_t& rune);
	uint32_t update_rune(const rune_t& rune);
	uint32_t del_rune(const uint32_t rune_id);

	void get_rune_map_info(RuneMap& rune_map);
	int get_rune_num_by_packet_type(rune_pack_type_t packet_type);

	/*@brief 将召唤等级为level的下一级激活（废弃）
	 */
	uint32_t add_call_level(uint32_t level);
	/*@brief 一次召唤结束，删除当前召唤等级(废弃)
	 */
	int erase_cur_call_level(uint32_t level);
	/*@brief 检查当前等级在召唤馆中是否拥有(废弃)
	 */
	bool has_level_in_call_level_vec(uint32_t level);
	/*@brief 获得召唤馆激活的等级(废弃)
	 */
	void get_call_level_list(std::vector<uint32_t>& level_vec);
	/*@brief 获得召唤馆激活的等级(新)
	*/
	void get_call_level_list_from_set(std::set<uint32_t>& level_set);

	/*@brief 处理脏数据：若召唤馆中等级超过4，则恢复至0
	 */
	void reset_call_level_list();
	void set_call_level_1st();

	/*@brief 将召唤等级为level的下一级激活(新)
	 */
	uint32_t add_to_call_level_set(uint32_t level);

	/*@brief 默认第0级始终要开启
	 */
	void reset_call_level_set();

	/*@brief 删除level召唤等级(新)
	 */
	uint32_t erase_from_call_level_set(uint32_t level);

	/*@brief 持久保存call level set
	 */
	uint32_t save_call_level_set(player_t* player); 

	/*@brief 从DB中获得召唤阵
	 */
	uint32_t get_call_level_set_from_DB(player_t* player);

	/*@brief 清除内存中召唤阵信息
	 */
	void clear_call_level_set();
	

private:
	RuneMap  m_rune_map;
	std::vector<uint32_t> m_call_level;//已废弃
	std::set<uint32_t> m_call_level_set;
};

#endif
