#ifndef __BUFF_H__
#define __BUFF_H__

#include "common.h"
#include "buff_conf.h"

class buff_handler_t {
public:
    buff_handler_t() {
        effect_func_map_.clear();
    }
    ~buff_handler_t() {
        effect_func_map_.clear();
    }
    void init();
    int do_buff(player_t *player, buff_effect_type_t effect_type, std::vector<uint32_t> &proc_val);

public:
    //处理函数
    static int effect_fun_exp_multi(player_t *player, const buff_effect_t &effect, std::vector<uint32_t> &proc_val);
    static int effect_fun_gold_multi(player_t *player, const buff_effect_t &effect, std::vector<uint32_t> &proc_val);
private:
    //处理函数
    typedef int (*effect_proc_func)(player_t *player, const buff_effect_t &effect, std::vector<uint32_t> &proc_val);
    std::map<uint32_t, effect_proc_func> effect_func_map_;
};

#endif
