#!/bin/sh

./rank_rm.sh $1 $2

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

if [[ $tm == "" ]]
     then exit
fi

family_id=`mysql -udplan -pdplan@0601 -h10.1.1.154 --skip-column-names -e "
    select value from dplan_db_${db_no}.attr_table_${table_no} where userid = ${uid} and u_create_tm = ${tm} and type = 701;
"`

if [[ $family_id == "" ]]
    then family_id=0;
fi

((family_db_no = $family_id % 100 ));
((family_table_no = ( $family_id / 1000 ) % 10 ));

family_db_no=`printf '%02d' $family_db_no`;
family_table_no=`printf '%01d' $family_table_no`;

mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
    delete from dplan_db_${db_no}.base_info_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};
    delete from dplan_db_${db_no}.item_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};
    delete from dplan_db_${db_no}.pet_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};
    delete from dplan_db_${db_no}.attr_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};
    delete from dplan_db_${db_no}.mail_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};
    delete from dplan_db_${db_no}.rune_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};
    delete from dplan_db_${db_no}.friend_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};
    delete from dplan_db_${db_no}.task_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};
    delete from dplan_db_${db_no}.buff_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};
    delete from dplan_db_${db_no}.home_visit_log_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};
    delete from dplan_other_db.transaction_table where account_id = ${uid} and s_create_tm = ${tm};
"

for i in {0..99}
do
    table_no=`printf '%02d' $i`;
    mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
    delete from dplan_other_db.nick_table_${table_no} where userid = ${uid} and u_create_tm = ${tm};"
done

mysql -udplan -pdplan@0601 -h10.1.1.154 -e "
    delete from dplan_family_db_${family_db_no}.family_member_table_${family_table_no} where userid= ${uid} and u_create_tm=${tm};
    delete from dplan_family_db_${family_db_no}.family_event_table_${family_table_no} where userid= ${uid} and u_create_tm=${tm};
    delete from dplan_family_db_${family_db_no}.family_log_table_${family_table_no} where userid= ${uid} and u_create_tm=${tm};
"
echo $?;
