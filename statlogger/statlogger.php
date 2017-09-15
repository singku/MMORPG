<?php


/**
 * @brief stat_reg_role 注册角色
 *
 * @param $gameid      游戏ID
 * @param $platformid  平台ID(不分平台填-1)
 * @param $zoneid       区号(不分区服时填-1)
 * @param $serverid     服号(不分区服时填-1)
 * @param $acct_id      帐号
 * @param $device       设备
 *
 * @return
 */
function stat_reg_role($gameid, $platformid, $zoneid, $serverid, $acct_id, $device)
{
    $hip = get_local_public_ip();
    if(empty($hip))
        $hip = "0";
	$curtime = time();
    $ret = true;

        $totalcontent = "_hip_=".$hip."\t_stid_=_newac_\t_sstid_=_newac_\t_gid_=".$gameid."\t_zid_=".$zoneid."\t_sid_=".$serverid."\t_pid_=".$platformid."\t_ts_=".$curtime."\t_acid_=".$acct_id."\t_plid_=-1";
    if(strlen($device))
    {
        $op = "\t_op_=item:_dev_";
        $totalcontent = $totalcontent.$op."\n";
    }
    else
    {
        $totalcontent = $totalcontent."\n";
    }

	    $ret = write_basic_log($gameid, $totalcontent);
        if($ret = false)
        {
            return false;
        }

        return true;
}

/**
 * @brief stat_login_online 活跃
 *
 * @param $gameid      游戏ID
 * @param $platformid  平台ID(不分平台填-1)
 * @param $zoneid       区号(不分区服时填-1)
 * @param $serverid     服号(不分区服时填-1)
 * @param $acct_id      帐号
 * @param $device       设备
 *
 * @return
 */
function stat_login_online($gameid, $platformid, $zoneid, $serverid, $acct_id, $device)
{
    $hip = get_local_public_ip();
    if(empty($hip))
        $hip = "0";
	$curtime = time();
    $ret = true;

        $totalcontent = "_hip_=".$hip."\t_stid_=_lgac_\t_sstid_=_lgac_\t_gid_=".$gameid."\t_zid_=".$zoneid."\t_sid_=".$serverid."\t_pid_=".$platformid."\t_ts_=".$curtime."\t_acid_=".$acct_id."\t_plid_=-1";
    if(strlen($device))
    {
        $op = "\t_op_=item:_dev_";
        $totalcontent = $totalcontent.$op."\n";
    }
    else
    {
        $totalcontent = $totalcontent."\n";
    }

	    $ret = write_basic_log($gameid, $totalcontent);
        if($ret = false)
        {
            return false;
        }

        return true;

}

/**
 * helper functions
 */

function is_utf8($str)
{
    $len = strlen($str);
    for($i = 0; $i < $len; $i++)
    {
        $c = ord($str[$i]);
        if ($c > 128)
        {
            if (($c > 247))
                return false;
            elseif ($c > 239)
                $bytes = 4;
            elseif ($c > 223)
                $bytes = 3;
            elseif ($c > 191)
                $bytes = 2;
            else
                return false;

            if (($i + $bytes) > $len)
                return false;

            while ($bytes > 1)
            {
                $i++;
                $b = ord($str[$i]);
                if ($b < 128 || $b > 191)
                    return false;
                $bytes--;
            }
        }
    }

    return true;
}

function get_local_public_ip()
{
    return $_SERVER['SERVER_ADDR'];
}

//stat_pay(10, -1, -1, -1, "47159775", 0, 1000, 0, "_buyitem_", 1023, 10, "dfdfg");
//stat_pay(10, -1, -1, -1, 234543475, 0, 1000, 0, "bu", 1023, 10, "");
//stat_pay(10, -1, -1, -1, "47159775", 0, 1000, 0, "_vipmonth_", "2个月", 2, "短信");
//stat_pay(10, -1, -1, -1, "47159775", 0, 1000, 1, "_buycoins_", "金币", 10, "财付通");
//stat_logger(13, -1, -1, -1, "47159775", -1, "怪兽个数统计", "4");

