#!/sbin/cgi
<?
	Function getReturnCode $cmd_code
	(
	    $res = (($cmd_code & 65280) / 256);
	    if ($res > 127)
	    {
	    	$res -= 256;
	    }
	    return $res;
	);

	Function split $delimiter, $string (
		$reminder = $string;
		$delimiterlen = strlen($delimiter);
	
		if ($delimiterlen > 0)
		{
			$chunk = strstr($reminder, $delimiter);
			$chunklen = strlen($chunk);
		}
		else
		{
			$chunklen = 0;
		}
		while ($chunklen != 0)
		{
			$reminderlen = strlen($reminder);
			$part = substr($reminder, 0, $reminderlen - $chunklen);
			if (($reminderlen - $chunklen) > 0)
			{
				$result[] = substr($reminder, 0, $reminderlen - $chunklen);
			}
			$reminder = substr($chunk, $delimiterlen, $chunklen - $delimiterlen);
			$chunk = strstr($reminder, $delimiter);
			$chunklen = strlen($chunk);
		}
		$result[] = $reminder;
		return $result;
	);

	if ($action != "traceroute")
	{
        echo "-255|Invalid or no action: " + $action;
        exit;
    }

	if (strlen($dst_host) < 0)
	{
        echo "-254|No host specified.";
        exit;
	}

	if (strlen($hop) < 0)
	{
        echo "-253|No hop specified.";
        exit;
	}

	$command = "/bin/traceroute -w 5";
	$no_resolve = 0;
	if (strlen($resolve) == 0)
	{
		$command += " -n";
		$no_resolve = 1;
	}
	$command += " -f " + EscapeShellCmd($hop)
		+ " -m " + EscapeShellCmd($hop)
		+ " " + EscapeShellCmd($dst_host);
		    
	exec($command+" 2>&1", $lines, $res);
    $res = getReturnCode($res);
   	if ($res)
   	{
	   	echo $res;
   		exit;
   	}
   	
	$line_count = count($lines);
	if ($line_count != 2 && ($no_resolve == 1 || $line_count != 3))
	{
		echo "-252|Invalid output line count: " + count($lines);
		echo "|"+$command;
		$i = 0;
		while ($i < count($lines))
		{
			echo "|"+$lines[$i];
			$i++;
		}
		exit;
	}

	$start_idx = $line_count - 2;
	$chunks = split(" ", $lines[$start_idx]);
	if(count($chunks) < 4)
	{
        echo "-251|Can't parse 1'st line: " + count($lines);
		exit;
	}
	$dst_addr = substr($chunks[3], 1, strlen($chunks[3]) - 3);
	$chunks = split(" ", $lines[$start_idx + 1]);
	if(count($chunks) < 4)
	{
        echo "-250|Can't parse 2'nd line: " + count($lines);
		exit;
	}
	$r_idx = 0;
	$c_idx = 1;
	$hop_no = $chunks[0];
	if ($chunks[$c_idx] == "*")
	{
		$result[$r_idx] = "*";
		$r_idx++;
		$c_idx++;
		if ($chunks[$c_idx] == "*")
		{
			$result[$r_idx] = "*";
			$r_idx++;
			$c_idx++;
			if ($chunks[$c_idx] == "*")
			{
				$result[$r_idx] = "*";
				$r_idx++;
				$c_idx++;
			}
		}
	}
	if ($r_idx == 3)
	{
		echo "0|"+$hop_no+"|&nbsp;|&nbsp;|"+$result[0]+"|"+$result[1]+"|"+$result[2]+"|0";
		exit;
	}
	
	$hop_host = $chunks[$c_idx];
	$c_idx++;
	if ($no_resolve)
	{
		$hop_addr = $hop_host;
	}
	else
	{
		$hop_addr = substr($chunks[$c_idx], 1, strlen($chunks[$c_idx]) - 2);
		$c_idx++;
	}
	while ($c_idx < count($chunks))
	{
		if ($chunks[$c_idx] == "*")
		{
			$result[$r_idx] = "*";
			$r_idx++;
		}
		elseif ($chunks[$c_idx] == "ms")
		{
			$result[$r_idx] += " ms";
			$r_idx++;
		}
		elseif (ereg("^!", $chunks[$c_idx]))
		{
			$r_idx--;
			$result[$r_idx] += " "+$chunks[$c_idx];
			$r_idx++;
		}
		else
		{
			$result[$r_idx] = $chunks[$c_idx];
		}
		$c_idx++;		
	}
	
	$hop_last = 0;
	if ($hop_addr == $dst_addr || ereg("!([ P]|[NHFS][^!]+![NHFS])", $lines[$start_idx + 1]))
	{
		$hop_last = 1;
	}
	echo "0|"+$hop_no+"|"+$hop_host+"|"+$hop_addr+"|"+$result[0]+"|"
		+$result[1]+"|"+$result[2]+"|"+$hop_last;
>
