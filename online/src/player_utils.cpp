#include <boost/lexical_cast.hpp>
#include <math.h>
#include "player_utils.h"
#include "attr_utils.h"
#include "player_conf.h"
#include "item_conf.h"
#include "data_proto_utils.h"
#include "global_data.h"
#include "item_conf.h"
#include "item.h"
#include "pet_utils.h"
#include "sys_ctrl.h"
#include "trans_prof.h"
#include "rank_utils.h"
#include "family_utils.h"
#include "rune_utils.h"
#include "task_utils.h"
#include "prize.h"
#include "tran_card.h"
#include "service.h"
#include "buff_conf.h"
#include "mail_utils.h"
#include "duplicate_utils.h"
#include "player_manager.h"
#include "common.h"
#include "skill_conf.h"
#include "task_processor.h"
#include "arena.h"
#include "equip_utils.h"
#include "arena.h"
#include "home_data.h"
#include "prize_processor.h"
#include "achieve.h"
#include "title.h"
#include "timer_procs.h"
#include "suit_conf.h"

uint32_t PlayerUtils::add_player_exp(player_t* player, uint32_t add_exp, 
        uint32_t* real_add_exp, bool addict_detec, 
        enum onlineproto::syn_attr_reason_t reason)
{
	//防沉迷
	if(addict_detec && check_player_addicted_threshold_none(player)){
		return 0;
	} else if (addict_detec && check_player_addicted_threshold_half(player)){
		 add_exp /= 2;
	}

    uint32_t cur_level = GET_A(kAttrLv);
	uint32_t old_level = cur_level;

    if (cur_level >= PLAYER_MAX_LEVEL) {
        add_exp = 0;
    }
    if (real_add_exp) {
        *real_add_exp = 0; 
    }
    if (add_exp == 0) {
        return 0; 
    }

    uint32_t org_exp = GET_A(kAttrExp);
    uint32_t need_exp = calc_level_up_need_exp(cur_level, cur_level + 1);
    //检测万一当前经验已经可以升级了
    while (need_exp <= org_exp) {
        org_exp -= need_exp;
        cur_level++;
        need_exp = calc_level_up_need_exp(cur_level, cur_level+1);
        if (cur_level >= PLAYER_MAX_LEVEL) {
            break;
        }
    }

    if (cur_level < PLAYER_MAX_LEVEL) {
        while (need_exp - org_exp <= add_exp) {//可升级
            add_exp -= (need_exp - org_exp);
            if (real_add_exp) {
                *real_add_exp += (need_exp - org_exp);
            }
            org_exp = 0;
            cur_level++;
            need_exp = calc_level_up_need_exp(cur_level, cur_level + 1);
            if (cur_level >= PLAYER_MAX_LEVEL) {
                break;
            }
        }

    }

    uint32_t add_level = cur_level - GET_A(kAttrLv);
	SET_A(kAttrLv, cur_level);
    if (cur_level >= 20) {
        if (GET_A(kAttrInviterUid) != 0 && GET_A(kAttrInviterAddTm) == 0) {
            AttrUtils::change_other_attr_value_pub(GET_A(kAttrInviterUid), 
                    GET_A(kAttrInviterCreateTm), kAttrInvitedPlayers, 1, false);
            SET_A(kAttrInviterAddTm, NOW());
        }
    }
    uint32_t left_exp;
    left_exp = org_exp + add_exp;
    uint32_t extra_adder;

    if (left_exp >= need_exp) {
        SET_A(kAttrExp, need_exp);
        extra_adder = need_exp - org_exp;
    } else {
        SET_A(kAttrExp, left_exp);
        extra_adder = add_exp;
    }

    if (real_add_exp) {
        *real_add_exp += extra_adder;
    }


    uint32_t ret = 0;
   
    if (add_level) {//
        if (GET_A(kAttrGuideFinished)) {//新手完成统计等级分布
            //游戏自定义统计等级分布
            Utils::write_msglog_new(player->userid, "基本", "活跃用户等级分布", Utils::to_string(cur_level));
            //等级统一接口
            g_stat_logger->level_up(Utils::to_string(player->userid), "", cur_level);

            // 初始化坐骑
            EquipUtils::add_init_mount(player);

            // 初始化翅膀
            EquipUtils::add_init_wing(player);

			//相关成就监听
					//ACH_02_INC_LEVEL
        }

        PetUtils::calc_pet_bag_size(player);
        if (reason == onlineproto::SYNC_EXP_FROM_BATTLE) {
            ret = PlayerUtils::calc_player_battle_value(player, onlineproto::SYNC_LEVELUP_FROM_BATTLE); 
        } else {
            ret = PlayerUtils::calc_player_battle_value(player, reason); 
        }
        //升级之后血回满
        SET_A(kAttrHp, GET_A(kAttrHpMax));
		//升到第10级之后，在竞技场中插入排名先
		if (old_level < ARENA_INSERT_RANK_BASED_LV &&
				cur_level >= ARENA_INSERT_RANK_BASED_LV) {
			if (GET_A(kAttrGuideFinished)) {
				RankUtils::rank_insert_last(
						NULL, player->userid, player->create_tm,
						commonproto::RANKING_ARENA,
						0, commonproto::RANKING_ORDER_ASC);
			}
		}
    }

    // 更新换装计划开始时间
    update_change_clothes_plan_start_time(player);

    return ret;
}

uint32_t PlayerUtils::calc_level_up_need_exp(uint32_t cur_level, uint32_t to_level)
{
    if (cur_level >= to_level) {
        return 0;
    }
    //if (to_level > PLAYER_MAX_LEVEL) {
    //    return 0xFFFFFFFF;
    //}

    uint32_t exp = 0;

    uint32_t temp_level = cur_level;
    while(temp_level < to_level) {
        exp += 10 * temp_level * temp_level * (100 + 10*(temp_level+1))/4;
        temp_level++;
    }

    return exp;
}

uint32_t PlayerUtils::calc_player_battle_value(player_t* player,
        enum onlineproto::syn_attr_reason_t reason) 
{
    //重算人物自身基础战斗属性数值
    std::map<uint32_t, uint32_t> attr_map;
    calc_player_self_battle_value(player, attr_map);

    //非战斗中更新能量
	// if (player->temp_info.dup_id == 0) {
		// attr_map[kAttrTp] = 100;
	// }

    //计算装备给予玩家附加的战斗属性数值
    uint32_t elem_damage_percent = 50; //不带属性石的时候默认克制百分比为50
    uint32_t equip_btl_value = 0; //装备本身的战斗力
    uint32_t equip_add_btl_value = 0; //装备给玩家增加的战斗力
    for (int i = kAttrHead; i <= kAttrOtherEquipAttr; i++) {
        uint32_t equip_slot_id = GET_A((attr_type_t)i);
        if (!equip_slot_id) {
            continue;
        }
        item_t *item = player->package->get_mutable_item_in_slot(equip_slot_id);
        if (item == NULL) {
            continue;
        }

        if (!g_item_conf_mgr.is_equip(item->item_id)) {
            WARN_TLOG("P:%u equip_pos:%u item:%u is not equip",
                    player->userid, i, item->item_id);
            continue;
        }

        commonproto::item_optional_attr_t opt_attr;
        opt_attr.ParseFromString(item->opt_attr);
        equip_btl_value += opt_attr.equip_power();
        equip_add_btl_value += opt_attr.magic_power() + opt_attr.total_damage_rate();
        uint32_t elem_type = (uint32_t)(opt_attr.elem_type());
        if (item_conf_mgr_t::is_valid_elem_type(elem_type)) {//属性石加克制系数及抗性
            uint32_t rate = item_conf_mgr_t::get_equip_elem_damage_rate(opt_attr.level());
            //克制系数(百分比)
            elem_damage_percent += rate;
            if (GET_A(kAttrPlayerElemType) != elem_type) {
                SET_A(kAttrPlayerElemType, elem_type);
            }
            //属性石抗性数值
            attr_type_t anti_attr = AttrUtils::get_player_anti_attr_by_equip_elem(elem_type);
            attr_map[anti_attr] += item_conf_mgr_t::get_equip_anti_value(opt_attr.level());
        }
    
        //洗练第一条 主属性、隐藏属性、抗性
        if (opt_attr.quench_1_type()) {
            attr_type_t attr = AttrUtils::get_player_attr_by_quench_type(opt_attr.quench_1_type());
            attr_map[attr] += opt_attr.quench_1_value();
        }

        for (int j = 0; j < opt_attr.equip_attrs().attrs_size(); j++) {
            uint32_t type = opt_attr.equip_attrs().attrs(j).type();
            uint32_t value = opt_attr.equip_attrs().attrs(j).value();
            uint32_t quench_rate = 0;
            //洗练第二条 如果是附加主属性额外转化比
            if (opt_attr.quench_2_type() == commonproto::EQUIP_QUENCH_TYPE_2_EQUIP_MAIN_ATTR) {
                quench_rate = opt_attr.quench_2_value();
                if (quench_rate > 50) {//最多只有50%
                    quench_rate = 50;
                }
            }

            if (item_conf_mgr_t::is_valid_equip_add_attr(type)) {
                // 目前只有数量加成
                attr_type_t attr = AttrUtils::get_player_attr_by_equip_attr((equip_add_attr_t)type);
                attr_map[(uint32_t)attr] += (value * (100 + quench_rate) / 100.0);//洗练

            } else if (item_conf_mgr_t::is_valid_equip_add_percent_attr(type)) {
                // 千分比加成
                // TODO toby 封装
                if (type == (uint32_t)EQUIP_ADD_ATTR_PERCENT_ALL) {
                    uint32_t attrs[] = {
                        kAttrHpMax, kAttrNormalAtk, kAttrNormalDef, kAttrSkillAtk, kAttrSkillDef,
                        kAttrCrit, kAttrAntiCrit, kAttrHit, kAttrDodge, kAttrBreakBlock, kAttrBlock,
                        kAttrAntiWater, kAttrAntiFire, kAttrAntiGrass, kAttrAntiLight, kAttrAntiDark,
                        kAttrAntiGround, kAttrAntiForce, kAttrPlayerElemDamageRateWater,
                        kAttrPlayerElemDamageRateFire, kAttrPlayerElemDamageRateGrass,
                        kAttrPlayerElemDamageRateLight, kAttrPlayerElemDamageRateDark,
                        kAttrPlayerElemDamageRateGround, kAttrPlayerElemDamageRateForce
                    };

                    for (uint32_t i = 0; i < array_elem_num(attrs);i++) {
                        uint32_t add_value = attr_map[attrs[i]] * (value / 1000.0);
                        attr_map[attrs[i]] += (add_value * (100 + quench_rate) /100.0);//洗练
                    }
                } else {
                    attr_type_t attr = AttrUtils::get_player_attr_by_equip_attr((equip_add_attr_t)type);
                    uint32_t add_value = attr_map[(uint32_t)attr] * (value / 1000.0);
                    attr_map[(uint32_t)attr] +=  (add_value * (100 + quench_rate) /100.0);//洗练
                }
            }
        }

        //装备洗练第三条 如果是加属性
        if (opt_attr.quench_3_type() == commonproto::EQUIP_QUENCH_TYPE_3_ATTR) {
            attr_type_t attr = AttrUtils::get_player_attr_by_equip_attr((equip_add_attr_t)opt_attr.quench_3_id());
            attr_map[(uint32_t)attr] +=  opt_attr.quench_3_value();
        }
    }

