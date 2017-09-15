
#include <stdlib.h>
#include <libtaomee++/utils/strings.hpp>

#include <vector>

#include "dll_iface.h"
#include "xmlutils.h"
#include "time_utils.h"
#include "global_data.h"
#include "xml_configs.h"
#include "utils.h"
#include "pet_conf.h"
#include "skill_conf.h"
#include "duplicate_conf.h"
#include "duplicate_trigger.h"
#include "builder_conf.h"
#include "map_conf.h"
#include "affix_conf.h"

#define ERR_RT \
    do { \
        if (g_load_conf_cnt == 0) { \
            return -1; \
        } else { \
            return 0; \
        } \
    } while (0)

// 加载地图配置信息
int load_map_config(xmlNodePtr root)
{
    map_conf_manager_t tmp;
    tmp.clear();
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)"map")) {
            cur = cur->next;
            continue;
        }

        map_conf_t map;
        DECODE_XML_PROP_UINT32(map.id, cur, "id");
        if (tmp.is_map_conf_exist(map.id)) {
            ERROR_TLOG("map %u already exists", map.id);
            ERR_RT;
        }
        tmp.add_map_conf(map);
        cur = cur->next;
    }

    g_map_conf_mgr.copy_from(tmp);
    return 0;
}


int load_skill_level_config(xmlNodePtr root, skill_conf_manager_t &mgr)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("skill"))) {
            cur = cur->next;
            continue;
        }
        skill_conf_t skill;
        uint32_t skill_id;
        DECODE_XML_PROP_UINT32(skill_id, cur, "parent_skill_id");
        if (!mgr.skill_conf_exist(skill_id)) {
            ERROR_TLOG("Skill_id %u not exist for skill.xml in skill_parent.xml",
                    skill_id);
            return -1;
        }
        skill_conf_t *parent_skill = mgr.find_mutable_skill_conf(skill_id);

        DECODE_XML_PROP_UINT32(skill.skill_level, cur, "level");
        skill.skill_id = skill_id * 10 + skill.skill_level;
        if (mgr.skill_conf_exist(skill.skill_id)) {
            ERROR_TLOG("Duplicate skill_id_level in skill.xml[id %u, level %u]",
                    skill_id, skill.skill_level);
            return -1;
        }
        DECODE_XML_PROP_UINT32(skill.learn_level, cur, "learn_level");
        DECODE_XML_PROP_UINT32(skill.cd, cur, "cd");
        DECODE_XML_PROP_UINT32(skill.sp, cur, "tp");
        DECODE_XML_PROP_UINT32_DEFAULT(skill.normal_hurt_rate, cur, "normal_hurt", 100);
        DECODE_XML_PROP_UINT32_DEFAULT(skill.skill_hurt_rate, cur, "skill_hurt", 100);

        DECODE_XML_PROP_UINT32_DEFAULT(skill.hits, cur, "hits", 0);
        if (!skill.hits) {
            skill.hits = parent_skill->hits;
        }
        char hurt_rate_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(hurt_rate_str, cur, "hurt_rate", "");
        std::vector<std::string> hurt_rate_vec = split(hurt_rate_str, ',');

        if (skill.hits == 1) {
            skill.hurt_rate.push_back(100);
        } else if (skill.hits == parent_skill->hits) { //沿用父技能的hits
            if (hurt_rate_vec.size() == 0) {//使用父技能的伤害率
                skill.hurt_rate = parent_skill->hurt_rate;
            } else if (hurt_rate_vec.size() != skill.hits) {//使用自己的伤害率 但数量不对
                ERROR_TLOG("Skill %u hits != hurt_rate.size()", skill.skill_id);
                return -1;
            } else {
                FOREACH(hurt_rate_vec, it) {
                    int rate = atoi((*it).c_str());
                    skill.hurt_rate.push_back(rate);
                }
            }
        } else { //使用自己的hits
            if (hurt_rate_vec.size() != skill.hits) {//使用自己的伤害率 但数量不对
                ERROR_TLOG("Skill %u hits != hurt_rate.size()", skill.skill_id);
                return -1;
            } else {
                FOREACH(hurt_rate_vec, it) {
                    int rate = atoi((*it).c_str());
                    skill.hurt_rate.push_back(rate);
                }
            }
        }
        mgr.add_skill_conf(skill);
        cur = cur->next;
    }
    return 0;
}

