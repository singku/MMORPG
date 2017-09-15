#include "pet_processor.h"
#include "player_utils.h"
#include "user_action_log_utils.h"
#include "attr_utils.h"
#include "pet_utils.h"
#include "data_proto_utils.h"
#include "pet_utils.h"
#include "service.h"
#include "utils.h"
#include "map_utils.h"
#include "item.h"
#include "sys_ctrl.h"
#include "task_utils.h"
#include "duplicate_utils.h"
#include "skill_conf.h"
#include "home_data.h"
#include "achieve.h"
#include "pet_pass_dup_conf.h"
//switch loc
int PetSwitchLocCmdProcessor::proc_pkg_from_client(
        player_t* player, const char* body, int bodylen) 
{
    PARSE_MSG;
    int ret = 0;
    //判断精灵是否存在
    Pet *p_pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm());
    Pet *s_pet = 0; //要替换的精灵
    if (!p_pet) {
        ret = cli_err_pet_not_exist;
    } else if (p_pet->loc() == (pet_location_type_t)(cli_in_.target_location())) {
        //位置没动
        cli_out_.Clear();
        return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    } else if (PetUtils::pets_full(player, (pet_location_type_t)(cli_in_.target_location()))) {
        ret = cli_err_target_pet_loc_full;
        s_pet = PetUtils::get_pet_in_loc(player, cli_in_.target_create_tm());
        if (!s_pet) {
            ret = cli_err_target_pet_not_exist;
        } else if (s_pet->loc() != (pet_location_type_t)(cli_in_.target_location())) {
            ret = cli_err_target_pet_loc_not_match;
        } else {
            ret = 0; //找到替换的了 则满了也没关系
        }
    }
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    pet_location_type_t original_loc = p_pet->loc();
    if (s_pet) {
        ret = PetUtils::set_pet_loc(player, s_pet->create_tm(), original_loc);
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd, ret); 
        }
    }

    ret = PetUtils::set_pet_loc(player, p_pet->create_tm(), (pet_location_type_t)cli_in_.target_location());
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret); 
    }

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int PetSetFightCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    //精灵是否存在
    Pet *p_pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm());
    if (!p_pet) {
        return send_err_to_player(player, 
                player->cli_wait_cmd, cli_err_target_pet_not_exist);
    }
	//如果副本指定的精灵正在出战，则不可以将其替下
	if (player->temp_info.dup_id) {
		const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(
				player->temp_info.dup_id);
		if (!dup) {
			RET_ERR(cli_err_duplicate_id_not_found);;
		}
		if (!(dup->can_fight_pets_id.empty())) {
			uint32_t pet_found = false;
			FOREACH(dup->can_fight_pets_id, it) {
				if (p_pet->pet_id() == *it) {
					pet_found = true;
					break;
				}
			}
			if (pet_found == false) {
				RET_ERR(cli_err_dup_assign_pet_can_not_switch_off);
			} 
		}
	}

    uint32_t fight_pos = cli_in_.fight_pos();
    if (!PetUtils::is_valid_fight_pos(fight_pos)) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_pet_fight_pos_invalid);
    }
    onlineproto::set_fight_pet_status_t status;
    Pet *dst_pet = 0;//要替换下的精灵

    uint32_t can_fight_num = PetUtils::can_fight_pets(player);

    if (p_pet->fight_pos()) {//收回
        status = onlineproto::PET_STATUS_CALLBACK;
    } else {//出战
        status = onlineproto::PET_STATUS_FIGHT;
        if (player->fight_pet[fight_pos-1] != 0) {
            dst_pet = player->fight_pet[fight_pos-1];
        }
        if (p_pet == dst_pet) {//已出战 又要出战?(bug)
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_pet_already_in_fight_pos);
        }
        if (!dst_pet && PetUtils::fight_pet_num(player) >= can_fight_num) {
            //出战精灵已满 又要新出战一个
            return send_err_to_player(player, player->cli_wait_cmd,
                    cli_err_fight_pet_full);
        }
    }

    if (status == onlineproto::PET_STATUS_FIGHT) {
        player->fight_pet[fight_pos - 1] = p_pet;
        p_pet->set_fight_pos(fight_pos);
        p_pet->cur_swim_exp = 0;
        p_pet->total_swim_exp = 0;
    } else {//收回
        player->fight_pet[p_pet->fight_pos() - 1] = NULL;
        p_pet->set_fight_pos(0);
    }
    PetUtils::save_pet(player, *p_pet, false, true);

    if (dst_pet) {
        dst_pet->set_fight_pos(0);
        PetUtils::save_pet(player, *dst_pet, false, true);
    }

    g_dbproxy->send_buf(0, player->userid, cache_cmd_set_user_info_outdate, 0, 0, 0);

    //如果在地图 则将精灵改变广播到地图
    MapUtils::sync_map_player_info(player, commonproto::PLAYER_FOLLOW_PET_CHANGE);

    //如果在打副本则同步给副本服务器
    if (player->temp_info.dup_id) {
        battleproto::cs_battle_relay btl_relay;
        btl_relay.set_uid(player->userid);
        btl_relay.set_create_tm(GET_A(kAttrCreateTm));
        btl_relay.set_cmd(cli_cmd_cs_0x0229_duplicate_switch_fight_pet);
        string str;
        onlineproto::cs_0x0229_duplicate_switch_fight_pet relay_msg;
        relay_msg.set_on_pet_create_tm(p_pet ?p_pet->create_tm() :0);
        relay_msg.set_off_pet_create_tm(dst_pet ?dst_pet->create_tm() :0);
        relay_msg.set_pos(p_pet ?p_pet->fight_pos() :0);
        relay_msg.SerializeToString(&str);
        btl_relay.set_pkg(str);
        DupUtils::send_to_battle(player, btl_cmd_msg_relay, btl_relay, NO_WAIT_SVR);
    }

    //回包
    send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    return 0;
}

//条件进化的构造函数
ConditionalPetEvolutionCmdProcessor::ConditionalPetEvolutionCmdProcessor()
{
    //将各种条件判断函数注册到func_map_中
    //func_map_[PET_ID_WO_LONG] = xxx_func;
}

int ConditionalPetEvolutionCmdProcessor::default_condition(player_t *player, Pet *pet)
{
    const pet_conf_t *pet_conf = PetUtils::get_pet_conf(pet->pet_id());
    assert(pet_conf);
    /*
    reduce_item_id_ = pet_conf->evolve_item;
    reduce_item_cnt_ = pet_conf->evolve_item_cnt;
    if (reduce_item_id_) {
        std::vector<reduce_item_info_t> reduce_vec;
        reduce_item_info_t reduce;
        reduce.item_id = reduce_item_id_;
        reduce.count = reduce_item_cnt_;
        int ret = check_swap_item_by_item_id(player, &reduce_vec, 0);
        return ret;
    }
    if (pet_conf->evolve_lv && pet->level() < pet_conf->evolve_lv) {
        return cli_err_pet_level_not_reach;
    }
    */
    if (pet_conf->evolve_talent && (uint32_t)pet->talent_level() < pet_conf->evolve_talent) {
        return cli_err_talent_level_not_reach;
    }
    return 0;
}

int ConditionalPetEvolutionCmdProcessor::after_evolve(player_t *player)
{
    /*
    if (reduce_item_id_ && reduce_item_cnt_) {
        reduce_single_item(player, reduce_item_id_, reduce_item_cnt_);
    }
    */
    return 0;
}

