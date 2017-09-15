for i in {0..99}
do
    db_no=`printf '%02d' $i`
    for j in {0..99}
    do
        table_no=`printf '%02d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop table dplan_db_${db_no}.mail_table_${table_no}"
    echo $db_no.$table_no ok
    done
done
