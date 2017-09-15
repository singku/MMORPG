#include "family_utils.h"
#include "global_data.h"
#include "service.h"
#include "utils.h"
#include "rank_utils.h"
#include "player_utils.h"
#include "family_conf.h"
#include "map_utils.h"
#include "map_conf.h"
#include <boost/lexical_cast.hpp>

bool FamilyUtils::is_valid_family_id(uint32_t family_id)
{
    if (family_id >= commonproto::FAMILY_ID_START) {
        return true;
    } 

    return false;
}


bool FamilyUtils::is_valid_family_rank_type(uint32_t type)
{
    if (type == commonproto::RANKING_TYPE_FAMILY) {
        return true;
    }

    return false;
}

bool FamilyUtils::is_valid_family_title(uint32_t title)
{
    if (title >= commonproto::FAMILY_TITLE_LEADER && 
            title <= commonproto::FAMILY_TITLE_MEMBER) {
        return true;
    } 

    return false;
}

int FamilyUtils::update_family_info(
        player_t *p, const dbproto::family_info_table_t &info, uint32_t flag)
{
    dbproto::cs_family_update_info  db_in_;
    db_in_.Clear();
    db_in_.set_flag(flag);
    db_in_.mutable_family_info()->CopyFrom(info);
    return g_dbproxy->send_msg(
            p, info.family_id(), 0, db_cmd_family_update_info, db_in_);
}

int FamilyUtils::update_family_member_info(
        player_t *p, const dbproto::family_member_table_t &info, uint32_t flag)
{
    dbproto::cs_family_update_member_info  db_in_;
    db_in_.Clear();
    db_in_.mutable_member_info()->CopyFrom(info);
    db_in_.set_flag(flag);
    return g_dbproxy->send_msg(
            p, info.family_id(),info.u_create_tm(), db_cmd_family_update_member_info, db_in_);
}

int FamilyUtils::update_family_rank_score(
        const uint32_t family_id, 
        const commonproto::family_info_t &family_info)
{
    // 只能更新本服家族排名
    if (family_info.server_id() != g_server_id) {
        return 0;
    }

    // 并发限制，只能在拉取家族信息后和创建家族时调用

    // 更新所有家族排名
    // 双条件排名
    // 家族等级+战力排名    
    uint32_t level = 0;
    FamilyUtils::compute_family_level(
            family_info.construct_value(), level);
    uint64_t score = 
        ((uint64_t)level << 32) + family_info.total_battle_value();
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_1, score);

    // 家族战力+等级排名 
    uint64_t battle_value = (uint64_t)family_info.total_battle_value();
    score =  (battle_value << 32) + level;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_2, score);

    // 组员数+等级排名
    uint64_t member_num = (uint64_t)family_info.member_num();
    score = (member_num << 32) + level;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_3, score);

    // 更新不需要审核的家族排名
    // 家族等级+战力排名    
    score = ((uint64_t)level << 32) + family_info.total_battle_value();
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_4, score);

    // 家族战力+等级排名 
    score =  (battle_value << 32) + level;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_5, score);

    // 组员数+等级排名
    score = (member_num << 32) + level;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_6, score);

    // 单条件排名
    // 家族等级排名    
    //FamilyUtils::compute_family_level(
    //family_info.construct_value(), level);
    score = (uint64_t)level;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_7, score);

    // 家族战力排名 
    battle_value = (uint64_t)family_info.total_battle_value();
    score = battle_value;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_8, score);

    // 组员数排名
    member_num = (uint64_t)family_info.member_num();
    score = member_num;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_9, score);

    // 战力限制排名
    battle_value = (uint64_t)family_info.base_join_value();
    score = battle_value;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_100, score);

    // 更新不需要审核的家族排名
    // 家族等级排名    
    score = (uint64_t)level;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_10, score);

    // 家族战力排名 
    score =  battle_value;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_11, score);

    // 组员数排名
    score = member_num;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_12, score);

    // 战力限制排名
    battle_value = (uint64_t)family_info.base_join_value();
    score = battle_value;
    RankUtils::rank_user_insert_score(
            family_id, 0, commonproto::RANKING_TYPE_FAMILY,
            commonproto::FAMILY_RANK_SUB_TYPE_200, score);
    return 0;
}

