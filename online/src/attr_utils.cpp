#include "attr_utils.h"
#include "proto_queue.h"
#include "player.h"
#include "global_data.h"
#include "service.h"
#include "utils.h"
#include "user_action_log_utils.h"
#include "sys_ctrl.h"
#include "dll_iface.h"
#include "statlogger/statlogger.h"
#include "time_utils.h"
#include "duplicate_conf.h"
#include "player_utils.h"
#include "sys_ctrl.h"
#include "prize.h"
#include "player_manager.h"
#include "rank_utils.h"

//////////////////////////helper/////////////////////////////
// 监控属性值是否超过上限, 如超过上限，报警
inline void monitor_attr_daily_limit(player_t* player, attr_type_t attr_type, int64_t* diff)
{
    if (!diff) {
        return ; 
    }

    const attr_config_t* attr_config = AttrUtils::get_attr_config(attr_type);
    if (attr_config == NULL) {
        return ; 
    }

    if (*diff > 0 && attr_config->daily_limit_key && attr_config->daily_limit_max) {
        assert(attr_config->daily_limit_key != (uint32_t)attr_type); // 否则会死循环
        uint32_t daily_num = GET_A((attr_type_t)attr_config->daily_limit_key);
        if (daily_num + *diff > attr_config->daily_limit_max) {
            ERROR_TLOG("%u daily attr exceed limit: type = %d, daily = %d, diff = %d, old_value = %u, \nstack: %s", 
                    player->userid, attr_type, daily_num, *diff, GET_A(attr_type), stack_trace().c_str()); 
            asynsvr_send_warning_msg("AttrExceedErr", player->userid, g_online_id, 0, 
                    get_server_name());

            if (attr_config->daily_restrict) {
                int can_add = attr_config->daily_limit_max - daily_num;
                if (can_add < 0) {
                    can_add = 0; 
                }

                *diff = can_add;
            }
        }
        SET_A((attr_type_t)attr_config->daily_limit_key, daily_num + *diff);
    }
}

int AttrUtils::set_attr_value(player_t* player, 
            uint32_t num_attr, attr_data_info_t *attrs,
            bool wait_ret,
            onlineproto::syn_attr_reason_t syn_attr_reason)
{
    std::vector<attr_data_info_t> attr_vec;
    attr_data_info_t attr_data;
    for (uint32_t i = 0; i < num_attr; i++) {
        attr_data.type = attrs[i].type;
        attr_data.value = attrs[i].value;
        attr_vec.push_back(attr_data);
    }
    return set_attr_value(player, attr_vec, wait_ret, syn_attr_reason);
}

int AttrUtils::set_attr_value(player_t* player, 
            std::map<uint32_t, uint32_t> &attr_map,
            bool wait_ret,
            onlineproto::syn_attr_reason_t sync_attr_reason)
{
    if (attr_map.size() == 0) {
        return 0; 
    }

    dbproto::cs_set_attr db_msg;
    db_msg.Clear();
    onlineproto::sc_0x0111_sync_attr cli_msg;
    cli_msg.Clear();
    std::vector<attr_change_log_info_t> attr_log_vec;
    attr_log_vec.clear();
    std::vector<attr_data_info_t> mutable_attr_list;

    FOREACH(attr_map, it) {
        uint32_t type = it->first;
        uint32_t value = it->second;
        uint32_t old_value = GET_A((attr_type_t)(type));
        uint32_t new_value = value;
        int64_t diff = (int64_t)new_value - (int64_t)old_value; // 防止new_value int32溢出
        int64_t new_diff = diff;
        if (new_diff == 0) {
            continue;
        }

        // 监控是否超过每日上限
        monitor_attr_daily_limit(player, (attr_type_t)type, &new_diff);

        attr_data_info_t mutable_attr;
        mutable_attr.type = type;
        mutable_attr.value = value;
        if (diff != new_diff) {
            mutable_attr.value = old_value + new_diff; 
        }

        //判定是否要进行特殊统计
        if (g_attr_stat_func_map.count(type)) {
            (g_attr_stat_func_map.find(type)->second)(player, 
                    old_value, old_value+new_diff);
        }

        // 组DB协议
        commonproto::attr_data_t *db_attr_data = db_msg.add_attrs();
        db_attr_data->set_type(mutable_attr.type);
        db_attr_data->set_value(mutable_attr.value);

        // 组online协议
        // 部分attr不同步给客户端
        if (!((type >= kAttrServiceStartID && type < kAttrServiceEndID)
            || (type >= kMinPetOwnAttrType && type < kMaxPetOwnAttrType))) {
            commonproto::attr_data_t *cli_attr_data = cli_msg.add_attr_list();
            cli_attr_data->set_type(mutable_attr.type);
            cli_attr_data->set_value(mutable_attr.value);
        }

        // 是否需要记录属性改变日志
        if (g_user_action_log_config.count((uint32_t)mutable_attr.type)) { //需要记录
            attr_change_log_info_t log;
            log.attr_type = mutable_attr.type;
            log.chg_val = new_diff;
            log.orig_val = old_value;
            log.chg_reason = (uint32_t)(sync_attr_reason);
            attr_log_vec.push_back(log);
        }
        mutable_attr_list.push_back(mutable_attr);
    }
    if (db_msg.attrs_size() == 0) {
        return 0;
    }

    // 同步db
    int ret = 0;
    if (wait_ret) {
        ret = g_dbproxy->send_msg(
                player, player->userid, player->create_tm, db_cmd_set_attr, db_msg);
    } else {
        ret = g_dbproxy->send_msg(NULL, player->userid, player->create_tm, db_cmd_set_attr, db_msg);
    }

    if (ret != 0) {
        ERROR_TLOG("%u set attr to db error", player->userid);
        return ret;
    }

    // 同步内存
    Attr* attr = player->attrs;
    for (uint32_t i = 0; i < mutable_attr_list.size(); i++) {
        attr_data_info_t mutable_attr = mutable_attr_list[i];
        attr->put_attr(mutable_attr);
    }

    //user_action_log
    UserActionLogUtils::log_attr_change(player, attr_log_vec); 
    
    cli_msg.set_reason(sync_attr_reason);
    // 同步客户端
    return send_msg_to_player(player, cli_cmd_cs_0x0111_sync_attr, cli_msg);
}

