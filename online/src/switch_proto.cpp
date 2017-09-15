#include "common.h"
#include "player.h"
#include "switch_proto.h"
#include "player_manager.h"
#include "service.h"
#include "dll_iface.h"
#include "global_data.h"
#include "friend.h"
#include "proto/home_svr/home_cmd.h"
#include "proto_processor.h"
#include "proto/client/pb0x06.pb.h"
#include "proto/client/pb0x05.pb.h"
#include "proto/client/cli_cmd.h"
#include "family_utils.h"
#include "player_utils.h"
#include "map_utils.h"
#include "proto/db/dbproto.mail.pb.h"
#include "rank_utils.h"

//业务协议相关
/*online启动时 必须要首先注册到switch 才允许客户端登录*/
//下面这个函数在switch连接上时会主动发送 然后启动一个超时定时器
int reg_to_switch_req(void *owner, void *data)
{
#if 1
    //NOTI(singku 检测sw注册超时)
    if (data) {//FIRST_REGIST_REQ
        //第一次主动调用注册函数 那么一定还未注,则online不可登录
        g_svr_loginable = false;
    }

    //超时调用的时候 如果已经收到注册回包了 那么是可以登录的了 不需要理会
    if (g_svr_loginable == true) {//可以登录 说明已经在sw注册过了
        return 0;
    }

    //超时调用
    if (!data) {//
        //注册到SW超时
        WARN_TLOG("REG_TO_SW: TIME OUT");
    }
#endif

    // 不检测心跳和超时的时候 既然要去sw注册 那么online一定目前是不可登陆的
    g_svr_loginable = false;
    switchproto::cs_register_server sw_msg;
    sw_msg.set_server_id(g_server_id);
    sw_msg.set_online_id(g_online_id);
    sw_msg.set_server_type(commonproto::SERVER_TYPE_ONLINE);
    sw_msg.set_listen_port(get_server_port());
    sw_msg.set_reg_mode(switchproto::SERVER_REG_MODE_WAIT);
    sw_msg.set_idc_zone(g_server_config.idc_zone);
    sw_msg.set_svr_name(get_server_name());
    struct in_addr inp;
    inet_aton(get_server_ip(), &inp);
    sw_msg.set_host_ip(ntohl(inp.s_addr));
    g_switch->send_msg(0, g_online_id, 0, sw_cmd_register_server, sw_msg);

    return 0;
}

int online_sync_player_info()
{
    switchproto::cs_online_sync_player_info syn_msg;
    std::vector<player_t *>player_vec;
    g_player_manager->get_player_list(player_vec);

    uint32_t count = 0;
    syn_msg.clear_player_list();
    FOREACH(player_vec, it) {
        player_t *player = *it;
        if (!player->userid) {
            WARN_TLOG("BUG!!!! uid_to_player_map_t has uid = 0");
            continue;
        }
        switchproto::sw_player_basic_info_t *info = syn_msg.add_player_list();
        info->set_userid(player->userid);
        info->set_create_tm(GET_A(kAttrCreateTm));
        int32_t vip = (is_vip(player)) ?1 :0;
        info->set_is_vip(vip);
        if (++count == 500) {//每次同步500人 包长大概在5K
            g_switch->send_msg(0, g_online_id, 0, sw_cmd_online_sync_player_info, syn_msg);
            count = 0;
            syn_msg.clear_player_list();
        }
    }

    if (count > 0) {
        g_switch->send_msg(0, g_online_id, 0, sw_cmd_online_sync_player_info, syn_msg);
    }
    return 0;
}

int online_report_player_onoff(player_t *player, uint8_t login_or_logout)
{
    if (g_svr_loginable == false) {//如果switch还未告知可连接
        return 0;
    }

    switchproto::cs_online_report_player_onoff rpt_msg;
    switchproto::sw_player_basic_info_t *info = rpt_msg.mutable_basic();
    info->set_userid(player->userid);
    info->set_create_tm(GET_A(kAttrCreateTm));
    info->set_online_id(g_online_id);
    info->set_server_id(g_server_id);
    int32_t vip = (is_vip(player)) ?1 :0;
    info->set_is_vip(vip);
    rpt_msg.set_login_or_logout(login_or_logout);
    return g_switch->send_msg(0, g_online_id, 0, sw_cmd_online_report_player_onoff, rpt_msg);
}

int SwitchRegCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
#if 0 //NOTI(singku) 不检测超时
    if (switch_tmr) {
        DEL_TIMER(switch_tmr);
    }
#endif

    g_svr_loginable = true;
    online_sync_player_info();
    return 0;
}

int SwitchGetSvrListCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    sw_out_.Clear();
    parse_message(body, bodylen, &sw_out_);
    return 0;
}

int SwitchNotifyKickPlayerCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    sw_out_.Clear();
    if (parse_message(body, bodylen, &sw_out_)) {
        return 0;
    }
    //发T人包给前端
    uint32_t uid = sw_out_.basic().userid();
    player_t *dst = g_player_manager->get_player_by_userid(uid);
    if (!dst) {
        WARN_TLOG("SWITCH send kick off cmd to u:%u BUT u is not exist", uid);
        return 0;
    }
    send_err_to_player(dst, 0, cli_err_kick_off);
	close_client_conn(dst->fdsess->fd);
    return 0;
}

int SwitchNotifyEraseEscortInfoCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
    sw_out_.Clear();
    if (parse_message(body, bodylen, &sw_out_)) {
        return 0;
    }
	uint32_t uid = sw_out_.uid();
	uint32_t create_tm = sw_out_.u_create_tm();
	uint64_t role_key = ROLE_KEY(ROLE(uid, create_tm));
	
	escort_info_t* player_ptr = g_escort_mgr.get_user_escort_info(role_key);	
	if (player_ptr == NULL) {
		return cli_err_escort_player_info_not_found;
	}
	g_escort_mgr.sync_player_info_other(uid, create_tm, onlineproto::ES_SHIP_DISAPPEAR);

	g_escort_mgr.del_player_from_escort(uid, create_tm);
	return 0;
}

int SwitchNotifyAttrChangedCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	sw_out_.Clear();
    if (parse_message(body, bodylen, &sw_out_)) {
        return 0;
    }
	player = g_player_manager->get_player_by_userid(sw_out_.uid());
	if (player == NULL) {
		ERROR_TLOG("Not Find Player In this online:uid=[%u]", sw_out_.uid());
		return 0;
	}
		
	uint32_t attr_size = sw_out_.attr_list_size();
	for (uint32_t i = 0; i < attr_size; ++i) {
		const commonproto::attr_data_change_t& attr_inf = sw_out_.attr_list(i);
		uint32_t type = attr_inf.type();
		uint32_t change_value = attr_inf.change_value();
		bool is_minus = attr_inf.is_minus();
		if (!is_minus) {
            if (type == kAttrGold || type == kAttrPaidGold) {//加金币
                AttrUtils::add_player_gold(player, change_value, type==kAttrPaidGold ?true :false, "其他渠道");
            } else if (type == kAttrDiamond || type == kAttrPaidDiamond) {//加钻石
                player_chg_diamond_and_sync(player, change_value, 0, 1, 
                        dbproto::CHANNEL_TYPE_SERVICE_ADD, type==kAttrPaidDiamond ?"获得付费钻石" :"获得免费钻石",
                        SYNC_DB, type==kAttrPaidDiamond ?true :false);
            } else {
                ADD_A((attr_type_t)type, change_value);
            }
		} else {
            if (type == kAttrGold || type == kAttrPaidGold) {//扣金币
                AttrUtils::sub_player_gold(player, change_value, "其他渠道");
            } else if (type == kAttrDiamond || type == kAttrPaidDiamond) {//扣钻石
                player_chg_diamond_and_sync(player, -change_value, 0, 1, 
                        dbproto::CHANNEL_TYPE_SERVICE_DEL, type==kAttrPaidDiamond ?"消耗付费钻石" :"消耗免费钻石",
                        SYNC_DB);
            } else {
                SUB_A((attr_type_t)type, change_value);
            }
		}
	}
	return 0;	
}

int SwitchTransmitCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    sw_out_.Clear();
    if (parse_message(body, bodylen, &sw_out_)) {
        return 0;
    }
    uint32_t transmit_type = sw_out_.transmit_type();
    uint32_t cmd = sw_out_.cmd();
    
    if (transmit_type == switchproto::SWITCH_TRANSMIT_USERS) {
		//hack
		//收到不同服玩家发来的消息的处理逻辑
		if (cmd == cli_cmd_cs_0x0605_notify_personal_message) {
			//正常情况下一定会有一个receiver,所以如果没有就
			//报错
			if (sw_out_.receivers_size() == 0) {
				return 0;
			}
            const switchproto::sw_player_basic_info_t &p_info = sw_out_.receivers(0);
			//如果receivers的id不在当前online中，就报错
            player_t* ply = g_player_manager->get_player_by_userid(p_info.userid());
			if (ply == NULL) {
				return 0;
			}
			
			onlineproto::sc_0x0605_notify_personal_message notify_personal_message_;
			if (parse_message(sw_out_.pkg().c_str(), sw_out_.pkg().size(), &notify_personal_message_)) {
				return 0;
			}
			//黑名单丢包，因为申请者在我黑名单中，所以就屏蔽掉对方发来的消息
			if (ply->friend_info->has_black(notify_personal_message_.sender(),
						notify_personal_message_.s_create_tm())) {
				return 0;
			}
			//如果自己好友中没有对方，则收到加好友提醒，而不是对方发的消息
			if (ply->friend_info->has_friend(notify_personal_message_.sender(),
						notify_personal_message_.s_create_tm()) == false) {
				onlineproto::sc_0x0606_notify_add_friend sc_0x0606_notify_add_friend_;
				sc_0x0606_notify_add_friend_.set_userid(notify_personal_message_.receiver());
				sc_0x0606_notify_add_friend_.set_friendid(notify_personal_message_.sender());
				sc_0x0606_notify_add_friend_.set_nick(notify_personal_message_.sender_nick());
				return send_msg_to_player(ply, cli_cmd_cs_0x0606_notify_add_friend, sc_0x0606_notify_add_friend_);
			}
        }

        //收到不同服玩家的添加好友请求处理的逻辑
        if (cmd == cli_cmd_cs_0x0606_notify_add_friend) {
            //正常情况下一定会有一个receiver,所以如果没有就报错
            if (sw_out_.receivers_size() == 0) {
                return 0;
            }
            const switchproto::sw_player_basic_info_t &p_info = sw_out_.receivers(0);
            //如果receivers的id不在当前online中，就报错
            player_t* ply = g_player_manager->get_player_by_userid(p_info.userid());
            if (ply == NULL) {
                return 0;
            }

            onlineproto::sc_0x0606_notify_add_friend notify_add_friend_;
            if (parse_message(sw_out_.pkg().c_str(), sw_out_.pkg().size(), &notify_add_friend_)) {
                return 0;
            }
            //黑名单丢包，因为申请者在我黑名单中，所以就屏蔽掉对方发来的好友申请
			if (ply->friend_info->has_black(notify_add_friend_.friendid(),
						notify_add_friend_.f_create_tm())) {
				return 0;
			}
            //只有当自己好友中没有对方时，才接收申请好友提示
			if (ply->friend_info->has_friend(notify_add_friend_.friendid(),
						notify_add_friend_.f_create_tm())) {
				return 0;
			}
        }

        // TODO toby 封装register
        // 家族消息通知
        if (cmd == cli_cmd_cs_0x0720_family_msg_notice) {
            onlineproto::sc_0x0720_family_msg_notice notice_out_;
            if (parse_message(
                    sw_out_.pkg().c_str(), sw_out_.pkg().size(), &notice_out_)) {
                return 0;
            }

            const switchproto::sw_player_basic_info_t &p_info = sw_out_.receivers(0);
            player = g_player_manager->get_player_by_userid(p_info.userid());
            if (player) {
                for (int i = 0; i < notice_out_.msgs_size();i++) {
                    commonproto::family_msg_t *family_msg = notice_out_.mutable_msgs(i);
                    if (family_msg->type() == commonproto::FAMILY_MSG_TYPE_DISMISS_SUCC ||
                            family_msg->type() == commonproto::FAMILY_MSG_TYPE_KICK_OUT) {
                        // 解散或者被踢出
                        //player->family_hall_map_id = 0;
                        player->family_hall_line_id = 0;
                        memset(player->family_name, 0, sizeof(player->family_name));
                        PlayerUtils::calc_player_battle_value(player, onlineproto::ATTR_OTHER_REASON);
                        MapUtils::sync_map_player_info(player, commonproto::PLAYER_FAMILY_CHANGE);
                    } else if (family_msg->type() == commonproto::FAMILY_MSG_TYPE_ACCPET_LEADER_REASSIGN) {
                        // 接受族长转让
                        if (player->temp_info.family_leader_reassign_role.userid == family_msg->sender().userid() &&
                                player->temp_info.family_leader_reassign_role.u_create_tm == family_msg->sender().u_create_tm()) {
                            player->temp_info.family_leader_reassign_response = true;
                        }
                    } else if (family_msg->type() == commonproto::FAMILY_MSG_TYPE_JOIN_SUCC) {
                        // 申请通过
                        STRCPY_SAFE(player->family_name, family_msg->family_name().c_str());
                        PlayerUtils::calc_player_battle_value(player, onlineproto::ATTR_OTHER_REASON);
                        player->family_apply_record->erase(family_msg->family_id());
                        commonproto::family_apply_record_t apply_record;
                        FOREACH(*(player->family_apply_record), iter) {
                            apply_record.add_family_ids(*iter);
                        }
                        PlayerUtils::update_user_raw_data(
                                player->userid, player->create_tm, dbproto::FAMILY_APPLY_RECORD, apply_record, "0");
                        MapUtils::sync_map_player_info(player, commonproto::PLAYER_FAMILY_CHANGE);
                    } else if (family_msg->type() == commonproto::FAMILY_MSG_TYPE_APPLY_JOIN_FAILED) {
                        // 申请被拒绝
                        player->family_apply_record->erase(family_msg->family_id());
                        commonproto::family_apply_record_t apply_record;
                        FOREACH(*(player->family_apply_record), iter) {
                            apply_record.add_family_ids(*iter);
                        }
                        PlayerUtils::update_user_raw_data(
                                player->userid, player->create_tm, dbproto::FAMILY_APPLY_RECORD, apply_record, "0");
                    }
                }
            }
        }

        if (cmd == cli_cmd_cs_0x0111_sync_attr) {
            for (int i = 0; i < sw_out_.receivers_size(); i++) {
                const switchproto::sw_player_basic_info_t &p_info = sw_out_.receivers(i);
                player = g_player_manager->get_player_by_userid(p_info.userid());
                if (player) {
                    onlineproto::sc_0x0111_sync_attr notice_out_;
                    if (parse_message(
                                sw_out_.pkg().c_str(), sw_out_.pkg().size(), &notice_out_)) {
                        return 0;
                    }

                    for (int i = 0 ;i < notice_out_.attr_list_size();i++) {
                        attr_type_t  type = (attr_type_t)notice_out_.mutable_attr_list(i)->type();
                        uint32_t value = notice_out_.mutable_attr_list(i)->value();

                        // 世界boss奖励通知
                        if (type == kAttrWorldBossRewardRecord) {
                            uint32_t world_boss_dup_id = commonproto::WORLD_BOSS_DUP_ID_1;
                            uint32_t cur_mask = GET_A(kAttrWorldBossRewardRecord);
                            uint32_t last_svr_id = GET_A(kAttrWorldBossLastSvrId);
                            //if ((cur_mask & 0x000f0000) == 0 &&
                            if((g_world_boss_mgr.get_reward_status(world_boss_dup_id,cur_mask)  != 
                                     commonproto::WORLD_BOSS_REWARD_ALREADY_GET) && 
                                    (g_world_boss_mgr.get_reward_status(world_boss_dup_id, value)  == 
                                     commonproto::WORLD_BOSS_REWARD_OPEN) && 
                                    last_svr_id  > 0 &&
                                    last_svr_id != g_online_id) {
                                // 没有领过奖，通知可以领奖，并且没有在本服参加过同场次的世界boss挑战活动,
                                // 已经参加过就不处理，等本服世界boss挑战结束后发奖励,
                                // 或者再次在结束前下线，在下次登录时领取参加过且最后被击杀的世界boss挑战奖励
                                g_world_boss_mgr.give_world_boss_single_user_reward(
                                        world_boss_dup_id, player->userid, player->create_tm, value);
                            }
                        } else {
                            SET_A(type, value);

                            // 家族属性同步, 申请处理、升级
                            if (type == kAttrFamilyLevel) {
                                // 家族等级改变
                                PlayerUtils::calc_player_battle_value(player, onlineproto::ATTR_OTHER_REASON);
                            }
                        }
                    }

                    // 家族属性同步, 踢出
                    if (GET_A(kAttrFamilyId) == 0) {
                        FamilyUtils::clear_self_family_info(player);
                    }
                }
            }
        }

		//用户转发
        for (int i = 0; i < sw_out_.receivers_size(); i++) {
            const switchproto::sw_player_basic_info_t &p_info = sw_out_.receivers(i);
            player = g_player_manager->get_player_by_userid(p_info.userid());
            if (player && (GET_A(kAttrCreateTm) == (uint32_t)p_info.create_tm() || (uint32_t)p_info.create_tm() == 0)) {
                send_buff_to_player(player, cmd, sw_out_.pkg().c_str(), sw_out_.pkg().size());
            }
        }

    //注意!!!下面的消息都是发给本服所有人的
    } else if (transmit_type == switchproto::SWITCH_TRANSMIT_SERVERS //跨服
            || transmit_type == switchproto::SWITCH_TRANSMIT_WORLD  //世界
            || transmit_type == switchproto::SWITCH_TRANSMIT_SYSNOTI) {//系统广播
        std::vector<player_t*> player_vec;
        g_player_manager->get_player_list(player_vec);
        FOREACH(player_vec, it) {
            player = *it;
            send_buff_to_player(player, cmd, sw_out_.pkg().c_str(), sw_out_.pkg().size());
        }
    } else {
        ERROR_TLOG("UNKOWN_SWITCH_TRANS: cmd:0x%04x, type=%d", cmd, transmit_type);
    }
    return 0;    
}

