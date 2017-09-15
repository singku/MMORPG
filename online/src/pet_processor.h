#ifndef PET_PROCESSOR_H
#define PET_PROCESSOR_H

#include "common.h"
#include "proto/client/pb0x03.pb.h"
#include "cmd_processor_interface.h"
#include "player.h"
#include "pet.h"

//改变精灵的位置
class PetSwitchLocCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t* player, const char* body, int bodylen);
private:
    onlineproto::cs_0x0307_change_pet_location cli_in_;
    onlineproto::sc_0x0307_change_pet_location cli_out_;
};

//设置精灵跟随出战
class PetSetFightCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0309_set_fight_pet_status cli_in_;
    onlineproto::sc_0x0309_set_fight_pet_status cli_out_;
};


//精灵条件进化
class ConditionalPetEvolutionCmdProcessor : public CmdProcessorInterface
{
public:
    ConditionalPetEvolutionCmdProcessor();
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x030C_conditional_pet_evolution cli_in_;
    onlineproto::sc_0x030C_conditional_pet_evolution cli_out_;
    typedef int (*func)(player_t* player, Pet* pet, uint32_t reason);
    std::map<uint32_t, func> func_map_;
    int default_condition(player_t *player, Pet *pet);
    int after_evolve(player_t *player);
    int check_condition(player_t* player, Pet* pet, uint32_t reason);
    uint32_t reduce_item_id_;
    uint32_t reduce_item_cnt_;
};

//精灵条件加经验
class ConditionalPetAddExpCmdProcessor : public CmdProcessorInterface
{
public:
    //构造函数的实现中注册处理函数
    ConditionalPetAddExpCmdProcessor();
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x030E_conditional_pet_add_exp cli_in_;
    onlineproto::sc_0x030E_conditional_pet_add_exp cli_out_;

    //注册处理函数
    typedef int (*func)(player_t* player, Pet *pet, uint32_t reason);
    std::map<uint32_t, func> func_map_;

    //20150611伙伴进修活动
    static int proc_add_exp_150611(player_t *player, Pet *pet, uint32_t reason);
    int proc_add_exp(player_t *player, Pet *pet, uint32_t reason);
};

//精灵条件升级
class ConditionalPetLevelUpCmdProcessor : public CmdProcessorInterface
{
public:
    ConditionalPetLevelUpCmdProcessor();
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x030D_conditional_pet_level_up cli_in_;
    onlineproto::sc_0x030D_conditional_pet_level_up cli_out_;

    typedef int (*func)(player_t* player, Pet* pet, uint32_t reason, uint32_t &to_level);
    std::map<uint32_t, func> func_map_;
    int check_condition(player_t* player, Pet* pet, uint32_t reason, uint32_t &to_level);
    //20150611伙伴进修活动
    static int proc_add_level_150611(player_t *player, Pet *pet, uint32_t reason, uint32_t &to_level);
	//一键满级
    static int proc_one_key_max_level(player_t *player, Pet *pet, uint32_t reason, uint32_t &to_level);
   //升级到指定等级
   	static int proc_one_key_appionted_level(player_t *player, Pet *pet,uint32_t reason, uint32_t &to_level);
  //五小强升级
	static int proc_one_key_one_level(player_t *player, Pet *pet,uint32_t reason, uint32_t &to_level);
};

//精灵条件设置天赋
class ConditionalPetSetTalentCmdProcessor : public CmdProcessorInterface
{
public:
    ConditionalPetSetTalentCmdProcessor();
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x030F_conditional_pet_set_talent cli_in_;
    onlineproto::sc_0x030F_conditional_pet_set_talent cli_out_;

    typedef int (*func)(player_t* player, Pet* pet, uint32_t reason, uint32_t &to_star);
    std::map<uint32_t, func> func_map_;
    int check_condition(player_t* player, Pet* pet, uint32_t reason, uint32_t &to_star);
	static int proc_one_key_add_talent(player_t* player, Pet* pet, uint32_t reason, uint32_t &to_star);
};

//精灵条件增加学习力
class ConditionalPetAddEffortCmdProcessor : public CmdProcessorInterface
{
public:
    ConditionalPetAddEffortCmdProcessor();
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0310_conditional_pet_add_effort cli_in_;
    onlineproto::sc_0x0310_conditional_pet_add_effort cli_out_;

    typedef int (*func) (player_t*, Pet*, 
            onlineproto::effort_alloc_data_t *alloc_data);
    std::map<uint32_t, func> func_map_;

    int proc_cond_add_effort(player_t *player, Pet * pet, uint32_t reason,
            onlineproto::effort_alloc_data_t *alloc_data = 0);
	//一键最佳特训
	static int proc_one_key_max_effort(player_t*, Pet*,
			onlineproto::effort_alloc_data_t *alloc_data = 0);
};

