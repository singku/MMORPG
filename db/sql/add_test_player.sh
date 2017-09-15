for uid in {10000..19999}
do
    table_no=$[uid/100%100];
    table_no=`printf '%02d' $table_no`;
    db_no=$[uid%100];
    nick="nick_$uid";
    db_no=`printf '%02d' $db_no`;
    tactic_table_no=$[uid/1000%10];

mysql -uroot -pta0mee -e "

REPLACE INTO dplan_db_$db_no.base_info_table_$table_no
 (userid, nick) values ($uid, \"$nick\");

";

done