int SwitchNotifyNewMailCmdProcessor::proc_pkg_from_serv(
        player_t *player, const char *body, int bodylen)
{
    sw_out_.Clear();
    if (parse_message(body, bodylen, &sw_out_)) {
        return 0;
    }
	onlineproto::sc_0x0505_inform_new_mail noti_msg;
	noti_msg.add_mail()->CopyFrom(sw_out_.mail_data());
	player_t* ply = g_player_manager->get_player_by_userid(sw_out_.userid());
	if (ply) {
		send_msg_to_player(ply, cli_cmd_cs_0x0505_inform_new_mail, noti_msg);
	}
	return 0;
}

int SwNotifyNewMailToSvrCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	sw_out_.Clear();
    if (parse_message(body, bodylen, &sw_out_)) {
        return 0;
    }
	onlineproto::sc_0x0505_inform_new_mail noti_msg;
	noti_msg.add_mail()->CopyFrom(sw_out_.mail_data());
	//找到该服所有在线的玩家
	std::vector<player_t*> player_list;
    g_player_manager->get_player_list(player_list);
	FOREACH(player_list, it) {
		player_t* p = *it;
		if (p->is_login) {
			dbproto::cs_mail_new db_in;
			db_in.mutable_mail_data()->CopyFrom(sw_out_.mail_data());
			g_dbproxy->send_msg(NULL, p->userid,
					p->create_tm, db_cmd_mail_new,
					db_in);

			send_msg_to_player(p, cli_cmd_cs_0x0505_inform_new_mail, noti_msg);
		}
	}
	return 0;
}

