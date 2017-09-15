
#ifndef USER_INFO_PROCS_H
#define USER_INFO_PROCS_H

#include "cmd_processor_interface.h"
#include "proto/db/dbproto.login.pb.h"
#include "proto/db/dbproto.base_info.pb.h"
#include "proto/db/dbproto.item.pb.h"
#include "proto/db/dbproto.task.pb.h"
#include "proto/db/dbproto.pet.pb.h"
#include "proto/db/dbproto.attr.pb.h"
#include "proto/db/dbproto.nick.pb.h"
#include "proto/db/dbproto.user_action_log.pb.h"
#include "proto/db/dbproto.rune.pb.h"
#include "proto/db/dbproto.mail.pb.h"
#include "proto/db/dbproto.home.pb.h"
#include "proto/db/dbproto.transaction.pb.h"
#include "proto/db/dbproto.friend.pb.h"
#include "proto/db/dbproto.chat.pb.h"
#include "proto/db/dbproto.family.pb.h"
#include "proto/db/dbproto.raw_data.pb.h"
#include "proto/db/dbproto.clisoft.pb.h"
#include "proto/db/dbproto.gift_code.pb.h"
#include "proto/db/dbproto.achieve.pb.h"
#include "proto/db/dbproto.mine.pb.h"

#define PARSE_DB_MSG(pbuf) \
    do { \
        pbuf.Clear();\
        if (!pbuf.ParseFromString(req_body)) { \
            return db_err_proto_format_err;  \
        } \
    } while (0)                                                                                                                    

#define Processor(ClassNamePrefix, arg)                                 \
    class ClassNamePrefix##CmdProcessor : public CmdProcessorInterface  \
    {                                                                   \
    public:                                                             \
        uint32_t process(userid_t userid, uint32_t u_create_tm,         \
                const std::string& req_body, std::string& ack_body);    \
    private:                                                            \
    dbproto::cs_##arg cs_##arg##_;                                      \
    dbproto::sc_##arg sc_##arg##_;                                      \
    };

//获取缓存需要的玩家信息
//cache_info
Processor(GetCacheInfo, get_cache_info)

//global_attr
Processor(GetGlobalAttr, get_global_attr)
Processor(UpdateGlobalAttr,update_global_attr)

//base_info
Processor(CreateRole, create_role)
Processor(GetBaseInfo, get_base_info)
Processor(GetLoginInfo, get_login_info)
Processor(ChangeNick, change_nick)

//check user
Processor(GetUserByNick, get_user_by_nick)
Processor(CheckUserExist, check_user_exist)
Processor(InsertNickAndUser, insert_nick_and_user)
Processor(DeleteNickAndUser, delete_nick_and_user)

//attrs
Processor(SetAttr, set_attr)
Processor(SetAttrOnce, set_attr_once)
Processor(GetAttr, get_attr)
Processor(DelAttr, del_attr)
Processor(RangeGetAttr, get_ranged_attr)
Processor(RangeClearAttr, clear_ranged_attr)
Processor(ChangeAttr, change_attr_value)


Processor(ItemChange, change_items)
Processor(PetSave, pet_save)
Processor(PetDelete, pet_del_info)
Processor(PetListGet, pet_list_get_info)

//log
Processor(InsertActionLog, insert_user_action_log)
Processor(GetActionLog, get_user_action_log)

//task
Processor(TaskSave, task_save)
Processor(TaskDel, task_del)

//rune
Processor(RuneSave, rune_save);
Processor(RuneDel, rune_del);

//mail
Processor(MailNew, mail_new);
Processor(MailGetAll, mail_get_all);
Processor(MailDelByIds, mail_del_by_ids);
Processor(MailGetByIds, mail_get_by_ids);
Processor(MailSetStatus, mail_set_status);

//transaction
Processor(NewTransaction, new_transaction);
Processor(GetTransactionList, get_transaction_list);
Processor(GetBuyPdTransList, get_buy_pd_trans_list);
Processor(VipOpTrans, new_vip_op_trans);
Processor(VipUserInfo, new_vip_user_info);

//Processor(CheckUserExistById, check_user_exist_by_id);
//Processor(GetUseridByNick, get_userid_by_nick);
Processor(SaveFriend, save_friend);
Processor(RemoveFriend, remove_friend);
Processor(ShowItem, show_item);
Processor(ShowPet, show_pet);

//family
Processor(FamilyCreate, family_create);
Processor(FamilyGetInfo, family_get_info);
Processor(FamilyUpdateInfo, family_update_info);
Processor(FamilyChangeInfo, family_change_info);
Processor(FamilyGetMemberInfo, family_get_member_info);
Processor(FamilyUpdateMemberInfo, family_update_member_info);
Processor(FamilyChangeMemberInfo, family_change_member_info);
Processor(FamilyDismissFamily, family_dismiss_family);
Processor(FamilyQuit, family_quit);
Processor(FamilyGetMemberList, family_get_member_list);
Processor(FamilyUpdateEvent, family_update_event);
Processor(FamilyDelEvent, family_del_event);
Processor(FamilyGetEventInfo, family_get_event_info);
Processor(FamilyGetEventList, family_get_event_list);
Processor(FamilyUpdateLog, family_update_log);
Processor(FamilyGetLogList, family_get_log_list);
Processor(FamilyGetNextLeader, family_get_next_leader);
Processor(FamilyGetRecommendList, family_get_recommend_list);
Processor(FamilyUpdateMatchInfo, family_update_match_info);
Processor(FamilyChangeMatchInfo, family_change_match_info);
Processor(FamilyDelMatchInfo, family_del_match_info);

Processor(HomeGetInfo, get_home_info);
Processor(HmAddVisit, add_visit_log);
Processor(GetHmVisitLog, get_visit_log);

//achieve
Processor(SaveAchieves, save_achieves);

//user buff
Processor(UserRawDataGet, user_raw_data_get);
Processor(UserRawDataUpdate, user_raw_data_update);
Processor(UserRawDataDel, user_raw_data_del);

Processor(SetCliSoftv, set_client_soft);
Processor(UseGiftCode, use_gift_code);
Processor(GetLoginTmInfo, get_login_tm_info);

Processor(SearchOccupyedMine, get_mine_info);
Processor(SaveNewMine, save_mine_info);
Processor(GetPlayerMineInfo, get_player_mine_info);
Processor(UpdateMine, update_mine_info);
Processor(DelMineInfo, del_mine_info);
Processor(IncreDefCnt, increment_defender_cnt);

#endif