int ConditionalPetEvolutionCmdProcessor::check_condition(player_t* player, Pet* pet, uint32_t reason)
{
    if (func_map_.count(reason) == 0) {//使用默认的检测函数
        return this->default_condition(player, pet);
    }
    return (func_map_.find(reason)->second)(player, pet, reason);
}

//精灵条件进化
int ConditionalPetEvolutionCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    uint32_t create_tm = cli_in_.create_tm();
    Pet* pet = PetUtils::get_pet_in_loc(player, create_tm, PET_LOC_UNDEF);
    if (pet == NULL) {
        return  send_err_to_player(player, player->cli_wait_cmd, cli_err_pet_not_exist);
    }
    reduce_item_id_ = 0;
    reduce_item_cnt_ = 0;
    const pet_conf_t* pet_conf = PetUtils::get_pet_conf(pet->pet_id());
    uint32_t evolve_id = pet_conf->evolve_to; 

    int ret = check_condition(player, pet, cli_in_.reason());
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    ret = PetUtils::pet_evolve(player, pet, evolve_id, pet->level(), false,
            onlineproto::EVOLUTION_FROM_OTHER);
    if (ret) {
         return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    this->after_evolve(player);

    cli_out_.Clear();
    cli_out_.set_create_tm(create_tm);
    cli_out_.set_reason(cli_in_.reason());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

//条件加经验的构造函数
ConditionalPetAddExpCmdProcessor::ConditionalPetAddExpCmdProcessor()
{
    //将各种处理函数注册到func_map_中
    func_map_[(uint32_t)(onlineproto::PET_COND_ADD_EXP_150611_ACTIVITY)] = proc_add_exp_150611;
}

int ConditionalPetAddExpCmdProcessor::proc_add_exp_150611(
        player_t *player, Pet *pet, uint32_t reason)
{
    if (GET_A(kDailyPetLevelIncreaseExp150611) >= GET_A_MAX(kDailyPetLevelIncreaseExp150611)) {
        return cli_err_daily_activity_fini;
    }
    uint32_t n = pet->level();
    if (n >= kMaxPetLevel) {
        return cli_err_pet_level_max;
    }
 //   if (n >= GET_A(kAttrLv)) {
 //       return cli_err_pet_level_surpressed_by_player;
 //   }
    uint32_t add_exp = (5 * n * n * n + 100 * n) / 4;
    uint32_t real_add_exp = 0;
    int ret = PetUtils::add_pet_exp(player, pet, add_exp, real_add_exp);
    if (ret) {
        return ret;
    }
    ADD_A(kDailyPetLevelIncreaseExp150611, 1);
	//增加伙伴进修次数
	AttrUtils::add_attr_in_special_time_range(player,
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivPetAdvance);
    return 0;
}

int ConditionalPetAddExpCmdProcessor::proc_add_exp(player_t* player, Pet *pet, uint32_t reason)
{
    if (PetUtils::check_is_pet_level_max(pet)) {
        return cli_err_pet_level_max;
    }
    if (func_map_.count(reason) == 0) {
        return cli_err_pet_cond_add_exp_cond_err;
    }
    return ((func_map_.find(reason))->second)(player, pet, reason);
}

int ConditionalPetAddExpCmdProcessor::proc_pkg_from_client(player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    int ret = 0;
    uint32_t create_tm = cli_in_.create_tm();
    Pet* pet = PetUtils::get_pet_in_loc(player, create_tm, PET_LOC_UNDEF);
    if (pet == NULL) {
        return  send_err_to_player(player, player->cli_wait_cmd, cli_err_pet_not_exist);
    }
    ret = proc_add_exp(player, pet, (uint32_t)cli_in_.reason());
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

//精灵条件升级
ConditionalPetLevelUpCmdProcessor::ConditionalPetLevelUpCmdProcessor()
{
    //注册条件升级的检查函数
    func_map_[(uint32_t)(onlineproto::PET_COND_LEVEL_UP_150611_ACTIVITY)]   = proc_add_level_150611;
    func_map_[(uint32_t)(onlineproto::PET_COND_LEVEL_UP_ONE_KEY_MAX_LEVEL)] = proc_one_key_max_level;
    func_map_[(uint32_t)(onlineproto::PET_COND_LEVEL_UP_ONE_KEY_APPIONTED_LEVEL)] = proc_one_key_appionted_level;
    func_map_[(uint32_t)(onlineproto::PET_COND_LEVEL_UP_ONE_KEY_ONE_LEVEL )] = proc_one_key_one_level;
}

int ConditionalPetLevelUpCmdProcessor::proc_add_level_150611(
        player_t *player, Pet *pet, uint32_t reason, uint32_t &to_level)
{
    if (!is_vip(player)) {
        return cli_err_not_vip;
    }
    if (GET_A(kDailyPetLevelIncreaseLevel150611) >= GET_A_MAX(kDailyPetLevelIncreaseLevel150611)) {
        return cli_err_daily_activity_fini;
    }
    //if (pet->level() >= GET_A(kAttrLv)) {
    //    return cli_err_pet_level_surpressed_by_player;
    // }
    if (pet->level() >= kMaxPetLevel) {
        return cli_err_pet_level_max;
    }
    PetUtils::pet_level_up(player, pet, 1, false);
    ADD_A(kDailyPetLevelIncreaseLevel150611, 1);
    return 0;
}

int ConditionalPetLevelUpCmdProcessor::proc_one_key_one_level(
        player_t *player, Pet *pet, uint32_t reason, uint32_t &to_level)
{
    if (pet->level() >= kMaxPetLevel) {
        return cli_err_pet_level_max;
    }
	uint32_t flag = GET_A(kDailyPetLevelIncreaseOneLevel);
	std::vector<uint32_t> pet_list;
	pet_list.push_back(1160);
	pet_list.push_back(1018);
	pet_list.push_back(1168);
	pet_list.push_back(1144);
	pet_list.push_back(1182);
	for(uint32_t i = 0; i < pet_list.size();i++){
		if(pet->pet_id() != pet_list[i]) {

			if(i >= pet_list.size()) {
				return cli_err_exped_pet_create_tm_err;
			}		  
		}
		else {
			if(taomee::test_bit_on(flag, i+1)){
				return cli_err_pet_already_level_up;
			}
			else{
				to_level = pet->level()+1;  
				flag = taomee::set_bit_on(flag, i+1);
				SET_A(kDailyPetLevelIncreaseOneLevel, flag);
                break;				
			}
		}
	}
    return 0;
}
int ConditionalPetLevelUpCmdProcessor::proc_one_key_max_level(
        player_t *player, Pet *pet, uint32_t reason, uint32_t &to_level)
{
    if (pet->level() >= kMaxPetLevel) {
        return cli_err_pet_level_max;
    }
//	uint32_t player_level = GET_A(kAttrLv);
//    if (pet->level() >= player_level ) {
//        return cli_err_pet_level_surpressed_by_player;
//    }
    uint32_t level_limit = kMaxPetLevel;
    // uint32_t level_limit = player_level > kMaxPetLevel ?kMaxPetLevel : player_level;
	// uint32_t level_up_need_exp = PetUtils::get_level_up_to_n_need_exp(pet, level_limit);
	// uint32_t level_up = level_limit - pet->level();
	//物品id
	const uint32_t reduce_item_id  = 50101;
    uint32_t reduce_item_cnt = 0; 
	for(uint32_t level_up = pet->level() + 1; level_up <= level_limit; level_up++){
		reduce_item_cnt += ceil(level_up / 6.0);
		DEBUG_TLOG("leve_up :%d reduce_item_cnt : %d ",level_up, reduce_item_cnt );
	}
	attr_type_t attr_type = kServiceBuyExp100point;	

	int ret = buy_attr_and_use(player, attr_type, reduce_item_id, reduce_item_cnt);
	if(ret){
		return ret;
	}
	to_level = level_limit;
    return 0;
}
//伙伴升级到指定等级并且可以大于玩家等级
int ConditionalPetLevelUpCmdProcessor::proc_one_key_appionted_level(player_t *player, Pet *pet, uint32_t reason, uint32_t &to_level)
{

	if(!is_gold_vip(player)){
	    return cli_err_not_vip;
	}
    if (pet->level() >= kMaxPetLevel) {
        return cli_err_pet_level_max;
	}
	uint32_t flag = GET_A(kDailyPetLevelIncreaseAppointedLevel);
    uint32_t appion_level = 50;
    uint32_t level_tmp = pet->level() > appion_level ? pet->level() : appion_level;
	std::vector<uint32_t> pet_attr;
	pet_attr.push_back(1160);
	pet_attr.push_back(1018);
	pet_attr.push_back(1168);
	pet_attr.push_back(1144);
	pet_attr.push_back(1182);
//	std::vector<uint32_t>::iterator it;
//	it = std::find(pet_attr.begin(), pet_attr.end(), pet->pet_id());
//	bool found_pet = false;
//	if (it != pet_attr.end()) {
//		found_pet = true;
//	}
//	if (found_pet) {
//	}
    for(uint32_t i = 0; i < pet_attr.size();i++)
	{
	      if(pet->pet_id() != pet_attr[i])
	      {
			  if(i >= pet_attr.size())
			  {
		       return cli_err_exped_pet_create_tm_err;
              }		  
		  }
		  else {
             
			  if (taomee::test_bit_on(flag, i+1)){
			      return cli_err_pet_already_level_up;
			   }
			  else{
	               flag = taomee::set_bit_on(flag, i+1);
	               SET_A(kDailyPetLevelIncreaseAppointedLevel, flag);
	               break;			   
			  }
		  }
	}
	to_level = level_tmp;
    return 0;
}
int ConditionalPetLevelUpCmdProcessor::check_condition(player_t* player, Pet* pet, uint32_t reason, uint32_t &to_level)
{
    if (func_map_.count(reason) == 0) {
        return cli_err_pet_cond_level_up_cond_err;
    }
    return ((func_map_.find(reason))->second)(player, pet, reason, to_level);
}

int ConditionalPetLevelUpCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    uint32_t create_tm = cli_in_.create_tm();
    Pet* pet = PetUtils::get_pet_in_loc(player, create_tm, PET_LOC_UNDEF);
    if (pet == NULL) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_pet_not_exist);
    }

    uint32_t to_level = 0;
    int ret = check_condition(player, pet, (uint32_t)cli_in_.reason(), to_level);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    uint32_t add_level = 0;
    if (pet->level() >= to_level) {
        add_level = 0;
    } else {
        add_level = to_level - pet->level();
        if (to_level > kMaxPetLevel ) {
            return send_err_to_player(player, player->cli_wait_cmd, cli_err_pet_level_max);
        }
        //升级
        ret = PetUtils::pet_level_up(player, pet, add_level, false);
        if (ret) {
            return send_err_to_player(player, player->cli_wait_cmd, ret);
        }
    }
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