int SwNtfFrozenAccountCmdProcessor::proc_pkg_from_serv(
		player_t *player, const char *body, int bodylen)
{
	sw_out_.Clear();
    if (parse_message(body, bodylen, &sw_out_)) {
        return 0;
    }
	onlineproto::sc_0x0008_noti_act_frozen noti_msg;
	noti_msg.set_type((commonproto::act_frozen_type_t)sw_out_.frozen_reason());
	noti_msg.set_data((uint32_t)sw_out_.dur());
	player_t* ply = g_player_manager->get_player_by_userid(sw_out_.userid());
	if (ply) {
		send_msg_to_player(ply, cli_cmd_cs_0x0008_noti_act_frozen, noti_msg);

	    //T他下线 TODO kevin:即刻踢他下线，还是用定时器延后几秒
	    close_client_conn(ply->fdsess->fd);
	}
	
	return 0;
}

int SwOnlyNotifyAttrChangedCmdProcessor::proc_pkg_from_serv(
	player_t *player, const char *body, int bodylen)
{
	sw_out_.Clear();
    if (parse_message(body, bodylen, &sw_out_)) {
        return 0;
    }
	player = g_player_manager->get_player_by_userid(sw_out_.uid());
	if (player == NULL) {
		ERROR_TLOG("Not Find Player In this online:uid=[%u]", sw_out_.uid());
		return 0;
	}
	uint32_t attr_size = sw_out_.attr_size();

    uint32_t add_diamond = 0;
	bool recharge_flag = false;
    LOCK_SVR_MSG;
	for (uint32_t i = 0; i < attr_size; ++i) {
		const commonproto::attr_data_t& attr_inf = sw_out_.attr(i);
        uint32_t type = attr_inf.type();
        uint32_t new_value = attr_inf.value();
        int32_t change_value =  (int)new_value - (int)GET_A((attr_type_t)type);

		if (change_value > 0) {
            if (type == kAttrGold || type == kAttrPaidGold) {//加金币
                AttrUtils::add_player_gold(player, change_value, type==kAttrPaidGold ?true :false, "其他渠道");
            } else if (type == kAttrDiamond || type == kAttrPaidDiamond) {//加钻石
				if (type == kAttrPaidDiamond) {
					recharge_flag = true;
				}
                player_chg_diamond_and_sync(player, change_value, 0, 1, 
                        dbproto::CHANNEL_TYPE_SERVICE_ADD, type==kAttrPaidDiamond ?"获得付费钻石" :"获得免费钻石",
                        false, type==kAttrPaidDiamond ?true :false);
                add_diamond += change_value;

            } else {
                SET_A((attr_type_t)type, new_value);
            }
		} else if (change_value < 0){
            if (type == kAttrGold || type == kAttrPaidGold) {//扣金币
                AttrUtils::sub_player_gold(player, -change_value, "其他渠道");
            } else if (type == kAttrDiamond || type == kAttrPaidDiamond) {//扣钻石
                player_chg_diamond_and_sync(player, change_value, 0, 1, 
                        dbproto::CHANNEL_TYPE_SERVICE_DEL, type==kAttrPaidDiamond ?"消耗付费钻石" :"消耗免费钻石",
                        false);
            } else {
                SET_A((attr_type_t)type, new_value);
            }
		}
		SET_A((attr_type_t)attr_inf.type(), attr_inf.value());
	}
    UNLOCK_SVR_MSG;
	//设置下最近充值的时间戳
	if (recharge_flag) {
		SET_A(kAttrFirstRechargeFlag, NOW());
	}
    //充钻抽卡
    if (add_diamond && TimeUtils::is_current_time_valid(TM_CONF_KEY_CHARGE_DIAMOND_DRAW, 0)) {//活动时间内
        ADD_A(kAttrChargeDiamondDrawPrizeChargeCnt, add_diamond);
    }
    if (add_diamond) {
        ADD_A(kDailyChargeDiamondCnt, add_diamond);
		//活动范围内增加充钻次数，与充值的钻石数量
		AttrUtils::add_attr_in_special_time_range(player,
				TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
				kAttrActivRechargeDiamondTimes);
		AttrUtils::add_attr_in_special_time_range(player, 
				TM_CONF_KEY_ACTIVITY_OPEN_TIME, 3,
				kAttrActivRechargeDiamondCnt, add_diamond);
    }
    //冲钻送豪礼
    if (add_diamond && TimeUtils::is_current_time_valid(TM_CONF_KEY_CHARGE_DIAMOND_GET_GIFT, 0)) {
        ADD_A(kAttrChargeDiamondGetGiftChargeCnt, add_diamond);
    }
	//限时充值排行
    // if (add_diamond && TimeUtils::is_current_time_valid(TM_CONF_KEY_RANKING_TIME_LIMIT, 1)) {//活动时间内
	if(add_diamond && g_srv_time_mgr.is_now_time_valid(TM_SUBKEY_DIAMOND_RECHARGE)){
        ADD_A(kAttrDiamondRechargeRankingCount, add_diamond);
		// uint32_t start_day = TimeUtils::get_start_time(TM_CONF_KEY_RANKING_TIME_LIMIT, 1);
		uint32_t start_day = g_srv_time_mgr.get_start_time(TM_SUBKEY_DIAMOND_RECHARGE);
		//更新充值排名
		RankUtils::rank_user_insert_score(
				player->userid, player->create_tm,
				commonproto::RANKING_TL_DIAMOND_RECHARGE, start_day,
				GET_A(kAttrDiamondRechargeRankingCount));
    }

#if 0 //取消
    //钻石充值礼
    if (add_diamond && TimeUtils::is_current_time_valid(TM_CONF_KEY_CHARGE_DIAMOND, 0)) {//活动时间内
        if (!TimeUtils::is_time_valid(GET_A(kAttrChargeDiamondLastJoinTm), TM_CONF_KEY_CHARGE_DIAMOND, 0)) {
            SET_A(kAttrChargeDiamondLastJoinTm, NOW());
            SET_A(kAttrChargeDiamondCnt, 0);
            SET_A(kAttrChargeDiamondGiftGetFlag, 0);
        }
        ADD_A(kAttrChargeDiamondCnt, add_diamond);
    }
#endif

    return 0;
}

