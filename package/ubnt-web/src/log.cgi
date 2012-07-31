#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
include("lib/log_head.tmpl");

$syslog_file = "/var/log/messages";

flush();

$syslog_status = cfg_get_def($cfg, "syslog.status", $syslog_status);
if ($syslog_status == "enabled") {
	if (($clr == "yes") && (fileinode($syslog_file) != -1)) {
		$fh = fopen($syslog_file, "w");
		fclose($fh);
		system("logger 'Messages cleared.'");
	}
	echo "<pre>";
	PassThru($cmd_log);
	echo "</pre>";
} else {
	include("lib/err_syslog.tmpl");
}

include("lib/log_tail.tmpl");
>
