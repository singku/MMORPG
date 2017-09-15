#include "bless_pet_utils.h"
#include "service.h"
const static uint32_t ttl_time = 24*60*60;
const static uint32_t max_cnt  = 50;//存50个队伍

bool BlessPetUtils::check_has_bless_pet(player_t *player, const uint32_t bless_id)
{
	const bless_pet_conf_t *conf_inf =
		g_bless_pet_conf_mgr.find_bless_pet_conf(bless_id);
	uint32_t pet_id = conf_inf->pet_id;
	return PetUtils::has_pet(player, pet_id);
}

bool BlessPetUtils::check_other_has_bless_pet(
		const commonproto::pet_list_t &pet_info, const uint32_t bless_id)
{
	bool check_pet_list = false;

	const bless_pet_conf_t *conf_inf =
		g_bless_pet_conf_mgr.find_bless_pet_conf(bless_id);
	uint32_t pet_id = conf_inf->pet_id;

	//检查伙伴
	for(int i = 0; i < pet_info.pet_list_size(); i++){
		commonproto::battle_pet_data_t pet_data;
		if(pet_id == pet_info.pet_list(i).base_info().pet_id()){
			check_pet_list = true;
			break;
		}
	}

	return  check_pet_list;
}

//更新祈福队伍信息，保留时间一天，subkey：当天0起始时间
int BlessPetUtils::update_bless_team_info_to_redis(player_t *player,
		const commonproto::bless_pet_team_info_t &team_info)
{
    // 更新祈福队伍-id映射V
	uint32_t key = rankproto::HASHSET_BLESS_PET_TEAM_MAP;
	uint32_t sub_key = TimeUtils::day_align_low(NOW());
	uint32_t type = rankproto::REDIS_INSERT_OR_UPDATE;

	rankproto::cs_hset_insert_or_update req_in_;
    std::ostringstream redis_key;
    redis_key << key << ":" << sub_key;

    req_in_.set_oper_type(type);
	req_in_.set_key(redis_key.str());
    req_in_.set_server_id(g_server_id);
    req_in_.set_key_ttl(ttl_time);

	rankproto::hset_field_t *field = req_in_.add_fields(); 
	std::string pkg;
	pkg.clear();

	team_info.SerializeToString(&pkg);
	uint64_t leader_key = ROLE_KEY( 
			ROLE(team_info.team_leader().userid(),
			   	team_info.team_leader().u_create_tm()));

    field->set_name(boost::lexical_cast<string>(leader_key));
    field->set_value(pkg);

    int ret = g_dbproxy->send_msg(
            NULL, player->userid, player->create_tm, 
			ranking_cmd_hset_insert_or_update, req_in_);

    if (ret) {
        return ret;
    }

    return 0;
}

//删除队伍信息
int BlessPetUtils::delete_bless_team_info_to_redis(player_t *player)
{
    // 更新祈福队伍-id映射V
	uint32_t key = rankproto::HASHSET_BLESS_PET_TEAM_MAP;
	uint32_t sub_key = TimeUtils::day_align_low(NOW());
	uint32_t type = rankproto::REDIS_DELETE;

	rankproto::cs_hset_insert_or_update req_in_;
    std::ostringstream redis_key;
    redis_key << key << ":" << sub_key;

    req_in_.set_oper_type(type);
	req_in_.set_key(redis_key.str());
    req_in_.set_server_id(g_server_id);

	rankproto::hset_field_t *field = req_in_.add_fields(); 
	std::string pkg = "0";

	uint64_t leader_key = ROLE_KEY( 
			ROLE(player->userid, player->create_tm));

    field->set_name(boost::lexical_cast<string>(leader_key));
    field->set_value(pkg);

    int ret = g_dbproxy->send_msg(
            NULL, player->userid, player->create_tm, 
			ranking_cmd_hset_insert_or_update, req_in_);

    if (ret) {
        return ret;
    }

    return 0;
}

int BlessPetUtils::push_bless_team_list(player_t *player)
{
    // 更新祈福队伍-id映射V
	uint32_t key     = rankproto::LIST_TYPE_BLESS_PET;
	uint32_t sub_key = TimeUtils::day_align_low(NOW());
	uint64_t value   = ROLE_KEY( 
			ROLE(player->userid, player->create_tm));

	rankproto::cs_list_lpush_member req_in_;	
	ostringstream reidis_key;
	reidis_key << key << ":" << sub_key;
	req_in_.set_key(reidis_key.str());
	req_in_.set_server_id(g_server_id);
	req_in_.set_max(max_cnt);
	req_in_.add_value(boost::lexical_cast<string>(value));

	int ret = g_dbproxy->send_msg(
			player, player->userid, 
			player->create_tm,
			ranking_cmd_list_lpush_member, 
			req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}

int BlessPetUtils::get_bless_team_list(player_t *player)
{
    // 更新祈福队伍-id映射V
	uint32_t key     = rankproto::LIST_TYPE_BLESS_PET;
	uint32_t sub_key = TimeUtils::day_align_low(NOW());
	uint32_t start    = 1;//拉取队伍id
	uint32_t end      = 20;//拉取队伍id起始

	rankproto::cs_list_get_range_member req_in_;	
	ostringstream reidis_key;
	reidis_key << key << ":" << sub_key;

	req_in_.set_key(reidis_key.str());
	req_in_.set_start(start);
	req_in_.set_end(end);
	req_in_.set_server_id(g_server_id);

	int ret = g_dbproxy->send_msg(
			player, player->userid, 
			player->create_tm,
			ranking_cmd_list_get_range_member, 
			req_in_);
	if (ret) {
		return ret;
	}
	return 0;
}
