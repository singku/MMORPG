#//dplan create database script 
#把下面所有的10.1.1.154改成对应的192.168.39.15/85
#//dplan_db_xx.xx_table_xx
#00-49 39.15 50-99 39.85

for i in {0..99}
do
    db_no=`printf '%02d' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "CREATE DATABASE IF NOT EXISTS dplan_db_${db_no}"
    for j in {0..99}
    do
        table_no=`printf '%02d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "

SET NAMES UTF8;
CREATE TABLE IF NOT EXISTS dplan_db_${db_no}.base_info_table_${table_no} 
(
    userid INT UNSIGNED NOT NULL,
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    init_server_id INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '初始服务器ID',
    server_id INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '当前服务器ID',
    nick VARCHAR(16) NOT NULL DEFAULT '',
    primary key (userid, u_create_tm)

) ENGINE = INNODB CHARSET = UTF8; 

CREATE TABLE IF NOT EXISTS dplan_db_${db_no}.item_table_${table_no}
(
    userid INT UNSIGNED NOT NULL,
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    item_id INT UNSIGNED NOT NULL,
    slot_id INT UNSIGNED NOT NULL,
    count INT UNSIGNED NOT NULL DEFAULT 0,
    using_count INT UNSIGNED NOT NULL DEFAULT 0,
    expire_time TIMESTAMP NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '过期时间, 0表示没有过期时间',
    item_optional_attr VARBINARY(1024) NOT NULL DEFAULT '',

    PRIMARY KEY (userid, u_create_tm, slot_id)
) ENGINE = INNODB CHARSET = UTF8;

CREATE TABLE IF NOT EXISTS dplan_db_${db_no}.pet_table_${table_no}
(
    userid INT UNSIGNED NOT NULL,
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    pet_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '精灵id',
    create_tm INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '精灵创建时间',
    level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '精灵等级',
    exp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '精灵经验',
    fight_pos TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '出战位 0表示不出战',
    is_excercise INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '精灵的历练信息',
    talent_level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '天赋等级',
    effort_hp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力生命',
    effort_normal_atk INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力普攻',
    effort_normal_def INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力普防',
    effort_skill_atk INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力技功',
    effort_skill_def INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力技防',
    anti_water INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '草抗',
    anti_fire INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '火抗',
    anti_grass INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '草抗',
    anti_light INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '光抗',
    anti_dark INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '暗抗',
    anti_ground INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '地抗',
    anti_force INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '武抗',
    crit INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '暴击',
    anti_crit INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '防暴',
    hit INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '命中',
    dodge INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '闪避',
    block INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '格挡',
    break_block INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '破格',
    atk_speed INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '攻速',
    crit_affect_rate INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '暴击伤害加成率',
    block_affect_rate INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '格挡伤害加成率',
    hp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '当前血量',
    max_hp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '最大血量终值',
    normal_atk INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '普攻终值',
    normal_def INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '普防终值',
    skill_atk INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '技攻终值',
    skill_def INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '技防终值',
	power INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '精灵的战斗力',
    rune_1_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '符文1ID',
    rune_2_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '符文2ID',
    rune_3_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '符文3ID',
    rune_4_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '符文4ID',
    rune_5_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '符文5ID',
    rune_6_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '符文6ID',
    chisel_pos INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '所在的刻印位置(1 2 3 4 5 6)',
    loc TINYINT UNSIGNED NOT NULL DEFAULT 2 COMMENT '精灵位置',
	rune_3_unlock_flag TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '精灵身上的3号符文格子开启状态0未开启, 1开启',
    quality INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '品质(颜色)',
	exercise_tm INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '精灵开始的锻炼时间戳',
	last_add_exp_tm INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '锻炼的精灵上次加经验的时间戳',
	pet_expedition_hp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '远征时精灵剩余的血量',
	exped_flag TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '远征状态：1参加远征；2参战',
	mon_cris_hp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '怪物危机精灵剩余的血量',
	mon_night_raid_hp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '夜袭精灵剩余的血量',
	effort_hp_lv INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力生命等级',
	effort_normal_atk_lv INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力普攻等级',
	effort_normal_def_lv INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力普防等级',
	effort_skill_atk_lv INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力技攻等级',
	effort_skill_def_lv INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力技防等级',
    elem_type INT UNSINGED NOT NULL DEFAULT 1 COMMENT '元素类型',
    elem_damage_percent INT UNSIGNED NOT NULL DEFAULT 50 COMMENT '默认属性克制系数',
	mine_fight_hp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '矿战血量',
	mine_fight_flag BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '正在攻击的矿',	
	defend_mine_id BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '正在防守的矿id'
    PRIMARY KEY (userid, u_create_tm, create_tm)
) ENGINE = INNODB CHARSET = UTF8;