    //装备评分榜
    if (GET_A(kAttrEquipBtlValue) != equip_btl_value) {
        SET_A(kAttrEquipBtlValue, equip_btl_value);
        if ((NOW() - GET_A(kAttrUpEquipBtlValueTm)) > UPDATE_POWER_TM_INTERVL) {
            RankUtils::rank_user_insert_score(
                    player->userid, player->create_tm,
                    (uint32_t)commonproto::RANKING_EQUIP_GRADE, 
                    0, equip_btl_value);
            SET_A(kAttrUpEquipBtlValueTm, NOW());
        }
    }

    if (GET_A(kAttrPlayerElemDamageRate) != elem_damage_percent) {
        SET_A(kAttrPlayerElemDamageRate, elem_damage_percent);
    }

    //计算刻印精灵给玩家转化的刻印属性
    for (int i = kAttrChisel1PetCreateTm; i <= kAttrChisel6PetCreateTm; i++) {
        uint32_t create_tm = GET_A(((attr_type_t)i));
        if (!create_tm) {
            continue;
        }
        Pet *pet = PetUtils::get_pet_in_loc(player, create_tm, PET_LOC_BAG);
        if (!pet) {
            continue;
        }
        const pet_conf_t* pet_conf = PetUtils::get_pet_conf(pet->pet_id());
        assert(pet_conf);
        uint32_t equip_pos = i - kAttrChisel1PetCreateTm + 1;
        if (!item_conf_mgr_t::is_valid_equip_body_pos(equip_pos)) {
            WARN_TLOG("P:%u equip_pos:%u invalid", player->userid, equip_pos);
            continue;
        }
        uint32_t equip_slot_id = GET_A(AttrUtils::get_player_attr_by_equip_pos((equip_body_pos_t)equip_pos));
        uint32_t default_item_id = item_conf_mgr_t::get_default_equip_item_id(equip_pos);

        const item_t *equip_item = player->package->get_const_item_in_slot(equip_slot_id);
        uint32_t trans_rate;
        attr_type_t trans_attr;
        const item_conf_t *item_conf = 0;
        uint32_t quality;

        uint32_t quench_2_type = 0;
        uint32_t quench_2_value = 0;
        if (equip_item) {//装备转化一级属性
            commonproto::item_optional_attr_t opt_attr;
            opt_attr.ParseFromString(equip_item->opt_attr);
            quality = opt_attr.level();
            item_conf = g_item_conf_mgr.find_item_conf(equip_item->item_id);
            quench_2_type = opt_attr.quench_2_type();
            quench_2_value = opt_attr.quench_2_value();
        } else {
            quality = 1; //default value;
            item_conf = g_item_conf_mgr.find_item_conf(default_item_id);
        }
        if (!item_conf) {
            continue;
        }
        trans_rate = item_conf_mgr_t::get_equip_trans_major_rate_by_quality(quality);
        battle_value_normal_type_t pet_attr;
        std::set<uint32_t> chisel_attrs_set;
        FOREACH(item_conf->chisel_attrs, it) {//刻印主属性
            pet_attr = AttrUtils::get_pet_attr_by_equip_chisel_attr(*it);
            trans_attr = AttrUtils::get_player_attr_by_pet_normal_attr(pet_attr);
            uint32_t value = pet->battle_value(pet_attr);
            value = value * trans_rate / 100.0;
            //洗练第二条的第二类 伙伴刻印主属性
            if (quench_2_type == commonproto::EQUIP_QUENCH_TYPE_2_PET_MAJOR_ATTR) {
                value = value * (100 + quench_2_value) / 100.0;
            }
            attr_map[(uint32_t)trans_attr] += value;
            chisel_attrs_set.insert((uint32_t)pet_attr);
        }
        //没刻印的主属性和二级属性
        trans_rate = item_conf_mgr_t::get_equip_trans_minor_rate_by_quality(quality);
        for(int i = kBattleValueNormalTypeHp; i <= kBattleValueNormalTypeSkillDef; i++) {
            if (chisel_attrs_set.count(i) != 0) {//已转化的主属性
                continue;
            }
            trans_attr = AttrUtils::get_player_attr_by_pet_normal_attr((battle_value_normal_type_t)i);
            uint32_t value = pet->battle_value(i);
            value = value * trans_rate / 100.0;
            //洗练第二条的第三类 伙伴没刻印的其余属性
            if (quench_2_type == commonproto::EQUIP_QUENCH_TYPE_2_PET_MINOR_ATTR) {
                value = value * (100 + quench_2_value) / 100.0;
            }
            attr_map[(uint32_t)trans_attr] += value;
        }
        //所有二级属性
        for (int i = kBattleValueHideTypeCrit; i <= kBattleValueHideTypeBreakBlock; i++) {
            battle_value_hide_type_t pet_attr = (battle_value_hide_type_t)i;
            attr_type_t trans_attr = AttrUtils::get_player_attr_by_pet_hide_attr(pet_attr);
            uint32_t value = pet->battle_value_hide(pet_attr);
            value = value * trans_rate / 100.0;
            //洗练第二条的第三类 伙伴没刻印的其余属性
            if (quench_2_type == commonproto::EQUIP_QUENCH_TYPE_2_PET_MINOR_ATTR) {
                value = value * (100 + quench_2_value) / 100.0;
            }
            attr_map[(uint32_t)trans_attr] += value;
        }
    }
	//计算转职加成 
	uint32_t cur_stage = GET_A(kAttrStage);
	uint32_t cur_prof = GET_A(kAttrCurProf);
	trans_prof_conf_t trans_prof_conf;
	if (cur_stage > 0) {
		//uint32_t ret = g_trans_prof_conf_manager.get_conf_by_prof_and_stage(cur_stage, cur_prof, trans_prof_conf);
		uint32_t ret = g_trans_prof_conf_manager.get_conf_by_prof_and_stage(cur_prof, cur_stage, trans_prof_conf);
		if (ret) {
            WARN_TLOG("trans_prof invalid : stage = %u, err = %u", cur_stage, ret);
		}
		else {
            attr_map[(uint32_t)kAttrHpMax] += trans_prof_conf.hp;
			attr_map[(uint32_t)kAttrNormalAtk] += trans_prof_conf.normal_atk;	
			attr_map[(uint32_t)kAttrNormalDef] += trans_prof_conf.normal_def;	
			attr_map[(uint32_t)kAttrSkillAtk] += trans_prof_conf.skill_atk;	
			attr_map[(uint32_t)kAttrSkillDef] += trans_prof_conf.skill_def;	
			attr_map[(uint32_t)kAttrCrit] += trans_prof_conf.crit;	
			attr_map[(uint32_t)kAttrAntiCrit] += trans_prof_conf.anti_crit;	
			attr_map[(uint32_t)kAttrHit] += trans_prof_conf.hit;	
			attr_map[(uint32_t)kAttrDodge] += trans_prof_conf.dodge;	
			attr_map[(uint32_t)kAttrBlock] += trans_prof_conf.block;	
			attr_map[(uint32_t)kAttrBreakBlock] += trans_prof_conf.break_block;	
			if (cur_stage > 2) {
				//TODO
				//加技能
			}
		}
	}

