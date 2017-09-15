#include "player_manager.h"
#include "server_manager.h"
#include <execinfo.h>
std::string stack_trace()
{
    // 打印堆栈信息
    void* buffs[100];
    int num_ptrs;
    num_ptrs = backtrace(buffs, array_elem_num(buffs));

    char** strings;
    strings = backtrace_symbols(buffs, num_ptrs);

    std::string stack;
    if (strings) {
        for (int i = 1; i < num_ptrs; i++) {
            stack += std::string(strings[i]) + "\n"; 
        }
        free(strings);
    }

    return stack;
}

int send_err_to_fdsess(fdsession_t *fdsess, uint32_t cmd, int err, uint32_t seq, uint32_t uid)
{
    svr_proto_header_t* header = (svr_proto_header_t *)(g_send_buf);
    uint32_t total_len = sizeof(*header);
    if (sizeof(g_send_buf) < total_len) {
        ERROR_TLOG("%u send buf size %lu not enough[need: %lu]",
                uid, sizeof(g_send_buf), total_len);
        return -1;
    }
 
    header->len = total_len;
    header->cmd = cmd;
    header->seq = seq;
    header->ret = err;
    header->uid = uid;

TRACE_TLOG("Send Err To fdsess Ok: [u:%u cmd:%d, hex_cmd:0X%04X err:%u]",
            uid, cmd, cmd, err);

    return send_pkg_to_client(fdsess, g_send_buf, header->len);
}

int player_t::send_msg_to_player(uint32_t cmd, const google::protobuf::Message &msg)
{
    svr_proto_header_t* header = (svr_proto_header_t *)(g_send_buf);

    uint32_t body_len = msg.ByteSize();
    uint32_t total_len = sizeof(*header) + body_len;
    if (sizeof(g_send_buf) < total_len) {
        ERROR_TLOG("%u send buf size %lu not enough[need: %lu]",
                this->uid, sizeof(g_send_buf), total_len);
        return -1;
    }
 
    msg.SerializeToArray(g_send_buf + sizeof(*header), 
            sizeof(g_send_buf) - sizeof(*header));

    header->len = total_len;
    header->cmd = cmd;
    header->seq = this->fdsess->fd;
    header->ret = 0;
    header->uid = this->uid;

TRACE_TLOG("Send Msg To Player Ok: [u:%u cmd:%d, hex_cmd:0X%04X len(%u+%u)=%u]\nmsg:%s\n[%s]",
            this->uid, cmd, cmd,
            sizeof(*header), body_len, total_len,
            msg.GetTypeName().c_str(), msg.Utf8DebugString().c_str());

    return send_pkg_to_client(this->fdsess, g_send_buf, header->len);
}

int player_t::send_err_to_player(uint32_t cmd, int err)
{
    svr_proto_header_t* header = (svr_proto_header_t *)(g_send_buf);
    uint32_t total_len = sizeof(*header);
    if (sizeof(g_send_buf) < total_len) {
        ERROR_TLOG("%u send buf size %lu not enough[need: %lu]",
                this->uid, sizeof(g_send_buf), total_len);
        return -1;
    }
 
    header->len = total_len;
    header->cmd = cmd;
    header->seq = this->fdsess->fd;
    header->ret = err;
    header->uid = this->uid;

TRACE_TLOG("Send Err To Player Ok: [u:%u cmd:%d, hex_cmd:0X%04X err:%u]",
            this->uid, cmd, cmd, err);

    return send_pkg_to_client(this->fdsess, g_send_buf, header->len);
}

int player_t::relay_to_player(void *data, uint32_t cmd, uint32_t data_len)
{
    if (sizeof(g_send_buf) < data_len) {
        ERROR_TLOG("%u send buf size %lu not enough[need: %lu]",
                this->uid, sizeof(g_send_buf), data_len);
        return -1;
    }
 
    memcpy(g_send_buf, data, data_len);

TRACE_TLOG("Relay To Player Ok: [u:%u cmd:%d, hex_cmd:0X%04X len:%u]",
            this->uid, cmd, cmd, data_len);

    return send_pkg_to_client(this->fdsess, g_send_buf, data_len);
}