int FamilyUtils::del_family_rank_info(uint32_t family_id)
{
    std::ostringstream key;
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_1;
    RankUtils::rank_del_user(family_id, 0, key.str());

    key.str("");
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_2;
    RankUtils::rank_del_user(family_id, 0, key.str());

    key.str("");
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_3;
    RankUtils::rank_del_user(family_id, 0, key.str());

    key.str("");
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

    key.str("");
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_7;
    RankUtils::rank_del_user(family_id, 0, key.str());

    key.str("");
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_8;
    RankUtils::rank_del_user(family_id, 0, key.str());

    key.str("");
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_9;
    RankUtils::rank_del_user(family_id, 0, key.str());

    key.str("");
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_10;
    RankUtils::rank_del_user(family_id, 0, key.str());

    key.str("");
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_11;
    RankUtils::rank_del_user(family_id, 0, key.str());

    key.str("");
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_12;
    RankUtils::rank_del_user(family_id, 0, key.str());

    key.str("");
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_100;
    RankUtils::rank_del_user(family_id, 0, key.str());

    key.str("");
    key << commonproto::RANKING_TYPE_FAMILY << ":" << 
        commonproto::FAMILY_RANK_SUB_TYPE_200;
    RankUtils::rank_del_user(family_id, 0, key.str());

    return 0;
}

int FamilyUtils::compute_family_level(
        const uint32_t construct_value, uint32_t &level)
{
    // 家族缺省等级为1
    level = 1;

    const std::map<uint32_t, family_level_conf_t> &level_config_map = 
        g_family_conf_mgr.get_level_conf_map();
    uint32_t i = 0; 
    FOREACH(level_config_map, iter) {
        if (construct_value < iter->second.need_construct_value && 
                iter->first > 1) {
            level = iter->first - 1;
            break;
        }
        ++i;
    }

    // 已经升到顶级
    if (i == level_config_map.size()) {
        level = i;
    }

    return 0;
}

/** 
 * @brief 成员离开后的家族信息更新
 * 
 * @param player 
 * @param family_id 
 * @param member_info 
 * 
 * @return 
 */