int load_skill_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    skill_conf_manager_t tmp;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("skill"))) {
            cur = cur->next;
            continue;
        }
        skill_conf_t skill;
        DECODE_XML_PROP_UINT32(skill.skill_id, cur, "id");
        if (tmp.skill_conf_exist(skill.skill_id)) {
            ERROR_TLOG("Duplicate skill in skill.xml id=%u", skill.skill_id);
            ERR_RT;
        }
        DECODE_XML_PROP_UINT32_DEFAULT(skill.hits, cur, "hits", 1);
        if (skill.hits == 0) {
            skill.hits = 1;
        }
        char hurt_rate_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(hurt_rate_str, cur, "hurt_rate", "");
        std::vector<std::string> hurt_rate_vec = split(hurt_rate_str, ',');

        if (hurt_rate_vec.size() && skill.hits != hurt_rate_vec.size()) {
            ERROR_TLOG("Skill %u hits != hurt_rate.size()", skill.skill_id);
            ERR_RT;
        }
        if (skill.hits == 1) {
            skill.hurt_rate.push_back(100);
        } else if (hurt_rate_vec.size() == 0){
            uint32_t mod = 100 % skill.hits;
            uint32_t rate = 100 - mod;
            for (uint32_t i = 0; i < (skill.hits - 1); i++) {
                skill.hurt_rate.push_back(rate/skill.hits);
            }
            skill.hurt_rate.push_back(rate/skill.hits + mod);
        } else {
            FOREACH(hurt_rate_vec, it) {
                int rate = atoi((*it).c_str());
                skill.hurt_rate.push_back(rate);
            }
        }
        skill.is_parent = 1;
        tmp.add_skill_conf(skill);
        cur = cur->next;
    }
    int ret = 0;
    tmp.show();

    const char *file = 0;
    xmlDocPtr doc = 0;
    xmlNodePtr head = 0;

    file = gen_full_path(g_server_config.conf_path, "skill.xml");
    doc = xmlParseFile(file);
    if (!doc) {
        xmlErrorPtr xptr = xmlGetLastError();
        ERROR_TLOG("Failed to Load %s [line:%u msg:%s]", file, xptr->line, xptr->message);
        xmlFreeDoc(doc);
        ERR_RT;
    }
    head = xmlDocGetRootElement(doc); 
    if (!head) {
        ERROR_TLOG("xmlDocGetRootElement error when loading file '%s'", file);
        xmlFreeDoc(doc);
        ERR_RT;
    }
    ret = load_skill_level_config(head, tmp);
    xmlFreeDoc(doc);
    if (ret) {
        ERR_RT;
    }

    FOREACH_NOINCR_ITER((tmp.const_skill_conf_map()), it) {
        //校验技能的字段
        skill_conf_t *skill = tmp.find_mutable_skill_conf(it->second.skill_id);
        if (skill->learn_level > kMaxPetLevel) {
            ERROR_TLOG("Skill %u learn level too large %u", 
                    skill->skill_id, skill->learn_level);
            ERR_RT;
        }
        if (skill->hits != skill->hurt_rate.size()) {
            ERROR_TLOG("Skill %u hits[%u] != hurt_rate_size[%u]",
                    skill->skill_id, skill->hits, skill->hurt_rate.size());
            ERR_RT;
        }
        uint32_t rate = 0;
        FOREACH(skill->hurt_rate, it2) {
            rate += *it2;
        }
        /*
        if (rate != 100) {
            ERROR_TLOG("Skill %u hits hurt_rate not equal 100", skill->skill_id);
            ERR_RT;
        }
        */
        it++;
    }
    g_skill_conf_mgr.copy_from(tmp);
    g_skill_conf_mgr.show();
    return 0;
}

