#include "title.h"
#include "data_proto_utils.h"
#include "player_utils.h"
#include "title_conf.h"
#include "map_utils.h"
#include "rank_utils.h"
#include "utils.h"
#include "player_utils.h"
#include "prize.h"
#include <boost/lexical_cast.hpp>

uint32_t Title::insert_one_title_to_memory(title_info_t& title_info)
{
	/*
	if (title_map_.count(title_info.title_id)) {
		return cli_err_has_exist_this_title;
	}
	title_map_.insert(std::pair<uint32_t, title_info_t>(title_info.title_id, title_info));
	*/
	title_map_[title_info.title_id] = title_info;
	return 0;
}

uint32_t Title::add_one_title(player_t* player, title_info_t& title_info, bool sync_cli)
{
	TRACE_TLOG("zjun0513,add_one_title,title_info,id=[%u],get_time=[%u]", title_info.title_id, title_info.get_time);
	uint32_t ret = insert_one_title_to_memory(title_info);
	if (ret) {
		return ret;
	}
	ret = sync_all_titles(player, false);
	if (ret) {
		return ret;
	}
	if (sync_cli) {
		onlineproto::sc_0x0618_notify_get_title notify_msg;
		notify_msg.mutable_title_info()->set_title_id(title_info.title_id);
		notify_msg.mutable_title_info()->set_get_time(title_info.get_time);
		send_msg_to_player(player, cli_cmd_cs_0x0618_notify_get_title, notify_msg);
	}
	return 0;	
}

uint32_t Title::get_title_info(uint32_t title_id, title_info_t& title_info)
{
	if (title_map_.count(title_id) == 0) {
		return cli_err_not_exist_this_title;
	}
	title_info = title_map_.find(title_id)->second;
	return 0;
}

uint32_t Title::pack_all_titles(commonproto::title_info_list_t* titles_ptr)
{
	FOREACH(title_map_, it) {
		commonproto::title_info_t* ptr = titles_ptr->add_title_info();
		ptr->set_title_id(it->second.title_id);
		ptr->set_get_time(it->second.get_time);
	}
	return 0;
}

uint32_t Title::erase_expire_titles(player_t* player)
{
	bool erase_flag = false;
	FOREACH_NOINCR_ITER(title_map_, it) {
		if (this->check_title_expire(it->second.title_id)) {
			TRACE_TLOG("Erase Expire Title:id=%u,get_time=%u",
					it->second.title_id, it->second.get_time);
			erase_flag = true;
			title_map_.erase(it++);
		} else {
			++it;
		}
	}
	if (erase_flag) {
		this->sync_all_titles(player, false);
	}
	return 0;
}

uint32_t Title::sync_all_titles(player_t* player, bool sync_cli)
{
	commonproto::title_info_list_t title_info;
	DataProtoUtils::pack_all_titles_info(player, &title_info);
	uint32_t ret = PlayerUtils::update_user_raw_data(player->userid, player->create_tm,
			dbproto::TITLE_INFO, title_info, "0");
	if (ret) {
		return ret;
	}
	if (sync_cli) {
		onlineproto::sc_0x0616_syn_all_titles sync_msg;
		sync_msg.mutable_list()->CopyFrom(title_info);
		send_msg_to_player(player, cli_cmd_cs_0x0616_syn_all_titles, sync_msg);
	}
	return 0;
}

