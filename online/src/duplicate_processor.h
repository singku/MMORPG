#ifndef __DUPLICATE_PROCESSOR_H__
#define __DUPLICATE_PROCESSOR_H__

#include "common.h"
#include "cmd_processor_interface.h"

class EnterDuplicateCmdProcessor : public CmdProcessorInterface {
public:
    EnterDuplicateCmdProcessor() {
        before_enter_func_map_.clear();
        after_enter_func_map_.clear();
        after_pack_func_map_.clear();
        register_proc_func();
    }
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    int before_enter_dup(player_t *player, uint32_t dup_id);
    int after_pack_btl(player_t *player, battleproto::cs_battle_duplicate_enter_map &btl_in);
    int after_enter_dup(player_t *player, uint32_t dup_id,
            battleproto::sc_battle_duplicate_enter_map &btl_out,
            onlineproto::sc_0x0201_duplicate_enter_map &cli_out);

    //给具体的副本玩法注册 进入之前的处理函数
    void register_proc_func();

private:
    //星灵挑战进入处理
    static int before_enter_trial_attr(player_t *player, uint32_t dup_id);
    static int after_enter_trial_attr(player_t *player, uint32_t dup_id, 
            battleproto::sc_battle_duplicate_enter_map &btl_out,
            onlineproto::sc_0x0201_duplicate_enter_map &cli_out);
    //一桶天下挑战进入处理
    static int before_enter_bucket(player_t *player, uint32_t dup_id);
    static int after_enter_bucket(player_t *player, uint32_t dup_id,
            battleproto::sc_battle_duplicate_enter_map &btl_out,
            onlineproto::sc_0x0201_duplicate_enter_map &cli_out);
	//TODO kevinz 试炼副本挑战进入处理
	static int before_enter_trial(player_t *player, uint32_t dup_id);
	static int after_enter_trial(player_t *player, uint32_t dup_id,
            battleproto::sc_battle_duplicate_enter_map &btl_out,
            onlineproto::sc_0x0201_duplicate_enter_map &cli_out);
    //进入新手挑战流程处理
    static int before_enter_starter(player_t *player, uint32_t dup_id);
	//怪物危机进入处理
	static int before_enter_monster_crisis(player_t *player, uint32_t dup_id);

    //进入即时竞技场
	static int before_enter_rpvp(player_t *player, uint32_t dup_id);
    static int after_pack_rpvp(player_t *player, battleproto::cs_battle_duplicate_enter_map &btl_in);
	static int after_enter_rpvp(player_t *player, uint32_t dup_id,
            battleproto::sc_battle_duplicate_enter_map &btl_out,
            onlineproto::sc_0x0201_duplicate_enter_map &cli_out);

    // 进入家族副本
    static int before_enter_family(player_t *player, uint32_t dup_id);
	//夜袭进入处理
	static int before_enter_night_raid(player_t *player, uint32_t dup_id);
    static int after_enter_night_raid(player_t *player, uint32_t dup_id,
            battleproto::sc_battle_duplicate_enter_map &btl_out,
            onlineproto::sc_0x0201_duplicate_enter_map &cli_out);

    // 进入世界boss副本
    static int before_enter_world_boss(player_t *player, uint32_t dup_id);
	static int after_enter_world_boss(player_t *player, uint32_t dup_id,
            battleproto::sc_battle_duplicate_enter_map &btl_out,
            onlineproto::sc_0x0201_duplicate_enter_map &cli_out);

	static int after_enter_mayin_bucket(player_t *player, uint32_t dup_id,
			battleproto::sc_battle_duplicate_enter_map &btl_out,
			onlineproto::sc_0x0201_duplicate_enter_map &cli_out);

	//进入修罗化身副本
    static int before_enter_challenge_demon(player_t *player, uint32_t dup_id);
	static int after_challenge_demon(player_t *player, uint32_t dup_id,
			battleproto::sc_battle_duplicate_enter_map &btl_out,
			onlineproto::sc_0x0201_duplicate_enter_map &cli_out);

