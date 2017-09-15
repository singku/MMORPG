#include "item_conf.h"
#include "attr_utils.h"
#include "player.h"

int item_conf_mgr_t::can_equip_cultivate_item(player_t *player, uint32_t item_id) {
    int ret = 0;
    if (!player) {
        return cli_err_data_error;
    }

    const item_conf_t *item_conf = find_item_conf(item_id);
    if (!item_conf) {
        return cli_err_item_not_exist;
    }

    if (item_conf->equip_body_pos == EQUIP_BODY_POS_MOUNT) {
        if (item_conf && item_conf->cult_level_limit > GET_A(kAttrMountLevel)) {
            return cli_err_mount_level_too_low;
        }
    }

    if (item_conf->equip_body_pos == EQUIP_BODY_POS_WING) {
        if (item_conf && item_conf->cult_level_limit > GET_A(kAttrWingLevel)) {
            return cli_err_wing_level_too_low;
        }
    }

    return ret;
}

