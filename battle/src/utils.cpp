
#include "utils.h"

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