ConditionalPetSetTalentCmdProcessor::ConditionalPetSetTalentCmdProcessor()
{
    //注册条件设置天赋的函数
	func_map_[onlineproto::PET_COND_SET_TALENT_ONE_KEY_ADD_TALENT] = proc_one_key_add_talent;
}

int ConditionalPetSetTalentCmdProcessor:: proc_one_key_add_talent(player_t* player, Pet* pet, uint32_t reason, uint32_t &to_star)
{
	//伙伴星级
    assert(pet);
    if (pet->talent_level() >= kPetTalentLevelFull) {//达到最大
        return cli_err_talent_level_full;
    }

    // const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet->pet_id());
    // if (!pet_conf) {
        // return cli_err_pet_id_invalid;
    // }
    // const uint32_t default_item_cnt[kPetTalentLevelFive] = {
        // kOneLevelTalentItemCnt,
        // kTwoLevelTalentItemCnt,
        // kThreeLevelTalentItemCnt,
        // kFourLevelTalentItemCnt,
        // kFiveLevelTalentItemCnt
    // };

    // uint32_t item_id    = pet_conf->talent_item;
	// uint32_t total_cnt  = default_item_cnt[pet->talent_level()];
	// uint32_t usable_cnt = player->package->get_total_usable_item_count(item_id); 
    // reduce_item_info_t reduce;
    // reduce.item_id = item_id;
    // reduce.count = usable_cnt;

	//物品id
	const uint32_t reduce_item_id  = 50113;
	// const uint32_t reduce_item_cnt = total_cnt - usable_cnt;
	const uint32_t reduce_item_cnt = 1;

	attr_type_t attr_type = kServiceBuyPetStarUp;	
	int ret = buy_attr_and_use(player, attr_type, reduce_item_id, reduce_item_cnt);
	if (ret) {
		return ret;
	}

    // std::vector<reduce_item_info_t> reduce_vec;
    // reduce_vec.push_back(reduce);
	// //扣除已有的碎片
    // ret = swap_item_by_item_id(player, &reduce_vec, 0, false);
    // if (ret) {
        // return ret;
    // }


	to_star = pet->talent_level() + 1;
    return 0;
}

int ConditionalPetSetTalentCmdProcessor::check_condition(player_t* player, Pet* pet, uint32_t reason, uint32_t &to_star)
{
    if (func_map_.count(reason) == 0) {
        return cli_err_pet_cond_set_talent_cond_err;
    }
    return ((func_map_.find(reason))->second)(player, pet, reason, to_star);
}

//精灵条件设置天赋
int ConditionalPetSetTalentCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG; 
    uint32_t create_tm = cli_in_.create_tm();
    Pet* pet = PetUtils::get_pet_in_loc(player, create_tm, PET_LOC_UNDEF);
    if (pet == NULL) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_pet_not_exist);
    }

    uint32_t to_level = 0;
    int ret = check_condition(player, pet, cli_in_.reason(), to_level);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    //设置天赋
    ret = PetUtils::cond_set_talent_level(player, pet, to_level);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }
    cli_out_.Clear();
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

//条件增加学习力的构造函数
ConditionalPetAddEffortCmdProcessor::ConditionalPetAddEffortCmdProcessor()
{
    //将条件处理函数都注册到func_map_中
    //func_map_;
    func_map_[(uint32_t)(onlineproto::PET_COND_ADD_EFFORT_ONE_KEY_TO_MAX)] = proc_one_key_max_effort;
}

