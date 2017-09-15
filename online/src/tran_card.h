#ifndef TRAN_CARD_H
#define TRAN_CARD_H

#include <stdint.h>
#include <common.h>
#include <vector>

extern const uint32_t up_cost;
extern const uint32_t KMaxTranSkillNum;
extern const uint32_t KMaxCardStarLv;
extern const uint32_t CardConfBaseValue;

struct tran_card_t {
	tran_card_t() : 
			card_id(0),
			card_star_level(0),
			choose_flag(0) { skill_ids.clear();}
	uint32_t card_id;	//变身卡id
	uint32_t card_star_level;	//卡牌星级
	uint32_t choose_flag;		//是否被选中：0.未选中;1.选中
	std::vector<uint32_t> skill_ids;	//该卡牌已经拥有的技能列表
};

class TranCard {
public:
	typedef std::map<uint32_t, tran_card_t> TranCardMap;
	TranCard() {
	}
	/*@brief 判断变身卡是否存在
	 */
	bool is_tran_id_exsit(uint32_t tran_id);	
	/*@brief 初始化变身卡
	 */
	uint32_t init_tranCard(player_t *player, uint32_t card_id, tran_card_t& tran_card, uint32_t level = 0);
	/*@brief 更新精灵卡信息
	 */
	int update_tranCard(uint32_t card_id, tran_card_t& tran_card);
	/*@brief 添加一个变身卡(用于内挂，或者运营)
	 */
	uint32_t add_tranCard(player_t *player, uint32_t card_id, uint32_t level = 0);
	/*@brief 获得card_id变身卡的信息
	 */
	int get_tranCard(uint32_t card_id, tran_card_t& tran_card);
	/*@brief 设置变身卡信息(不同于TranCard::update_tranCard)
	 */
	int set_tranCard(tran_card_t& tran_card);
	/*@brief 获得内存中所保存的所有变身卡信息(只是一个copy)
	 */
	void get_tran_map_info(TranCardMap& tran_map);
private:
	
	
private:
	TranCardMap m_tran_card;
};

int sync_notify_tran_card_info(player_t *player, const vector<tran_card_t>& tran_vec, uint32_t flag);

#endif
