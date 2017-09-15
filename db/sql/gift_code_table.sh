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

