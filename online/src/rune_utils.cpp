#include "rune_utils.h"
#include "player.h"
#include "global_data.h"
#include "data_proto_utils.h"
#include "proto/db/dbproto.rune.pb.h"
#include "service.h"
#include "common.h"
#include "pet_utils.h"

uint32_t RuneUtils::get_rune(player_t *player, const uint32_t rune_id, rune_t& rune)
{
	return player->rune_meseum->get_rune(rune_id, rune);
}

uint32_t RuneUtils::get_rune_exp(uint32_t rune_type, uint32_t level, uint32_t& exp)
{
    if (rune_type == kRuneGray || rune_type == kRuneRed) {
        return cli_err_rune_type_no_exp;
    }
    if (level > kMaxRuneLv || level < 2) {
        return cli_err_rune_update_exceed_limit;
    }
	uint32_t up_level = level + 1;
	/*获得up_level等级对应的经验值*/
	int ret = g_rune_exp_conf_mgr.get_exp_by_level_from_rune_exp_conf(rune_type, up_level, exp);
	if (ret != 0) {
		return ret;
	}
    return 0;
}

uint32_t RuneUtils::up_rune_grade_by_exp(
		player_t *player, uint32_t exp, rune_t& rune, uint32_t& bottle_exp)
{
	bottle_exp = AttrUtils::get_attr_value(player, kAttrRuneExpBottle);
	if (exp > bottle_exp) {
		ERROR_TLOG("uid=[%u],bottle lack exp:exp=[%u],bottle_exp=[%u]", 
				player->userid, exp, bottle_exp);
		return cli_err_rune_exp_less;
	}
	rune_conf_t rune_conf;
	/*获得符文配表信息*/
	int ret = RuneUtils::get_rune_conf_data(rune.conf_id, rune_conf);
    if (ret) {
        return ret;
    }
	/*符文能量不能升级*/
	if (rune_conf.rune_type == kRuneRed || rune_conf.rune_type == kRuneGray) {
		return cli_err_red_or_gray_can_not_upgrade;
	}

	bottle_exp -= exp;
	attr_data_info_t attr = { 
		kAttrRuneExpBottle, bottle_exp 
	};
	rune.exp += exp;
	uint32_t tmp_level;
	RuneUtils::get_rune_level_by_exp(rune.exp, rune_conf.rune_type, tmp_level);
	if (tmp_level < rune.level) {
		ERROR_TLOG("uid=[%u]:upgrade failed:[%u][%u]", 
				player->userid, tmp_level, rune.level);
		return cli_err_update_rune_failed;
	}
	rune.level = tmp_level;
	/*保存经验瓶*/
	AttrUtils::set_attr_value(player, 1, &attr, false);
	return 0;
}

uint32_t RuneUtils::get_rune_conf_data(
		uint32_t conf_id, rune_conf_t& rune_conf)
{
	rune_conf_t* rune_info = g_rune_conf_mgr.get_rune_conf_t_info(conf_id);
	if (rune_info == NULL) {
		return cli_err_rune_conf_id_not_exist;
	}
	rune_conf = *rune_info;
	return 0;
}

//
int RuneUtils::check_equit_rune(player_t *player, Pet* pet, rune_t& rune, uint32_t pos) 
{
	/*非法数据*/
	if (pet->get_rune_array(pos) == -1) {
		return cli_err_data_error;
	}
	/*位置已经被占*/
	if (pet->get_rune_array(pos) != 0) {
		return cli_err_pos_already_equip_rune;
	}
	
	/*已装备有相同功能符文*/
	uint32_t ret = check_runes_func_same_or_not(player, rune.id, pet);
	if (ret) {
		return ret;
	}
	return 0;
}

/*@根据背包类型，判断该背包是否已满
 */