int load_builder_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    builder_conf_manager_t tmp;

    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)"builder")) {
            cur = cur->next;
            continue;
        }
        builder_conf_t builder_conf;

        DECODE_XML_PROP_UINT32(builder_conf.id, cur, "id");
        if (tmp.builder_conf_exist(builder_conf.id)) {
            ERROR_TLOG("builder %u already exists", builder_conf.id);
            ERR_RT;  
        }
        uint32_t type = 0;
        DECODE_XML_PROP_UINT32(type, cur, "elem_type");
        if (type > kMaxPetElemTypeValue) {
            ERROR_TLOG("builder %u elem_type not exist[%u]", builder_conf.id, type);
            ERR_RT;
        }
        builder_conf.elem_type = (pet_elem_type_t)type;
        DECODE_XML_PROP_UINT32(type, cur, "grow_type");
        if (type != kPetGrowType1 && type != kPetGrowType2) {
            ERROR_TLOG("builder %u grow_type not exist[%u]", builder_conf.id, type);
            ERR_RT;
        }
        DECODE_XML_PROP_UINT32(builder_conf.sex, cur, "sex");
        if (builder_conf.sex != 0 && builder_conf.sex != 1) {
            ERROR_TLOG("builder %u sex invalid", builder_conf.sex);
            ERR_RT;
        }

        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.level, cur, "level", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.mon_type, cur, "mon_type", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.req_power, cur, "req_power", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.max_dp, cur, "max_dp", 0);

        if (builder_conf.mon_type > MON_TYPE_BOSS) {
            ERROR_TLOG("builder %u mon_type %u invalid", builder_conf.id, builder_conf.mon_type);
            ERR_RT;
        }

        DECODE_XML_PROP_UINT32(builder_conf.basic_normal_battle_values[kBattleValueNormalTypeHp], cur, "hp");
        DECODE_XML_PROP_UINT32(builder_conf.basic_normal_battle_values[kBattleValueNormalTypeNormalAtk], cur, "normal_atk");
        DECODE_XML_PROP_UINT32(builder_conf.basic_normal_battle_values[kBattleValueNormalTypeNormalDef], cur, "normal_def");
        DECODE_XML_PROP_UINT32(builder_conf.basic_normal_battle_values[kBattleValueNormalTypeSkillAtk], cur, "skill_atk");
        DECODE_XML_PROP_UINT32(builder_conf.basic_normal_battle_values[kBattleValueNormalTypeSkillDef], cur, "skill_def");
        DECODE_XML_PROP_UINT32(builder_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeHp], cur, "hp_level_add");
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeNormalAtk], cur, "normal_atk_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeNormalDef], cur, "normal_def_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeSkillAtk], cur, "skill_atk_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeSkillDef], cur, "skill_def_level_add", 0);

        DECODE_XML_PROP_UINT32(builder_conf.basic_hide_battle_values[kBattleValueHideTypeCrit], cur, "crit");
        DECODE_XML_PROP_UINT32(builder_conf.basic_hide_battle_values[kBattleValueHideTypeAntiCrit], cur, "anti_crit");
        DECODE_XML_PROP_UINT32(builder_conf.basic_hide_battle_values[kBattleValueHideTypeDodge], cur, "dodge");
        DECODE_XML_PROP_UINT32(builder_conf.basic_hide_battle_values[kBattleValueHideTypeHit], cur, "hit");
        DECODE_XML_PROP_UINT32(builder_conf.basic_hide_battle_values[kBattleValueHideTypeBlock], cur, "block");
        DECODE_XML_PROP_UINT32(builder_conf.basic_hide_battle_values[kBattleValueHideTypeBreakBlock], cur, "break_block");
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_hide_battle_values[kBattleValueHideTypeCritAffectRate], cur, "crit_affect_rate", 100);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_hide_battle_values[kBattleValueHideTypeBlockAffectRate], cur, "block_affect_rate", 50);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_hide_battle_values_grow[kBattleValueHideTypeCrit], cur, "crit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_hide_battle_values_grow[kBattleValueHideTypeAntiCrit], cur, "anti_crit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_hide_battle_values_grow[kBattleValueHideTypeDodge], cur, "dodge_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_hide_battle_values_grow[kBattleValueHideTypeHit], cur, "hit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_hide_battle_values_grow[kBattleValueHideTypeBlock], cur, "block_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_hide_battle_values_grow[kBattleValueHideTypeBreakBlock], cur, "break_block_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_hide_battle_values_grow[kBattleValueHideTypeCritAffectRate], cur, "crit_affect_rate_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.basic_hide_battle_values_grow[kBattleValueHideTypeBlockAffectRate], cur, "block_affect_rate_level_add", 0);

        DECODE_XML_PROP_UINT32(builder_conf.basic_hide_battle_values[kBattleValueHideTypeAtkSpeed], cur, "atk_speed");
        DECODE_XML_PROP_UINT32_DEFAULT(builder_conf.is_level_add, cur, "is_level_add", 1);

        char skill_str[256] = {0};
        //DECODE_XML_PROP_STR(skill_str, cur, "skill_ids");
        DECODE_XML_PROP_STR_DEFAULT(skill_str, cur, "skill_ids", "");
        std::vector<string> skill_vec = split(skill_str, ',');
        FOREACH(skill_vec, it) {
            uint32_t skill = atoi((*it).c_str());
            if (!g_skill_conf_mgr.skill_conf_exist(skill)) {
                ERROR_TLOG("Builder %u skill_id:%u not exist",
                        builder_conf.id, skill);
                ERR_RT;
            }
            builder_conf.skill_ids.push_back(skill);
        }

        TRACE_TLOG("builder #%u: elem_type = %u",
                builder_conf.id, builder_conf.elem_type);

        for (int i = 0; i < kMaxBattleValueTypeNum; i++) {
            TRACE_TLOG("builder #%u, battle type %u, value = %u",
                    builder_conf.id, i, builder_conf.basic_normal_battle_values[i]); 
        }

        tmp.add_builder_conf(builder_conf);
        cur = cur->next;
    }
    g_builder_conf_mgr.copy_from(tmp);
    return 0;
}

