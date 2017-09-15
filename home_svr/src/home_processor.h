#ifndef __HOME_PROCESSOR_H__
#define __HOME_PROCESSOR_H__

#include "common.h"
#include "cmd_processor_interface.h"

//进入小屋
class EnterHomeCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
    homeproto::cs_enter_home cli_in_;
    homeproto::sc_enter_home cli_out_;
    dbproto::cs_get_home_info db_in_;
    dbproto::sc_get_home_info db_out_;
};

//离开小屋
class ExitHomeCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen) {
        return 0;
    }
private:
    homeproto::cs_exit_home cli_in_;
	homeproto::sc_exit_home cli_out_;
};

//玩家信息改变同步
class HomeSyncPlayerInfoCmdProcessor: public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen) {
        return 0;
    }
private:
	homeproto::cs_sync_map_player_info cli_in_;
};

//玩家位置信息改变同步
class HomePlayerStateChangeCmdProcessor: public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen) {
        return 0;
    }
private:
	homeproto::cs_change_state cli_in_;
};

// 家族大厅相关协议都不能有回调处理, home_svr的Player管理是限定在小屋玩法用的
//家族大厅信息改变同步
class FamilyHallSyncMapPlayerInfoCmdProcessor: public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen) {
        return 0;
    }
private:
	homeproto::cs_family_hall_sync_map_player_info cli_in_;
	homeproto::sc_family_hall_sync_map_player_info cli_out_;
};

//家族大厅位置信息改变同步
class FamilyHallStateChangeCmdProcessor: public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen) {
        return 0;
    }
private:
	homeproto::cs_family_hall_change_state cli_in_;
	homeproto::sc_family_hall_change_state cli_out_;
};

// 进入家族大厅
class EnterFamilyHallCmdProcessor: public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen) {
        return 0;
    }
private:
	homeproto::cs_enter_family_hall cli_in_;
	homeproto::sc_enter_family_hall cli_out_;
};

// 离开家族大厅
class LeaveFamilyHallCmdProcessor: public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen) {
        return 0;
    }
private:
	homeproto::cs_leave_family_hall cli_in_;
	homeproto::sc_leave_family_hall cli_out_;
};

//生成访客日志
class GenVisitLogCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen) {
        return 0;
    }
private:
	homeproto::cs_gen_home_visit_log cli_in_;
};

class NotipetExerciseChange : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
    int proc_pkg_from_serv(player_t* player, const char* body, int bodylen) {
        return 0;
    }
private:
	homeproto::cs_home_pet_exercise_notify cli_in_;
};

#endif