uint32_t RuneUtils::judge_rune_full_by_packet_type(player_t *player, rune_pack_type_t pack_type)
{
	uint32_t count = player->rune_meseum->get_rune_num_by_packet_type(pack_type);
	uint32_t rune_count_limit = get_rune_limit_count_by_pack_type(player, pack_type);
	if (pack_type == kCollectPack) {
		uint32_t open_num = GET_A(kAttrOpenRunePosNum);
		rune_count_limit += open_num;
	}
	TRACE_TLOG("Tr_rune count get limit:uid=[%u],count=[%u],limit=[%u],pack_type=[%u]", 
			player->userid, count, rune_count_limit, (uint32_t)pack_type);
	if (count >= rune_count_limit) {
		ERROR_TLOG("rune count get limit:uid=[%u],count=[%u],limit=[%u],pack_type=[%u]", 
				player->userid, count, rune_count_limit, (uint32_t)pack_type);
		if (kTransferPack == pack_type) {
			return cli_err_rune_pack_full;
		} else if (kCollectPack == pack_type) {
			return cli_err_rune_collect_pack_full;
		} else {
			return cli_err_rune_tmp_pack_full;
		}
	}
	return 0;
}

/*@根据符文经验，获得当前的等级
 */
uint32_t RuneUtils::get_rune_level_by_exp(uint32_t rune_exp, uint32_t rune_type, uint32_t& rune_level)
{
	rune_exp_conf_t* runeExp_ptr = g_rune_exp_conf_mgr.get_rune_exp_conf_t_info(rune_type);
	if (runeExp_ptr == NULL) {
		return cli_err_rune_not_exit;
	}
	uint32_t tmp_level = 1;
	FOREACH(runeExp_ptr->exp_vec, it) {
		if (rune_exp >= *it) {
			++tmp_level;
			continue;
		}
		break;
	}
	rune_level = tmp_level;
	return 0;
}

uint32_t RuneUtils::db_save_rune(player_t* player, const rune_t& rune)
{
	dbproto::cs_rune_save db_cs_rune_save;
	commonproto::rune_data_t* rune_data = db_cs_rune_save.mutable_rune_data();
	rune_data->set_runeid(rune.id);
	rune_data->set_index(rune.conf_id);
	rune_data->set_exp(rune.exp);
	rune_data->set_level(rune.level);
	rune_data->set_pack_type(rune.pack_type);
	rune_data->set_pet_catch_time(rune.pet_catch_time);
	rune_data->set_grid_id(rune.grid_id);
	int ret = g_dbproxy->send_msg(NULL, player->userid,
			player->create_tm,
			db_cmd_save_rune, db_cs_rune_save);
	if (ret != 0) {
		return cli_err_sys_err;
	}

	return 0;
}

uint32_t RuneUtils::db_del_rune(player_t* player, uint32_t rune_id)
{
	dbproto::cs_rune_del db_cs_rune_del;
	db_cs_rune_del.set_runeid(rune_id);
	int ret = g_dbproxy->send_msg(NULL, player->userid, 
		player->create_tm, db_cmd_del_rune, db_cs_rune_del);
	if (ret != 0) {
		ERROR_TLOG("del rune to db failed:id=[%u]", player->userid); 
		return cli_err_sys_err;
	}
	return 0;
}

/*
uint32_t RuneUtils::call_rune(player_t* player, uint32_t level, uint32_t& conf_id, uint32_t& open_flag, uint32_t& price)
{
	open_flag = 0;
	conf_id = 0;
	typedef std::vector<rune_rate_conf_t> RuneRateVec;
	RuneRateVec tem_vec;
	uint32_t ret = g_rune_rate_conf_mgr.get_rune_rate_conf_info(level, tem_vec);
	if (ret) {
		ERROR_TLOG("rune_rate_xml:level_value_err:level=[%u]", level);
		return ret;
	}
	//uint32_t rate_value = rand() % 1000 + 1;
	uint32_t rate_value = ranged_random(1, 1000);
	uint32_t sum = 0;
	uint32_t callrate_value = 0;
	FOREACH(tem_vec, it) {
		sum += it->rate;
		if (sum > rate_value) {
			conf_id = it->conf_id;
			break;
		}
	}
	ret = g_rune_rate_conf_mgr.get_call_info_by_level(level, callrate_value, price);
	if (ret) {
		return ret;
	}
	uint32_t rand_rate = rand() % 100;
	bool flag = player->rune_meseum->has_level_in_call_level_vec(level + 1);
	if (rand_rate < callrate_value) {
		//当前召唤的等级不是最高等级，且下一级没有被激活
		if (level < KMaxCallLevel && flag == false) {
			open_flag = 1;
		}
	}
	return 0;
}
*/

