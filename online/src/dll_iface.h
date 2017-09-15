
#ifndef SERVER_H
#define SERVER_H

#ifndef OL_REVISION
#define OL_REVISION 0
#endif

extern "C" {
#include <libtaomee/log.h>
}
#include <string>

struct server_config_t
{
    uint32_t gameid; 
    uint32_t verifyid;
    char security_code[10];
    uint32_t idc_zone;
    char dbproxy_name[256];
    char switch_name[256];
    char battle_name[256];
    char battle_center_name[256];
	char rank_name[256];
    char conf_path[256];
    char statistic_file[256];
    char wonderful_word_addr[256];
    uint32_t use_gm; // 是否使用gm命令
    uint32_t shut_addiction;
    uint32_t donot_use_wonderful;
    char seer2_dx_addr[256];
    char seer2_wt_addr[256];
    char seer_addr[256];
	uint32_t version_department;//大于0表示是版署服
};

std::string stack_trace();
const char *gen_full_path(const char *base_path, const char *file_name);

#endif
