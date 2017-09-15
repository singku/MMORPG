#include "family_processor.h"

#include "player.h"
#include "data_proto_utils.h"
#include "player_utils.h"
#include "item.h"
#include "prize.h"
#include "rank_utils.h"
#include "global_data.h"
#include "service.h"
#include "utils.h"
#include "family_utils.h"
#include "map_conf.h"
#include "family_conf.h"

int FamilyCreateCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    std::string family_name = cli_in_.family_name();
    if (family_name.size() >= commonproto::FAMILY_NAME_MAX_LEN) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_name_too_long);
    }
    int err = Utils::check_dirty_name(family_name);
    if (err) {
        return send_err_to_player(player, player->cli_wait_cmd, err);
    }

    std::string pet_name = cli_in_.pet_name();
    //if (pet_name.size() >= commonproto::FAMILY_PET_NAME_MAX_LEN) {
    if (pet_name.size() > 0) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_pet_name_too_len);
    }

    // 家族名字不能是纯数字
    if (Utils::is_number(family_name)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_name_cannot_be_number);
    }

    //不能加入多个家族
    if (GET_A(kAttrFamilyId) > 0) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_repeat_join_family);
    }

    // 检查等级
    if (GET_A(kAttrLv) < commonproto::FAMILY_CREATE_LEVEL) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_level_too_low);
    }

    // 名字查重
    std::vector<std::string> values;
    values.push_back(cli_in_.family_name());

    return RankUtils::set_is_member(
            player, player->userid, player->create_tm, 0,
            rankproto::SET_FAMILY_NAME, 0, values);
}

int FamilyCreateCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case ranking_cmd_set_is_member:
            return proc_pkg_from_serv_aft_set_is_member(player, body, bodylen);
        case db_cmd_family_create:
            return proc_pkg_from_serv_aft_create_family(player, body, bodylen);
        case db_cmd_family_update_info:
            return proc_pkg_from_serv_aft_update_family_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyCreateCmdProcessor::proc_pkg_from_serv_aft_set_is_member(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_set_is_member_out_);
    if (rank_set_is_member_out_.flags(0)) {
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_name_already_exist);
    }

    int err = 0;
    if (cli_in_.type() == 0) {
        err = buy_attr_and_use(
            player, kServiceCreateFamily, 
            commonproto::SERVICE_PRODUCT_CREATE_FAMILY, 1);
    } else {
        err = buy_attr_and_use(
            player, kServiceCreateFamily, 
            commonproto::SERVICE_PRODUCT_CREATE_FAMILY_GOLD, 1);
    }

    if (err) {
        return send_err_to_player(player, player->cli_wait_cmd, err);
    }

    db_create_family_in_.Clear();
    db_create_family_in_.set_server_id(g_server_id);
    return g_dbproxy->send_msg(
            player, player->userid, player->create_tm, 
            db_cmd_family_create, db_create_family_in_);
}

int FamilyCreateCmdProcessor::proc_pkg_from_serv_aft_create_family(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_create_family_out_);

    family_create_session_t *session = 
        (family_create_session_t *)player->session;
    session->family_id = db_create_family_out_.family_id();

    if (!FamilyUtils::is_valid_family_id(session->family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    

    // 初始化家族信息
    uint32_t create_time = NOW();
    dbproto::family_info_table_t family_update_info;
    family_update_info.Clear();
    family_update_info.set_family_id(session->family_id);
    family_update_info.set_family_name(cli_in_.family_name());
    family_update_info.set_pet_name(cli_in_.pet_name());
    family_update_info.set_creator_id(player->userid);
    family_update_info.set_u_create_tm(player->create_tm);
    family_update_info.set_create_time(create_time);
    family_update_info.set_server_id(g_server_id);
    uint32_t level = 0;
    FamilyUtils::compute_family_level(0, level);
    family_update_info.set_level(level);

    family_info.Clear();
    family_info.set_family_id(session->family_id);
    family_info.set_level(level);
    family_info.set_create_time(create_time);
    family_info.set_family_name(cli_in_.family_name());
    family_info.set_pet_name(cli_in_.pet_name());
    family_info.set_construct_value(0);
    family_info.set_total_battle_value(GET_A(kAttrCurBattleValue));
    family_info.set_member_num(1);
    family_info.set_last_member_login_time(GET_A(kAttrLastLoginTm));
    family_info.set_server_id(g_server_id);

    return FamilyUtils::update_family_info(
            player, family_update_info, dbproto::DB_UPDATE_AND_INESRT);
}

int FamilyCreateCmdProcessor::proc_pkg_from_serv_aft_update_family_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_update_family_info_out_);

    family_create_session_t *session = 
        (family_create_session_t *)player->session;

    // 初始化成员信息
    commonproto::family_member_info_t member_info;
    member_info.set_userid(player->userid);
    member_info.set_u_create_tm(player->create_tm);
    member_info.set_title(commonproto::FAMILY_TITLE_LEADER);
    member_info.set_battle_value(GET_A(kAttrCurBattleValue));
    member_info.set_last_login_time(GET_A(kAttrLastLoginTm));
    member_info.set_last_logout_time(GET_A(kAttrLastLogoutTm));
    FamilyUtils::member_join_family(player, family_info, member_info);

    // 更新家族排名
    FamilyUtils::update_family_rank_score(session->family_id, family_info);

    // 更新家族名称库
    std::vector<std::string> vector;
    vector.push_back(cli_in_.family_name());
    RankUtils::set_insert_member(
            NULL, player->userid, player->create_tm, 0,
            rankproto::SET_FAMILY_NAME, 
            0, vector);

    // 更新家族id库
    vector.clear();
    vector.push_back(Utils::to_string(session->family_id));
    RankUtils::set_insert_member(
            NULL, player->userid, player->create_tm, 0,
            rankproto::SET_FAMILY_ID, 
            0, vector);

    // 更新家族名字-id映射表
    std::vector<rankproto::hset_field_t> v_fields;
    rankproto::hset_field_t name_field;
    name_field.set_name(cli_in_.family_name());
    name_field.set_value(Utils::to_string(session->family_id));
    v_fields.push_back(name_field);
    RankUtils::hset_insert_or_update(
            NULL, player->userid, player->create_tm, 0,
            rankproto::HASHSET_FAMILY_NAME_ID_MAP,
            0, rankproto::REDIS_INSERT_OR_UPDATE,
            &v_fields);

    // 更新id-名字映射表
    v_fields.clear();
    rankproto::hset_field_t id_name_field;
    id_name_field.set_name(Utils::to_string(session->family_id));
    id_name_field.set_value(cli_in_.family_name());
    v_fields.push_back(id_name_field);
    RankUtils::hset_insert_or_update(
            NULL, player->userid, player->create_tm, 0,
            rankproto::HASHSET_FAMILY_ID_NAME_MAP,
            0, rankproto::REDIS_INSERT_OR_UPDATE,
            &v_fields);

    // 更新推荐家族库
    FamilyUtils::update_family_match_info(family_info);

    onlineproto::sc_0x0701_family_create cli_out_;
    cli_out_.set_family_id(session->family_id);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyDismissCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    uint32_t family_id = GET_A(kAttrFamilyId);
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 账号密码验证
    act_login_with_verify_code_req_t* req_body = (act_login_with_verify_code_req_t *)(m_send_buf_);
    memset(req_body, 0, sizeof(*req_body));
    hex2bin(req_body->passwd_md5_two, cli_in_.passwd().substr(0, 32).c_str(), 16); 
    req_body->verifyid = g_server_config.verifyid; 
    req_body->region = g_server_config.idc_zone; 
    req_body->gameid = g_server_config.gameid; 
    req_body->ip = player->fdsess->remote_ip;
    //hex2bin(req_body->verify_session, 
            //cli_in_.verify_image_session().substr(0, 32).c_str(), 16);
    //strncpy(req_body->verify_code, cli_in_.verify_code().c_str(), 
            //sizeof(req_body->verify_code));
            //req_body->login_channel = 0; 
    //strncpy(req_body->login_promot_tag, cli_in_.tad().c_str(), 
    //sizeof(req_body->login_promot_tag));

    //strncpy(session->tad, cli_in_.tad().c_str(),
            //sizeof(session->tad));
    return g_dbproxy->send_to_act(player, player->userid,
            act_cmd_login_with_verify_code, (const char *)req_body, sizeof(*req_body));
}

int FamilyDismissCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case act_cmd_login_with_verify_code: 
            return proc_pkg_from_serv_aft_check_session(
                    player, body, bodylen);
        case ranking_cmd_string_insert:
            return proc_pkg_from_serv_aft_lock_acquire(
                    player, body, bodylen);
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyDismissCmdProcessor::proc_pkg_from_serv_aft_check_session(
        player_t* player, const char* body, int bodylen)
{
    act_login_with_verify_code_ack_t* ack_body = 
            (act_login_with_verify_code_ack_t *)body;
    if (!(ack_body->flag == ACT_LOGIN_ACK_FLAG_SUCC
        || ack_body->flag == ACT_LOGIN_ACK_FLAG_LOGIN_IN_DIFF_CITY
        || ack_body->flag == ACT_LOGIN_ACK_FLAG_LOGIN_IN_DIFF_CITY_TOO_MUCH)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_passwd_err);
    }

    // 申请家族操作锁
    uint32_t family_id = GET_A(kAttrFamilyId);
    std::ostringstream redis_key;
    redis_key << rankproto::STRING_FAMILY_DEAL_LOCK <<":"<< family_id;
    lock_key_ = redis_key.str();
    std::string value = Utils::to_string(player->userid);
    return RankUtils::lock_acquire(
            player, lock_key_, value, commonproto::FAMILY_LOCK_EXPIRE_TIME);
}

int FamilyDismissCmdProcessor::proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_lock_acquire_out_);

    if(!rank_lock_acquire_out_.flag()) {
        // 锁被占用，返回提示
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_wait_for_lock_release);
    } else {
        // 申请锁成功,保存记录
        if (player->family_lock_sets != NULL) {
            player->family_lock_sets->push_back(lock_key_);
        }
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    // 检查族长身份
    db_get_info_in_.Clear();
    db_get_info_in_.set_flag(dbproto::DB_FAMILY_GET_INFO_WITH_MEMBER_LIST);
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, db_cmd_family_get_info, db_get_info_in_);
}

int FamilyDismissCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_info_out_);
    if (db_get_info_out_.mutable_family_info()->leader_id() != player->userid) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_need_leader);
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    std::string family_name(player->family_name);

    FamilyUtils::dismiss_family(player, family_id);
    FamilyUtils::clear_self_family_info(player);

    // 通知家族成员
    for (int i = 0; i < db_get_info_out_.members_size();i++) {
        // 更新属性，并发送通知
        std::vector<commonproto::attr_data_t> attr_vec;
        commonproto::attr_data_t attr_data;
        attr_data.set_type(kAttrFamilyId);
        attr_data.set_value(0);
        attr_vec.push_back(attr_data);
        attr_data.set_type(kAttrFamilyLevel);
        attr_data.set_value(0);
        attr_vec.push_back(attr_data);
        AttrUtils::update_other_attr_value(
                db_get_info_out_.members(i).userid(), 
                db_get_info_out_.members(i).u_create_tm(),
                attr_vec);
		//发送家族解散通知
        if (player->userid != db_get_info_out_.members(i).userid()) {
            FamilyUtils::send_family_msg_notice(
                    player,  db_get_info_out_.members(i).userid(), 
                    db_get_info_out_.members(i).u_create_tm(),
                    commonproto::FAMILY_MSG_TYPE_DISMISS_SUCC,
                    family_name, family_id);
        }

        PlayerUtils::calc_player_battle_value(player, onlineproto::ATTR_OTHER_REASON);
    }

    // 释放锁
    FamilyUtils::family_lock_release(player, lock_key_);
    lock_key_="";

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyGetInfoCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    
    if (cli_in_.type() == 1) {
        // 根据名字查询家族信息
        std::ostringstream redis_key;
        redis_key << rankproto::HASHSET_FAMILY_NAME_ID_MAP <<":"<< 0;
        rank_get_field_info_in_.Clear();
        rank_get_field_info_in_.set_key(redis_key.str());
        rank_get_field_info_in_.add_field_names(cli_in_.family_name());
        rank_get_field_info_in_.set_server_id(0);

        return g_dbproxy->send_msg(
                player, player->userid, player->create_tm, 
                ranking_cmd_hset_get_field_info, rank_get_field_info_in_);
    } 

    // 根据id查询家族信息
    uint32_t family_id = cli_in_.family_id();
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    //如果当前家族就是请求的家族ID
    uint32_t cur_family_id = GET_A(kAttrFamilyId);
    if (FamilyUtils::is_valid_family_id(cur_family_id) && cur_family_id == cli_in_.family_id()) {
        FOREACH((*(player->family_apply_record)), it) {
            uint32_t apply_family_id = *it;
            FamilyUtils::family_del_event(apply_family_id, player->userid, player->create_tm,
                    commonproto::FAMILY_EVENT_TYPE_APPLY);
        }
        if (player->family_apply_record->size()) {
            commonproto::family_apply_record_t apply_record;
            apply_record.Clear();
            PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
                    dbproto::FAMILY_APPLY_RECORD, apply_record, "0");
        }
    }

    db_get_info_in_.Clear();
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, db_cmd_family_get_info, db_get_info_in_);
}

int FamilyGetInfoCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case ranking_cmd_hset_get_field_info:
            return proc_pkg_from_serv_aft_family_rank_hset_get_field_info(
                    player, body, bodylen);
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        case cache_cmd_ol_req_users_info:
            return proc_pkg_from_serv_aft_get_cache_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyGetInfoCmdProcessor::proc_pkg_from_serv_aft_family_rank_hset_get_field_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_get_field_info_out_);

    if (rank_get_field_info_out_.fields_size() == 0) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }
    uint32_t family_id = atoi_safe(rank_get_field_info_out_.mutable_fields(0)->value().c_str());
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    db_get_info_in_.Clear();
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, db_cmd_family_get_info, db_get_info_in_);
}

int FamilyGetInfoCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_info_out_);
    cli_out_.Clear();
    cli_out_.mutable_family_info()->CopyFrom(db_get_info_out_.family_info());
    //uint32_t family_id = cli_in_.family_id();
    uint32_t family_id = db_get_info_out_.mutable_family_info()->family_id();
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 检查是否同服
    if (db_get_info_out_.mutable_family_info()->server_id() != g_server_id) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 更新排名信息
    FamilyUtils::update_family_rank_score(
            family_id, db_get_info_out_.family_info());

    // 更新推荐家族库
    FamilyUtils::update_family_match_info(db_get_info_out_.family_info());

    // 更新家族名称库
    std::vector<std::string> vector;
    vector.push_back(db_get_info_out_.mutable_family_info()->family_name());
    RankUtils::set_insert_member(
            NULL, player->userid, player->create_tm, 0,
            rankproto::SET_FAMILY_NAME, 
            0, vector);

    // 更新家族id库
    vector.clear();
    vector.push_back(Utils::to_string(db_get_info_out_.mutable_family_info()->family_id()));
    RankUtils::set_insert_member(
            NULL, player->userid,player->create_tm, 0, 
            rankproto::SET_FAMILY_ID, 
            0, vector);

    // 更新家族名字-id映射表
    std::vector<rankproto::hset_field_t> v_fields;
    rankproto::hset_field_t name_field;
    name_field.set_name(db_get_info_out_.mutable_family_info()->family_name());
    name_field.set_value(Utils::to_string(db_get_info_out_.mutable_family_info()->family_id()));
    v_fields.push_back(name_field);
    RankUtils::hset_insert_or_update(
            NULL, player->userid, player->create_tm, 0,
            rankproto::HASHSET_FAMILY_NAME_ID_MAP,
            0, rankproto::REDIS_INSERT_OR_UPDATE,
            &v_fields);

    // 更新家族id-名字映射表
    v_fields.clear();
    rankproto::hset_field_t id_name_field;
    id_name_field.set_name(Utils::to_string(db_get_info_out_.mutable_family_info()->family_id()));
    id_name_field.set_value(db_get_info_out_.mutable_family_info()->family_name());
    v_fields.push_back(id_name_field);
    RankUtils::hset_insert_or_update(
            NULL, player->userid, player->create_tm, 0,
            rankproto::HASHSET_FAMILY_ID_NAME_MAP,
            0, rankproto::REDIS_INSERT_OR_UPDATE,
            &v_fields);

    CACHE_OUT_MSG(cli_out_);
    cache_info_in_.Clear();
    commonproto::role_info_t *role_info = cache_info_in_.add_roles();
    role_info->set_userid(cli_out_.family_info().leader_id());
    role_info->set_u_create_tm(cli_out_.family_info().leader_create_tm());
    role_info->set_server_id(g_server_id);
    return g_dbproxy->send_msg(
            player, player->userid, player->create_tm, 
            cache_cmd_ol_req_users_info, cache_info_in_);
}

int FamilyGetInfoCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(cache_info_out_);
    PARSE_OUT_MSG(cli_out_);

    // 拉取族长信息
    if (cache_info_out_.user_infos_size() > 0) {
        const commonproto::battle_player_data_t &player_info = 
        cache_info_out_.user_infos(0);
        cli_out_.mutable_family_info()->set_leader_name(
                player_info.base_info().nick());
    }

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyGetMemberInfoCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    uint32_t family_id = cli_in_.family_id();
    db_get_member_info_in_.Clear();
    commonproto::role_info_t *role_info = db_get_member_info_in_.add_users();
    role_info->CopyFrom(cli_in_.role());
    //role_info->set_userid(cli_in_.role.userid());
    //role_info->set_u_create_tm(cli_in_.role.u_create_tm());
    //role_info->set_server_id(0);
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_member_info, db_get_member_info_in_);
}

int FamilyGetMemberInfoCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case db_cmd_family_get_member_info:
            return proc_pkg_from_serv_aft_family_get_member_info(
                    player, body, bodylen);
        case cache_cmd_ol_req_users_info:
            return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
        case sw_cmd_sw_is_player_online:
            return proc_pkg_from_serv_aft_get_user_ol_info(player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyGetMemberInfoCmdProcessor::proc_pkg_from_serv_aft_family_get_member_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_member_info_out_);
    cli_out_.mutable_member_info()->CopyFrom(
            db_get_member_info_out_.member_infos(0));

    CACHE_OUT_MSG(cli_out_);
    // 拉取玩家缓存名字信息
    cache_info_in_.Clear();
    commonproto::role_info_t *role_info = cache_info_in_.add_roles();
    role_info->CopyFrom(cli_in_.role());
    //role_info->set_userid(cli_in_.userid());
    //role_info->set_u_create_tm(0);
    //role_info->set_server_id(0);
    return g_dbproxy->send_msg(
			    player, player->userid, player->create_tm, 
			    cache_cmd_ol_req_users_info, cache_info_in_);
}

int FamilyGetMemberInfoCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(cache_info_out_);
    PARSE_OUT_MSG(cli_out_);

    if (cache_info_out_.user_infos_size() > 0) {
        const commonproto::battle_player_data_t &player_info = 
        cache_info_out_.user_infos(0);
        cli_out_.mutable_member_info()->set_name(
                player_info.base_info().nick());
        cli_out_.mutable_member_info()->set_level(
                player_info.base_info().level());
        cli_out_.mutable_member_info()->set_vip_type(
                (commonproto::player_vip_type_t)get_player_vip_flag(player));

        CACHE_OUT_MSG(cli_out_);

        cs_sw_is_online_in_.Clear();
        switchproto::sw_player_online_info_t* sw_player_online_info =
            cs_sw_is_online_in_.add_ol_info();
        sw_player_online_info->set_userid(player_info.base_info().user_id());
        sw_player_online_info->set_u_create_tm(player_info.base_info().create_tm());
        sw_player_online_info->set_is_online(0);
        sw_player_online_info->set_team(0);

    	return g_switch->send_msg(player, g_online_id, player->create_tm,
            sw_cmd_sw_is_player_online, cs_sw_is_online_in_);
    }

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyGetMemberInfoCmdProcessor::proc_pkg_from_serv_aft_get_user_ol_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(sc_sw_is_online_out_);
    PARSE_OUT_MSG(cli_out_);

    if (sc_sw_is_online_out_.ol_info_size()) {
        cli_out_.mutable_member_info()->set_is_online(
                sc_sw_is_online_out_.mutable_ol_info(0)->is_online());
    }

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyGetMemberListCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    uint32_t page_no = cli_in_.list_in().page_no();
    uint32_t page_size = cli_in_.list_in().page_size();
    if (page_no < 1 || page_size < 1 || 
            page_size > MAX_GET_FAMILY_MEMBER_LIST_PAGE_SIZE) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_page_size_over_limit);
    }

    uint32_t family_id = cli_in_.list_in().family_id();
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    db_get_member_list_in_.mutable_list_in()->CopyFrom(cli_in_.list_in());
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_member_list, db_get_member_list_in_);
}

int FamilyGetMemberListCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{

    switch (player->serv_cmd) {
        case db_cmd_family_get_member_list:
            return proc_pkg_from_serv_aft_get_member_list(player, body, bodylen);
        case cache_cmd_ol_req_users_info:
            return proc_pkg_from_serv_aft_get_cache_info(player, body, bodylen);
        case sw_cmd_sw_is_player_online:
            return proc_pkg_from_serv_aft_get_user_ol_info(player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyGetMemberListCmdProcessor::proc_pkg_from_serv_aft_get_member_list(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_member_list_out_);

	uint32_t page_member_num = 
        db_get_member_list_out_.mutable_list_out()->page_member_num();

    cache_info_in_.Clear();
	for (uint32_t i = 0; i < page_member_num ; i++) {
		uint32_t userid = 
            db_get_member_list_out_.mutable_list_out()->mutable_member_info(i)->userid();
        uint32_t create_tm = 
            db_get_member_list_out_.mutable_list_out()->mutable_member_info(i)->u_create_tm();
        commonproto::role_info_t *role_info = cache_info_in_.add_roles();
        role_info->set_userid(userid);
        role_info->set_u_create_tm(create_tm);
        role_info->set_server_id(g_server_id);
	}
    return g_dbproxy->send_msg(
			    player, player->userid, player->create_tm, 
			    cache_cmd_ol_req_users_info, cache_info_in_);
}

int FamilyGetMemberListCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(cache_info_out_);
    uint32_t member_num = cache_info_out_.user_infos_size();
    cli_out_.Clear();

	uint32_t page_member_num = 
        db_get_member_list_out_.mutable_list_out()->page_member_num();
    uint32_t total_member_num = 
        db_get_member_list_out_.mutable_list_out()->total_member_num();
    cli_out_.mutable_list_out()->set_total_member_num(total_member_num);
    cli_out_.mutable_list_out()->set_page_member_num(page_member_num);

    // 拉取玩家个人信息
    cs_sw_is_online_in_.Clear();
    for (uint32_t i = 0; i < page_member_num; i++) {
        commonproto::family_member_info_t *member_info = 
            cli_out_.mutable_list_out()->add_member_info();
        member_info->CopyFrom(db_get_member_list_out_.mutable_list_out()->member_info(i));

        for (uint32_t j = 0; j < member_num;j++) {
            const commonproto::battle_player_data_t &player_info = 
                cache_info_out_.user_infos(j);
            if (member_info->userid() == player_info.base_info().user_id()) {
                member_info->set_name(player_info.base_info().nick());
                member_info->set_level(player_info.base_info().level());
                player_vip_type_t vip_type = get_player_base_info_vip_flag(
                        player_info.base_info());
                member_info->set_vip_type((commonproto::player_vip_type_t)vip_type);
                break;
            }
        }

        // 添加在线查询请求
        switchproto::sw_player_online_info_t* sw_player_online_info =
            cs_sw_is_online_in_.add_ol_info();
        sw_player_online_info->set_userid(member_info->userid());
        sw_player_online_info->set_u_create_tm(member_info->u_create_tm());
        sw_player_online_info->set_is_online(0);
        sw_player_online_info->set_team(0);
    }
    
    CACHE_OUT_MSG(cli_out_);
    return g_switch->send_msg(player, g_online_id, player->create_tm,
            sw_cmd_sw_is_player_online, cs_sw_is_online_in_);
}

int FamilyGetMemberListCmdProcessor::proc_pkg_from_serv_aft_get_user_ol_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(sc_sw_is_online_out_);
    PARSE_OUT_MSG(cli_out_);

    onlineproto::sc_0x0710_family_get_member_list      tmp_out_;
    tmp_out_.mutable_list_out()->set_total_member_num(cli_out_.mutable_list_out()->total_member_num());

    for (int i = 0; i < sc_sw_is_online_out_.ol_info_size();i++) {
		switchproto::sw_player_online_info_t* ol_info = sc_sw_is_online_out_.mutable_ol_info(i);
        for (int j = 0; j < cli_out_.mutable_list_out()->member_info_size(); j++) {
            if (cli_in_.list_in().flag() == commonproto::FAMILY_MEMBER_LIST_ONLINE) {
                commonproto::family_member_info_t *member_info = 
                    cli_out_.mutable_list_out()->mutable_member_info(j);
                if (ol_info->userid() == member_info->userid() && ol_info->is_online()) {
                commonproto::family_member_info_t *tmp_info = 
                    tmp_out_.mutable_list_out()->add_member_info();
                    tmp_info->CopyFrom(*member_info);
                    uint32_t page_num = tmp_out_.mutable_list_out()->page_member_num();
                    tmp_out_.mutable_list_out()->set_page_member_num(page_num + 1);
                }
            } else {
                commonproto::family_member_info_t *member_info = 
                    cli_out_.mutable_list_out()->mutable_member_info(j);
                if (ol_info->userid() == member_info->userid()) {
                    member_info->set_is_online(ol_info->is_online());
                    break;
                }
            }
        }
    }

    if (cli_in_.list_in().flag() == commonproto::FAMILY_MEMBER_LIST_ONLINE) {
        return send_msg_to_player(player, player->cli_wait_cmd, tmp_out_);
    }

    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyApplyCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    uint32_t family_id = cli_in_.family_id();

    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 检查是否重复加入
    uint32_t cur_family_id = GET_A(kAttrFamilyId);
    if (cur_family_id) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_repeat_join_family);
    }

    // 检查是否24小时内加入
    uint32_t last_leave_time = GET_A(kAttrLastLeaveFamilyTime);
    if (NOW() < last_leave_time + (24 * 60 * 60)){
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_join_family_too_fast);
    }

    // 申请操作锁
    std::ostringstream redis_key;
    redis_key << rankproto::STRING_FAMILY_DEAL_LOCK <<":"<< family_id;
    lock_key_ = redis_key.str();
    std::string value = Utils::to_string(player->userid);
    return RankUtils::lock_acquire(
            player, lock_key_, value, 
            commonproto::FAMILY_LOCK_EXPIRE_TIME);
}

int FamilyApplyCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case ranking_cmd_string_insert:
            return proc_pkg_from_serv_aft_lock_acquire(
                    player, body, bodylen);
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyApplyCmdProcessor::proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_lock_acquire_out_);

    uint32_t family_id = cli_in_.family_id();
    if(!rank_lock_acquire_out_.flag()) {
        // 锁被占用，返回提示
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_wait_for_lock_release);
    } else {
        // 申请锁成功,保存记录
        if (player->family_lock_sets != NULL) {
            player->family_lock_sets->push_back(lock_key_);
        }
    }

    db_get_family_info_in_.Clear();
    db_get_family_info_in_.set_flag(dbproto::DB_FAMILY_GET_INFO_WITH_MEMBER_LIST);
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_info, db_get_family_info_in_);
}

int FamilyApplyCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_info_out_);

    commonproto::family_info_t *family_info = 
        db_get_family_info_out_.mutable_family_info();
    // 检查家族是否存在
    if (family_info && 
            !FamilyUtils::is_valid_family_id(family_info->family_id())) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 检查是否同服
    if (family_info->server_id() != g_server_id) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 检查家族是否自动过期 
    if (FamilyUtils::is_family_out_of_date(*family_info)) {
        // 解散家族
        FamilyUtils::dismiss_family(player, cli_in_.family_id());
        for (int i = 0; i < db_get_family_info_out_.members_size();i++) {
            // 通知家族成员
            // 更新属性，并发送通知
            std::vector<commonproto::attr_data_t> attr_vec;
            commonproto::attr_data_t attr_data;
            attr_data.set_type(kAttrFamilyId);
            attr_data.set_value(0);
            attr_vec.push_back(attr_data);
            attr_data.set_type(kAttrFamilyLevel);
            attr_data.set_value(0);
            attr_vec.push_back(attr_data);
            AttrUtils::update_other_attr_value(
                    db_get_family_info_out_.members(i).userid(), 
                    db_get_family_info_out_.members(i).u_create_tm(),
                    attr_vec);
			// 发送家族过期解散消息
			commonproto::family_msg_type_t type = commonproto::FAMILY_MSG_TYPE_OUT_DATA;
			const commonproto::family_info_t &family_info_ = db_get_family_info_out_.family_info();
			FamilyUtils::send_family_msg_notice(
                    player, db_get_family_info_out_.members(i).userid(), 
                    db_get_family_info_out_.members(i).u_create_tm(),
                    type, family_info_.family_name(), family_info_.family_id());
        }

        // 返回客户端通知
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }
    
    // 检查家族人数
    if (FamilyUtils::is_family_member_full(
                *family_info, commonproto::FAMILY_TITLE_MEMBER)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_too_many_member);
    } 

    // 检查加入战力限制
    if (family_info->base_join_value() > GET_A(kAttrBattleValueRecord)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_no_enough_battle_value_join);
    }

    if (family_info->join_type() == commonproto::FAMILY_JOIN_NO_LIMIT) {
        // 可以直接加入
        // 初始化成员信息
        commonproto::family_member_info_t member_info;
        member_info.set_userid(player->userid);
        member_info.set_u_create_tm(player->create_tm);
        member_info.set_title(commonproto::FAMILY_TITLE_MEMBER);
        member_info.set_battle_value(GET_A(kAttrCurBattleValue));
        member_info.set_last_login_time(GET_A(kAttrLastLoginTm));
        member_info.set_last_logout_time(GET_A(kAttrLastLogoutTm));
		// int ret = 0;
        FamilyUtils::member_join_family(
                player, db_get_family_info_out_.family_info(), member_info);
    } else {
        // 需要审核

        // 事件表更新 
        db_family_update_event_in_.Clear();
        db_family_update_event_in_.mutable_family_event()->set_family_id(
                cli_in_.family_id());
        db_family_update_event_in_.mutable_family_event()->set_userid(
                player->userid);
        db_family_update_event_in_.mutable_family_event()->set_u_create_tm(
                player->create_tm);
        db_family_update_event_in_.mutable_family_event()->set_event_type(
                commonproto::FAMILY_EVENT_TYPE_APPLY);
        db_family_update_event_in_.mutable_family_event()->set_event_time(NOW());
        g_dbproxy->send_msg(
                0, cli_in_.family_id(), 0,
                db_cmd_family_update_event, db_family_update_event_in_);

        // 更新申请记录
        player->family_apply_record->insert(cli_in_.family_id());
        commonproto::family_apply_record_t apply_record;
        FOREACH(*(player->family_apply_record), iter) {
            apply_record.add_family_ids(*iter);
        }
        PlayerUtils::update_user_raw_data(
                player->userid, player->create_tm, 
                dbproto::FAMILY_APPLY_RECORD, apply_record, "0");
    }

    // 释放锁
    FamilyUtils::family_lock_release(player, lock_key_);
    lock_key_="";

    cli_out_.Clear();
    cli_out_.set_family_id(cli_in_.family_id());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyQuitCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    uint32_t family_id = GET_A(kAttrFamilyId);
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }
    
    // 申请操作锁
    std::ostringstream redis_key;
    redis_key << rankproto::STRING_FAMILY_DEAL_LOCK <<":"<< family_id;
    lock_key_ = redis_key.str();
    std::string value = Utils::to_string(player->userid);
    return RankUtils::lock_acquire(
            player, lock_key_, value, 
            commonproto::FAMILY_LOCK_EXPIRE_TIME);
}

int FamilyQuitCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case ranking_cmd_string_insert:
            return proc_pkg_from_serv_aft_lock_acquire(
                    player, body, bodylen);
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        case db_cmd_family_get_next_leader:
            return proc_pkg_from_serv_aft_family_get_next_leader(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyQuitCmdProcessor::proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_lock_acquire_out_);

    uint32_t family_id = GET_A(kAttrFamilyId);

    if(!rank_lock_acquire_out_.flag()) {
        // 锁被占用，返回提示
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_wait_for_lock_release);
    } else {
        // 申请锁成功,保存记录
        if (player->family_lock_sets != NULL) {
            player->family_lock_sets->push_back(lock_key_);
        }
    }

    db_get_family_info_in_.Clear();
    db_get_family_info_in_.set_flag(dbproto::DB_FAMILY_GET_INFO_WITH_MEMBER_LIST);
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_info, db_get_family_info_in_);
}

int FamilyQuitCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_info_out_);

    // 族长不能退出家族，只能转让或者解散,下面的自动移交规则废除
    if (player->userid == db_get_family_info_out_.mutable_family_info()->leader_id()) {
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_leader_cannot_leave);
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    // 检查是否最后一人, 是就自动解散, 不用单独退出
    if (db_get_family_info_out_.mutable_family_info()->member_num() == 1 && 
            db_get_family_info_out_.members(0).userid() == player->userid) {
        FamilyUtils::dismiss_family(player, family_id);
        FamilyUtils::clear_self_family_info(player);
        cli_out_.Clear();
        return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    } 

    // 不是最后一人,退出家族
    commonproto::family_member_info_t member;
    member.set_userid(player->userid);
    member.set_u_create_tm(player->create_tm);
    member.set_battle_value(GET_A(kAttrCurBattleValue));
    FamilyUtils::member_leave_family(
            player, family_id, member);

    // 是族长退出就移交
    if (player->userid == db_get_family_info_out_.mutable_family_info()->leader_id()) {
        db_get_next_leader_in_.Clear();
        db_get_next_leader_in_.set_leader_id(player->userid);
        db_get_next_leader_in_.set_leader_create_tm(player->create_tm);
        return g_dbproxy->send_msg(
                player, family_id, player->create_tm, 
                db_cmd_family_get_next_leader, db_get_next_leader_in_);
    }

    // 不是族长，直接退出
    cli_out_.Clear();
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyQuitCmdProcessor::proc_pkg_from_serv_aft_family_get_next_leader(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_next_leader_out_);
    uint32_t next_leader_id = db_get_next_leader_out_.next_leader_id();
    uint32_t next_leader_create_tm = db_get_next_leader_out_.next_leader_create_tm();
    if (is_valid_uid(next_leader_id) > 0) {
        uint32_t family_id = GET_A(kAttrFamilyId);
        FamilyUtils::leader_reassign(
                player, family_id, player->userid, 
                player->create_tm, next_leader_id, next_leader_create_tm);
    }

    // 释放锁
    FamilyUtils::family_lock_release(player, lock_key_);
    lock_key_="";
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyLeaderReassignCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    // 不能转让给自己
    if (player->userid == cli_in_.role().userid() && 
            player->create_tm == cli_in_.role().u_create_tm()){
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_cannot_reassign_self);
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_not_exist);
    }

    // 检查应答交互
    if (!(is_valid_uid(player->temp_info.family_leader_reassign_role.userid) && 
            player->temp_info.family_leader_reassign_role.userid == cli_in_.role().userid() &&
            player->temp_info.family_leader_reassign_role.u_create_tm == cli_in_.role().u_create_tm() &&
            player->temp_info.family_leader_reassign_response)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_reassign_need_response);
    }
    
    // 申请操作锁
    std::ostringstream redis_key;
    redis_key << rankproto::STRING_FAMILY_DEAL_LOCK <<":"<< family_id;
    lock_key_ = redis_key.str();
    std::string value = Utils::to_string(player->userid);
    return RankUtils::lock_acquire(
            player, lock_key_, value, 
            commonproto::FAMILY_LOCK_EXPIRE_TIME);
}

int FamilyLeaderReassignCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case ranking_cmd_string_insert:
            return proc_pkg_from_serv_aft_lock_acquire(
                    player, body, bodylen);
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyLeaderReassignCmdProcessor::proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_lock_acquire_out_);

    uint32_t family_id = GET_A(kAttrFamilyId);
    if(!rank_lock_acquire_out_.flag()) {
        // 锁被占用，返回提示
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_wait_for_lock_release);
    } else {
        // 申请锁成功,保存记录
        if (player->family_lock_sets != NULL) {
            player->family_lock_sets->push_back(lock_key_);
        }
    }

    db_get_family_info_in_.Clear();
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_info, db_get_family_info_in_);
}

int FamilyLeaderReassignCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_info_out_);
    // 检查自己是否族长
    if (player->userid != db_get_family_info_out_.mutable_family_info()->leader_id()) {
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_reassign_need_leader);
    }

    uint32_t target_userid = player->temp_info.family_leader_reassign_role.userid;
    uint32_t target_u_create_tm = player->temp_info.family_leader_reassign_role.u_create_tm;
    // 族长转让
    uint32_t family_id = GET_A(kAttrFamilyId);
    FamilyUtils::leader_reassign(
            player, family_id, player->userid, player->create_tm, 
            target_userid, target_u_create_tm);

    cli_out_.Clear();
    cli_out_.mutable_role()->set_userid(
            player->temp_info.family_leader_reassign_role.userid);
    cli_out_.mutable_role()->set_u_create_tm(
            player->temp_info.family_leader_reassign_role.u_create_tm);

    // 清除转让应答记录
    player->temp_info.family_leader_reassign_role.userid = 0;
    player->temp_info.family_leader_reassign_role.u_create_tm = 0;
    player->temp_info.family_leader_reassign_response = false;

    // 家族日志更新
    std::vector<commonproto::family_log_para_t> paras;
    commonproto::family_log_para_t para;
    para.set_type(0);
    para.set_value(Utils::to_string(0));
    para.mutable_role()->set_userid(player->userid);
    para.mutable_role()->set_u_create_tm(player->create_tm);
    paras.push_back(para);
    para.set_type(0);
    para.set_value(Utils::to_string(0));
    para.mutable_role()->set_userid(target_userid);
    para.mutable_role()->set_u_create_tm(target_u_create_tm);
    paras.push_back(para);
    FamilyUtils::insert_family_log(
            family_id, commonproto::FAMILY_LOG_TYPE_LEADER_REASSIGN, paras);

    // 释放锁
    FamilyUtils::family_lock_release(player, lock_key_);
    lock_key_="";
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilySetMemberTitleCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    // 不能修改自己身份
    if (player->userid == cli_in_.role().userid() &&
            player->create_tm == cli_in_.role().u_create_tm()){
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_cannot_reassign_self);
    }

    family_set_member_title_session_t *session = 
        (family_set_member_title_session_t *)player->session;
    session->role.userid = cli_in_.role().userid();
    session->role.u_create_tm = cli_in_.role().u_create_tm();
    session->title = cli_in_.title();

    uint32_t family_id = GET_A(kAttrFamilyId);

    // 申请操作锁
    std::ostringstream redis_key;
    redis_key << rankproto::STRING_FAMILY_DEAL_LOCK <<":"<< family_id;
    lock_key_ = redis_key.str();
    std::string value = Utils::to_string(player->userid);
    return RankUtils::lock_acquire(
            player, lock_key_, value, commonproto::FAMILY_LOCK_EXPIRE_TIME);
}

int FamilySetMemberTitleCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case ranking_cmd_string_insert:
            return proc_pkg_from_serv_aft_lock_acquire(
                    player, body, bodylen);
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        case db_cmd_family_get_member_info:
            return proc_pkg_from_serv_aft_family_get_member_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}



int FamilySetMemberTitleCmdProcessor::proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_lock_acquire_out_);

    uint32_t family_id = GET_A(kAttrFamilyId);

    if(!rank_lock_acquire_out_.flag()) {
        // 锁被占用，返回提示
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_wait_for_lock_release);
    } else {
        // 申请锁成功,保存记录
        if (player->family_lock_sets != NULL) {
            player->family_lock_sets->push_back(lock_key_);
        }
    }

    db_get_family_info_in_.Clear();
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_info, db_get_family_info_in_);
}

int FamilySetMemberTitleCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_info_out_);

    family_set_member_title_session_t *session = 
        (family_set_member_title_session_t *)player->session;

    uint32_t family_id = GET_A(kAttrFamilyId);
    // 检查身份人数限制
    //if (cli_in_.title() == commonproto::FAMILY_TITLE_VICE_LEADER) {
    if (session->title == commonproto::FAMILY_TITLE_VICE_LEADER) {
        if (FamilyUtils::is_family_member_full(
                    db_get_family_info_out_.family_info(), 
                    commonproto::FAMILY_TITLE_VICE_LEADER)) {
            return send_err_to_player(
                    player, player->cli_wait_cmd, cli_err_family_too_many_vice_leader);
        } 
    }

    db_get_member_info_in_.Clear();
    commonproto::role_info_t *role_info = db_get_member_info_in_.add_users();
    role_info->set_userid(player->userid);
    role_info->set_u_create_tm(player->create_tm);
    role_info->set_server_id(g_server_id);
    role_info = db_get_member_info_in_.add_users();
    role_info->set_userid(session->role.userid);
    role_info->set_u_create_tm(session->role.u_create_tm);
    //role_info->CopyFrom(cli_in_.role());
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_member_info, db_get_member_info_in_);
}

int FamilySetMemberTitleCmdProcessor::proc_pkg_from_serv_aft_family_get_member_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_member_info_out_);

    commonproto::family_member_info_t *setter = 
            db_get_member_info_out_.mutable_member_infos(0);
    commonproto::family_member_info_t *setted = 
            db_get_member_info_out_.mutable_member_infos(1);

    uint32_t family_id = GET_A(kAttrFamilyId);

    family_set_member_title_session_t *session = 
        (family_set_member_title_session_t *)player->session;

    // 权限检查
    //int err = check_set_title_right(setter, setted, cli_in_.title());
    int err = check_set_title_right(setter, setted, session->title);
    if (err) {
        return send_err_to_player(
                player, player->cli_wait_cmd, err);
    }
    if (cli_in_.title() == 0) {
        // 踢出成员
        FamilyUtils::member_leave_family(player, family_id, *setted);
    }  else {
        // 成员任命或降职
        // 身份更新
        dbproto::cs_family_update_member_info up_info;
        up_info.Clear();
        up_info.mutable_member_info()->set_family_id(family_id);
        //up_info.mutable_member_info()->set_userid(cli_in_.role().userid());
        //up_info.mutable_member_info()->set_u_create_tm(cli_in_.role().u_create_tm());
        //up_info.mutable_member_info()->set_title(cli_in_.title());
        up_info.mutable_member_info()->set_userid(session->role.userid);
        up_info.mutable_member_info()->set_u_create_tm(session->role.u_create_tm);
        up_info.mutable_member_info()->set_title(session->title);
        up_info.set_flag(dbproto::DB_UPDATE_NO_INSERT);
        g_dbproxy->send_msg(
                0, family_id, 0, db_cmd_family_update_member_info, up_info);    

        uint32_t msg_type[][3] = {
            {commonproto::FAMILY_TITLE_VICE_LEADER, 
                commonproto::FAMILY_MSG_TYPE_TITLE_VICE_LEADER, 
                commonproto::FAMILY_LOG_TYPE_PROMOTION},
            {commonproto::FAMILY_TITLE_MEMBER, 
                commonproto::FAMILY_MSG_TYPE_TITLE_MEMBER,
                commonproto::FAMILY_LOG_TYPE_DEMOTION},
        };

        for (uint32_t i = 0; i < array_elem_num(msg_type);i++) {
            //if (cli_in_.title() == msg_type[i][0]) {
            if (session->title == msg_type[i][0]) {
                // 更新玩家职位记录
                std::vector<commonproto::attr_data_t> attr_vec;
                commonproto::attr_data_t attr_data;
                attr_data.set_type(kAttrFamilyTitle);
                attr_data.set_value(msg_type[i][0]);
                attr_vec.push_back(attr_data);
                //AttrUtils::update_other_attr_value(
                //cli_in_.role().userid(), cli_in_.role().u_create_tm(),attr_vec);
                AttrUtils::update_other_attr_value(
                        session->role.userid, session->role.u_create_tm,attr_vec);

                // 通知玩家
                //FamilyUtils::send_family_msg_notice(
                        //player, cli_in_.role().userid(), cli_in_.role().u_create_tm(),
                        //(commonproto::family_msg_type_t)msg_type[i][1],
                        //player->family_name, family_id);
                FamilyUtils::send_family_msg_notice(
                        player, session->role.userid, session->role.u_create_tm,
                        (commonproto::family_msg_type_t)msg_type[i][1],
                        player->family_name, family_id);

                // 家族日志更新
                std::vector<commonproto::family_log_para_t> paras;
                commonproto::family_log_para_t para;
                para.set_type(0);
                para.set_value(Utils::to_string(0));
                //para.mutable_role()->CopyFrom(cli_in_.role());
                para.mutable_role()->set_userid(session->role.userid);
                para.mutable_role()->set_u_create_tm(session->role.u_create_tm);
                paras.push_back(para);
                FamilyUtils::insert_family_log(
                        family_id, (commonproto::family_log_type_t)msg_type[i][2], paras);
                break;
            }
        }
    }

    // 释放锁
    FamilyUtils::family_lock_release(player, lock_key_);
    lock_key_="";

    cli_out_.Clear();
    //cli_out_.mutable_role()->CopyFrom(cli_in_.role());
    //cli_out_.set_title(cli_in_.title());
    cli_out_.mutable_role()->set_userid(session->role.userid);
    cli_out_.mutable_role()->set_u_create_tm(session->role.u_create_tm);
    cli_out_.set_title(session->title);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilySetMemberTitleCmdProcessor::check_set_title_right(
        commonproto::family_member_info_t *setter,
        commonproto::family_member_info_t *setted, uint32_t title)
{
    int err = cli_err_family_no_right_to_change_title;
    if (setter == NULL || setted == NULL) {
        return err;
    }

    // 只有完全符合数组预设的身份才能设置
    // [目标身份, 设置者身份, 被设置者身份]
    uint32_t title_conds[][3] = {
        // 任命副族长
        {commonproto::FAMILY_TITLE_VICE_LEADER, commonproto::FAMILY_TITLE_LEADER, 
            commonproto::FAMILY_TITLE_MEMBER},
        // 降职为普通成员
        {commonproto::FAMILY_TITLE_MEMBER, commonproto::FAMILY_TITLE_LEADER, 
            commonproto::FAMILY_TITLE_VICE_LEADER},
        // 族长踢出副族长
        {0, commonproto::FAMILY_TITLE_LEADER, commonproto::FAMILY_TITLE_VICE_LEADER},
        // 族长踢出组员
        {0, commonproto::FAMILY_TITLE_LEADER, commonproto::FAMILY_TITLE_MEMBER},
        // 副族长踢出组员
        {0, commonproto::FAMILY_TITLE_VICE_LEADER, commonproto::FAMILY_TITLE_MEMBER},
    };

    for (uint32_t i = 0; i < array_elem_num(title_conds); i++) {
         if (title == title_conds[i][0] &&
                 setter->title() == title_conds[i][1] &&
                 setted->title() == title_conds[i][2]) {
                err = 0;
                break;
         }
    }

    return err;
}

int FamilyConstructCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    uint32_t family_id = cli_in_.family_id();
    if (!(family_id == GET_A(kAttrFamilyId) && 
            FamilyUtils::is_valid_family_id(family_id))){
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);       
    }

    CHECK_MSG_DATA_IN_CLOSED_INTERVAL(
            uint32_t,
            cli_in_.type(), 
            commonproto::FAMILY_CONSTUCT_TYPE_1,
            commonproto::FAMILY_CONSTUCT_TYPE_3,
            cli_err_data_error
            );

    // 建设规则
    uint32_t cond[][2] = {
        {0, commonproto::FAMILY_LOG_TYPE_CONSTRUCT_ITEM},
        {kDailyFamilyConstructGold, commonproto::FAMILY_LOG_TYPE_CONSTRUCT_GOLD},
        {kDailyFamilyConstructDiamond, commonproto::FAMILY_LOG_TYPE_CONSTRUCT_DIAMOND},
    };

    const family_contribute_conf_t * contribute_conf = 
        g_family_conf_mgr.get_contribute_config(cli_in_.type());
    if (contribute_conf == NULL) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_data_error);
    }

    uint32_t limit_type = cond[cli_in_.type() - 1][0];
    uint32_t construct_value = contribute_conf->construct_value;
    uint32_t contribute_value = contribute_conf->contribution;
    uint32_t req_add = contribute_conf->req_add;
    uint32_t base_cost_value = contribute_conf->base_cost_value;

    uint32_t log_type = cond[cli_in_.type() - 1][1];

    // 检查每日限制
    if (limit_type > 0 && GET_A((attr_type_t)limit_type) == 0) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_construct_over_daily_limit);
    }

    // 检查金币建设规则
    if (cli_in_.type() == commonproto::FAMILY_CONSTUCT_TYPE_2) {
        uint32_t level = GET_A(kAttrLv);
        uint32_t level_limit = g_family_conf_mgr.get_common_config(13, 20);
        if (level < level_limit) {
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_gold_construct_level_low);
        }
    }

    uint32_t family_level = GET_A(kAttrFamilyLevel);
    family_level = (family_level > 0) ? family_level:1;
    uint32_t cost_item_cnt = (family_level - 1) * req_add + base_cost_value;

    if (cli_in_.type() == commonproto::FAMILY_CONSTUCT_TYPE_1) {
        std::vector<reduce_item_info_t> reduce_vec;
        reduce_item_info_t reduce;
        reduce.item_id = contribute_conf->cost_item;
        reduce.count = cost_item_cnt;
        reduce_vec.push_back(reduce);
        int ret = swap_item_by_item_id(player, &reduce_vec, 0, false);
        if (ret) {
            ERROR_TLOG("family construct reduce item failed :%u", ret);
            return send_err_to_player(player, player->cli_wait_cmd, ret);
        }
    } else if (cli_in_.type() == commonproto::FAMILY_CONSTUCT_TYPE_2) {
        int ret = AttrUtils::sub_player_gold(player, cost_item_cnt, "家族建设");
        if (ret) {
            ERROR_TLOG("family construct consume gold failed :%u", ret);
            return send_err_to_player(player, player->cli_wait_cmd, ret);
        }
    } else if (cli_in_.type() == commonproto::FAMILY_CONSTUCT_TYPE_3) {
        uint32_t cost_item_id = commonproto::SERVICE_PRODUCT_FAMILY_CONSTRUCT;
        //// 钻石建设消耗=基础值+(家族等级-1)*req_add
        int err = buy_attr_and_use(
                player, kServiceFamilyConstructDiamond, 
                cost_item_id, cost_item_cnt);
        if (err) {
            return send_err_to_player(player, player->cli_wait_cmd, err);
        }
    }

    if (limit_type > 0) {
        SUB_A((attr_type_t)limit_type, 1);
    }

    //防沉迷
	if(check_player_addicted_threshold_none(player)){
		construct_value = 0;
        contribute_value = 0;
	} else if (check_player_addicted_threshold_half(player)){
		  construct_value /= 2;
          contribute_value /= 2;
	}

    // 更新家族建设值
    db_change_info_in_.Clear();
    dbproto::family_info_change_data_t *family_change_data = 
        db_change_info_in_.mutable_change_info();
    family_change_data->set_family_id(family_id);
    family_change_data->set_construct_value(construct_value);
    g_dbproxy->send_msg(
            0, family_id, 0, db_cmd_family_change_info, db_change_info_in_);

    // 更新个人累计贡献
    db_change_member_info_in_.Clear();
    dbproto::family_member_change_data_t *change_data = 
        db_change_member_info_in_.mutable_change_member_info();
    change_data->set_family_id(family_id);
    change_data->set_userid(player->userid);
    change_data->set_left_construct_value(contribute_value);
    change_data->set_total_construct_value(contribute_value);
    g_dbproxy->send_msg(
            0, family_id, 0, db_cmd_family_change_member_info, 
            db_change_member_info_in_);

    // 更新个人可消耗贡献
    ADD_A(kAttrFamilyContributeValue, contribute_value);

    // 更新家族日志
    std::vector<commonproto::family_log_para_t> paras;
    commonproto::family_log_para_t para;
    para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_UID);
    para.set_value(Utils::to_string(0));
    para.mutable_role()->set_userid(player->userid);
    para.mutable_role()->set_u_create_tm(player->create_tm);
    paras.push_back(para);

    para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_DATA);
    para.set_value(Utils::to_string(cost_item_cnt));
    para.mutable_role()->set_userid(0);
    para.mutable_role()->set_u_create_tm(0);
    paras.push_back(para);

    para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_DATA);
    para.set_value(Utils::to_string(contribute_value));
    para.mutable_role()->set_userid(0);
    para.mutable_role()->set_u_create_tm(0);
    paras.push_back(para);

    para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_DATA);
    para.set_value(Utils::to_string(construct_value));
    para.mutable_role()->set_userid(0);
    para.mutable_role()->set_u_create_tm(0);
    paras.push_back(para);
    FamilyUtils::insert_family_log(
            family_id, (commonproto::family_log_type_t)log_type, paras);

    db_get_family_info_in_.Clear();
    db_get_family_info_in_.set_flag(dbproto::DB_FAMILY_GET_INFO_WITH_MEMBER_LIST);
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_info, db_get_family_info_in_);
}

int FamilyConstructCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyConstructCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_info_out_);
    uint32_t construct_value = 
        db_get_family_info_out_.mutable_family_info()->construct_value();
    uint32_t family_level = 
        db_get_family_info_out_.mutable_family_info()->level();

    uint32_t family_id = GET_A(kAttrFamilyId);
    FamilyUtils::compute_family_level(construct_value, family_level);
    uint32_t add_level = 0;
    // 更新家族等级，只增不减
    if (family_level > 
            db_get_family_info_out_.mutable_family_info()->level()) {
        add_level = family_level - db_get_family_info_out_.mutable_family_info()->level();

        db_update_family_info_in_.Clear();
        dbproto::family_info_table_t *update_data = 
            db_update_family_info_in_.mutable_family_info();
        update_data->set_family_id(family_id);
        update_data->set_level(family_level);
        FamilyUtils::update_family_info(NULL, *update_data, dbproto::DB_UPDATE_NO_INSERT);

        SET_A(kAttrFamilyLevel, family_level);

        // 更新家族日志
        std::vector<commonproto::family_log_para_t> paras;
        commonproto::family_log_para_t para;
        para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_DATA);
        para.set_value(Utils::to_string(family_level));
        para.mutable_role()->set_userid(0);
        para.mutable_role()->set_u_create_tm(0);
        paras.push_back(para);

        para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_DATA);
        para.set_value(Utils::to_string(family_level));
        para.mutable_role()->set_userid(0);
        para.mutable_role()->set_u_create_tm(0);
        paras.push_back(para);

        para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_DATA);
        uint32_t level_addition = FamilyUtils::get_family_level_addition(family_level);
        para.set_value(Utils::to_string(level_addition));
        para.mutable_role()->set_userid(0);
        para.mutable_role()->set_u_create_tm(0);
        paras.push_back(para);
        FamilyUtils::insert_family_log(
                family_id, commonproto::FAMILY_LOG_TYPE_LEVEL_UP, paras);

        // 修改其他成员家族等级
        for (int i = 0; i < db_get_family_info_out_.members_size();i++) {
            if (player->userid != db_get_family_info_out_.members(i).userid()) {
                std::vector<commonproto::attr_data_t> attr_vec;
                commonproto::attr_data_t attr_data;
                attr_data.set_type(kAttrFamilyLevel);
                attr_data.set_value(family_level);
                attr_vec.push_back(attr_data);
                AttrUtils::update_other_attr_value(
                        db_get_family_info_out_.members(i).userid(), 
                        db_get_family_info_out_.members(i).u_create_tm(),
                        attr_vec);
            }
        }

        PlayerUtils::calc_player_battle_value(player, onlineproto::ATTR_OTHER_REASON);
    }

    cli_out_.Clear();
    cli_out_.set_type(cli_in_.type());
    cli_out_.set_add_level(add_level);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyUpdateMsgInfoCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    // 检查家族id
    uint32_t family_id = GET_A(kAttrFamilyId);
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 检查长度限制
    if (cli_in_.board_msg().size() > commonproto::MAX_FAMILY_BOARD_MSG_LENGTH) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_board_msg_too_long);
    }

    int err = Utils::check_dirty_name(cli_in_.board_msg(), false);
    if (err) {
        return send_err_to_player(player, player->cli_wait_cmd, err);
    }

    db_get_member_info_in_.Clear();
    //db_get_member_info_in_.add_userids(player->userid);
    commonproto::role_info_t *role_info = db_get_member_info_in_.add_users();
    role_info->set_userid(player->userid);
    role_info->set_u_create_tm(player->create_tm);
    role_info->set_server_id(g_server_id);
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_member_info, db_get_member_info_in_);
}

int FamilyUpdateMsgInfoCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case db_cmd_family_get_member_info:
            return proc_pkg_from_serv_aft_family_get_member_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyUpdateMsgInfoCmdProcessor::proc_pkg_from_serv_aft_family_get_member_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_member_info_out_);
    //  检查身份
    if (db_get_member_info_out_.member_infos(0).title() != 
            commonproto::FAMILY_TITLE_LEADER) {
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_no_right_to_update_msg);
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    dbproto::family_info_table_t family_info;
    family_info.Clear();
    family_info.set_family_id(family_id);
    family_info.set_board_msg(cli_in_.board_msg());
    FamilyUtils::update_family_info(NULL, family_info, dbproto::DB_UPDATE_NO_INSERT);

    // 更新家族日志
    std::vector<commonproto::family_log_para_t> paras;
    commonproto::family_log_para_t para;
    para.set_type(commonproto::FAMILY_LOG_PARA_TYPE_UID);
    para.set_value(Utils::to_string(0));
    para.mutable_role()->set_userid(player->userid);
    para.mutable_role()->set_u_create_tm(player->create_tm);
    paras.push_back(para);
    FamilyUtils::insert_family_log(
            family_id, commonproto::FAMILY_LOG_TYPE_BOARD_MSG_UPDATE, paras);

    cli_out_.Clear();
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int RequireRandomNameCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    if (!(cli_in_.type() >= commonproto::NAME_POOL_FAMILY)) {
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_data_error);
    }

    cli_out_.Clear();
    cli_out_.set_name(Utils::get_rand_name(cli_in_.type()));
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyInviteCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    uint32_t family_id = GET_A(kAttrFamilyId);
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    cs_get_user_by_nick_in_.Clear();
    cs_get_user_by_nick_in_.set_nick(cli_in_.username(0));
    return g_dbproxy->send_msg(player, player->userid, player->create_tm,
            db_cmd_get_userid_by_nick , cs_get_user_by_nick_in_);
}

int FamilyInviteCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case db_cmd_get_userid_by_nick:
            return proc_pkg_from_serv_aft_get_userid(
                    player, body, bodylen);
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyInviteCmdProcessor::proc_pkg_from_serv_aft_get_userid(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(sc_get_user_by_nick_out_);
    family_invite_session_t *session = 
        (family_invite_session_t *)player->session;
    session->userid = sc_get_user_by_nick_out_.userid();
    session->u_create_tm = sc_get_user_by_nick_out_.u_create_tm();

    if (!is_valid_uid(session->userid)) {
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_user_not_exist);
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    db_get_family_info_in_.Clear();
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, db_cmd_family_get_info, db_get_family_info_in_);
}

int FamilyInviteCmdProcessor::send_family_invite_notice(
        player_t *player, uint32_t userid)
{
    switchproto::cs_sw_transmit_only cs_sw_transmit_only_;
    switchproto::sw_player_basic_info_t* sw_player_basic_info = 
        cs_sw_transmit_only_.add_receivers();
    sw_player_basic_info->set_userid(userid);

    cs_sw_transmit_only_.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
    cs_sw_transmit_only_.set_cmd(cli_cmd_cs_0x0715_family_invite);

    uint32_t family_id = GET_A(kAttrFamilyId);
    onlineproto::sc_0x0715_family_invite invite_out_;
    invite_out_.set_family_id(family_id);
    std::string pkg;
    invite_out_.SerializeToString(&pkg);
    cs_sw_transmit_only_.set_pkg(pkg);

    g_switch->send_msg(NULL, g_online_id, 0, sw_cmd_sw_transmit_only,
            cs_sw_transmit_only_);
    return 0;
}

int FamilyInviteCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_info_out_);
    // 检查家族人数
    if (FamilyUtils::is_family_member_full(
                db_get_family_info_out_.family_info(), 
                commonproto::FAMILY_TITLE_MEMBER)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_too_many_member);
    } 
    family_invite_session_t *session = (family_invite_session_t *)player->session;

    // 事件表更新 
    uint32_t family_id = GET_A(kAttrFamilyId);
    db_family_update_event_in_.Clear();
    db_family_update_event_in_.mutable_family_event()->set_family_id(
            family_id);
    db_family_update_event_in_.mutable_family_event()->set_userid(session->userid);
    db_family_update_event_in_.mutable_family_event()->set_u_create_tm(session->u_create_tm);
    db_family_update_event_in_.mutable_family_event()->set_event_type(
            commonproto::FAMILY_EVENT_TYPE_INVITE);
    db_family_update_event_in_.mutable_family_event()->set_event_time(NOW());
    g_dbproxy->send_msg(
            0, family_id, 0, 
            db_cmd_family_update_event, db_family_update_event_in_);

    // 通知玩家
    FamilyUtils::send_family_msg_notice(
            player, session->userid, session->u_create_tm, 
            commonproto::FAMILY_MSG_TYPE_INVITE,
            player->family_name,
            family_id);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyDealInviteCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    // 先删除家族邀请事件
    FamilyUtils::family_del_event(
            cli_in_.family_id(), player->userid, player->create_tm,
            commonproto::FAMILY_EVENT_TYPE_INVITE);

    if (cli_in_.type() == 0) {//拒绝
        cli_out_.set_family_id(cli_in_.family_id());
        cli_out_.set_type(cli_in_.type());
        RET_MSG;
    }
    // 不能重复加入
    uint32_t cur_family_id = GET_A(kAttrFamilyId);
    if (FamilyUtils::is_valid_family_id(cur_family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_repeat_join_family);
    }

    uint32_t invite_family_id = cli_in_.family_id();
    if (!FamilyUtils::is_valid_family_id(invite_family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 申请操作锁
    std::ostringstream redis_key;
    redis_key << rankproto::STRING_FAMILY_DEAL_LOCK <<":"<< invite_family_id;
    lock_key_ = redis_key.str();
    std::string value = Utils::to_string(player->userid);
    return RankUtils::lock_acquire(
            player, lock_key_, value, 
            commonproto::FAMILY_LOCK_EXPIRE_TIME);
}

int FamilyDealInviteCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case ranking_cmd_string_insert:
            return proc_pkg_from_serv_aft_lock_acquire(
                    player, body, bodylen);
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        case db_cmd_family_get_event_info:
            return proc_pkg_from_serv_aft_family_get_event(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyDealInviteCmdProcessor::proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_lock_acquire_out_);

    if(!rank_lock_acquire_out_.flag()) {
        // 锁被占用，返回提示
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_wait_for_lock_release);
    } else {
        // 申请锁成功,保存记录
        if (player->family_lock_sets != NULL) {
            player->family_lock_sets->push_back(lock_key_);
        }
    }

    uint32_t family_id = cli_in_.family_id();
    db_get_family_info_in_.Clear();
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_info, db_get_family_info_in_);
}

int FamilyDealInviteCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_info_out_);
    // 拉取家族信息验证
    uint32_t family_id = cli_in_.family_id();
    if (!FamilyUtils::is_valid_family_id(
                db_get_family_info_out_.mutable_family_info()->family_id())) {
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_not_exist);
    }

    commonproto::family_info_t *family_info = 
        db_get_family_info_out_.mutable_family_info();

    // 检查家族是否自动过期 
    if (FamilyUtils::is_family_out_of_date(*family_info)) {
        // 解散家族
        FamilyUtils::dismiss_family(player, cli_in_.family_id());
        for (int i = 0; i < db_get_family_info_out_.members_size();i++) {
            uint32_t member_id = db_get_family_info_out_.members(i).userid();
            uint32_t u_create_tm = db_get_family_info_out_.members(i).u_create_tm();
            // 更新属性，并发送通知更新在线用户内存记录
            std::vector<commonproto::attr_data_t> attr_vec;
            commonproto::attr_data_t attr_data;
            attr_data.set_type(kAttrFamilyId);
            attr_data.set_value(0);
            attr_vec.push_back(attr_data);
            attr_data.set_type(kAttrFamilyLevel);
            attr_data.set_value(0);
            attr_vec.push_back(attr_data);
            AttrUtils::update_other_attr_value(
                    member_id, u_create_tm,attr_vec);
            
            // 通知家族成员
            FamilyUtils::send_family_msg_notice(
                    player, member_id, u_create_tm,
                    commonproto::FAMILY_MSG_TYPE_OUT_DATA, 
                    family_info->family_name(), family_info->family_id());
        }

        // 返回客户端通知
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 检查是否同服
    if (family_info->server_id() != g_server_id) {
        FamilyUtils::family_del_event(
                family_info->family_id(), player->userid, player->create_tm,
                commonproto::FAMILY_EVENT_TYPE_INVITE);

        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 检查家族人数
    if (FamilyUtils::is_family_member_full(
                *family_info, commonproto::FAMILY_TITLE_MEMBER)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_too_many_member);
    } 

    db_get_family_event_in_.Clear();
    db_get_family_event_in_.set_family_id(family_id);
    db_get_family_event_in_.set_userid(player->userid);
    db_get_family_event_in_.set_u_create_tm(player->create_tm);
    db_get_family_event_in_.set_type(commonproto::FAMILY_EVENT_TYPE_INVITE);
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_event_info, db_get_family_event_in_);
}


int FamilyDealInviteCmdProcessor::proc_pkg_from_serv_aft_family_get_event(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_event_out_);

    // 拉取邀请事件验证
    if (db_get_family_event_out_.has_family_event()) {
        //  更新家族事件
        uint32_t family_id = cli_in_.family_id();
        uint32_t event_status = commonproto::FAMILY_EVENT_DEAL_NO;
        uint32_t msg_type = commonproto::FAMILY_MSG_TYPE_REFUSE_INVITE;
        const commonproto::family_info_t &family_info = 
            db_get_family_info_out_.family_info();
        if (cli_in_.type() == 1) {
            // 接受家族邀请
            // 初始化成员信息
            commonproto::family_member_info_t member_info;
            member_info.set_userid(player->userid);
            member_info.set_u_create_tm(player->create_tm);
            member_info.set_title(commonproto::FAMILY_TITLE_MEMBER);
            member_info.set_battle_value(GET_A(kAttrCurBattleValue));
            member_info.set_last_login_time(GET_A(kAttrLastLoginTm));
            member_info.set_last_login_time(GET_A(kAttrLastLogoutTm));
            FamilyUtils::member_join_family(
                    player, family_info, member_info);

            event_status = commonproto::FAMILY_EVENT_DEAL_ACCPET_APPLY;
            msg_type = commonproto::FAMILY_MSG_TYPE_ACCPET_INVITE;
        } 

        // 通知邀请方
        FamilyUtils::send_family_msg_notice(
                player, cli_in_.role().userid(), cli_in_.role().u_create_tm(),
                (commonproto::family_msg_type_t)msg_type,
                family_info.family_name(),
                family_id);
    }

    // 释放锁
    FamilyUtils::family_lock_release(player, lock_key_);
    lock_key_="";

    cli_out_.Clear();
    cli_out_.set_family_id(cli_in_.family_id());
    cli_out_.set_type(cli_in_.type());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyEventDealCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen){
    PARSE_MSG;

    // 不能重复加入
    uint32_t cur_family_id = GET_A(kAttrFamilyId);
    uint32_t family_id = cli_in_.family_id();
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 只能处理自己家族事件
    if (cur_family_id != family_id) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 申请操作锁
    std::ostringstream redis_key;
    redis_key << rankproto::STRING_FAMILY_DEAL_LOCK <<":"<< family_id;
    lock_key_ = redis_key.str();
    std::string value = Utils::to_string(player->userid);
    return RankUtils::lock_acquire(
            player, lock_key_, value, 
            commonproto::FAMILY_LOCK_EXPIRE_TIME);

}

int FamilyEventDealCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case ranking_cmd_string_insert:
            return proc_pkg_from_serv_aft_lock_acquire(
                    player, body, bodylen);
        case db_cmd_family_get_event_info:
            return proc_pkg_from_serv_aft_family_get_event(
                    player, body, bodylen);
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        case db_cmd_get_attr:
            return proc_pkg_from_serv_aft_get_attr_info(
                    player, body, bodylen);
        case db_cmd_family_get_member_info:
            return proc_pkg_from_serv_aft_family_get_member_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyEventDealCmdProcessor::proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_lock_acquire_out_);

    if(!rank_lock_acquire_out_.flag()) {
        // 锁被占用，返回提示
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_wait_for_lock_release);
    } else {
        // 申请锁成功,保存记录
        if (player->family_lock_sets != NULL) {
            player->family_lock_sets->push_back(lock_key_);
        }
    }

    uint32_t family_id = cli_in_.family_id();
    db_get_family_event_in_.Clear();
    db_get_family_event_in_.set_family_id(family_id);
    db_get_family_event_in_.set_userid(cli_in_.role().userid());
    db_get_family_event_in_.set_u_create_tm(cli_in_.role().u_create_tm());
    db_get_family_event_in_.set_type(cli_in_.event_type());
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_event_info, db_get_family_event_in_);
}

int FamilyEventDealCmdProcessor::proc_pkg_from_serv_aft_family_get_event(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_event_out_);

    // 申请或邀请事件不存在
    if (db_get_family_event_out_.mutable_family_event()->event_time() == 0) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_event_not_exist);
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    db_get_family_info_in_.Clear();
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, db_cmd_family_get_info, db_get_family_info_in_);
}

int FamilyEventDealCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_info_out_);

    attr_info_in_.Clear();
    attr_info_in_.add_type_list(kAttrFamilyId);
    attr_info_in_.add_type_list(kAttrCurBattleValue);
    attr_info_in_.add_type_list(kAttrLastLoginTm);
    attr_info_in_.add_type_list(kAttrLastLogoutTm);
    return g_dbproxy->send_msg(
            player, cli_in_.role().userid(), cli_in_.role().u_create_tm(),
            db_cmd_get_attr, attr_info_in_);
}

int FamilyEventDealCmdProcessor::proc_pkg_from_serv_aft_get_attr_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(attr_info_out_);

    uint32_t family_id = GET_A(kAttrFamilyId);
    db_get_member_info_in_.Clear();
    //db_get_member_info_in_.add_userids(player->userid);
    commonproto::role_info_t *role_info = db_get_member_info_in_.add_users();
    role_info->set_userid(player->userid);
    role_info->set_u_create_tm(player->create_tm);
    role_info->set_server_id(g_server_id);
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_member_info, db_get_member_info_in_);
}

int FamilyEventDealCmdProcessor::proc_pkg_from_serv_aft_family_get_member_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_member_info_out_);
    //  检查身份
    uint32_t title = db_get_member_info_out_.member_infos(0).title();
    if ( title != commonproto::FAMILY_TITLE_LEADER && title != 
            commonproto::FAMILY_TITLE_VICE_LEADER){
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_cannot_deal_event);
    }

    // 检查家族人数
    if (cli_in_.deal_choice() == commonproto::FAMILY_EVENT_DEAL_ACCPET_APPLY &&
            FamilyUtils::is_family_member_full(
                db_get_family_info_out_.family_info(), 
                commonproto::FAMILY_TITLE_MEMBER)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_too_many_member);
    } 

    const commonproto::family_info_t &family_info = db_get_family_info_out_.family_info();

    // 检查是否同服
    if (family_info.server_id() != g_server_id) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 检查家族管理员身份 检查家族信息
    uint32_t family_id = cli_in_.family_id();

    // 检查重复加入
    if (cli_in_.deal_choice() == commonproto::FAMILY_EVENT_DEAL_ACCPET_APPLY) {
        for (int i = 0; i < attr_info_out_.attrs_size();i++) {
            commonproto::attr_data_t * attr_data = attr_info_out_.mutable_attrs(i);
            if (attr_data->type() == kAttrFamilyId &&
                    attr_data->value() > 0) {
                // 删除家族事件
                FamilyUtils::family_del_event(
                        family_id, cli_in_.role().userid(), 
                        cli_in_.role().u_create_tm(), cli_in_.event_type());

                return send_err_to_player(
                        player, player->cli_wait_cmd, cli_err_repeat_join_family);
            }
        }
    }

    //uint32_t msg_type = 0;
    if (cli_in_.event_type() == commonproto::FAMILY_EVENT_TYPE_APPLY &&
            cli_in_.deal_choice() == commonproto::FAMILY_EVENT_DEAL_ACCPET_APPLY) {
        // 接受家族申请
        // 初始化成员信息
        commonproto::family_member_info_t member_info;
        member_info.set_userid(cli_in_.role().userid());
        member_info.set_u_create_tm(cli_in_.role().u_create_tm());
        member_info.set_title(commonproto::FAMILY_TITLE_MEMBER);
        for (int i = 0; i < attr_info_out_.attrs_size();i++) {
            commonproto::attr_data_t * attr_data = attr_info_out_.mutable_attrs(i);
            if (attr_data->type() == kAttrCurBattleValue) {
                member_info.set_battle_value(attr_data->value());
            } else if (attr_data->type() == kAttrLastLoginTm) {
                member_info.set_last_login_time(attr_data->value());
            } else if (attr_data->type() == kAttrLastLogoutTm) {
                member_info.set_last_logout_time(attr_data->value());
            }
        }
        FamilyUtils::member_join_family(
                player, db_get_family_info_out_.family_info(), member_info);
        // 通知玩家
        FamilyUtils::send_family_msg_notice(
                player, member_info.userid(), member_info.u_create_tm(),
                commonproto::FAMILY_MSG_TYPE_JOIN_SUCC,
                family_info.family_name(),
                family_info.family_id());
    } else if (cli_in_.event_type() == commonproto::FAMILY_EVENT_TYPE_APPLY &&
            cli_in_.deal_choice() == commonproto::FAMILY_EVENT_DEAL_REFUSE_APPLY) {
        // 拒绝申请
        // 通知玩家
        FamilyUtils::send_family_msg_notice(
                player,cli_in_.role().userid(), cli_in_.role().u_create_tm(),
                commonproto::FAMILY_MSG_TYPE_APPLY_JOIN_FAILED,
                family_info.family_name(),
                family_info.family_id());
    } 

    // 删除家族事件
    FamilyUtils::family_del_event(
            family_id, cli_in_.role().userid(), 
            cli_in_.role().u_create_tm(), cli_in_.event_type());

    // 释放锁
    FamilyUtils::family_lock_release(player, lock_key_);
    lock_key_="";
    cli_out_.Clear();
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyGetEventListCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    uint32_t page_no = cli_in_.page_no();
    uint32_t page_size = cli_in_.page_size();
    if (page_no < 1 || page_size < 1 || 
            page_size > MAX_GET_FAMILY_EVENT_LIST_PAGE_SIZE) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_page_size_over_limit);
    }

    uint32_t family_id = cli_in_.family_id();
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    db_get_event_list_in_.Clear();
    db_get_event_list_in_.set_family_id(family_id);
    db_get_event_list_in_.set_userid(cli_in_.role().userid());
    db_get_event_list_in_.set_u_create_tm(cli_in_.role().u_create_tm());
    db_get_event_list_in_.set_type(cli_in_.type());
    db_get_event_list_in_.set_page_no(page_no);
    db_get_event_list_in_.set_page_size(page_size);

    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_event_list, db_get_event_list_in_);
}

int FamilyGetEventListCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{

    switch (player->serv_cmd) {
        case db_cmd_family_get_event_list:
            return proc_pkg_from_serv_aft_get_event_list(player, body, bodylen);
        case cache_cmd_ol_req_users_info:
            return proc_pkg_from_serv_aft_get_cache_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyGetEventListCmdProcessor::proc_pkg_from_serv_aft_get_event_list(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_event_list_out_);

    uint32_t page_num = db_get_event_list_out_.page_num();
    uint32_t total_num = db_get_event_list_out_.total_num();

    cli_out_.Clear();
    cli_out_.mutable_event_infos()->CopyFrom(db_get_event_list_out_.event_infos());
    cli_out_.set_total_num(total_num);
    cli_out_.set_page_num(page_num);

    CACHE_OUT_MSG(cli_out_);
    // 拉取事件目标玩家基本信息
    cache_info_in_.Clear();
    for (int i = 0; i < db_get_event_list_out_.event_infos_size(); i++) {
        commonproto::role_info_t *role_info = cache_info_in_.add_roles();
        role_info->set_userid(
                db_get_event_list_out_.mutable_event_infos(i)->role().userid());
        role_info->set_u_create_tm(
                db_get_event_list_out_.mutable_event_infos(i)->role().u_create_tm());
        role_info->set_server_id(g_server_id);
    }
    return g_dbproxy->send_msg(
            player, player->userid, player->create_tm, 
            cache_cmd_ol_req_users_info, cache_info_in_);
}

int FamilyGetEventListCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(cache_info_out_);
    PARSE_OUT_MSG(cli_out_);

    onlineproto::sc_0x0716_family_get_event_list      cli_tmp_out_;
    for (int i = 0; i < cli_out_.event_infos_size(); i++) {
        for (int j = 0; j < cache_info_out_.user_infos_size();j++) {
            commonproto::battle_player_data_t *player_info = 
                cache_info_out_.mutable_user_infos(j);
            // 过滤异常导致的其他服家族事件
            if (cli_out_.mutable_event_infos(i)->role().userid() ==
                    player_info->mutable_base_info()->user_id() &&
                cli_out_.mutable_event_infos(i)->role().u_create_tm() ==
                    player_info->mutable_base_info()->create_tm() &&
                    player_info->mutable_base_info()->server_id() == g_server_id) {
                commonproto::family_event_t *f_event = cli_tmp_out_.add_event_infos();
                f_event->CopyFrom(cli_out_.event_infos(i));
                f_event->mutable_user_info()->CopyFrom(player_info->base_info());
            } else {
                // 删除家族事件
                commonproto::family_event_t *f_event = cli_out_.mutable_event_infos(i);
                FamilyUtils::family_del_event(
                        f_event->family_id(),f_event->role().userid(), 
                        f_event->role().u_create_tm(),f_event->event_type());
            }
        }
    }

    cli_tmp_out_.set_total_num(cli_out_.total_num());
    cli_tmp_out_.set_page_num(cli_out_.page_num());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_tmp_out_);
}

int FamilyGetLogListCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    uint32_t page_no = cli_in_.page_no();
    uint32_t page_size = cli_in_.page_size();
    if (page_no < 1 || page_size < 1 || 
            page_size > MAX_GET_FAMILY_EVENT_LIST_PAGE_SIZE) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_page_size_over_limit);
    }

    uint32_t family_id = cli_in_.family_id();
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    db_get_log_list_in_.Clear();
    db_get_log_list_in_.set_family_id(family_id);
    //db_get_log_list_in_.set_type(cli_in_.type());
    db_get_log_list_in_.set_type(commonproto::FAMILY_LOG_TYPE_All);
    db_get_log_list_in_.set_page_no(page_no);
    db_get_log_list_in_.set_page_size(page_size);

    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_log_list, db_get_log_list_in_);
}

int FamilyGetLogListCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{

    switch (player->serv_cmd) {
        case db_cmd_family_get_log_list:
            return proc_pkg_from_serv_aft_get_log_list(player, body, bodylen);
        case cache_cmd_ol_req_users_info:
            return proc_pkg_from_serv_aft_get_cache_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyGetLogListCmdProcessor::proc_pkg_from_serv_aft_get_log_list(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_log_list_out_);

	uint32_t page_num = db_get_log_list_out_.page_num();
    uint32_t total_num = db_get_log_list_out_.total_num();

    cli_out_.Clear();
    cache_info_in_.Clear();
    // 解析日志参数列表
    for (int i = 0; i < db_get_log_list_out_.log_infos_size(); i++) {
        commonproto::family_log_t *log_info = db_get_log_list_out_.mutable_log_infos(i);
        commonproto::family_log_t *out_log = cli_out_.add_log_infos();

        std::vector<std::string> log_para_str = split(log_info->log_str(), ';');
        FOREACH(log_para_str, iter) {
            std::vector<std::string> para_info = split(*iter, ',');
            if (para_info.size() != 4) {
                return send_err_to_player(
                        player, player->cli_wait_cmd, cli_err_family_log_para_illegal);
            }

            uint32_t type = atoi_safe(para_info[0].c_str());
            if (type == commonproto::FAMILY_LOG_PARA_TYPE_UID) {
                // 取出userid类型的参数
                commonproto::role_info_t *role_info = cache_info_in_.add_roles();
                role_info->set_userid(atoi_safe(para_info[2].c_str()));
                role_info->set_u_create_tm(atoi_safe(para_info[3].c_str()));
                role_info->set_server_id(g_server_id);
            }

            commonproto::family_log_para_t *out_para = out_log->add_log_paras();
            out_para->set_type(type);
            out_para->set_value(para_info[1]);
            out_para->mutable_role()->set_userid(atoi(para_info[2].c_str()));
            out_para->mutable_role()->set_u_create_tm(atoi(para_info[3].c_str()));
        }

        out_log->set_log_type(log_info->log_type());
        out_log->set_log_time(log_info->log_time());
        out_log->set_log_str("");
    }

    if (cache_info_in_.roles_size() == 0 ) {
        // 没有需要转换的userid
        cli_out_.set_total_num(total_num);
        cli_out_.set_page_num(page_num);
        return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    }

    cli_out_.set_total_num(total_num);
    cli_out_.set_page_num(page_num);

    CACHE_OUT_MSG(cli_out_);
    return g_dbproxy->send_msg(
			    player, player->userid, player->create_tm, 
			    cache_cmd_ol_req_users_info, cache_info_in_);

}

int FamilyGetLogListCmdProcessor::proc_pkg_from_serv_aft_get_cache_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(cache_info_out_);
    PARSE_OUT_MSG(cli_out_);
    // 解析日志中的玩家名字
    for (int i = 0; i < cli_out_.log_infos_size();i++) {
        commonproto::family_log_t *out_log = cli_out_.mutable_log_infos(i);
        for (int j = 0; j < out_log->log_paras_size();j++) {
            commonproto::family_log_para_t *out_para = out_log->mutable_log_paras(j);
            // 把userid替换成玩家名称
            if (out_para->type() == commonproto::FAMILY_LOG_PARA_TYPE_UID) {
                for (int k = 0; k < cache_info_out_.user_infos_size();k++) {
                    const commonproto::battle_player_data_t &player_info = 
                        cache_info_out_.user_infos(k);
                    uint32_t userid = out_para->role().userid();
                    uint32_t create_tm = out_para->role().u_create_tm();
                    if (userid == player_info.base_info().user_id() &&
                            create_tm == player_info.base_info().create_tm()) {
                        out_para->set_type(
                                (uint32_t)commonproto::FAMILY_LOG_PARA_TYPE_NICK);
                        out_para->set_value(player_info.base_info().nick());
                    }
                }
            }
        }
    }
    
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyGetRecommendListCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    // 检查是否重复加入
    uint32_t cur_family_id = GET_A(kAttrFamilyId);
    if (cur_family_id) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_repeat_join_family);
    }

    db_get_list_in_.Clear();
    db_get_list_in_.set_battle_value(GET_A(kAttrCurBattleValue));

    uint32_t low_percent = g_family_conf_mgr.get_common_config(10, 90);
    uint32_t high_percent = g_family_conf_mgr.get_common_config(11, 150);
    uint32_t recommend_num = g_family_conf_mgr.get_common_config(12, 10);

    db_get_list_in_.set_low_percent(low_percent);
    db_get_list_in_.set_high_percent(high_percent);
    db_get_list_in_.set_recommend_num(recommend_num);
    db_get_list_in_.set_server_id(g_server_id);

    return g_dbproxy->send_msg(
            player, 0, 0,
            db_cmd_family_get_recommend_list, db_get_list_in_);
}

int FamilyGetRecommendListCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case db_cmd_family_get_recommend_list:
            return proc_pkg_from_serv_aft_get_recommend_list(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyGetRecommendListCmdProcessor::proc_pkg_from_serv_aft_get_recommend_list(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_list_out_);
    std::vector<commonproto::family_rank_info_t> list_vec;
    for (int i = 0; i < db_get_list_out_.family_list_size(); i++) {
        list_vec.push_back(db_get_list_out_.family_list(i));
    }
    std::random_shuffle(list_vec.begin(), list_vec.end());
    cli_out_.Clear();
    FOREACH(list_vec, iter) {
        commonproto::family_rank_info_t *rank_info = cli_out_.add_family_list();
        rank_info->CopyFrom(*iter);
    }
    //cli_out_.mutable_family_list()->CopyFrom(db_get_list_out_.family_list());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyConfigCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    uint32_t family_id = GET_A(kAttrFamilyId);
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 检查族长身份
    db_get_family_info_in_.Clear();
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_info, db_get_family_info_in_);
}

int FamilyConfigCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyConfigCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_family_info_out_);
    if (db_get_family_info_out_.mutable_family_info()->leader_id() != player->userid) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_need_leader);
    }

    commonproto::family_info_t family_info = db_get_family_info_out_.family_info();
    // 检查是否同服
    if (family_info.server_id() != g_server_id) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    dbproto::family_info_table_t update_data;
    update_data.set_family_id(family_id);
    update_data.set_join_type(cli_in_.type());
    update_data.set_base_join_value(cli_in_.battle_value());
    FamilyUtils::update_family_info(NULL, update_data, dbproto::DB_UPDATE_NO_INSERT);

    family_info.set_join_type(cli_in_.type());
    if (cli_in_.type() == commonproto::FAMILY_JOIN_NEED_AUTH) {
        // 删除不需要审核的家族排行记录
        std::ostringstream key;
        key << commonproto::RANKING_TYPE_FAMILY << ":" << 
            commonproto::FAMILY_RANK_SUB_TYPE_4;
        RankUtils::rank_del_user(family_id, 0, key.str());

        key.str("");
        key << commonproto::RANKING_TYPE_FAMILY << ":" << 
            commonproto::FAMILY_RANK_SUB_TYPE_5;
        RankUtils::rank_del_user(family_id, 0, key.str());

        key.str("");
        key << commonproto::RANKING_TYPE_FAMILY << ":" << 
            commonproto::FAMILY_RANK_SUB_TYPE_6;
        RankUtils::rank_del_user(family_id, 0, key.str());
    }  else if (cli_in_.type() == commonproto::FAMILY_JOIN_NO_LIMIT){
        // 增加不需要审核的家族排行记录
        FamilyUtils::update_family_rank_score(family_id, family_info);
    }

    // 更新推荐家族库
    FamilyUtils::update_family_match_info(family_info);

    cli_out_.Clear();
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyMsgDelCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    if (cli_in_.msg_id_size() > commonproto::MAX_DEL_FAMILY_MSG_NUM) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_del_msg_too_many);
    }

    for (int i = 0; i < cli_in_.msg_id_size();i++) {
        db_user_raw_data_del_in_.Clear();
        db_user_raw_data_del_in_.set_type(dbproto::FAMILY_OFFLINE_MESSAGE);
        db_user_raw_data_del_in_.set_buff_id(cli_in_.msg_id(i));
        g_dbproxy->send_msg(
                0, player->userid, player->create_tm,
                db_cmd_user_raw_data_del, db_user_raw_data_del_in_);
    }

    cli_out_.Clear();
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyLeaderReassignRequestCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    // 不能转让给自己
    if (cli_in_.role().userid() == player->userid &&
            cli_in_.role().u_create_tm() == player->create_tm) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_cannot_reassign_self);
    }

    if(!is_valid_uid(cli_in_.role().userid())) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_userid_not_find);
    }

    // 拉取家族信息
    uint32_t family_id = GET_A(kAttrFamilyId);
    db_get_info_in_.Clear();
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, db_cmd_family_get_info, db_get_info_in_);
}

int FamilyLeaderReassignRequestCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case db_cmd_family_get_info:
            return proc_pkg_from_serv_aft_family_get_info(
                    player, body, bodylen);
        case db_cmd_family_get_member_info:
            return proc_pkg_from_serv_aft_family_get_member_info(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                    player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyLeaderReassignRequestCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_info_out_);

	const commonproto::family_info_t &family_info = db_get_info_out_.family_info();
    // 发起转让 只有族长可以发出消息
    if (cli_in_.type() == 0) {
        if (family_info.leader_id() != player->userid) {
            return send_err_to_player(
                    player, player->cli_wait_cmd, cli_err_family_reassign_need_leader);
        }
    }

    // 检查是否同服
    if (family_info.server_id() != g_server_id) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    db_get_member_info_in_.Clear();
    commonproto::role_info_t *role_info = db_get_member_info_in_.add_users();
    role_info->CopyFrom(cli_in_.role());
    //role_info->set_userid(cli_in_.userid());
    //role_info->set_u_create_tm(0);
    //role_info->set_server_id(0);
    return g_dbproxy->send_msg(
            player, family_id, player->create_tm, 
            db_cmd_family_get_member_info, db_get_member_info_in_);
}

int FamilyLeaderReassignRequestCmdProcessor::proc_pkg_from_serv_aft_family_get_member_info(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(db_get_member_info_out_);
    commonproto::family_member_info_t *member_info = 
            db_get_member_info_out_.mutable_member_infos(0);

    uint32_t family_id = GET_A(kAttrFamilyId);

    // 检查双方是否同一家族
    if (member_info->userid() == 0) {
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_reassign_need_same_family);
    }

    // 检查对方是否在线
    if (member_info->is_online() == false) {
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_reassign_need_online);
    }

	const commonproto::family_info_t &family_info = db_get_info_out_.family_info();
    uint32_t msg_type = 0;
    if (cli_in_.type() == 1) {
        // 回复族长，是否接受转让
        if (cli_in_.response() == false) {
            msg_type = commonproto::FAMILY_MSG_TYPE_REFUSE_LEADER_REASSIGN;
        } else {
            msg_type = commonproto::FAMILY_MSG_TYPE_ACCPET_LEADER_REASSIGN;
        }
    } else if (cli_in_.type() == 0) {
        // 通知被转让用户
        msg_type = commonproto::FAMILY_MSG_TYPE_LEADER_REASSIGN;
        player->temp_info.family_leader_reassign_role.userid = cli_in_.role().userid();
        player->temp_info.family_leader_reassign_role.u_create_tm = cli_in_.role().u_create_tm();
        player->temp_info.family_leader_reassign_response = false;
    }

    FamilyUtils::send_family_msg_notice(
            player, cli_in_.role().userid(), cli_in_.role().u_create_tm(),
            (commonproto::family_msg_type_t)msg_type,
            family_info.family_name(), family_id);

    cli_out_.Clear();
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyCancelApplyCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;

    if (cli_in_.family_ids_size() > commonproto::MAX_MUTI_FAMILY_OPER_NUM) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_too_many_family_oper);
    }

    if (cli_in_.family_ids_size() == 0) {
        cli_out_.Clear();
        return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
    }

    index = 0;
    uint32_t family_id = 0;
    // 从请求id集合中找到下一个有效的家族申请记录
    for (int i = 0; i < cli_in_.family_ids_size();i++) {
        if (player->family_apply_record->count(cli_in_.family_ids(index)) != 0) {
            family_id = cli_in_.family_ids(index);
            break;
        }
        index++;
    }

    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }

    // 申请操作锁
    std::ostringstream redis_key;
    redis_key << rankproto::STRING_FAMILY_DEAL_LOCK <<":"<< family_id;
    lock_key_ = redis_key.str();
    std::string value = Utils::to_string(player->userid);
    return RankUtils::lock_acquire(
            player, lock_key_, value, 
            commonproto::FAMILY_LOCK_EXPIRE_TIME);
}

int FamilyCancelApplyCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    switch (player->serv_cmd) {
        case ranking_cmd_string_insert:
            return proc_pkg_from_serv_aft_lock_acquire(
                    player, body, bodylen);
        default:
            return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_cmd_not_find);
    }

    return 0;
}

int FamilyCancelApplyCmdProcessor::proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen)
{
    PARSE_SVR_MSG(rank_lock_acquire_out_);

    if(!rank_lock_acquire_out_.flag()) {
        // 锁被占用，返回提示
        return send_err_to_player(
                player, player->cli_wait_cmd, 
                cli_err_family_wait_for_lock_release);
    } else {
        // 申请锁成功,保存记录
        if (player->family_lock_sets != NULL) {
            player->family_lock_sets->push_back(lock_key_);
        }
    }
    uint32_t family_id = cli_in_.family_ids(index);

    // 删除家族事件
    FamilyUtils::family_del_event(
            family_id, player->userid, player->create_tm,
            commonproto::FAMILY_EVENT_TYPE_APPLY);

    // 更新申请记录
    player->family_apply_record->erase(family_id);
    commonproto::family_apply_record_t apply_record;
    FOREACH(*(player->family_apply_record), iter) {
        apply_record.add_family_ids(*iter);
    }
    PlayerUtils::update_user_raw_data(
            player->userid, player->create_tm, 
            dbproto::FAMILY_APPLY_RECORD, apply_record, "0");

    // 释放锁
    FamilyUtils::family_lock_release(player, lock_key_);
    lock_key_="";

    if ((index + 1) < (uint32_t)cli_in_.family_ids_size()) {
        // 循环处理其他申请记录
        // 从请求id集合中找到下一个有效的家族申请记录
        family_id = 0;
        for (int i = 0; i < cli_in_.family_ids_size();i++) {
            index++;
            if (player->family_apply_record->count(cli_in_.family_ids(index)) != 0) {
                family_id = cli_in_.family_ids(index);
                break;
            }
        }

        if (FamilyUtils::is_valid_family_id(family_id)) {
            // 申请操作锁
            std::ostringstream redis_key;
            redis_key << rankproto::STRING_FAMILY_DEAL_LOCK <<":"<< family_id;
            lock_key_ = redis_key.str();
            std::string value = Utils::to_string(player->userid);
            return RankUtils::lock_acquire(
                    player, lock_key_, value, 
                    commonproto::FAMILY_LOCK_EXPIRE_TIME);
        }

        // 有效的家族申请记录已经处理完,协议返回
    }

    cli_out_.Clear();
    cli_out_.mutable_family_ids()->CopyFrom(cli_in_.family_ids());
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}


int FamilyGetApplyListCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    cli_out_.Clear();
    FOREACH(*(player->family_apply_record), iter) {
        cli_out_.add_family_id(*iter);
    }
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyTechUpCmdProcessor::proc_pkg_from_client(
        player_t *player, const char *body, int bodylen)
{
    PARSE_MSG;
    cli_out_.Clear();
    uint32_t tech_id = cli_in_.tech_id();
    const family_tech_conf_t *tech_conf = g_family_conf_mgr.get_tech_config(tech_id);
    if (tech_conf == NULL) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_tech_not_exist);
    }

    uint32_t family_id = GET_A(kAttrFamilyId);
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_not_exist);
    }
    
    uint32_t tech_lv = GET_A((attr_type_t)tech_conf->lv_attr);
    uint32_t max_lv = 5 * GET_A(kAttrFamilyLevel);
    if (tech_lv >= max_lv) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_tech_lv_reach_max);
    }

    // level * Math.ceil(level / coefficient) + req
    uint32_t cost_value = tech_lv * ceil(tech_lv * 1.0/ tech_conf->coefficient) + tech_conf->req;
    uint32_t own_value = GET_A(kAttrFamilyContributeValue);
    if (own_value < cost_value) {
        return send_err_to_player(
                player, player->cli_wait_cmd, cli_err_family_contribute_not_enough);
    } 
    SUB_A(kAttrFamilyContributeValue, cost_value);
    // 增加等级
    ADD_A((attr_type_t)tech_conf->lv_attr, 1);

    PlayerUtils::calc_player_battle_value(player);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int FamilyNameChangeCmdProcessor::proc_pkg_from_client(player_t *player,
		const char *body, int bodylen)
{
    PARSE_MSG;
	std::string new_name = cli_in_.new_family_name();
	if (new_name.size() > commonproto::FAMILY_NAME_MAX_LEN) {
		RET_ERR(cli_err_family_name_too_long);
	}
	int err = Utils::check_dirty_name(new_name);
	if (err) {
		RET_ERR(err);
	}
	if (Utils::is_number(new_name)) {
		RET_ERR(cli_err_family_name_cannot_be_number);
	}
	if (GET_A(kAttrLv) < commonproto::FAMILY_CREATE_LEVEL) {
		RET_ERR(cli_err_level_too_low);
	}
	std::vector<std::string> values;
	values.push_back(new_name);
	return RankUtils::set_is_member(player, player->userid,
			player->create_tm, 0, rankproto::SET_FAMILY_NAME, 0, values);
}

int FamilyNameChangeCmdProcessor::proc_pkg_from_serv(player_t *player,
		const char *body, int bodylen)
{
	switch (player->serv_cmd) {
		case ranking_cmd_set_is_member:
			return proc_pkg_from_serv_aft_set_is_member(player, body, bodylen);
		case db_cmd_family_get_member_info:
			return proc_pkg_from_serv_aft_get_member_info(player, body, bodylen);
		case db_cmd_family_get_info:
			return proc_pkg_from_serv_aft_family_get_info(player, body, bodylen);
	}
	return 0;
}

int FamilyNameChangeCmdProcessor::proc_pkg_from_serv_aft_set_is_member(
	player_t *player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(rank_set_is_member_out_);
	if (rank_set_is_member_out_.flags(0)) {
		RET_ERR(cli_err_family_name_already_exist);
	}
	dbproto::cs_family_get_member_info db_in;
	commonproto::role_info_t* role_info = db_in.add_users();
	role_info->set_userid(player->userid);
	role_info->set_u_create_tm(player->create_tm);
	role_info->set_server_id(g_server_id);
	return g_dbproxy->send_msg(player, GET_A(kAttrFamilyId),
			player->create_tm, db_cmd_family_get_member_info,
			db_in);
}

int FamilyNameChangeCmdProcessor::proc_pkg_from_serv_aft_get_member_info(
	player_t* player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(db_get_member_info_out_);
    if (!(db_get_member_info_out_.member_infos(0).title() == 
			commonproto::FAMILY_TITLE_LEADER || 
		db_get_member_info_out_.member_infos(0).title() == 
			commonproto::FAMILY_TITLE_VICE_LEADER)) {
			RET_ERR(cli_err_family_change_name_must_leader);
    }
	//扣除改名卡物品
	int ret = reduce_single_item(player, 38002, 1);
	if (ret) {
		RET_ERR(cli_err_no_enough_item_num);
	}
	dbproto::family_info_table_t db_in;
	db_in.set_family_id(GET_A(kAttrFamilyId));
	db_in.set_family_name(cli_in_.new_family_name());
	FamilyUtils::update_family_info(
			player, db_in, dbproto::DB_UPDATE_AND_INESRT);
	dbproto::cs_family_get_info db_get_info;
	db_get_info.set_flag(dbproto::DB_FAMILY_GET_INFO_WITH_MEMBER_LIST);
	return g_dbproxy->send_msg(player, GET_A(kAttrFamilyId), 
			player->create_tm, db_cmd_family_get_info, db_get_info);
}

int FamilyNameChangeCmdProcessor::proc_pkg_from_serv_aft_family_get_info(
		player_t* player, const char *body, int bodylen)
{
	PARSE_SVR_MSG(db_get_info_out_);
	for (int i = 0; i < db_get_info_out_.members_size(); ++i) {
		//给每个人发邮件
		FamilyUtils::send_family_msg_notice(player,
				db_get_info_out_.members(i).userid(),
				db_get_info_out_.members(i).u_create_tm(),
				commonproto::FAMILY_MSG_TYPE_FAMILY_CHANGE_NAME,
				cli_in_.new_family_name(), GET_A(kAttrFamilyId));
	}
	//世界广播
	onlineproto::sc_0x0611_system_noti noti;
	noti.set_reason(commonproto::SYSTEM_NOTI_FAMILY_CHANGE_NAME);
	std::string old_name = cli_in_.old_family_name();
	std::string new_name = cli_in_.new_family_name();
	static char msg[256];
	snprintf(msg, sizeof (msg), "[pi=%u|%u]%s[/pi]将家族名【[cl=0x0066ff]%s[/cl]】更改为【[cl=0x0066ff]%s[/cl]】！",
				player->userid, player->create_tm, player->nick, old_name.c_str(), new_name.c_str());
	std::string info;
	info.assign(msg);
	noti.set_info(info);
	uint16_t cmd = cli_cmd_cs_0x0611_system_noti;
	Utils::switch_transmit_msg(
			switchproto::SWITCH_TRANSMIT_WORLD, cmd,
			noti);
    return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
