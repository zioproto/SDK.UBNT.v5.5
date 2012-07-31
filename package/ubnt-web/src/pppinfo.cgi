#!/sbin/cgi
<?
include("lib/settings.inc");
if (fileinode($cfg_file_bak) != -1) {
$cfg = @cfg_load($cfg_file_bak);
} else {
$cfg = @cfg_load($cfg_file);
}
include("lib/l10n.inc");
include("lib/misc.inc");

if ($action == "restartppp") {
	$cmd = "killall pppd";
	exec($cmd);
	sleep(2);
}
/* Sample output from pppstats -r ppp0
      IN   PACK VJCOMP  RATIO  UBYTE  |      OUT   PACK VJCOMP  RATIO  UBYTE
   10677    173      0   1.00      0  |    15628    115      0   1.00      0
 */
$pppstats_regexp = "[[:space:]]*([^[:space:]]+)[[:space:]]*([^[:space:]]+)[[:space:]]*([^[:space:]]+)[[:space:]]*([^[:space:]]+)[[:space:]]*([^[:space:]]+)[[:space:]]*\|[[:space:]]*([^[:space:]]+)[[:space:]]*([^[:space:]]+)[[:space:]]*([^[:space:]]+)[[:space:]]*([^[:space:]]+)[[:space:]]*([^[:space:]]+)";

include("lib/ppp_head.tmpl");
flush();

$pppstatus = cfg_get_def($cfg, "ppp.status", "disabled");
$ppp1status = cfg_get_def($cfg, "ppp.1.status", "disabled");

if ($pppstatus != "enabled" && $ppp1status != "enabled") {
	$status = -1;
	$error_msg = dict_translate("PPPoE is not enabled.");
	include("lib/ppp_info.tmpl");
} else {
	$status = 0;

	$fp = @fopen("/proc/uptime", "r");
	$line = fgets($fp, 255);
	$now = intval($line);
	fclose($fp);

	$i = 0;
	while ($i < 1) { /* for future listing of all ppp interfaces */
		$iface = "ppp$i";
		$file = "/etc/ppp/info."+$iface;
		if (fileinode($file) == -1) {
			$status = 0;
			$error_msg = dict_translate("Not connected");
		} else {
			$status = 1;
			$ppp = @cfg_load($file);
			$started = cfg_get_def($ppp, "started", $now);
			$duration_secs = $now - intval($started);
			if ($duration_secs < 0) {
				$duration_secs = 0;
			}
			$duration = secsToCountdown($duration_secs);
			$name = cfg_get($ppp, "name");
			$ipaddr = cfg_get($ppp, "iplocal");
			$ipremote = cfg_get($ppp, "ipremote");
			$dns1 = cfg_get($ppp, "dns1");
			$dns2 = cfg_get($ppp, "dns2");
			$device = cfg_get($ppp, "device");

			Exec($cmd_pppstats + " $iface", $arr, $result);
			if ($result == 0 && ereg($pppstats_regexp, $arr[1], $res)) {
				$in_bytes = $res[1];
				$in_packets = $res[2];
				$in_vjcomp = $res[3];
				$in_ratio = $res[4];
				$in_ubyte = $res[5];
				$out_bytes = $res[6];
				$out_packets = $res[7];
				$out_vjcomp = $res[8];
				$out_ratio = $res[9];
				$out_ubyte = $res[10];
			} else {
				$in_bytes = 0;
				$in_packets = 0;
				$in_vjcomp = 0;
				$in_ratio = 0;
				$in_ubyte = 0;
				$out_bytes = 0;
				$out_packets = 0;
				$out_vjcomp = 0;
				$out_ratio = 0;
				$out_ubyte = 0;
			}
			$bytestr = bytes_to_human($in_bytes);
			if (strlen($bytestr) > 0) {
				$in_bytes_str = "$in_bytes ($bytestr)";
			} else {
				$in_bytes_str = "$in_bytes";
			}
			$bytestr = bytes_to_human($out_bytes);
			if (strlen($bytestr) > 0) {
				$out_bytes_str = "$out_bytes ($bytestr)";
			} else {
				$out_bytes_str = "$out_bytes";
			}
		}

		include("lib/ppp_info.tmpl");
		$i++;
	}
}

include("lib/ppp_tail.tmpl");
>