int AttrUtils::set_attr_value(player_t* player, 
            std::vector<attr_data_info_t> &attr_vec,
            bool wait_ret,
            onlineproto::syn_attr_reason_t sync_attr_reason)
{
    if (attr_vec.size() == 0) {
        return 0; 
    }
    std::map<uint32_t, uint32_t> attr_map;
    FOREACH(attr_vec, it) {
        const attr_data_info_t &attr = *it;
        if (attr_map.count(attr.type) == 0) {
            attr_map[attr.type] = attr.value;
        } else {
            uint32_t cur = GET_A((attr_type_t)attr.type);
            int diff = attr.value - cur;
            uint32_t new_val = attr_map[attr.type];
            if (diff > 0) {
                new_val += diff;
            } else if (new_val < (uint32_t)(-diff)){
                new_val = 0;
            } else {
                new_val -= (-diff);
            }
            attr_map[attr.type] = new_val;
        }
    }
    return set_attr_value(player, attr_map, wait_ret, sync_attr_reason);
}

int AttrUtils::set_single_attr_value(player_t* player,
        enum attr_type_t type, uint32_t value, 
        bool wait_ret, 
        onlineproto::syn_attr_reason_t syn_attr_reason)
{
    attr_data_info_t info;
    info.type = type;
    info.value = value;
    std::vector<attr_data_info_t> attr_vec;
    attr_vec.push_back(info);
    return AttrUtils::set_attr_value(player, attr_vec, wait_ret, syn_attr_reason);
}

int AttrUtils::sub_attr_value(player_t* player, 
            enum attr_type_t type, uint32_t value, 
            bool wait_ret,
            onlineproto::syn_attr_reason_t syn_attr_reason)
{
    attr_data_info_t attr_data;
    uint32_t org_value = GET_A(type);
    attr_data.type = type;
    if (org_value >= value) {
        attr_data.value = org_value - value;
    } else {
        return cli_err_lack_usable_thing;
    }
    std::vector<attr_data_info_t> attr_vec;
    attr_vec.push_back(attr_data);
    return AttrUtils::set_attr_value(player, attr_vec, wait_ret, syn_attr_reason);
}

int AttrUtils::add_attr_value(player_t* player,
        enum attr_type_t type, uint32_t value, 
        bool wait_ret,
        onlineproto::syn_attr_reason_t syn_attr_reason) 
{
    uint32_t value_before = GET_A(type);
    uint32_t value_aft = value_before + value;
    uint32_t value_limit = GET_A_MAX(type);
    if (value_limit != kAttrMaxNoLimit) { 
        value_aft = value_aft > value_limit? value_limit: value_aft;
    }

    return AttrUtils::set_single_attr_value(player, type, value_aft, 
            wait_ret, syn_attr_reason);
}

uint32_t AttrUtils::get_attr_max_limit(const player_t *player, enum attr_type_t type)
{
    std::map<uint32_t, attr_config_t>::iterator it = g_attr_configs.find(type);
    if (it == g_attr_configs.end()){
        return kAttrMaxNoLimit;
    }
    if (!player) {
        return (it->second).max;
    }
    //return is_this_year_vip(player) ?(it->second).svip_max 
    //    :(is_vip(player) ?(it->second.vip_max) :(it->second).max);
	return is_gold_vip(player) ?it->second.svip_max
		  : (is_silver_vip(player) ?it->second.vip_max : it->second.max);
}

uint32_t AttrUtils::get_attr_initial_value(const player_t *player, enum attr_type_t type)
{
    std::map<uint32_t, attr_config_t>::iterator it = g_attr_configs.find(type);
    if (it == g_attr_configs.end()){
        return 0;
    }
    attr_config_t &attr = it->second;
    if (!player) {
        return attr.initial;
    }
    //return is_this_year_vip(player) ?attr.svip_initial
    //    :(is_vip(player) ?attr.vip_initial :attr.initial);
	return is_gold_vip(player) ?attr.svip_initial
		  :(is_silver_vip(player) ?attr.vip_initial : attr.initial);
}

uint32_t AttrUtils::get_attr_value(const player_t* player, enum attr_type_t type)
{
    Attr* attr = player->attrs;
    uint32_t value = 0;
    int ret = attr->get_value_by_type(type, &value);

    if (ret != 0){
        return get_attr_initial_value(player, type);
    }

    return value;
}

bool AttrUtils::is_valid_cli_attr(uint32_t type)
{
    return  (type > (uint32_t)kAttrCliOnlyStart && type < (uint32_t)kAttrCliOnlyEnd);
}

/** 
 * @brief 属性对客户端是否可以开放查询
 * 
 * @param type 属性type
 * 
 * @return 
 */
bool AttrUtils::is_valid_cli_get_attr(uint32_t type)
{
    //return  (type > (uint32_t)kAttrDuplicateRecordStart  && type < (uint32_t)kAttrDuplicateRecordEnd);
	if ((type > (uint32_t)kAttrDuplicateRecordStart  && type < (uint32_t)kDailyDuplicateEnterStart)) {
		return true;
	} else if (type == (uint32_t)kAttrPraiseCount) {
		return true;
	} else {
		return false;
	}
}