    // 家族加成
    FamilyUtils::family_attr_addition(player, attr_map);
    // 团队加成
    PetUtils::pet_group_addition(player, attr_map);
    // 坐骑翅膀加成
    EquipUtils::cultivate_equip_addition(player, attr_map);

	//其他杂项加成
	PlayerUtils::calc_extra_add_attr(player, attr_map);
    
    //非战斗中更新能量
	if (player->temp_info.dup_id == 0) {
		attr_map[kAttrTp] =attr_map[kAttrMaxTp];
	}

    std::vector<attr_data_info_t> attr_vec;
    FOREACH(attr_map, it) {
        attr_data_info_t attr;
        attr.type = it->first;
        attr.value = it->second;
        attr_vec.push_back(attr);
    }
    AttrUtils::set_attr_value(player, attr_vec, false, reason);

    uint32_t cur_battle_value = 
        PlayerUtils::calc_attr_to_battle_value(player, attr_map) + equip_add_btl_value;
    
    //NOTI(singku)如果玩家还没有完成新手副本 则不更新任何排行榜
    if (!GET_A(kAttrGuideFinished)) {
        return 0;
    }

    //如果当前战斗力超过最高战力，则要更新到排行榜
    PlayerUtils::update_player_power_to_rank(player, cur_battle_value);

	//更新限时战力排行

    if (GET_A(kAttrBattleValueRecord) < cur_battle_value) {//更新最高战力
        SET_A(kAttrBattleValueRecord, cur_battle_value);
    }

    AttrUtils::set_single_attr_value(player, kAttrCurBattleValue, cur_battle_value, false, reason);

	if (cur_battle_value > GET_A(kAttrBattleValueRecord)) {
		SET_A(kAttrBattleValueRecord, cur_battle_value);
		//如果玩家在运宝，更新运宝玩家的战力
		PlayerUtils::update_player_escort_power(player->userid, player->create_tm, cur_battle_value);
		//成就监听
		//ACH_01_INC_POWER
	}

    // 更新家族成员战力
    uint32_t family_id = GET_A(kAttrFamilyId);
    if (FamilyUtils::is_valid_family_id(family_id)) {
        dbproto::family_member_table_t up_info;
        up_info.Clear();
        up_info.set_family_id(family_id);
        up_info.set_userid(player->userid);
        up_info.set_u_create_tm(player->create_tm);
        up_info.set_battle_value(cur_battle_value);
        FamilyUtils::update_family_member_info(0, up_info, dbproto::DB_UPDATE_NO_INSERT);
    }


    return 0;
}

uint32_t PlayerUtils::calc_player_equip_cur_battle_value_by_player(player_t* player) 
{
    uint32_t equip_battle_value_sum = 0;
    for(int i = kAttrHead; i <= kAttrFoot; i++) {
        uint32_t slot_id = GET_A((attr_type_t)i);
        if (!slot_id) {
            continue;
        }
        const item_t* item = player->package->get_const_item_in_slot(slot_id);
        if(g_item_conf_mgr.is_equip(item->item_id)) {
            commonproto::item_optional_attr_t opt_attr;
            opt_attr.ParseFromString(item->opt_attr);
            if(!opt_attr.has_level()) {
                return -1;
            }
            for(int j=0; j<opt_attr.equip_attrs().attrs_size(); j++) {
                commonproto::attr_data_t attr_data = opt_attr.equip_attrs().attrs(i);
                equip_battle_value_sum +=  attr_data.value();
            }				
        }
    }
    return equip_battle_value_sum;
}

uint32_t PlayerUtils::calc_attr_to_battle_value(
        player_t *player, std::map<uint32_t, uint32_t> &attr_map)
{
    std::map<uint32_t, float> attr_ratio;
    attr_ratio[kAttrHpMax] = 0.5;
    attr_ratio[kAttrNormalAtk] = 1.5;
    attr_ratio[kAttrNormalDef] = 3;
    attr_ratio[kAttrSkillAtk] = 1.5;
    attr_ratio[kAttrSkillDef] = 3;
    attr_ratio[kAttrCrit] = 1;
    attr_ratio[kAttrAntiCrit] = 1;
    attr_ratio[kAttrHit] = 1;
    attr_ratio[kAttrDodge] = 1;
    attr_ratio[kAttrBlock] = 1;
    attr_ratio[kAttrBreakBlock] = 1;
    attr_ratio[kAttrAntiWater] = 2;
    attr_ratio[kAttrAntiFire] = 2;
    attr_ratio[kAttrAntiGrass] = 2;
    attr_ratio[kAttrAntiLight] = 2;
    attr_ratio[kAttrAntiDark] = 2;
    attr_ratio[kAttrAntiGround] = 2;
    attr_ratio[kAttrAntiForce] = 2;
    uint32_t battle_value = 0;
    FOREACH(attr_map, iter) {
        if (attr_ratio.count(iter->first) > 0) {
            battle_value += attr_ratio[iter->first] * iter->second;
        }
    }

    return battle_value;
}

uint32_t PlayerUtils::player_equip_level_up(player_t* player, uint32_t equip_slot_id, 
        enum onlineproto::sync_item_reason_t reason) 
{	
	uint32_t ret = 0; 
	item_t* item = player->package->get_mutable_item_in_slot(equip_slot_id);
	if(!item) {
		return cli_err_item_not_exist;
	}

    if (!g_item_conf_mgr.is_equip(item->item_id)) {
        return cli_err_equip_not_found;
    }
    commonproto::item_optional_attr_t item_optional_attr;
    item_optional_attr.ParseFromString(item->opt_attr);
    uint32_t cur_level = item_optional_attr.level();
    if(cur_level >= EQUIP_LEVEL_MAX) {
        return cli_err_equip_level_max;
    }
    const item_conf_t* item_conf = g_item_conf_mgr.find_item_conf(item->item_id);
    assert(item_conf);
    attr_type_t slot_attr = AttrUtils::get_player_attr_by_equip_pos(item_conf->equip_body_pos);

    uint32_t target_level = cur_level + 1;
    uint32_t target_item_id = 0;
    if (target_level > (uint32_t)item_conf->quality_max) { //要变颜色
        target_item_id = item_conf->next_quality_item;
        if (!g_item_conf_mgr.find_item_conf(target_item_id)) {
            return cli_err_equip_level_max;
        }
    } else {
        target_item_id = item->item_id;
    }
    uint32_t consume_gold = cur_level * 1000;
    
    //升级固定消耗金币 道具 同品级的物品
    //金币是否足够
    if (!AttrUtils::is_player_gold_enough(player, consume_gold)) {
        return cli_err_gold_not_enough;
    }

    //如果要消耗和自己一模一样的物品 则下面这个记录是否消耗
    bool need_reduce_same_item = false;
    uint32_t need_same_item_cnt = 0;
    //道具是否满足
    std::vector<reduce_item_info_t> reduce_vec;
    FOREACH(item_conf->material, it) {
        uint32_t consume_id = it->first;
        uint32_t consume_cnt = it->second;
        if (consume_id == item->item_id) { 
            need_same_item_cnt += consume_cnt;
            need_reduce_same_item = true;
            //如果要消耗自己一样的物品
            if (item->using_count == 0) {
                //如果本物品使用中的数量为0 则可用数量要>=consume_cnt+1
                uint32_t usable_cnt = player->package->get_total_usable_item_count(consume_id);
                if (usable_cnt < consume_cnt + 1) {
                    return cli_err_no_enough_item_num;
                }
            } else {
                //如果本物品在使用中 则可用数量要>=consume_cnt
                uint32_t usable_cnt = player->package->get_total_usable_item_count(consume_id);
                if (usable_cnt < consume_cnt) {
                    return cli_err_no_enough_item_num;
                }
            }
        } else {
            if (player->package->get_total_usable_item_count(consume_id) < consume_cnt) {
                return cli_err_no_enough_item_num;
            }
            reduce_item_info_t reduce;
            reduce.item_id = consume_id;
            reduce.count = consume_cnt;
            reduce_vec.push_back(reduce);
        }
    }

    //消耗固定金币
    AttrUtils::sub_player_gold(player, consume_gold, "装备合成");

    //消耗进阶道具
    //先消耗和自己一样的id的物品
    if (need_reduce_same_item) {
        std::vector<item_t*> same_item_vec;
        player->package->get_items_by_id(item->item_id, same_item_vec);
        FOREACH(same_item_vec, it) {
            item_t *same_item = *it;
            if (same_item == item) {//自己不能被消耗
                continue;
            }
            uint32_t usable_cnt = same_item->count - same_item->using_count;
            if (need_same_item_cnt <= usable_cnt) {
                reduce_item_by_slot_id(player, same_item->slot_id, need_same_item_cnt);
                break;
            } else {
                reduce_item_by_slot_id(player, same_item->slot_id, usable_cnt);
                need_same_item_cnt -= usable_cnt;
            }
        }
    }
    //消耗其他的
    if (reduce_vec.size()) {
        ret = swap_item_by_item_id(player, &reduce_vec, 0);
        if (ret) {
            return ret;
        }
    }
    

