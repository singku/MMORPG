#!/bin/sh
for i in {0..99}
do
    db_no=`printf '_%02d' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "CREATE DATABASE IF NOT EXISTS dplan_family_db${db_no}"
    for j in {0..99}
    do
        table_no=`printf '_%02d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
        SET NAMES UTF8;
        CREATE TABLE IF NOT EXISTS dplan_family_db${db_no}.family_log_table${table_no}
        (
    
        	log_id INT(11) UNSIGNED  NOT NULL  DEFAULT 0 AUTO_INCREMENT  COMMENT  '日志id',

        	family_id INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '家族id',

        	log_type INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '日志类型',

        	log_msg VARCHAR(500)   NOT NULL  DEFAULT ''   COMMENT  '日志内容',

        	log_time INT(11) UNSIGNED  NOT NULL  DEFAULT 0   COMMENT  '日志发生时间戳',

        	PRIMARY KEY (log_id),INDEX(family_id),INDEX(log_time)
        ) ENGINE = INNODB CHARSET = UTF8;
        "
    
        echo dplan_family_db$db_no.family_log_table$table_no ok
    done
done
    
