#ifndef __PLAYER_UTILS_H__
#define __PLAYER_UTILS_H__

#include "common.h"
#include "utils.h"
#include "player.h"
#include "player_conf.h"

class PlayerUtils {

public: //inline functions
    //计算玩家自身基础的某类型战斗力数值
    static inline uint32_t calc_player_base_prop_by_type(player_t* player, 
            enum battle_value_normal_type_t prop_type) {

        uint32_t prof_idx = GET_A(kAttrCurProf);
        const player_conf_t *p_conf = g_player_conf_mgr.find_player_conf(prof_idx);
        if (!p_conf) {
            SET_A(kAttrCurProf, PROF_WARRIOR);
            p_conf = g_player_conf_mgr.find_player_conf(PROF_WARRIOR);
            assert(p_conf);
        }
        return p_conf->basic_normal_battle_values[prop_type] + 
            (GET_A(kAttrLv) - 1) * p_conf->basic_normal_battle_values_grow[prop_type]/100.0; 
    }

    //计算玩家自身隐藏的某类型战斗里数值
    static inline uint32_t calc_player_advance_prop_by_type(player_t* player, 
            enum battle_value_hide_type_t prop_type) {

        uint32_t prof_idx = GET_A(kAttrCurProf);
        const player_conf_t *p_conf = g_player_conf_mgr.find_player_conf(prof_idx);
        if (!p_conf) {
            SET_A(kAttrCurProf, PROF_WARRIOR);
            p_conf = g_player_conf_mgr.find_player_conf(PROF_WARRIOR);
            assert(p_conf);
        }

        uint32_t value = p_conf->basic_hide_battle_values[prop_type] + 
            (GET_A(kAttrLv) - 1) * p_conf->basic_hide_battle_values_grow[prop_type]/100.0;
        return value;
    }

    //计算玩家自身的战斗力数值
    static inline uint32_t calc_player_self_battle_value(player_t *player, 
            std::map<uint32_t, uint32_t> &attr_map) {

        attr_map.clear();
        attr_data_info_t attr;
        attr.type = kAttrHpMax;
        attr.value = calc_player_base_prop_by_type(player, kBattleValueNormalTypeHp);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrExpMax;
        attr.value = calc_player_exp_max(player);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrNormalAtk;  
        attr.value = calc_player_base_prop_by_type(player, kBattleValueNormalTypeNormalAtk);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrNormalDef;  
        attr.value = calc_player_base_prop_by_type(player, kBattleValueNormalTypeNormalDef);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrSkillAtk;  
        attr.value = calc_player_base_prop_by_type(player, kBattleValueNormalTypeSkillAtk);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrSkillDef;  
        attr.value = calc_player_base_prop_by_type(player, kBattleValueNormalTypeSkillDef);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrCrit;  
        attr.value = calc_player_advance_prop_by_type(player, kBattleValueHideTypeCrit);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrAntiCrit;  
        attr.value = calc_player_advance_prop_by_type(player, kBattleValueHideTypeAntiCrit);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrHit;  
        attr.value = calc_player_advance_prop_by_type(player, kBattleValueHideTypeHit);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrDodge;  
        attr.value = calc_player_advance_prop_by_type(player, kBattleValueHideTypeDodge);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrBlock;  
        attr.value = calc_player_advance_prop_by_type(player, kBattleValueHideTypeBlock);
        attr_map[attr.type] = attr.value;

        attr.type = kAttrBreakBlock;  
        attr.value = calc_player_advance_prop_by_type(player, kBattleValueHideTypeBreakBlock);
        attr_map[attr.type] = attr.value;

        //TODO(singku)默认暴击加成和格挡加成数值
        attr_map[kAttrCritDamageRate] = 80;
        attr_map[kAttrBlockDamageRate] = 40;
		//能量初始值
        attr_map[kAttrMaxTp] = 100;

        //玩家默认不带抗性
        attr_map[kAttrAntiWater] = 0;
        attr_map[kAttrAntiFire] = 0;
        attr_map[kAttrAntiGrass] = 0;
        attr_map[kAttrAntiLight] = 0;
        attr_map[kAttrAntiDark] = 0;
        attr_map[kAttrAntiGround] = 0;
        attr_map[kAttrAntiForce] = 0;

        return 0;
    }