/**
 * 用户付费时调用:开通VIP或 购买游戏内道具
 * gameid 游戏ID
 * platformid 平台ID 默认填写-1  表示全平台，如果一个游戏需要放到不同的平台运营，这里就需要填写对应的平台ID，需要与游戏内部保持一致
 * zoneid 游戏区ID 默认填写-1
 * serverid 游戏服ID 默认填写-1
 * acctid 用户帐号ID
 * isvip 是否VIP用户 0：不是  1：是
 * pay_amount 本次付费金额
 * currency 本次付费的货币单位 0:米币 1：人民币
 * pay_reason  可取值 _buyitem_:通过米币购买游戏内道具 _vipmonth_:开通游戏内VIP包月 _buycoins_:购买游戏内货币 _costfree_:系统赠送
 * output 付费购买的产出物  购买道具时为具体的道具ID或名称   开通VIP时为具体的VIP信息，比如1个月 3个月  购买游戏币时为游戏币一级货币的名称
 * outcnt 付费购买的产出物数量  购买道具时为道具的数量 开通VIP时为具体的开通时长  购买游戏币时为购买的游戏币数量
 * pay_channel 付费渠道  支付宝 财付通等
 *
 */
function stat_pay($gameid, $platformid, $zoneid, $serverid, $acct_id, $isvip, $pay_amount, $currency, $pay_reason, $output, $outcnt, $paychannel)
{
    $hip = get_local_public_ip();
    if(empty($hip))
        $hip = "0";

    $ret = true;
	if( ($isvip !=0 && $isvip != 1) ||
	    ($currency !=0  && $currency != 1) ||
	    ($pay_reason != "_buyitem_" && $pay_reason != "_vipmonth_" && $pay_reason != "_buycoins_" && $pay_reason != "_costfree_")
    	  )
	{
		write_error_log($gameid, "pay function para not right: isvip =".$isvip."currency=".$currency."reason=".$pay_reason."\n");
		return false;
	}
	$curtime = time();

	$op = "\t_op_=sum:_amt_|item_sum:_vip_,_amt_|item_sum:_ccy_,_amt_";
	$paracontent = "\t_vip_=".$isvip."\t_amt_=".$pay_amount."\t_ccy_=".$currency;

    if(strlen($paychannel))
    {
        $op .= "|item_sum:_paychannel_,_amt_|item:_paychannel_";
        $paracontent .= "\t_paychannel_=".$paychannel;
    }
    if($pay_reason != "_costfree_")
    {
        $totalcontent = "_hip_=".$hip."\t_stid_=_acpay_\t_sstid_=_acpay_\t_gid_=".$gameid."\t_zid_=".$zoneid."\t_sid_=".$serverid."\t_pid_=".$platformid."\t_ts_=".$curtime."\t_acid_=".$acct_id."\t_plid_=-1".$paracontent.$op."\n";
	    $ret = write_basic_log($gameid, $totalcontent);
        if($ret = false)
        {
            return false;
        }
    }

	$divcontent = "_hip_=".$hip."\t_stid_=_acpay_\t_sstid_=".$pay_reason."\t_gid_=".$gameid."\t_zid_=".$zoneid."\t_sid_=".$serverid."\t_pid_=".$platformid."\t_ts_=".$curtime."\t_acid_=".$acct_id."\t_plid_=-1".$paracontent.$op."\n";
	$ret = write_basic_log($gameid, $divcontent);
    if($ret = false)
    {
        return false;
    }


	switch($pay_reason)
	{
	case "_buyitem_"://购买道具
		$opitem = "\t_op_=sum:_golds_";
		$para = "\t_isvip_=".$isvip."\t_item_=".$output."\t_itmcnt_=".$outcnt."\t_golds_=".$pay_amount;
		$content = "_hip_=".$hip."\t_stid_=_buyitem_\t_sstid_=_mibiitem_\t_gid_=".$gameid."\t_zid_=".$zoneid."\t_sid_=".$serverid."\t_pid_=".$platformid."\t_ts_=".$curtime."\t_acid_=".$acct_id."\t_plid_=-1".$para.$opitem."\n";
	 	$ret = write_basic_log($gameid,$content);
		break;
	case "_vipmonth_"://开通VIP包月
		$opitem = "\t_op_=item:_amt_|item_sum:_amt_,_payamt_";
		$para = "\t_amt_=".$outcnt."\t_payamt_=".$pay_amount;
		$content = "_hip_=".$hip."\t_stid_=_buyvip_\t_sstid_=_buyvip_\t_gid_=".$gameid."\t_zid_=".$zoneid."\t_sid_=".$serverid."\t_pid_=".$platformid."\t_ts_=".$curtime."\t_acid_=".$acct_id."\t_plid_=-1".$para.$opitem."\n";
	 	$ret = write_basic_log($gameid,$content);
		break;
	case "_buycoins_"://购买游戏内一级货币
		$opitem = "\t_op_=sum:_golds_";
		$para = "\t_golds_=".$outcnt;
		$content = "_hip_=".$hip."\t_stid_=_getgolds_\t_sstid_=_userbuy_\t_gid_=".$gameid."\t_zid_=".$zoneid."\t_sid_=".$serverid."\t_pid_=".$platformid."\t_ts_=".$curtime."\t_acid_=".$acct_id."\t_plid_=-1".$para.$opitem."\n";
	 	$ret = write_basic_log($gameid, $content);
		break;
	}

	return $ret;

}