#if 0
int sw_notify_player_forbid_talk(player_t *p,
                                ISeer20SWProto::sw_msg_head_t *msghead,
                                ISeer20SWProto::sw_notify_player_forbid_talk_out *msg)
{
    //设置禁言时间 SW已经去DB设置过了
    //为了确认 这里依然去DB设置一次
    p = PLAYER_MGR.get_player_by_uid(msg->userid());
    if (!p) {
        return 0;
    }

    //设置online
    p->set_forbid_talk_dur(msg->dur());
    p->set_forbid_talk_start_tm(NOW());
   
    //sync db
    ISeer20DBProto::db_forbid_player_talk_in db_msg;
    db_msg.set_duration(msg->dur());
    send_to_db(p, db_msg, DONT_WAIT_DB_ACK);

    if (p->status == player_status_away) {
        return 0;
    }
    //noti cli
    ISeer20CSProto::notify_forbid_talk_out noti;
    noti.set_remain_time(msg->dur());
    send_msg_to_player(p, noti, DONT_CLEAR_WAITING_MSG);

    return 0;
}

int sw_notify_player_frozen_account(player_t *p,
                                ISeer20SWProto::sw_msg_head_t *msghead,
                                ISeer20SWProto::sw_notify_player_frozen_account_out *msg)
{
    //封停账号
    p = PLAYER_MGR.get_player_by_uid(msg->userid());
    if (!p) {
        return 0;
    }

    //sync to db
    ISeer20DBProto::db_frozen_account_in db_msg;
    db_msg.set_duration(msg->dur());
    send_to_db(p, db_msg, DONT_WAIT_DB_ACK);

    if (p->status != player_status_away) {
        //online 发送通知
        ISeer20CSProto::notify_frozen_account_out noti;
        noti.set_remain_time(msg->dur());
        send_msg_to_player(p, noti, DONT_CLEAR_WAITING_MSG);
    }
    
    //1秒钟后断线
    p->timer_mgr.add_timer(timer_kind_force_player_offline_timeout);
    return 0;
}