int AttrUtils::ranged_clear(player_t* player, uint32_t low, uint32_t high, bool noti_client)
{
    if (low > high) {
        return 0; 
    }

    INFO_TLOG("ranged clear attr: player = %u, low = %u, high = %u",
            player->userid, low, high);

    Attr* attr = player->attrs;

    // 清理内存
    std::vector<uint32_t> type_list;
    attr->ranged_clear(low, high, type_list);

    // 数据库清理
    dbproto::cs_clear_ranged_attr cs_clear_ranged_attr;
    cs_clear_ranged_attr.set_low(low);
    cs_clear_ranged_attr.set_high(high);

    int ret = g_dbproxy->send_msg(NULL, player->userid, player->create_tm,
            db_cmd_clear_ranged_attr, cs_clear_ranged_attr);
    if (ret != 0) {
        return -1; 
    }
    // 通知客户端
    if (noti_client) {
        onlineproto::sc_0x0111_sync_attr cli_msg;
        for (uint32_t i = 0; i < type_list.size(); i++) {
            commonproto::attr_data_t *cli_attr_data = cli_msg.add_attr_list();
            cli_attr_data->set_type(type_list[i]);
            cli_attr_data->set_value(0);
        }
        send_msg_to_player(player, cli_cmd_cs_0x0111_sync_attr, cli_msg);
    }
    return 0;
}

int AttrUtils::ranged_reset(player_t *player, uint32_t low, uint32_t high, bool noti_client)
{
    if (low > high) {
        return 0;
    }

    std::map<uint32_t, uint32_t> attr_map;
    for (uint32_t i = low; i <= high; i++) {
        if (!has_attr(player, (attr_type_t)i)) {//还没有这个属性 则不设置
            continue;
        }
        if (GET_A((attr_type_t)i) == GET_A_INIT((attr_type_t)i)) {//无须重设
            continue;
        }
        attr_map[i] = GET_A_INIT((attr_type_t)i);
        if (attr_map.size() == 100) {
            set_attr_value(player, attr_map);
            attr_map.clear();
        }
    }
    return set_attr_value(player, attr_map);
}

int AttrUtils::cache_last_day_attr_value(player_t* player)
{
	player->temp_info.last_day_swim_times = GET_A(kDailyHasSwim);
	player->temp_info.last_day_escort_times = GET_A(kDailyEscortTimes);
	player->temp_info.last_day_mon_crisis_reset_times = GET_A(kDailyMonsterCrisisReset);

	return 0;
}

int AttrUtils::set_resource_retrieve_value(player_t *player)
{
	uint32_t swim_retrieve = (1 - player->temp_info.last_day_swim_times) >= 1 ? 1:0;
	//运宝资源找回份数
	uint32_t escort_tmp = 2 - player->temp_info.last_day_escort_times;
	uint32_t escort_retrieve = escort_tmp >= 2 ? 2 : escort_tmp;
	//怪物危机找回份数
	uint32_t times = 1;
	if( is_vip(player)){
		times = 2;
	}
	uint32_t mon_tmp = times - player->temp_info.last_day_mon_crisis_reset_times;
	uint32_t mon_crisis_retrieve = mon_tmp >= times ? times : mon_tmp; 

	SET_A(kDailyRetrieveSwimResource, swim_retrieve);
	SET_A(kDailyRetrieveEscortResource, escort_retrieve);
    if (GET_A(kAttrDupHighestUnlock1Id) == 0) {
        SET_A(kDailyRetrieveMonCrisisResource, 0);
    } else {
        SET_A(kDailyRetrieveMonCrisisResource, mon_crisis_retrieve);
    }
	return 0;
}
int AttrUtils::reset_daily_attr_value(player_t* player, bool noti_client)
{
    uint32_t last_daily_clean_date = GET_A(kAttrLastDailyAttrCleanDate);
    uint32_t date = Utils::get_date();
    if (last_daily_clean_date == date) {
        return 0;
    }
    // 缓存前一天daily attr（用于资源找回）
    AttrUtils::cache_last_day_attr_value(player);

    //普通每日属性
    int ret = AttrUtils::ranged_reset(player, kMinDailyAttrType, 
            kMaxDailyAttrType, noti_client);
    if (ret) {
        return -1; 
    }

    //副本每日属性
    //第一部分
    ret = AttrUtils::ranged_reset(player, kMinDupDailyAttr, 
            kMinDupDailyAttr + g_cur_max_dup_id - 1, noti_client);
    //第二部分
    ret = AttrUtils::ranged_reset(player, kMinDupDailyAttr1, 
            kMinDupDailyAttr1 + g_cur_max_dup_id - 1, noti_client);
    if (ret) {
        return -1; 
    }

	//设置资源找回参数
	set_resource_retrieve_value(player);

    //刷新冲钻抽卡卡牌
    std::vector<cache_prize_elem_t> result;
    refresh_player_charge_diamond_draw_prize_info(player, result);

	//记录牙牙活动范围内的登录次数
	AttrUtils::add_attr_in_special_time_range(player,
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivLoginCnt);
    
    SET_A(kAttrLastDailyAttrCleanDate, date);
    return 0;
}

int AttrUtils::clear_special_attr_value(player_t *player, bool noti_client)
{
    return 0;
}

int AttrUtils::reset_weekly_attr_value(player_t* player, bool noti_client)
{
    time_t cur_time = NOW();
    time_t last_clean_time = AttrUtils::get_attr_value(
            player, kAttrLastWeeklyAttrCleanDate);
    
    if (TimeUtils::check_is_week_past(last_clean_time, cur_time)) {
        int ret = AttrUtils::ranged_reset(player, 
                kMinWeeklyAttrType, kMaxWeeklyAttrType, noti_client); 
        if (ret) {
            return -1;
        }
        //副本每周属性
        ret = AttrUtils::ranged_reset(player, kMinDupWeeklyAttr, 
                kMinDupWeeklyAttr + g_cur_max_dup_id - 1, noti_client);
        if (ret) {
            return -1; 
        }

        return SET_A(kAttrLastWeeklyAttrCleanDate, cur_time);
    }
    return 0;
}
    
int AttrUtils::reset_monthly_attr_value(player_t* player, bool noti_client)
{
    time_t cur_time = NOW();
    time_t last_clean_time = GET_A(kAttrLastMonthlyAttrCleanDate);
    
    if (TimeUtils::check_is_month_past(last_clean_time, cur_time)) {
        int ret = AttrUtils::ranged_reset(player, 
                kMinMonthlyAttrType, kMaxMonthlyAttrType, noti_client); 
    
        if (ret) {
            return -1;
        }
        return SET_A(kAttrLastMonthlyAttrCleanDate, cur_time);
    }
    return 0;
}