/**
 * 自定义统计项接口 目前只支持人数人次
 * playerid 为角色ID ，默认填写-1即可
 * gameid 游戏ID
 * platformid 平台ID 默认填写-1  表示全平台，如果一个游戏需要放到不同的平台运营，这里就需要填写对应的平台ID，需要与游戏内部保持一致
 * zoneid 游戏区ID 默认填写-1
 * serverid 游戏服ID 默认填写-1
 * acctid 用户帐号ID
 * playerid 用户角色ID 不需要分角色时填写-1即可
 * stname 统计项名称
 * sstname 子统计项名称
 * 下面内容请忽略
 * array 自定义key-value数组, 数组内容格式必须为$arr = array("peter"=>32, "jone"=>34); 其中peter和jone为key 32 34为value
 * key不能以_开头和结尾 不可以包含 = : , | \t  ; 中的任意一个字符
 * value 也不可以包含 = : , | \t  ; 中的任意一个字符
 * */
function stat_logger($gameid, $platformid, $zoneid, $serverid, $acct_id, $playerid, $stname, $sstname)
{
    $ret = true;
    $hip = get_local_public_ip();
    if(empty($hip))
        $hip = "0";

    if(strlen($stname) && strlen($sstname))
    {
	    $curtimestamp = time();
	    $totalcontent = "_hip_=".$hip."\t_stid_=".$stname."\t_sstid_=".$sstname."\t_gid_=".$gameid."\t_zid_=".$zoneid."\t_sid_=".$serverid."\t_pid_=".$platformid."\t_ts_=".$curtimestamp."\t_acid_=".$acct_id."\t_plid_=".$playerid."\n";
	    $ret = write_custom_log($gameid, $totalcontent);
    }
    return $ret;
}




function write_error_log($gameid, $err_msg)
{
	umask(0000);
	$curhour = (int)(floor(time() / 3600) * 3600);//每小时一个文件
	if(!file_exists("/opt/taomee/stat/data/log"))
	{
    		mkdir("/opt/taomee/stat/data/log/");
	}
	$error_file = "/opt/taomee/stat/data/log/".$gameid."_error_".$curhour;
	$error_file_fd = fopen($msgfile, "ab");
	if(!$error_file_fd)
	{
		return;
	}
	chmod($error_file, 0777);
	fwrite($error_file_fd, $err_msg);

	fclose($error_file_fd);
	return;


}

function write_basic_log($gameid, $msg_content)
{
	umask(0000);
	$curhh = (int)(floor(time() / 5) * 5);//每5秒一个文件
	if(!file_exists("/opt/taomee/stat/data/inbox"))
	{
    		mkdir("/opt/taomee/stat/data/inbox/");
	}
	$msgfile = "/opt/taomee/stat/data/inbox/".$gameid."_game_basic_".$curhh;
	$basic_file_fd = fopen($msgfile, "ab");
	if(!$basic_file_fd)
	{
		write_error_log($gameid, $msgfile." open failed.\n");
		return false;
	}
	chmod($msgfile, 0777);
	if(!fwrite($basic_file_fd, $msg_content))
	{
		write_error_log($gameid, $msg_content." write failed.\n");
	    fclose($basic_file_fd);
	    return false;
	}

	fclose($basic_file_fd);
	return true;
}


function write_custom_log($gameid, $msg_content)
{
	umask(0000);
	$curhc = (int)(floor(time() / 100) * 100);//每100秒一个文件
    if(!file_exists("/opt/taomee/stat/data/inbox"))
	{
		mkdir("/opt/taomee/stat/data/inbox/");
	}
	$msgfile = "/opt/taomee/stat/data/inbox/".$gameid."_game_custom_".$curhc;
	$basic_file_fd = fopen($msgfile, "ab");
	if(!$basic_file_fd)
	{
		write_error_log($gameid, $msgfile." open failed.\n");
		return false;
	}
	chmod($msgfile, 0777);
	if(!fwrite($basic_file_fd, $msg_content))
	{
		write_error_log($gameid, $msg_content." write failed.\n");
	    fclose($basic_file_fd);
	    return false;
	}

	fclose($basic_file_fd);
	return true;
}


?>
