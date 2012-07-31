#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
include("lib/link.inc");
include("lib/fw_head.tmpl");
flush();

$firewall_status = cfg_get_firewall($cfg, "disabled", $netmode);

if ($firewall_status == "enabled") {
	if ($netmode != "bridge") {
		PassThru($cmd_iptables + " -L FIREWALL -nv");
	} else {
		PassThru($cmd_ebtables + " -L FIREWALL --Lc");
	}
} else {
	include("lib/err_fw.tmpl");
}

include("lib/pfw_tail.tmpl");
>