bool Title::check_title_expire(uint32_t title_id)
{
	//称号id合法性应该在此函数之外检查
	title_conf_t* title_ptr = g_title_conf_mgr.get_title_conf(title_id);
	if (title_ptr) {
		if (title_ptr->end_type == 1) {
			return false;
		} else if (title_ptr->end_type == 2) {
			if (NOW() > title_ptr->end_param) {
				return true;
			} else {
				return false;
			}
		} else if (title_ptr->end_type == 3) {
			title_info_t title_info;
			get_title_info(title_id, title_info);
			if (NOW() > title_info.get_time + title_ptr->end_param) {
				return true;
			} else {
				return false;
			}
		} else if (title_ptr->end_type == 4) {
			title_info_t title_info;
			get_title_info(title_id, title_info);
			//找到获得该称号时间的对应的周五0点的时间戳
			uint32_t expire_time = TimeUtils::get_next_x_time(title_info.get_time, 5);
			if (NOW() > expire_time) {
				return true;
			} else {
				return false;
			}
			TRACE_TLOG("zjun0521,check_title_expire:id=%uexpire_time=%u,get_time=%u,now=%u",
					title_id, expire_time, title_info.get_time, NOW());
		}
	}
	return true;
}

bool Title::check_player_has_this_title(uint32_t title_id)
{
	return title_map_.count(title_id) > 0 ? true : false;
}