int ConditionalPetAddEffortCmdProcessor::proc_one_key_max_effort(
		player_t* player, Pet* pet, onlineproto::effort_alloc_data_t *alloc_data)
{
	//以后台为准
	alloc_data->Clear();
	//最大可获得的lv
	uint32_t level_limit = pet->level() > kMaxPetLevel ?kMaxPetLevel :pet->level();
	//获得主属性idx
	int idx = 0;
    const pet_conf_t *pet_conf = g_pet_conf_mgr.find_pet_conf(pet->pet_id());
    if (!pet_conf) {
        return cli_err_pet_id_invalid;
    }
	//成长率最高的idx
	const uint32_t rate[] = {4000, 1500, 600, 1500, 600}; 
	for(uint32_t i = 0; i < kMaxBattleValueTypeNum; i++){
		if(pet_conf->basic_normal_battle_values_grow[idx] / rate[idx] < 
				pet_conf->basic_normal_battle_values_grow[i] / rate[i]){
			idx = i;
		}
	}

	uint32_t cur_effort_lv = pet->effort_lv(idx);
	if (cur_effort_lv >= level_limit) {
		return 0;
	}

	int need_val  = PetUtils::get_effort_lv_up_to_n_need_val(pet, idx,
			level_limit);

	attr_type_t attr_type = kServiceBuyPetSpecialtraining;	
	//升主属性
	const uint32_t item_id = 50103;
	const uint32_t item_cnt = ceil(need_val*0.1);
    // DEBUG_TLOG("need_val : %d ", need_val);
	int ret = buy_attr_and_use(player, attr_type, item_id, item_cnt);
	if(ret){
		return ret;
	}

	uint32_t real_add = 0;
	PetUtils::add_pet_effort(player, pet, idx, need_val, real_add, false);
	return 0;
}

int ConditionalPetAddEffortCmdProcessor::proc_cond_add_effort(
        player_t *player, Pet *pet, uint32_t reason,
        onlineproto::effort_alloc_data_t *alloc_data)
{
    if (func_map_.count(reason) == 0) {
        return cli_err_pet_cond_add_effort_cond_err;
    }
    return ((func_map_.find(reason))->second)(player, pet, alloc_data);
}

int ConditionalPetAddEffortCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm(), PET_LOC_UNDEF);
    if (pet == NULL) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_pet_not_exist);
    }
   
    int ret = proc_cond_add_effort(player, pet, cli_in_.reason(), 
            cli_in_.mutable_alloc_data());
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int GetPetCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    cli_out_.Clear();
    DataProtoUtils::pack_player_pet_list(player, 
            cli_out_.mutable_pet_list(), (pet_location_type_t)cli_in_.loc());
    cli_out_.set_loc(cli_in_.loc());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int PetChiselCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    Pet *pet = PetUtils::get_pet_in_loc(player, cli_in_.create_tm(), PET_LOC_BAG);
    if (!pet) {
        return send_err_to_player(player, player->cli_wait_cmd, 
                cli_err_pet_not_in_bag);
    }
    //最多只有6个刻印孔
    if (!PetUtils::is_valid_chisel_pos(cli_in_.position())) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_pet_chisel_pos_invalid);
    }

    //卸下 if (pet->chisel_pos() == cli_in_.position()) {

    cli_out_.Clear();
    uint32_t target_create_tm = GET_A(((attr_type_t)(kAttrChisel1PetCreateTm + cli_in_.position() - 1)));
    Pet *target_pet = PetUtils::get_pet_in_loc(player, target_create_tm, PET_LOC_BAG);
    uint32_t chisel_pos = pet->chisel_pos();
    
    bool unload = false;
    if (PetUtils::is_valid_chisel_pos(chisel_pos)) {
        if (target_pet == pet) { //卸下
            pet->set_chisel_pos(0);
            unload = true;
        } else if (target_pet) {//两个刻印精灵位置互换
            target_pet->set_chisel_pos(chisel_pos);
            pet->set_chisel_pos(cli_in_.position());
            SET_A(((attr_type_t)(kAttrChisel1PetCreateTm + chisel_pos - 1)), target_pet->create_tm());
        } else {//刻印精灵换刻印孔
            pet->set_chisel_pos(cli_in_.position());
            SET_A(((attr_type_t)(kAttrChisel1PetCreateTm + chisel_pos - 1)), 0);
        }
    } else {
        if (target_pet) {//刻印精灵被替换下场
            target_pet->set_chisel_pos(0);
            pet->set_chisel_pos(cli_in_.position());
        } else {//新精灵刻印上场
            pet->set_chisel_pos(cli_in_.position());
        }
    }

    if (unload) {
        SET_A(((attr_type_t)(kAttrChisel1PetCreateTm + cli_in_.position() - 1)), 0);
    } else {
        SET_A(((attr_type_t)(kAttrChisel1PetCreateTm + cli_in_.position() - 1)), pet->create_tm());
    }

    // 更新新手任务
    if (PlayerUtils::get_chisel_pet_num(player) == 1) {
        SET_A(kAttrRookieGuide6PetDefend, 1);
    }

    cli_out_.set_type(pet->chisel_pos() ?1 :0);
    cli_out_.set_position(pet->chisel_pos());
    cli_out_.set_create_tm(pet->create_tm());
    PetUtils::save_pet(player, *pet, false, true);
    if (target_pet && target_pet != pet) {
        cli_out_.set_target_type(target_pet->chisel_pos() ?1 :0);
        cli_out_.set_target_position(target_pet->chisel_pos());
        cli_out_.set_target_create_tm(target_pet->create_tm());
        PetUtils::save_pet(player, *target_pet, false, true);
    }

    PlayerUtils::calc_player_battle_value(player);

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int PetCallBornCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    if (cli_in_.pet_conf_id() == 1167 || cli_in_.pet_conf_id() == 1168) {
        // 更新新手引导
        SET_A(kAttrRookieGuide4Pet, 1);
    }

    static uint32_t default_item_cnt[kPetTalentLevelFive] = {
        commonproto::CallOneLevelTalentItemCnt,
        commonproto::CallTwoLevelTalentItemCnt,
        commonproto::CallThreeLevelTalentItemCnt,
        commonproto::CallFourLevelTalentItemCnt,
        commonproto::CallFiveLevelTalentItemCnt
    };
    static uint32_t default_gold_cnt[kPetTalentLevelFive] = {
        commonproto::CallOneLevelTalentGoldCnt,
        commonproto::CallTwoLevelTalentGoldCnt,
        commonproto::CallThreeLevelTalentGoldCnt,
        commonproto::CallFourLevelTalentGoldCnt,
        commonproto::CallFiveLevelTalentGoldCnt
    };

    const pet_conf_t *pet_conf = PetUtils::get_pet_conf(cli_in_.pet_conf_id());
    if (!pet_conf) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_target_pet_not_exist);
    }
    uint32_t item_id = pet_conf->talent_item;
    if (!PetUtils::check_can_create_pet(player, cli_in_.pet_conf_id())) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_already_own_pet);
    }

    uint32_t need_gold = default_gold_cnt[pet_conf->born_talent-1];
    if (!AttrUtils::is_player_gold_enough(player, need_gold)) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_gold_not_enough);
    }

    int ret = reduce_single_item(player, item_id, default_item_cnt[pet_conf->born_talent-1]);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd,
                cli_err_call_pet_born_item_not_enough);
    }

    ret = AttrUtils::sub_player_gold(player, need_gold, "召唤"+pet_conf->name);
    if (ret) {
        send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    uint32_t create_tm;
    PetUtils::create_pet(player, cli_in_.pet_conf_id(), 1, false, &create_tm);

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int UpdateSkillCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    uint32_t parent_skill_id = cli_in_.parent_skill_id();
    const skill_conf_t *skill = g_skill_conf_mgr.find_skill_conf(parent_skill_id);
    if (skill == NULL) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_skill_id_not_exist);
    }

    // 检查是否拥有精灵,并取拥有精灵的最高等级
    PetUtils::update_pet_skill_by_id(player, parent_skill_id);

    // 更新新手引导
    uint32_t skill_num = PlayerUtils::get_equip_skill_num(player);
    if (skill_num == 1) {
        SET_A(kAttrRookieGuide1Skill, 1);
    } else if(skill_num == 2) {
        SET_A(kAttrRookieGuide3Skill, 1);
    }

    if(skill->skill_type == (uint32_t)commonproto::MAX_SKILL_NUM) {
        SET_A(kAttrRookieGuide5Skill, 1);
    }

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

