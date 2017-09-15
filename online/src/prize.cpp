#include "prize.h"
#include "prize_conf.h"
#include "global_data.h"
#include "player.h"
#include "global_data.h"
#include "utils.h"
#include "attr.h"
#include "attr_utils.h"
#include "item_conf.h"
#include "item.h"
#include "pet_utils.h"
#include "player_utils.h"
#include "package.h"
#include "service.h"
#include <bitset>
#include "sys_ctrl.h"
#include "rune_utils.h"
#include "chat_processor.h"
#include "rank_utils.h"
#include "title.h"

void cache_prize_to_proto_prize(std::vector<cache_prize_elem_t> &cache_vec,
        google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> &pb_list,
        bool only_get_show)
{
    FOREACH(cache_vec, it) {
        const cache_prize_elem_t &elem = *it;
        if (only_get_show && elem.show == 0) {
            continue;
        }

        commonproto::prize_elem_t *inf = pb_list.Add();
        inf->set_type((commonproto::prize_elem_type_t)elem.type);
        inf->set_id(elem.id);
        inf->set_level(elem.level);
        inf->set_talent_level(elem.talent_level);
        inf->set_count(elem.count);
        inf->set_pow(elem.pow);
        inf->set_show_type(elem.show);
    }
}

int transaction_pack_prize_except_item(player_t *player, 
        uint32_t prize_id,
	    std::vector<cache_prize_elem_t> &award_elems, 
        std::vector<cache_prize_elem_t> *display_elems, 
		bool addict_detec,
        uint32_t except_id)
{
    assert(player);
    award_elems.clear();
    if (display_elems) {
        display_elems->clear();
    }

    int err = 0;
    const prize_config_t *prize_conf = 
        g_prize_conf_mgr.get_prize_conf(prize_id);
    if (!prize_conf) {
        return cli_err_prize_id_not_exist;
    }

    //如果开启了随机掉落 则随机OUT项
    std::map<uint32_t, uint32_t> award_rate_map;
    std::map<uint32_t, uint32_t> display_rate_map;
    award_rate_map.clear();
    std::set<uint32_t> award_hit_idx_set;
    std::set<uint32_t> display_hit_idx_set;
    //构建idx列表 idx的顺序为 物品->属性->精灵->符文
    uint32_t idx = 0;
    uint32_t total_prizable_elem = 0;
    FOREACH(prize_conf->prize_items, it) {
        if ((*it).id == except_id || is_prize_exceed_limit(player, *it, (*it).calc_type)) {
            idx++;
            continue; 
        }
        total_prizable_elem++;
        award_rate_map[idx] = it->award_rate;
        display_rate_map[idx] = it->display_rate;
        idx++;
    }
    FOREACH(prize_conf->prize_attrs, it) {
        if ((*it).id == except_id || is_prize_exceed_limit(player, *it, (*it).calc_type)) {
            idx++;
            continue; 
        }
        total_prizable_elem++;
        award_rate_map[idx] = it->award_rate;
        display_rate_map[idx] = it->display_rate;
        idx++;
    }
    FOREACH(prize_conf->prize_pets, it) {
        if ((*it).id == except_id || is_prize_exceed_limit(player, *it, (*it).calc_type)) {
            idx++;
            continue; 
        }
        total_prizable_elem++;
        award_rate_map[idx] = it->award_rate;
        display_rate_map[idx] = it->display_rate;
        idx++;
    }
	FOREACH(prize_conf->prize_runes, it) {
		if ((*it).id == except_id || is_prize_exceed_limit(player, *it, (*it).calc_type)) {
			idx++;
			continue; 
		}
		total_prizable_elem++;
		award_rate_map[idx] = it->award_rate;
		display_rate_map[idx] = it->display_rate;
		idx++;
	}
	FOREACH(prize_conf->prize_titles, it) {
		if ((*it).id == except_id || is_prize_exceed_limit(player, *it, (*it).calc_type)) {
			idx++;
			continue;
		}
		total_prizable_elem++;
		award_rate_map[idx] = it->award_rate;
		display_rate_map[idx] = it->display_rate;
		idx++;
	}
    uint32_t select_cnt = 0;
    if (prize_conf->rand_mode > 0) {//给定了输出量
        if (prize_conf->rand_mode > (int)total_prizable_elem) {
            //没那么多 则取最多
            select_cnt = total_prizable_elem;
        } else {
            select_cnt = prize_conf->rand_mode;
        }
        Utils::rand_select_uniq_m_from_n_with_r(award_rate_map, 
                award_hit_idx_set, select_cnt);

    } else if (prize_conf->rand_mode == 0) {//没给则全要
        select_cnt = total_prizable_elem;
        Utils::rand_select_uniq_m_from_n_with_r(award_rate_map, 
                award_hit_idx_set, select_cnt);

    } else if (prize_conf->rand_mode == SELF_RAND_MODE) {//自我随机
        Utils::rand_select_from_set(award_rate_map, award_hit_idx_set);
    }

    //当rand_mode == -1是 select_cnt无效
    if (select_cnt && award_hit_idx_set.size() != select_cnt) {
        return cli_err_sys_err;
    }

    if (prize_conf->display_cnt != 0) {
        FOREACH(award_hit_idx_set, it) {
            display_rate_map.erase(*it);//剔除已经能获得的idx
        }
        Utils::rand_select_uniq_m_from_n_with_r(display_rate_map,
                display_hit_idx_set, prize_conf->display_cnt);
        if (display_hit_idx_set.size() != prize_conf->display_cnt) {
            return cli_err_sys_err;
        }
    }

    //随机到的物品和显示的属性分别放在award_hit_idx_set 和display_hid_idx_set中
    
    // 开始随机到的东西是否可以添加
    uint32_t item_idx = 0;
    uint32_t item_idx_base = 0;
    std::vector<add_item_info_t> add_vec;
    cache_prize_elem_t inf;
    for (uint32_t i = 0; i < prize_conf->prize_items.size(); i++) {
        inf.clear();
        item_idx = item_idx_base + i;
        const prize_elem_t &item_elem = prize_conf->prize_items[i];       
		//奖励需要购买
		if(item_elem.price != 0){
			inf.price = item_elem.price;
			inf.price_type = item_elem.price_type;
		}
        uint32_t multi = TimeUtils::get_current_time_prize_multi(item_elem.adjust_type);
        if (item_elem.count == 0) {
            continue;

        } else if (prize_conf->display_cnt != 0 && display_hit_idx_set.count(item_idx) > 0) {
            //显示
            if (display_elems) {
                inf.type = (uint32_t)(commonproto::PRIZE_ELEM_TYPE_ITEM);
                inf.id = item_elem.id;
                inf.count = COUNT_TRANS(item_elem.count, multi, item_elem.calc_type);
                inf.level = item_elem.level;
                display_elems->push_back(inf);
            }
            continue;

        } else if (award_hit_idx_set.count(item_idx) == 0){
            //不显示 也未命中
            continue;
        }
        inf.type = (uint32_t)commonproto::PRIZE_ELEM_TYPE_ITEM;
        inf.id = item_elem.id;
        inf.level = item_elem.level;
        inf.show = prize_conf->show ?prize_conf->show :item_elem.show;
        inf.notice = item_elem.notice;
        if (addict_detec) {
            if (check_player_addicted_threshold_none(player)) {
                inf.count = 0;
            } else if (check_player_addicted_threshold_half(player)) {
                //判定是否属性沉迷减半物品
                if (g_item_conf_mgr.is_addicted_attr_item(item_elem.id)) {
                    inf.count = COUNT_TRANS(item_elem.count, multi, item_elem.calc_type) / 2;

                //是否属性物品(不能减半)
                } else if (g_item_conf_mgr.is_attr_item(item_elem.id)) {
                    inf.count = COUNT_TRANS(item_elem.count, multi, item_elem.calc_type);

                //普通物品
                } else {
                    inf.count = COUNT_TRANS(item_elem.count, multi, item_elem.calc_type)/2;
                }               
            } else {
			    inf.count = COUNT_TRANS(item_elem.count, multi, item_elem.calc_type);
            }
		} else {
			inf.count = COUNT_TRANS(item_elem.count, multi, item_elem.calc_type);
		}

        if (item_elem.duration) {
            inf.expire_time = NOW() + item_elem.duration * 86400;
        } else {
            inf.expire_time = 0;
        }
        award_elems.push_back(inf);
        add_item_info_t add_item(inf.id, inf.count, inf.expire_time, inf.level);
        add_vec.push_back(add_item);
        add_prize_limit(player, item_elem);
    }

    // 检查精灵是否可以添加
    uint32_t pet_base_idx = prize_conf->prize_items.size() + prize_conf->prize_attrs.size();
    uint32_t pet_idx = 0;
    std::set<const prize_elem_t*> add_pet_set;
    add_pet_set.clear();
    for (uint32_t i = 0; i < prize_conf->prize_pets.size(); i++) {
        inf.clear();
        pet_idx = pet_base_idx + i;
        const prize_elem_t &pet_config = prize_conf->prize_pets[i];
		//奖励需要购买
		if(pet_config.price != 0){
			inf.price = pet_config.price;
			inf.price_type = pet_config.price_type;
		}
		if (prize_conf->display_cnt != 0 && display_hit_idx_set.count(pet_idx) > 0) {
			//显示精灵
            if (display_elems) {
                inf.type = (uint32_t)commonproto::PRIZE_ELEM_TYPE_PET;
                inf.id = pet_config.id;
                inf.level = pet_config.level;
                inf.talent_level = pet_config.talent_level;
                display_elems->push_back(inf);
            }
			continue;

		} else if (award_hit_idx_set.count(pet_idx) == 0) {
			continue;

		}

		//本身已经拥有该伙伴
		bool succ = PetUtils::check_can_create_pet(player, pet_config.id);
		//判断是否同时抽到多只
		bool has_pet = false;
		FOREACH(award_elems,it){
			cache_prize_elem_t  tmp = *it;
			if(tmp.id == pet_config.id && tmp.type == 2){
				has_pet = true;
			}
		}
		//NOTI(vince)已有精灵转化为对应碎片
		if(!succ || has_pet){
			const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet_config.id);
			if (!pet_conf) return 0;
			int32_t talent_level = pet_config.talent_level > 1 ?
			  	pet_config.talent_level : 1;
			/* 不同星级对应的碎片数量 */
			uint32_t trans_num[] = {
				kOneTLevelItemCnt,    
				kTwoTLevelItemCnt, 
				kThreeTLevelItemCnt, 
				kFourTLevelItemCnt,
				kFiveTLevelItemCnt, 
			};
            inf.type = (uint32_t)(commonproto::PRIZE_ELEM_TYPE_ITEM);
            inf.id = pet_conf->talent_item;
            inf.count = trans_num[talent_level - 1];
            inf.expire_time = 0;
            inf.show = prize_conf->show ?prize_conf->show :pet_config.show;
			inf.notice = pet_config.notice;
            add_item_info_t add_item(inf.id, inf.count, inf.expire_time, inf.level);
            add_vec.push_back(add_item);

        } else {
			inf.type = (uint32_t)commonproto::PRIZE_ELEM_TYPE_PET;
			inf.id = pet_config.id;
			inf.level = pet_config.level;
			inf.talent_level = pet_config.talent_level;
			inf.notice = pet_config.notice;
            inf.show = prize_conf->show ?prize_conf->show :pet_config.show;
            inf.count = 1;
		}
        award_elems.push_back(inf);
        add_prize_limit(player, pet_config);
	}

	// 检查物品是否可以添加/扣除
	err = check_swap_item_by_item_id(player, 0, &add_vec, NO_ADDICT_DETEC);
	if (err) {
		return err;
	}

	//检查符文是否可以添加
	uint32_t rune_base_idx = prize_conf->prize_items.size() 
        + prize_conf->prize_attrs.size() 
        + prize_conf->prize_pets.size();
	uint32_t rune_idx;
	std::set<const prize_elem_t*> add_rune_set;
	for (uint32_t i = 0; i < prize_conf->prize_runes.size(); ++i) {
        inf.clear();
		rune_idx = rune_base_idx + i;
		const prize_elem_t &rune_config = prize_conf->prize_runes[i];
		//奖励需要购买
		if(rune_config.price != 0){
			inf.price = rune_config.price;
			inf.price_type = rune_config.price_type;
		}
		if (prize_conf->display_cnt != 0 && display_hit_idx_set.count(rune_idx) > 0) {
            if (display_elems) {
                inf.type = (uint32_t)commonproto::PRIZE_ELEM_TYPE_RUNE;
                inf.id = rune_config.id;
                inf.level = rune_config.level;
				inf.count = 1;
                display_elems->push_back(inf);
            }
			continue;

		} else if (award_hit_idx_set.count(rune_idx) == 0) {
			continue;

		}

		inf.type = (uint32_t)commonproto::PRIZE_ELEM_TYPE_RUNE;
		inf.id = rune_config.id;
		inf.level = rune_config.level;
		inf.notice = rune_config.notice;
        inf.show = prize_conf->show ?prize_conf->show :rune_config.show;
		inf.count = 1;
        award_elems.push_back(inf);
        add_prize_limit(player, rune_config);
    }

	uint32_t title_base_idx = prize_conf->prize_items.size()
			+ prize_conf->prize_attrs.size()
			+ prize_conf->prize_pets.size()
			+ prize_conf->prize_runes.size();
	uint32_t title_idx;
	std::set<const prize_elem_t*> add_title_set;
	for(uint32_t i = 0; i < prize_conf->prize_titles.size(); ++i) {
		inf.clear();
		title_idx = title_base_idx + i;
		const prize_elem_t &title_config = prize_conf->prize_titles[i];
		//奖励需要购买
		if(title_config.price != 0){
			inf.price = title_config.price;
			inf.price_type = title_config.price_type;
		}
		if (prize_conf->display_cnt != 0 && display_hit_idx_set.count(title_idx) > 0) {
			if (display_elems) {
				inf.type = (uint32_t)commonproto::PRIZE_ELEM_TYPE_TITLE;
				inf.id = title_config.id;
				inf.count = 1;
				display_elems->push_back(inf);
			}
			continue;
		} else if (award_hit_idx_set.count(title_idx) == 0) {
			continue;
		}
		inf.type = (uint32_t)commonproto::PRIZE_ELEM_TYPE_TITLE;
		inf.id = title_config.id;
		inf.count = 1;
		award_elems.push_back(inf);
		add_prize_limit(player, title_config);
	}

    // 添加属性
    std::map<uint32_t, uint32_t> attr_chg_map;
    uint32_t attr_base_idx = prize_conf->prize_items.size();
    uint32_t attr_idx;
    for (uint32_t i = 0; i < prize_conf->prize_attrs.size(); i++) {
        inf.clear();
        attr_idx = attr_base_idx + i;
        const prize_elem_t &attr_config = prize_conf->prize_attrs[i]; 
		//奖励需要购买
		if(attr_config.price != 0){
			inf.price = attr_config.price;
			inf.price_type = attr_config.price_type;
		}
        uint32_t multi = TimeUtils::get_current_time_prize_multi(attr_config.adjust_type);
        inf.type = (uint32_t)commonproto::PRIZE_ELEM_TYPE_ATTR;
        inf.id = attr_config.id;
        inf.notice = attr_config.notice;
        inf.show = prize_conf->show ?prize_conf->show :attr_config.show;
        if (prize_conf->display_cnt != 0 && display_hit_idx_set.count(attr_idx) > 0) {
            if (display_elems) {
                inf.count = COUNT_TRANS(attr_config.count, multi, attr_config.calc_type);
                display_elems->push_back(inf);
            }
            continue;

        } else if (award_hit_idx_set.count(attr_idx) == 0) {
            continue;
        }

		//防沉迷，属性减半
		if(addict_detec && check_player_addicted_threshold_none(player)){
			inf.count = 0;

		} else if (addict_detec && check_player_addicted_threshold_half(player)) {
			inf.count = COUNT_TRANS(attr_config.count, multi, attr_config.calc_type) / 2;

		} else {
			inf.count = COUNT_TRANS(attr_config.count, multi, attr_config.calc_type);
		}

        award_elems.push_back(inf);
        add_prize_limit(player, attr_config);
    }


    return 0;
}