    static inline uint32_t get_player_need_hp_to_maxhp(player_t* player) {
        return GET_A(kAttrHpMax) - GET_A(kAttrHp);
    }

    static inline uint32_t calc_player_exp_max(player_t* player) {

        uint32_t cur_lv = GET_A(kAttrLv);
        return  10*cur_lv*cur_lv*(100+10*(cur_lv+1))/4;
    }

    //改变内存中玩家的血量 不同步到DB
    static inline uint32_t chg_player_hp(player_t* player, int32_t chg,
            enum onlineproto::syn_attr_reason_t sync_reason = onlineproto::ATTR_OTHER_REASON) {
        int32_t cur_hp = (int32_t)(GET_A(kAttrHp));
        int32_t max_hp = player->temp_info.tmp_max_hp; //(int32_t)(GET_A(kAttrHpMax));
        cur_hp += chg;
        if (cur_hp < 0) {
            cur_hp = 0;
        } else if (cur_hp >= max_hp) {
            cur_hp = max_hp;
        }
        SET_A(kAttrHp, cur_hp);
        return 0;
    }

	static inline uint32_t chg_player_night_raid_hp(player_t* player, int32_t chg) {
		int32_t cur_hp = (int32_t)(GET_A(kAttrNightRaidUserCurHp));
		int32_t max_hp = (int32_t)(GET_A(kAttrHpMax));
        cur_hp += chg;
        if (cur_hp < 0) {
            cur_hp = 0;
        } else if (cur_hp >= max_hp) {
            cur_hp = max_hp;
        }
		SET_A(kAttrNightRaidUserCurHp, cur_hp);
		return 0;
	}
    static inline uint32_t player_hide_battle_value_to_rate(prof_type_t prof, 
            battle_value_hide_type_t type, uint32_t value) {

        const player_conf_t *player_conf = g_player_conf_mgr.find_player_conf(prof);
        if (!player_conf) {
            return 0;
        }
        double p = value*1.0 / player_conf->basic_hide_battle_values_coeff[type];
        return (p * 100 / (1 + p));
    }

    //生成玩家头饰的redis缓存key
    static inline string make_head_cache_key(uint32_t uid, uint32_t u_create_tm) {
        ostringstream key;
        key.clear();
        key << "cache_player_head_item:" << uid << "_" << u_create_tm;
        return key.str();
    }

    static inline uint32_t get_cache_uid_from_key(string key) {
        std::vector<string> tmp_str_vec = split(key, ':');
        if (tmp_str_vec.size() != 2) {
            return 0;
        }
        tmp_str_vec = split(tmp_str_vec[1].c_str(), '_');
        if (tmp_str_vec.size() != 2) {
            return 0;
        }
        return atoi_safe(tmp_str_vec[0].c_str());
    }

    static inline uint32_t get_cache_ucrtm_from_key(string key) {
        std::vector<string> tmp_str_vec = split(key, ':');
        if (tmp_str_vec.size() != 2) {
            return 0;
        }
        tmp_str_vec = split(tmp_str_vec[1].c_str(), '_');
        if (tmp_str_vec.size() != 2) {
            return 0;
        }
        return atoi_safe(tmp_str_vec[1].c_str());
    }

public:
	static uint32_t add_player_exp(player_t* player, uint32_t add_exp, 
            uint32_t* real_add_exp, bool addict_detec = true, 
            enum onlineproto::syn_attr_reason_t sync_reason = onlineproto::ATTR_OTHER_REASON);

    static uint32_t calc_level_up_need_exp(uint32_t cur_level, uint32_t to_level);