uint32_t RuneUtils::init_rune(player_t *player, uint32_t conf_id, rune_t& rune, uint32_t level) {
	uint32_t rune_id = GET_A(kAttrRuneId) + 1;
	rune.id = rune_id;
	rune.conf_id = conf_id;
	if (level >= 2 and level <= kMaxRuneLv) {
		rune_conf_t rune_conf;
		uint32_t ret = RuneUtils::get_rune_conf_data(rune.conf_id, rune_conf);
		if (ret) {
			return ret;
		}
		ret = g_rune_exp_conf_mgr.get_exp_by_level_from_rune_exp_conf(
				rune_conf.rune_type, level, rune.exp);
		if (ret != 0) {
			return ret;
		}
		rune.level = level;
	} else {
		rune.exp = 0;
		rune.level = 1;
	}
	rune.pack_type = kRunePack;
	rune.pet_catch_time = 0;
	rune.grid_id = 0;
	SET_A(kAttrRuneId, rune_id);
	return 0;
}

/*@brief 添加一个符文（包括更新内存以及数据库）
 */
int RuneUtils::add_rune(
		player_t *player, 
		uint32_t conf_id, uint32_t level,
		get_rune_channel_t flag) {
	rune_t rune;

	init_rune(player, conf_id, rune, level);
	//若是系统赠送，直接进入转换背包，若转换背包已经满了，则前端先隐藏
	if (flag == GET_RUNE_SYSTEM_SEND) {
		rune.pack_type = kRunePack;
	}

	//保存符文（内存)
	int ret;
	ret = player->rune_meseum->save_rune(rune);
	if (ret) {
		return ret;
	}
	//保存符文（DB）
	ret = RuneUtils::db_save_rune(player, rune);	
	if (ret) {
		return ret;
	}
	sync_notify_rune_data_info(player, create_rune_cli_t_vec(rune, 1));
	return 0;
}

int RuneUtils::sync_notify_rune_data_info(player_t *player, const vector<rune_cli_t>& rune_vec) {
	onlineproto::sc_0x0326_sync_rune_change_item  rune_msg;
	for (uint32_t i = 0; i < rune_vec.size(); ++i) {
		commonproto::rune_data_cli_t*  rune_ptr = rune_msg.add_runes();
		commonproto::rune_data_t* rune_info = rune_ptr->mutable_rune_info();
		rune_info->set_runeid(rune_vec[i].rune.id);
		rune_info->set_index(rune_vec[i].rune.conf_id);
		rune_info->set_exp(rune_vec[i].rune.exp);
		rune_info->set_level(rune_vec[i].rune.level);
		rune_info->set_pack_type(rune_vec[i].rune.pack_type);
		rune_info->set_pet_catch_time(rune_vec[i].rune.pet_catch_time);
		rune_info->set_grid_id(rune_vec[i].rune.grid_id);
		rune_ptr->set_flag(rune_vec[i].flag);
	}
	send_msg_to_player(player, cli_cmd_cs_0x0326_sync_rune_change_item, rune_msg);
	return 0;
}

uint32_t RuneUtils::alter_rune_info(player_t *player, rune_t& rune) {
	player->rune_meseum->update_rune(rune);
	uint32_t ret = db_save_rune(player, rune);
	if (ret) {
		return ret;
	}
	sync_notify_rune_data_info(player, create_rune_cli_t_vec(rune, 2));
	return 0;
}