    //NOTI(singku) online内部存储的slot_id是根据item_id和db的slot_id组合成的64位slot_id
    //升级后变了颜色之后 还需要更换物品ID这里更换物品ID后得更新内部slot_id
    item_optional_attr.set_level(target_level);
    player->package->update_item_id(item, target_item_id);
    item_optional_attr.SerializeToString(&(item->opt_attr));
    //初始化各项信息
    init_equip_attr(item);

    std::vector<item_t> item_vec;
    item_vec.push_back(*item);
    db_item_change(player, item_vec, reason);

    if (item->using_count) { //在使用中
        calc_player_battle_value(player);
        SET_A(slot_attr, item->slot_id);
    }

	return ret;
}

int PlayerUtils::gm_debug_add_all_pet(player_t *player, uint32_t arg1, uint32_t arg2)
{
    const std::map<uint32_t, pet_conf_t>& pet_conf = g_pet_conf_mgr.const_pet_conf_map();

    uint32_t i = 0;
    FOREACH(pet_conf, iter) {
        if (iter->first >= 1001) {
            PetUtils::create_pet(player, iter->first, arg1, false);
        }
        i++;
    }

    return 0;
}

int PlayerUtils::gm_debug_fini_task(player_t *player, uint32_t arg1, uint32_t arg2)
{
    // 从配表和内存中读取信息    
    const task_conf_t* task_conf =  g_task_conf_mgr.find_task_conf(arg1);
    if (task_conf == NULL) {
        return 0;
    }
    uint32_t step_count = task_conf->step_count; 
    std::vector<task_t> task_vec;
    task_t task_info;
    task_info.task_id = arg1;
    for (uint32_t i = 0; i < step_count;i++) {
        step_t step_info;
        task_info.step_list.push_back(step_info);
    }
    task_vec.push_back(task_info);

    //当可以领取奖励时
    for (uint32_t i = 1; i <= step_count + 1; i++ ){
        if (TaskUtils::is_accept_task_bonus(player, arg1, step_count)) {
            //读奖励列表
            std::vector<std::string> args_list = split(task_conf->bonus_id, ',');
            std::vector<uint32_t> prize_list;
            FOREACH(args_list, it) {
                prize_list.push_back(atoi_safe((*it).c_str()));
            }

            //设置标志位
            for (uint32_t i = 0; i < task_vec.size(); i++) {
                task_vec[i].bonus_status = kTaskBonusAccept;
                task_vec[i].done_times += 1;
            }

            //发奖
            onlineproto::sc_0x0112_notify_get_prize msg;
            int ret = 0;
            for (uint32_t i = 0; i < prize_list.size(); i++) {
                ret = transaction_proc_prize(player, prize_list[i], msg);
                if (ret) {
                    return ret;
                }
            }
        }

        //更新任务信息到内存和db
        int ret = TaskUtils::update_task_and_sync_db(player, task_vec, i, false);
        if (ret != 0) {
            return send_err_to_player(player, player->cli_wait_cmd, ret); 
        }
    }

    onlineproto::sc_0x0403_complete_task noti_out_;
    noti_out_.set_task_id(task_conf->task_id);    
    send_msg_to_player(player, cli_cmd_cs_0x0403_complete_task, noti_out_);
    return 0;
}


int PlayerUtils::gm_debug_set_family_exp(player_t *player, uint32_t arg1, uint32_t arg2)
{
    if (player == NULL) {
        return 0;
    }

    if (!FamilyUtils::is_valid_family_id(arg1)) {
        return cli_err_family_not_exist;
    }

    dbproto::cs_family_update_info   db_update_family_info_in_;
    db_update_family_info_in_.Clear();
    dbproto::family_info_table_t *update_data = db_update_family_info_in_.mutable_family_info();
    update_data->set_family_id(arg1);
    update_data->set_construct_value(arg2);
    FamilyUtils::update_family_info(NULL, *update_data, dbproto::DB_UPDATE_NO_INSERT);
    return 0;
}

int PlayerUtils::gm_debug_world_boss(
        player_t *player, uint32_t arg1, uint32_t arg2, std::string &ret_str)
{
    ret_str = "命令执行没效果";

    uint32_t nowtime = NOW();
    bool ready_flag = TimeUtils::is_time_valid(nowtime, 3, 1) || 
        TimeUtils::is_time_valid(nowtime, 3, 3)  || 
        TimeUtils::is_time_valid(nowtime, 3, 5);

    bool fight_flag = TimeUtils::is_time_valid(nowtime, 3, 2) || 
        TimeUtils::is_time_valid(nowtime, 3, 4)  || 
        TimeUtils::is_time_valid(nowtime, 3, 6);

    world_boss_dup_info_t *dup_info = g_world_boss_mgr.get_world_boss_dup_info(
            commonproto::WORLD_BOSS_DUP_ID_1);
    if (dup_info == NULL) {
        g_world_boss_mgr.init(commonproto::WORLD_BOSS_DUP_ID_1);
        dup_info = g_world_boss_mgr.get_world_boss_dup_info(commonproto::WORLD_BOSS_DUP_ID_1);
        if (dup_info == NULL) {
		    return cli_err_world_boss_info_not_exist;
        }
    }

    uint32_t dup_id = commonproto::WORLD_BOSS_DUP_ID_1;
    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
    if (dup == NULL) {
        ERROR_TLOG("gm world_boss_trig,dup id not exist,dup_id=[%u]", dup_id);
        return cli_err_duplicate_id_not_found;
    } 

    onlineproto::sc_0x023E_noti_world_boss_dup_status noti_msg;
    if (arg1 == 1) {
        // 触发世界boss开始
        if (dup_info->status == commonproto::WORLD_BOSS_DUP_CLOSED) {
            // 触发世界boss副本事件开始
            g_world_boss_mgr.clear_dup_record(dup_id);
            dup_info->status = commonproto::WORLD_BOSS_DUP_FIGHT;
            dup_info->start_time = NOW();

            noti_msg.set_status((commonproto::world_boss_dup_status_t)dup_info->status);
            g_player_manager->send_msg_to_all_player(cli_cmd_cs_0x023E_noti_world_boss_dup_status, noti_msg);

            player_t tmp_player;
            tmp_player.userid = 0;
            battleproto::cs_battle_duplicate_trig btl_in_;
            btl_in_.set_dup_id(dup_id);
            btl_in_.set_map_id(dup->map_id);
            btl_in_.set_trig_type(commonproto::WORLD_BOSS_TRIG_OPEN);
            DupUtils::send_to_battle(&tmp_player, btl_cmd_duplicate_trig, btl_in_, NO_WAIT_SVR);

            TRACE_LOG("world boss gm trig, ready_flag:%u, fight_flag:%u,dup_status:%u", ready_flag, fight_flag, dup_info->status);
            ret_str = "世界boss活动开始了";
            noti_gm_debug_msg(player, ret_str);
        } else {
            ret_str += " 世界boss活动已经开始了";
            noti_gm_debug_msg(player, ret_str);
        }
    } else {
        // 触发世界boss结束
        if (dup_info->status == commonproto::WORLD_BOSS_DUP_FIGHT) {
            dup_info->status = commonproto::WORLD_BOSS_DUP_CLOSED;
            noti_msg.set_status((commonproto::world_boss_dup_status_t)dup_info->status);
            g_player_manager->send_msg_to_all_player(cli_cmd_cs_0x023E_noti_world_boss_dup_status, noti_msg);

            // 触发奖励
            g_world_boss_mgr.give_world_boss_reward(commonproto::WORLD_BOSS_DUP_ID_1);

            // 触发世界boss副本事件结束
            player_t tmp_player;
            tmp_player.userid = 0;
            battleproto::cs_battle_duplicate_trig btl_in_;
            btl_in_.set_dup_id(dup_id);
            btl_in_.set_map_id(dup->map_id);
            btl_in_.set_trig_type(commonproto::WORLD_BOSS_TRIG_SHUTDOWN);
            DupUtils::send_to_battle(&tmp_player, btl_cmd_duplicate_trig, btl_in_, NO_WAIT_SVR);

            g_world_boss_mgr.clear_dup_record(commonproto::WORLD_BOSS_DUP_ID_1);
            dup_info->end_time = NOW();

            TRACE_LOG("world boss gm trig, ready_flag:%u, fight_flag:%u,dup_status:%u", ready_flag, fight_flag, dup_info->status);

            ret_str = "世界boss活动结束了";
            noti_gm_debug_msg(player, ret_str);
        } else {
            ret_str += " 世界boss活动已经结束了";
            noti_gm_debug_msg(player, ret_str);
        }
    }

    return 0;
}