int transaction_proc_packed_prize(player_t *player, 
		const std::vector<cache_prize_elem_t> &award_elems,
        onlineproto::sc_0x0112_notify_get_prize *msg,
        commonproto::prize_reason_t reason, 
        string stat_name)
{
    assert(player);
    std::vector<add_item_info_t> add_vec;
    add_item_info_t add_item;
    int err = 0;
    google::protobuf::RepeatedPtrField<commonproto::prize_elem_t> show_elems;
	//加入奖励榜单
    google::protobuf::RepeatedPtrField<commonproto::prize_bulletin_info_t> notice_elems;

    std::map<uint32_t, uint32_t> attr_chg_map;

    if (!msg) {
        onlineproto::sc_0x0112_notify_get_prize tmp;
        msg = &tmp;
    }
    msg->set_reason(reason);

    uint32_t show_type = 0;
    uint32_t notice_type = 0;
	bool update_power_flag = false;
	std::vector<cache_prize_elem_t> pet_vec;
	pet_vec.clear();
    FOREACH(award_elems, it) {
        const cache_prize_elem_t &elem = *it;
		//是精灵
		if(elem.type == (uint32_t)commonproto::PRIZE_ELEM_TYPE_PET){
			FOREACH(pet_vec, it){
				cache_prize_elem_t  tmp = *it;
				//相同的精灵转化为碎片
				if(tmp.id == elem.id){
					pet_transform_chips(elem);
				} else {
					pet_vec.push_back(elem);
				}
			}
		}
		if (msg) {
            commonproto::prize_elem_t *inf = msg->add_award_elems();
            inf->set_type((commonproto::prize_elem_type_t)(elem.type));
            inf->set_id(elem.id);
            inf->set_count(elem.count);
            inf->set_level(elem.level);
            inf->set_talent_level(elem.talent_level);
        }
        if (elem.show) {
            commonproto::prize_elem_t *inf = show_elems.Add();
            inf->set_type((commonproto::prize_elem_type_t)(elem.type));
            inf->set_id(elem.id);
            inf->set_count(elem.count);
            inf->set_level(elem.level);
            inf->set_talent_level(elem.talent_level);
            show_type = elem.show;
        }

        if (elem.notice) {
            commonproto::prize_bulletin_info_t*inf = notice_elems.Add();
            inf->set_nick(player->nick, strlen(player->nick));
            inf->mutable_elem()->set_type(
					(commonproto::prize_elem_type_t)(elem.type));
            inf->mutable_elem()->set_id(elem.id);
            inf->mutable_elem()->set_count(elem.count);
            inf->mutable_elem()->set_level(elem.level);
            inf->mutable_elem()->set_talent_level(elem.talent_level);
            notice_type = elem.notice;
        }

        //如果是物品
		if (elem.type == (uint32_t)commonproto::PRIZE_ELEM_TYPE_ITEM) {
            add_item_info_t add_item;
            add_item.item_id = elem.id;
            add_item.count = elem.count;
            add_item.expire_time  = elem.expire_time;
            add_item.level = elem.level;
            add_vec.push_back(add_item);

        //如果是属性
        } else if (elem.type == (uint32_t)commonproto::PRIZE_ELEM_TYPE_ATTR) {
            //拦截经验奖励
            if (elem.id == kAttrExp) {
                uint32_t real_add;
                PlayerUtils::add_player_exp(player, elem.count, &real_add, NO_ADDICT_DETEC);
                continue;

            } else if (elem.id == kAttrPetExp) { //拦截特殊的伙伴经验奖励
                for (int i = 0; i < MAX_FIGHT_POS; i++) {
                    Pet *pet = player->fight_pet[i];
                    if (!pet) {
                        continue;
                    }
                    uint32_t real_add_exp;
                    PetUtils::add_pet_exp(player, pet, elem.count, real_add_exp, NO_ADDICT_DETEC);
                }
                continue;

            } else if (elem.id == kAttrGold || elem.id == kAttrPaidGold) {//拦截金币奖励
                if (elem.count < 0) {//减少
                    AttrUtils::sub_player_gold(player, -(elem.count), stat_name);
                } else {//增多
                    AttrUtils::add_player_gold(player, elem.count, elem.id == kAttrGold ?false :true, stat_name);
                }
                continue;
			}

			//判断奖励是否加增加战斗属性
			update_power_flag = check_is_need_update_power(elem.id);

            if (attr_chg_map.count(elem.id) == 0) {
                attr_chg_map[elem.id] = GET_A((attr_type_t)elem.id);
            }
            int32_t chg_val = elem.count;
            uint32_t value = attr_chg_map[elem.id];
            if (chg_val < 0) {
                uint32_t fabs = -chg_val;
                if (value < fabs) {
                    value = 0;
                } else {
                    value -= fabs;
                }
            } else {
                value += chg_val;
                uint32_t value_limit = GET_A_MAX((attr_type_t)elem.id);
                if (value_limit != kAttrMaxNoLimit && value_limit < value) { 
                    value = value_limit;
                }
            }
            attr_chg_map[elem.id] = value;

        //如果是精灵
        } else if (elem.type == (uint32_t)commonproto::PRIZE_ELEM_TYPE_PET) {
			pet_talent_level_t talent_level = (pet_talent_level_t)elem.talent_level; 
            err = PetUtils::create_pet(player, elem.id, elem.level, false, 0,talent_level);
            if (err) {
				continue;
			}

        //如果是符文
        } else if (elem.type == (uint32_t)commonproto::PRIZE_ELEM_TYPE_RUNE) {
			err = RuneUtils::add_rune(player, elem.id, elem.level, GET_RUNE_SYSTEM_SEND);
			if (err) {
			    continue;
			}
		} else if (elem.type == (uint32_t)commonproto::PRIZE_ELEM_TYPE_TITLE) {
			title_info_t title_info;
			title_info.title_id = elem.id;
			title_info.get_time = NOW();
			err = player->title->add_one_title(player, title_info, true);
			if (err) {
				continue;
			}
		}
    }

    AttrUtils::set_attr_value(player, attr_chg_map);
	//如果奖励中有 额外属性，则刷新下玩家战力
	if (update_power_flag) {
		PlayerUtils::calc_player_battle_value(player);
	}

    swap_item_by_item_id(player, 0, &add_vec, NO_WAIT_SVR, 
            onlineproto::SYNC_REASON_PRIZE_ITEM, NO_ADDICT_DETEC);

    if (show_elems.size()) {//要展示
        SystemNotify::SystemNotifyForPrize(player,
                &show_elems, (commonproto::system_noti_reason_t)show_type);
    }

	if(notice_elems.size()){//要加入奖励榜
		RankUtils::lpush_bulletin_list(player,
			   	(commonproto::prize_reason_t)msg->reason(),
			   	5, notice_elems, 50);	
	}
    return 0;
}