uint32_t RuneUtils::del_rune_info(player_t *player, uint32_t rune_id) {
	rune_t rune;
	int ret = get_rune(player, rune_id, rune);
	if (ret) {
		return ret;
	}
	ret = player->rune_meseum->del_rune(rune_id);
	if (ret) {
		return ret;
	}
	ret = db_del_rune(player, rune_id);
	if (ret) {
		return ret;
	}
	sync_notify_rune_data_info(player, create_rune_cli_t_vec(rune, 0));
	return 0;
}

uint32_t RuneUtils::check_rune_call_condition(player_t *player, uint32_t level)
{
	//检查临时背包是否已满
	uint32_t ret = RuneUtils::judge_rune_full_by_packet_type(player, kRunePack);
	if (ret) {
		return ret;
	}
	//钱是否充足
	uint32_t callrate_value = 0, price = 0;
	ret = g_rune_rate_conf_mgr.get_call_info_by_level(level, callrate_value, price);
	if (ret) {
		return ret;
	}
	if (!AttrUtils::is_player_gold_enough(player, price)) {
        return cli_err_gold_not_enough;
    }
	//检查当前级是否已经激活
	std::set<uint32_t> call_level_list_set;
	player->rune_meseum->get_call_level_list_from_set(call_level_list_set);
	if (call_level_list_set.count(level) == 0 && level != 0) {
		return cli_err_rune_call_level_not_activate;
	}
	return 0;
}

/**
 *@brief 在召唤阵level中概率性的获得符文conf_id
 *@param player
 *@param level
 *@param conf_id
 *@return 成功：0； 失败：返回错误码
 */
uint32_t RuneUtils::cal_rune_conf_id_on_calling(
		player_t* player, uint32_t level, 
		uint32_t& conf_id) {
	conf_id = 0;
	typedef std::vector<rune_rate_conf_t> RuneRateVec;
	RuneRateVec tmp_vec;
	uint32_t ret = g_rune_rate_conf_mgr.get_rune_rate_conf_info(level, tmp_vec);
	if (ret) {
		return ret;
	}
	uint32_t rate_value = taomee::ranged_random(1, 999);
	uint32_t sum = 0;
	FOREACH(tmp_vec, it) {
		sum += it->rate;
		if (sum > rate_value) {
			conf_id = it->conf_id;
			break;
		}
	}
	//预防上面的代码循环时，没有给conf_id赋到值，为了避免conf_id为0;
	//这里做了做样的处理
	if (conf_id == 0) {
		conf_id = 100;
	}
	//若是第一次召唤，则必出conf_id 为1的符文
	if (player && GET_A(kAttrRuneId) == 0) {
		conf_id = 1;
	}
	return 0;
}

std::vector<rune_cli_t> RuneUtils::create_rune_cli_t_vec(rune_t& rune, uint32_t flag) 
{
	std::vector<rune_cli_t> tem_vec;
	rune_cli_t tem_cli;
	tem_cli.rune = rune;
	tem_cli.flag = flag;
	tem_vec.push_back(tem_cli);
	return tem_vec;
}


/**
 *@brief 获得该类型符文背包的极限容量
 *@param： pack_type 背包类型
 *@return：rune_count_limit 该类型背包的符文极限数量 
 */
uint32_t RuneUtils::get_rune_limit_count_by_pack_type(const player_t *player, rune_pack_type_t pack_type) {
	uint32_t rune_count_limit = 0;
	if (pack_type == kTransferPack) {
		rune_count_limit = kMaxRuneTransferPack + GET_A(kAttrBuyRuneTransPackPage) * kMaxRuneTransferPack;
	} else if (pack_type == kCollectPack) {
		rune_count_limit = AttrUtils::get_attr_max_limit(player, kAttrRuneCollectorCount);
	} else if (pack_type == kRunePack) {
		rune_count_limit = kMaxRuneCallPack; 
	}
	return rune_count_limit;
}

