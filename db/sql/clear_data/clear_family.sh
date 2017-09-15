#!/bin/sh

#清除全服数据
for i in {0..99}
do
    db_no=`printf '%02d' $i`
    for j in {0..99}
    do
        #修改位置
        #break;
        table_no=`printf '%02d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_db_${db_no}.attr_table_${table_no} where type >= 701 and type <= 720"
        echo "delete from dplan_db_${db_no}.attr_table_${table_no} where type >= 701 and type <= 720 ok"

        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_family_db_${db_no}.family_log_table_${table_no}"
        echo "delete from dplan_family_db_${db_no}.family_log_table_${table_no} ok"

        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_db_${db_no}.buff_table_${table_no} where buff_type = 4"
        echo "delete from dplan_db_${db_no}.buff_table_${table_no} where buff_type = 4 ok"
    done

    for j in {0..9}
    do
        table_no=`printf '%01d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_family_db_${db_no}.family_info_table_${table_no}"
        echo "delete from dplan_family_db_${db_no}.family_info_table_${table_no} ok"

        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_family_db_${db_no}.family_event_table_${table_no}"
        echo "delete from dplan_family_db_${db_no}.family_event_table_${table_no} ok"

        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_family_db_${db_no}.family_member_table_${table_no}"
        echo "delete from dplan_family_db_${db_no}.family_member_table_${table_no} ok"
    done
done

##清除指定玩家数据 
##users=(101076  101077 101078)
#users=(101077)
#for u in ${users[*]}
#do
    #for family_id in {100000..100100}
    #do
        #db_no=$(($family_id % 100))
        #table_no=$((($family_id / 100) % 100))

        #db_no=`printf '%02d' $db_no`
        #table_no=$(($table_no /10))

        #mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_family_db_${db_no}.family_info_table_${table_no} where family_id = $family_id"
        #echo "delete from dplan_family_db_${db_no}.family_info_table_${table_no} where family_id = $family_id"
        #echo dplan_family_${db_no}.family_info_table_${table_no} ok

##mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_family_db_${db_no}.family_member_table_${table_no} where family_id = $family_id and userid = $u"
        #echo "delete from dplan_family_db_${db_no}.family_member_table_${table_no} where family_id = $family_id and userid = $u"
        #echo dplan_family_${db_no}.family_member_table_${table_no} ok

##mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_family_db_${db_no}.family_event_table_${table_no} where family_id = $family_id and userid = $u"
        #echo "delete from dplan_family_db_${db_no}.family_event_table_${table_no} where family_id = $family_id and userid = $u"
        #echo dplan_family_db_${db_no}.family_event_table_${table_no} ok


    #done
#done

#for u in ${users[*]}
#do
    #db_no=$(($u % 100))
    #table_no=$((($u / 100) % 100))

    #db_no=`printf '%02d' $db_no`
    #table_no=`printf '%02d' $table_no`

    #mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_db_${db_no}.attr_table_${table_no} where type >= 701 and type <= 720 and userid = $u"
    #echo "delete from dplan_db_${db_no}.attr_table_${table_no} where type in (701) and userid = $u "
    #echo dplan_db_${db_no}.attr_table_${table_no} ok
#done

CUR_PATH=`pwd`
echo $CUR_PATH

REDIS_PATH="/home/toby/dplan/servers/redis-2.8.13/src"
cd $REDIS_PATH

key_file="$CUR_PATH/redis_sets"
t_key_file="$CUR_PATH/t_redis_sets"
cmd_file="$CUR_PATH/cmd_file"

for ((i=0;i<1024;i++));
do
    echo "select $i" > $cmd_file
    echo "keys *" >> $cmd_file
    cat $cmd_file | ./redis-cli > $key_file
    cat $key_file | grep "rankings:5\|hashset:1:\|hashset:2:\|hashset:3\|hashset:4\|set:1:\|set:2:\|set:3:\|set:4:\|string:1\|family" > $t_key_file
    cat $t_key_file | xargs ./redis-cli -n $i del
    cat $t_key_file
done

mysql -udplan -pdplan@0601 -h10.1.1.154 -e "truncate dplan_other_db.family_id_table;alter table dplan_other_db.family_id_table auto_increment = 100000"

mysql -udplan -pdplan@0601 -h10.1.1.154 -e "truncate dplan_other_db.family_match_info_table;"
echo "delete from dplan_other_db.family_match_info_table ok"

echo "delete done"

