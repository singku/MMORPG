
#ifndef DB_ERRNO_H
#define DB_ERRNO_H

enum db_errno_t 
{
    db_err_succ = 0, // 操作成功
    db_err_record_not_found = 20000, //查询的记录集没有找到
    db_err_proto_format_err = 20001, // 包体格式错误
    db_err_cmd_not_find = 20002, // 命令号不存在
    db_err_role_already_exist = 20003, // 角色已经存在
    db_err_user_not_find = 20004, // 米米号不存在
    db_err_gen_sql_from_proto_failed = 20005, // 生成sql错误
    db_err_user_not_own_pet= 20006, //用户无此精灵

    db_err_attr_del = 20008,//attr表删除操作失败
    db_err_attr_set = 20009,//attr表保存操作失败
    db_err_attr_get = 20010, //无法从attr表中得到数据
    db_err_user_not_own_rune= 20011, //用户无此符文
    db_err_family_not_find = 20012, // 家族不存在
    db_err_sys_err = 20013, // 系统错误
    db_err_invalid_replay_key = 20014, // 无效的replay key
    db_err_friend_add_attention = 20015, //friend表加关注操作失败
    db_err_friend_set = 20016, //修改好友状态失败
    db_err_friend_remove_attention = 20017, //friend表取消关注失败
    db_err_friend_set_recent_list = 20018, //修改最近联系人列表失败
    db_err_family_already_exist = 20020, // 家族已经存在
    db_err_friend_reduplicate_add = 20021,//重复添加好友
    db_err_friend_reduplicate_remove_attention = 20022, //取消不存在之好友
    db_err_friend_reduplicate_set_black = 20023,//重复加屏蔽
    db_err_friend_reduplicate_remove_black = 20024, //重复取消屏蔽
    db_err_nick_not_exist = 20025, //nick不存在于nick-table中
    db_err_nick_already_exist = 20026, //nick已经存在
    db_err_money_lack = 20027, //钱不够
    db_err_attr_init = 20028, //创建用户时初始化attr错误
	db_err_rune_save = 20029, //符文更新失败
    db_err_friend_del = 20030, //修改好友状态失败

    db_err_escort_rob = 20034, //用户被拦截次数已满
    db_err_global_attr_not_enough = 20035, // 全服属性不足
	db_err_update_gift_count = 20036,	//更新好友礼物数量失败
	db_err_must_give_pd_id = 20037,	//必须指定商品id
	db_err_save_achieve_err = 20038,	//保存成就失败
	db_err_save_mine_id = 20039,		//保存矿场信息失败
	db_err_delete_mine = 20040,		//删除矿场失败
	db_err_exceed_mine_def_cnt_limit = 20041,	//超过该矿所能承载的最大采矿人数
};

#endif