const attr_config_t* AttrUtils::get_attr_config(attr_type_t attr_type)
{
    std::map<uint32_t, attr_config_t>::iterator it = g_attr_configs.find(attr_type);

    if (it == g_attr_configs.end()) {
        return NULL; 
    } else {
        return &it->second;    
    }

    return NULL;
}

void AttrUtils::register_stat_func()
{
    //属性变化值的统计函数
    g_attr_stat_func_map.clear();
    //注册函数
}

attr_type_t AttrUtils::get_duplicate_daily_times_attr(uint32_t dup_id)
{
    assert(dup_id <= MAX_DUPLICATE_ID);
    return (attr_type_t)(kDailyDuplicateEnterStart + dup_id);
}

attr_type_t AttrUtils::get_duplicate_daily_reset_times_attr(uint32_t dup_id)
{
    assert(dup_id <= MAX_DUPLICATE_ID);
    return (attr_type_t)(kDailyResetDuplicate1Time + dup_id - 1);
}

attr_type_t AttrUtils::get_duplicate_weekly_times_attr(uint32_t dup_id)
{
    assert(dup_id <= MAX_DUPLICATE_ID);
    return (attr_type_t)(kWeeklyDuplicateEnterStart + dup_id);
}

attr_type_t AttrUtils::get_duplicate_pass_time_attr(uint32_t dup_id)
{
    assert(dup_id <= MAX_DUPLICATE_ID);
    return (attr_type_t)(kAttrDuplicateRecord1Time + dup_id - 1);
}

attr_type_t AttrUtils::get_duplicate_best_time_attr(uint32_t dup_id)
{
    assert(dup_id <= MAX_DUPLICATE_ID);
    return (attr_type_t)(kAttrDuplicateRecord1BestTime + dup_id - 1);
}

attr_type_t AttrUtils::get_duplicate_best_star_attr(uint32_t dup_id)
{
    assert(dup_id <= MAX_DUPLICATE_ID);
    return (attr_type_t)(kAttrDuplicateRecord1Star + dup_id - 1);
}

attr_type_t AttrUtils::get_duplicate_last_play_time_attr(uint32_t dup_id)
{
    assert(dup_id <= MAX_DUPLICATE_ID);
    return (attr_type_t)(kAttrDuplicateRecord1LastTime + dup_id - 1);
}

attr_type_t AttrUtils::get_tran_card_id_attr(uint32_t card_id)
{
	return (attr_type_t)(kAttrTransCardStart + 2 * (card_id -1) + 1);
}

attr_type_t AttrUtils::get_tran_card_star_level_attr(uint32_t card_id) 
{
	return (attr_type_t)(kAttrTransCardStart + 2 * (card_id - 1) + 2);
}

attr_type_t AttrUtils::get_tran_choose_flag(uint32_t card_id) 
{
	return (attr_type_t)(kAttrTransCardStart + 2 * card_id + 3);
}

attr_type_t AttrUtils::get_dup_pass_tm_duration_attr(uint32_t dup_id)
{
	assert(dup_id <= MAX_DUPLICATE_ID);
	return (attr_type_t)(kAttrDupExtraRecordStart + dup_id);
}

uint32_t AttrUtils::get_pet_pass_dup_tm_attr(uint32_t dup_id,
		uint32_t pet_id, uint32_t& type)
{
	assert(dup_id <= MAX_DUPLICATE_ID);
	const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_id);
	if (pet_conf == NULL) {
		return cli_err_pet_id_invalid;
	}
	type = pet_conf->dup_pass_time_attr + dup_id;
	return 0;
}

attr_type_t AttrUtils::get_dup_revival_cnt_attr(uint32_t dup_id)
{
	assert(dup_id <= MAX_DUPLICATE_ID);
	return (attr_type_t)(kAttrDup1RevivalCnt + dup_id - 1);
}

attr_type_t AttrUtils::get_dup_surplus_hp_percent(uint32_t dup_id)
{
	assert(dup_id <= MAX_DUPLICATE_ID);
	return (attr_type_t)(kAttrDup1HpPercent + dup_id - 1);
}

attr_type_t AttrUtils::get_dup_power_record_attr(uint32_t dup_id)
{
	assert(dup_id <= MAX_DUPLICATE_ID);
	return (attr_type_t)(kAttrDup1PowerRecord + dup_id - 1);
}

