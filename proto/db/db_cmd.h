
#ifndef DB_CMD_H
#define DB_CMD_H

enum db_cmd_t
{
    //db号段      0x8200 - 0x8AFF
    //////////////////////////////////// dplan_db_00 ~ 99 号段 ////////////////////////////////
    /* 基础信息命令号段 0x8200 ~ 0x83FF */
    db_cmd_check_user_exist = 0x8200,
    db_cmd_create_role = 0x8201,
    db_cmd_get_base_info = 0x8202,
    db_cmd_save_base_info = 0x8203,
    db_cmd_get_login_info = 0x8204,
    db_cmd_set_attr = 0x8205,
    db_cmd_get_attr = 0x8206,
    db_cmd_get_ranged_attr = 0x8207,
    db_cmd_clear_ranged_attr = 0x8208,
    db_cmd_change_attr_value  = 0x8209,
    db_cmd_del_attr = 0x820A,
    db_cmd_change_items = 0x820B,
    db_cmd_change_nick = 0x820C,
    db_cmd_pet_save = 0x820D,
    db_cmd_pet_delete = 0x820E,
    db_cmd_get_cache_info = 0x820F,		//拉取用户缓存信息协议
	db_cmd_pet_list_get = 0x8210,	//拉取精灵列表

	db_cmd_task_del = 0x8211,
	db_cmd_task_save = 0x8212,
	
	db_cmd_save_rune = 0x8213,
	db_cmd_del_rune = 0x8214,

	db_cmd_mail_new = 0x8216, //产生新邮件
	db_cmd_mail_get_all = 0x8215, //得到所有邮件信息
	db_cmd_mail_del_by_ids = 0x8217, //根据id删除邮件
	db_cmd_mail_get_by_ids = 0x8218, //根据id得到邮件详情
    db_cmd_mail_set_status = 0x8219, //设置邮件状态

	db_cmd_check_user_exist_by_id = 0x821D, //检查用户是否存在
	db_cmd_save_friend = 0x821E,
	db_cmd_remove_friend = 0x821F,

	// 小屋协议 0x8221-0x8230
	db_cmd_get_home_info = 0x8221,      //拉取小屋信息
	db_cmd_get_hm_visit_log	= 0x8222,	//拉取访客日志
	db_cmd_hm_add_visit_log = 0x8223,
	db_cmd_update_gift_count = 0x8224,	//小屋送礼：更新给好友送礼的次数

	db_cmd_user_raw_data_get = 0x8231,	//获取玩家raw_data信息
	db_cmd_user_raw_data_update = 0x8232,	//更新玩家的raw_data
	db_cmd_user_raw_data_del = 0x8233,	//删除玩家的raw_data

    db_cmd_show_item = 0x8234, /* 展示物品 */
    db_cmd_show_pet = 0x8235, /* 展示精灵 */

	//成就相关
	db_cmd_save_achieves = 0x8236,	/* 保存成就信息*/

    // 家族协议0x82A0-0x82FF
    db_cmd_family_get_info = 0x082A0,
    db_cmd_family_update_info = 0x082A1,
    db_cmd_family_get_member_info = 0x082A2,
    db_cmd_family_update_member_info = 0x082A3,
    db_cmd_family_quit = 0x082A4,
    db_cmd_family_dismiss_family = 0x082A5,
    db_cmd_family_change_info = 0x082A6,
    db_cmd_family_get_member_list = 0x082A7,
    db_cmd_family_get_event_info = 0x082A8,
    db_cmd_family_del_event = 0x082A9,
    db_cmd_family_update_event = 0x082AA,
    db_cmd_family_get_event_list = 0x082AB,
    db_cmd_family_update_log = 0x082AC,
    db_cmd_family_get_log_list = 0x082AD,
    db_cmd_family_change_member_info = 0x082AE,
    db_cmd_family_get_next_leader = 0x082AF,
    db_cmd_family_get_recommend_list = 0x082B0,


    /* 0x8400 ~ 0x85FF nick库 action_log库*/
    /* 0x8400 ~ 0x85FF nick库 action_log库 transaction库*/
    db_cmd_get_userid_by_nick = 0x8400,
    db_cmd_insert_nick_and_userid = 0x8401,
    db_cmd_insert_user_action_log = 0x8402,
    db_cmd_get_user_action_log= 0x8403,
    db_cmd_new_transaction = 0x8404, /* 产生一条交易记录 */
    db_cmd_get_transaction_list = 0x8405, /* 拉取用户交易明细 */
	db_cmd_get_buy_pd_trans_list = 0x8406,	/*拉取用户钻石商品的交易明细（供客服使用）*/
    db_cmd_get_login_tm_info = 0x8407, /*拉取用户某时间段内的登录信息*/

    // 家族协议
    db_cmd_family_create = 0x8408,  //创建家族
    db_cmd_family_update_match_info = 0x08409, // 更新家族推荐库
    db_cmd_family_change_match_info = 0x0840A, // 增量更新家族推荐库(成员数)
    db_cmd_family_del_match_info = 0x0840B, // 删除家族推荐库记录

    // nick
    db_cmd_delete_nick_and_userid = 0x840C, //删除昵称

	//全服属性 
	db_cmd_get_global_attr = 0x840D, /* 获取全副属性*/
	db_cmd_update_global_attr = 0x840E, /* 更新全服属性 */


    //客户端软件版本
    db_cmd_set_clisoftv = 0x840F, /*设置玩家的客户端软件版本*/


    //兑换码
    db_cmd_use_gift_code = 0X8410, /*尝试使用兑换码*/

	db_cmd_new_vip_op_trans = 0x8411,	//产生一条vip购买操作
	db_cmd_new_vip_user_info = 0x8412,	//产生一条vip信息记录

	db_cmd_search_occupyed_mine = 0x8413,	//搜索指定服中被占领的矿山 
	db_cmd_save_one_new_mine = 0x8414,	//保存一个新矿信息
	db_cmd_get_player_mine_info = 0x8415,	//获取我的矿场信息
	db_cmd_update_mine_info = 0x8416,	//更新矿的信息（主要是更换矿主战力信息等）
	db_cmd_del_mine_info = 0x8417,	//删除一个矿的信息
	db_cmd_increment_defender_cnt = 0x8418,	//尝试协防该矿，使得该矿人数增一

};


#endif