int load_pet_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    pet_conf_manager_t tmp;

    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)"pet")) {
            cur = cur->next;
            continue;
        }
        pet_conf_t pet_conf;

        DECODE_XML_PROP_UINT32(pet_conf.id, cur, "id");
        if (tmp.pet_conf_exist(pet_conf.id)) {
            ERROR_TLOG("pet %u already exists", pet_conf.id);
            ERR_RT;  
        }
        uint32_t type = 0;
        DECODE_XML_PROP_UINT32(type, cur, "elem_type");
        if (type > kMaxPetElemTypeValue) {
            ERROR_TLOG("pet %u elem_type not exist[%u]", pet_conf.id, type);
            ERR_RT;
        }
        pet_conf.elem_type = (pet_elem_type_t)type;
        /*
        DECODE_XML_PROP_UINT32(type, cur, "grow_type");
        if (type != kPetGrowType1 && type != kPetGrowType2) {
            ERROR_TLOG("pet %u grow_type not exist[%u]", pet_conf.id, type);
            ERR_RT;
        }
        */
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.sex, cur, "sex", 0);
        if (pet_conf.sex != 0 && pet_conf.sex != 1) {
            ERROR_TLOG("pet %u sex invalid", pet_conf.sex);
            ERR_RT;
        }

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.level, cur, "level", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.req_power, cur, "req_power", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.max_dp, cur, "max_dp", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.mon_type, cur, "mon_type", 0);
        if (pet_conf.mon_type > MON_TYPE_BOSS) {
            ERROR_TLOG("pet %u mon_type %u invalid", pet_conf.id, pet_conf.mon_type);
            ERR_RT;
        }

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values[kBattleValueNormalTypeHp], cur, "hp", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values[kBattleValueNormalTypeNormalAtk], cur, "normal_atk", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values[kBattleValueNormalTypeNormalDef], cur, "normal_def", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values[kBattleValueNormalTypeSkillAtk], cur, "skill_atk", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values[kBattleValueNormalTypeSkillDef], cur, "skill_def", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeHp], cur, "hp_level_add", 1);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeNormalAtk], cur, "normal_atk_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeNormalDef], cur, "normal_def_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeSkillAtk], cur, "skill_atk_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_normal_battle_values_grow[kBattleValueNormalTypeSkillDef], cur, "skill_def_level_add", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeCrit], cur, "crit", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeAntiCrit], cur, "anti_crit", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeDodge], cur, "dodge", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeHit], cur, "hit", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeBlock], cur, "block", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeBreakBlock], cur, "break_block", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeCritAffectRate], cur, "crit_affect_rate", 100);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeBlockAffectRate], cur, "block_affect_rate", 50);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeCrit], cur, "crit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeAntiCrit], cur, "anti_crit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeDodge], cur, "dodge_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeHit], cur, "hit_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeBlock], cur, "block_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeBreakBlock], cur, "break_block_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeCritAffectRate], cur, "crit_affect_rate_level_add", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values_grow[kBattleValueHideTypeBlockAffectRate], cur, "block_affect_rate_level_add", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.basic_hide_battle_values[kBattleValueHideTypeAtkSpeed], cur, "atk_speed", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.extra_battle_values[kBattleValueNormalTypeHp], cur, "hp_n", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.extra_battle_values[kBattleValueNormalTypeNormalAtk], cur, "normal_atk_n", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.extra_battle_values[kBattleValueNormalTypeNormalDef], cur, "normal_def_n", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.extra_battle_values[kBattleValueNormalTypeSkillAtk], cur, "skill_atk_n", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.extra_battle_values[kBattleValueNormalTypeSkillDef], cur, "skill_def_n", 0);

        DECODE_XML_PROP_UINT32_DEFAULT(pet_conf.is_level_add, cur, "is_level_add", 1);

        char skill_str[256] = {0};
        //DECODE_XML_PROP_STR(skill_str, cur, "skill_ids");
        DECODE_XML_PROP_STR_DEFAULT(skill_str, cur, "skill_ids", "");
        std::vector<string> skill_vec = split(skill_str, ',');
        FOREACH(skill_vec, it) {
            uint32_t skill = atoi((*it).c_str());
            if (!g_skill_conf_mgr.skill_conf_exist(skill)) {
                ERROR_TLOG("Pet %u skill_id:%u not exist",
                        pet_conf.id, skill);
                ERR_RT;
            }
            pet_conf.skill_ids.push_back(skill);
        }

        std::vector<uint32_t> affix_id_pool;
        char affix_str[1000] = {0};
        DECODE_XML_PROP_STR_DEFAULT(affix_str, cur, "affix_list", "");
        std::vector<string> affix_vec = split(affix_str, ';');
        FOREACH(affix_vec, it) {
            affix_id_pool.clear();
            std::vector<string> affix_item_vec = split(*it, ',');
            FOREACH(affix_item_vec, iter) {
                uint32_t affix_id = atoi((*iter).c_str());
                if (!g_affix_conf_mgr.affix_conf_exist(affix_id) && 
                        !g_affix_conf_mgr.is_affix_type(affix_id) && affix_id > 0) {
                    ERROR_TLOG("affix config not exist, id:%u", affix_id);
                    return -1;
                }

                affix_id_pool.push_back(affix_id);
            }

            if (affix_id_pool.size() > 0) {
                pet_conf.affix_list_pool.push_back(affix_id_pool);
            }
        }

        TRACE_TLOG("pet #%u: elem_type = %u",
                pet_conf.id, pet_conf.elem_type);

        for (int i = 0; i < kMaxBattleValueTypeNum; i++) {
            TRACE_TLOG("pet #%u, battle type %u, value = %u",
                    pet_conf.id, i, pet_conf.basic_normal_battle_values[i]); 
        }

        tmp.add_pet_conf(pet_conf);
        cur = cur->next;
    }
    g_pet_conf_mgr.copy_from(tmp);
    return 0;
}