int FamilyUtils::member_leave_family(
       player_t *player,
       const uint32_t family_id, 
       const commonproto::family_member_info_t &member_info)
{
    uint32_t leave_userid = member_info.userid();
    uint32_t leave_u_create_tm = member_info.u_create_tm();
    if (!is_valid_uid(leave_userid)) {
        return 0;
    }
    
    if (player == NULL) {
        return 0;
    }

    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return 0;
    }

    // 对家族信息变更只能用change接口, update会互相覆盖
    // 更新家族db信息
    dbproto::cs_family_change_info db_in_;
    db_in_.mutable_change_info()->set_family_id(family_id);
    db_in_.mutable_change_info()->set_member_num(-1);
    g_dbproxy->send_msg(
            0, family_id, 0, db_cmd_family_change_info, db_in_);

    // 清除成员db记录
    dbproto::cs_family_quit db_quit_in_;
    db_quit_in_.set_userid(leave_userid);
    db_quit_in_.set_u_create_tm(member_info.u_create_tm());
    g_dbproxy->send_msg(0, family_id, 0, db_cmd_family_quit, db_quit_in_);

    // 更新家族在线记录
    std::vector<std::string> vector;
    vector.push_back(
            boost::lexical_cast<std::string>(
                ROLE_KEY(ROLE(leave_userid, leave_u_create_tm))));
    RankUtils::set_del_member(
            NULL, player->userid, player->create_tm,
            rankproto::SET_FAMILY_ONLINE_USERIDS, 
            family_id,
            vector);

    // 更新家族成员id缓存
    vector.clear();
    vector.push_back(
            boost::lexical_cast<std::string>(
                ROLE_KEY(ROLE(leave_userid, leave_u_create_tm))));
    RankUtils::set_del_member(
            NULL, player->userid, player->create_tm,
            rankproto::SET_FAMILY_ALL_USERIDS, 
            family_id,
            vector);

    // 更新id attr
    if (player && player->userid == member_info.userid() ) {
        // 主动退出家族
        FamilyUtils::clear_self_family_info(player);
        SET_A(kAttrLastLeaveFamilyTime, NOW());

        memset(player->family_name, 0, sizeof(player->family_name));

        // 家族日志更新
        std::vector<commonproto::family_log_para_t> paras;
        commonproto::family_log_para_t para;
        para.set_type(0);
        para.set_value(Utils::to_string(0));
        para.mutable_role()->set_userid(player->userid);
        para.mutable_role()->set_u_create_tm(player->create_tm);
        paras.push_back(para);
        FamilyUtils::insert_family_log(
                family_id, commonproto::FAMILY_LOG_TYPE_LEAVE, paras);

        PlayerUtils::calc_player_battle_value(player);
        MapUtils::sync_map_player_info(player, commonproto::PLAYER_FAMILY_CHANGE);
    } else {
        // 强制退出家族
        // 更新db属性
        std::vector<commonproto::attr_data_t> attr_vec;
        commonproto::attr_data_t attr_data;
        attr_data.set_type(kAttrFamilyId);
        attr_data.set_value(0);
        attr_vec.push_back(attr_data);

        attr_data.set_type(kAttrFamilyLevel);
        attr_data.set_value(0);
        attr_vec.push_back(attr_data);

        attr_data.set_type(kAttrFamilyTitle);
        attr_data.set_value(0);
        attr_vec.push_back(attr_data);

        attr_data.set_type(kAttrLastLeaveFamilyTime);
        attr_data.set_value(NOW());
        attr_vec.push_back(attr_data);
        AttrUtils::update_other_attr_value(
                leave_userid, leave_u_create_tm,  attr_vec);

        // 家族日志更新
        std::vector<commonproto::family_log_para_t> paras;
        commonproto::family_log_para_t para;
        para.set_type(0);
        para.set_value(Utils::to_string(0));
        para.set_value(Utils::to_string(leave_userid));
        para.set_value(Utils::to_string(leave_u_create_tm));
        paras.push_back(para);
        para.set_type(0);
        para.mutable_role()->set_userid(player->userid);
        para.mutable_role()->set_u_create_tm(player->create_tm);
        paras.push_back(para);
        FamilyUtils::insert_family_log(
                family_id, commonproto::FAMILY_LOG_TYPE_KICK_OUT, paras);

        // 通知玩家
        send_family_msg_notice(
                player, member_info.userid(), member_info.u_create_tm(),
                commonproto::FAMILY_MSG_TYPE_KICK_OUT,
                player->family_name,
                family_id);
    }

    // 更新家族推荐库信息
    FamilyUtils::change_family_match_info(family_id, -1);
    return 0;
}

/** 
 * @brief 新成员加入后的家族信息更新
 * 
 * @param player 
 * @param family_id 
 * @param member_info 
 * 
 * @return 
 */
