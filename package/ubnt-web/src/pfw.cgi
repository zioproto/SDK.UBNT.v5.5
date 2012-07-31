#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
include("lib/link.inc");
include("lib/pfw_head.tmpl");
flush();

$port_forward_status = cfg_get_def($cfg, "iptables.sys.portfw.status", "disabled");
if ($port_forward_status == "enabled") {
	echo "<pre>";
	PassThru($cmd_iptables + "-t nat -L PORTFORWARD -nv");
	echo "</pre>";
} else {
	include("lib/err_pfw.tmpl");
}

include("lib/pfw_tail.tmpl");
>