int load_affix_config(xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;

    affix_conf_manager_t tmp;
    affix_conf_t affix_conf;

    while (cur) { 
        if (!xmlStrEqual(cur->name, (const xmlChar*)("affix"))) {
            cur = cur->next;
            continue;
        }

        DECODE_XML_PROP_UINT32(affix_conf.affix_id, cur, "id");
        DECODE_XML_PROP_UINT32_DEFAULT(affix_conf.type, cur, "type", 0);
        DECODE_XML_PROP_UINT32_DEFAULT(affix_conf.buff_id, cur, "buff_id", 0);

        if (tmp.affix_conf_exist(affix_conf.affix_id)) {
            ERROR_TLOG("repeated affix config, id:%u", affix_conf.affix_id);
            return -1;
        }
        tmp.add_affix_conf(affix_conf);
        cur = cur->next;
    }
    tmp.init_type_map();
    g_affix_conf_mgr.copy_from(tmp);
    return 0;
}

int load_duplicate_script(duplicate_t &dup, xmlNodePtr root)
{
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar*)("action"))) {
            cur = cur->next;
            continue;
        }
        //加载action
        char action_type[256] = {0};
        DECODE_XML_PROP_STR(action_type, cur, "type");
        uint32_t client_only;
        DECODE_XML_PROP_UINT32_DEFAULT(client_only, cur, "client_only", 0);
        if (client_only) {//仅客户端使用的action
            cur = cur->next;
            continue;
        }
        DupActionBase *action = create_action_by_name(string(action_type));
        if (!action) {
            ERROR_TLOG("Unknown dup[%u] action :%s", dup.duplicate_id, action_type);
            return -1;
        }
        int ret = action->load_action_info(dup, cur);
        if (ret) {
            delete action;
            return -1;
        }
        dup.action_vec.push_back(action);
    
        cur = cur->next;
    }
    return 0;
}

