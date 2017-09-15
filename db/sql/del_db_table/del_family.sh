for i in {0..99}
do
    db_no=`printf '%02d' $i`
    for j in {0..9}
    do
        table_no=`printf '%01d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop table dplan_family_db_${db_no}.family_info_table_${table_no}"
        echo dplan_family_db_${db_no}.family_info_table_${table_no} ok
    done
done

for i in {0..99}
do
    db_no=`printf '%02d' $i`
    for j in {0..9}
    do
        table_no=`printf '%01d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop table dplan_family_db_${db_no}.family_member_table_${table_no}"
        echo dplan_family_db_${db_no}.family_member_table_${table_no} ok
    done
done

#删除旧分表
for i in {0..9}
do
    db_no=`printf '%01d' $i`
    for j in {0..99}
    do
        table_no=`printf '%02d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop table dplan_family_db_${db_no}.family_member_table_${table_no}"
        echo dplan_family_db_${db_no}.family_member_table_${table_no} ok
    done
done



for i in {0..99}
do
    db_no=`printf '%02d' $i`
    for j in {0..9}
    do
        table_no=`printf '%01d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop table dplan_family_db_${db_no}.family_event_table_${table_no}"
        echo dplan_family_db_${db_no}.family_event_table_${table_no} ok
    done
done

for i in {0..99}
do
    db_no=`printf '%02d' $i`
    for j in {0..99}
    do
        table_no=`printf '%02d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop table dplan_family_db_${db_no}.family_log_table_${table_no}"
        echo drop table dplan_family_db_${db_no}.family_log_table_${table_no} ok
    done
done

mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop table dplan_other_db.family_id_table"

for i in {0..99}
do
    db_no=`printf '%02d' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop database dplan_family_db_${db_no}"
done


