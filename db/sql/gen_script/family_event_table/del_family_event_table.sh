#!/bin/sh
for i in {0..99}
do
    db_no=`printf '_%02d' $i`
    
    for j in {0..9}
    do
        table_no=`printf '_%01d' $j`
        mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop table dplan_family_db${db_no}.family_event_table${table_no}"
        echo dplan_family_db$db_no.family_event_table$table_no ok
    done
done
    