attr_type_t AttrUtils::get_player_attr_by_equip_attr(equip_add_attr_t equip_attr)
{
    switch(equip_attr){
    case EQUIP_ADD_ATTR_HP:
    case EQUIP_ADD_ATTR_PERCENT_HP:
        return kAttrHpMax;
    case EQUIP_ADD_ATTR_NATK:
    case EQUIP_ADD_ATTR_PERCENT_NATK:
        return kAttrNormalAtk;
    case EQUIP_ADD_ATTR_NDEF:
    case EQUIP_ADD_ATTR_PERCENT_NDEF:
        return kAttrNormalDef;
    case EQUIP_ADD_ATTR_SATK:
    case EQUIP_ADD_ATTR_PERCENT_SATK:
        return kAttrSkillAtk;
    case EQUIP_ADD_ATTR_SDEF:
    case EQUIP_ADD_ATTR_PERCENT_SDEF:
        return kAttrSkillDef;

    case EQUIP_ADD_ATTR_CRIT:
    case EQUIP_ADD_ATTR_PERCENT_CRIT:
        return kAttrCrit;
    case EQUIP_ADD_ATTR_ANTI_CRIT:
    case EQUIP_ADD_ATTR_PERCENT_ANTI_CRIT:
        return kAttrAntiCrit;
    case EQUIP_ADD_ATTR_HIT:
    case EQUIP_ADD_ATTR_PERCENT_HIT:
        return kAttrHit;
    case EQUIP_ADD_ATTR_DODGE:
    case EQUIP_ADD_ATTR_PERCENT_DODGE:
        return kAttrDodge;
    case EQUIP_ADD_ATTR_BLOCK:
    case EQUIP_ADD_ATTR_PERCENT_BLOCK:
        return kAttrBlock;
    case EQUIP_ADD_ATTR_BREAK_BLOCK:
    case EQUIP_ADD_ATTR_PERCENT_BREAK_BLOCK:
        return kAttrBreakBlock;

    case EQUIP_ADD_ATTR_ANTI_WATER:
    case EQUIP_ADD_ATTR_PERCENT_ANTI_WATER:
        return kAttrAntiWater;
    case EQUIP_ADD_ATTR_ANTI_FIRE:
    case EQUIP_ADD_ATTR_PERCENT_ANTI_FIRE:
        return kAttrAntiFire;
    case EQUIP_ADD_ATTR_ANTI_GRASS:
    case EQUIP_ADD_ATTR_PERCENT_ANTI_GRASS:
        return kAttrAntiGrass;
    case EQUIP_ADD_ATTR_ANTI_LIGHT:
    case EQUIP_ADD_ATTR_PERCENT_ANTI_LIGHT:
        return kAttrAntiLight;
    case EQUIP_ADD_ATTR_ANTI_DARK:
    case EQUIP_ADD_ATTR_PERCENT_ANTI_DARK:
        return kAttrAntiDark;
    case EQUIP_ADD_ATTR_ANTI_GROUND:
    case EQUIP_ADD_ATTR_PERCENT_ANTI_GROUND:
        return kAttrAntiGround;
    case EQUIP_ADD_ATTR_ANTI_FORCE:
    case EQUIP_ADD_ATTR_PERCENT_ANTI_FORCE:
        return kAttrAntiForce;

    case EQUIP_ADD_ATTR_DAMAGE_RATE_WATER:
    case EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_WATER:
        return kAttrPlayerElemDamageRateWater;
    case EQUIP_ADD_ATTR_DAMAGE_RATE_FIRE:
    case EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_FIRE:
        return kAttrPlayerElemDamageRateFire;
    case EQUIP_ADD_ATTR_DAMAGE_RATE_GRASS:
    case EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_GRASS:
        return kAttrPlayerElemDamageRateGrass;
    case EQUIP_ADD_ATTR_DAMAGE_RATE_LIGHT:
    case EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_LIGHT:
        return kAttrPlayerElemDamageRateLight;
    case EQUIP_ADD_ATTR_DAMAGE_RATE_DARK:
    case EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_DARK:
        return kAttrPlayerElemDamageRateDark;
    case EQUIP_ADD_ATTR_DAMAGE_RATE_GROUND:
    case EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_GROUND:
        return kAttrPlayerElemDamageRateGround;
    case EQUIP_ADD_ATTR_DAMAGE_RATE_FORCE:
    case EQUIP_ADD_ATTR_PERCENT_DAMAGE_RATE_FORCE:
        return kAttrPlayerElemDamageRateForce;

    case EQUIP_ADD_ATTR_MAGIC_ENERGY:
        return kAttrMaxTp;

    default:
        return kAttrHpMax;
    }
}

attr_type_t AttrUtils::get_player_attr_by_pet_normal_attr(battle_value_normal_type_t pet_normal_attr)
{
    switch (pet_normal_attr) {
    case kBattleValueNormalTypeHp:
        return kAttrHpMax;
    case kBattleValueNormalTypeNormalAtk:
        return kAttrNormalAtk;
    case kBattleValueNormalTypeNormalDef:
        return kAttrNormalDef;
    case kBattleValueNormalTypeSkillAtk:
        return kAttrSkillAtk;
    case kBattleValueNormalTypeSkillDef:
        return kAttrSkillDef;
    default:
        return kAttrHpMax;
    }
}

attr_type_t AttrUtils::get_player_attr_by_pet_hide_attr(battle_value_hide_type_t pet_hide_attr)
{
    switch (pet_hide_attr) {
    case kBattleValueHideTypeCrit:
        return kAttrCrit;
    case kBattleValueHideTypeAntiCrit:
        return kAttrAntiCrit;
    case kBattleValueHideTypeHit:
        return kAttrHit;
    case kBattleValueHideTypeDodge:
        return kAttrDodge;
    case kBattleValueHideTypeBlock:
        return kAttrBlock;
    case kBattleValueHideTypeBreakBlock:
        return kAttrBreakBlock;
    case kBattleValueHideTypeCritAffectRate:
        return kAttrCritDamageRate;
    case kBattleValueHideTypeBlockAffectRate:
        return kAttrBlockDamageRate;
    case kBattleValueHideTypeAtkSpeed:
        return kAttrAtkSpeed;
    default:
        return kAttrHp;
    }
}

battle_value_normal_type_t AttrUtils::get_pet_attr_by_equip_chisel_attr(equip_add_attr_t equip_attr)
{
    switch(equip_attr){
    case EQUIP_ADD_ATTR_HP:
        return kBattleValueNormalTypeHp;
    case EQUIP_ADD_ATTR_NATK:
        return kBattleValueNormalTypeNormalAtk;
    case EQUIP_ADD_ATTR_NDEF:
        return kBattleValueNormalTypeNormalDef;
    case EQUIP_ADD_ATTR_SATK:
        return kBattleValueNormalTypeSkillAtk;
    case EQUIP_ADD_ATTR_SDEF:
        return kBattleValueNormalTypeSkillDef;
    default:
        return kBattleValueNormalTypeHp;
    }
}