int transaction_proc_prize(player_t *player, 
        uint32_t prize_id,
        onlineproto::sc_0x0112_notify_get_prize &msg,
        commonproto::prize_reason_t reason,
        onlineproto::sync_item_reason_t item_reason,
		bool addict_detec)
{
    msg.Clear();
    msg.set_reason(reason);

    std::vector<cache_prize_elem_t> award_elems;
    std::vector<cache_prize_elem_t> display_elems;

    int ret = transaction_pack_prize_except_item(player, prize_id,
	    award_elems, &display_elems, addict_detec, 0);
    if (ret) {
        return ret;
    }

    if (award_elems.size()) {
        transaction_proc_packed_prize(player, award_elems, &msg, 
                reason, g_prize_conf_mgr.get_prize_desc(prize_id));
    }
    if (display_elems.size()) {
        cache_prize_to_proto_prize(display_elems, *(msg.mutable_display_elems()));
    }
    return 0;
}

int transaction_pack_prize(player_t *player, uint32_t prize_id,
		std::vector<cache_prize_elem_t> &result, bool addict_detec)
{
    std::vector<cache_prize_elem_t> display_elems;
	return	transaction_pack_prize_except_item(player, prize_id, result, 0, addict_detec);
}

void merge_cache_prize(std::vector<cache_prize_elem_t> &cache_vec, uint32_t num)
{
	std::map<cache_prize_elem_t, uint32_t> tmp_map;
	tmp_map.clear();
    FOREACH(cache_vec, it) {
        cache_prize_elem_t &elem = *it;
		if(tmp_map.count(elem) == 0){
			tmp_map.insert(pair<cache_prize_elem_t, uint32_t>(elem, elem.count));
		} else {
			std::map<cache_prize_elem_t, uint32_t>::iterator it;
			it = tmp_map.find(elem);
			it->second += elem.count;
		}
	}
	cache_vec.clear();
	std::vector<cache_prize_elem_t>().swap(cache_vec);

    FOREACH(tmp_map, it) {
		cache_prize_elem_t elem = it->first;
		elem.count = it->second * num;
		cache_vec.push_back(elem);
	}
}

