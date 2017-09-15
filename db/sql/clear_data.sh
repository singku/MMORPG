for i in {0..99}
do
    db_no=`printf '%02d' $i`
    for j in {0..99}
    do
        table_no=`printf '%02d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_db_${db_no}.base_info_table_${table_no}"
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_db_${db_no}.item_table_${table_no}"
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_db_${db_no}.pet_table_${table_no}"
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_db_${db_no}.attr_table_${table_no}"
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_db_${db_no}.mail_table_${table_no}"
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_db_${db_no}.rune_table_${table_no}"
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_db_${db_no}.task_table_${table_no}"
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_db_${db_no}.friend_table_${table_no}"
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_db_${db_no}.buff_table_${table_no}"
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_db_${db_no}.home_visit_log_table_${table_no}"

    echo $db_no.$table_no ok
    done
done

mysql -udplan -pdplan@0601 -h10.1.1.154 -e " truncate table dplan_other_db.transaction_table"
for i in {0..99}
do
    table_no=`printf '%02d' $i`;
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
    truncate table dplan_other_db.nick_table_${table_no};"
done