attr_type_t AttrUtils::get_player_attr_by_equip_pos(equip_body_pos_t pos)
{
    switch (pos) {
    case EQUIP_BODY_POS_NONE:
        return kAttrEquipNone;
    case EQUIP_BODY_POS_HEAD:
        return kAttrHead;
    case EQUIP_BODY_POS_BODY:
        return kAttrBody;
    case EQUIP_BODY_POS_HAND:
        return kAttrHand;
    case EQUIP_BODY_POS_LEG:
        return kAttrLeg;
    case EQUIP_BODY_POS_FOOT:
        return kAttrFoot;
    case EQUIP_BODY_POS_ATTR:
        return kAttrOtherEquipAttr;

    case EQUIP_FASHION_BODY_POS_HEAD:
        return kAttrHeadDecorate;
    case EQUIP_FASHION_BODY_POS_FACE:
        return kAttrFaceDecorate;
    case EQUIP_FASHION_BODY_POS_CLOTH:
        return kAttrClothesDecorate;
    case EQUIP_FASHION_BODY_POS_WEAPON:
        return kAttrWeaponDecorate;
    case EQUIP_FASHION_BODY_POS_COAT:
        return kAttrCoatDecorate;
    case EQUIP_FASHION_BODY_POS_WING:
        return kAttrWingDecorate;
    case EQUIP_FASHION_BODY_POS_TAIL:
        return kAttrTailDecorate;

    case EQUIP_BODY_POS_MOUNT:
        return kAttrOtherEquip2;
    case EQUIP_BODY_POS_WING:
        return kAttrOtherEquip3;

    default:
        return kAttrEquipNone;
    }
}

//根据装备的元素属性找到对应的玩家抗性属性
attr_type_t AttrUtils::get_player_anti_attr_by_equip_elem(uint32_t elem_type)
{
    switch(elem_type) {
    case EQUIP_ELEM_TYPE_WATER:
        return kAttrAntiWater;
    case EQUIP_ELEM_TYPE_FIRE:
        return kAttrAntiFire;
    case EQUIP_ELEM_TYPE_GRASS:
        return kAttrAntiGrass;
    case EQUIP_ELEM_TYPE_LIGHT:
        return kAttrAntiLight;
    case EQUIP_ELEM_TYPE_DARK:
        return kAttrAntiDark;
    case EQUIP_ELEM_TYPE_GROUND:
        return kAttrAntiGround;
    case EQUIP_ELEM_TYPE_FORCE:
        return kAttrAntiForce;
    default:
        return (attr_type_t)0;
    }
}

//根据装备的洗练属性找到玩家的属性
attr_type_t AttrUtils::get_player_attr_by_quench_type(uint32_t quench_type)
{
    switch (quench_type) {
    case commonproto::EQUIP_QUENCH_TYPE_NONE: //没有第一条洗练
        break;
    case commonproto::EQUIP_QUENCH_TYPE_1_CRIT: //第一条洗练暴击
        return kAttrCrit;
    case commonproto::EQUIP_QUENCH_TYPE_1_ANTI_CRIT: //第一条洗练防暴
        return kAttrAntiCrit;   
    case commonproto::EQUIP_QUENCH_TYPE_1_HIT: //第一条洗练命中
        return kAttrHit;
    case commonproto::EQUIP_QUENCH_TYPE_1_DODGE: //第一条洗练闪避
        return kAttrDodge;
    case commonproto::EQUIP_QUENCH_TYPE_1_BLOCK: //第一条洗练格挡
        return kAttrBlock;
    case commonproto::EQUIP_QUENCH_TYPE_1_BREAK_BLOCK: //第一条洗练破格
        return kAttrBreakBlock;
    // case commonproto::EQUIP_QUENCH_TYPE_1_HP: //第一条洗练生命
        // return kAttrHp;
    case commonproto::EQUIP_QUENCH_TYPE_1_HP: //第一条洗练生命
        return kAttrHpMax ;
    case commonproto::EQUIP_QUENCH_TYPE_1_NATK: //第一条洗练普攻
        return kAttrNormalAtk;
    case commonproto::EQUIP_QUENCH_TYPE_1_NDEF: //第一条洗练普防
        return kAttrNormalDef;
    case commonproto::EQUIP_QUENCH_TYPE_1_SATK: //第一条洗练魔攻
        return kAttrSkillAtk;
    case commonproto::EQUIP_QUENCH_TYPE_1_SDEF: //第一条洗练魔防
        return kAttrSkillDef;
    case commonproto::EQUIP_QUENCH_TYPE_1_ANTI_WATER: //第一条洗练水抗
        return kAttrAntiWater;
    case commonproto::EQUIP_QUENCH_TYPE_1_ANTI_FIRE: //第一条洗练火抗
        return kAttrAntiFire;
    case commonproto::EQUIP_QUENCH_TYPE_1_ANTI_GRASS: //第一条洗练草抗
        return kAttrAntiGrass;
    case commonproto::EQUIP_QUENCH_TYPE_1_ANTI_LIGHT: //第一条洗练光抗
        return kAttrAntiLight;
    case commonproto::EQUIP_QUENCH_TYPE_1_ANTI_DARK: //第一条洗练暗抗
        return kAttrAntiDark;
    case commonproto::EQUIP_QUENCH_TYPE_1_ANTI_GROUND: //第一条洗练地抗
        return kAttrAntiGround;
    case commonproto::EQUIP_QUENCH_TYPE_1_ANTI_FORCE: //第一条洗练武抗
        return kAttrAntiForce;
    default:
        break;
    }
    return (attr_type_t)0;
}

bool AttrUtils::is_player_gold_enough(player_t* player, uint32_t gold_num)
{
    uint32_t total_gold = get_player_gold(player);
    if (total_gold < gold_num) {
        return false;
    }
    return true;
}

uint32_t AttrUtils::get_player_gold(player_t *player) 
{
    return (GET_A(kAttrPaidGold) + GET_A(kAttrGold));
}

