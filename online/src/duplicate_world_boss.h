#ifndef __DUPLICATE_WORLD_BOSS_H_
#define __DUPLICATE_WORLD_BOSS_H_

#include "common.h"

struct user_damage_info_t {
    user_damage_info_t(){
        user_key = 0;
        damage = 0;
        nick.clear();
    }
    uint64_t user_key;
    uint32_t damage;
    std::string nick;
};

struct world_boss_stage_time_t {
    world_boss_stage_time_t() {
        stage = 0;
    }
    uint32_t stage; // commonproto::world_boss_dup_status_t
    time_limit_t time_limit;
};
typedef std::vector<world_boss_stage_time_t> world_boss_time_config_vec_t;


// 世界boss副本信息
struct world_boss_dup_info_t {
    world_boss_dup_info_t() {
        dup_id = 0;
        status = 0;
        boss_hp = 0;
        boss_maxhp = 0;
        boss_lv = 0;
        boss_exp = 0;
        start_time = 0;
        end_time = 0;
        reward_flag = false;
        kill_user_key = 0;
        kill_flag = false;
        damage_map.clear();
        top_n_users.clear();
        reward_mask_map.clear();
    }
     uint32_t dup_id;
     uint32_t status; // commonproto.world_boss_dup_status_t
     uint32_t start_time; //最近一次世界boss玩法开始时间,从可以挑战开始计算
     uint32_t end_time;   // 最近一次世界boss玩法结束时间
     uint32_t boss_hp;
     uint32_t boss_maxhp;
     uint32_t boss_lv;
     uint32_t boss_exp;
     bool reward_flag; // false 未发奖, true 已发奖
     uint64_t kill_user_key;  // 最后一击用户id
     bool kill_flag;        // false boss未击杀, true boss已被击杀
     // <user_key, damage>
     std::map<uint64_t, user_damage_info_t> damage_map; // 参与战斗的用户集合
     std::list<user_damage_info_t> top_n_users;     // top n伤害的用户
     // <user_key, reward_mask> reward_mask bit1-4 是否可以得奖, bit17-20 是否已发奖,0否1是
     std::map<uint64_t, uint32_t> reward_mask_map;  // 奖励记录
};

class duplicate_world_boss_mgr_t {
    public:
        duplicate_world_boss_mgr_t() {
            world_dup_map_.clear();
            stage_time_index.clear();
        }

        ~duplicate_world_boss_mgr_t() {
        }

        world_boss_dup_info_t *get_world_boss_dup_info(uint32_t dup_id) {
            std::map<uint32_t, world_boss_dup_info_t>::iterator iter = 
                world_dup_map_.find(dup_id);
            if (iter == world_dup_map_.end()) {
                return NULL;
            }

            return &(iter->second);
        }

        int init(uint32_t dup_id);
        int clear_player_record(
            uint32_t dup_id, uint32_t userid, uint32_t u_create_tm, uint32_t svr_id);
        int clear_player_damage_record(
            uint32_t dup_id, uint32_t userid, uint32_t u_create_tm);
        int clear_dup_record(uint32_t dup_id);

        uint32_t get_next_status_left_time(uint32_t dup_id);

        int give_world_boss_reward(uint32_t dup_id);
        int give_world_boss_single_user_reward(
                uint32_t dup_id, uint32_t userid, 
                uint32_t u_create_tm, uint32_t &reward_mask);
        uint32_t get_reward_status(
                uint32_t dup_id, uint32_t reward_mask);

        uint32_t get_current_start_time(uint32_t dup_id);
        uint32_t compute_start_end_time(
            uint32_t dup_id, time_t timestamp, uint32_t flag);;

        uint32_t world_boss_sys_notify(
            uint32_t type, uint32_t dup_id, std::vector<uint32_t> &paras);

    private:
        // <dup_id, info>
        std::map<uint32_t, world_boss_dup_info_t> world_dup_map_;

        // 以周日期建立的时间配置索引,计算下一阶段倒计时用
        // <weekday(1-7), <time_config> >
        std::map<uint32_t, world_boss_time_config_vec_t> stage_time_index;

        /*std::set<uint32_t> world_boss_dup_set_;   // 所有世界boss副本id集合*/
};


#endif