int sw_notify_gm_change_attr(player_t *p,
                            ISeer20SWProto::sw_msg_head_t *msghead,
                            ISeer20SWProto::sw_notify_change_attr_out *msg)
{
    p = PLAYER_MGR.get_player_by_uid(msg->userid());
    if (!p) {
        return 0;
    }
    ISeer20DBProto::db_update_player_basic_info_in db_msg;
    ISeer20Comm::player_basic_info_t *basic = db_msg.mutable_basic_info();

    int32_t gcoins_rank_new = 0;
    if (gcoins_rank_new < 0) gcoins_rank_new = 0;
    //改online
    if (msg->coin()) {
        p->set_coin(msg->coin());
        basic->set_coin(msg->coin());
    } 
    if (msg->gcoin()) {
        p->set_gcoin(msg->gcoin());
        basic->set_gcoin(msg->gcoin());
    }
    if (msg->energy()) {
        p->set_cur_energy(msg->energy());
        basic->set_cur_energy(msg->energy());
    }
    if (msg->vip_exp()){
        int32_t chg_vip_exp = msg->vip_exp() - p->vip_exp();
        if (chg_vip_exp) {
            p->player_absorb_gain_vip_exp_and_notify(chg_vip_exp);
            // 充值活动 经验的增加只能是金豆的变化引起的
            //condition_key_t key;
            //key.push_back(chg_vip_exp);
            //update_activity_by_type(p, ACT_TYP_CHARGE, key, chg_vip_exp, DONT_WAIT_DB_ACK);
            update_charge_activity(p,chg_vip_exp,NOW());

            // 补发充值 发消息给rank
            p->set_gcoins_total_rank(p->gcoins_total_rank() + chg_vip_exp);
            ISeer20DBProto::db_update_player_basic_info_in db_msg;
            ISeer20Comm::player_basic_info_t *basic_info = db_msg.mutable_basic_info();
            basic_info->set_gcoins_total_rank(p->gcoins_total_rank());
            db_update_player_basic_info_req(p, &db_msg, DONT_WAIT_DB_ACK);

            p->rank_data.set_score_and_sync(ISeer20RKProto::RANK_TYPE1_GCOINS_TOTAL,
                    ISeer20RKProto::RANK_TYPE2_TOTAL, p->gcoins_total_rank());
        } else {
            p->set_vip_exp(msg->vip_exp());
        }
    }

    // 发消息给db
    send_to_db(NULL, db_msg, DONT_WAIT_DB_ACK, msg->userid(), msg->role_tm());

    // 发消息给客户端
    ISeer20CSProto::notify_player_attr_chg_out noti_msg;
    noti_msg.set_new_coin(p->coin()); 
    noti_msg.set_new_gcoin(p->gcoin());
    noti_msg.set_cur_energy(p->cur_energy());
    noti_msg.set_vip_exp(p->vip_exp());
    noti_msg.set_vip_level(p->vip_level());
    return send_msg_to_player(p, noti_msg, DONT_CLEAR_WAITING_MSG);
}

int sw_notify_gm_new_mail(player_t *p,
                        ISeer20SWProto::sw_msg_head_t *msghead,
                        ISeer20SWProto::sw_notify_new_mail_out *msg)
{
    p = PLAYER_MGR.get_player_by_uid(msg->userid());
    if (!p) {
        return 0;
    }

    //online 添加邮件 通知客户端
    return notify_player_new_mail(p, msg->new_mail());
}


//广播通知
int sw_notify_gm_send_noti(player_t *p,
                        ISeer20SWProto::sw_msg_head_t *msghead,
                        ISeer20SWProto::sw_notify_gm_noti_out *msg)
{
    std::string content(msg->content());
    GLOBAL_MSG_CLIENT.SendMsg_NoDirtyCheck(content, /*MsgChannel_trumpet_server*/MsgChannel_lottery);
    return 0;
}
#endif
