#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libtaomee++/utils/strings.hpp>
#include <dbser/benchapi.h>
#include <dbser/mysql_iface.h>
#include <dbser/db_macro.h>

#include "proto/common/svr_proto_header.h"

#include "server.h"
#include "cmd_procs.h"
#include "proto/db/db_cmd.h"
#include "proto_processor.h"
#include "base_info_table.h"
#include "item_table.h"
#include "task_info_table.h"
#include "pet_table.h"
#include "nick_table.h"
#include "attr_table.h"
#include "user_action_log_table.h"
#include "rune_table.h"
#include "mail_table.h"
#include "transaction_table.h"
#include "friend_table.h"
#include "hm_visit_log_table.h"
#include "family_id_table.h"
#include "family_info_table.h"
#include "family_member_table.h"
#include "rawdata_table.h"
#include "family_event_table.h"
#include "family_log_table.h"
#include "family_match_info_table.h"
#include "global_attr_table.h"
#include "clisoft_table.h"
#include "gift_code_table.h"
#include "vip_op_trans_table.h"
#include "vip_user_info_table.h"
#include "achieve_table.h"
#include "mine_table.h"

ProtoProcessor *processor;

mysql_interface *g_db = NULL;
server_config_t g_config;
BaseInfoTable* g_base_info_table = NULL;
AttrTable *g_attr_table = NULL;
ItemTable *g_item_table = NULL;
TaskInfoTable* g_task_table = NULL;
PetTable  *g_pet_table = NULL;
NickTable *g_nick_table = NULL;
UserActionLogTable *g_user_action_log_table = NULL;
RuneTable *g_rune_table = NULL;
MailTable *g_mail_table = NULL;
TransactionTable *g_transaction_table = NULL;
FriendTable *g_friend_table = NULL;
HmVisitLogTable *g_hm_visit_table = NULL;
AchieveTable *g_achieve_table = NULL;

FamilyIdTable *g_family_id_table = NULL;
FamilyInfoTable *g_family_info_table = NULL;
FamilyMemberTable *g_family_member_table = NULL;
RawDataTable *g_raw_data_table = NULL;
FamilyEventTable *g_family_event_table = NULL;
FamilyLogTable *g_family_log_table = NULL;
FamilyMatchInfoTable *g_family_match_info_table = NULL;

GlobalAttrTable* g_global_attr_table = NULL;
CliSoftTable *g_clisoftv_table = NULL;
GiftCodeTable *g_gift_code_table = NULL;

VipOpTransTable* g_vip_op_trans_table = NULL;

VipUserInfoTable* g_vip_user_info_table = NULL;

MineInfoTable *g_mine_info_table = NULL;

void pb_log_handler(google::protobuf::LogLevel level,
        const char *filename, int line, const std::string &message)
{
    static const char *level_names[] = {"INFO", "WARNING", "ERROR", "FATAL" };
    ERROR_LOG("[%s %s:%d] %s",
            level_names[level], filename, line, message.c_str());
    DEBUG_LOG("[%s %s:%d] %s",
            level_names[level], filename, line, message.c_str());
}