int player_t::relay_to_svr(void *data, uint32_t cmd, uint32_t data_len)
{
    if (!this->btl) {
        return -1;
    }

    if (this->btl->send_buf_to_svr(this->uid, cmd, data, data_len)) {
        return -1;
    }

    return 0;
}

player_t* player_manager_t::create_new_player(player_t &player)
{
    player_t *p = new player_t();
    if (!p) {
        ERROR_LOG("Error: failed calloc player_t, uid=%u", player.uid);
        return 0;
    }

    p->uid = player.uid;
    p->score = player.score;
    p->fdsess = player.fdsess;
    p->btl_name = player.btl_name;
    p->btl = 0;
    p->index = this->index++;

    uid_to_player_map_[p->uid] = p;

	return p;
}

void player_manager_t::destroy_player(player_t *p)
{
    assert(p);
    uid_to_player_map_.erase(p->uid);
    rpvp_index_to_player_map_.erase(p->index);
    if (ppve_players_map_.count(p->svr_id)) {
        dupid_to_players_map_t &dup_map = ppve_players_map_.find(p->svr_id)->second;
        if (dup_map.count(p->dup_id)) {
            id_to_player_map_t &index_map = dup_map.find(p->dup_id)->second;
            index_map.erase(p->index);
        }
    }

    if (p->btl) {
        if (p->btl_start)  {
            onlineproto::cs_0x0208_duplicate_exit msg;
            p->btl->send_msg_to_svr(p->uid, cli_cmd_cs_0x0208_duplicate_exit, msg);
        }
        p->btl->del_player(p);
    }
    DEBUG_TLOG("destroy_player uid=%u add=%p svr_id=%u btl=%p dup_id=%u stack:[%s]", 
            p->uid, p, p->svr_id, p->btl, p->dup_id, stack_trace().c_str());

    delete p;
}

void player_manager_t::batch_destroy_players(int fd)
{
    id_to_player_map_iter_t it = uid_to_player_map_.begin();

    for(; it != uid_to_player_map_.end();) {
        player_t *p = it->second;
        if (p->fdsess->fd != fd) {
            it ++;
            continue;
        }
        it ++;
        destroy_player(p);
    }
}

void player_manager_t::add_player_to_rpvp_match(player_t *p)
{
    rpvp_index_to_player_map_[p->index] = p;
}

void player_manager_t::add_player_to_ppve_match(player_t *p, uint32_t svr_id, uint32_t dup_id)
{
    if (ppve_players_map_.count(svr_id) == 0) {
        id_to_player_map_t index_map;
        index_map[p->index] = p;
        dupid_to_players_map_t dup_map;
        dup_map[dup_id] = index_map;
        ppve_players_map_[svr_id] = dup_map;
        return;
    }

    dupid_to_players_map_t &dup_map = ppve_players_map_.find(svr_id)->second;
    if (dup_map.count(dup_id) == 0) {
        id_to_player_map_t index_map;
        index_map[p->index] = p;
        dup_map[dup_id] = index_map;
        return;
    }

    id_to_player_map_t &index_map = dup_map.find(dup_id)->second;
    index_map[p->index] = p;
}

