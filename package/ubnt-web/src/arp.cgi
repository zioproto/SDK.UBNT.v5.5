#!/sbin/cgi
<?
include("lib/settings.inc");
include("lib/misc.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
$arp_regexp="([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]*$";
include("lib/arp_head.tmpl");
flush();

$fp = @fopen("/proc/net/arp", "r");
if ($fp > -1) {
$line=fgets($fp,255);
while(!feof($fp)) {
	$line=fgets($fp,255);
	if (ereg($arp_regexp,$line,$res) && $res[4] != "00:00:00:00:00:00") {
		$f = devname2ifname($res[6]);
		echo "<tr><td class=\"str\">" + $res[1] + "</td><td class=\"str\">" + strtoupper($res[4]) + "</td><td>" + $f + "</td></tr>\n";
	}
}
fclose($fp);
}
include("lib/arp_tail.tmpl");
>
