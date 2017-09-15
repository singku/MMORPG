#!/bin/sh
for i in {0..99}
do
    db_no=`printf '_%02d' $i`
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "drop database dplan_family_db${db_no}"
    echo "drop database dplan_family_db${db_no}"
done