#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
$pagetitle = dict_translate("Applying...");
$message = dict_translate("msg_conf_applied|Configuration is being applied, please stand by...");
$duration = $soft_reboot_time;
include("lib/link.inc");
include("lib/ipcheck.inc");
$added_time = 0;
if ($cfg != -1) {
       	$sshd_state = cfg_get_def($cfg, "sshd.status", "disabled");
       	if ($sshd_state == "enabled") {
       		if ((fileinode($dss_priv_filename) == -1) ||
       			(fileinode($rsa_priv_filename) == -1)) {
       				$added_time = $dss_rsa_gen_time;
       				$duration = $duration + $added_time;
       		}
       	}
}
$sr_delay = 1;
if (strlen($testmode) != 0) {
	$fp = @fopen($test_lock_file, "w");
	if ($fp != -1) {
		@fputs($fp, $test_mode_time);
		@fclose($fp);
		chmod($test_lock_file, 755);
		bgexec($sr_delay, $cmd_softrestart+"test");
        }
} else {
	chmod($cfg_file, "644");
	$fp = @fopen($test_lock_file, "r");
	if ($fp != -1) {
		@fclose($fp);
        	@unlink($test_lock_file);
		exec("/usr/bin/sort $cfg_file_bak > $cfg_file");
		bgexec(0, $cmd_cfgsave);
	}
	else {
		bgexec($sr_delay, $cmd_softrestart+"save");
	}
}
exit;
>