CREATE TABLE IF NOT EXISTS dplan_db_${db_no}.attr_table_${table_no}
(
    userid INT UNSIGNED NOT NULL,
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    type INT UNSIGNED NOT NULL COMMENT '数据名',
    value INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '数据值', 
   
    PRIMARY KEY (userid, u_create_tm, type)
) ENGINE = INNODB CHARSET = UTF8;

CREATE TABLE IF NOT EXISTS dplan_db_${db_no}.mail_table_${table_no}
(
	userid INT UNSIGNED NOT NULL,
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
	mailid varchar(48) NOT NULL COMMENT '邮件id 邮件唯一标识',
	status TINYINT UNSIGNED NOT NULL COMMENT '0 未查看 1 已查看但未领取附件 2已查看且已领取附件 3 已删除',
    send_time INT UNSIGNED NOT NULL COMMENT '邮件发送时间 unixtime',
	sender varchar(32) NOT NULL DEFAULT '',
	title varchar(64) NOT NULL DEFAULT '' COMMENT '邮件标题',
	content varchar(1024) NOT NULL DEFAULT '' COMMENT '邮件正文',
	attachment varchar(512) NOT NULL DEFAULT '' COMMENT '附件',

	PRIMARY KEY (userid, u_create_tm, mailid)
 ) ENGINE = INNODB CHARSET = UTF8;

