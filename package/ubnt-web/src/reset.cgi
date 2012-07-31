#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
if ($REQUEST_METHOD == "POST") {
    exec($cmd_cfgdef);
    $pagetitle = dict_translate("Reset to Defaults");
    $message = dict_translate("msg_reseted_defaults|Device has been reset to defaults.");
    $duration = $reboot_time;
    include("lib/link.inc");
    $cfg = @cfg_load($def_cfg_file);
    if ($cfg == -1) {
	    include("lib/busy.tmpl");
	    exit;
    }

    include("lib/ipcheck.inc");
	if ($cfgnetmode == "bridge") {
		$ipaddress = cfg_get_ipv4addr($cfg, $br_iface, $defip);
	} else {
		$ipaddress = cfg_get_ipv4addr($cfg, $lan_iface, $defip);
	}
    $duration = $duration + $dss_rsa_gen_time;
    $f=@fopen($fwup_lock, "w");
    @fclose($f);
    include("lib/reboot.tmpl");
    bgexec(4, $cmd_reboot);
    exit;
}
$title = dict_translate("Reset to Defaults");
$question = dict_translate("qst_reset_defaults|Do you really want to reset the device to defaults?");
$answer = dict_translate("Yes, reset!");
include("lib/question.tmpl");
>