int load_duplicate_config(xmlNodePtr root)
{
    g_born_area_idx = 0;
    duplicate_conf_manager_t tmp;
    int ret = 0;
    xmlNodePtr cur = root->xmlChildrenNode;
    while (cur) {
        if (!xmlStrEqual(cur->name, (const xmlChar *)("duplicate"))) {
            cur = cur->next;
            continue;
        }
        duplicate_t dup;
        DECODE_XML_PROP_UINT32(dup.duplicate_id, cur, "id");
        if (tmp.duplicate_exist(dup.duplicate_id)) {
            ERROR_TLOG("Duplicate duplicate in duplicate.xml id=%u", dup.duplicate_id);
            ERR_RT;
        }

        uint32_t type = 0;
        decode_xml_prop_uint32_default(&type, cur, "battle_type", 1);
        if (type <= DUP_BTL_TYPE_ERR || type >= DUP_BTL_TYPE_END) {
            ERROR_TLOG("Duplicate btl_type invalid dup:%u, btl_type:%u",
                    dup.duplicate_id, type);
            ERR_RT;
        }
        dup.battle_type = (duplicate_battle_type_t)type;
        decode_xml_prop_uint32_default(&(dup.time_limit), cur, "time_limit", 300);
        decode_xml_prop_uint32_default(&(dup.boss_show_time_limit), cur, "boss_time_limit", 0);
        decode_xml_prop_uint32_default(&(dup.mode), cur, "mode", onlineproto::DUP_MODE_TYPE_NORMAL);
        decode_xml_prop_uint32_default(&(dup.req_power), cur, "recommended_bv", 0);
        decode_xml_prop_uint32_default(&(dup.add_fight_level), cur, "add_fight_level", 0);

        char dup_monster_id_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(dup_monster_id_str, cur, "mon_ids", "");
        std::vector<std::string> mon_list = split(dup_monster_id_str, ',');
        FOREACH(mon_list, it) {
            uint32_t pet_id = atoi((*it).c_str());
            if (!g_pet_conf_mgr.pet_conf_exist(pet_id)) {
                ERROR_TLOG("Duplicate monster id:%u for duplicate:%u not exist", 
                        pet_id, dup.duplicate_id);
                ERR_RT; 
            }
            dup.mon_vec.push_back(pet_id);
        }

        char dup_builder_id_str[256] = {0};
        DECODE_XML_PROP_STR_DEFAULT(dup_builder_id_str, cur, "builder_ids", "");
        std::vector<std::string> builder_list = split(dup_builder_id_str, ',');
        FOREACH(builder_list, it) {
            uint32_t builder_id = atoi((*it).c_str());
            if (!g_builder_conf_mgr.builder_conf_exist(builder_id)) {
                ERROR_TLOG("Duplicate builder id:%u for duplicate:%u not exist", 
                        builder_id, dup.duplicate_id);
                ERR_RT; 
            }
            dup.builder_vec.push_back(builder_id);
        }

        uint32_t script_id = 0;
        DECODE_XML_PROP_UINT32(script_id, cur, "script_id");
        if (script_id == 0) {
            ERROR_TLOG("Duplicate %u's script_id is 0", dup.duplicate_id);
            ERR_RT;
        }
        //读取副本脚本
        char tmp_str[1024];
        snprintf(tmp_str, sizeof(tmp_str), "%s/duplicate/dup_%u.xml", g_server_config.conf_path, script_id);
        xmlDocPtr doc = xmlParseFile(tmp_str);
        if (!doc) {
            xmlErrorPtr xptr = xmlGetLastError();
            ERROR_TLOG("Failed to Load %s [line:%u msg:%s]", tmp_str, xptr->line, xptr->message);
            xmlFreeDoc(doc);
            ERR_RT;
        }
        xmlNodePtr head = xmlDocGetRootElement(doc);
        if (!head) {
            ERROR_TLOG("xmlDocGetRootElement error when loading file '%s'", tmp_str);
            xmlFreeDoc(doc);
            ERR_RT;
        }
        ret = load_duplicate_script(dup, head);
        xmlFreeDoc(doc);
        if (ret) {
            ERR_RT;
        }

        tmp.add_duplicate(dup);
        cur = cur->next;
    }

    g_duplicate_conf_mgr.copy_from(tmp);
    
    FOREACH(g_duplicate_conf_mgr.const_dup_map(), it) {
        show_dup(it->first);
    }
    return 0;
}


