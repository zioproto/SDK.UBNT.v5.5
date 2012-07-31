#!/sbin/cgi
<?
include("lib/settings.inc");
include("lib/l10n.inc");
include("lib/link.inc");
include("lib/system.inc");
  
if ($REQUEST_METHOD == "POST") {
	if (isset($network_data) && strlen($network_data) > 100) {
		if (cfg_put($cfg_file, $network_data) != 0) {
        		$error_msg = dict_translate("msg_cfg_save_failed|Failed to save changes. Try again.");
		}
        } else {
        	$error_msg = dict_translate("msg_cfg_invalid|Failed to save changes. Invalid configuration data received. Try again.");
	}
}

$cfg = @cfg_load($cfg_file);
if ($cfg == -1) {
         include("lib/busy.tmpl");
         exit;
}
$netmode = cfg_get_def($cfg, "netmode", "bridge");
$wmode = strtolower(cfg_get_def($cfg, "radio.1.mode", "managed"));

include("/etc/board.inc");
$threeg_inc = "/etc/3g.inc";
if (fileSize($threeg_inc) > 0) {
	$threeg_modem = 1;
	include($threeg_inc);
}

include("lib/network.tmpl");
>
