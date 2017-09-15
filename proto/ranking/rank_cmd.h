#ifndef __rank_cmd_
#define __rank_cmd_

const uint32_t cli_cmd_base = 0x9200;
enum ranking_cmd_t 
{
    ranking_cmd_get_users_rank = 0x9201, //u 获取排名（若排名不存在，根据所传的参数，来判定是否创建排名）
										//可以获取1个指定的排行榜的多个玩家的对应的排名
    ranking_cmd_get_ranking_users = 0x9202,	 //u
    ranking_cmd_switch_arena_ranking_user = 0x9203, //u
    ranking_cmd_get_ranking_list = 0x9204,	//u
    ranking_cmd_ranking_insert_score = 0x9205,//u
	
	ranking_cmd_save_battle_report = 0x9206,	//保存战报(不仅仅适用于竞技场)  u
	ranking_cmd_get_battle_report = 0x9207,	//获取战报 u
	ranking_cmd_get_battle_report_key = 0x9208,	//获取近期的N份战报的key

    ranking_cmd_hset_insert_or_update = 0x9209,  //新增hash表或更新hash表域值
    ranking_cmd_hset_get_info = 0x920a,  // 查询hash表所有域值
    ranking_cmd_set_insert_member = 0x920b,  // 向Set中增加成员
    ranking_cmd_set_del_member = 0x920c,  // 删除Set中的成员
    ranking_cmd_set_get_all_member = 0x920d,  // 获取Set所有成员
    ranking_cmd_redis_del_key = 0x920f,  // 删除redis中指定key集合
    ranking_cmd_set_is_member = 0x920e,  // 判断是否set成员
    ranking_cmd_string_insert = 0x9210,  //新增string值,如果已存在则不修改
    ranking_cmd_hset_insert = 0x9211,  //新增hash表值,如果已存在则不修改
    ranking_cmd_redis_del_key_str = 0x9212,  // 删除redis中指定key集合
    ranking_cmd_hset_update_family_hall_info = 0x9213, // 更新家族大厅在线信息
    ranking_cmd_hset_get_family_hall_info = 0x9214, // 拉取家族大厅在线信息
    ranking_cmd_clear_family_info = 0x9215, // 清空家族信息
    ranking_cmd_hset_get_field_info = 0x9218,  // 查询hash表指定域值
	ranking_cmd_get_last_n_week_rank = 0x921A,	//获取排行榜前n周的周排名
	ranking_cmd_get_user_multi_rank = 0x921B,	//获取玩家若干sub_key排名
	ranking_cmd_rank_insert_last = 0x921C,	//插入到最后一名
    ranking_cmd_del_user_rank   = 0x921D,   // 删除用户排名记录
	ranking_cmd_get_users_by_score = 0x921E,   //根据分数范围获取玩家id

	ranking_cmd_dump_rank = 0x921F,		//dump排名服

	//Confirm kevin测试用 非业务命令号
	ranking_cmd_test_for_zjun = 0x9220, //测试

	ranking_cmd_list_lpush_member = 0x9221,   //list头部插入数据
	ranking_cmd_list_get_range_member = 0x9222,   //根据范围获取list数据

	ranking_cmd_hset_get_all_field_info = 0x9223,//查询hash表指定的key下面所有field信息
	ranking_cmd_hset_insert_bytes = 0x9224,	//新增hash表值,pbbuf中使用bytes

	ranking_cmd_get_hash_len = 0x9225,	//获得哈希表 key 中域的数量
	ranking_cmd_save_common_btl_report = 0x9226,	//保存通用的战报
	ranking_cmd_get_common_btl_report = 0x9227,
};

#endif