int FamilyUtils::member_join_family(
       player_t *player,
       const commonproto::family_info_t &family_info,
       const commonproto::family_member_info_t &member_info)
{
    // 只判断id有效性，不判断是否重复加入
    uint32_t userid = member_info.userid();
    if (!is_valid_uid(userid)) {
        return 0;
    }

    if (player == NULL) {
        return 0;
    }

    
    uint32_t family_id = family_info.family_id();
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return 0;
    }

    uint32_t create_tm = member_info.u_create_tm();

    uint32_t battle_value = member_info.battle_value();
    // 插入成员信息
    dbproto::family_member_table_t up_info;
    up_info.set_family_id(family_id);
    up_info.set_userid(userid);
    up_info.set_u_create_tm(create_tm);
    up_info.set_battle_value(battle_value);
    up_info.set_title(member_info.title());
    up_info.set_join_time(NOW());
    up_info.set_last_login_time(member_info.last_login_time());
    up_info.set_last_logout_time(member_info.last_logout_time());
    FamilyUtils::update_family_member_info(0, up_info, dbproto::DB_UPDATE_AND_INESRT);

    // 更新家族信息
    dbproto::cs_family_change_info db_change_family_info_in_;
    db_change_family_info_in_.mutable_change_info()->set_family_id(family_id);
    db_change_family_info_in_.mutable_change_info()->set_member_num(1);
    g_dbproxy->send_msg(
            0, family_id, 0, db_cmd_family_change_info, db_change_family_info_in_);

    // 更新家族在线记录
    std::vector<std::string> family_user_vec;
    family_user_vec.push_back(
            boost::lexical_cast<std::string>(ROLE_KEY(ROLE(userid, create_tm))));
    RankUtils::set_insert_member(
            NULL, userid, member_info.u_create_tm(), g_server_id,
            rankproto::SET_FAMILY_ONLINE_USERIDS, 
            family_id,
            family_user_vec);

    // 更新家族成员id缓存
    family_user_vec.clear();
    family_user_vec.push_back(
            boost::lexical_cast<std::string>(ROLE_KEY(ROLE(userid, create_tm))));
    RankUtils::set_insert_member(
            NULL, userid, member_info.u_create_tm(), g_server_id,
            rankproto::SET_FAMILY_ALL_USERIDS, 
            family_id,
            family_user_vec);

    // 更新attr
    uint32_t family_level = family_info.level();
    if (player->userid == member_info.userid() && 
            player->create_tm == member_info.u_create_tm()) {
        // 自己加入家族
        SET_A(kAttrFamilyId, family_id);
        SET_A(kAttrFamilyLevel, family_level);
        SET_A(kAttrFamilyTitle, commonproto::FAMILY_TITLE_MEMBER);
        SET_A(kAttrLastLeaveFamilyTime, 0);

        STRCPY_SAFE(player->family_name, family_info.family_name().c_str());

        // 更新属性
        PlayerUtils::calc_player_battle_value(player, onlineproto::ATTR_OTHER_REASON);

        MapUtils::sync_map_player_info(player, commonproto::PLAYER_FAMILY_CHANGE);
    } else {
        // 申请加入通过
        std::vector<commonproto::attr_data_t> attr_vec;
        commonproto::attr_data_t attr_data;
        attr_data.set_type(kAttrFamilyId);
        attr_data.set_value(family_id);
        attr_vec.push_back(attr_data);

        attr_data.set_type(kAttrFamilyLevel);
        attr_data.set_value(family_level);
        attr_vec.push_back(attr_data);

        attr_data.set_type(kAttrFamilyTitle);
        attr_data.set_value(commonproto::FAMILY_TITLE_MEMBER);
        attr_vec.push_back(attr_data);

        attr_data.set_type(kAttrLastLeaveFamilyTime);
        attr_data.set_value(0);
        attr_vec.push_back(attr_data);

        AttrUtils::update_other_attr_value(userid, create_tm, attr_vec);
    }

    // 家族日志更新
    std::vector<commonproto::family_log_para_t> paras;
    commonproto::family_log_para_t para;
    para.set_type(0);
    para.set_value(Utils::to_string(0));
    para.mutable_role()->set_userid(userid);
    para.mutable_role()->set_u_create_tm(create_tm);
    paras.push_back(para);
    FamilyUtils::insert_family_log(
            family_id, commonproto::FAMILY_LOG_TYPE_JOIN, paras);

    // 更新家族推荐库信息
    FamilyUtils::change_family_match_info(family_id, 1);

    
    return 0;
}


/** 
 * @brief 检查各职位数量限制
 * 
 * @param family_info 
 * @param type 
 * 
 * @return 
 */
bool FamilyUtils::is_family_member_full(
    const commonproto::family_info_t &family_info, uint32_t type)
{
    uint32_t family_level = family_info.level();

    if (!is_valid_family_level(family_level)) {
        return false;
    }

    const family_level_conf_t *level_conf = g_family_conf_mgr.get_level_config(family_level);
    if (level_conf == NULL) {
        return false;
    }

    if (type == (uint32_t)commonproto::FAMILY_TITLE_VICE_LEADER) {
        // 检查副族长数
        if ((uint32_t)family_info.vice_leader_num() >= level_conf->max_vice_leader_num) {
            return true;
        }
    } else if (type == (uint32_t)commonproto::FAMILY_TITLE_MEMBER) {
        // 检查成员数
        if ((uint32_t)family_info.member_num() >= level_conf->max_member_num) {
            return true;
        }
    }

    return false;
}