int PlayerUtils::player_say_cmd(player_t *player, const string &org_cmd, string &ret_content)
{
    string str_arg1, str_arg2;
    uint32_t arg1, arg2;

    ret_content.clear();
    int err = 0;
    string cmd;
    std::vector<string> cmd_args = split(org_cmd, ' ');
    if (!cmd_args.size()) {
        goto ERR_RT;
    }

    cmd = cmd_args[0];
    if (cmd_args.size() >= 3) {
        str_arg1 = cmd_args[1];
        str_arg2 = cmd_args[2];
        arg1 = atoi(str_arg1.c_str());
        arg2 = atoi(str_arg2.c_str());

    } else if (cmd_args.size() >= 2) {
        str_arg1 = cmd_args[1];
        arg1 = atoi(str_arg1.c_str());
    }

    if (cmd == "add_item") {
        if (cmd_args.size() < 3) {
            goto ERR_RT;
        }
        err = add_single_item(player, arg1, arg2);
        ret_content.assign(org_cmd);

    } else if (cmd == "add_pet") {
        if (cmd_args.size() < 3) {
            goto ERR_RT;
        }
		bool succ = PetUtils::check_can_create_pet(player, arg1);
		//已有精灵转化为对应碎片vince TODO
		if(!succ){
			const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(arg1);
			if (!pet_conf) return 0;
			uint32_t item = pet_conf->talent_item;
			int32_t talent_level =pet_conf->born_talent;
			talent_level = talent_level >= 1 ? talent_level : 1;
			/* 不同星级对应的碎片数量 */
			uint32_t trans_num[] = {
				kOneTLevelItemCnt,    
				kTwoTLevelItemCnt, 
				kThreeTLevelItemCnt, 
				kFourTLevelItemCnt,
				kFiveTLevelItemCnt, 
			};

			add_item_info_t add_item;
			add_item.item_id = item;
            add_item.expire_time = 0;
			add_item.count = trans_num[talent_level - 1];

			std::vector<add_item_info_t> add_vec;
			add_vec.push_back(add_item);
			// 扣除/添加物品
			err = swap_item_by_item_id(player, 
					0, &add_vec, false, 
                    onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);
		}else {
			err = PetUtils::create_pet(player, arg1, arg2, false, 0);
			ret_content.assign(org_cmd);
		}

    } else if (cmd =="del_pet") {
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        if (player->pet_id_pets->count(arg1) == 0) {
            goto ERR_RT;
        }
        std::vector<Pet*> pet_vec = (player->pet_id_pets->find(arg1))->second;
        FOREACH(pet_vec, it) {
            Pet *pet = *it;
            err = PetUtils::del_pet(player, pet);
        }
        ret_content.assign(org_cmd);

    } else if (cmd == "add_all_pet") {
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        gm_debug_add_all_pet(player, arg1, arg2);
        ret_content.assign(org_cmd);
    } else if (cmd == "del_all_pet") {
        FOREACH(*(player->pet_id_pets), iter) {
            std::vector<Pet*> pet_vec = iter->second;
            FOREACH(pet_vec, it) {
                Pet *pet = *it;
                err = PetUtils::del_pet(player, pet);
            }
        }
        ret_content.assign(org_cmd);
    } else if (cmd == "add_rune") {
        if (cmd_args.size() < 3) {
            goto ERR_RT;
        }
        if (arg2 > kMaxRuneLv) {
            arg2 = kMaxRuneLv;
        }
        err = RuneUtils::add_rune(player, arg1, arg2);
        ret_content.assign(org_cmd);

    } else if (cmd == "add_gold") {
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        AttrUtils::add_player_gold(player, arg1, false, "GM手加");
        ret_content.assign(org_cmd);

    } else if (cmd == "add_diamond") {
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        ADD_A(kAttrDiamond, arg1);
        ret_content.assign(org_cmd);

    } else if (cmd == "get_attr") {
        if (cmd_args.size() < 3) {
            goto ERR_RT;
        }
        if (arg1 > arg2) {
            goto ERR_RT;
        }
        std::ostringstream ostr;
        ostr.str("");
        for (uint32_t i = arg1; i <= arg2; i++) {
            ostr << i << ":";
            ostr << GET_A((attr_type_t)i) << " ";
        }
        ret_content.assign(ostr.str());

    } else if (cmd == "set_attr") {
        if (cmd_args.size() < 3) {
            goto ERR_RT;
        }
        if (arg1 != kAttrExp) {
            SET_A((attr_type_t)arg1, arg2);
            PlayerUtils::calc_player_battle_value(player);
        } else {
            SET_A(kAttrExp, 0);
            uint32_t real_add;
            PlayerUtils::add_player_exp(player, arg2, &real_add);
        }
        ret_content.assign(org_cmd);

    } else if (cmd == "del_task") {
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        TaskUtils::del_task_record(player, arg1);
        ret_content.assign(org_cmd);

    } else if (cmd == "add_prize" ) {
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        onlineproto::sc_0x0112_notify_get_prize noti_prize_msg;
        transaction_proc_prize(player, arg1, noti_prize_msg);
        send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_prize_msg);
        ret_content.assign(org_cmd);

    } else if (cmd == "add_tran_card") {
        if (cmd_args.size() < 3) {
            goto ERR_RT;
        }

		err = player->m_tran_card->add_tranCard(player, arg1, arg2);
        ret_content.assign(org_cmd);

    } else if (cmd == "set_level") {
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        if (arg1 > PLAYER_MAX_LEVEL) {
            goto ERR_RT;
        }
        SET_A(kAttrLv, arg1);
        PlayerUtils::calc_player_battle_value(player);
        ret_content.assign(org_cmd);

    } else if (cmd == "fini_task") {
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        
        gm_debug_fini_task(player, arg1, arg2);
        ret_content.assign(org_cmd);
    } else if (cmd == "world_boss_trig") {
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        std::string ret_str = org_cmd;
        gm_debug_world_boss(player, arg1, arg2, ret_str);
        ret_content.assign(ret_str);

    } else if (cmd == "set_svip") {
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        if (arg1 == 1) {
            //设置年费VIP
        } else {
            //去掉年费VIP
        }
    
    } else if (cmd == "fini_all_dup") {//完成所有副本
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        uint32_t star = arg1 > 3 ?3 :arg1;

        const std::map<uint32_t, duplicate_t> &dup_map = g_duplicate_conf_mgr.const_dup_map();
        FOREACH(dup_map, it) {
            uint32_t dup_id = it->first;
            SET_A(AttrUtils::get_duplicate_pass_time_attr(dup_id), NOW());
            SET_A(AttrUtils::get_duplicate_best_time_attr(dup_id), 10);
            SET_A(AttrUtils::get_duplicate_best_star_attr(dup_id), star);
            SET_A(AttrUtils::get_duplicate_last_play_time_attr(dup_id), NOW());
        }
    
    } else if (cmd == "fini_dup") {
        if (cmd_args.size() < 3) {
            goto ERR_RT;
        }
        uint32_t dup_id = arg1;
        uint32_t star = arg2 > 3 ?3 :arg1;
        if (!g_duplicate_conf_mgr.duplicate_exist(dup_id)) {
            goto ERR_RT;
        }

        SET_A(AttrUtils::get_duplicate_pass_time_attr(dup_id), NOW());
        SET_A(AttrUtils::get_duplicate_best_time_attr(dup_id), 10);
        SET_A(AttrUtils::get_duplicate_best_star_attr(dup_id), star);
        SET_A(AttrUtils::get_duplicate_last_play_time_attr(dup_id), NOW());
    
    } else if(cmd == "set_item_expire") {
        const item_conf_t *item_conf = g_item_conf_mgr.find_item_conf(arg1);
        if (!item_conf) {
            return cli_err_item_not_exist;
        }
        std::vector<item_t*> item_list;
        player->package->get_items_by_id(arg1, item_list);

        std::vector<item_t> change_item_vec;
        FOREACH(item_list, iter) {
            (*iter)->expire_time = NOW() + arg2;
            change_item_vec.push_back(*(*iter));
        }

        int ret = db_item_change(
                player, change_item_vec, onlineproto::SYNC_REASON_NONE, true);
        if (ret) {
            return ret;
        }
    } else if (cmd == "set_family_exp"){
        if (cmd_args.size() < 2) {
            goto ERR_RT;
        }
        
        int ret = gm_debug_set_family_exp(player, arg1, arg2);
        if (ret) {
            return ret;
        }

        ret_content.assign(org_cmd);
    } else if (cmd == "fini_all_task") {
        // 0 所有任务 1 主线任务 2 支线任务
        if (cmd_args.size() < 1) {
            goto ERR_RT;
        }

        const std::map<uint32_t, task_conf_t> &task_map = g_task_conf_mgr.const_task_conf_map();
        FOREACH(task_map, iter) {
            if (arg1 == 0 || iter->second.type == arg1) {
                gm_debug_fini_task(player, iter->second.task_id, 0);
            }
        }
        
        ret_content.assign(org_cmd);
    } else if (cmd == "add_title"){
		if (cmd_args.size () < 2) {
			goto ERR_RT;
		}
		title_info_t title_info;
		title_info.title_id = arg1;
		title_info.get_time = NOW();
		err = player->title->add_one_title(player, title_info, true);

    } else if (cmd == "erase_pass_dup_record") {
		const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(arg1);
		if (dup == NULL) {
			goto ERR_RT;
		}
		DupUtils::clean_dup_pass_record(player, arg1);

	} else {
        goto ERR_RT;
    }


ERR_RT:
    return err;
}

