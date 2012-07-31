#!/sbin/cgi
<?
include("lib/settings.inc");
include("lib/l10n.inc");
@unlink($cfg_file);
@unlink($test_lock_file);
exec($cmd_cfgrestore);
if (fileinode($cfg_file) == -1) {
	exec($cmd_cfgrestore + " -t 2");
	if (fileinode($cfg_file) == -1) {
		exec("cp " + $def_cfg_file + " " + $cfg_file);
	}
}
if (strlen($testmode) != 0) {
	$cfg = @cfg_load($cfg_file);
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
	$country = cfg_get_country($cfg, $wlan_iface, $country);
	if ($country != 511) {
		$sr_delay = 1;
	}
	else {
		$sr_delay = 2;
	}
	bgexec($sr_delay, $cmd_softrestart+"test");
}
>
OK