bool FamilyUtils::is_valid_family_level(uint32_t level)
{
    if (level >= 1 && level <= commonproto::FAMILY_MAX_LEVEL) {
        return true;
    }

    return false;
}


int FamilyUtils::insert_family_log(
        uint32_t family_id, uint32_t type, 
        std::vector<commonproto::family_log_para_t> &paras)
{
    dbproto::cs_family_update_log  db_in_; 
    dbproto::family_log_table_t *up_info = db_in_.mutable_family_log();
 
    std::string log_msg;
    FOREACH(paras, it) {
        if (it == (paras.end() - 1)) {
            log_msg = log_msg + Utils::to_string(it->type()) + ',' + it->value() + ',' 
                + Utils::to_string(it->role().userid()) + ',' 
                + Utils::to_string(it->role().u_create_tm());
        } else {
            log_msg = log_msg + Utils::to_string(it->type()) + ',' + it->value() + ','
                + Utils::to_string(it->role().userid()) + ',' 
                + Utils::to_string(it->role().u_create_tm()) + ';';
        }
    }

    if (log_msg.size() > commonproto::MAX_FAMILY_LOG_MSG_LENGTH) {
        return cli_err_family_log_msg_too_long;
    } 

    up_info->set_log_id(0);
    up_info->set_family_id(family_id);
    up_info->set_log_type(type);
    up_info->set_log_msg(log_msg);
    up_info->set_log_time(NOW());
    g_dbproxy->send_msg(
            NULL, family_id, 0, db_cmd_family_update_log, db_in_);
    return 0;
}

int FamilyUtils::leader_reassign(
        player_t *player,
        uint32_t family_id, uint32_t old_leader_id, uint32_t old_create_tm,
        uint32_t new_leader_id, uint32_t new_create_tm)
{
    if (family_id == 0 || 
            old_leader_id == 0 || 
            new_leader_id == 0) {
        return 0;
    }

    dbproto::cs_family_update_member_info up_info;
    up_info.Clear();
    up_info.mutable_member_info()->set_family_id(family_id);
    up_info.mutable_member_info()->set_userid(new_leader_id);
    up_info.mutable_member_info()->set_u_create_tm(new_create_tm);
    up_info.mutable_member_info()->set_title(commonproto::FAMILY_TITLE_LEADER);
    up_info.set_flag(dbproto::DB_UPDATE_NO_INSERT);

    g_dbproxy->send_msg(
            0, family_id, 0, db_cmd_family_update_member_info, up_info);

    // 更新属性，并发送通知
    std::vector<commonproto::attr_data_t> attr_vec;
    commonproto::attr_data_t attr_data;
    attr_data.set_type(kAttrFamilyTitle);
    attr_data.set_value(commonproto::FAMILY_TITLE_LEADER);
    attr_vec.push_back(attr_data);
    AttrUtils::update_other_attr_value(new_leader_id, new_create_tm, attr_vec);

    // 自己变成普通成员
    up_info.Clear();
    up_info.mutable_member_info()->set_family_id(family_id);
    up_info.mutable_member_info()->set_userid(old_leader_id);
    up_info.mutable_member_info()->set_u_create_tm(old_create_tm);
    up_info.mutable_member_info()->set_title(commonproto::FAMILY_TITLE_MEMBER);
    up_info.set_flag(dbproto::DB_UPDATE_NO_INSERT);
    g_dbproxy->send_msg(
            0, family_id, 0, db_cmd_family_update_member_info, up_info);
    SET_A(kAttrFamilyTitle, 0);

    return 0;
}

int FamilyUtils::dismiss_family(player_t *player, uint32_t family_id)
{
    if (!is_valid_family_id(family_id)) {
        return 0;
    }

    // 删除家族db记录
    dbproto::cs_family_dismiss_family db_dismiss_family_in_;
    g_dbproxy->send_msg(
            0, family_id, 0,
            db_cmd_family_dismiss_family, db_dismiss_family_in_);

    // 删除家族redis记录
    rankproto::cs_clear_family_info rank_clear_family_in_;
    rank_clear_family_in_.add_family_ids(family_id);
    rank_clear_family_in_.set_server_id(g_server_id);
    g_dbproxy->send_msg(
            0, family_id, 0, ranking_cmd_clear_family_info, rank_clear_family_in_);

    // 删除家族排名记录
    FamilyUtils::del_family_rank_info(family_id);

    // 删除家族推荐库记录
    FamilyUtils::del_match_info(family_id);

    // 删除内存记录
    if (player) {
        memset(player->family_name, 0, sizeof(player->family_name));
        MapUtils::sync_map_player_info(player, commonproto::PLAYER_FAMILY_CHANGE);
    }
    return 0;
}