int PlayerUtils::update_user_raw_data(
		uint32_t userid, uint32_t create_tm, dbproto::user_raw_data_type_t type,
		const google::protobuf::Message& message, const std::string &buff_id)
{
	dbproto::cs_user_raw_data_update db_user_raw_data;
	dbproto::raw_data_table_t* buf_ptr = db_user_raw_data.mutable_raw_data(); 
	buf_ptr->set_userid(userid);
	buf_ptr->set_u_create_tm(create_tm);
	buf_ptr->set_rawdata_type((uint32_t)type);
	buf_ptr->set_rawdata_id(buff_id);
	std::string pkg;
	message.SerializeToString(&pkg);
	buf_ptr->set_rawdata(pkg);

	return g_dbproxy->send_msg(
		NULL, userid, create_tm,
		db_cmd_user_raw_data_update, db_user_raw_data);
}

int PlayerUtils::get_user_raw_data(
		player_t* player, dbproto::user_raw_data_type_t type, const std::string buff_id)
{
	dbproto::cs_user_raw_data_get db_user_raw_data;
	db_user_raw_data.set_type(type);
	db_user_raw_data.set_buff_id(buff_id);
	return g_dbproxy->send_msg(player, player->userid, player->create_tm,
		db_cmd_user_raw_data_get, db_user_raw_data);
}

int PlayerUtils::delete_user_raw_data(uint32_t userid, uint32_t create_tm, 
            dbproto::user_raw_data_type_t type, const std::string &buff_id)
{
	dbproto::cs_user_raw_data_del db_msg;
    db_msg.set_type(type);
    db_msg.set_buff_id(buff_id);
	return g_dbproxy->send_msg(0, userid, create_tm,
		db_cmd_user_raw_data_del, db_msg);
}

uint32_t PlayerUtils::get_chisel_pet_cnt(player_t *player)
{
    uint32_t cnt = 0;
    for (int i = kAttrChisel1PetCreateTm; i <= kAttrChisel6PetCreateTm; i++) {
        if (GET_A((attr_type_t)i) > 0 ) {
            cnt++;
        }
    }
    return cnt;
}

int PlayerUtils::sync_player_buff(player_t *player)
{
    onlineproto::sc_0x0141_sync_buff sync_msg;
    commonproto::user_buffs_t info;
    DataProtoUtils::pack_player_buff(player, &info);
    update_user_raw_data(player->userid, player->create_tm, dbproto::BUFF_INFO, info, "0");
    sync_msg.mutable_buffs()->CopyFrom(info);
    return send_msg_to_player(player, cli_cmd_cs_0x0141_sync_buff, sync_msg);
}

int PlayerUtils::add_player_buff(player_t *player, uint32_t buff_id)
{
    const buff_conf_t *buff_conf = g_buff_conf_mgr.find_buff_conf(buff_id);
    uint32_t over_type_id = buff_conf->over_type_id;
    player->buff_id_map->erase(over_type_id);
    player->buff_id_map->insert(make_pair(over_type_id, buff_id));
    return 0;
}

int PlayerUtils::noti_gm_debug_msg(player_t *player, std::string &content)
{
    if (!g_server_config.use_gm) {
        return 0;
    }

    onlineproto::sc_0x060A_say noti_msg;
    noti_msg.set_content(content);
    noti_msg.set_type(onlineproto::SAY_TYPE_MAP);
    if (player == NULL) {
        std::vector<player_t*> player_list;
        g_player_manager->get_player_list(player_list);
        for (uint32_t i = 0; i < player_list.size(); i++) {
            player_t* player = player_list[i]; 
            noti_msg.set_userid(player->userid);
			noti_msg.set_create_tm(player->create_tm);
            noti_msg.set_name(player->nick);
            send_msg_to_player(player,cli_cmd_cs_0x060A_say, noti_msg);
        }
    } else {
        noti_msg.set_userid(player->userid);
		noti_msg.set_create_tm(player->create_tm);
        noti_msg.set_name(player->nick);
        send_msg_to_player(player, cli_cmd_cs_0x060A_say, noti_msg);
    }

    return 0;
}

uint32_t PlayerUtils::get_equip_skill_num(player_t *player)
{
    uint32_t num = 0;
    for (uint32_t i = 0 ; i < commonproto::MAX_SKILL_NUM; i++) {
        uint32_t skill_id = GET_A((attr_type_t)(kAttrSkill1 + i));
        const skill_conf_t *skill = g_skill_conf_mgr.find_skill_conf(skill_id);
        if (skill != NULL) {
            ++num;
        }
    }

    return num;
}

uint32_t PlayerUtils::get_chisel_pet_num(player_t *player)
{
    uint32_t num = 0;
    for (uint32_t i = 0 ; i < kMaxChiselPos; i++) {
        uint32_t create_tm = GET_A((attr_type_t)(kAttrChisel1PetCreateTm + i));
        if (create_tm > 0) {
            ++num;
        }
    }

    return num;
}

uint32_t PlayerUtils::obj_hp_add_common(uint32_t max_hp, uint32_t btl_value)
{
    // if (ARENA_COF1 > btl_value) {
        // const double ARENA_BTL_PARA = (ARENA_COF1 - btl_value) / ARENA_COF2;
        // uint32_t arena_hp = ARENA_BTL_PARA * max_hp;
        // if (arena_hp > max_hp) {
            // max_hp = arena_hp;
        // }
    // }
	uint32_t enlarge_hp = 20 * max_hp;
	if (enlarge_hp > max_hp) {
		max_hp = enlarge_hp;
	}
    return max_hp;
}

uint32_t PlayerUtils::send_mail_notify_home_pets_found_item(player_t* player)
{
	std::vector<Pet*> pets_vec;
	uint32_t pass_time = 4 * 3600;
	if (is_gold_vip(player)) {
		pass_time = 2 * 3600;
	} else if (is_silver_vip(player)) {
		pass_time = 3 * 3600;
	}
	PetUtils::get_exercise_pets(player, pets_vec, pass_time);
	if (!pets_vec.empty()) {
		new_mail_t new_mail;
		new_mail.sender.assign("系统邮件");
		new_mail.title.assign("小屋伙伴找到物品");
		new_mail.content.assign("您有伙伴在小屋寻宝过程中找到物品，"
				"快去看看吧\n<a href='event:home'><u><font color='#5cda08'>我要进入小屋</font></u></a>");
		MailUtils::add_player_new_mail(player, new_mail);
	}
	return 0;
}

uint32_t PlayerUtils::leave_current_home(player_t* player, bool need_back)
{
	role_info_t orig_host = player->home_data->at_whos_home();
	if (orig_host.userid && orig_host.u_create_tm) {
		homeproto::cs_exit_home home_msg;
		home_msg.set_userid(player->userid);
		home_msg.set_create_tm(player->create_tm);
		if (need_back) {
			g_dbproxy->send_msg(
					player, orig_host.userid, orig_host.u_create_tm, 
					home_cmd_exit_home, home_msg, player->userid);
		} else {
			g_dbproxy->send_msg(
					0, orig_host.userid, orig_host.u_create_tm, 
					home_cmd_exit_home, home_msg, player->userid);
		}
		player->home_data->set_at_home(ROLE(0, 0));
	}
	return 0;
}

//发放封测系列的奖励；后续要屏蔽此函数
uint32_t PlayerUtils::recv_mail_get_test_prize(player_t* player)
{
	if (!TimeUtils::is_current_time_valid(TM_CONF_KEY_TEST_GIFT, 0)) {
		return 0;
	}
	if (GET_A(kAttrTestInPowerRankRangeFlag)) {
		return 0;
	}
	//一次封测中该玩家战力排名是否有在奖励范围内
	const player_power_rank_conf_t* ptr = NULL;
	ptr = g_ply_power_rank_conf_mgr.find_player_info_in_1st(player->userid);
	if (ptr) {
		std::string title = "领取展翅首测战力前十的奖励";
		std::string content = "展翅首测：当时战力:" + 
			boost::lexical_cast<string>(ptr->power_value) + 
			"\t 排名：" + boost::lexical_cast<string>(ptr->power_rank);
		PlayerUtils::generate_new_mail(player, title, content, ptr->prize_id);
	}
	ptr = NULL;
	ptr = g_ply_power_rank_conf_mgr.find_player_info_in_2nd(player->userid);
	if (ptr) {
		std::string title = "领取青春内测战力前十的奖励";
		std::string content = "青春内测：当时战力:" + 
			boost::lexical_cast<string>(ptr->power_value) + 
			"\t 排名：" + boost::lexical_cast<string>(ptr->power_rank);
		PlayerUtils::generate_new_mail(player, title, content, ptr->prize_id);
	}

	if (g_joined_test_uid_conf_mgr.is_uid_exist_in_1st(player->userid)) {
		uint32_t first_prize_id = 2015;
		std::string title = "领取参与展翅首测的奖励";
		std::string content = "感谢您参与赤瞳之刃展翅首测，为了感激您对赤瞳之刃的关注和支持，赤瞳特为你备上首测大礼，祝您在赤瞳之刃先声夺人，一斩必杀！";
		PlayerUtils::generate_new_mail(player, title, content, first_prize_id);
	}

	if (g_joined_test_uid_conf_mgr.is_uid_exist_in_2nd(player->userid)) {
		uint32_t second_prize_id = 2018;
		std::string title("领取青春内测战力相关奖励");
		std::string content("感谢您参与赤瞳之刃青春内测，并且战力值达到6888，为了感激您对赤瞳之刃的关注和支持，赤瞳特为你备上测试大礼，祝您在赤瞳之刃先声夺人，一斩必杀！");
		PlayerUtils::generate_new_mail(player, title, content, second_prize_id);
	}

	//论坛合影奖励
	std::vector<uint32_t> group_photo_vec;
	group_photo_vec.push_back(150686);
	group_photo_vec.push_back(302903796);
	group_photo_vec.push_back(365138404);
	group_photo_vec.push_back(38783219);
	group_photo_vec.push_back(34067792);
	group_photo_vec.push_back(186816416);
	std::vector<uint32_t>::iterator it;
	it = std::find(group_photo_vec.begin(), group_photo_vec.end(), player->userid);
	if (it != group_photo_vec.end()) {
		uint32_t photo_prize_id = 2309;
		std::string title("领取论坛合影的奖励");
		std::string content("感谢参加论坛合影活动，请领取幸运奖励");
		PlayerUtils::generate_new_mail(player, title, content, photo_prize_id);
	}

	SET_A(kAttrTestInPowerRankRangeFlag, NOW());
	return 0;
}

