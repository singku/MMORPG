#include "mcast_utils.h"
#include "global_data.h"
#include "xml_configs.h"
#include "dll_iface.h"

uint32_t McastUtils::reload_configs(const std::string& conf_name)
{
    int ret = 0;
    if (conf_name.compare("map.xml") == 0) {
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "map.xml"), load_map_config);
    } else if (conf_name.compare("skill_parent.xml") == 0) { 
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "skill_parent.xml"), load_skill_config);
    } else if (conf_name.compare("affix.xml") == 0) {
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "affix.xml"), load_affix_config);
    } else if (conf_name.compare("pet.xml") == 0) {
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "pet.xml"), load_pet_config);
    } else if (conf_name.compare("builder.xml") == 0) {
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "builder.xml"), load_builder_config);
    } else if (conf_name.compare("duplicate.xml") == 0) {
        ret = load_xmlconf(gen_full_path(g_server_config.conf_path, "duplicate.xml"), load_duplicate_config);
    }
    return ret;
}