int GetTitleCmdProcessor::proc_pkg_from_client(player_t* player,
		const char* body, int bodylen)
{
	//去排行榜中检测自己是否是本周第一
	if (GET_A(kWeeklyCheckRankInfo) == 0 ||
			TimeUtils::check_is_week_past(GET_A(kWeeklyCheckRankInfo), NOW())) {
		std::vector<rank_key_order_t> key_vec;
		struct rank_key_order_t tmp;

		uint32_t friday_date = TimeUtils::get_prev_friday_date();

		tmp.key.key = commonproto::RANKING_MOUNT;
		tmp.key.sub_key = friday_date;
        tmp.order = commonproto::RANKING_ORDER_DESC;
		key_vec.push_back(tmp);

		tmp.key.key = commonproto::RANKING_WING;
		tmp.key.sub_key = friday_date;
        tmp.order = commonproto::RANKING_ORDER_DESC;
		key_vec.push_back(tmp);

		tmp.key.key = commonproto::RANKING_ACHIEVEMENT;
		tmp.key.sub_key = friday_date;
        tmp.order = commonproto::RANKING_ORDER_DESC;
		key_vec.push_back(tmp);

		tmp.key.key = commonproto::RANKING_TOTAL_POWER;
		tmp.key.sub_key = friday_date;
        tmp.order = commonproto::RANKING_ORDER_DESC;
		key_vec.push_back(tmp);

		tmp.key.key = commonproto::RANKING_SPIRIT_TOTAL_POWER;
		tmp.key.sub_key = friday_date;
        tmp.order = commonproto::RANKING_ORDER_DESC;
		key_vec.push_back(tmp);
		return RankUtils::get_user_rank_info_by_keys(player, key_vec,
				player->userid, player->create_tm);
	}
	cli_out_.Clear();
	DataProtoUtils::pack_all_titles_info(player, cli_out_.mutable_list());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int GetTitleCmdProcessor::proc_pkg_from_serv(player_t* player,
		const char *body, int bodylen)
{
	PARSE_SVR_MSG(rank_out_);
	if (rank_out_.rank_info_size() > 2) {
		for (int i = 0; i < rank_out_.rank_info_size(); ++i) {
			const commonproto::rank_player_info_t& inf = rank_out_.rank_info(i);
			std::string str = inf.rank_key();
			std::vector<std::string> str_key = split(str, ':');
			uint32_t key = boost::lexical_cast<uint32_t>(str_key[0]);
			if (inf.rank() != 1) {
				continue;
			}
			uint32_t prize_id = 0;
			std::string content;
			if (key == commonproto::RANKING_MOUNT) {
				prize_id = 11820;
				content = "恭喜获得坐骑排行榜第一名,获得相应称号，快去看看吧";
				onlineproto::sc_0x0112_notify_get_prize noti_msg;
				transaction_proc_prize(player, prize_id, noti_msg,
						commonproto::PRIZE_REASON_NO_REASON, onlineproto::SYNC_REASON_NONE,
						NO_ADDICT_DETEC);
				send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_msg);

				std::string title = "领取称号奖励";
				PlayerUtils::generate_new_mail(player, title, content);
			} 
			if (key == commonproto::RANKING_WING) {
				prize_id = 11821;
				content = "恭喜获得翅膀排行榜第一名,获得相应称号，快去看看吧";
				onlineproto::sc_0x0112_notify_get_prize noti_msg;
				transaction_proc_prize(player, prize_id, noti_msg,
						commonproto::PRIZE_REASON_NO_REASON, onlineproto::SYNC_REASON_NONE,
						NO_ADDICT_DETEC);
				send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_msg);

				std::string title = "领取称号奖励";
				PlayerUtils::generate_new_mail(player, title, content);
			} 
			if (key == commonproto::RANKING_ACHIEVEMENT) {
				prize_id = 11822;
				content = "恭喜获得成就排行榜第一名,获得相应称号，快去看看吧";
				onlineproto::sc_0x0112_notify_get_prize noti_msg;
				transaction_proc_prize(player, prize_id, noti_msg,
						commonproto::PRIZE_REASON_NO_REASON, onlineproto::SYNC_REASON_NONE,
						NO_ADDICT_DETEC);
				send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_msg);

				std::string title = "领取称号奖励";
				PlayerUtils::generate_new_mail(player, title, content);
			} 
			if (key == commonproto::RANKING_TOTAL_POWER) {
				prize_id = 11808;
				content = "恭喜获得总战力排行榜第一名,获得相应称号，快去看看吧";
				onlineproto::sc_0x0112_notify_get_prize noti_msg;
				transaction_proc_prize(player, prize_id, noti_msg,
						commonproto::PRIZE_REASON_NO_REASON, onlineproto::SYNC_REASON_NONE,
						NO_ADDICT_DETEC);
				send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_msg);

				std::string title = "领取称号奖励";
				PlayerUtils::generate_new_mail(player, title, content);
			} 
			if (key == commonproto::RANKING_SPIRIT_TOTAL_POWER) {
				prize_id = 11818;
				content = "恭喜获得伙伴总战力排行榜第一名,获得相应称号，快去看看吧";
				onlineproto::sc_0x0112_notify_get_prize noti_msg;
				transaction_proc_prize(player, prize_id, noti_msg,
						commonproto::PRIZE_REASON_NO_REASON, onlineproto::SYNC_REASON_NONE,
						NO_ADDICT_DETEC);
				send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_msg);

				std::string title = "领取称号奖励";
				PlayerUtils::generate_new_mail(player, title, content);
			}
		}
		//拉取排名服名次
		uint32_t friday_date = TimeUtils::get_prev_friday_date();
		uint32_t time = TimeUtils::date_to_time(friday_date);
		std::vector<rank_key_order_t> key_vec;
		struct rank_key_order_t tmp;
		tmp.key.key = commonproto::RANKING_ARENA;
		tmp.key.sub_key = time;
        tmp.order = commonproto::RANKING_ORDER_ASC;
		key_vec.push_back(tmp);

		tmp.key.key = commonproto::RANKING_RPVP;
		tmp.key.sub_key = time;
		tmp.order = commonproto::RANKING_ORDER_DESC;
		key_vec.push_back(tmp);
		return RankUtils::get_user_rank_info_by_keys(player, key_vec,
				player->userid, player->create_tm);
	} else if (rank_out_.rank_info_size() == 2) {
		//const commonproto::rank_player_info_t& inf = rank_out_.rank_info(0);
		for (int i = 0; i < rank_out_.rank_info_size(); ++i) {
			const commonproto::rank_player_info_t& inf = rank_out_.rank_info(i);
			std::string str = inf.rank_key();
			std::vector<std::string> str_key = split(str, ':');
			uint32_t key = boost::lexical_cast<uint32_t>(str_key[0]);
			if (inf.rank() == 1 && key == commonproto::RANKING_ARENA) {
				uint32_t prize_id = 11819;
				onlineproto::sc_0x0112_notify_get_prize noti_msg;
				transaction_proc_prize(player, prize_id, noti_msg,
						commonproto::PRIZE_REASON_NO_REASON, onlineproto::SYNC_REASON_NONE,
						NO_ADDICT_DETEC);
				send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_msg);
				std::string title = "领取称号奖励";
				std::string content = "恭喜获得排位赛排行榜第一名,获得相应称号，快去看看吧";
				PlayerUtils::generate_new_mail(player, title, content);
			}
			if (key == commonproto::RANKING_RPVP) {
				//Confirm kevin : 竞技场称号不发了
				continue;

				/*
				uint32_t prize_id = 0;
				if (inf.rank() == 1) {
					prize_id = 11830;
				} else if (inf.rank() >= 2 && inf.rank() <= 3) {
					prize_id = 11831;
				} else if (inf.rank() >= 4 && inf.rank() <= 10) {
					prize_id = 11832;
				}
				if (prize_id) {
					onlineproto::sc_0x0112_notify_get_prize noti_msg;
					transaction_proc_prize(player, prize_id, noti_msg,
							commonproto::PRIZE_REASON_NO_REASON, onlineproto::SYNC_REASON_NONE,
							NO_ADDICT_DETEC);
					send_msg_to_player(player, cli_cmd_cs_0x0112_notify_get_prize, noti_msg);
				}
				std::string title = "领取称号奖励";
				std::string str_rank = boost::lexical_cast<std::string>(inf.rank());
				std::string content = "恭喜获得竞技场排行榜第"+ str_rank +"名,获得相应称号，快去看看吧";
				PlayerUtils::generate_new_mail(player, title, content);
				*/
			}
		}
	}
	SET_A(kWeeklyCheckRankInfo, NOW());
	cli_out_.Clear();
	DataProtoUtils::pack_all_titles_info(player, cli_out_.mutable_list());
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