// uint32_t PlayerUtils::update_player_gold_consume_to_rank(
		// player_t* player, uint32_t count, uint32_t key, uint32_t subkey)
// {
	// if(0 == count){
		// return 0;
	// }
	// ADD_A(kAttrGoldConsumeRankingCount, count);
	// //更新充值排名
	// RankUtils::rank_user_insert_score(
			// player->userid, player->create_tm,
			// key, subkey, GET_A(kAttrGoldConsumeRankingCount));
// }

uint32_t PlayerUtils::update_player_power_to_rank(player_t* player, uint32_t new_power)
{
    //降低不更新榜
    if (new_power <= GET_A(kAttrBattleValueRecord)) {
        return 0;
    }

	RankUtils::rank_user_insert_score(
			player->userid, player->create_tm,
			(uint32_t)commonproto::RANKING_TOTAL_POWER, 
			0, new_power);

	uint32_t sub_key = TimeUtils::get_prev_friday_date();
	RankUtils::rank_user_insert_score(
			player->userid, player->create_tm,
			(uint32_t)commonproto::RANKING_TOTAL_POWER, 
			sub_key, new_power);
	SET_A(kLastUpdatePowerTm, NOW());

	//更新限时战力榜
    // if (TimeUtils::is_current_time_valid(TM_CONF_KEY_RANKING_TIME_LIMIT, 2)) {
    if (g_srv_time_mgr.is_now_time_valid(TM_SUBKEY_POWER)) {
		uint32_t power = new_power;
		// uint32_t start_day = TimeUtils::get_start_time(TM_CONF_KEY_RANKING_TIME_LIMIT, 2);
		uint32_t start_day = g_srv_time_mgr.get_start_time(TM_SUBKEY_POWER);
		//更新充值排名
		RankUtils::rank_user_insert_score(
				player->userid, player->create_tm,
				(uint32_t)commonproto::RANKING_TL_TOTAL_POWER, 
				start_day, power);
	}
	return 0;
}

uint32_t PlayerUtils::generate_new_mail(player_t* player, 
		const string title, const string content, uint32_t prize_id)
{
	if (prize_id) {
		std::vector<cache_prize_elem_t> prize_vec;
		prize_vec.clear();
		int ret = transaction_pack_prize(player, prize_id, prize_vec, NO_ADDICT_DETEC);
		if (ret) {
			ERROR_TLOG("Generate New Mail Err, No Prize,prize_id:[%u]Uid:[%u]",
					prize_id, player->userid);
		}
		if (prize_vec.size()) {
			new_mail_t new_mail;
			new_mail.sender.assign("系统邮件");
			new_mail.title = title;
			new_mail.content = content; 
			std::string attachment;
			MailUtils::serialize_prize_to_attach_string(prize_vec, attachment);
			new_mail.attachment = attachment;
			MailUtils::add_player_new_mail(player, new_mail);
		}
	} else {
		new_mail_t new_mail;
		new_mail.sender.assign("系统邮件");
		new_mail.title = title;
		new_mail.content = content; 
		new_mail.attachment.clear();
		MailUtils::add_player_new_mail(player, new_mail);
	}
	return 0;
}

bool PlayerUtils::test_vip_state(player_t* player)
{
	bool exceed_flag = false;
	if (GET_A(kAttrGoldVipEndTime) && NOW() > GET_A(kAttrGoldVipEndTime)) {
		exceed_flag = true;
	} else if (GET_A(kAttrSilverVipEndTime) && NOW() > GET_A(kAttrSilverVipEndTime)) {
		exceed_flag = true;
	}
	return exceed_flag;
}

uint32_t PlayerUtils::update_player_escort_power(uint32_t userid,
		uint32_t u_create_tm, uint32_t top_power)
{
	uint64_t role_key = ROLE_KEY(ROLE(userid, u_create_tm));
	escort_info_t* escort_ptr = g_escort_mgr.get_user_escort_info(role_key); 
	if (escort_ptr == NULL) {
		return cli_err_deal_rob_result_but_suf_not_ext;
	}
	escort_ptr->power = top_power;
	return 0;
}

uint32_t PlayerUtils::recv_system_mail(player_t* player, mail_type_t mail_type)
{
	switch (mail_type) {
		case GET_TEST_PRIZE:
			PlayerUtils::recv_mail_get_test_prize(player);
			break;
		case GET_CREATE_ROLE_PRIZE:
			PlayerUtils::recv_mail_get_create_prize(player);
			break;
		case GET_MAYIN_BUCKET_PRIZE:
			PlayerUtils::recv_mayin_bucket_prize(player);
			break;
		case GET_MAYIN_FLOWER_PRIZE:
			PlayerUtils::recv_mayin_flower_prize(player);
			break;
		case GET_MERGE_SVR_PRIZE:
			PlayerUtils::recv_merge_server_prize(player);
			break;
		default:
			break;
	}
	return 0;
}

uint32_t PlayerUtils::recv_mail_get_create_prize(player_t* player)
{
	/*
	if (!TimeUtils::is_current_time_valid(TM_CONF_KEY_CREATE_ROLE_PRIZE, 0)) {
		return 0;
	}
	*/
	if (GET_A(kAttrRecvCreateRoleMailPrizeFlag)) {
		return 0;
	}
	uint32_t prize_id = 5027;
	std::string title = "恭喜获得价值288钻新手礼包";
	std::string content = "尊敬的天选者，这是来自卡奥斯人民的一点心意，祝你在之后的冒险中所向披靡！";
	PlayerUtils::generate_new_mail(player, title, content, prize_id);

	SET_A(kAttrRecvCreateRoleMailPrizeFlag, NOW());
	return 0;
}

uint32_t PlayerUtils::calc_extra_add_attr(player_t* player,
		std::map<uint32_t, uint32_t> &attr_map)
{
	attr_map[kAttrHpMax] += GET_A(kAttrExtraMaxHp);
	attr_map[kAttrNormalAtk] += GET_A(kAttrExtraNormalAtk);
	attr_map[kAttrNormalDef] += GET_A(kAttrExtraNormalDef);
	attr_map[kAttrSkillAtk] += GET_A(kAttrExtraSkillAtk);
	attr_map[kAttrSkillDef] += GET_A(kAttrExtraSkillDef);

	attr_map[kAttrCrit] += GET_A(kAttrExtraCrit);
	attr_map[kAttrAntiCrit] += GET_A(kAttrExtraAntiCrit);
	attr_map[kAttrHit] += GET_A(kAttrExtraHit);
	attr_map[kAttrDodge] += GET_A(kAttrExtraDodge);
	attr_map[kAttrBlock] += GET_A(kAttrExtraBlock);
	attr_map[kAttrBreakBlock] += GET_A(kAttrExtraBreakBlock);
	return 0;
}

uint32_t PlayerUtils::recv_mayin_bucket_prize(player_t* player)
{
	uint32_t last_logout_tm = GET_A(kAttrLastLogoutTm);
	if (TimeUtils::is_same_day(NOW(), last_logout_tm) == false) {
		for (uint32_t i = 0; i < 4; ++i) {
			if (GET_A(attr_type_t(kAttrMayinGift01RecvState + i)) == 1) {
				SET_A(attr_type_t(kAttrMayinGift01RecvState + i), 2);
				//发奖励
				uint32_t base_prize_id = 11201;
				std::string title = "获得玛音奖励";
				std::string content = "玛音奖励获取";
				PlayerUtils::generate_new_mail(player, title, content, base_prize_id + i);
			}
			SET_A(attr_type_t(kAttrMayinGift01RecvState + i), 0);
		}
	}
	return 0;
}