void player_manager_t::do_rpvp_match()
{
    player_t *player = 0;
    player_t *dst = 0;
    FOREACH_NOINCR_ITER(rpvp_index_to_player_map_, it) {
        player = it->second;
        uint32_t t = NOW() - player->request_tm;
#if 0
        if (t > MATCH_TIME) {
            //通知玩家匹配失败
            battleproto::sc_battle_duplicate_enter_map result;
            result.set_dup_id(player->dup_id);
            result.set_map_id(player->map_id);
            result.set_match_result(false);
            player->send_msg_to_player(btl_cmd_notify_match_result, result);
            
            //销毁玩家
            typeof(it) tmp = it;
            tmp++;
            destroy_player(player);
            it = tmp;
            continue;
        }
#endif
        uint32_t sa = player->score;
        uint32_t st1 = 3000.0/ sa * (t+2);
        uint32_t st2 = 3000.0/ sa * t;
        uint32_t low = sa > st1 ?(sa - st1) :0;
        uint32_t high = sa + st2;
        bool matched = false;
        FOREACH(rpvp_index_to_player_map_, it2) {
            if (it == it2) {
                continue;
            }
            dst = it2->second;
            if (dst->score >= low && dst->score <= high) {
                matched = true;
                rpvp_index_to_player_map_.erase(it2);
                rpvp_index_to_player_map_.erase(it++);
                break;
            }
        }

        if (matched) {//告诉btl这两人匹配到了 进入副本战斗
            server_t *svr = SERVER_MGR.get_server_by_name(player->btl_name);
            if (player->btl) {
                player->btl->del_player(player);
            }
            if (dst->btl) {
                dst->btl->del_player(dst);
            }
            svr->add_player(player);
            svr->add_player(dst);
            player->btl = svr;
            dst->btl = svr;
            dst->btl_name = player->btl_name;
            player->btl_start = true;
            dst->btl_start = true;
            battleproto::cs_battle_duplicate_enter_map btl_in;
            btl_in.set_dup_id(player->dup_id);
            btl_in.set_map_id(player->map_id);
            btl_in.set_svr_id(player->svr_id);
            btl_in.mutable_player()->ParseFromString(player->btl_info);
            btl_in.mutable_player()->set_x_pos(850);
            btl_in.mutable_player()->set_y_pos(400);
            commonproto::battle_player_data_t *btlp = btl_in.add_other_players();
            btlp->ParseFromString(dst->btl_info);
            btlp->set_x_pos(1198);
            btlp->set_y_pos(400);
            player->btl->send_msg_to_svr(player->uid, btl_cmd_enter_duplicate, btl_in);
            continue;
        } else {
            it++;
        }
    }
}

void player_manager_t::do_ppve_match()
{
    player_t *player = 0;

    FOREACH(ppve_players_map_, svrit) {
        dupid_to_players_map_t &dup_map = svrit->second;
        //DEBUG_TLOG("SVR:%u", svrit->first);

        FOREACH(dup_map, dupit) {
            //DEBUG_TLOG("DUP:%u", dupit->first);
            id_to_player_map_t &index_map = dupit->second;

            while (index_map.size()) {
                typeof(index_map.begin()) idit = index_map.begin();
                player = idit->second;
                //DEBUG_TLOG("P: index:%u id:%u size:%u", idit->first, player->uid, index_map.size());

                uint32_t t = NOW() - player->request_tm;
                if (t > MATCH_TIME) {
                    //通知玩家匹配失败
                    battleproto::sc_battle_duplicate_enter_map result;
                    result.set_dup_id(player->dup_id);
                    result.set_map_id(player->map_id);
                    result.set_match_result(false);
                    player->send_msg_to_player(btl_cmd_notify_match_result, result);
                    //销毁玩家
                    destroy_player(player);
                    continue;
                }
                uint32_t need_players = player->need_team_members;
                if (index_map.size() < need_players) {
                    break;
                }
                std::vector<player_t*> p_vec;
                p_vec.push_back(player);
                index_map.erase(idit++);
                for (uint32_t i = 0; i < need_players-1 && index_map.size(); i++) {
                    p_vec.push_back(idit->second);
                    index_map.erase(idit++);
                }

                server_t *svr = SERVER_MGR.get_server_by_name(player->btl_name);
                battleproto::cs_battle_duplicate_enter_map btl_in;
                btl_in.set_dup_id(player->dup_id);
                btl_in.set_map_id(player->map_id);
                btl_in.set_svr_id(player->svr_id);
                FOREACH(p_vec, itp) {
                    player_t *dst = *itp;
                    if (dst->btl && dst->btl != svr) {
                        dst->btl->del_player(dst);
                    }
                    svr->add_player(dst);
                    dst->btl = svr;
                    dst->btl_name = player->btl_name;
                    dst->btl_start = true;
                    if (dst == player) {
                        btl_in.mutable_player()->ParseFromString(player->btl_info);
                    } else {
                        commonproto::battle_player_data_t *btlp = btl_in.add_other_players();
                        btlp->ParseFromString(dst->btl_info);
                    }
                    DEBUG_TLOG("PPVE MATCHED: DUP:%u MAP:%u Svr:%u UID:%u", 
                            dst->dup_id, dst->map_id, dst->svr_id, dst->uid);
                }
                player->btl->send_msg_to_svr(player->uid, btl_cmd_enter_duplicate, btl_in);
            }
        }
    }
}
