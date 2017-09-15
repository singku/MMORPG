#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "common.h"

struct home_data_t {
    home_data_t() {
        home_info_loaded_ = false;
        home_type_ = 0;
        at_home_player_map_.clear();
    }

    //房间信息是否已加载
    bool home_info_loaded_;
    //房型
    uint32_t home_type_;
    //在房间里的玩家
    std::map<uint64_t, player_t*> at_home_player_map_;
};

class player_t {
public:
    player_t() { 
        userid_ = 0;
		u_create_tm_ = 0;
		svr_id_ = 0;
        name_.clear();
        fdsess_ = 0;
        wait_cmd_ = 0;
        wait_serv_cmd_ = 0;
        serv_cmd_ = 0;
        home_data_ = 0;
        at_host_.userid = 0;
        at_host_.u_create_tm = 0;
        x_ = 0;
        y_ = 0;
        has_follow_pet_ = false;
        wait_host_.userid = 0;
        wait_host_.u_create_tm = 0;
        map_player_info_.Clear();
        header_uid_ = 0;
        header_u_create_tm_ = 0;
    }
    ~player_t() {
        if (home_data_) {
            delete home_data_;
            home_data_ = 0;
        }
    }

public:
    int home_broadcast_leave(player_t *p);
    int home_broadcast_enter(player_t *p);
    int home_broadcast(uint16_t cmd, const google::protobuf::Message &msg, player_t *skip=0);

public:
    inline role_info_t role() {
        role_info_t tmp;
        tmp.userid = userid_;
        tmp.u_create_tm = u_create_tm_;
        return tmp;
    }

    inline uint64_t role_key() {
        return comp_u64(userid_, u_create_tm_);
    }

    inline role_info_t wait_host() {
        return wait_host_;
    }
    inline void set_wait_host(role_info_t host) {
        wait_host_ = host;
    }
    inline void set_name(string name) {
        name_ = name;
    }
    inline const string &name() {
        return name_;
    }
    inline void set_fdsess(fdsession_t *fdsess) {
        fdsess_ = fdsess;
    }
    inline const fdsession_t *fdsess() {
        return fdsess_;
    }
    inline void set_userid(uint32_t uid) {
        userid_ = uid;
    }
    inline uint32_t uid() {
        return userid_;
    }
    inline void set_u_create_tm(uint32_t tm) {
        u_create_tm_ = tm;
    }
    inline uint32_t u_create_tm() {
        return u_create_tm_;
    }

    inline uint32_t cur_seq() {
        return cur_seq_;
    }
    inline void set_cur_seq(uint32_t seq) {
        cur_seq_ = seq;
    }
    inline uint32_t header_uid() {
        return header_uid_;
    }
    inline void set_header_uid(uint32_t uid) {
        header_uid_ = uid;
    }
    inline uint32_t header_u_create_tm() {
        return header_u_create_tm_;
    }
    inline void set_header_u_create_tm(uint32_t tm) {
        header_u_create_tm_ = tm;
    }
    inline void set_home_type(uint32_t home_type) {
        if (!home_data_) return;
        home_data_->home_type_ = home_type ? home_type : DEFAULT_HOME_TYPE;
    }
    inline uint32_t home_type() {
        if (!home_data_) return 0;
        return home_data_->home_type_;
    }
    inline void add_visitor(player_t *p) {
        if (!home_data_) return;
        uint64_t key = comp_u64(p->uid(), p->u_create_tm());
        home_data_->at_home_player_map_[key] = p;
        home_broadcast_enter(p);
    }
    inline void del_visitor(player_t *p) {
        if (!home_data_) return;
        home_broadcast_leave(p);
        uint64_t key = comp_u64(p->uid(), p->u_create_tm());
        home_data_->at_home_player_map_.erase(key);
    }
    inline bool has_visitor() {
        if (!home_data_) return false;
        return (!home_data_->at_home_player_map_.empty());
    }
    inline void set_home_info_loaded() {
        if (!home_data_) return;
        home_data_->home_info_loaded_ = true;
    }
    inline bool home_info_loaded() {
        if (!home_data_) return false;
        return home_data_->home_info_loaded_;
    }
    inline void init_home_info() {
        if (home_data_) delete home_data_;
        home_data_ = new home_data_t();
    }
    inline void set_at_host(role_info_t host) {
        at_host_ = host;
    }
    inline role_info_t at_whos_home() {
        return at_host_;
    }
	inline bool has_follow_pet() {
		return has_follow_pet_;
	}

    inline void set_map_player_info(const commonproto::map_player_data_t &info) {
        map_player_info_.CopyFrom(info);
    }
    inline commonproto::map_player_data_t &get_map_player_info() {
        return map_player_info_;
    }

public:
	uint32_t u_create_tm_;	//创建时间
	uint32_t svr_id_;	//所在的服
    uint32_t userid_; // 米米号
    string name_;
    uint32_t sex_;
    uint32_t x_;
    uint32_t y_;
    fdsession_t *fdsess_; //实际上应该是online的session 如果玩家不在线则为0
    uint32_t wait_cmd_; // 客户端请求命令号
    uint32_t wait_serv_cmd_; // 等待服务端的命令号
    uint32_t serv_cmd_; // 请求服务端命令号
    uint32_t cur_seq_; // 当前请求包的seq编号
    uint32_t header_uid_; // 当前请求包的包头uid 回包要对的上号
    uint32_t header_u_create_tm_; // 当前请求包的包头uid 回包要对的上号


    home_data_t *home_data_;
    bool has_follow_pet_;
    //onlineproto::pet_data_t follow_pet_info_; //跟随的精灵的信息
    role_info_t at_host_; //此刻在谁家呆着
    role_info_t wait_host_; //等待谁的房间信息
    string cache_string; //缓存请求包
    commonproto::map_player_data_t map_player_info_; //玩家的所有打包好的protobuf信息
};

// 发送protobuf到online
int send_msg_to_player(player_t* player, uint32_t cmd, const google::protobuf::Message& message, bool finished = true);
//通过sw发送通知消息到online
int send_noti_to_player(player_t *player, uint32_t cmd, const google::protobuf::Message& message);

// 发送错误码到客户端
int send_err_to_player(player_t* player, uint32_t cmd, const uint32_t ret);
int send_err_to_fdsess(const fdsession_t *fdsess, uint32_t uid, uint32_t u_create_tm, 
        uint32_t cb_uid, uint32_t cb_u_create_tm,
        uint32_t cmd, uint32_t seq, uint32_t ret);

#endif
