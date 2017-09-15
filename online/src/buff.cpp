#include "global_data.h"
#include "player.h"
#include "buff.h"
#include "buff_conf.h"

void buff_handler_t::init()
{
    effect_func_map_.clear();
    effect_func_map_[(uint32_t)server_buff_effect_exp_multi] = effect_fun_exp_multi;
    effect_func_map_[(uint32_t)server_buff_effect_gold_multi] = effect_fun_gold_multi;
}

int buff_handler_t::effect_fun_exp_multi(player_t *player, const buff_effect_t &effect, std::vector<uint32_t> &proc_val)
{
    if (proc_val.size() == 0) {
        return 0;
    }
    uint32_t exp = proc_val[0];
    uint32_t rate = effect.args[0];

    exp = ((100 + rate) / 100.0 * exp);
    proc_val[0] = exp;
    return 0;
}

int buff_handler_t::effect_fun_gold_multi(player_t *player, const buff_effect_t &effect, std::vector<uint32_t> &proc_val)
{
    if (proc_val.size() == 0) {
        return 0;
    }

    uint32_t gold = proc_val[0];
    uint32_t rate = effect.args[0];

    gold = ((100 + rate) / 100.0 * gold);
    proc_val[0] = gold;
    return 0;
}

int buff_handler_t::do_buff(player_t *player, buff_effect_type_t effect_type, std::vector<uint32_t> &proc_val)
{
    FOREACH((*(player->buff_id_map)), it) {
        uint32_t buff_id = it->second;
        const buff_conf_t *buff_conf = g_buff_conf_mgr.find_buff_conf(buff_id);
        if (!buff_conf) {
            continue;
        }
        //buff有没有这个effect
        if (buff_conf->effects.count((uint32_t)effect_type) == 0) {
            continue;
        }
        //有没有处理这个effect的函数
        if (effect_func_map_.count((uint32_t)effect_type) == 0) {
            continue;
        }
        const buff_effect_t &effect = (buff_conf->effects.find((uint32_t)effect_type))->second;

        (effect_func_map_.find((uint32_t)effect_type)->second)(player, effect, proc_val);
    }
    return 0;
}