//精灵条件设置品级
ConditionalPetAwakenCmdProcessor::ConditionalPetAwakenCmdProcessor()
{
    //注册条件升级的检查函数
    func_map_[(uint32_t)(onlineproto::PET_COND_AWAKEN_DEFAULT)] = proc_default_add_quality;
    func_map_[(uint32_t)(onlineproto::PET_COND_AWAKEN_ONE_KEY_QUALITY_LEVEL_UP)] = proc_one_key_add_quality;
}

int ConditionalPetAwakenCmdProcessor::proc_default_add_quality(
        player_t *player, Pet *pet, const pet_quality_conf_t * pq_conf,
		uint32_t reason, uint32_t &to_quality)
{
    const pet_conf_t *pet_conf = PetUtils::get_pet_conf(pet->pet_id());

    //消耗物品
    std::vector<reduce_item_info_t> cost_item(pq_conf->cost_item);
    int ret = swap_item_by_item_id(player, &cost_item, 0, false);
    if (ret) {
        return ret;
    }

    //扣金币
    ret = AttrUtils::sub_player_gold(player, pq_conf->cost_gold, "觉醒" + pet_conf->name); 
    if (ret) {
        return ret;
    }

	to_quality = pq_conf->next_id;
    return 0;
}

int ConditionalPetAwakenCmdProcessor::proc_one_key_add_quality(
        player_t *player, Pet *pet, const pet_quality_conf_t * pq_conf,
		uint32_t reason, uint32_t &to_quality)
{
    const uint32_t id = pq_conf->id < 1? 1 : pq_conf->id;
	const uint32_t data[][2] = {
		{50104, kServiceBuyPetGreen},
		{50105, kServiceBuyPetGreen1},
		{50106, kServiceBuyPetBlue},
		{50107, kServiceBuyPetBlue1},
		{50108, kServiceBuyPetBlue2},
		{50109, kServiceBuyPetPurple},
		{50110, kServiceBuyPetPurple1},
		{50111, kServiceBuyPetPurple2},
		{50112, kServiceBuyPetPurple3},
	};
	const uint32_t reduce_item_id  = data[id - 1][0];
	const uint32_t reduce_item_cnt = 1;
	attr_type_t attr_type = (attr_type_t)data[id - 1][1];	

	int ret = buy_attr_and_use(player, attr_type, reduce_item_id, reduce_item_cnt);
	if(ret){
		return ret;
	}
	to_quality = pq_conf->next_id;
    return 0;
}

int ConditionalPetAwakenCmdProcessor::check_condition(player_t* player,
	   Pet* pet, const pet_quality_conf_t * pq_conf, uint32_t reason, 
	   uint32_t &to_quality)
{
    if (func_map_.count(reason) == 0) {
        return cli_err_pet_cond_level_up_cond_err;
    }
    return ((func_map_.find(reason))->second)(player, pet, pq_conf, reason, to_quality);
}

int ConditionalPetAwakenCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    // 更新新手引导
    SET_A(kAttrRookieGuide9PetWakenUp, 1);

    // 检查条件
    uint32_t create_tm = cli_in_.create_tm();
    Pet* pet = PetUtils::get_pet_in_loc(player, create_tm, PET_LOC_UNDEF);
    if (pet == NULL) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_pet_not_exist);
    }

    const pet_conf_t *pet_conf = PetUtils::get_pet_conf(pet->pet_id());
    if (pet_conf == NULL) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_pet_not_exist);
    }

    const pet_quality_conf_t * pq_conf = 
        g_pet_quality_conf_mgr.get_pet_quality_conf(pet_conf->waken_type, pet->quality());
    if (pq_conf == NULL) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_illegal_pet_quality);
    } 

    if (pq_conf->next_id > commonproto::MAX_PET_QUALITY) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_over_max_pet_quality);
    }

    if (pet->quality() == commonproto::MAX_PET_QUALITY) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_over_max_pet_quality);
    }

    if (pet->level() < pq_conf->pet_level) {
        return send_err_to_player(player, player->cli_wait_cmd, cli_err_pet_level_not_reach);
    }

    uint32_t to_quality = 0;
    int ret = check_condition(player, pet, pq_conf, (uint32_t)cli_in_.reason(), to_quality);
    if (ret) {
        return send_err_to_player(player, player->cli_wait_cmd, ret);
    }

    // 更新悬赏任务
    TaskUtils::update_task_condition_attr(player, REWARD_TASK_ITEM_PET_WAKEN_UP, 1);

    // 完成觉醒
    onlineproto::sc_0x032C_pet_awaken_notify noti_msg;
    noti_msg.Clear();
    commonproto::pet_info_t *old_pet_info = noti_msg.mutable_old_pet_info();
    DataProtoUtils::pack_player_pet_info(player, pet, old_pet_info);

    pet->set_quality(to_quality);
    pet->calc_battle_value(player);
    pet->set_hp(pet->max_hp());
    PetUtils::save_pet(player, *pet, false, true, true, onlineproto::POWER_CHANGE_FROM_OTHER); 

    commonproto::pet_info_t *new_pet_info = noti_msg.mutable_new_pet_info();
    DataProtoUtils::pack_player_pet_info(player, pet, new_pet_info);
    noti_msg.set_create_tm(pet->create_tm());

    // 通知前端
    send_msg_to_player(player, cli_cmd_cs_0x032C_pet_awaken_notify, noti_msg);

	//成就监听
//			ACH_07_GET_QUALITY_PET
	AttrUtils::add_attr_in_special_time_range(player,
			TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
			kAttrActivPetUpQualityCnt);

    cli_out_.Clear();
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

void PetOpBeginCmdProcessor::register_proc_func()
{
	op_begin_func_map_[(uint32_t)HM_PET_EXERCISE] = pet_exercise_begin;
	op_begin_func_map_[(uint32_t)HM_PET_FIND_BOX] = pet_find_box_begin;
}

int PetOpBeginCmdProcessor::proc_pkg_by_pet_op(player_t* player, 
		Pet* pet, uint32_t pos, uint32_t op_type)
{
	if (op_begin_func_map_.count(op_type) == 0) {
		return cli_err_hm_pet_op_type_err;
	}
	return (op_begin_func_map_.find(op_type)->second)(player, pet, pos);
}

