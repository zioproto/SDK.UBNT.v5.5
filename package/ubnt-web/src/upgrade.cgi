#!/sbin/cgi
<?
include("lib/settings.inc");
include("lib/system.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");

$is_get=0;$is_fw_exist=0;$is_post=0;

if ($REQUEST_METHOD == "GET") {
	$is_get=1;
}
if (fileinode($firmware_file) != -1) {
	$is_fw_exist=1;
}
if ($REQUEST_METHOD == "POST") {
	$is_post=1;
}
  if ($is_get && !$is_fw_exist) {
      $fwversion = fw_get_version();
      @unlink($firmware_file);
      cleanup_dir($upload_dir);
      include("lib/fwupload.tmpl");
  } elseif ($is_fw_exist || $is_post) {
      $fwversion = fw_get_version();
      if ($is_fw_exist) {
         $fwfile = $firmware_file;
      }
      if (!fw_validate($fwfile)) {
      	 $error_msg = dict_translate("msg_bad_fwimage|Bad firmware update image.");
         @unlink($fwfile);
         @unlink($firmware_file);
         include("lib/fwupload.tmpl");
      } else {
         $newfwversion = fw_extract_version($firmware_file);
	 if (fw_is_thirdparty($firmware_file)) {
		 $error_msg = dict_translate("warn_third_party_firmware|WARNING: Uploaded firmware is third-party, make sure you're familiar with recovery procedure!");
	 }
         include("lib/fwflash.tmpl");
      }
  }
>