/** 
 * @brief 离开家族时 清理个人信息里的家族记录
 * 
 * @param player 
 * 
 * @return 
 */
int FamilyUtils::clear_self_family_info(player_t *player)
{
    // 离开大厅
    if(player->cur_map_id == FAMILY_MAP_ID)  {
        MapUtils::leave_map(player);
    }

    SET_A(kAttrFamilyId, 0);
    SET_A(kAttrFamilyLevel, 0);
    SET_A(kAttrFamilyTitle, 0);
    memset(player->family_name, 0, sizeof(player->family_name));

    // 家族副本信息不重置
    //SET_A(kAttrFamilyBossHp, 0);
    //SET_A(kAttrFamilyBossStage, 0);
    //SET_A(kAttrFamilyBossLv, 0);
    //SET_A(kAttrFamilyPlayerLv, 0);
    //SET_A(kAttrFamilyBossLastResetTime, 0);
    //SET_A(kAttrFamilyBossLastFightFailedTime, 0);
    //SET_A(kAttrFamilyBossFightTimes, 0);
    //SET_A(kAttrFamilyBossDamage, 0;

    return 0;
}

int FamilyUtils::update_family_match_info(const commonproto::family_info_t &info)
{
    if (!FamilyUtils::is_valid_family_id(info.family_id())) {
        return 0;
    }

    uint32_t full_flag = 0;
    if (FamilyUtils::is_family_member_full(
                info, commonproto::FAMILY_TITLE_MEMBER)) {
        full_flag = 1;
    } 
    dbproto::cs_family_update_match_info match_info;
    match_info.mutable_match_info()->set_family_id(info.family_id());
    match_info.mutable_match_info()->set_server_id(info.server_id());
    match_info.mutable_match_info()->set_family_name(info.family_name());
    match_info.mutable_match_info()->set_member_num(info.member_num());
    match_info.mutable_match_info()->set_family_level(info.level());
    match_info.mutable_match_info()->set_join_type(info.join_type());
    match_info.mutable_match_info()->set_base_join_value(info.base_join_value());
    match_info.mutable_match_info()->set_is_full(full_flag);
    if (info.total_battle_value() > 0) {
        match_info.mutable_match_info()->set_total_battle_value(info.total_battle_value());
    }
   if (info.create_time() > 0) {
        match_info.mutable_match_info()->set_create_time(info.create_time());
   }
    g_dbproxy->send_msg(
            0, 0, 0, db_cmd_family_update_match_info, match_info);
    return 0;
}

int FamilyUtils::change_family_match_info(uint32_t family_id, int member_num)
{
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return 0;
    }

    // 推荐家族中的成员数量需要实时信息,避免出现显示家族未满，但无法加入
    // 战力等信息在查询家族,创建和解散时更新
    dbproto::cs_family_change_match_info change_info;
    change_info.mutable_change_match_info()->set_family_id(family_id);
    change_info.mutable_change_match_info()->set_member_num(member_num);
    g_dbproxy->send_msg(
            0, 0, 0, db_cmd_family_change_match_info, change_info);
    return 0;
}

int FamilyUtils::del_match_info(uint32_t family_id)
{
    if (!FamilyUtils::is_valid_family_id(family_id)) {
        return 0;
    }

    dbproto::cs_family_del_match_info del_match_info;
    del_match_info.set_family_id(family_id);
    g_dbproxy->send_msg(
            0, 0, 0, db_cmd_family_del_match_info, del_match_info);
    return 0;
}

