#!/bin/sh

uid=$1;
svrid=$2;

if [[ $uid == "" ]] 
    then exit 
fi

if [[ $svrid == "" ]] 
    then exit 
fi

((db_no = $uid % 100 ));
((table_no = ( $uid / 100 ) % 100 ));

db_no=`printf '%02d' $db_no`;
table_no=`printf '%02d' $table_no`;

tm=`mysql -udplan -pdplan@0601 -h10.1.1.154 --skip-column-names -e "
    select u_create_tm from dplan_db_${db_no}.base_info_table_${table_no} where userid = ${uid} and init_server_id = ${svrid};
"`
echo $tm
