#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
if ($REQUEST_METHOD == "POST") {
	exec($cmd_cfgsave);
	$pagetitle = dict_translate("Configuration flashed");
	$message = dict_translate("msg_config_flashed|Configuration has been flashed.");
	include("lib/msgpage.tmpl");
	exit;
}
$title = dict_translate("Flash configuration");
$question = dict_translate("qst_config_flash|Do you want to save current configuration to flash?");
$answer = dict_translate("Yes, save!");
include("lib/question.tmpl");
>
