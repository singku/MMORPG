USE dplan_other_db;
DELIMITER //
DROP PROCEDURE IF EXISTS proc_add_global_attr; 
CREATE procedure proc_add_global_attr(
	IN server_in int,  #对应的服 
	IN key_in int,     #对应type全局属性
	IN subkey_in int,  #对应subtype,为0表示每小时限量，非0为time_configid
	IN diff_in int,    #全局变量拟修改值
	IN limit_in int,   #全局变量上限
	OUT ret_out int    #返回值1修改成功，-1修改失败
)
BEGIN
	DECLARE get_val int default 0;
	DECLARE EXIT HANDLER FOR SQLEXCEPTION ROLLBACK;

	START TRANSACTION;

	INSERT INTO global_attr_table
	(server_id, type, subtype, value)
	VALUES (server_in, key_in, subkey_in, diff_in)
	ON DUPLICATE KEY UPDATE
	value = value + diff_in;

	SELECT value
	INTO get_val 
	FROM global_attr_table 
	WHERE type =key_in AND subtype=subkey_in AND server_id = server_in;

	IF get_val > limit_in THEN
		BEGIN
			SET ret_out = -1;
			ROLLBACK;
		END;
	ELSE
		BEGIN
			SET ret_out = 1;
			COMMIT;
		END;
	END IF;
END  
//
DELIMITER ;
