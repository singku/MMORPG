#!/bin/sh

from_server=$1
to_server=$2

#清原服排行
echo "
SELECT $1
KEYS *" | redis-cli | 
while read row;do
    echo "select $1
    DEL $row" | redis-cli >/dev/null
done

#清本服排行
echo "
SELECT $2
KEYS *" | redis-cli | 
while read row;do
    echo "select $2
    DEL $row" | redis-cli >/dev/null
done

#玩家数据合并
for i in {0..99}
do
    db_no=`printf '%02d' $i`
    for j in {0..99}
    do
        table_no=`printf '%02d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
        update dplan_db_${db_no}.base_info_table_${table_no} set server_id = $2;"
    done
done

#家族合并
mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
update dplan_other_db.family_match_info_table set server_id = $2;
update dplan_other_db.family_id_table set server_id = $2;
"

