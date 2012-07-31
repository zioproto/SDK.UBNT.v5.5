#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
include("lib/link.inc");

if (strlen($iface) == 0) {
	$iface = $wlan_iface;
}

init_board_inc($iface);

$wmode = w_get_mode($iface);

if ($feature_ap_scan != 1) {
	if ($wmode == 3) {
		include("lib/err_scan.tmpl");
		exit;
	}
}

$chanlist_active = file("/proc/sys/net/"+$iface+"/chanlist");
$chans =  $chanlist_active[0];

$cmd_regex = "([[:print:]]*):([[:print:]]*)$";

Function get_cmdresult $cmd, $rgx, $ridx
(
	Exec($cmd, $arr, $result);
	if ($result == 0) {
		if (ereg($rgx, $arr[0], $res)) {
			return $res[$ridx];
		}
	}

	return "";
);

Function get_clksel $iface
(
	global $cmd_regex;
	$phyname = get_phyname($iface);
	$cmd = "iwpriv $phyname GetClkSel";

	$result = get_cmdresult($cmd, $cmd_regex, 2);
	switch ($result) {
		case "4";
			$clksel = 2;
			break;
		case "2";
			$clksel = 1;
			break;
		default;
			$clksel = 0;
			break;
	}

	return $clksel;
);

Function get_ieeemode $iface
(
	global $cmd_regex;
	$cmd = "iwpriv " + $iface + " get_mode";

	return get_cmdresult(EscapeShellCmd($cmd), $cmd_regex, 2);
);

$clksel = get_clksel($iface);
$ieee_mode = get_ieeemode($iface);

include("lib/survey.tmpl");
>
