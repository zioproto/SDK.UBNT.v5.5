#!/sbin/cgi
<?
include("lib/settings.inc");
include("lib/system.inc");
$hwaddr = if_get_hwaddr($wlan_iface);
if (strlen($hwaddr) == 0) {
	$hwaddr = if_get_hwaddr($eth0_iface);
}
$version = fw_get_version();
$p = strtok($version, ".");

$filename = $p + "-" + ereg_replace(":", "", $hwaddr) + ".cfg";

header("Content-Type: application/force-download");
header("Content-Disposition: attachment; filename=" + $filename);
passthru("cat /tmp/system.cfg");
exit;
>
