#ifndef __FAMILY_PROCESSOR__
#define __FAMILY_PROCESSOR__

#include "common.h"
#include "cmd_processor_interface.h"

class FamilyCreateCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

    struct family_create_session_t
    {
        uint32_t family_id;
    };
private:
    int proc_pkg_from_serv_aft_set_is_member(
        player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_create_family(
            player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_update_family_info(
            player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0701_family_create    cli_in_;
    onlineproto::sc_0x0701_family_create    cli_out_;

    commonproto::family_info_t family_info;
    dbproto::cs_family_create   db_create_family_in_;
    dbproto::sc_family_create   db_create_family_out_;
    dbproto::cs_family_update_info   db_update_family_info_in_;
    dbproto::sc_family_update_info   db_update_family_info_out_;
    rankproto::cs_set_is_member     rank_set_is_member_in_;
    rankproto::sc_set_is_member     rank_set_is_member_out_;
};

class FamilyDismissCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
    
private:
    int proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_family_get_info(
            player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_check_session(
        player_t* player, const char* body, int bodylen);

    onlineproto::cs_0x0702_family_dismiss   cli_in_;
    onlineproto::sc_0x0702_family_dismiss   cli_out_;
    dbproto::cs_family_get_info     db_get_info_in_;
    dbproto::sc_family_get_info     db_get_info_out_;
    rankproto::cs_string_insert     rank_lock_acquire_in_;
    rankproto::sc_string_insert     rank_lock_acquire_out_;
    std::string  lock_key_;
   char m_send_buf_[4096]; 
};

class FamilyGetInfoCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_family_get_info(
            player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_get_cache_info(
        player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_family_rank_hset_get_field_info(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0707_family_get_info      cli_in_;
    onlineproto::sc_0x0707_family_get_info      cli_out_;
    dbproto::cs_family_get_info     db_get_info_in_;
    dbproto::sc_family_get_info     db_get_info_out_;
    rankproto::cs_hset_get_field_info     rank_get_field_info_in_;
    rankproto::sc_hset_get_field_info     rank_get_field_info_out_;
    cacheproto::cs_batch_get_users_info     cache_info_in_;
    cacheproto::sc_batch_get_users_info     cache_info_out_;
};

class FamilyGetMemberInfoCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_family_get_member_info(
            player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_get_cache_info(
        player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_get_user_ol_info(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0708_family_get_member_info      cli_in_;
    onlineproto::sc_0x0708_family_get_member_info      cli_out_;
    dbproto::cs_family_get_member_info     db_get_member_info_in_;
    dbproto::sc_family_get_member_info     db_get_member_info_out_;
    cacheproto::cs_batch_get_users_info     cache_info_in_;
    cacheproto::sc_batch_get_users_info     cache_info_out_;
	switchproto::cs_sw_is_online cs_sw_is_online_in_;
	switchproto::sc_sw_is_online sc_sw_is_online_out_;
};

class FamilyGetMemberListCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_get_member_list(
            player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_get_cache_info(
            player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_get_user_ol_info(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0710_family_get_member_list      cli_in_;
    onlineproto::sc_0x0710_family_get_member_list      cli_out_;
    dbproto::cs_family_get_member_list     db_get_member_list_in_;
    dbproto::sc_family_get_member_list     db_get_member_list_out_;
    cacheproto::cs_batch_get_users_info     cache_info_in_;
    cacheproto::sc_batch_get_users_info     cache_info_out_;
	switchproto::cs_sw_is_online cs_sw_is_online_in_;
	switchproto::sc_sw_is_online sc_sw_is_online_out_;
};

class FamilyApplyCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen);
    onlineproto::cs_0x0703_family_apply      cli_in_;
    onlineproto::sc_0x0703_family_apply      cli_out_;
    dbproto::cs_family_get_info     db_get_family_info_in_;
    dbproto::sc_family_get_info     db_get_family_info_out_;
    dbproto::cs_family_update_member_info      db_family_update_member_info_in_;
    dbproto::sc_family_update_member_info      db_family_update_member_info_out_;
    dbproto::cs_family_change_info          db_change_family_info_in_;
    dbproto::sc_family_change_info          db_change_family_info_out_;
    dbproto::cs_family_update_event          db_family_update_event_in_;
    dbproto::sc_family_update_event          db_family_update_event_out_;

    rankproto::cs_string_insert     rank_lock_acquire_in_;
    rankproto::sc_string_insert     rank_lock_acquire_out_;
    std::string  lock_key_;
};

class FamilyQuitCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:

    int proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_family_get_next_leader(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0704_family_quit      cli_in_;
    onlineproto::sc_0x0704_family_quit      cli_out_;
    dbproto::cs_family_get_info     db_get_family_info_in_;
    dbproto::sc_family_get_info     db_get_family_info_out_;
    dbproto::cs_family_get_next_leader     db_get_next_leader_in_;
    dbproto::sc_family_get_next_leader     db_get_next_leader_out_;

    rankproto::cs_string_insert     rank_lock_acquire_in_;
    rankproto::sc_string_insert     rank_lock_acquire_out_;
    std::string  lock_key_;
};

class FamilyRecommendListCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    /*int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);*/

private:
};

class FamilyLeaderReassignCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    /*int proc_pkg_from_serv_aft_send_switch_msg(*/
    /*player_t *player, const char *body, int bodylen);*/

    int proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0705_family_leader_reassign      cli_in_;
    onlineproto::sc_0x0705_family_leader_reassign      cli_out_;
    dbproto::cs_family_get_info     db_get_family_info_in_;
    dbproto::sc_family_get_info     db_get_family_info_out_;
    rankproto::cs_string_insert     rank_lock_acquire_in_;
    rankproto::sc_string_insert     rank_lock_acquire_out_;
    std::string  lock_key_;
    bool is_online;
};

class FamilySetMemberTitleCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

    struct family_set_member_title_session_t
    {
        role_info_t role;
        uint32_t title;
    };

private:
    int proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_family_get_member_info(
            player_t *player, const char *body, int bodylen);

    int check_set_title_right(
        commonproto::family_member_info_t *setter,
        commonproto::family_member_info_t *setted, uint32_t title);

    int proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0706_family_set_member_title      cli_in_;
    onlineproto::sc_0x0706_family_set_member_title      cli_out_;
    dbproto::cs_family_get_info     db_get_family_info_in_;
    dbproto::sc_family_get_info     db_get_family_info_out_;
    dbproto::cs_family_get_member_info     db_get_member_info_in_;
    dbproto::sc_family_get_member_info     db_get_member_info_out_;
    rankproto::cs_string_insert     rank_lock_acquire_in_;
    rankproto::sc_string_insert     rank_lock_acquire_out_;
    std::string  lock_key_;
};

class FamilyConstructCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0709_family_contribute      cli_in_;
    onlineproto::sc_0x0709_family_contribute      cli_out_;
    dbproto::cs_family_change_info db_change_info_in_;
    dbproto::cs_family_change_member_info db_change_member_info_in_;
    dbproto::cs_family_update_info   db_update_family_info_in_;
    dbproto::cs_family_get_info     db_get_family_info_in_;
    dbproto::sc_family_get_info     db_get_family_info_out_;
};

class FamilyUpdateMsgInfoCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_family_get_member_info(
            player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0711_family_update_msg_info      cli_in_;
    onlineproto::sc_0x0711_family_update_msg_info      cli_out_;
    dbproto::cs_family_get_member_info     db_get_member_info_in_;
    dbproto::sc_family_get_member_info     db_get_member_info_out_;
};

class FamilyMsgDelCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

private:
    onlineproto::cs_0x0721_family_msg_del      cli_in_;
    onlineproto::sc_0x0721_family_msg_del      cli_out_;
    dbproto::cs_user_raw_data_del     db_user_raw_data_del_in_;
    dbproto::sc_user_raw_data_del     db_user_raw_data_del_out_;
};

class RequireRandomNameCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    /*int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);*/

private:
    onlineproto::cs_0x0714_require_random_name      cli_in_;
    onlineproto::sc_0x0714_require_random_name      cli_out_;

};

