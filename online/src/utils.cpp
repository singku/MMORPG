
#include "utils.h"
#include "sys_ctrl.h"
#include "global_data.h"
#include "player.h"
#include "player_manager.h"
#include "service.h"
extern "C" {
#include <libtaomee/tm_dirty/utf8_punc.h>
}

static char hex_buf[65536];

std::string Utils::bin2hex(const std::string& bin) 
{
    char* buf;
    if (bin.size() * 2 + 1 > sizeof(hex_buf)) {
        buf = (char *)malloc(bin.size() * 2 + 1); 
        if (buf == NULL) {
            return ""; 
        }
    } else {
        buf = hex_buf;
    }

    bin2hex_frm(buf, (char *)bin.c_str(), bin.size(), false);

    std::string str = std::string(buf);

    if (buf != hex_buf) {
        free(buf); 
    }

    return str;
}

std::string Utils::to_string(uint32_t n)
{
    snprintf(hex_buf, sizeof(hex_buf), "%u", n);

    return std::string(hex_buf);
}

std::string Utils::to_string(uint64_t n)
{
    snprintf(hex_buf, sizeof(hex_buf), "%lu", n);

    return std::string(hex_buf);
}

std::string Utils::to_string(int n)
{
    snprintf(hex_buf, sizeof(hex_buf), "%d", n);

    return std::string(hex_buf);
}

bool Utils::find_first_zero(uint32_t var, uint32_t &idx)
{
    if (var == 0) {
        idx = 0;
        return true;
    }
    if (var == 0xFFFFFFFF) {
        idx = 0;
        return false;
    }
    uint32_t tmp = var ^ (var + 1); 
    std::bitset<32> tmp_bset = std::bitset<32>(tmp);
    idx = tmp_bset.count() - 1;
    return true;
}

bool Utils::find_first_one(uint32_t var, uint32_t &idx)
{
    if (var == 0) {
        idx = 0;
        return false;
    }
    if (var == 0xFFFFFFFF) {
        idx = 0;
        return true;
    }
    uint32_t tmp = var ^ (var - 1);
    std::bitset<32> tmp_bset = std::bitset<32>(tmp);
    idx = tmp_bset.count() - 1;
    return true;
}

uint32_t Utils::select_from_rate_list(std::vector<uint32_t>& list)
{
    uint32_t rate = 0;

    uint32_t size = list.size();
    
    for (uint32_t i = 0; i < size; ++i) {
        rate += list[i];
    }

	if(rate < 1){
		ERROR_TLOG("is_valid rate_list, rate =%u",
				rate); 
		return size;
	}

    uint32_t select = taomee::ranged_random(1, rate);
    
    for (uint32_t i = 0; i < size; ++i) {
        if (select <= list[i]) {
            return i;
        } else {
            select = select - list[i];
        }
    }
    return size;
}

void Utils::select_n_from_m(uint32_t begin, uint32_t end, uint32_t n, std::vector<uint32_t>& list)
{
    if (begin + n >= end + 1) {
        return;
    }

    if (n == 0) {
        return ;
    }

    uint32_t remain = end - begin + 1;
    uint32_t total = remain;
    uint32_t sel = n;
    for (uint32_t i = 0; i < total; ++i) {
        uint32_t rand = taomee::ranged_random(1, remain);
        if (rand <= sel) {
            list.push_back(begin + i);
            --remain;
            --sel;
        } else {
            --remain;
        }
        if (sel == 0) {
            break;
        }
    }
}

bool Utils::is_activity_open(uint32_t type, time_t time)
{
    std::string val;
    val.clear();
    if(g_module_mgr.get_module_conf_string((uint32_t)type, "open", val)) {
        if (val != "1") {
            return false;
        }
    }

    val.clear();
    if(!g_module_mgr.get_module_conf_string((uint32_t)type, "start_time", val)) {
        return false;
    } else {
        uint32_t start = 0;
        if (TimeUtils::time_str_to_long(val.c_str(), start)) {
            return false;
        }

        if (time < start) {
            return false;
        }
    }

    val.clear();
    if(!g_module_mgr.get_module_conf_string((uint32_t)type, "end_time", val)) {
        return false;
    } else {
        uint32_t end = 0;
        if (TimeUtils::time_str_to_long(val.c_str(), end)) {
            return false;
        }

        if (time > end) {
            return false;
        }
    }

    return true;
}