//奖励减半
void half_cache_prize(std::vector<cache_prize_elem_t> &cache_vec)
{
    FOREACH(cache_vec, it) {
        cache_prize_elem_t &elem = *it;
		uint32_t type = elem.type;
		//物品，属性，符文才减半
		if(type == 1 || type == 3 || type == 5){
			elem.count = ceil(elem.count * 0.5);
		}
	}
}

void refresh_player_charge_diamond_draw_prize_info(player_t *player, std::vector<cache_prize_elem_t> &result)
{
    uint32_t prize_ids[] = {
        11101, 11102, 11103
    };//1*4, 2*150, 3*232
    uint32_t pow[] = {
        4, 150, 232
    };
    std::vector<cache_prize_elem_t> tmp;
    for (uint32_t i = 0; i < array_elem_num(prize_ids); i++) {
        tmp.clear();
        transaction_pack_prize(player, prize_ids[i], tmp, NO_ADDICT_DETEC);
        FOREACH(tmp, it1) {
            cache_prize_elem_t &elem = *it1;
            elem.pow = pow[i];
        }
        typeof(result.begin()) it = result.end();
        result.insert(it, tmp.begin(), tmp.end());
    }
    commonproto::prize_elem_list_t pb_msg;
    cache_prize_to_proto_prize(result, *(pb_msg.mutable_prize_list()));
    PlayerUtils::update_user_raw_data(player->userid, player->create_tm, 
            dbproto::CHARGE_DIAMOND_DRAW_PRIZE_ACTIVITY, pb_msg, "0");
    *(player->daily_charge_diamond_draw_cards_info) = result;
}

bool check_is_need_update_power(uint32_t elem_id)
{
	if (elem_id >= kAttrExtraNormalAtk && elem_id <= kAttrExtraMaxHp) {
		return true;
	} else {
		return false;
	}
}

int pet_transform_chips(const cache_prize_elem_t &pet_elem)
{
	cache_prize_elem_t &elem = const_cast<cache_prize_elem_t &>(pet_elem);

	const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(elem.id);
	if (!pet_conf) {return -1;}
	int32_t talent_level = pet_conf->born_talent > 1 ?
		pet_conf->born_talent : 1;
	/* 不同星级对应的碎片数量 */
	uint32_t trans_num[] = {
		kOneTLevelItemCnt,    
		kTwoTLevelItemCnt, 
		kThreeTLevelItemCnt, 
		kFourTLevelItemCnt,
		kFiveTLevelItemCnt, 
	};

	//转化为碎片
	elem.type = (uint32_t)commonproto::PRIZE_ELEM_TYPE_ITEM;
	elem.id = pet_conf->talent_item;
	elem.count = trans_num[talent_level - 1];
	elem.expire_time = 0;
	elem.level = 0;
	elem.talent_level = 0;
	return 0;
}