uint32_t PetOpBeginCmdProcessor::pet_find_box_begin(
		player_t* player, Pet* pet, uint32_t pos)
{
	if (pet == NULL) {
		return cli_err_pet_not_exist;
	}
	uint32_t pet_create_tm = pet->create_tm();
	//如果是3，4，5格子，需要判断相关的格子是否已经开启
	if (pos >= 3 && 
			GET_A(attr_type_t((uint32_t)kAttrFindBoxPos3IsOpen + pos - 3)) == 0) {
		return cli_err_exercise_pos_not_activation;
	}
	//该位置当前应该是空置的
	if (GET_A(attr_type_t((uint32_t)kAttrFindBoxPetCreateTm1 + pos - 1))) {
		return cli_err_exercise_pos_has_been_occupation;
	}

	//该伙伴当前应该是空闲的
	if (pet->exercise_tm()) {
		return cli_err_pet_already_exercise;
	}
	uint32_t item_id = HM_NORNAL_KEY;
	std::vector<uint32_t> items_vec;
	uint32_t first_rate, second_rate, third_rate;
	//宝箱
	uint32_t pet_power = pet->power();
	if (rand() % 100 < HM_FOUND_BOX_RATE) {
		third_rate = (pet_power / (80000 * 1.0)) * 100;
		second_rate = (pet_power / (28000 * 1.0)) * 100;
		first_rate = 100 - second_rate - third_rate;
		items_vec.push_back(HM_NORMAL_BOX);
		items_vec.push_back(HM_BRONZE_BOX);
		items_vec.push_back(HM_GOLD_BOX);
	} else {
		//钥匙
		third_rate = (pet_power / (250000 * 1.0)) * 100;
		second_rate = (pet_power / (56000 * 1.0)) * 100;
		first_rate = 100 - second_rate - third_rate;
		items_vec.push_back(HM_NORNAL_KEY);
		items_vec.push_back(HM_BRONZE_KEY);
		items_vec.push_back(HM_GOLD_KEY);
	}

	std::set<uint32_t> display_hit_idx_set;
	std::map<uint32_t, uint32_t> award_rate_map;
	award_rate_map[0] = first_rate;
	award_rate_map[1] = second_rate;
	award_rate_map[2] = third_rate;
	Utils::rand_select_uniq_m_from_n_with_r(award_rate_map, display_hit_idx_set, 1);
	if (display_hit_idx_set.size() == 1) {
		std::set<uint32_t>::iterator it = display_hit_idx_set.begin();
		uint32_t idx = *it;
		//这里idx范围判断是确保数组不越界
		if (idx >= 0 && idx <= 2) {
			item_id = items_vec[idx];
		}
	}
	//判断当前vip位是否都开启
	bool all_open_flag = true;
	for (uint32_t i = 0; i < 3; ++i) {
		if (GET_A(attr_type_t((uint32_t)kAttrFindBoxPos3IsOpen + i)) == 0) {
			all_open_flag = false;
			break;
		}
	}
	//计算当前已经有几个伙伴在寻宝
	uint32_t find_box_pet_count = 0;
	for (uint32_t i = 0; i < 5; ++i) {
		if (GET_A(attr_type_t((uint32_t)kAttrFindBoxPetCreateTm1 + i))) {
			++find_box_pet_count;
		}
	}
	if (all_open_flag) {
		if (find_box_pet_count >= 4) {
			item_id = HM_GOLD_BOX;
		} else if (find_box_pet_count >= 2) {
			item_id = HM_BRONZE_BOX;
		}
	}
	SET_A(attr_type_t((uint32_t)kAttrFindBoxPetCreateTm1 + pos - 1), pet_create_tm);
	SET_A(attr_type_t((uint32_t)kAttrFindBoxPos1Item + pos - 1), item_id);
	return 0;
}

uint32_t PetOpBeginCmdProcessor::pet_exercise_begin(
		player_t* player, Pet* pet, uint32_t pos)
{
	if (pet == NULL) {
		return cli_err_pet_not_exist;
	}
	uint32_t pet_create_tm = pet->create_tm();
	if (pos >= 3 && 
			GET_A(attr_type_t((uint32_t)kAttrExercisePos3IsOpen + pos - 3)) == 0) {
		return cli_err_exercise_pos_not_activation;
	}
	if (GET_A(attr_type_t((uint32_t)kAttrExercisePetCreateTm1 + pos - 1))) {
		return cli_err_exercise_pos_has_been_occupation;
	}

	//如果该伙伴已经在操作中，则返回错误码
	if (pet->exercise_tm()) {
		return cli_err_pet_already_exercise;
	}
	SET_A(attr_type_t((uint32_t)kAttrExercisePetCreateTm1 + pos - 1), pet_create_tm);

	return 0;
}

int PetOpBeginCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
	PARSE_MSG;
	//坑位初始化
	uint32_t pos = cli_in_.position();
	if (!(pos >= 1 && pos <= 5)) {
		//return
		RET_ERR(cli_err_exercise_pos_invalid);
	}
	Pet *pet = PetUtils::get_pet_in_loc(player, cli_in_.create_time());
	if (pet == NULL) {
		//return
		RET_ERR(cli_err_pet_not_exist);
	}
	uint32_t op_type = cli_in_.op_type();
	uint32_t ret = this->proc_pkg_by_pet_op(player, pet, pos, op_type);
	if (ret) {
		//return
		RET_ERR(ret);
	}
	//操作开始的时间
	pet->set_exercise_tm(NOW());
	uint16_t tmp_op_type = op_type;
	uint16_t tmp_pos = pos;
	uint32_t type_and_pos = Utils::comp_u16(tmp_op_type, tmp_pos);
	
	pet->set_exercise_pos(type_and_pos);
	PetUtils::save_pet(player, *pet, false, true);
	//通知在我小屋里的其他玩家,必须要在save_pet之后调用
	if (op_type == HM_PET_EXERCISE) {
		PetUtils::sync_pet_exercise_stat_to_hm(player, pet);
	}
	cli_out_.Clear();
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

void PetRecallCmdProcessor::register_proc_func()
{
	pet_recall_func_map_[(uint32_t)HM_PET_EXERCISE] = pet_exercise_recall;
	pet_recall_func_map_[(uint32_t)HM_PET_FIND_BOX] = pet_find_box_recall;
}

int PetRecallCmdProcessor::proc_pkg_when_pet_recall(player_t* player, Pet* pet,
		uint32_t pos, uint32_t op_type,
		onlineproto::sc_0x032F_pet_op_end& cli_out)
{
	if (pet_recall_func_map_.count(op_type) == 0) {
		return cli_err_hm_pet_op_type_err;
	}
	return (pet_recall_func_map_.find(op_type)->second) (player, pet, pos, cli_out);
}

int PetRecallCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
	uint32_t create_tm = cli_in_.create_time();
	uint32_t pos = cli_in_.position();
	uint32_t op_type = cli_in_.op_type();
	if (!(pos >= 1 && pos <= 5)) {
		//return
		RET_ERR(cli_err_exercise_pos_invalid);
	}
	Pet *pet = PetUtils::get_pet_in_loc(player, create_tm);
	if (pet == NULL) {
		//return
		RET_ERR(cli_err_pet_not_exist);
	}
	//精灵应该在操作中
	if (pet->exercise_tm() == 0) {
		//return
		RET_ERR(cli_err_pet_has_stop_exercice);
	}
	//判断操作类型以及位置 与 后台保存的是否一致
	uint16_t high = 0, low = 0;
	Utils::decomp_u16(pet->exercise_pos(), high, low);
	if (!(high == op_type && low == pos)) {
		//return
		RET_ERR(cli_err_hm_pet_op_type_err);
	}
	cli_out_.Clear();
	uint32_t ret= proc_pkg_when_pet_recall(player, pet, pos, op_type, cli_out_);
	if (ret) {
		//return
		RET_ERR(ret);
	}
	pet->set_exercise_tm(0);
	pet->set_exercise_pos(0);
	PetUtils::save_pet(player, *pet, false, true);

	//通知在我小屋里的其他玩家
	if (op_type == HM_PET_EXERCISE) {
		PetUtils::sync_pet_exercise_stat_to_hm(player, pet);
	}
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

