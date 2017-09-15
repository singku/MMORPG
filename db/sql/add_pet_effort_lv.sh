for i in {0..99}
do
	db_no=`printf '%02d' $i`
	for j in {0..99}
	do
		table_no=`printf '%02d' $j`
	mysql -udplan -pdplan@0601 -h10.1.1.154 -e "alter table dplan_db_${db_no}.pet_table_${table_no} add effort_hp_lv INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力生命等级';"
	mysql -udplan -pdplan@0601 -h10.1.1.154 -e "alter table dplan_db_${db_no}.pet_table_${table_no} add effort_normal_atk_lv INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力普攻等级';"
	mysql -udplan -pdplan@0601 -h10.1.1.154 -e "alter table dplan_db_${db_no}.pet_table_${table_no} add effort_normal_def_lv INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力普防等级';"
	mysql -udplan -pdplan@0601 -h10.1.1.154 -e "alter table dplan_db_${db_no}.pet_table_${table_no} add effort_skill_atk_lv INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力技攻等级';"
	mysql -udplan -pdplan@0601 -h10.1.1.154 -e "alter table dplan_db_${db_no}.pet_table_${table_no} add effort_skill_def_lv INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '学习力技防等级';"
	done
	echo $db_no.$table_no ok
done