uint32_t AttrUtils::add_player_gold(player_t *player, uint32_t add_num, bool gold_from_diamond, string get_stat_name)
{
    string sub_stat_name;
    if (gold_from_diamond) {
        ADD_A(kAttrPaidGold, add_num);
        sub_stat_name = "付费金币产出";
    } else {
        ADD_A(kAttrGold, add_num);
        sub_stat_name = "免费金币产出";
    }

    //统计
    StatInfo stat_info;
    stat_info.add_info("item", get_stat_name);
    stat_info.add_info(sub_stat_name, add_num);
    stat_info.add_op(StatInfo::op_item, "item");
    stat_info.add_op(StatInfo::op_item_sum, "item", sub_stat_name);
    g_stat_logger->log("金币系统", "金币产出", Utils::to_string(player->userid), "", stat_info);

    //统计总量
    StatInfo stat_info_all;
    stat_info_all.add_info(sub_stat_name, add_num);
    stat_info_all.add_op(StatInfo::op_sum, sub_stat_name);
    g_stat_logger->log("金币系统", "金币产出总计", Utils::to_string(player->userid), "", stat_info_all);
    return 0;
}


uint32_t AttrUtils::sub_player_gold(player_t* player, uint32_t gold_num, string consume_stat_name)
{
	if (gold_num == 0) {
		return 0;
	}
	uint32_t paid_gold = GET_A(kAttrPaidGold);
	uint32_t free_gold = GET_A(kAttrGold);

	uint64_t total_gold = paid_gold + free_gold;
	if (total_gold < (uint64_t)gold_num) {
		return cli_err_gold_not_enough; 
	}
    uint32_t original_gold_num = gold_num;

	uint32_t paid_gold_change = 0;
	// 先扣除收费金币
	if (paid_gold > 0) {
		if (paid_gold >= gold_num) {
			paid_gold -= gold_num; 
			paid_gold_change = gold_num;
			gold_num = 0;
		} else {
			gold_num -= paid_gold;      
			paid_gold_change = paid_gold;
			paid_gold = 0; 
		}
        //assert(paid_gold >= 0);
	}
	// 再扣除免费金币
	if (gold_num > 0) {
		free_gold -= gold_num; 
        //assert(free_gold >= 0);
	}
    std::vector<attr_data_info_t> attr_vec;
    attr_data_info_t attr_data;
    attr_data.type = kAttrGold;
    attr_data.value = free_gold;
    attr_vec.push_back(attr_data);
    attr_data.type = kAttrPaidGold;
    attr_data.value = paid_gold;
    attr_vec.push_back(attr_data);
    attr_data.type = kDailySpendGold;
    attr_data.value = GET_A(kDailySpendGold) + original_gold_num;
    attr_vec.push_back(attr_data);

    //统计 paid_gold_change gold_num

    //统计
    StatInfo stat_info;
    stat_info.add_info("item", consume_stat_name.empty() ?"未知消耗" :consume_stat_name);
    stat_info.add_info("付费金币消耗", paid_gold_change);
    stat_info.add_info("免费金币消耗", gold_num);
    stat_info.add_op(StatInfo::op_item, "item");
    stat_info.add_op(StatInfo::op_item_sum, "item", "付费金币消耗");
    stat_info.add_op(StatInfo::op_item_sum, "item", "免费金币消耗");
    g_stat_logger->log("金币系统", "金币消耗", Utils::to_string(player->userid), "", stat_info);

    //统计总量
    StatInfo stat_info_all;
    stat_info_all.add_info("付费金币消耗", paid_gold_change);
    stat_info_all.add_info("免费金币消耗", gold_num);
    stat_info_all.add_op(StatInfo::op_sum, "付费金币消耗");
    stat_info_all.add_op(StatInfo::op_sum, "免费金币消耗");
    g_stat_logger->log("金币系统", "金币消耗总计", Utils::to_string(player->userid), "", stat_info_all);
	//金币消耗排行
    // if (TimeUtils::is_current_time_valid(TM_CONF_KEY_RANKING_TIME_LIMIT, 3)) {
    if (g_srv_time_mgr.is_now_time_valid(TM_SUBKEY_GOLD_CONSUME)) {
		add_attr_value(player, kAttrGoldConsumeRankingCount, gold_num);
		// uint32_t start_day = TimeUtils::get_start_time(TM_CONF_KEY_RANKING_TIME_LIMIT, 3);
		uint32_t start_day = g_srv_time_mgr.get_start_time(TM_SUBKEY_GOLD_CONSUME);
		//更新金币消耗排名
		RankUtils::rank_user_insert_score(
				player->userid, player->create_tm,
				commonproto::RANKING_TL_GOLD_CONSUME, start_day,
				get_attr_value(player, kAttrGoldConsumeRankingCount));
	}

	return AttrUtils::set_attr_value(player, attr_vec);
}

bool AttrUtils::has_attr(const player_t* player, enum attr_type_t type) 
{
	if (!player->attrs->has_attr(type)) {
		return false;
	}
	return true;
}

//修改其他玩家的属性值
int AttrUtils::change_other_attr_value(
	uint32_t userid, uint32_t create_tm,
	std::vector<attr_data_chg_info_t>& attr_vec)
{
	//TODO kevin 判断玩家是否同服在线，同服直接调用SUB_A,ADD_A
	player_t *player = g_player_manager->get_player_by_userid(userid);
	if (player) {
		FOREACH(attr_vec, iter) {
			uint32_t type = iter->type;
			uint32_t change_value = iter->change_value;
			uint32_t is_minus = iter->is_minus;
			if (!is_minus) {
				ADD_A((attr_type_t)type, change_value);
			} else {
				SUB_A((attr_type_t)type, change_value);
			}
		}
		return 0;
	}
	switchproto::cs_sw_change_other_attr sw_in;
	sw_in.set_uid(userid);
	sw_in.set_u_create_tm(create_tm);
	FOREACH(attr_vec, it) {
		commonproto::attr_data_change_t *data_ptr = sw_in.add_attr_list();
		data_ptr->set_type(it->type);
		data_ptr->set_change_value(it->change_value);
		data_ptr->set_is_minus(it->is_minus);
		data_ptr->set_max_value(it->max_value);
	}
	g_switch->send_msg(
		NULL, g_online_id, 0, sw_cmd_sw_change_other_attr,
		sw_in);
	return 0;
}