uint32_t PlayerUtils::recv_merge_server_prize(player_t* player)
{
	if (player->init_server_id >= 3) {
		return 0;
	}
	if (GET_A(kAttrTestRecvMergeSvrPrize) || GET_A(kAttrCreateTm) > 1436457600) {
		return 0;
	}
	uint32_t prize_id = 2052;
	std::string title = "领取合服奖励";
	std::string content = "恭喜 一斩必杀 和 百兽王化 服务器完成联服，今后两个服务器的玩家可以一起互动了！\n以下是您的联服礼包，请查收！\nPs 您的登陆和充值方式不会发生任何变化！";
	PlayerUtils::generate_new_mail(player, title, content, prize_id);
	SET_A(kAttrTestRecvMergeSvrPrize, NOW());
	return 0;
}

uint32_t PlayerUtils::recv_mayin_flower_prize(player_t* player)
{
	if (!TimeUtils::is_current_time_valid(TM_CONF_KEY_MAYIN_FLOWER, 1)) {
		return 0;
	}
	if (GET_A(kAttrTestInMayinFlowerFlag)) {
		return 0;
	}
	//米米号，排名
	std::map<uint64_t, uint32_t> flower_rank_map;
	//一区
	flower_rank_map[1129758635738834687] = 1; //263042430  1428465407
	flower_rank_map[1680795930429631177] = 2; //userid:391340798        create_tm:1429088969
	flower_rank_map[2154646821654541560] = 3; //userid:501667806        create_tm:1428468984
	flower_rank_map[70936763622602982] = 4;	//userid:16516252 create_tm:1430108390
	flower_rank_map[1673797749371722976] = 5;	//userid:389711407        create_tm:1428577504
	flower_rank_map[1359408819069040088] = 6;	//userid:316512030        create_tm:1428469208
	flower_rank_map[2480834220392763556] = 7;	//userid:577614228        create_tm:1428476068
	flower_rank_map[1209673623336691112] = 8;	//userid:281649088        create_tm:1428465064
	flower_rank_map[865633475774738002] = 9;	//userid:201545999        create_tm:1430089298
	flower_rank_map[362825309447564309] = 10;	//userid:84476850 create_tm:1428466709

	//二区
	flower_rank_map[340850826539738670] = 1;	//userid:79360517 create_tm:1431086638
	flower_rank_map[1780413148491716534] = 2;	//userid:414534739        create_tm:1430820790
	flower_rank_map[279176187090178921] = 3;	//userid:65000771 create_tm:1430393705
	flower_rank_map[764892191759794575] = 4;	//userid:178090341        create_tm:1431306639
	flower_rank_map[1769472170276457058] = 5;	//userid:411987344        create_tm:1430555234
	flower_rank_map[1725514352527784760] = 6;	//userid:401752617        create_tm:1430371128
	flower_rank_map[578184663412188413] = 7;	//userid:134619107        create_tm:1430463741
	flower_rank_map[302216577804349838] = 8;	//userid:70365280 create_tm:1430466958
	flower_rank_map[381088343615729019] = 9;	//userid:88729044 create_tm:1430383995
	flower_rank_map[1745148009542450135] = 10;	//userid:406323934        create_tm:1430387671
	uint32_t rank = 0;
	uint64_t role_key = ROLE_KEY(ROLE(player->userid, player->create_tm));
	if (flower_rank_map.count(role_key) == 0) {
		return 0;
	} else {
		rank = flower_rank_map.find(role_key)->second;
	}
	uint32_t prize_id = 0;
	switch (rank) {
		case 1:
			prize_id = 11510;
			break;
		case 2:
		case 3:
			prize_id = 11511;
			break;
		case 4:
		case 5:
		case 6:
			prize_id = 11512;
			break;
		default:
			prize_id = 11513;
			break;
	}
	std::string title = "领取奖励";
	std::string content = "恭喜你在520玛音爱之表白活动中，获得第" + boost::lexical_cast<std::string>(rank) + "名,获得玛音赠送的一下礼物，请注意查收哦！";
	PlayerUtils::generate_new_mail(player, title, content, prize_id);
	SET_A(kAttrTestInMayinFlowerFlag, NOW());
	return 0;
}

//检查安装的称号是否已经过期，过期则从玩家身上卸下
uint32_t PlayerUtils::deal_equiped_title(player_t* player)
{
	if (GET_A(kAttrEquipTitleId) == 0) {
		return 0;
	}
	if (player->title->check_title_expire(GET_A(kAttrEquipTitleId))) {
		SET_A(kAttrEquipTitleId, 0);
	}
	return 0;
}

uint32_t PlayerUtils::deal_after_merger_server(player_t* player)
{
	//怪物危机榜
	if (GET_A(kAttrDealMergeServerTm)) {
		return 0;
	}
	SET_A(kAttrDealMergeServerTm, NOW());
	uint32_t mon_dup_id = GET_A(kAttrDupHighestUnlock1Id);
	uint32_t revers_value = (uint32_t)commonproto::RANK_REVERSE_INT64_HIGH_SCORE;
	uint32_t progress = revers_value - mon_dup_id;
	uint32_t total_time = GET_A(kAttrMonsterHistoryBestTime);
	uint64_t mon_score = ((uint64_t)progress << 32) | total_time;
	RankUtils::rank_user_insert_score(
			player->userid, player->create_tm,
			commonproto::RANKING_MONSTER_CRISIS, 
			0, mon_score);

	//坐骑，翅膀不用处理

	//伙伴总战力 7
	RankUtils::rank_user_insert_score(
			player->userid, player->create_tm,
			commonproto::RANKING_SPIRIT_TOTAL_POWER, 
			0, GET_A(kPetTotalPower));

	//总战力 12
	RankUtils::rank_user_insert_score(
			player->userid, player->create_tm,
			commonproto::RANKING_TOTAL_POWER, 
			0, GET_A(kAttrBattleValueRecord));
	//伙伴总战力 14
	RankUtils::rank_user_insert_score(
			player->userid, player->create_tm,
			commonproto::RANKING_SPIRIT_TOP_5_POWER, 
			0, GET_A(kPetNTopestPower));
	//装备榜 8
	RankUtils::rank_user_insert_score(
			player->userid, player->create_tm,
			(uint32_t)commonproto::RANKING_EQUIP_GRADE, 
			0, GET_A(kAttrEquipBtlValue));
	//成就榜 11
	uint32_t achieve_point = GET_A(kAttrAchievePoint);
	uint32_t btl_record = GET_A(kAttrBattleValueRecord);
	uint64_t achieve_score = ((uint64_t)achieve_point << 32) | btl_record;
	RankUtils::rank_user_insert_score(
			player->userid, player->create_tm,
			commonproto::RANKING_ACHIEVEMENT,
			0, achieve_score);

	//世界Boss 13
	RankUtils::rank_user_insert_score(
			player->userid, player->create_tm,
			commonproto::RANKING_TYPE_WORLD_BOSS_DAMAGE_1, 
			g_online_id, GET_A(kAttrWorldBossDamage));

	//竞技场 24
	RankUtils::rank_user_insert_score(player->userid, player->create_tm,
			(uint32_t)commonproto::RANKING_RPVP,
			0, GET_A(kAttrRpvpScore), 0xFFFFFFFF);

	//一桶天下 4
	RankUtils::rank_user_insert_score(player->userid, player->create_tm,
			(uint32_t)commonproto::RANKING_TYPE_BUCKET,
			0, GET_A(kAttrBucketMaxScore), 0xFFFFFFFF);

	return 0;
}

//检查player套装buff
uint32_t PlayerUtils::calc_player_suit_buff_info(player_t* player)
{
	//套装id，装备个数
	std::map<uint32_t, uint32_t> suit_equip_cnt_map;
	suit_equip_cnt_map.clear();
	for (int i = kAttrHead; i <= kAttrOtherEquipAttr; i++) {
		uint32_t equip_slot_id = GET_A((attr_type_t)i);
		if (!equip_slot_id) {
			continue;
		}
		item_t *item = player->package->get_mutable_item_in_slot(equip_slot_id);
		if (item == NULL) {
			continue;
		}

		if (!g_item_conf_mgr.is_equip(item->item_id)) {
			WARN_TLOG("P:%u equip_pos:%u item:%u is not equip",
					player->userid, i, item->item_id);
			continue;
		}

		uint32_t suit_id = g_suit_conf_mgr.find_suit_id(item->item_id);
		if(suit_id != 0){
			suit_equip_cnt_map[suit_id] += 1;
		} else {
			continue;
		}
	}

	//检查是否满足套装buff触发条件
	if(suit_equip_cnt_map.size() != 0){
		check_player_suit_buff(player, suit_equip_cnt_map );
	}
	return 0;
}

uint32_t PlayerUtils::check_player_suit_buff(player_t* player, const std::map<uint32_t, uint32_t> &suit_equip_cnt_map)
{
    std::map<uint32_t, uint32_t>& suit_buff_map = *(player->suit_buff_map); 
	suit_buff_map.clear();
	FOREACH(suit_equip_cnt_map, it){
		uint32_t id = it->first;
		uint32_t num = it->second;

		const std::map<uint32_t, uint32_t>& trigger_map = 
			g_suit_conf_mgr.find_suit_conf(id)->trigger_buff_map;

		//找满足条件的buff_id
		FOREACH(trigger_map, it){
			if(num >= it->first){
				suit_buff_map[id] = it->second;
			}
		}
	}

	return 0;
}
