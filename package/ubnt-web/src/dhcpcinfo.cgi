#!/sbin/cgi
<?
include("lib/settings.inc");
if (fileinode($cfg_file_bak) != -1) {
$cfg = @cfg_load($cfg_file_bak);
} else {
$cfg = @cfg_load($cfg_file);
}
$l10n_no_cookies = 1;
include("lib/l10n.inc");
include("lib/misc.inc");

Function dequote $str (
	$str = chop($str);
	$str = ereg_replace("^\"[[:space:]]*", "", $str);
	$str = ereg_replace("[[:space:]]*\"$", "", $str);
	return $str;
);

$dhcpcstatus = cfg_get_def($cfg, "dhcpc.status", "disabled");
$i = 1;
/* support up to 4 interfaces, just as udhcpc script does.. */
while ($i < 5) {
	$i_status = cfg_get_def($cfg, "dhcpc.$i.status", "disabled");
	if ($i_status == "enabled") {
		$iface = cfg_get_def($cfg, "dhcpc.$i.devname", "");
		if (strlen($iface) > 0) {
			$ifaces[] = $iface;
		}
	}
	$i++;
}

$fp = @fopen("/proc/uptime", "r");
$line = fgets($fp, 255);
$now = intval($line);
fclose($fp);

if ($dhcpcstatus != "enabled" || count($ifaces) == 0) {
	$status = -1;
	$error_msg = dict_translate("DHCP Client is not enabled.");
	include("lib/dhcpc_head.tmpl");
} elseif ($action == "release" || $action == "renew") {
	if ($action == "release") {
		$signal = "-USR2";
	} else {
		$signal = "-USR1";
	}
	$pid = 0;
	if (strlen($ifname) > 0) {
		$file = "/etc/udhcpc/info."+$ifname;
		if (fileinode($file) != -1) {
			$dhcpc = @cfg_load($file);
			$pid = cfg_get($dhcpc, "u_pid");
		}
	}
	$cmd_all = "killall $signal udhcpc";
	if ($pid == 0 || strlen($pid) == 0) {
		$cmd = $cmd_all;
	} else {
		$cmd = "kill $signal $pid";
	}
	$status = 0;
	exec($cmd,$out,$rc);
	if ($rc != 0) {
		exec($cmd_all,$out,$rc);
		if ($rc != 0) {
			$status = -1;
			$error_msg = dict_translate("Failed on $action");
		}
	}
	if ($status == 0) {
		sleep(2);
	}
	include("lib/dhcpc_head.tmpl");
} else {
	$status = 0;
	include("lib/dhcpc_head.tmpl");

	$dhcpc_i = 0;
	$N = count($ifaces);
	while ($dhcpc_i < $N) {
		$iface = $ifaces[$dhcpc_i];
		$file = "/etc/udhcpc/info."+$iface;
		if (fileinode($file) == -1) {
			$status = -1;
			$error_msg = dict_translate("Not connected.");
		} else {
			$status = 0;
			$dhcpc = @cfg_load($file);
			$pid = cfg_get($dhcpc, "u_pid");
			$started = cfg_get($dhcpc, "u_started");
			$leasetime = cfg_get($dhcpc, "u_leasetime");
			$remain = intval($leasetime) + intval($started) - $now;
			if ($remain < 0) {
				$remain = 0;
			}
			$lease_time = secsToCountdown(intval($leasetime));
			$lease_left = secsToCountdown($remain);

			$ip = cfg_get($dhcpc, "u_ip");
			$netmask = cfg_get($dhcpc, "u_subnet");
			$serverid = cfg_get($dhcpc, "u_serverid");
			$gateway = cfg_get($dhcpc, "u_router");
			$domain = cfg_get($dhcpc, "u_domain");
			$hostname = cfg_get($dhcpc, "u_hostname");
			$dnsline = cfg_get($dhcpc, "u_dns");
			$dnsline = dequote($dnsline);
			$t = strtok($dnsline, " ");
			while ($t) {
				$dns[] = $t;
				$t = strtok(" ");
			}
		}
		if (($dhcpc_i + 1) >= $N) {
			$is_last = 1;
		} else {
			$is_last = 0;
		}
		include("lib/dhcpc_info.tmpl");
		$dhcpc_i++;
	}
}
include("lib/dhcpc_tail.tmpl");
>