int FamilyUtils::family_lock_release(player_t *player, std::string &lock_key_str)
{
    if (player == NULL) {
        return 0;
    }

    RankUtils::lock_release(0, player->userid, player->create_tm, lock_key_str);
    if (player->family_lock_sets != NULL) {
        FOREACH(*(player->family_lock_sets), it) {
            if(*it == lock_key_str) {
                player->family_lock_sets->erase(it);
                break;
            }
        }
    }

    return 0;
}

bool FamilyUtils::is_family_out_of_date(commonproto::family_info_t &family_info)
{
    if (!family_info.has_family_id() || 
            !FamilyUtils::is_valid_family_id(family_info.family_id())) {
        return false;
    }

    if (family_info.last_member_login_time() > 0 && 
        NOW() > family_info.last_member_login_time() + 30 * 24 * 60 * 60) {
        return true;
    }

    return false;
}

int FamilyUtils::send_family_msg_notice( player_t *player,
		uint32_t reciever, uint32_t create_tm, commonproto::family_msg_type_t type, 
		std::string family_name, uint32_t family_id)
{
    if (player == NULL) {
        return 0;
    }

    onlineproto::sc_0x0720_family_msg_notice msg_notice;
    commonproto::family_msg_t *family_msg = msg_notice.add_msgs();
    // family_msg->set_type(commonproto::FAMILY_MSG_TYPE_JOIN_SUCC);
    family_msg->set_type(type);
    family_msg->mutable_sender()->set_userid(player->userid);
    family_msg->mutable_sender()->set_u_create_tm(player->create_tm);
    std::string name(player->nick);
    family_msg->set_sender_name(name);
    family_msg->set_family_name(family_name);
    family_msg->set_family_id(family_id);
    std::string buff_id = Utils::gen_uuid();
    family_msg->set_msg_id(buff_id);
    // 更新离线消息
    PlayerUtils::update_user_raw_data(
            reciever, create_tm, dbproto::FAMILY_OFFLINE_MESSAGE, *family_msg, buff_id);

    // 在线通知
    switchproto::cs_sw_transmit_only cs_sw_transmit_only_;
    switchproto::sw_player_basic_info_t* sw_player_basic_info = 
        cs_sw_transmit_only_.add_receivers();
    sw_player_basic_info->set_userid(reciever);
    sw_player_basic_info->set_create_tm(0);
    cs_sw_transmit_only_.set_transmit_type(switchproto::SWITCH_TRANSMIT_USERS);
    cs_sw_transmit_only_.set_cmd(cli_cmd_cs_0x0720_family_msg_notice);

    std::string pkg;
    msg_notice.SerializeToString(&pkg);
    cs_sw_transmit_only_.set_pkg(pkg);

    g_switch->send_msg(NULL, g_online_id, 0, sw_cmd_sw_transmit_only,
            cs_sw_transmit_only_);
    return 0;
}

/** 
 * @brief 登陆时推送离线家族消息
 * 
 * @param player 
 * @param login_info 
 * 
 * @return 
 */
int FamilyUtils::send_offline_family_msg_notice(
        player_t *player,
        const dbproto::sc_get_login_info &login_info)
{
    if (player == NULL){
        return 0;
    }

    if (login_info.family_msgs_size() > 0) {
        onlineproto::sc_0x0720_family_msg_notice msg_notice;
        msg_notice.mutable_msgs()->CopyFrom(login_info.family_msgs());
        send_msg_to_player(player, cli_cmd_cs_0x0720_family_msg_notice, msg_notice);

        // 更新申请记录
        bool flag = false;
        for (int i = 0; i < msg_notice.msgs_size();i++) {
            commonproto::family_msg_t *family_msg = msg_notice.mutable_msgs(i);
            if (family_msg->type() == commonproto::FAMILY_MSG_TYPE_JOIN_SUCC ||
                   family_msg->type() == commonproto::FAMILY_MSG_TYPE_APPLY_JOIN_FAILED) {
                        player->family_apply_record->erase(family_msg->family_id());
            }
        }

        if (flag) {
            commonproto::family_apply_record_t apply_record;
            FOREACH(*(player->family_apply_record), iter) {
                apply_record.add_family_ids(*iter);
            }
            PlayerUtils::update_user_raw_data(
                    player->userid, player->create_tm, dbproto::FAMILY_APPLY_RECORD, apply_record, "0");
        }
    }

    return 0;
}

