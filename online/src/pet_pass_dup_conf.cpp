#include "pet_pass_dup_conf.h"

bool order_by_dup_id_asc(dup_shop_id_t dup_shopid_1, dup_shop_id_t dup_shopid_2) {
	return dup_shopid_1.dup_id < dup_shopid_2.dup_id;
}

bool order_by_dup_id_des(dup_shop_id_t dup_shopid_1, dup_shop_id_t dup_shopid_2) {
	return dup_shopid_1.dup_id > dup_shopid_2.dup_id;
}