	static int before_enter_star_pet(player_t* player, uint32_t dup_id);

private:
    onlineproto::cs_0x0201_duplicate_enter_map cli_in_;
    onlineproto::sc_0x0201_duplicate_enter_map cli_out_;
    battleproto::cs_battle_duplicate_enter_map btl_in_;
    battleproto::sc_battle_duplicate_enter_map btl_out_;

    typedef int (*before_func)(player_t* player, uint32_t dup_id);
    typedef int (*after_func)(player_t* player, uint32_t dup_id,
            battleproto::sc_battle_duplicate_enter_map &btl_out,
            onlineproto::sc_0x0201_duplicate_enter_map &cli_out);
    typedef int (*after_pack_func)(player_t *player, battleproto::cs_battle_duplicate_enter_map &btl_in);

    std::map<uint32_t, before_func> before_enter_func_map_;
    std::map<uint32_t, after_func> after_enter_func_map_;
    std::map<uint32_t, after_pack_func> after_pack_func_map_;
};

class DuplicateMatchResultCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
};

class LeaveDuplicateCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0203_duplicate_leave_map cli_in_;
    onlineproto::sc_0x0203_duplicate_leave_map cli_out_;
    battleproto::cs_battle_relay btl_in_;
    battleproto::sc_battle_relay btl_out_;
};

class ReadyDuplicateCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0205_duplicate_battle_ready cli_in_;
    onlineproto::sc_0x0205_duplicate_battle_ready cli_out_;
    battleproto::cs_battle_relay btl_in_;
    battleproto::sc_battle_relay btl_out_;
};

class ExitDuplicateCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0208_duplicate_exit cli_in_;
    onlineproto::sc_0x0208_duplicate_exit cli_out_;
    battleproto::cs_battle_relay btl_in_;
    battleproto::sc_battle_relay btl_out_;
};

class HitDuplicateObjCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x020A_duplicate_hit_character cli_in_;
    onlineproto::sc_0x020A_duplicate_hit_character cli_out_;
    battleproto::cs_battle_relay btl_in_;
    battleproto::sc_battle_relay btl_out_;
};

class StatDuplicateCmdProcessor : public CmdProcessorInterface {
public:
    StatDuplicateCmdProcessor() {
        after_battle_func_map_.clear();
        register_proc_func();
    }
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

private:
    void register_proc_func();
    int after_battle(player_t *player, uint32_t dup_id, bool win);
public:
    //星灵挑战结束后
    static int after_trial_attr(player_t *player, uint32_t dup_id, bool win);
    //一桶天下挑战结束后
    static int after_bucket(player_t *player, uint32_t dup_id, bool win);
	//玛音试练挑战结束后
	static int after_mayin_bucket(player_t *player, uint32_t dup_id, bool win);
	//试炼副本挑战结束后
	static int after_trial(player_t *player, uint32_t dup_id, bool win);
    //新手副本结束后
    static int after_starter(player_t *player, uint32_t dup_id, bool win);
    
	//怪物危机副本结束后
	static int after_monster_crisis(player_t *player, uint32_t dup_id, bool win);

    //家族副本结束后
    static int after_family(player_t *player, uint32_t dup_id, bool win);

	//夜袭副本结束后
	static int after_night_raid(player_t *player, uint32_t dup_id, bool win);

    //即时竞技场结束后
    static int after_rpvp(player_t *player, uint32_t dup_id, bool win);

private:
    onlineproto::cs_0x020F_duplicate_stat_info cli_in_;
    onlineproto::sc_0x020F_duplicate_stat_info cli_out_;

    typedef int (*func)(player_t *player, uint32_t dup_id, bool win);
    std::map<uint32_t, func> after_battle_func_map_;
};

class ToDuplicateNextPhaseCmdProcessor : public CmdProcessorInterface {
public:
    ToDuplicateNextPhaseCmdProcessor() {
        func_map_.clear();
        register_func();
    }
    void register_func();
    int before_to_next_phase(player_t *player, uint32_t dup_id, uint32_t phase_id);

    static int phase_func_starter(player_t *player, uint32_t phase_id);
    static int phase_func_dark_eye_will(player_t *player, uint32_t phase_id);

public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0211_duplicate_to_next_phase cli_in_;
    onlineproto::sc_0x0211_duplicate_to_next_phase cli_out_;
    battleproto::cs_battle_relay btl_in_;
    battleproto::sc_battle_relay btl_out_;