int FamilyUtils::family_attr_addition(
        player_t *player, std::map<uint32_t, uint32_t> &attr_map)
{
    // 家族基础加成
    uint32_t family_level = GET_A(kAttrFamilyLevel);
    if (!is_valid_family_level(family_level)) {
        return 0;
    }

    uint32_t attrs[11] = {
        kAttrHpMax,
        kAttrNormalAtk,
        kAttrNormalDef,
        kAttrSkillAtk,
        kAttrSkillDef,
        kAttrCrit,
        kAttrAntiCrit,
        kAttrHit,
        kAttrDodge,
        kAttrBreakBlock,
        kAttrBlock,
    };

    uint32_t level_addition = get_family_level_addition(family_level);
    for (uint32_t i = 0; i < array_elem_num(attrs);i++) {
        attr_map[attrs[i]] *= (1000 + level_addition) / 1000.0;
    }

    // 家族技能加成
    for (uint32_t tech_id = commonproto::MIN_FAMILY_TECH_ID; tech_id <= commonproto::MAX_FAMILY_TECH_ID; tech_id++) {
        const family_tech_conf_t* tech_conf = g_family_conf_mgr.get_tech_config(tech_id);
        if (tech_conf == NULL) {
            continue;
        }
        uint32_t tech_lv = GET_A((attr_type_t)tech_conf->lv_attr);
        attr_map[tech_conf->attr_id] += tech_conf->base_value * tech_lv;
    }

    return 0;
}

int FamilyUtils::get_family_level_addition(uint32_t level)
{
    if (!(level > 0 && level <= commonproto::FAMILY_MAX_LEVEL)) {
        return 0;
    }

    uint32_t level_addition[commonproto::FAMILY_MAX_LEVEL] =  {10, 14, 18, 22, 26, 30, 34, 38, 45, 50};
    return level_addition[level - 1];
}

/** 
 * @brief 删除家族事件
 * 
 * @param family_id     家族id
 * @param userid        事件目标用户id
 * @param type          事件类型
 * 
 * @return 
 */
int FamilyUtils::family_del_event(
            uint32_t family_id, uint32_t userid, uint32_t create_tm, uint32_t type)
{
    dbproto::cs_family_del_event db_family_del_event_in_;
    db_family_del_event_in_.Clear();
    db_family_del_event_in_.set_family_id(family_id);
    db_family_del_event_in_.set_userid(userid);
    db_family_del_event_in_.set_u_create_tm(create_tm);
    db_family_del_event_in_.set_type(type);
    return g_dbproxy->send_msg(
            0, family_id, 0,
            db_cmd_family_del_event, db_family_del_event_in_);
}

bool FamilyUtils::is_valid_family_dup_stage_id(uint32_t stage_id)
{
    if (stage_id > 0 && stage_id <= commonproto::MAX_FAMILY_DUP_STAGE_ID) {
        return true;
    }

    return false;
}

/** 
 * @brief 登陆时刷新家族成员信息
 * 
 * @param player 
 * 
 * @return 
 */
int FamilyUtils::refresh_family_member_info(player_t *player)
{
    if (player == NULL) {
        return 0;
    }  

    uint32_t family_id = GET_A(kAttrFamilyId);
    if (FamilyUtils::is_valid_family_id(family_id)) {
        dbproto::family_member_table_t up_info;
        up_info.Clear();
        up_info.set_family_id(family_id);
        up_info.set_userid(player->userid);
        up_info.set_u_create_tm(player->create_tm);
        up_info.set_battle_value(GET_A(kAttrCurBattleValue));
        up_info.set_last_login_time(GET_A(kAttrLastLoginTm));
        FamilyUtils::update_family_member_info(0, up_info, dbproto::DB_UPDATE_NO_INSERT);

        // 更新家族在线记录
        std::vector<std::string> vector;
        vector.push_back(
                boost::lexical_cast<std::string>(
                    ROLE_KEY(ROLE(player->userid, player->create_tm))));
        RankUtils::set_insert_member(
                NULL, player->userid, player->create_tm, g_server_id,
                rankproto::SET_FAMILY_ONLINE_USERIDS, 
                family_id,
                vector);
    }

    return 0;
}
