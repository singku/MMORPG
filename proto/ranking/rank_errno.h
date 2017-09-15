#ifndef RANK_ERRNO_H
#define RANK_ERRNO_H

enum rank_err_t {
    rank_err_invalid_cmd             = 60001,
    rank_err_invalid_proto_len       = 60002,
    rank_err_redis_not_available     = 60003,
    rank_err_user_not_in_ranking     = 60004,
    rank_err_too_many_result         = 60005,
    rank_err_spt_replay_no_find      = 60006,
    rank_err_sys_err                 = 60007,
    rank_err_invalid_userid          = 60008,
	rank_err_parse_err				 = 60009,
    rank_err_empty_result            = 60010,
	rank_err_week_sub_key			 = 60011,
	rank_err_arena_week_rank_not_exsit = 60012,
	rank_err_get_rank_err			 = 60013,//拉取个人排名时出错
	rank_err_get_rank_count_err		 = 60014, //拉取排名总数时出错
	rank_err_get_rank_count_str_err		 = 60015, //拉取排名总数时出错
	rank_err_redis_key_not_found	 = 60016,	//redis key没有		
	rank_err_lexical_cast_err		 = 60017,	//转换字符串出错
	rank_err_get_redis_socre_err	 = 60018,  //获得排行榜分数时错误
	rank_err_rank_not_found	 		 = 60019,	//排名获取错误
	rank_err_btl_rep_get_err		 = 60020,  //战报获取错误
	rank_err_get_btl_key_err		 = 60021,  //获取战报key错误
	rank_err_save_btl_pkg_err		 = 60022,	//保存战报时，设置过期时间失败
	rank_err_last_n_week_sub_key	 = 60023,	//拉取近N周排行出错
	rank_err_last_n_week_rank_not_exsit = 60024,
	rank_err_insert_rank_last		 = 60025,	//插入最后一行排名失败
	rank_err_exped_match_failed		 = 60026,	//远征根据精灵战力匹配对手失败
	rank_err_hash_field_not_exist = 60027,	//hash给定的key ,对应的field不存在
};

#endif
