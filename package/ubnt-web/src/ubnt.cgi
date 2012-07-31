#!/sbin/cgi
<?
SecureVar("cmd*");
SecureVar("lines");
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
include("lib/link.inc");
include("lib/misc.inc");
include("lib/system.inc");

if ($cfg == -1) {
	include("lib/busy.tmpl");
	exit;
}

$show_warn = 0;
$airmaxpridef = 3;
$wmode_type = get_wmode_type(cfg_get_wmode($cfg, $wlan_iface));
$ieee_mode = cfg_get_ieee_mode($cfg, $wlan_iface, $ieee_mode);
$ieee_mode = strtolower($ieee_mode);
$netmode_cfg = cfg_get_def($cfg, "netmode", "bridge");
if (strlen($netmode)==0) {
	$netmode = $netmode_cfg;
}
$country = cfg_get_country($cfg, $wlan_iface, $country);
$obey_regulatory_status = cfg_get_obey($cfg, $wlan_iface, $obey_default);
$clksel = cfg_get_clksel($cfg, $wlan_iface, $clksel);
$chanshift = cfg_get_chanshift($cfg, $wlan_iface, $chanshift);
$radio1_chanbw = cfg_get_def($cfg, "radio.1.chanbw", "0");

if ($REQUEST_METHOD == "POST") {
	cfg_set($cfg, "airview.tcp_port", $av_tcp_port);
	if ($radio1_ccode_fixed == 0) {
		if (strlen($polling)) {
			$polling = "enabled";
		} else {
			$polling = "disabled";
		}
		cfg_set($cfg, "radio.1.polling", $polling);
	}

	cfg_set($cfg, "radio.1.pollingnoack", $polling_noack_value);
	if ($wmode_type == 2) {
		cfg_set($cfg, "radio.1.polling_fh", $polling_airselect_value);
		cfg_set($cfg, "radio.1.polling_fh_time", $polling_airselect_interval);
		cfg_set($cfg, "radio.1.polling_fh_announce_cnt", $polling_airselect_announce_cnt);
		set_scan_channels($cfg, 1, $polling_airselect_channels);
		if (IsSet($polling_airselect_value) && $polling_airselect_value != 0) {
			$sl_status = cfg_get_channel_scan_list($cfg, 1, "disabled");
			if ($sl_status == "disabled") {
				set_channel_scan_list($cfg, 1, "enabled");
				$show_warn = 1;
			}
			/* check if frequency exists in list, set to first one if not */
			$freq = cfg_get_def($cfg, "radio.1.freq", 0);
			if ($freq != 0) {
				$found = 0;
				$newfreq = 0;
				$tok = strtok($polling_airselect_channels, ",");
				while($tok && $found == 0);
				if ($newfreq == 0) {
					$newfreq = $tok;
				}
				if ($tok == $freq) {
					$found = 1;
				}
				$tok = strtok(",");
				endwhile;
				if (!$found) {
					cfg_set($cfg, "radio.1.freq", $newfreq);
					$show_warn = 1;
				}
			}
		}
	}

	if (strlen($airmaxpri)) {
		cfg_set($cfg, "radio.1.pollingpri", $airmaxpri);
	}

	if (!strlen($polling_airsync)) {
		$polling_airsync="0";
	}

	if (strlen($airsync_status)) {
		$airsync_status = "enabled";
	}
	else {
		$airsync_status = "disabled";
	}

	if (strlen($airsync_slot_override)) {
		$airsync_slot_override = "enabled";
	}
	else {
		$airsync_slot_override = "disabled";
	}

	cfg_set($cfg, "radio.1.airsync.status", $airsync_status);
	cfg_set($cfg, "radio.1.airsync.mode", $airsync_mode);
	cfg_set($cfg, "radio.1.airsync.port", $airsync_port);
	cfg_set($cfg, "radio.1.airsync.ip", $airsync_master);
	cfg_set($cfg, "radio.1.airsync.slot.override", $airsync_slot_override);
	cfg_set($cfg, "radio.1.airsync.slot.up", $airsync_up_slot);
	cfg_set($cfg, "radio.1.airsync.slot.down", $airsync_down_slot);

	cfg_update_dmz_mgmt($cfg);
	cfg_save($cfg, $cfg_file);
	cfg_set_modified($cfg_file);
	$message = dict_translate("Configuration saved");
}

if ($radio1_caps & $radio_cap_airmax_only) {
	$polling = "enabled";
} else {
	$polling = cfg_get_def($cfg, "radio.1.polling", $polling);
}
$polling_noack = cfg_get_def($cfg, "radio.1.pollingnoack", "0");

if ($wmode_type == 2) {
	$polling_airselect = cfg_get_def($cfg, "radio.1.polling_fh", "0");
	$polling_airselect_interval = cfg_get_def($cfg, "radio.1.polling_fh_time", "3000");
	if (strlen($polling_airselect_interval) == 0) { $polling_airselect_interval = "3000"; }
	$polling_airselect_announce_cnt = cfg_get_def($cfg, "radio.1.polling_fh_announce_cnt", "10");
	if (strlen($polling_airselect_announce_cnt) == "0") { $polling_airselect_announce_cnt = "10"; }
	$polling_airselect_channels = cfg_get_scan_channels($cfg, 1, "");
}

$airmaxpri = cfg_get_def($cfg, "radio.1.pollingpri", $airmaxpridef);
if (strlen($airmaxpri) == 0) { $airmaxpri = $airmaxpridef; }

$airsync_status = cfg_get_def($cfg, "radio.1.airsync.status", "disabled");
$airsync_mode = cfg_get_def($cfg, "radio.1.airsync.mode", "1");
$airsync_port = cfg_get_def_ne($cfg, "radio.1.airsync.port", "64250");
$airsync_master = cfg_get_def($cfg, "radio.1.airsync.ip", $airsync_master);
$airsync_slot_override = cfg_get_def($cfg, "radio.1.airsync.slot.override", "disabled");
$airsync_up_slot = cfg_get_def($cfg, "radio.1.airsync.slot.up", "2000");
$airsync_down_slot = cfg_get_def($cfg, "radio.1.airsync.slot.down", "4000");

if (!strlen($polling_airsync)) {
	$polling_airsync="0";
}

$av_tcp_port = cfg_get_def($cfg, "airview.tcp_port", $airview_tcp_port);

include("lib/ubnt.tmpl");
>
