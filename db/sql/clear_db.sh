for i in {0..99}
do
    db_no=`printf '%02d' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "DROP DATABASE dplan_db_${db_no}"
done

mysql -udplan -pdplan@0601 -h10.1.1.154 -e "DROP DATABASE dplan_other_db"

for i in {0..99}
do
    db_no=`printf '%02d' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "DROP DATABASE dplan_family_db_${db_no}"
done