int AttrUtils::change_other_attr_value_pub(
		uint32_t userid, uint32_t create_tm,
		uint32_t type, uint32_t change_value,
		bool is_minus)
{
	std::vector<attr_data_chg_info_t> attr_vec;
	attr_data_chg_info_t chg_info;
	chg_info.type = type;
	chg_info.change_value = change_value;
	chg_info.is_minus = is_minus;
	attr_vec.push_back(chg_info);
	AttrUtils::change_other_attr_value(userid, create_tm, attr_vec);
	return 0;
}

/** 
 * @brief 修改其他玩家属性，不能随便使用，这里没有检查属性上限等操作，与修改个人属性的差别要查看SET_A宏定义
          目前只用来修改其他人的家族属性
 * 
 * @param userid 
 * @param attr_vec 
 * 
 * @return 
 */
int AttrUtils::update_other_attr_value(
        uint32_t userid, uint32_t u_create_tm, std::vector<commonproto::attr_data_t>& attr_vec)
{
	//判断玩家是否同服在线
	player_t *player = g_player_manager->get_player_by_userid(userid);
	if (player) {
		FOREACH(attr_vec, iter) {
			SET_A((attr_type_t)iter->type(), iter->value());
		}
		return 0;
	}

    std::vector<attr_change_log_info_t> attr_log_vec;
    attr_log_vec.clear();

    switchproto::cs_sw_transmit_only cs_sw_transmit_only_;
    switchproto::sw_player_basic_info_t* sw_player_basic_info = 
        cs_sw_transmit_only_.add_receivers();
    sw_player_basic_info->set_userid(userid);
    sw_player_basic_info->set_create_tm(u_create_tm);

    cs_sw_transmit_only_.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
    cs_sw_transmit_only_.set_cmd(cli_cmd_cs_0x0111_sync_attr);

    onlineproto::sc_0x0111_sync_attr notice_out_;
    notice_out_.set_reason(onlineproto::SYNC_ATTR_OTHER_USER);

    dbproto::cs_set_attr db_msg;
	FOREACH(attr_vec, it) {
		commonproto::attr_data_t *data_ptr = notice_out_.add_attr_list();
		data_ptr->set_type(it->type());
		data_ptr->set_value(it->value());

        commonproto::attr_data_t *db_attr_data = db_msg.add_attrs();
        db_attr_data->set_type(it->type());
        db_attr_data->set_value(it->value());
    
        // 是否需要记录属性改变日志
        if (g_user_action_log_config.count((uint32_t)it->type())) { //需要记录
            attr_change_log_info_t log;
            log.attr_type = it->type();
            log.chg_val = it->value();
            log.orig_val = 0;
            log.chg_reason = 0;
            attr_log_vec.push_back(log);
        }
    }

    int ret = g_dbproxy->send_msg(NULL, userid, u_create_tm, db_cmd_set_attr, db_msg);
    if (ret != 0) {
        ERROR_TLOG("update other attr, %u(%u) set attr to db failed", userid, u_create_tm);
        return ret;
    }

    player_t tmp_player;
    tmp_player.userid = userid;
    tmp_player.create_tm = u_create_tm;
    UserActionLogUtils::log_attr_change(&tmp_player, attr_log_vec); 

    // 通知玩家同步内存和前端属性
    std::string pkg;
    notice_out_.SerializeToString(&pkg);
    cs_sw_transmit_only_.set_pkg(pkg);
    g_switch->send_msg(NULL, g_online_id, 0, sw_cmd_sw_transmit_only,
            cs_sw_transmit_only_);

	return 0;
}

/** 
 * @brief 更新元素挑战当前属性类型
 * 
 * @return 
 */
int AttrUtils::update_element_dup_attr_type(player_t *player)
{
    // 2014-11-04 00:00:00
    //uint32_t time_start = 1415030400;

    const time_limit_t *time_config = TimeUtils::get_time_limit_config(1,1);
    if (time_config == NULL) {
        return 0;
    }
    uint32_t time_start = time_config->start_time;

    uint32_t now_time = NOW();
    if (time_start > now_time) {
        SET_A(kAttrDupElementType, commonproto::DUP_ELEMENT_TYPE_START);
        return 0;
    }

    // 初始化或者重置
    //if (GET_A(kAttrDupElementType) == 0) {
        //SET_A(kAttrDupElementType, commonproto::DUP_ELEMENT_TYPE_START);
    //}

    uint32_t total_day_cnt = 0;
    std::vector< std::pair<uint32_t,uint32_t> > v_days;
    FOREACH(g_duplicate_conf_mgr.const_dup_map(), iter) {
        if (iter->second.mode == DUP_MODE_TYPE_ELEM_DUP &&
                iter->second.element_id >= commonproto::DUP_ELEMENT_TYPE_START &&
                 iter->second.element_id <= commonproto::DUP_ELEMENT_TYPE_END) {
            bool flag = false;
            FOREACH(v_days, v_iter) {
                if (iter->second.element_id == v_iter->second) {
                    flag = true;
                    break;
                }
            }
            if (flag == false) {
                v_days.push_back(
                        std::pair<uint32_t,uint32_t>(
                            iter->second.switch_days, iter->second.element_id));
                total_day_cnt += iter->second.switch_days;
            }
        }
    }

    if (total_day_cnt == 0 ) {
        return 0;
    }

    // 根据循环天数计算当前属性类型
    uint32_t days_gap = TimeUtils::get_days_between(time_start, now_time);
    uint32_t cycle_day = days_gap % total_day_cnt;
    uint32_t step_days = 0;
    FOREACH(v_days, iter) {
        step_days += iter->first;

        if (cycle_day < step_days) {
            if (GET_A(kAttrDupElementType) != iter->second) {
                SET_A(kAttrDupElementType, iter->second);
            }
            break;
        }
    }

    return 0;
}

int AttrUtils::add_attr_in_special_time_range(player_t* player,
		uint32_t key, uint32_t sub_key,
		attr_type_t type, uint32_t value)
{
	if (!TimeUtils::is_current_time_valid(key, sub_key)) {
		//清掉指定的数据，并设置在活动时间内
		return 0;
	}
	ADD_A(type, value);
	return 0;
}
