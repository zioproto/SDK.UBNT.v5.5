<?

$trigger = "/usr/bin/trigger_url";
$cookie = " -d 'Cookie: AIROS_SESSIONID=$airosid'";

$scheme = "http";

Function writeStatus $status, $message
(
	header("Content-Type: application/json");
	echo "{ \"status\" : " + $status + ", \"message\" : \"" + $message + "\" }";
);

Function getRetVal $ret_val
(
	$res = (($ret_val & 65280) / 256);
	if ($res > 127)
	{
		$res -= 256;
	}
	return $res;
);

Function buildUrl $scheme, $ip, $port, $path
(
	$url = "$scheme://$ip";
	if ($port != 80 && $port != 443)
	{
		$url += ":$port";
	}

	$url += $path;
	return $url;
);

Function writeConfig $fname, $login, $passwd
(
	$fd = fopen($fname, "w");
	if ($fd == -1) {
		writeStatus(-1, "Internal error.");
		exit;
	}
	fputs($fd, "-a=username=$login\n");
	fputs($fd, "-a=password=$passwd\n");
	fclose($fd);
);

Function doLogin $ip, $port, $login, $passwd
(
	global $trigger, $cookie, $scheme;

	$cfgfile = "/tmp/trigger.txt";
	writeConfig($cfgfile, $login, $passwd);

	$cmd = $trigger + $cookie;
	$cmd += " -c $cfgfile";

	if ($port == 443)
	{
		$scheme = "https";
	}

	$url = buildUrl($scheme, $ip, $port, "/login.cgi");
	$full_cmd = "$cmd $url";

	exec($full_cmd, $lines, $res);
	$res = getRetVal($res);

	if ($res == -11 && $lines[0] == "/index.cgi")
	{
		# redirect to index.cgi means login succeeded
		unlink($cfgfile);
		return 0;
	}

	if ($res == -6 && $port != 443)
	{
		# read error, try ssl
		$scheme = "https";
		$url = buildUrl($scheme, $ip, $port, "/login.cgi");
		$full_cmd = "$cmd $url";

		exec($full_cmd, $new_lines, $new_res);
		$new_res = getRetVal($new_res);

		if ($new_res == -11 && $new_lines[0] == "/index.cgi")
		{
			unlink($cfgfile);
			return 0;
		}
	}

	unlink($cfgfile);

	if ($res == 0)
	{
		writeStatus(-1, "Login credentials are invalid.");
		exit;
	}

	if ($res == -21)
	{
		writeStatus(-1, "Request timed out -<br/>remote IP is not accessible.");
		exit;
	}

	if ($res == -3)
	{
		writeStatus(-1, "Invalid remote port or<br/>web server is not running.");
		exit;
	}

	if ($res != -11 && $lines[0] != "/index.cgi")
	{
		writeStatus(-1, "Specified device is not<br/>compatible for speedtest.");
		exit;
	}

	if ($lines[0] != "/index.cgi")
	{
		if (ereg("https://" + $ip + ":?[[:digit:]]*/login.cgi", $lines[0], $regs))
		{
			writeStatus(-1, "Invalid remote port specified.");
			exit;
		}
		else
		{
			writeStatus(-1, "Specified device is not<br/>compatible for speedtest.");
			exit;
		}
	}
);

Function doLogout $ip, $port
(
	global $scheme, $trigger, $cookie;
	$url = buildUrl($scheme, $ip, $port, "/logout.cgi");
	$cmd = "$trigger $cookie '$url'";
	exec($cmd, $lines, $res);
);

Function doRequest $ip, $port, $path, $login, $passwd
(
	global $trigger, $cookie, $scheme;

	doLogin($ip, $port, $login, $passwd);

	$url = buildUrl($scheme, $ip, $port, $path);
	$cmd = $trigger + $cookie;
	$cmd += " '" + $url + "'";
	exec($cmd, $lines, $res);
	$res = getRetVal($res);

	if ($res == 0)
	{
		$i = 0;
		while ($i < count($lines))
		{
			echo $lines[$i] + "\n";
			$i++;
		}
		doLogout($ip, $port);
		exit;
	}
	else
	{
		writeStatus(-1, "Specified device is not<br/>compatible for speedtest.");
		exit;
	}
);

Function loadModule
(
	$command = "/usr/bin/lsmod | grep spdtst";
	exec($command, $lines, $res);
	if (count($lines) == 0)
	{
		exec("/usr/bin/insmod /lib/modules/`uname -r`/spdtst.ko", $lines, $res);
		$res = getRetVal($res);
	}

	exec($command, $lines, $res);
	if (count($lines) == 0)
	{
		writeStatus(-1, "Speedtest initialization failed.");
		exit;
	}

	return 0;
);