uint32_t PetRecallCmdProcessor::pet_find_box_recall(player_t* player, Pet* pet, 
		uint32_t pos, onlineproto::sc_0x032F_pet_op_end& cli_out)
{
	if (pet == NULL) {
		return cli_err_pet_not_exist;
	}
	uint32_t pet_create_tm = pet->create_tm();
	if (pos >= 3 && 
			GET_A(attr_type_t((uint32_t)kAttrFindBoxPos3IsOpen + pos - 3)) == 0) {
		return cli_err_exercise_pos_not_activation;
	}
	if (GET_A(attr_type_t((uint32_t)kAttrFindBoxPetCreateTm1 + pos - 1)) != pet_create_tm) {
		return cli_err_hm_pet_not_in_this_pos;
	}
	//寻宝操作2小时必须物品
	uint32_t real_count = 0;
	uint32_t pass_time = NOW() - pet->exercise_tm();
	uint32_t need_times = 4 * 3600;
	if (is_gold_vip(player)) {
		need_times = 2 * 3600;
	} else if (is_silver_vip(player)) {
		need_times = 3 * 3600;
	}
	if (pass_time > need_times) {
		real_count = 1;
	}
	uint32_t item_id = 0;
	if (real_count) {
        std::vector<add_item_info_t> add_vec;
        add_item_info_t item_info;
        item_info.item_id = GET_A((attr_type_t)((uint32_t)kAttrFindBoxPos1Item + pos - 1));
		item_id = item_info.item_id;
        item_info.count = real_count;
        add_vec.push_back(item_info);
        swap_item_by_item_id(player, NULL, &add_vec, false);    
		ADD_A(kAttrHmFindBoxCount, 1);
    }
	SET_A(attr_type_t((uint32_t)kAttrFindBoxPos1Item + pos - 1), 0);
	SET_A(attr_type_t((uint32_t)kAttrFindBoxPetCreateTm1 + pos - 1), 0);
	cli_out.set_op_type(onlineproto::HM_PET_FIND_BOX);
	cli_out.set_find_item(item_id);
	return 0;
}

uint32_t PetRecallCmdProcessor::pet_exercise_recall(player_t* player, Pet* pet,
		uint32_t pos, onlineproto::sc_0x032F_pet_op_end& cli_out)
{
	if (pet == NULL) {
		return cli_err_pet_not_exist;
	}
	uint32_t pet_create_tm = pet->create_tm();
	if (pos >= 3 && 
			GET_A(attr_type_t((uint32_t)kAttrExercisePos3IsOpen + pos - 3)) == 0) {
		return cli_err_exercise_pos_not_activation;
	}
	if (GET_A(attr_type_t((uint32_t)kAttrExercisePetCreateTm1 + pos - 1)) != pet_create_tm) {
		return cli_err_hm_pet_not_in_this_pos;
	}

	uint32_t pass_time = NOW() - pet->exercise_tm();
	//锻炼过程的时间最大12小时
	uint32_t duration = commonproto::HM_PET_EXERCISE_DURATION;
	if (pass_time > duration) {
		pass_time = duration;
	}
	uint32_t add_exp = 0, rate = 1;
	if (is_gold_vip(player)) {
		rate = 3;
	} else if (is_silver_vip(player)) {
		rate = 2;
	}
	add_exp = ceil(rate * pet->level() * pass_time / (commonproto::HM_TIME_INTER * 1.0));
	uint32_t real_add_exp;
	PetUtils::add_pet_exp(player, pet, add_exp, 
			real_add_exp, ADDICT_DETEC, onlineproto::EXP_FROM_EXERCISE);
	SET_A((attr_type_t)((uint32_t)kAttrExercisePetCreateTm1 + pos - 1), 0);
	cli_out.set_op_type(onlineproto::HM_PET_EXERCISE);
	cli_out.set_add_exp(real_add_exp);

	return 0;
}

