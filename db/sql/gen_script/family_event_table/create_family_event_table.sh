#!/bin/sh
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

        	event_type INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '事件类型',

        	event_status INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '事件处理结果',

        	event_time INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '事件发生时间戳',

        	PRIMARY KEY (family_id,userid,event_type)
        ) ENGINE = INNODB CHARSET = UTF8;
        "
    
        echo dplan_family_db$db_no.family_event_table$table_no ok
    done
done
    