//Confirm kevin 称号加战斗属性需求被砍掉了,其实卸载与安装称号这两个协议可以合并成一个协议；
//现在保留这两个协议，以防策划未来又将称号加战斗属性的需求给添加
int EquipTitleCmdProcessor::proc_pkg_from_client(player_t* player,
		const char* body, int bodylen)
{
	PARSE_MSG;
	if (GET_A(kAttrEquipTitleId) == cli_in_.title_id()) {
		RET_ERR(cli_err_has_equip_this_title);
	}
	//查看玩家是否已经拥有该称号
	if (!player->title->check_player_has_this_title(cli_in_.title_id())) {
		RET_ERR(cli_err_not_get_this_title);
	}
	//检查该称号是否已经过期，过期不能安装
	if (player->title->check_title_expire(cli_in_.title_id())) {
		RET_ERR(cli_err_this_title_has_expire);
	}
	SET_A(kAttrEquipTitleId, cli_in_.title_id());
	MapUtils::sync_map_player_info(player, commonproto::PLAYER_TITLE_CHANGE);
	cli_out_.Clear();
	cli_out_.set_title_id(cli_in_.title_id());	
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}

int UnloadTitleCmdProcessor::proc_pkg_from_client(player_t* player,
		const char* body, int bodylen)
{
	PARSE_MSG;
	if (GET_A(kAttrEquipTitleId) != cli_in_.title_id()) {
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_not_equip_thit_title);
	}
	if (!player->title->check_player_has_this_title(cli_in_.title_id())) {
		SET_A(kAttrEquipTitleId, 0);
		return send_err_to_player(player, player->cli_wait_cmd,
				cli_err_not_get_this_title);
	}
	SET_A(kAttrEquipTitleId, 0);
	MapUtils::sync_map_player_info(player, commonproto::PLAYER_TITLE_CHANGE);
	cli_out_.Clear();
	cli_out_.set_title_id(cli_in_.title_id());	
	return send_msg_to_player(player, player->cli_wait_cmd, cli_out_);
}
