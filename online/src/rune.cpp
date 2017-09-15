#include "rune.h"
#include "attr_utils.h"
#include "player.h"

const uint32_t kMaxRuneCallPack  = 20;
const uint32_t kMaxRuneLv = 6;
const uint32_t kMaxRuneTransferPack  = 20;
//const uint32_t KMaxCallLevel = AttrUtils::get_attr_max_limit(NULL, kAttrRuneCallLevel);
const uint32_t KMaxCallLevel = 3; //kAttrRuneCallLevel在 属性表中的max值
const uint32_t KGrayRunePrice = 200;
const uint32_t OPEN_RUNE_POS_PRICE = 100;
const uint32_t RUNE_FRAGMENT_CONF_ID = 100;

bool RuneMeseum::has_rune(const uint32_t rune_id) {
	RuneMap::iterator it = m_rune_map.find(rune_id);
	if (it == m_rune_map.end()) {
		return false;
	}
	return true;
}

void RuneMeseum::get_rune_map_info(RuneMap& rune_map) {
	rune_map = m_rune_map;
}

//从内存中读出rune_id符文的信息
uint32_t RuneMeseum::get_rune(const uint32_t rune_id, rune_t& rune) {
	RuneMap::iterator it = m_rune_map.find(rune_id);
	if (it == m_rune_map.end()) {
		return cli_err_rune_not_exit;
	}
	rune = it->second;
	return 0;
}

//向内存更新rune_id符文的信息
uint32_t RuneMeseum::update_rune(const rune_t& rune) {
	if (!has_rune(rune.id)) {
		return cli_err_rune_not_exit;
	}
	m_rune_map[rune.id] = rune;
	return 0;
}

//在内存中删除rune_id符文
uint32_t RuneMeseum::del_rune(const uint32_t rune_id) {
	RuneMap::iterator it = m_rune_map.find(rune_id);
	if (it == m_rune_map.end()) {
		return cli_err_rune_not_exit;
	}
	m_rune_map.erase(it);
	return 0;
}

int RuneMeseum::get_rune_num_by_packet_type(rune_pack_type_t packet_type) {
	int count = 0;
	std::map<uint32_t, rune_t>::iterator it = m_rune_map.begin();
	for (; it != m_rune_map.end(); ++it) {
		if (it->second.pack_type == static_cast<uint32_t>(packet_type)) {
			++count;
		}
	}
	return count;
}


uint32_t RuneMeseum::save_rune(rune_t& rune)
{
	if (has_rune(rune.id)) {
		return cli_err_rune_id_conflict;
	}
	m_rune_map[rune.id] = rune;
	return 0;
}

/*@brief 添加召唤等级
 */
uint32_t RuneMeseum::add_call_level(uint32_t level)
{
	if (has_level_in_call_level_vec(level)) {
		return cli_err_rune_target_level_has_exist;
	}
	m_call_level.push_back(level);
	std::sort(m_call_level.begin(), m_call_level.end());
	return 0;
}

int RuneMeseum::erase_cur_call_level(uint32_t level)
{
	if (m_call_level.size() > 1 && level != 0 && m_call_level[0] == 0) {
		std::vector<uint32_t>::iterator it = std::find(m_call_level.begin(), m_call_level.end(), level);
		if (it != m_call_level.end()) {
			m_call_level.erase(it);
			std::sort(m_call_level.begin(), m_call_level.end());
			return 0;
		}
	}
	return -1;
}

/*@brief 检查当前等级在召唤馆中是否拥有:有：true; 无：false
 */
bool RuneMeseum::has_level_in_call_level_vec(uint32_t level)
{
	std::vector<uint32_t>::iterator it = std::find(m_call_level.begin(), m_call_level.end(), level);
	if (it == m_call_level.end()) {
		return false;
	}
	return true;
}

/*@brief 获得召唤馆激活的等级（已废弃）
 */
void RuneMeseum::get_call_level_list(std::vector<uint32_t>& level_vec)
{
	level_vec = m_call_level;
}

/*@brief 获得召唤馆激活的等级(新)
 */
void RuneMeseum::get_call_level_list_from_set(std::set<uint32_t>& level_set) {
	level_set = m_call_level_set;
}

void RuneMeseum::reset_call_level_list() {
	m_call_level.resize(1, 0);
	if (m_call_level.size()) {
		if (m_call_level[0] != 0) {
			m_call_level[0] =0;
		}
	}
}

void RuneMeseum::set_call_level_1st() {
	if (m_call_level.size()) {
		if (m_call_level[0] != 0) {
			m_call_level[0] = 0;
		}
	}
}

uint32_t RuneMeseum::add_to_call_level_set(uint32_t level) {
	if (level > KMaxCallLevel) {
		return cli_err_can_not_activate_this_level;
	}
	if (m_call_level_set.count(level) == 0) {
		m_call_level_set.insert(level);
	}
	return 0;
}

uint32_t RuneMeseum::erase_from_call_level_set(uint32_t level) {
	if (m_call_level_set.count(level) == 0) {
		return cli_err_will_erase_level_not_exist;
	}
	m_call_level_set.erase(level);
	return 0;
}

void RuneMeseum::reset_call_level_set() {
	std::set<uint32_t>::iterator it = m_call_level_set.find(0);
	if (it == m_call_level_set.end()) {
		m_call_level_set.insert(0);
	}
}

uint32_t RuneMeseum::save_call_level_set(player_t* player) {
	reset_call_level_set();
	//const size_t BITLEN = KMaxCallLevel+1;
	//长度是属性表中KMaxCallLevel 的 max值 + 1
	std::bitset<4> b;
	b.reset();
	FOREACH(m_call_level_set, it) {
		if (*it > KMaxCallLevel) {
			return cli_err_data_error;
		}
		if (!b.test(*it)) {
			b.set(*it);
		}
	}
	uint32_t save_value = b.to_ulong();
	SET_A(kAttrCallLevelInfo, save_value);
	return 0;
}

uint32_t RuneMeseum::get_call_level_set_from_DB(player_t* player) {
	clear_call_level_set();
	uint32_t value = GET_A(kAttrCallLevelInfo);
	std::bitset<4> b((unsigned long)value);
	for (uint32_t i = 0; i < b.size(); ++i) {
		if (b.test(i)) {
			m_call_level_set.insert(i);
		}
	}
	return 0;
}

void RuneMeseum::clear_call_level_set() {
	m_call_level_set.clear();
}
