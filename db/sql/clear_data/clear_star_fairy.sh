#!/bin/sh

##清除全服数据
#for i in {0..99}
#do
    #db_no=`printf '%02d' $i`
    #for j in {0..99}
    #do
        ##修改位置
        #for dup_id in {301..303}
        #do
            #attr1=$(( 750 + 4 * ( $dup_id - 1 ) + 1))
            #attr2=$(( 750 + 4 * ( $dup_id - 1 ) + 2))
            #attr3=$(( 750 + 4 * ( $dup_id - 1 ) + 3))
            #attr4=$(( 750 + 4 * ( $dup_id - 1 ) + 4))

            #table_no=`printf '%02d' $j`
            #mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_db_${db_no}.attr_table_${table_no} where type in ($attr1, $attr2, $attr3, $attr4) "

            #echo "delete from dplan_db_${db_no}.attr_table_${table_no} where type in ($attr1, $attr2, $attr3, $attr4) and userid in (101076) "
        #done
        #echo dplan_db_${db_no}.attr_table_${table_no} ok
    #done
#done

#清除指定玩家数据 
users=(101076  101077 101078 100024)
for u in ${users[*]}
do
    echo $u
    db_no=$(($u % 100))
    db_no=`printf '%02d' $db_no`
    table_no=$((($u / 100) % 100))
    table_no=`printf '%02d' $table_no`

    #修改位置
    for dup_id in {801..815}
    do
        attr1=$(( 750 + 4 * ( $dup_id - 1 ) + 1))
        attr2=$(( 750 + 4 * ( $dup_id - 1 ) + 2))
        attr3=$(( 750 + 4 * ( $dup_id - 1 ) + 3))
        attr4=$(( 750 + 4 * ( $dup_id - 1 ) + 4))
        attr5=252

        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "delete from dplan_db_${db_no}.attr_table_${table_no} where type in ($attr1, $attr2, $attr3, $attr4, $attr5, 11012, 11013, 255, 256, 257) and userid = $u"

        echo "delete from dplan_db_${db_no}.attr_table_${table_no} where type in ($attr1, $attr2, $attr3, $attr4, $attr5) and userid = $u "

        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "insert into dplan_db_${db_no}.attr_table_${table_no} values (101076, 11011, 3) on duplicate key update value = 3"
    done
    echo dplan_db_${db_no}.attr_table_${table_no} ok

done


#清除redis数据
CUR_PATH=`pwd`
echo $CUR_PATH

REDIS_PATH="/home/toby/dplan/servers/redis-2.8.13/src"
cd $REDIS_PATH

key_file="$CUR_PATH/redis_sets"
t_key_file="$CUR_PATH/t_redis_sets"
echo "KEYS *" | ./redis-cli > $key_file

#修改位置
cat $key_file | grep "rankings:3:\|rankings:2:" > $t_key_file

cat $t_key_file | xargs ./redis-cli del
cat $t_key_file

echo "delete done"

rm $key_file -f
rm $t_key_file -f