#define CONFIG_READ_INTVAL(data, name) \
    do { \
        int ret = -1; \
        ret = config_get_intval(#name, ret); \
        if (ret == -1) { \
            ERROR_LOG("not find config '%s'", #name); \
            return -1; \
        } \
        data.name = ret; \
        DEBUG_LOG("set config '%s' = %d", \
#name, data.name); \
    } while (0);

#define CONFIG_READ_STRVAL(data, name) \
    do { \
        const char *conf_str = NULL; \
        conf_str = config_get_strval(#name); \
        if (conf_str == NULL) { \
            ERROR_LOG("not find config '%s'", #name); \
            return -1; \
        } \
        strncpy(data.name, conf_str, sizeof(data.name) - 1); \
        DEBUG_LOG("set config '%s' = '%s'",  \
#name, data.name); \
    } while (0);

int init_tables(mysql_interface* db)
{
    if (db == NULL) {
        return -1;    
    }

    g_base_info_table = new BaseInfoTable(db);
    g_item_table = new ItemTable(db);
    g_pet_table = new PetTable(db);
    g_nick_table = new NickTable(db);
    g_attr_table = new AttrTable(db);
	g_task_table = new TaskInfoTable(db);
	g_rune_table = new RuneTable(db);
    g_user_action_log_table = new UserActionLogTable(db);
	g_mail_table = new MailTable(db);
    g_transaction_table = new TransactionTable(db);
	g_friend_table = new FriendTable(db);
	g_hm_visit_table = new HmVisitLogTable(db);
	g_achieve_table = new AchieveTable(db);
    g_family_id_table = new FamilyIdTable(db);
    g_family_info_table = new FamilyInfoTable(db);
    g_family_member_table = new FamilyMemberTable(db);
    g_family_event_table = new FamilyEventTable(db);
    g_family_log_table = new FamilyLogTable(db);
	g_raw_data_table = new RawDataTable(db);
    g_family_match_info_table = new FamilyMatchInfoTable(db);
    g_global_attr_table = new GlobalAttrTable(db);
    g_clisoftv_table = new CliSoftTable(db);
    g_gift_code_table = new GiftCodeTable(db);
	g_vip_op_trans_table = new VipOpTransTable(db);
	g_vip_user_info_table = new VipUserInfoTable(db);

	g_mine_info_table = new MineInfoTable(db);

    return 0;
}

int init_cmd_procs()
{
    processor->register_cmd(db_cmd_create_role, new CreateRoleCmdProcessor()); 
    processor->register_cmd(db_cmd_get_base_info,new GetBaseInfoCmdProcessor());
    processor->register_cmd(db_cmd_get_cache_info, new GetCacheInfoCmdProcessor());
    processor->register_cmd(db_cmd_change_nick, new ChangeNickCmdProcessor());
    processor->register_cmd(db_cmd_get_login_info, new GetLoginInfoCmdProcessor());

    processor->register_cmd(db_cmd_get_userid_by_nick,new GetUserByNickCmdProcessor());
    processor->register_cmd(db_cmd_check_user_exist, new CheckUserExistCmdProcessor());
    processor->register_cmd(db_cmd_insert_nick_and_userid,new InsertNickAndUserCmdProcessor());
    processor->register_cmd(db_cmd_delete_nick_and_userid,new DeleteNickAndUserCmdProcessor());

    processor->register_cmd(db_cmd_set_attr,new SetAttrCmdProcessor());    
    processor->register_cmd(db_cmd_get_attr,new GetAttrCmdProcessor());    
    processor->register_cmd(db_cmd_del_attr,new DelAttrCmdProcessor());    
    processor->register_cmd(db_cmd_get_ranged_attr,new RangeGetAttrCmdProcessor());    
    processor->register_cmd(db_cmd_clear_ranged_attr,new RangeClearAttrCmdProcessor());    
    processor->register_cmd(db_cmd_change_attr_value,new ChangeAttrCmdProcessor());
    processor->register_cmd(db_cmd_get_login_tm_info, new GetLoginTmInfoCmdProcessor());

    processor->register_cmd(db_cmd_change_items, new ItemChangeCmdProcessor());
    processor->register_cmd(db_cmd_pet_save, new PetSaveCmdProcessor());
    processor->register_cmd(db_cmd_pet_delete,new PetDeleteCmdProcessor());
    processor->register_cmd(db_cmd_pet_list_get, new PetListGetCmdProcessor());

	processor->register_cmd(db_cmd_task_save,new TaskSaveCmdProcessor());
	processor->register_cmd(db_cmd_task_del,new TaskDelCmdProcessor());
    processor->register_cmd(db_cmd_new_transaction, new NewTransactionCmdProcessor());
    processor->register_cmd(db_cmd_get_transaction_list, new GetTransactionListCmdProcessor());
	//拉取玩家交易明细，供客服使用
    processor->register_cmd(db_cmd_get_buy_pd_trans_list, new GetBuyPdTransListCmdProcessor());
	processor->register_cmd(db_cmd_save_rune, new RuneSaveCmdProcessor());
	processor->register_cmd(db_cmd_del_rune, new RuneDelCmdProcessor());

    processor->register_cmd(db_cmd_insert_user_action_log, new InsertActionLogCmdProcessor());
    processor->register_cmd(db_cmd_get_user_action_log, new GetActionLogCmdProcessor());

    processor->register_cmd(db_cmd_new_vip_op_trans, new VipOpTransCmdProcessor());
    processor->register_cmd(db_cmd_new_vip_user_info, new VipUserInfoCmdProcessor());

    processor->register_cmd(db_cmd_mail_new, new MailNewCmdProcessor());
	processor->register_cmd(db_cmd_mail_get_all, new MailGetAllCmdProcessor());
	processor->register_cmd(db_cmd_mail_del_by_ids, new MailDelByIdsCmdProcessor());
	processor->register_cmd(db_cmd_mail_get_by_ids, new MailGetByIdsCmdProcessor());
	processor->register_cmd(db_cmd_mail_set_status, new MailSetStatusCmdProcessor());

	//小屋相关
	processor->register_cmd(db_cmd_get_home_info, new HomeGetInfoCmdProcessor());
	processor->register_cmd(db_cmd_hm_add_visit_log, new HmAddVisitCmdProcessor());
	//processor->register_cmd(db_cmd_update_gift_count, new UpdateGiftCountCmdProcessor());
	processor->register_cmd(db_cmd_get_hm_visit_log, new GetHmVisitLogCmdProcessor());
	//db_cmd_save_achieves 	SaveAchievesCmdProcessor

	//processor->register_cmd(db_cmd_check_user_exist_by_id, new CheckUserExistByIdCmdProcessor());
	
	processor->register_cmd(db_cmd_save_friend, new SaveFriendCmdProcessor());
	processor->register_cmd(db_cmd_remove_friend, new RemoveFriendCmdProcessor());
	processor->register_cmd(db_cmd_show_item, new ShowItemCmdProcessor());
	processor->register_cmd(db_cmd_show_pet, new ShowPetCmdProcessor());


	processor->register_cmd(db_cmd_family_create, new FamilyCreateCmdProcessor());
	processor->register_cmd(db_cmd_family_update_info, new FamilyUpdateInfoCmdProcessor());
	processor->register_cmd(db_cmd_family_get_info, new FamilyGetInfoCmdProcessor());
	processor->register_cmd(db_cmd_family_get_member_info, new FamilyGetMemberInfoCmdProcessor());
	processor->register_cmd(db_cmd_family_get_member_list, new FamilyGetMemberListCmdProcessor());
	processor->register_cmd(db_cmd_family_dismiss_family, new FamilyDismissFamilyCmdProcessor());
	processor->register_cmd(db_cmd_family_update_member_info, new FamilyUpdateMemberInfoCmdProcessor());
	processor->register_cmd(db_cmd_family_quit, new FamilyQuitCmdProcessor());
	processor->register_cmd(db_cmd_family_change_info, new FamilyChangeInfoCmdProcessor());
	processor->register_cmd(db_cmd_family_get_event_info, new FamilyGetEventInfoCmdProcessor());
	processor->register_cmd(db_cmd_family_update_event, new FamilyUpdateEventCmdProcessor());
	processor->register_cmd(db_cmd_family_del_event, new FamilyDelEventCmdProcessor());
	processor->register_cmd(db_cmd_family_update_log, new FamilyUpdateLogCmdProcessor());
	processor->register_cmd(db_cmd_family_get_log_list, new FamilyGetLogListCmdProcessor());
	processor->register_cmd(db_cmd_family_change_member_info, new FamilyChangeMemberInfoCmdProcessor());
	processor->register_cmd(db_cmd_family_get_next_leader, new FamilyGetNextLeaderCmdProcessor());
	processor->register_cmd(db_cmd_family_get_event_list, new FamilyGetEventListCmdProcessor());
	processor->register_cmd(db_cmd_family_get_recommend_list, new FamilyGetRecommendListCmdProcessor());
	processor->register_cmd(db_cmd_family_update_match_info, new FamilyUpdateMatchInfoCmdProcessor());
	processor->register_cmd(db_cmd_family_change_match_info, new FamilyChangeMatchInfoCmdProcessor());
	processor->register_cmd(db_cmd_family_del_match_info, new FamilyDelMatchInfoCmdProcessor());

	//processor->register_cmd(db_cmd_user_buff_get, new UserBuffGetCmdProcessor());
	processor->register_cmd(db_cmd_user_raw_data_get, new UserRawDataGetCmdProcessor());
	processor->register_cmd(db_cmd_user_raw_data_update, new UserRawDataUpdateCmdProcessor());
	processor->register_cmd(db_cmd_user_raw_data_del, new UserRawDataDelCmdProcessor());
	//全服属性
	processor->register_cmd(db_cmd_get_global_attr, new GetGlobalAttrCmdProcessor());
	processor->register_cmd(db_cmd_update_global_attr, new UpdateGlobalAttrCmdProcessor());

	processor->register_cmd(db_cmd_set_clisoftv, new SetCliSoftvCmdProcessor());
    processor->register_cmd(db_cmd_use_gift_code, new UseGiftCodeCmdProcessor());

    processor->register_cmd(db_cmd_search_occupyed_mine, new SearchOccupyedMineCmdProcessor());
    processor->register_cmd(db_cmd_save_one_new_mine, new SaveNewMineCmdProcessor());
    processor->register_cmd(db_cmd_del_mine_info, new DelMineInfoCmdProcessor());

    processor->register_cmd(db_cmd_get_player_mine_info, new GetPlayerMineInfoCmdProcessor());
    processor->register_cmd(db_cmd_update_mine_info, new UpdateMineCmdProcessor());

    processor->register_cmd(db_cmd_increment_defender_cnt, new IncreDefCntCmdProcessor());

    return 0;
}

int handle_init(
        int argc, 
        char **argv, 
        int proc_type)
{
#ifdef ENABLE_TRACE_LOG
#ifdef USE_TLOG
    SET_LOG_LEVEL(tlog_lvl_trace);
    SET_TIME_SLICE_SECS(86400);
#endif
#endif

    if (proc_type == PROC_WORK) {

        SetLogHandler(pb_log_handler);

        processor = new ProtoProcessor();

        CONFIG_READ_STRVAL(g_config, mysql_host);
        CONFIG_READ_STRVAL(g_config, mysql_user);
        CONFIG_READ_STRVAL(g_config, mysql_passwd);
        CONFIG_READ_STRVAL(g_config, mysql_charset);
        CONFIG_READ_INTVAL(g_config, mysql_port);

        // 初始化db连接
        g_db = new mysql_interface(
                g_config.mysql_host,
                g_config.mysql_user,
                g_config.mysql_passwd, 
                g_config.mysql_port,
                NULL, 
                g_config.mysql_charset);

        if (!g_db->init_ok) {
            ERROR_LOG("init db failed"); 
            return -1;
        }

        g_db->set_is_log_debug_sql(1);
        g_db->set_is_only_exec_select_sql(0);

        //char sql_buf[512];
        int ret = 0;
        //int affected_rows = 0;

        // 设置字符集
        ret = mysql_set_character_set(&g_db->handle, g_config.mysql_charset);
        if (ret != 0) {
            ERROR_LOG("ste charset '%s' failed", g_config.mysql_charset); 
            return -1;
        }

        // 设置默认是自动提交事务
        //ret = mysql_autocommit(&g_db->handle, 1);
        //if (ret != 0) {
            //ERROR_LOG("set autocommit true failed\n"); 
            //return -1;
        //}

        // 初始化表
        ret = init_tables(g_db);
        if (ret != 0) {
            ERROR_LOG("init tables failed"); 
            return -1;
        }

        // 初始化协议函数
        ret = init_cmd_procs();
        if (ret != 0) {
            ERROR_LOG("init cmd procs failed");        
            return -1;
        }

        return 0;
    } else if (proc_type == PROC_CONN) {
        processor = new ProtoProcessor();
        return 0;
    }

    return 0;
}

int handle_input(
        const char* recv_buf, 
        int recv_len, 
        const skinfo_t* skinfo)
{
    return processor->get_pkg_len(recv_buf, recv_len);
}

int handle_process(
        char *recv, 
        int recv_len, 
        char** send_buf, 
        int* send_len, 
        const skinfo_t*)
{
    //bin2hex(hex_buf, recv, recv_len, sizeof(hex_buf));
    //DEBUG_LOG("I %d[%s]", recv_len, hex_buf);

    int ret = processor->process(recv, recv_len, send_buf, send_len);

    //bin2hex(hex_buf, *send_buf, *send_len, sizeof(hex_buf));
    //DEBUG_LOG("O[%s]", hex_buf);

    return ret;
}

void handle_fini(
        int proc_type)
{
    if (proc_type == PROC_WORK) {
        delete processor; 
    } else if (proc_type == PROC_CONN) {
        delete processor; 
    }
}

int handle_filter_key(const char *buf, int len, uint32_t *key)
{
     if (config_get_intval("dont_use_filter_key", 0) == 1) {
         return -1;
     }
     svr_proto_header_t *header = (svr_proto_header_t *)buf;
     *key = header->uid;
     return 0;
}
