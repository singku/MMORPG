#include "tran_card.h"
#include "player.h"

const uint32_t up_cost = 10000;
const uint32_t KMaxTranSkillNum = 6;
const uint32_t KMaxCardStarLv = 10;
const uint32_t CardConfBaseValue = 40000;

bool TranCard::is_tran_id_exsit(uint32_t tran_id) {
	TranCardMap::iterator iter = m_tran_card.find(tran_id);
	if (iter == m_tran_card.end()) {
		return false;
	}
	return true;
}

int TranCard::update_tranCard(uint32_t card_id, tran_card_t& tran_card) {
	TranCardMap::iterator iter = m_tran_card.find(card_id);
	if (iter == m_tran_card.end()) {
		return -1;
	}
	m_tran_card[card_id] = tran_card;
	return 0;
}

uint32_t TranCard::add_tranCard(player_t *player, uint32_t card_id, uint32_t level) {
	tran_card_t tran_card;
	uint32_t ret = init_tranCard(player, card_id, tran_card, level);
	if (ret) {
		return ret;
	}

	set_tranCard(tran_card);

	uint32_t ser_card_id = card_id - CardConfBaseValue;
	attr_type_t attr = AttrUtils::get_tran_card_id_attr(ser_card_id);
	uint32_t tmp_card = GET_A(attr);
	if (tmp_card && (tmp_card < CardConfBaseValue + 1|| tmp_card > CardConfBaseValue + 100)) {
		ERROR_TLOG("init_tran_card_failed:tmp_card=[%u],attr=[%u],card_id=[%u]", tmp_card,attr, card_id);
		return cli_err_err_tran_id_cause_ini_failed;
	}
	if (!tmp_card) {
		SET_A(attr, card_id);
		SET_A((attr_type_t)(attr + 1), level);
	} else {
		ERROR_LOG("init_tranCard,err02,[%u]", tmp_card);
		return cli_err_tran_id_has_exist_in_attr;
	}

	std::vector<tran_card_t> tem_vec; 
	tem_vec.push_back(tran_card);
	sync_notify_tran_card_info(player, tem_vec, 1);
	return 0;
}

int TranCard::get_tranCard(uint32_t card_id, tran_card_t& tran_card) {
	TranCardMap::iterator iter = m_tran_card.find(card_id);
	if (iter == m_tran_card.end()) {
		return -1;
	}
	tran_card = iter->second;
	return 0;
}

int TranCard::set_tranCard(tran_card_t& tran_card) {
	TranCardMap::iterator iter = m_tran_card.find(tran_card.card_id);
	if (iter != m_tran_card.end()) {
		return -1;
	}
	m_tran_card.insert(TranCardMap::value_type(tran_card.card_id, tran_card));
	return 0;
}

uint32_t TranCard::init_tranCard(player_t *player, uint32_t card_id, tran_card_t& tran_card, uint32_t level) {
	TranCardMap::iterator iter = m_tran_card.find(card_id);
	if (iter != m_tran_card.end()) {
		ERROR_LOG("card has exsit=[%u]", card_id);
		return cli_err_tran_card_has_exsit;
	}
	if ((card_id < CardConfBaseValue + 1 || card_id > CardConfBaseValue + 100)) {
		ERROR_LOG("init_tranCard,err01,card_id=[%u]", card_id);
		return cli_err_data_error;
	}
	tran_card.card_id = card_id;
	tran_card.card_star_level = level;
	tran_card.choose_flag = 0;
	tran_card.skill_ids.clear();
	//add_tranCard(card_id, tran_card);
	return 0;
}

void TranCard::get_tran_map_info(TranCardMap& tran_map) {
	tran_map = m_tran_card;
}

int sync_notify_tran_card_info(player_t *player, const vector<tran_card_t>& tran_vec, uint32_t flag) {
	onlineproto::sc_0x0121_sync_tran_card tran_msg;
	for (uint32_t i = 0; i < tran_vec.size(); ++i) {
		commonproto::tran_card_cli_t* tran_ptr = tran_msg.add_tran_card_list();
		commonproto::tran_card_t* info_ptr = tran_ptr->mutable_card_info();
		info_ptr->set_card_id(tran_vec[i].card_id);
		info_ptr->set_card_star_level(tran_vec[i].card_star_level);
		tran_ptr->set_flag(flag);
		/*
		for (uint32_t j = 0; j < tran_vec[i].skill_ids.size(); ++j) {
			tran_ptr->add_skill_ids(tran_vec[i].skill_ids[j]);
		}
		*/
	}
	send_msg_to_player(player, cli_cmd_cs_0x0121_sync_tran_card, tran_msg);
	return 0;
}
