#ifndef XML_CONFIGS_H
#define XML_CONFIGS_H

#include "xmlutils.h"
#include "global_data.h"

int load_map_config(xmlNodePtr root);
int load_skill_config(xmlNodePtr root);
int load_affix_config(xmlNodePtr root);
int load_pet_config(xmlNodePtr root);
int load_builder_config(xmlNodePtr root);
int load_duplicate_config(xmlNodePtr root);
void show_dup(uint32_t dup_id);

#endif