CREATE TABLE IF NOT EXISTS dplan_db_${db_no}.rune_table_${table_no} (
    userid int(10) unsigned NOT NULL,
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    runeid int(10) unsigned NOT NULL COMMENT '符文id',
    rune_type tinyint(3) unsigned NOT NULL COMMENT '配置表中对应的类型 id',
    level tinyint(3) unsigned NOT NULL COMMENT '等级',
    exp int(10) unsigned NOT NULL default '0' COMMENT '经验',
    pack_type tinyint(3) unsigned NOT NULL COMMENT '符文位置 0符文馆 1转化背包 2收藏背包 3精灵身上',
    pet_catch_time int(10) unsigned NOT NULL COMMENT '装备该精灵上',
    grid_id tinyint(3) unsigned NOT NULL DEFAULT 0 COMMENT '若果装在精灵身上表示符文所在精灵符文的格子id(1-6) 否则为0',

    PRIMARY KEY  (userid, u_create_tm, runeid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


CREATE TABLE IF NOT EXISTS dplan_db_${db_no}.friend_table_${table_no} 
(
    userid INT UNSIGNED NOT NULL,
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    friendid INT UNSIGNED NOT NULL,
    f_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    is_friend TINYINT(1) NOT NULL DEFAULT 0 COMMENT '是否是好友',
    is_black TINYINT(1) NOT NULL DEFAULT 0 COMMENT '是否属于黑名单',
    is_recent TINYINT(1) NOT NULL DEFAULT 0 COMMENT '是否最近联系人',
	is_temp tinyint(3) unsigned NOT NULL,

    PRIMARY KEY (userid, u_create_tm, friendid, f_create_tm)
)ENGINE = INNODB CHARSET = UTF8;

CREATE TABLE IF NOT EXISTS dplan_db_${db_no}.task_table_${table_no} 
(
    userid  INT UNSIGNED NOT NULL,
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    task_id  INT UNSIGNED NOT NULL COMMENT '任务id',
    status  INT UNSIGNED NOT NULL COMMENT '任务状态--每个bit位表示',
    done_times  INT UNSIGNED NOT NULL COMMENT '做的次数',
    cond_status varchar(128) NOT NULL default '',
    bonus_status tinyint(3) unsigned NOT NULL COMMENT '任务奖励领取标志：0未领取，1已领取',
	task_fin_tm INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '任务首次完成的时间戳',

    PRIMARY KEY(userid, u_create_tm, task_id)
) ENGINE = INNODB CHARSET = UTF8;

CREATE TABLE IF NOT EXISTS dplan_db_${db_no}.buff_table_${table_no}
(
    userid  INT(11) UNSIGNED NOT NULL  DEFAULT 0   COMMENT  '用户id',
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    buff_type  INT(11) UNSIGNED NOT NULL DEFAULT 0   COMMENT  'buff类型',
    buff_id varbinary(36) NOT NULL default '' COMMENT 'buff id',
    buff_data  varbinary(4096) NOT NULL  DEFAULT ''   COMMENT  'buff数据 最大4k',

    PRIMARY KEY (userid, u_create_tm, buff_type, buff_id)
) ENGINE = INNODB CHARSET = UTF8;

CREATE TABLE IF NOT EXISTS dplan_db_${db_no}.home_visit_log_table_${table_no} (
	userid INT UNSIGNED NOT NULL COMMENT '主人ID',
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
	guestid INT UNSIGNED NOT NULL COMMENT '访客ID',
    g_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
	guestname VARCHAR(32) NOT NULL DEFAULT '' COMMENT '访客名字',
	date timestamp NOT NULL default '0000-00-00 00:00:00' COMMENT '访问日期',
	action_type INT UNSIGNED NOT NULL  DEFAULT 1 COMMENT '访客操作类型1:nothing 2:sweeping 3:msg 4:flower 5:throw egg 6:send gift',
	detail_info VARCHAR(1024) NOT NULL DEFAULT '' COMMENT '访问细节(留言内容)',
	gift_id int(10) unsigned NOT NULL DEFAULT 0 COMMENT '碎片在属性表中的id',
    guestsex TINYINT UNSIGNED NOT NULL DEFAULT 0

) ENGINE = INNODB CHARSET = UTF8

"
    echo $db_no.$table_no ok
    done
    
done

#single db dplan_other_db 192.168.39.15
#transaction_table
mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
SET NAMES UTF8;
CREATE DATABASE IF NOT EXISTS dplan_other_db;
USE dplan_other_db;
CREATE TABLE IF NOT EXISTS dplan_other_db.transaction_table
(
    transaction_id bigint unsigned not null auto_increment primary key comment '交易ID',
    transaction_time timestamp not null default '0000-00-00 00:00:00' comment '交易时间',
    server_no int unsigned not null default 0 comment '区服ID',
    account_id int unsigned not null default 0 comment '付费帐号',
    s_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    dest_account_id int unsigned not null default 0 comment '收获帐号',
    d_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    channel_id int unsigned not null default 0 comment '渠道号(0:充值货币 1:游戏产出货币 2:客服添加 3:购买消耗 4:客服减少)',
    pay_gate_trans_id int unsigned not null default 0 comment 'channel_id对应系统的交易号',
    product_id int unsigned not null default 0 comment '商品ID 非物品ID',
    product_type int unsigned not null default 0 comment '商品类型(1:永久型 2:即时消耗性 3:不可续期天权型 4:可续期天权型 5:消耗型)',
    product_duration int unsigned not null default 0 comment '天权型商品有效期天数',
    product_count smallint unsigned not null default 0 comment '商品数量',
    money_num int not null comment '钱的变化数 增加为正 减少为负',
    result tinyint unsigned not null comment '0成功 1失败',
    server_num int unsigned not null default 0 comment '区服ID 账号平台用',

    key (transaction_time),
    key (server_no),
    key (account_id, s_create_tm),
    key (dest_account_id, d_create_tm),
    key (channel_id),
    key (pay_gate_trans_id)

) ENGINE = INNODB CHARSET = UTF8;
"
echo "trannsaction_table ok"

mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
SET NAMES UTF8;
CREATE DATABASE IF NOT EXISTS dplan_other_db;
USE dplan_other_db;
CREATE TABLE IF NOT EXISTS dplan_other_db.client_soft_version_table
(
    userid int unsigned not null default 0 comment '账号',
    u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
    soft1_v varchar(256) not null default '' comment 'soft1版本',
    soft2_v varchar(256) not null default '' comment 'soft2版本',
    soft3_v varchar(256) not null default '' comment 'soft3版本',
    soft4_v varchar(256) not null default '' comment 'soft4版本',

    PRIMARY KEY (userid, u_create_tm)
) ENGINE = INNODB CHARSET = UTF8;
"

mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
SET NAMES UTF8;
CREATE DATABASE IF NOT EXISTS dplan_other_db;
USE dplan_other_db;
CREATE TABLE IF NOT EXISTS dplan_other_db.vip_user_info_table
(
	user_id int unsigned not null default 0 comment '玩家米米号',
	u_create_tm int unsigned not null default 0 comment '玩家创建角色的时间戳',
	server_num int unsigned not null default 1 comment '区服',
	begin_time datetime not null default 0 comment '会员开始时间', 
	end_time datetime not null default 0 comment '会员到期时间',
	time_flag int unsigned not null default 0 comment '用户模式：0:非vip用户；1:非vip自动续费用户；2:自动续费用户',
	fee_flag int unsigned not null default 0 comment '最后一次充值充值渠道ID',
	curr_time datetime not null default 0 comment '最近一次操作时间',
	vip_type tinyint unsigned not null comment '用户vip类型：bit1:普通vip,bit2:年费vip',
	ct_vip_type tinyint unsigned not null default 0 comment '赤瞳vip类型：1:白银vip;2:黄金vip',
    primary key (user_id, u_create_tm, ct_vip_type)
) ENGINE = INNODB CHARSET = UTF8; 
"
echo "vip_user_info_table ok"

mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
SET NAMES UTF8;
CREATE DATABASE IF NOT EXISTS dplan_other_db;
USE dplan_other_db;
CREATE TABLE IF NOT EXISTS dplan_other_db.vip_op_trans_table
(
	op_id int unsigned not null auto_increment primary key comment '日志ID自增量',
	user_id int unsigned not null default 0 comment '用户米米号',
	u_create_tm int unsigned not null default 0 comment '玩家创建角色的时间戳',
	server_num int unsigned not null default 1 comment '区服',
	cmd_id int unsigned not null default 0 comment '协议命令ID',
	trade_id int unsigned not null default 0 comment '该操作发生的交易号,不要重复',
	apply_time timestamp not null default '0000-00-00 00:00:00' comment '操作时间',
	begin_time datetime not null default 0 comment '上一次操作会员有效开始时间',
	end_time datetime not null default 0 comment '上一次操作会员有效到期时间',
	time_flag int unsigned not null default 0 comment '1:非自动续费，2:自动续费',
	fee_flag int unsigned not null default 0 comment '用户充值渠道ID',
	action_type tinyint unsigned not null default 0 comment '操作类型',
	time_length smallint unsigned not null default 0 comment '对过期时间操作的天数',
	vip_type tinyint unsigned not null default 0 comment '用户vip类型：bit1:普通vip,bit2:年费vip',
	ct_vip_type tinyint unsigned not null default 0 comment '赤瞳vip类型：1:白银vip;2:黄金vip'
) ENGINE = INNODB CHARSET = UTF8; 
"
echo "vip_op_log_table ok"

#//100 nick tables on dplan_other_db
for i in {0..99}
do
    table_no=`printf '%02d' $i`
    mysql -udplan -pdplan@0601 -e "
    SET NAMES UTF8;
    CREATE TABLE IF NOT EXISTS dplan_other_db.nick_table_${table_no} 
    (
        nick  VARCHAR(16) NOT NULL DEFAULT '' PRIMARY KEY,
        userid  INT UNSIGNED NOT NULL,
        u_create_tm INT UNSIGNED NOT NULL DEFAULT 0
    ) ENGINE = INNODB CHARSET = UTF8;"
    echo   "nick_table: table_no:${table_no} ok"
done

#family_table
#建立家族id表
#dplan_other_db.family_id_table 192.168.39.15
mysql -udplan -pdplan@0601 -h10.1.1.154 -e "CREATE DATABASE IF NOT EXISTS dplan_other_db"
mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
SET NAMES UTF8;
CREATE TABLE IF NOT EXISTS dplan_other_db.family_id_table
(
    family_id INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '家族id',
    server_id INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '所在服务器ID',
    PRIMARY KEY (family_id)
 ) ENGINE = INNODB  AUTO_INCREMENT=100000 CHARSET = UTF8;
"

#建立家族信息表
#100 db 10 tables;
#dplan_family_db_00-99.family_info_table_0-9 
#00-49 192.168.39.15 50-99 192.168.39.85
for i in {0..99}
do
    db_no=`printf '%02d' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "CREATE DATABASE IF NOT EXISTS dplan_family_db_${db_no}"
    for j in {0..9}
    do
        table_no=`printf '%01d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
        SET NAMES UTF8;
        CREATE TABLE IF NOT EXISTS dplan_family_db_${db_no}.family_info_table_${table_no}
        (
         family_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族id',
         family_name VARCHAR(50) NOT NULL DEFAULT '' COMMENT '家族名',
         construct_value INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族建设值',
         level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族等级',
         pet_name VARCHAR(50) NOT NULL DEFAULT '' COMMENT '家族精灵名',
         pet_level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族精灵等级',
         member_num INT NOT NULL DEFAULT 0 COMMENT '家族总成员数',
         create_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族创建时间',
         dismiss_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族解散时间',
         status INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族状态',
         join_type INT NOT NULL DEFAULT 0 COMMENT '加盟条件类型',
         base_join_value INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '加入战力要求',
         creator_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族创建者',
         u_create_tm INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族创建者的角色创建时间',
         board_msg VARCHAR(500) NOT NULL DEFAULT '' COMMENT '家族公告',
         last_member_login_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '最近一次成员登陆时间',
         PRIMARY KEY (family_id),index(family_name)
        ) ENGINE = INNODB CHARSET = UTF8;
        "
        echo $db_no.family_info_$table_no ok
    done
done


#建立家族成员表
#dplan_family_db_00-99.family_member_table_0-9
#100 db 10 tables
#00-49 39.15 50-99 39.85
for i in {0..99}
do
    db_no=`printf '%02d' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "CREATE DATABASE IF NOT EXISTS dplan_family_db_${db_no}"
    for j in {0..9}
    do
        table_no=`printf '%01d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
        SET NAMES UTF8;
        CREATE TABLE IF NOT EXISTS dplan_family_db_${db_no}.family_member_table_${table_no}
        (
         family_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族id',
         userid INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '成员米米号',
         u_create_tm INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '成员的角色创建时间',
         title INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '成员头衔',
         status INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '成员状态',
         join_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '加入时间',
         left_construct_value INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '剩余建设值',
         total_construct_value INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '累计建设值',
         last_login_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '上次登录时间',
         last_logout_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '上次登出时间',
         battle_value INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '个人战力',
         PRIMARY KEY (family_id, userid)
        ) ENGINE = INNODB CHARSET = UTF8;
        "
        echo $db_no.family_member_table_$table_no ok
    done
done


#建立家族事件表
#dplan_family_db_00-99.family_event_table_0-9
for i in {0..99}
do
    db_no=`printf '_%02d' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "CREATE DATABASE IF NOT EXISTS dplan_family_db${db_no}"
    for j in {0..9}
    do
        table_no=`printf '_%01d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
        SET NAMES UTF8;
        CREATE TABLE IF NOT EXISTS dplan_family_db${db_no}.family_event_table${table_no}
        (
        	family_id INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '家族id',
        	userid INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '事件目标用户id',
            u_create_tm INT UNSIGNED NOT NULL DEFAULT 0,
        	event_type INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '事件类型',
        	event_status INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '事件处理结果',
        	event_time INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '事件发生时间戳',

        	PRIMARY KEY (family_id,userid,event_type)
        ) ENGINE = INNODB CHARSET = UTF8;
        "
    
        echo dplan_family_db$db_no.family_event_table$table_no ok
    done
done

#建立家族日志表 
#dplan_family_db_00-99.family_log_table_00-99
for i in {0..99}
do
    db_no=`printf '%02d' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "CREATE DATABASE IF NOT EXISTS dplan_family_db_${db_no}"
    for j in {0..99}
    do
        table_no=`printf '%02d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
        SET NAMES UTF8;
        CREATE TABLE IF NOT EXISTS dplan_family_db_${db_no}.family_log_table_${table_no}
        (
         log_id INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '日志id',
         family_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '家族id',
         log_type INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '日志类型',
         log_msg VARCHAR(500) NOT NULL DEFAULT '' COMMENT '日志内容',
         log_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '日志时间戳',
         PRIMARY KEY (log_id),index(family_id),index(log_time)
        ) ENGINE = INNODB CHARSET = UTF8;
        "
        echo $db_no.family_log_table_$table_no ok
    done
done

#建立家族匹配信息表
for i in {0..0}
do
    db_no=`printf '' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "CREATE DATABASE IF NOT EXISTS dplan_other_db${db_no}"
    for j in {0..0}
    do
        table_no=`printf '' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
        SET NAMES UTF8;
        CREATE TABLE IF NOT EXISTS dplan_other_db${db_no}.family_match_info_table${table_no}
        (
    
        	family_id INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '家族id',
            server_id INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '所在服务器ID',
        	family_name VARCHAR(50)   NOT NULL  DEFAULT ''   COMMENT  '家族名字',
        	total_battle_value INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '总战力',
        	member_num INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '成员数量',
        	family_level INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '家族等级',
        	join_type INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '加入条件类型',
        	base_join_value INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '加入战力要求',
        	is_full INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '是否满员',
        	create_time INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '创建时间',

        	PRIMARY KEY (family_id, server_id),INDEX(base_join_value),INDEX(create_time)
        ) ENGINE = INNODB CHARSET = UTF8;
        "
    
        echo dplan_other_db$db_no.family_match_info_table$table_no ok
    done
done
 
#global_attr_table
#建立全局属性表
mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
CREATE DATABASE IF NOT EXISTS dplan_other_db;
USE dplan_other_db;
CREATE TABLE IF NOT EXISTS dplan_other_db.global_attr_table
(
    server_id INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '所在服务器ID',
    type INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '类型', 
    subtype INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '子类型',
    value INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '数值',

    PRIMARY KEY (server_id, type, subtype)

) ENGINE = INNODB CHARSET = UTF8;

"
echo "global_attr_table ok"

#procedure proc_add_global_attr_table
#建立全局属性update存储过程
mysql -udplan -pdplan@0601 -h10.1.1.154  < proc_add_global_attr.sql
echo "create procedure  create_proc_add_global_attr succ!"


#兑换码礼包
mysql -udplan -pdplan@0601 -e "
SET NAMES UTF8;
DROP TABLE IF EXISTS dplan_other_db.gift_code_table;
CREATE TABLE IF NOT EXISTS dplan_other_db.gift_code_table 
(
    name  VARCHAR(32) NOT NULL DEFAULT '' COMMENT '兑换码名称',
    type INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '兑换码类型',
    code VARCHAR(32) NOT NULL DEFAULT '' COMMENT '兑换码',
    prize_id INT NOT NULL DEFAULT 0 COMMENT '兑换码对应的奖励ID',
    start TIMESTAMP NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '起始使用时间',
    end TIMESTAMP NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '过期时间',
    userid INT NOT NULL DEFAULT 0 COMMENT '使用者ID',
    u_create_tm INT NOT NULL DEFAULT 0 COMMENT '使用者的角色创建时间',
    on_server_id INT NOT NULL DEFAULT 1 COMMENT '在哪个服被用掉',
    used_tm TIMESTAMP NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '被使用的时间',
    status INT NOT NULL DEFAULT 0 COMMENT '兑换码状态 0:正常 1:预删除',

    PRIMARY KEY(code)

 ) ENGINE = INNODB CHARSET = UTF8;"