int OpenExerciseBoxCmdProcessor::proc_pkg_from_client(
		player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
	uint32_t key_id = 0, box_id = 0, prize_id = 0;
	if (cli_in_.box_type() == onlineproto::NORMAL_EXER_BOX) {
		key_id = HM_NORNAL_KEY; box_id = HM_NORMAL_BOX; prize_id = 9981;
	} else if (cli_in_.box_type() == onlineproto::BRONZE_EXER_BOX) {
		key_id = HM_BRONZE_KEY; box_id = HM_BRONZE_BOX; prize_id = 9982;
	} else if (cli_in_.box_type() == onlineproto::GOLD_EXER_BOX) {
		key_id = HM_GOLD_KEY; box_id = HM_GOLD_BOX; prize_id = 9983;
	}
	//判断物品是否足够
	std::vector<reduce_item_info_t> reduce_vec;
	reduce_item_info_t reduce_item;
	reduce_item.item_id = key_id; 
	reduce_item.count = 1;
	reduce_vec.push_back(reduce_item);
	reduce_item.item_id = box_id; reduce_item.count = 1;
	reduce_vec.push_back(reduce_item);
	int ret = swap_item_by_item_id(player, &reduce_vec, 0, false);
	if (ret) {
		RET_ERR(ret);
	}

	//
	onlineproto::sc_0x0112_notify_get_prize noti_prize_msg;
	transaction_proc_prize(player, prize_id, 
			noti_prize_msg,
			commonproto::PRIZE_REASON_ESCORT_RESULT_REWARD,
			onlineproto::SYNC_REASON_PRIZE_ITEM);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int DijuLightLampCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
    PARSE_MSG;
	uint32_t pet_create_tm = cli_in_.pet_create_tm();
	Pet* pet = PetUtils::get_pet_in_loc(player, pet_create_tm);
	if (pet == NULL) {
		RET_ERR(cli_err_bag_pet_not_exist);
	}
	if (!(cli_in_.lamp_id() >= 1 && cli_in_.lamp_id() <= 5)) {
		RET_ERR(cli_err_lamp_type_err);
	}
	commonproto::pet_optional_attr_t pb_opt_attr;
	pb_opt_attr.ParseFromString(pet->pet_opt_attr);
	std::string debug_str = pb_opt_attr.Utf8DebugString();
	std::string name = pb_opt_attr.GetTypeName();
	TRACE_TLOG("DijuLightLamp:'%s'\nmsg:[%s]\n",
			name.c_str(), debug_str.c_str());
	const commonproto::lamp_info_list_t& pb_list = pb_opt_attr.lamp();
	uint32_t old_lamp_lv = 0;
	if (pb_list.lamp_list_size()) {
		const commonproto::lamp_info_t& pb_info =  pb_list.lamp_list(0);
		//判断灯泡能否升级
		uint32_t ret = PetUtils::check_lamp_lv_condition(player,
				pet, pb_info, cli_in_.lamp_id(), old_lamp_lv);
		if (ret) {
			RET_ERR(ret);
		}
	}
	uint32_t need_consume_gold = floor(((old_lamp_lv + 120) * old_lamp_lv * old_lamp_lv + 1000) * 0.6);
	if (!AttrUtils::is_player_gold_enough(player, need_consume_gold)) {
		RET_ERR(cli_err_gold_not_enough);
	}
	AttrUtils::sub_player_gold(player, need_consume_gold, "帝具升级");
	commonproto::lamp_info_list_t* pb_list_ptr = pb_opt_attr.mutable_lamp();
	commonproto::lamp_info_t* pb_lamp_ptr;
	if (pb_list_ptr->lamp_list_size() == 0) {
		pb_lamp_ptr = pb_list_ptr->add_lamp_list();
	} else {
		pb_lamp_ptr = pb_list_ptr->mutable_lamp_list(0);
	}
	PetUtils::lamp_lv_up(player, cli_in_.lamp_id(), *pb_lamp_ptr);

	debug_str.clear();
	name.clear();
	debug_str = pb_opt_attr.Utf8DebugString();
	name = pb_opt_attr.GetTypeName();
	TRACE_TLOG("Pet Opt Attr:'%s'\nmsg:[%s]\n",
			name.c_str(), debug_str.c_str());

	pb_opt_attr.SerializeToString(&pet->pet_opt_attr);
	PetUtils::save_pet(player, *pet, false, true);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	
	/*
	uint32_t lamp_state = 0;
	if (pb_opt_attr.lamp_state_size() == 0) {
		pb_opt_attr.add_lamp_state(lamp_state);
	}

	uint32_t pb_size = pb_opt_attr.lamp_state_size();
	//阶段错误
	if (pb_size < cli_in_.stage()) {
		RET_ERR(cli_err_diju_cli_stage_val_err);
	}

	lamp_state = pb_opt_attr.lamp_state(cli_in_.stage() - 1);
	TRACE_TLOG("DiJu Lamp Level Up, old_lamp_state=[%u]", lamp_state);
	//判断这个灯是否能点
	std::map<uint32_t, uint32_t> lamp_level_map;
	std::map<uint32_t, uint32_t>::iterator it;
	PetUtils::convert_pet_lamp_state_to_level_map(lamp_state, lamp_level_map);
	if (lamp_level_map.count(cli_in_.lamp_id()) == 0 && cli_in_.lamp_id() > 1) {
		if (lamp_level_map.count(cli_in_.lamp_id() - 1) == 0) {
			RET_ERR(cli_err_must_light_the_lamb_before);
		}
	}
	uint32_t level = lamp_level_map[cli_in_.lamp_id()];
	if (level == 3) {
		RET_ERR(cli_err_this_light_level_has_get_limit);
	}
	lamp_level_map[cli_in_.lamp_id()] = level + 1;

	uint32_t new_lamp_state = 0;
	PetUtils::convert_level_map_to_pet_lamp_state(lamp_level_map, new_lamp_state);
	//根据等级扣金币
	uint32_t conf1[] = {2200, 2200, 2200, 2200, 2200, 5100};
	uint32_t conf = conf1[cli_in_.lamp_id() - 1];
	uint32_t level_conf = level * 5;
	if (level == 0) {
		level_conf = 1;
	}
	uint32_t need_consume_gold = conf * (1 + (cli_in_.stage() - 1) * 0.2) * level_conf;
	TRACE_TLOG("DiJu,Lamp Level Up:need_gold=[%u],conf=[%u],level=[%u]",
			need_consume_gold, conf, level);

	if (!AttrUtils::is_player_gold_enough(player, need_consume_gold)) {
		RET_ERR(cli_err_gold_not_enough);
	}
	AttrUtils::sub_player_gold(player, need_consume_gold, "帝具升级");
	pb_opt_attr.set_lamp_state(cli_in_.stage() - 1, new_lamp_state);

	debug_str.clear();
	name.clear();
	debug_str = pb_opt_attr.Utf8DebugString();
	name = pb_opt_attr.GetTypeName();
	TRACE_TLOG("Pet Opt Attr:'%s'\nmsg:[%s]\n",
			name.c_str(), debug_str.c_str());

	pb_opt_attr.SerializeToString(&pet->pet_opt_attr);
	PetUtils::save_pet(player, *pet, false, true);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
	*/
}

/*
int DijuUpStageCmdProcessor::proc_pkg_from_client(
		player_t* player, const char* body, int bodylen)
{
	//目前最高的为六阶
	PARSE_MSG;
	uint32_t pet_create_tm = cli_in_.pet_create_tm();
	Pet* pet = PetUtils::get_pet_in_loc(player, pet_create_tm);
	if (pet == NULL) {
		RET_ERR(cli_err_bag_pet_not_exist);
	}
	commonproto::pet_optional_attr_t pb_opt_attr;
	pb_opt_attr.ParseFromString(pet->pet_opt_attr);
	uint32_t cur_stage = pb_opt_attr.lamp_state_size();
	if (cur_stage == 0) {
		RET_ERR(cli_err_diju_up_stage_invalid);
	}
	std::map<uint32_t, uint32_t> lamp_level_map;
	PetUtils::convert_pet_lamp_state_to_level_map(
			pb_opt_attr.lamp_state(cur_stage - 1), lamp_level_map);
	//当前灯需要全部点亮，方可升阶
	FOREACH(lamp_level_map, it) {
		TRACE_TLOG("Old Stage Lamp State, lamp_id=[%u],level=[%u]",
				it->first, it->second);
	}
	if (lamp_level_map.size() < 6) {
		RET_ERR(cli_err_diju_up_stage_must_light_all_lamp);
	}
	uint32_t next_stage = cur_stage + 1;
	if (GET_A(kAttrLv) < (next_stage - 1) * 10) {
		RET_ERR(cli_err_level_too_low);
	}
	if (cur_stage >= 6) {
		RET_ERR(cli_err_diju_stage_get_limit);
	}
	pb_opt_attr.add_lamp_state(0);
	pet->pet_opt_attr.clear();
	pb_opt_attr.SerializeToString(&pet->pet_opt_attr);
	PetUtils::save_pet(player, *pet, false, true);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
*/

int PetDijuAwakeCmdProcessor::proc_pkg_from_client(player_t* player,
		const char* body, int bodylen)
{
	PARSE_MSG;
	Pet* pet = PetUtils::get_pet_in_loc(player, cli_in_.pet_create_tm());
	if (pet == NULL) {
		RET_ERR(cli_err_bag_pet_not_exist);
	}
	commonproto::pet_optional_attr_t pb_opt_attr;
	pb_opt_attr.ParseFromString(pet->pet_opt_attr);
	if (pb_opt_attr.awake_state()) {
		RET_ERR(cli_err_pet_diju_has_awaked);
	} 
	//判断是否通关帝具副本所有的关卡
	pet_pass_dup_conf_t* conf_ptr = g_pet_pass_dup_conf_mgr.get_pet_pass_dup_info_conf(
			pet->pet_id());
	if (conf_ptr == NULL) {
		RET_ERR(cli_err_pet_not_join_pass_dup_activity);
	}
	std::vector<dup_shop_id_t>& dup_shop_id_vec = conf_ptr->dup_shopid_map[PET_PASS_DUP_DIJU_AWAKE];
	FOREACH(dup_shop_id_vec, it) {
		uint32_t type = 0;
		uint32_t ret = AttrUtils::get_pet_pass_dup_tm_attr(
				it->dup_id, pet->pet_id(), type);
		if (ret) {
			RET_ERR(ret);
		}
		if (GET_A((attr_type_t)type) == 0) {
			ERROR_TLOG("dup_attr_id,%u", type);
			RET_ERR(cli_err_diju_awake_cond_not_get);
		}
	}
	//觉醒帝具
	pb_opt_attr.set_awake_state(1);
	pb_opt_attr.SerializeToString(&pet->pet_opt_attr);
    PetUtils::save_pet(player, *pet, false, true);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