void show_dup(uint32_t dup_id)
{
    static int i = 0;
    i++;

    const duplicate_t *dup = g_duplicate_conf_mgr.find_duplicate(dup_id);
    if (!dup) {
        return;
    }

    TRACE_TLOG("Dup:%u btl_type:%u time_limit:%u",
            dup->duplicate_id, dup->battle_type, dup->time_limit);
    FOREACH(dup->action_vec, it2) {
        DupActionBase* action = *it2;
        TRACE_TLOG("\t action type:%s", get_action_name(action->get_type()));
        FOREACH(action->args, it3) {
            TRACE_TLOG("\t\t action args:%u", *it3);
        }
        FOREACH(action->cond_group, it3) {
            FOREACH((*it3), it4) {
                DupCondBase *cond = *it4;
                TRACE_TLOG("\t\t cond type:%s", get_cond_name(cond->get_type()));
                FOREACH(cond->args, it5) {
                    TRACE_TLOG("\t\t\t cond args:%u", *it5);
                }
            }
            if (it3+1 != action->cond_group.end()) {
                TRACE_TLOG("\t\t---------");
            }
        }
        if (action->get_type() == dup_action_type_actor_born) {
            DupActionActorBorn *tmp = (DupActionActorBorn*)action;
            FOREACH(tmp->born_areas, it3) {
                const duplicate_born_area_t &area = it3->second;
                TRACE_TLOG("\t\t action born_area(idx:%u) flush_method:%u flush_max:%u x:%u y:%u r:%u",
                        area.g_area_idx, area.flush_method, area.flush_max, area.born_x, area.born_y, area.born_radius);
                FOREACH(area.born_actors, it4) {
                    const duplicate_actor_t &actor = *it4;
                    TRACE_TLOG("\t\t\t action actor id:%u type:%u team:%u", actor.id, actor.type, actor.team);
                }
            }
        }
    }
    FOREACH(dup->cond_map, it) {
        TRACE_TLOG("Cond: %s", get_cond_name(it->first));
        FOREACH((it->second), it2) {
            DupCondBase *cond = *it2;
            TRACE_TLOG("\t--------------%p", cond);

            FOREACH(cond->args, it3) {
                TRACE_TLOG("\targs:%u", *it3);
            }
            FOREACH(cond->listen_action, it3) {
                DupActionBase *action = *it3;
                TRACE_TLOG("\tlisten action :%s", get_action_name(action->get_type()));
                FOREACH(action->args, it4) {
                    TRACE_TLOG("\t\t action_args:%u", *it4);
                }
            }
        }
    }
}