//获取精灵
class GetPetCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x031A_get_pets cli_in_;
    onlineproto::sc_0x031A_get_pets cli_out_;
};

//精灵刻印
class PetChiselCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0323_chisel_on cli_in_;
    onlineproto::sc_0x0323_chisel_on cli_out_;
};

//精灵召唤
class PetCallBornCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x0328_call_pet_born cli_in_;
    onlineproto::sc_0x0328_call_pet_born cli_out_;
};

//更换技能
class UpdateSkillCmdProcessor : public CmdProcessorInterface
{
public:
    int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
    onlineproto::cs_0x032A_update_skill cli_in_;
    onlineproto::sc_0x032A_update_skill cli_out_;
};

// 精灵觉醒
class ConditionalPetAwakenCmdProcessor : public CmdProcessorInterface
{
    public:
		//构造函数的实现中注册处理函数
		ConditionalPetAwakenCmdProcessor();
        int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
    private:
        onlineproto::cs_0x032B_pet_awaken cli_in_;
		onlineproto::sc_0x032B_pet_awaken cli_out_;

		typedef int (*func)(player_t* player, Pet* pet, const pet_quality_conf_t * pq_conf, uint32_t reason, uint32_t &to_quality);
		std::map<uint32_t, func> func_map_;
		int check_condition(player_t* player, Pet* pet, const pet_quality_conf_t * pq_conf, uint32_t reason, uint32_t &to_quality);
		static int proc_one_key_add_quality(player_t *player, Pet *pet, const pet_quality_conf_t * pq_conf, uint32_t reason, uint32_t &to_quality);
		static int proc_default_add_quality(player_t *player, Pet *pet, const pet_quality_conf_t * pq_conf, uint32_t reason, uint32_t &to_quality);
};

//精灵操作开始
class PetOpBeginCmdProcessor : public CmdProcessorInterface
{
public:
	PetOpBeginCmdProcessor() {
		op_begin_func_map_.clear();
		register_proc_func();
	}

public:
	int proc_pkg_by_pet_op(player_t* player, Pet* pet, uint32_t pos, uint32_t op_type);
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);

private:
	static uint32_t pet_find_box_begin(player_t *player, Pet* pet, uint32_t pos);
	static uint32_t pet_exercise_begin(player_t *player, Pet* pet, uint32_t pos);
private:
	void register_proc_func();
	typedef uint32_t (*op_begin_func) (player_t* player, Pet* pet, uint32_t pos);

	std::map<uint32_t, op_begin_func> op_begin_func_map_;
	onlineproto::cs_0x032D_pet_op_begin cli_in_;
	onlineproto::sc_0x032D_pet_op_begin cli_out_;
};

//精灵操作结束
class PetRecallCmdProcessor : public CmdProcessorInterface
{
public:
	PetRecallCmdProcessor() {
		pet_recall_func_map_.clear();
		register_proc_func();
	}
public:
	int proc_pkg_when_pet_recall(player_t* player, Pet* pet,
			uint32_t pos, uint32_t op_type, 
			onlineproto::sc_0x032F_pet_op_end& cli_out);
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	static uint32_t pet_find_box_recall(player_t* player, Pet* pet,
			uint32_t pos, onlineproto::sc_0x032F_pet_op_end& cli_out);
	static uint32_t pet_exercise_recall(player_t* player, Pet* pet,
			uint32_t pos, onlineproto::sc_0x032F_pet_op_end& cli_out);
private:
	void register_proc_func();
	typedef uint32_t (*pet_recall_func) (player_t* player, Pet* pet,
			uint32_t pos, onlineproto::sc_0x032F_pet_op_end& cli_out);
	std::map<uint32_t, pet_recall_func> pet_recall_func_map_;
	onlineproto::cs_0x032F_pet_op_end cli_in_;
	onlineproto::sc_0x032F_pet_op_end cli_out_;
};

//开启小屋宝箱
class OpenExerciseBoxCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0331_open_exercise_box cli_in_;
	onlineproto::sc_0x0331_open_exercise_box cli_out_;
};

class DijuLightLampCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0335_diju_light_lamp cli_in_;
	onlineproto::sc_0x0335_diju_light_lamp cli_out_;
};

class PetDijuAwakeCmdProcessor : public CmdProcessorInterface
{
public:
	int proc_pkg_from_client(player_t *player, const char *body, int bodylen);
private:
	onlineproto::cs_0x0259_pet_diju_awake cli_in_;
	onlineproto::sc_0x0259_pet_diju_awake cli_out_;
};

#endif