/**
 * @brief: 更新召唤阵
 * @param: player
 * @param: level: 当前的召唤等级
 *@return 成功：0； 失败：返回错误码
 */
uint32_t RuneUtils::update_call_level_vec(player_t* player, uint32_t level)
{
	uint32_t callrate_value = 0, price = 0;
	uint32_t ret = g_rune_rate_conf_mgr.get_call_info_by_level(
			level, callrate_value, price);
	if (ret) {
		return ret;
	}
	uint32_t rand_rate = ranged_random(1, 100);
	//激活下一级成功：当前召唤的等级不是最高等级，且下一级没有被激活
	if (rand_rate < callrate_value && 
			level < KMaxCallLevel) {
		player->rune_meseum->add_to_call_level_set(level + 1);
	}
	//这一阵列召唤过后，从m_call_level_set中删除
	//第0阵列永远开启
	if (level != 0) {
		player->rune_meseum->erase_from_call_level_set(level);
	}
	return 0;
}

uint32_t RuneUtils::rune_swallow_get_rune_info(player_t* player, uint32_t stand_id, uint32_t mov_id, rune_t& stand_rune, rune_t& mov_rune) {
	uint32_t ret = RuneUtils::get_rune(player, mov_id, mov_rune);
	if (ret) {
		return ret;
	}
	ret = RuneUtils::get_rune(player, stand_id, stand_rune);
	if (ret) {
		return ret;
	}
	return 0;
}

uint32_t RuneUtils::rune_swallow_check_mov_conditon(const rune_t& stand_rune, const rune_t& mov_rune)
{
	/*阻止非法数据*/
	//同一id符文不能合并
	if (stand_rune.id == mov_rune.id) {
		return cli_err_swallow_itself;
	}
	//移动符文在精灵背包中，且固定符文不在精灵背包中；
	if (mov_rune.pack_type == kPetPack) {
		return cli_err_pet_pack_rune_can_not_unequ;
	}
	if (mov_rune.pack_type == kRunePack || stand_rune.pack_type == kRunePack) {
		return cli_err_rune_pack_can_not_swallow;
	}
	//若有满级的符文，则不能参与吞噬
	/*
	if ((stand_rune.pack_type != kPetPack && mov_rune.pack_type != kPetPack) && 
			(stand_rune.level == kMaxRuneLv || mov_rune.level == kMaxRuneLv)) {
		return cli_err_rune_top_level_can_not_swal;
	}
	*/
	return 0;
}