Function getSessionId
(
	exec("cat /proc/net/spdtst/stctl | grep Session", $lines, $ret);
	if ($ret != 0)
	{
		return 0;
	}

	if (ereg("Session ID: ([[:digit:]]+), State: ([[:digit:]]+), Flags:", $lines[0], $regs))
	{
		return $regs[1];
	}

	return 0;
);

Function actionStart
(
	global $ticket, $target, $duration, $direction;
	$res = loadModule();
	if ($res != 0)
	{
		writeStatus(-1, "Failed to load speedtest module.");
		exit;
	}

	# TODO: check if other speedtest is not running
	$old = getSessionId();
	if ($old > 0)
	{
		actionStop($old);
	}

	# init local
	exec("echo " + $ticket + " init " + $target + " > /proc/net/spdtst/stctl", $lines, $res);
	$res = getRetVal($res);
	if ($res != 0)
	{
		writeStatus(-1, "Failed to initialize speedtest module.");
		exit;
	}

	if (IsSet($duration) == 1)
	{
		exec("echo " + $ticket + " duration " + $duration + " > /proc/net/spdtst/stctl", $lines, $res);
		$res = getRetVal($res);
		if ($res != 0)
		{
			writeStatus(-1, "Failed to set test duration.");
			exit;
		}

		exec("echo " + $ticket + " direction " + $direction + " > /proc/net/spdtst/stctl", $lines, $res);
		$res = getRetVal($res);
		if ($res != 0)
		{
			writeStatus(-1, "Failed to set test direction.");
			exit;
		}
	}

	# start test
	exec("echo " + $ticket + " start " + " > /proc/net/spdtst/stctl", $lines, $res);
	$res = getRetVal($res);
	if ($res != 0)
	{
		writeStatus(-1, "Speedtest start failed.");
		exit;
	}

	return $res;
);

Function actionSlave
(
	global $ticket;

	loadModule();
	$old = getSessionId();
	if ($old > 0)
	{
		actionStop($old);
	}

	exec("echo " + $ticket + " slave > /proc/net/spdtst/stctl", $lines, $res);
	$res = getRetVal($res);
	if ($res != 0)
	{
		writeStatus(-1, "Failed to initialize remote host.");
		exit;
	}

	return 0;
);

Function actionStop $ticket
(
	exec("echo " + $ticket + " stop > /proc/net/spdtst/stctl", $lines, $res);
	$res = getRetVal($res);
	return $res;
);

Function actionRemote
(
	global $ticket, $target, $port, $login, $passwd;
	doRequest($target, $port, "/sptest_action.cgi?action=slave&ticket=" + $ticket, $login, $passwd);
);

Function actionStatus
(
	$session = -1;
	$state = -1;
	$duration = 0;
	$flags = 0;
	$tx = 0;
	$rx = 0;

	$session_regex = "Session ID: ([[:digit:]]+), State: ([[:digit:]]+), Flags: 0x([[:digit:]]{4})";
	$duration_regex = "Duration: ([[:digit:]]+), Data size:";
	$tx_regex = "[[:digit:]]+pps \(([[:digit:]]+)bps\) - time : [[:digit:]]+us";
	$rx_regex = "[[:digit:]]+pps \(([[:digit:]]+)bps\) - time : [[:digit:]]+us";

	$lines = @file("/proc/net/spdtst/stctl");
	$count = count($lines);

	if ($count > 10)
	{
		if (ereg($session_regex, $lines[1], $regs))
		{
			$session = $regs[1];
			$state = $regs[2];
			$flags = HexDec($regs[3]);
		}

		if ($state == 10) # completed
		{
			if (ereg($duration_regex, $lines[2], $regs))
			{
				$duration = $regs[1];
			}
			if (ereg($tx_regex, $lines[7], $regs))
			{
				$tx = doubleval($regs[1]) / (1024*1024);
			}
			if (ereg($rx_regex, $lines[10], $regs))
			{
				$rx = doubleval($regs[1]) / (1024*1024);
			}
		}
	}

	header("Content-Type: application/json");
	echo "{ \n";
	echo "\t\"session\" : " + $session + ",\n";
	echo "\t\"state\" : " + $state + ",\n";
	echo "\t\"flags\" : " + $flags + ",\n";
	echo "\t\"duration\" : " + $duration + ",\n";
	echo "\t\"tx\" : " + $tx + ",\n";
	echo "\t\"rx\" : " + $rx + "\n";
	echo "}\n";

	exit;
);

>