    //计算玩家战斗力(精灵(符文+自身)刻印 装备, 自身)
    static uint32_t calc_player_battle_value(player_t* player, 
            enum onlineproto::syn_attr_reason_t reason = onlineproto::ATTR_OTHER_REASON);
    
    static inline uint32_t calc_player_equip_cur_battle_value_by_player(player_t* player);

    static uint32_t calc_attr_to_battle_value(
        player_t *player, std::map<uint32_t, uint32_t> &attr_map);

    static uint32_t player_equip_level_up(player_t* player, uint32_t equip_slot_id,
            enum onlineproto::sync_item_reason_t reason = onlineproto::SYNC_REASON_EQUIP_LEVELP_UP);

    static int player_say_cmd(player_t *player, const string &org_cmd, string &ret_content);

	static int update_user_raw_data(
            uint32_t userid, uint32_t create_tm, dbproto::user_raw_data_type_t type, 
            const google::protobuf::Message& message, const std::string &buff_id);

	static int get_user_raw_data(player_t* player, dbproto::user_raw_data_type_t type, const std::string buff_id="0");

	static int delete_user_raw_data(uint32_t userid, uint32_t create_tm, 
            dbproto::user_raw_data_type_t type, const std::string &buff_id);

    static uint32_t get_chisel_pet_cnt(player_t *player);

    static int sync_player_buff(player_t *player);
    static int add_player_buff(player_t *player, uint32_t buff_id);

    static int gm_debug_add_all_pet(player_t *player, uint32_t arg1, uint32_t arg2);
    static int gm_debug_fini_task(player_t *player, uint32_t arg1, uint32_t arg2);
    static int gm_debug_world_boss(
            player_t *player, uint32_t arg1, uint32_t arg2, std::string &ret_str);
    static int noti_gm_debug_msg(player_t *player, std::string &content);
    static int gm_debug_set_family_exp(player_t *player, uint32_t arg1, uint32_t arg2);

    static uint32_t get_equip_skill_num(player_t *player);

    static uint32_t get_chisel_pet_num(player_t *player);

    static uint32_t obj_hp_add_common(uint32_t max_hp, uint32_t btl_value);

	static uint32_t send_mail_notify_home_pets_found_item(player_t* player);

	static uint32_t leave_current_home(player_t* player, bool need_back = false);

	static uint32_t recv_mail_get_test_prize(player_t* player);

	//更新玩家总战力到排行榜
	static uint32_t update_player_power_to_rank(player_t* player, uint32_t new_power);
	//更新玩家金币消耗到排行榜
	// static uint32_t update_player_gold_consume_to_rank(player_t* player, uint32_t count, uint32_t key, uint32_t subkey);

	//产生一封新的邮件
	static uint32_t generate_new_mail(player_t* player, 
			const string title, const string content,
			uint32_t prize_id = 0);

	static bool test_vip_state(player_t* player);

	static uint32_t update_player_escort_power(uint32_t userid,
			uint32_t u_create_tm, uint32_t top_power);

	static uint32_t recv_system_mail(player_t* player, mail_type_t mail_type);
	
	static uint32_t recv_mail_get_create_prize(player_t* player);

	static uint32_t recv_mayin_bucket_prize(player_t* player);

	static uint32_t recv_mayin_flower_prize(player_t* player);

	static uint32_t recv_merge_server_prize(player_t* player);

    //各种杂项功能的战斗属性加成
	static uint32_t calc_extra_add_attr(player_t* player,
			std::map<uint32_t, uint32_t> &attr_map);

	static uint32_t deal_equiped_title(player_t* player);
	
	static uint32_t deal_after_merger_server(player_t* player);
	static uint32_t calc_player_suit_buff_info(player_t* player);
	static uint32_t check_player_suit_buff(player_t* player, const std::map<uint32_t, uint32_t> &suit_equip_cnt_map);

};

#endif