    typedef int (*func)(player_t *player, uint32_t phase_id);
    std::map<uint32_t, func> func_map_;
};

class RevivalDuplicateCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0212_duplicate_revival cli_in_;
    onlineproto::sc_0x0212_duplicate_revival cli_out_;
    battleproto::cs_battle_duplicate_revival btl_in_;
    battleproto::sc_battle_duplicate_revival btl_out_;
    bool auto_revival_flag;
};

class DuplicatePickUpCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0215_duplicate_pick_up_dead_pet_prize cli_in_;
    onlineproto::sc_0x0215_duplicate_pick_up_dead_pet_prize cli_out_;
};

class DuplicateBtlNotifyKillCharacterCmdProcessor : public CmdProcessorInterface {
public:
    DuplicateBtlNotifyKillCharacterCmdProcessor() {
        kill_func_map_.clear();
        register_proc_func();
    }
public:
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    void register_proc_func();
    int when_kill(player_t *player, battleproto::sc_battle_duplicate_notify_kill_character &msg);
private:
    static int when_kill_in_bucket(player_t *player, 
            battleproto::sc_battle_duplicate_notify_kill_character &msg);

	static int when_kill_in_mayin_bucket(player_t* player,
			battleproto::sc_battle_duplicate_notify_kill_character &msg);

	static int when_kill_in_dark_eye_will(player_t* player,
			battleproto::sc_battle_duplicate_notify_kill_character &msg);
private:
    battleproto::sc_battle_duplicate_notify_kill_character btl_out_;
    typedef int (*func)(player_t *player, battleproto::sc_battle_duplicate_notify_kill_character &msg);
    std::map<uint32_t, func> kill_func_map_;
};

class DuplicateBtlNotifyEndCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    battleproto::sc_battle_duplicate_notify_end btl_out_;
};

class DuplicateBtlNotifyRelayCmdProcessor : public CmdProcessorInterface {
public:
    DuplicateBtlNotifyRelayCmdProcessor() {
        func_map_.clear();
        register_phase_func();
    }

    void register_phase_func();
    int before_notify_next_phase(player_t *player, uint32_t dup_id,
            onlineproto::sc_0x0213_duplicate_notify_to_next_phase &next_msg);

    static int before_notify_next_phase_starter(player_t *player, uint32_t dup_id,
            onlineproto::sc_0x0213_duplicate_notify_to_next_phase &next_msg);

    //记录黑瞳的决心的阶段
    static int before_notify_next_phase_dark_eye_will(player_t *player, uint32_t dup_id,
            onlineproto::sc_0x0213_duplicate_notify_to_next_phase &next_msg);

#define RELAY_CMD_FUNC(f)  \
    static int relay_notify_##f( \
            DuplicateBtlNotifyRelayCmdProcessor * const p_this, \
            player_t *player, battleproto::sc_battle_relay &btl_out); \

    RELAY_CMD_FUNC(monster_born)
    RELAY_CMD_FUNC(to_next_phase)
    RELAY_CMD_FUNC(role_recover)
    RELAY_CMD_FUNC(hit_charactor)

public:
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);

private:
    battleproto::sc_battle_relay btl_out_;

    typedef int (*phase_func)(player_t* player, uint32_t dup_id,
            onlineproto::sc_0x0213_duplicate_notify_to_next_phase &next_msg);
    std::map<uint32_t, phase_func> func_map_;

    typedef int (*relay_cmd_func)(
            DuplicateBtlNotifyRelayCmdProcessor * const p_this,
            player_t *player, battleproto::sc_battle_relay &btl_out);
    std::map<uint32_t, relay_cmd_func> relay_cmd_func_map_;
};

class OneKeyPassDuplicateCmdProcessor : public CmdProcessorInterface {
public:
	OneKeyPassDuplicateCmdProcessor() {
		one_key_pass_map_.clear();
		register_proc_func();
	}
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

private:
	void register_proc_func();