std::string Utils::get_rand_name(uint32_t type)
{
    std::string name;
    std::map<uint32, rand_name_pool_t>::iterator iter = g_rand_name_pool.find(type);
    if (iter == g_rand_name_pool.end()) return name;

    for (uint32_t i = 0; i < commonproto::MAX_RAND_NAME_TAG_POS_TYPE;i++) {
        uint32_t size = iter->second.name_pool[i].size();
        if (size > 0) {
            uint32_t idx = rand()%size;
            name = name + iter->second.name_pool[i][idx];
        }
    }
    return  name;
}

/** 
 * @brief 賍词检查
 * 
 * @param name 被检查字串
 * @param flag true 检查标点, false 不检查标点
 * 
 * @return 
 */
int Utils::check_dirty_name(const std::string &name, bool flag)
{
    int err = 0;
    char nick_buf[64] = {0};
    char new_nick_buf[64] = {0};
    snprintf(nick_buf, sizeof(nick_buf) - 1, "%s", name.c_str());
    int len = strlen(nick_buf);
    int dlen = len;

    if (flag) {
        string_filter((unsigned char*)new_nick_buf, (unsigned char *)nick_buf, &dlen, 1);
        if (dlen != len) {//有全角/标点/符号
            return cli_err_name_invalid_char;
        }
    }

    string nick(nick_buf);
    if (nick == "" || nick.size() == 0) {
        return cli_err_name_empty;
    }
    int ret = tm_dirty_check(0, (char*)nick.c_str());
    if (ret) {
        return cli_err_name_dirty;
    }

    return err;
}

int Utils::switch_transmit_msg(
		switchproto::switch_transmit_type_t tran_type,
		uint16_t cmd, const google::protobuf::Message& message,
		const std::vector<uint32_t> *recv_id,
		const std::vector<uint32_t> *svr_id)
{
	switchproto::cs_sw_transmit_only sw_in_;
    if (recv_id) {
        FOREACH((*recv_id), it1) {
            switchproto::sw_player_basic_info_t* pb_ptr = sw_in_.add_receivers();
            pb_ptr->set_userid(*it1);
            pb_ptr->set_create_tm(0);
        }
    }

    if (svr_id) {
        FOREACH((*svr_id), it2) {
            commonproto::svr_info_t* pb_ptr = sw_in_.add_servers();
            pb_ptr->set_svr_id(*it2);
            pb_ptr->set_type(commonproto::SERVER_TYPE_ALL);
            pb_ptr->set_ip(0);
            pb_ptr->set_port(0);
        }
    }

	sw_in_.set_transmit_type(tran_type);
	sw_in_.set_cmd(cmd);
	std::string pkg;
	message.SerializeToString(&pkg);
	sw_in_.set_pkg(pkg);
	int ret = g_switch->send_msg(0, g_online_id, 0, sw_cmd_sw_transmit_only, sw_in_);
	if (ret) {
		return ret;
	}
	return 0;
}

void Utils::write_msglog_new(userid_t userid, const std::string& dir, const std::string& name, const std::string& subname)
{
    //NOTI(singku)如果用户用的是支持性不好的老机器 则不做自定义统计
    if (g_player_manager->is_player_has_old_machine(userid)) {
        return;
    }

    StatInfo stat;
    stat.add_info("item", Utils::clean_msgname(subname));
    stat.add_op(StatInfo::op_item, "item");
    g_stat_logger->log(Utils::clean_msgname(dir), 
            Utils::clean_msgname(name), Utils::to_string(userid), "", stat); 
}

void Utils::write_msglog_count(userid_t userid, const std::string& dir, const std::string& name, const std::string& subname, uint32_t count)
{
    //NOTI(singku)如果用户用的是支持性不好的老机器 则不做自定义统计
    if (g_player_manager->is_player_has_old_machine(userid)) {
        return;
    }

    StatInfo stat;
    stat.add_info(Utils::clean_msgname(subname), count);
    stat.add_op(StatInfo::op_sum, Utils::clean_msgname(subname));
    g_stat_logger->log(Utils::clean_msgname(dir), 
            Utils::clean_msgname(name), Utils::to_string(userid), "", stat); 
}

