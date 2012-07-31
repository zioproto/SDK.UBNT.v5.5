#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
if ($REQUEST_METHOD == "POST") {
    include("lib/link.inc");
    $pagetitle = dict_translate("Rebooting...");
    $message = dict_translate("msg_device_rebooting|Device is rebooting, please stand by...");
    $duration = $reboot_time;
    include("lib/ipcheck.inc");
    $f=@fopen($fwup_lock, "w");
    @fclose($f);
    include("lib/reboot.tmpl");
    bgexec(4, $cmd_reboot);
    exit;
}
$title = dict_translate("Reboot the Device");
$question = dict_translate("qst_reboot|Do you really want to reboot the device?");
$answer = dict_translate("Yes, reboot!");
include("lib/question.tmpl");
>