	typedef uint32_t (*one_key_pass_func) (
            player_t* player, const std::vector<onlineproto::pass_dup_t>& pass_dup, uint32_t mode);
    static uint32_t pass_normal_dup(
		player_t* player, const std::vector<onlineproto::pass_dup_t>& pass_dup, uint32_t mode);
    static uint32_t pass_monster_crisis(
		player_t* player, const std::vector<onlineproto::pass_dup_t>& pass_dup, uint32_t mode);
    onlineproto::cs_0x0226_one_key_pass_duplicate cli_in_;
    onlineproto::sc_0x0226_one_key_pass_duplicate cli_out_;
	std::map<uint32_t, one_key_pass_func> one_key_pass_map_;
};

#if 0
class DupSwitchFightPetCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0229_duplicate_switch_fight_pet cli_in_;
    onlineproto::sc_0x0229_duplicate_switch_fight_pet cli_out_;
    battleproto::cs_battle_relay btl_in_;
    battleproto::sc_battle_relay btl_out_;
};
#endif

class DupFrontMonFlushReqCmdProcessor: public CmdProcessorInterface{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x022B_duplicate_mon_flush_request cli_in_;
    onlineproto::sc_0x022B_duplicate_mon_flush_request cli_out_;
    battleproto::cs_battle_relay btl_in_;
};

class DupBuyDailyCntCmdProcessor: public CmdProcessorInterface{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x022C_buy_duplicate_cnt cli_in_;
    onlineproto::sc_0x022C_buy_duplicate_cnt cli_out_;
};

class ResetDupCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0234_reset_dup cli_in_;
	onlineproto::sc_0x0234_reset_dup cli_out_;
};

class SkillEffectCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x022D_skill_affect cli_in_;
    onlineproto::sc_0x022D_skill_affect cli_out_;
    battleproto::cs_battle_relay btl_in_;
};

class GiveUpMatchCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
};

//对战中心报告 你连接的对战服挂了
class BattleNotifyDownCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
    battleproto::sc_battle_notify_battle_down noti_msg_;
};


/** 
 * @brief 查询世界boss副本信息
 */
class GetWorldBossDupInfoCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x023F_get_world_boss_dup_info cli_in_;
    onlineproto::sc_0x023F_get_world_boss_dup_info cli_out_;
};

//夜袭
class PVEPMatchCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
	int proc_pkg_from_serv(player_t *player, const char *body, int bodylen);
private:
	int proc_pkg_from_serv_aft_get_infos_from_redis(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_infos_from_db(
			player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv_aft_get_infos_from_cache(
			player_t* player, const char* body, int bodylen);
    onlineproto::cs_0x0248_pvep_match_opponent cli_in_;
    onlineproto::sc_0x0248_pvep_match_opponent cli_out_;
	// onlineproto::cs_0x0240_expedition_into_scene cli_in_;
	// onlineproto::sc_0x0240_expedition_into_scene cli_out_;
	rankproto::sc_get_users_by_score_range rank_out_;
	dbproto::sc_user_raw_data_get db_out_;
	cacheproto::sc_batch_get_users_info  cache_info_out_;
};

//夜袭买活
class PVEPRevivalCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0249_pvep_revival cli_in_;
    onlineproto::sc_0x0249_pvep_revival cli_out_;
};
//夜袭重置
class PVEPResetCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0250_pvep_reset cli_in_;
    onlineproto::sc_0x0250_pvep_reset cli_out_;
};

//夜袭重置
class PVEPPrizeTotalCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
	int proc_pkg_from_serv(player_t* player, const char* body, int bodylen);
private:
	onlineproto::cs_0x0251_night_raid_prize_total cli_in_;
	onlineproto::sc_0x0251_night_raid_prize_total cli_out_;
	dbproto::sc_user_raw_data_get db_out_;
};

class BuyDupCleanCDCmdProcessor : public CmdProcessorInterface {
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0256_buy_dup_clean_cd cli_in_;
    onlineproto::sc_0x0256_buy_dup_clean_cd cli_out_;
};

//购买一键通关副本
class BuyPassDupCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0257_buy_pass_dup cli_in_;
	onlineproto::sc_0x0257_buy_pass_dup cli_out_;
};

class PetBuyPassDupCmdProcessor : public CmdProcessorInterface {
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0258_pet_buy_pass_dup cli_in_;
	onlineproto::sc_0x0258_pet_buy_pass_dup cli_out_;
};

#endif