uint32_t RuneUtils::swallow_rule_cal(player_t* player, rune_t& stand_rune, rune_t& mov_rune) {
	//静止吞噬移动：若移动
	rune_conf_t tem_mov_rune_con;
	rune_conf_t tem_stand_rune_con;
	uint32_t ret = RuneUtils::get_rune_conf_data(mov_rune.conf_id, tem_mov_rune_con);
	if (ret) {
		return ret;
	}
	ret = RuneUtils::get_rune_conf_data(stand_rune.conf_id, tem_stand_rune_con);
	if (ret) {
		return ret;
	}
	uint32_t swap_flag = 0;
	if (tem_mov_rune_con.rune_type > tem_stand_rune_con.rune_type) {
		swap_flag = 1;
	} else if (tem_mov_rune_con.rune_type == tem_stand_rune_con.rune_type) {
		if (mov_rune.exp >= stand_rune.exp) {
			swap_flag = 1;
		}
	}
	//若静止符文在精灵身上，且移动符文不在精灵身上；则规定，动吞噬静；必须交换
	//update_pet_flag 0: 都不在精灵身上；1：一个在精灵身上；2：两个都在精灵身上
	uint32_t update_pet_flag = 0;
	if ((rune_pack_type_t)stand_rune.pack_type == kPetPack 
			&& (rune_pack_type_t)mov_rune.pack_type != kPetPack) {
		swap_flag = 1;
		update_pet_flag = 1;
	}
	if (1 == swap_flag) {
		rune_t tem_rune = mov_rune;
		mov_rune = stand_rune;
		stand_rune = tem_rune;

		rune_conf_t tem_rune_conf = tem_mov_rune_con;
		tem_mov_rune_con = tem_stand_rune_con;
		tem_stand_rune_con = tem_rune_conf;
	}
	uint32_t top_exp = 0;
	g_rune_exp_conf_mgr.get_exp_by_level_from_rune_exp_conf(
			tem_stand_rune_con.rune_type, kMaxRuneLv, top_exp);
	stand_rune.exp = stand_rune.exp + mov_rune.exp + tem_mov_rune_con.tran_add_exp;
	//超出满级的多余的经验放入经验瓶中
	if (stand_rune.exp > top_exp) {
		ADD_A(kAttrRuneExpBottle, stand_rune.exp - top_exp);
		stand_rune.exp = top_exp;
	}
	if (1 == swap_flag) {
		//现在的静止符文 获取 原来的静止符文的背包类型, 精灵create_tm
		uint32_t tmp_type = stand_rune.pack_type;	
		stand_rune.pack_type = mov_rune.pack_type;
		mov_rune.pack_type = (rune_pack_type_t)tmp_type;
		uint32_t tmp_time = stand_rune.pet_catch_time;
		stand_rune.pet_catch_time = mov_rune.pet_catch_time;
		mov_rune.pet_catch_time = tmp_time;
	}
	uint32_t tmp_level;
	RuneUtils::get_rune_level_by_exp(
			stand_rune.exp, tem_stand_rune_con.rune_type, tmp_level);
	stand_rune.level = tmp_level;

	//若两个符文都在精灵身上，则清除被吞噬符文的pos
	if (stand_rune.pack_type == kPetPack && mov_rune.pack_type == kPetPack) {
		if (update_pet_flag == 1) {
			ERROR_TLOG("rune swallow:data err");
			return cli_err_data_error;
		}
		update_pet_flag = 2;
		rune_t tem_rune;
		//若有交换，则现在的静止的stand_rune.id就是原来移动的
		if (1 == swap_flag) {
			tem_rune = stand_rune;
		} else {
			tem_rune = mov_rune;
		}
		RuneUtils::clean_rune_pos_in_pet(player, tem_rune);
	}
	if (update_pet_flag == 1) {
		RuneUtils::update_rune_pos_in_pet(player, stand_rune, mov_rune);
	}
	if (update_pet_flag) {
		RuneUtils::update_pet_info(player, stand_rune);
	}
	return 0;
}

uint32_t RuneUtils::update_pet_info(player_t* player, const rune_t& rune) {
	Pet* pet = NULL;
	if (rune.pack_type != kPetPack || !rune.pet_catch_time) {
		return cli_err_data_error;
	}
	pet = PetUtils::get_pet_in_loc(player, rune.pet_catch_time, PET_LOC_BAG);
	if (pet == NULL) {
		return cli_err_bag_pet_not_exist;
	}
	PetUtils::save_pet(player, *pet, false, true); 
	return 0;
}

/**
 *@brief 清除符文身上的pos位置,仅用于两个吞噬的符文都在精灵身上
 *@param player
 *@param rune 要被吞噬掉的符文
 *@return 成功：0； 失败：返回错误码
 */
