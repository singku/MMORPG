/**
 * @file mail.h
 * @brief  邮件信息模块
 * @author colin colin@taomee.com
 * @version 1.0.0
 * @date 2014-07-24
 *       
 */

#ifndef MAIL_INFO_H
#define MAIL_INFO_H

#include <stdio.h>
#include <stdint.h>
#include "proto.h"
#include "proto/db/dbproto.data.pb.h"
#include <map>
#include <vector>

//TODO kevin 与 prize_elem_type_t 保持一致
enum mail_attach_type_t
{
	kMailAttachItem = 1, //物品
	kMailAttachPet = 2,	//精灵
	kMailAttachAttr = 3, //属性
	kMailAttachTitle = 4, //称号
	kMailAttachRune = 5, //符文
};

enum mail_status_t
{
	kMailUnRead = commonproto::UNREAD,
	kMailUnGetAttach = commonproto::UNGETATTACH,
	kMailAttacheGot = commonproto::GOTATTACH,
	kMailDeleted = commonproto::DELETED,
};

struct attachment_t
{
	mail_attach_type_t type; //奖励类型
	uint32_t id; //奖励物品id
	uint32_t count; //奖励物品数量
};

struct mail_t
{
	string  mailid; //系统生成的uuid
	mail_status_t status;
	uint32_t send_time; //邮件发送
	std::string sender; //发送者名称
	std::string title; // 邮件标题
	std::string content; // 邮件内容
	//邮件附件 (typd,id,cnt;type,id,cnt...)
	std::string attachment; //邮件附件
};

struct new_mail_t {
    string sender;
    string title;
    string content;
    //std::vector<attachment_t> attaches;
	string attachment;
};

#endif
