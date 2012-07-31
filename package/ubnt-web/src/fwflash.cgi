#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
$is_discard = 0;
if (strlen($discard) > 1) {
      $is_discard = 1;
}

if ($REQUEST_METHOD == "POST" || $do_update > 1) {
      include("lib/system.inc");
      include("lib/link.inc");

      $pagetitle=dict_translate("Firmware Update");
      $message = dict_translate("msg_firmware_upgrading|Firmware is being updated.<br> This operation takes several minutes to complete - <br>meanwhile <span style=\"color: red\">DO NOT POWEROFF</span> the device!");
      $duration=$upgrade_time;
      include("lib/ipcheck.inc");
      $f=@fopen($fwup_lock, "w");
      @fclose($f);
      include("lib/reboot.tmpl");
      bgexec(4, "/sbin/fwupdate -m");
} elseif (($REQUEST_METHOD == "GET") || $is_discard) {
	include("lib/system.inc");
	$fwversion = fw_get_version();
	@unlink($firmware_file);
	cleanup_dir($upload_dir);
	include("lib/fwupload.tmpl");
  }
>