uint32_t RuneUtils::clean_rune_pos_in_pet(player_t* player, const rune_t& rune) {
	Pet* pet = NULL;
	if (rune.pack_type != kPetPack || !rune.pet_catch_time) {
		ERROR_TLOG("clean_rune_pos,err;pack_type=[%u],catch_time=[%u]",
				rune.pack_type, rune.pet_catch_time);
		return cli_err_mov_rune_not_in_pet;
	}
	pet = PetUtils::get_pet_in_loc(player, rune.pet_catch_time, PET_LOC_BAG);
	if (pet == NULL) {
		ERROR_TLOG("clean_rune_pos,pet null,catch_time=[%u]", rune.pet_catch_time);
		return cli_err_bag_pet_not_exist;
	}
	uint32_t pos;
	if (pet->get_rune_idx_by_id(rune.id, pos)) {
		ERROR_TLOG("rune not in pet pos,pos=[%u],rune_id=[%u]", pos, rune.id);
		return cli_err_data_error;
	}
	pet->set_rune_array(pos, 0);
	PetUtils::save_pet(player, *pet, false, true);
	return 0;
}

/**
 *@brief 更新精灵身上的符文id;用于符文吞噬协议中：只有一个符文在精灵身上的吞噬情形
 *此刻，符文一定在后台代码中经过了交换
 *@param player
 *@param stand_rune 交换后的主动吞噬符文
 *@param mov_rune 交换后的被吞噬的符文
 *@return 成功：0； 失败：返回错误码
 */
uint32_t RuneUtils::update_rune_pos_in_pet(player_t* player, rune_t& stand_rune, const rune_t& mov_rune)
{
	Pet* pet = NULL;
	if (stand_rune.pack_type != kPetPack || !stand_rune.pet_catch_time) {
		ERROR_TLOG("update_rune_pos,err;pack_type=[%u],catch_time=[%u]", 
				stand_rune.pack_type, stand_rune.pet_catch_time);
		return cli_err_mov_rune_not_in_pet;
	}
	pet = PetUtils::get_pet_in_loc(player, stand_rune.pet_catch_time, PET_LOC_BAG);
	if (pet == NULL) {
		ERROR_TLOG("update_rune_pos,pet null,catch_time=[%u]", stand_rune.pet_catch_time);
		return cli_err_bag_pet_not_exist;
	}
	//找到原来的符文pos，在该pos处换上吞噬后的符文id
	uint32_t pos;
	if (pet->get_rune_idx_by_id(mov_rune.id, pos)) {
		ERROR_TLOG("rune not in pet pos,pos=[%u],rune_id=[%u]", pos, mov_rune.id);
		return cli_err_data_error;
	}
	pet->set_rune_array(pos, stand_rune.id);
	stand_rune.grid_id = pos + 1;
	PetUtils::save_pet(player, *pet, false, true);
	return 0;
}

uint32_t RuneUtils::check_runes_func_same_or_not(player_t* player, uint32_t rune_id, Pet* pet, uint32_t stand_id)
{
	if (pet == NULL) {
		return cli_err_pet_not_exist;
	}
	rune_t trune;
	player->rune_meseum->get_rune(rune_id, trune);
	rune_conf_t tmp_conf;
	get_rune_conf_data(trune.conf_id, tmp_conf);
	std::vector<uint32_t> runeid_vec;
	pet->get_rune_info_equiped_in_pet(runeid_vec);
	FOREACH(runeid_vec, it) {
		if (*it == 0) {
			continue;
		}
		rune_t tmp_rune;
		player->rune_meseum->get_rune(*it, tmp_rune);
		rune_conf_t tmp_rune_conf;
		get_rune_conf_data(tmp_rune.conf_id, tmp_rune_conf);
		if (tmp_conf.fun_type == tmp_rune_conf.fun_type && *it != stand_id) {
			return cli_err_already_equip_same_type_rune;
		} 
	}
	return 0;
}

rune_attr_type_t RuneUtils::get_rune_attr_type(player_t* player, uint32_t rune_id)
{
	rune_t rune;
	uint32_t ret = RuneUtils::get_rune(player, rune_id, rune);
	if (ret) {
		return kRuneNotDef;
	}
	rune_conf_t rune_conf;
	ret = RuneUtils::get_rune_conf_data(rune.conf_id, rune_conf);	
	if (ret) {
		return kRuneNotDef;
	}
	return (rune_attr_type_t)rune_conf.rune_type;
}