class FamilyInviteCmdProcessor : public CmdProcessorInterface {
public:
    struct family_invite_session_t
    {
        uint32_t userid;
        uint32_t u_create_tm;
    };
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
    
private:
    int proc_pkg_from_serv_aft_get_userid(
        player_t *player, const char *body, int bodylen);
    int send_family_invite_notice(player_t *player, uint32_t userid);

    int proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0715_family_invite      cli_in_;
    onlineproto::sc_0x0715_family_invite      cli_out_;
    dbproto::cs_get_user_by_nick                 cs_get_user_by_nick_in_;
    dbproto::sc_get_user_by_nick                 sc_get_user_by_nick_out_;
    dbproto::cs_family_get_info     db_get_family_info_in_;
    dbproto::sc_family_get_info     db_get_family_info_out_;
    dbproto::cs_family_update_event          db_family_update_event_in_;
    dbproto::sc_family_update_event          db_family_update_event_out_;
};

class FamilyDealInviteCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen);

    int proc_pkg_from_serv_aft_family_get_event(
            player_t *player, const char *body, int bodylen);

    int proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0713_family_deal_invite      cli_in_;
    onlineproto::sc_0x0713_family_deal_invite      cli_out_;
    dbproto::cs_family_get_event_info   db_get_family_event_in_;
    dbproto::sc_family_get_event_info   db_get_family_event_out_;
    dbproto::cs_family_get_info     db_get_family_info_in_;
    dbproto::sc_family_get_info     db_get_family_info_out_;
    rankproto::cs_string_insert     rank_lock_acquire_in_;
    rankproto::sc_string_insert     rank_lock_acquire_out_;
    std::string  lock_key_;
};

class FamilyEventDealCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen);

    int proc_pkg_from_serv_aft_family_get_event(
            player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_family_get_info(
            player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_get_attr_info(
            player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_family_get_member_info(
            player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0717_family_event_deal      cli_in_;
    onlineproto::sc_0x0717_family_event_deal      cli_out_;
    dbproto::cs_family_get_event_info   db_get_family_event_in_;
    dbproto::sc_family_get_event_info   db_get_family_event_out_;
    dbproto::cs_family_get_info     db_get_family_info_in_;
    dbproto::sc_family_get_info     db_get_family_info_out_;
    dbproto::cs_family_get_member_info     db_get_member_info_in_;
    dbproto::sc_family_get_member_info     db_get_member_info_out_;
    dbproto::cs_get_attr         attr_info_in_;
    dbproto::sc_get_attr         attr_info_out_;
    rankproto::cs_string_insert     rank_lock_acquire_in_;
    rankproto::sc_string_insert     rank_lock_acquire_out_;
    std::string  lock_key_;
};

class FamilyGetEventListCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_get_event_list(
            player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_get_cache_info(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0716_family_get_event_list      cli_in_;
    onlineproto::sc_0x0716_family_get_event_list      cli_out_;
    dbproto::cs_family_get_event_list     db_get_event_list_in_;
    dbproto::sc_family_get_event_list     db_get_event_list_out_;
    cacheproto::cs_batch_get_users_info     cache_info_in_;
    cacheproto::sc_batch_get_users_info     cache_info_out_;
};

class FamilyGetLogListCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_get_log_list(
        player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_get_cache_info(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x071E_family_get_log_list      cli_in_;
    onlineproto::sc_0x071E_family_get_log_list      cli_out_;
    dbproto::cs_family_get_log_list     db_get_log_list_in_;
    dbproto::sc_family_get_log_list     db_get_log_list_out_;
    cacheproto::cs_batch_get_users_info     cache_info_in_;
    cacheproto::sc_batch_get_users_info     cache_info_out_;
};

class FamilyGetRecommendListCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_get_recommend_list(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x071F_family_get_recommend_list      cli_in_;
    onlineproto::sc_0x071F_family_get_recommend_list      cli_out_;
    dbproto::cs_family_get_recommend_list     db_get_list_in_;
    dbproto::sc_family_get_recommend_list     db_get_list_out_;
};

class FamilyConfigCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0722_family_config      cli_in_;
    onlineproto::sc_0x0722_family_config      cli_out_;
    dbproto::cs_family_get_info     db_get_family_info_in_;
    dbproto::sc_family_get_info     db_get_family_info_out_;
    dbproto::cs_family_update_info   db_update_family_info_in_;
    dbproto::sc_family_update_info   db_update_family_info_out_;
};

class FamilyLeaderReassignRequestCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_family_get_info(
        player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv_aft_family_get_member_info(
            player_t *player, const char *body, int bodylen);

    onlineproto::cs_0x0723_family_leader_reassign_request    cli_in_;
    onlineproto::sc_0x0723_family_leader_reassign_request    cli_out_;
    dbproto::cs_family_get_info     db_get_info_in_;
    dbproto::sc_family_get_info     db_get_info_out_;
    dbproto::cs_family_get_member_info     db_get_member_info_in_;
    dbproto::sc_family_get_member_info     db_get_member_info_out_;
};

class FamilyCancelApplyCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int proc_pkg_from_serv_aft_lock_acquire(
        player_t *player, const char *body, int bodylen);
    onlineproto::cs_0x0724_family_cancel_apply      cli_in_;
    onlineproto::sc_0x0724_family_cancel_apply      cli_out_;
    rankproto::cs_string_insert     rank_lock_acquire_in_;
    rankproto::sc_string_insert     rank_lock_acquire_out_;
    std::string  lock_key_;
    uint32_t index;
};

class FamilyGetApplyListCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

private:
    onlineproto::cs_0x0712_family_get_apply_list      cli_in_;
    onlineproto::sc_0x0712_family_get_apply_list      cli_out_;
};

class FamilyTechUpCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

private:
    onlineproto::cs_0x0725_family_tech_up       cli_in_;
    onlineproto::sc_0x0725_family_tech_up       cli_out_;
};

class FamilyNameChangeCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	int proc_pkg_from_serv_aft_set_is_member(
			player_t* player, const char* body, int bodylen);

	int proc_pkg_from_serv_aft_get_member_info(
			player_t* player, const char* body, int bodylen);

	int proc_pkg_from_serv_aft_family_get_info(
			player_t* player, const char* body, int bodylen);

	onlineproto::cs_0x0726_change_family_name cli_in_;
	onlineproto::sc_0x0726_change_family_name cli_out_;
    rankproto::sc_set_is_member     rank_set_is_member_out_;
	dbproto::sc_family_get_member_info     db_get_member_info_out_;
	dbproto::sc_family_get_info  db_get_info_out_;
};
#endif // __FAMILY_PROCESSOR__
