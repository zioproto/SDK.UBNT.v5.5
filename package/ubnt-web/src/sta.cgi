#!/sbin/cgi
<?
include("lib/settings.inc");
$cmd = $cmd_wstalist;
if (strlen($ifname) > 0) {
	$cmd += " -i " + $ifname;
}
if (strlen($sta_mac) > 0) {
	$cmd += " -a " + $sta_mac;
}
PassThru(EscapeShellCmd($cmd));
>
