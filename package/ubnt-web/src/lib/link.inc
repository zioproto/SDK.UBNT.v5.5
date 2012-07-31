<?
Function get_iface_index $cfg, $iface
(
	$index = 0; /* not found */
	$idx = 1;
	$devname = "dummy";

	while ($index == 0 && strlen($devname) != 0 && $idx <= 32) {
		$devname = cfg_get_def($cfg, "netconf.$idx.devname", "");
		if ($devname == $iface) {
			$index = $idx;
		}
		$idx++;
	}
	return $index;
);

Function get_wlan_index $iface
(
	global $wlan_iface;
	$index = 0;
	if ($iface == $wlan_iface) {
		$index = "1";
	}
	return $index;
);

Function get_phyname $index
(
	return "wifi0";
);

Function set_wmode $cfg, $wlan_iface, $wmode, $wds_chkbox, $ch_freq, $macclone (
	$netmode = cfg_get_def($cfg, "netmode", "bridge");
	$wds = "disabled";
	$wireless = "managed";
	if ($macclone == "enabled" || $macclone == "on") {
		$macclone = "enabled";
	}
	else {
		$macclone = "disabled";
	}

	switch ($wmode)
	{
	case "aprepeater";
		$wireless = "master";
		$macclone = "disabled";
		$wds = "enabled";
		break;
	case "ap";
		$wireless = "master";
		$macclone = "disabled";
		if ($wds_chkbox == "on" || $wds_chkbox == "enabled") {
			$wds = "enabled";
			$macclone = "disabled";
		}
		break;
	case "sta";
		if ($wds_chkbox == "on" || $wds_chkbox == "enabled") {
			$wds = "enabled";
			$macclone = "disabled";
		}
		break;
	default;
		break;
	};

	$arpnat_st = "enabled";
	if ($wds == "enabled" || $macclone == "enabled" || $wireless == "master") {
		$arpnat_st = "disabled";
	}
	else {
		cfg_set($cfg, "ebtables.sys.status", "enabled");
	}
	cfg_set($cfg, "ebtables.sys.arpnat.status", $arpnat_st);

	$idx = get_wlan_index($wlan_iface);
	cfg_set($cfg, "wireless.$idx.wds.status", $wds);
	cfg_set($cfg, "radio.$idx.mode", $wireless);
	if (strlen($ch_freq) != 0 && $wireless == "master") {
		cfg_set($cfg, "radio.$idx.freq", $ch_freq);
	}
	cfg_set($cfg, "wireless.$idx.macclone", $macclone);
);

Function cfg_get_wmode $cfg, $wlan_iface (
	$idx = get_wlan_index($wlan_iface);
	$mode = strtolower(cfg_get_def($cfg, "radio.$idx.mode", "managed"));
	$wds = cfg_get_def($cfg, "wireless.$idx.wds.status", "disabled");
	if ($wds == "enabled") {
		if ($mode == "master") {
			$wdsinfo = get_wds_info($cfg, $idx);
			$wi = 1;
			$wc = 0;
			$wn = count($wdsinfo);
			while ($wi < $wn) {
				if (strlen($wdsinfo[$wi]) > 0) {
					$wc++;
				}
				$wi++;
			}
			if ($wdsinfo[0] == "enabled" || $wc > 0) {
				$result = "aprepeater";
			} else {
				$result = "ap";
			}
		}
		else {
			$result =  "sta";
		}
	}
	else {
		if ($mode == "master") {
			$result = "ap";
		}
		else {
			$result = "sta";
		}
	}
	return $result;
);

Function cfg_get_wpa_auth $cfg, $idx, $wmode, $wpa_auth (
	if (strlen($wpa_auth) == 0) {
		$wpa_auth = "WPA-PSK";
	}
	if ($wmode == "ap" || $wmode == "aprepeater") {
		$wpa_auth = cfg_get_def($cfg, "aaa.$idx.wpa.key.1.mgmt", $wpa_auth);
	}
	else {
		$wpa_auth = cfg_get_def($cfg, "wpasupplicant.profile.$idx.network.1.key_mgmt.1.name", $wpa_auth);
	}
	return $wpa_auth;
);

Function cfg_get_wpa_eap $cfg, $idx, $wmode, $wpa_eap (
	if (strlen($wpa_eap) == 0) {
		$wpa_eap = "TTLS";
	}
	if ($wmode != "ap" && $wmode != "aprepeater") {
		$wpa_eap = cfg_get_def($cfg, "wpasupplicant.profile.$idx.network.1.eap.1.name", $wpa_eap);
	}
	return $wpa_eap;
);

Function cfg_get_wpa_inner $cfg, $idx, $wpa_inner (
	if (strlen($wpa_inner) == 0) {
		$wpa_inner = "auth=MSCHAPV2";
	}
	return  cfg_get_def($cfg, "wpasupplicant.profile.$idx.network.1.phase2", $wpa_inner);
);

Function cfg_get_wpa_ident $cfg, $idx, $wpa_ident (
	return  cfg_get_def($cfg, "wpasupplicant.profile.$idx.network.1.identity", $wpa_ident);
);

Function cfg_get_wpa_user $cfg, $idx, $wpa_user (
	if (strlen($wpa_user) == 0) {
		$wpa_user = "anonymous@myisp.com";
	}
	return  cfg_get_def($cfg, "wpasupplicant.profile.$idx.network.1.anonymous_identity", $wpa_user);
);

Function cfg_get_wpa_passwd $cfg, $idx, $wpa_passwd (
	return  cfg_get_def($cfg, "wpasupplicant.profile.$idx.network.1.password", $wpa_passwd);
);

Function get_wmode_type $wmode (
	if ($wmode == "sta") {
		$type = 1;
	}
	else {
		if ($wmode == "ap" || $wmode == "aprepeater") {
			$type = 2;
		}
		else {
			$type = 3;
		}
	}
	return $type;
);

Function set_essid $cfg, $idx, $essid (
	return cfg_set($cfg, "wireless.$idx.ssid", $essid);
);

Function set_hide_ssid $cfg, $idx, $hidessid (
	if ($hidessid == "on") {
		$hidessid = "enabled";
	}
	elseif ($hidessid != "enabled") {
		$hidessid = "disabled";
	}
	return cfg_set($cfg, "wireless.$idx.hide_ssid", $hidessid);
);

Function cfg_get_hide_ssid $cfg, $idx, $def_hide (
	return cfg_get_def($cfg, "wireless.$idx.hide_ssid", $def_hide);
);

Function set_apmac $cfg, $idx, $apmac (
	return cfg_set($cfg, "wireless.$idx.ap", $apmac);
);

Function set_txpower $cfg, $idx, $power (
	return cfg_set($cfg, "radio.$idx.txpower", $power);
);

Function set_obey $cfg, $idx, $obey (
	cfg_del($cfg, "radio.$idx.regulatory");
	cfg_set($cfg, "radio.$idx.reg_obey", $obey);
	return cfg_set($cfg, "radio.$idx.obey", $obey);
);

Function set_country $cfg, $idx, $country, $subsystemid (
	$result = $cfg;
	if (strlen($country)) {
		if ($idx == 1) {
			cfg_set($cfg, "radio.countrycode", $country);
		}
		cfg_set($cfg, "radio.$idx.countrycode", $country);
		$result = cfg_set($cfg, "radio.$idx.subsystemid", $subsystemid);
	}
	return $result;
);

Function set_def_wep_key_id $cfg, $idx, $wep_key_id(
	return cfg_set($cfg, "wireless.$idx.security.default_key", $wep_key_id);
);

Function set_wep_key $cfg, $idx, $wep_key_id, $wep_key, $wep_key_type (
	if ($wep_key_type == 2) {
		$wep_key = "s:" + $wep_key;
	}
	return cfg_set($cfg, "wireless.$idx.security." + $wep_key_id + ".key", $wep_key);
);

Function set_security $cfg, $iface, $idx, $security, $wep_key_len, $wmode (
	global $br_iface;

	switch ($security)
	{
	case "wpa";
	case "wpa2";
		$pairwise="TKIP CCMP";
		break;
	case "wpatkip";
	case "wpa2tkip";
		$pairwise="TKIP";
		break;
	case "wpaaes";
	case "wpa2aes";
		$pairwise="CCMP";
		break;
	}

	switch ($security)
	{
	case "wep64";
	case "wep128";
		cfg_set($cfg, "wireless.$idx.security.type", $security);
		cfg_set($cfg, "wpasupplicant.device.$idx.status", "disabled");
		cfg_set($cfg, "wpasupplicant.status", get_wpasupplicant_status($cfg));
		cfg_set($cfg, "aaa.$idx.status", "disabled");
		cfg_set($cfg, "aaa.status", get_aaa_status($cfg));
		break;
	case "wep";
		cfg_set($cfg, "wireless.$idx.security.type", $wep_key_len);
		cfg_set($cfg, "wpasupplicant.device.$idx.status", "disabled");
		cfg_set($cfg, "wpasupplicant.status", get_wpasupplicant_status($cfg));
		cfg_set($cfg, "aaa.$idx.status", "disabled");
		cfg_set($cfg, "aaa.status", get_aaa_status($cfg));
		break;
	case "wpa";
	case "wpatkip";
	case "wpaaes";
	case "wpa2";
	case "wpa2tkip";
	case "wpa2aes";
		cfg_set($cfg, "ebtables.sys.status", "enabled");
		cfg_set($cfg, "ebtables.sys.eap.status", "enabled");
		cfg_set($cfg, "ebtables.sys.eap.1.status", "enabled");
		cfg_set($cfg, "ebtables.sys.eap.1.devname", $iface);
		if ($wmode == "ap" || $wmode == "aprepeater")
		{
			cfg_set($cfg, "wpasupplicant.device.$idx.status", "disabled");
			cfg_set($cfg, "wpasupplicant.status", get_wpasupplicant_status($cfg));
			cfg_set($cfg, "wireless.$idx.security.type", "none");
			cfg_set($cfg, "aaa.status", "enabled");
			cfg_set($cfg, "aaa.$idx.status", "enabled");
			if (substr($security, 0, 4) == "wpa2") {
				cfg_set($cfg, "aaa.$idx.wpa.mode", 2);
			}
			else {
				cfg_set($cfg, "aaa.$idx.wpa.mode", 1);
			}
			cfg_set($cfg, "aaa.$idx.wpa.1.pairwise", $pairwise);
			cfg_set($cfg, "aaa.$idx.driver", "madwifi");
			cfg_set($cfg, "aaa.$idx.devname", $iface);
			cfg_set($cfg, "aaa.$idx.br.devname", $br_iface);
			cfg_set($cfg, "aaa.$idx.ssid", cfg_get_essid($cfg, $idx, ""));
		}
		else {
			cfg_set($cfg, "aaa.$idx.status", "disabled");
			cfg_set($cfg, "aaa.status", get_aaa_status($cfg));
			cfg_set($cfg, "wireless.$idx.security.type", "none");
			cfg_set($cfg, "wpasupplicant.status", "enabled");
			cfg_set($cfg, "wpasupplicant.device.$idx.status", "enabled");
			cfg_set($cfg, "wpasupplicant.device.$idx.devname", $iface);
			cfg_set($cfg, "wpasupplicant.device.$idx.driver", "madwifi");
			if (substr($security, 0, 4) == "wpa2") {
				cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.proto.1.name", "RSN");
			}
			else {
				cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.proto.1.name", "WPA");
			}
			cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.pairwise.1.name", $pairwise);
			cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.ssid", cfg_get_essid($cfg, $idx, ""));
		}
		break;
	default;
		cfg_set($cfg, "wireless.$idx.security.type", "none");
		cfg_set($cfg, "aaa.$idx.status", "disabled");
		cfg_set($cfg, "aaa.status", get_aaa_status($cfg));
		cfg_set($cfg, "wpasupplicant.device.$idx.status", "disabled");
		cfg_set($cfg, "wpasupplicant.status", get_wpasupplicant_status($cfg));
		cfg_set($cfg, "ebtables.sys.eap.status", "disabled");
		break;
	}
);

Function set_wpa_ap $cfg, $idx, $wpa_auth, $wpa_key, 
			$radius_auth_ip, $radius_auth_port, $radius_auth_secret, 
                        $radius_acct_ip, $radius_acct_port, $radius_acct_secret, 
                        $radius_acct_status
(
	if (!strlen($wpa_auth)) {
		$wpa_auth = "WPA-PSK";
	}

	cfg_set($cfg, "aaa.$idx.wpa.key.1.mgmt", $wpa_auth);
	if ($wpa_auth == "WPA-PSK") {
		cfg_set($cfg, "aaa.$idx.radius.auth.1.status", "disabled");
		cfg_set($cfg, "aaa.$idx.radius.acct.1.status", "disabled");
		cfg_set($cfg, "aaa.$idx.wpa.psk", $wpa_key);
		cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.psk", $wpa_key);
	}
	else {
		if (!strlen($radius_acct_status)) {
			$radius_acct_status = "disabled";
		}
		cfg_set($cfg, "aaa.$idx.radius.auth.1.status", "enabled");
		cfg_set($cfg, "aaa.$idx.radius.auth.1.ip", $radius_auth_ip);
		cfg_set($cfg, "aaa.$idx.radius.auth.1.port", $radius_auth_port);
		cfg_set($cfg, "aaa.$idx.radius.auth.1.secret", $radius_auth_secret);
                if ($radius_acct_status == "enabled") {
       			cfg_set($cfg, "aaa.$idx.radius.acct.1.status", "enabled");
	       		cfg_set($cfg, "aaa.$idx.radius.acct.1.ip", $radius_acct_ip);
       			cfg_set($cfg, "aaa.$idx.radius.acct.1.port", $radius_acct_port);
	       		cfg_set($cfg, "aaa.$idx.radius.acct.1.secret", $radius_acct_secret);
                } else {
                	cfg_set($cfg, "aaa.$idx.radius.acct.1.status", "disabled");
                }
	}
        cfg_set($cfg, "aaa.$idx.radius.macacl.status", "disabled");
);

Function set_radius_macacl $cfg, $iface, $idx, $enabled, 
	$radius_auth_ip, $radius_auth_port, $radius_auth_secret,
	$radius_acct_ip, $radius_acct_port, $radius_acct_secret,
        $radius_acct_status, $radius_macacl_format, $radius_mac_acl_passwd
(
	if (!strlen($enabled)) {
		$enabled = "disabled";
	}

	cfg_set($cfg, "aaa.$idx.radius.macacl.status", $enabled);
	cfg_set($cfg, "aaa.$idx.radius.auth.1.status", $enabled);
	if (!strlen($radius_acct_status) || $enabled == "disabled") {
		$radius_acct_status = "disabled";
	}
	cfg_set($cfg, "aaa.$idx.radius.acct.1.status", $radius_acct_status);
        
	if ($enabled == "enabled") {
        	cfg_set($cfg, "aaa.status", "enabled");
		cfg_set($cfg, "aaa.$idx.devname", $iface);
		cfg_set($cfg, "aaa.$idx.radius.macacl.format", $radius_macacl_format);
                if (strlen($radius_mac_acl_passwd) == 0) {
                	$radius_mac_acl_passwd = "enabled";
                }
		cfg_set($cfg, "aaa.$idx.radius.macacl.password.status", $radius_mac_acl_passwd);
		cfg_set($cfg, "aaa.$idx.status", "enabled");
		cfg_set($cfg, "aaa.$idx.wpa.mode", 0);
	       	cfg_set($cfg, "aaa.$idx.radius.auth.1.ip", $radius_auth_ip);
       		cfg_set($cfg, "aaa.$idx.radius.auth.1.port", $radius_auth_port);
	       	cfg_set($cfg, "aaa.$idx.radius.auth.1.secret", $radius_auth_secret);
                if ($radius_acct_status == "enabled") {
	       		cfg_set($cfg, "aaa.$idx.radius.acct.1.ip", $radius_acct_ip);
       			cfg_set($cfg, "aaa.$idx.radius.acct.1.port", $radius_acct_port);
	       		cfg_set($cfg, "aaa.$idx.radius.acct.1.secret", $radius_acct_secret);
                }
        } else {
        	cfg_set($cfg, "aaa.status", "disabled");
                cfg_set($cfg, "aaa.$idx.status", "disabled");
        }
);

Function cfg_get_radius_ip $cfg, $idx, $mode, $radius_ip (
	return cfg_get_def($cfg, "aaa.$idx.radius."+$mode+".1.ip", $radius_ip);
);

Function cfg_get_radius_port $cfg, $idx, $mode, $radius_port (
	return cfg_get_def($cfg, "aaa.$idx.radius."+$mode+".1.port", $radius_port);
);

Function cfg_get_radius_secret $cfg, $idx, $mode, $radius_secret (
	return cfg_get_def($cfg, "aaa.$idx.radius."+$mode+".1.secret", $radius_secret);
);

Function set_wpa_sta $cfg, $idx,
	$wpa_auth, $wpa_key, $wpa_eap, $wpa_inner, $wpa_ident,
	$wpa_user, $wpa_passwd, $apmac (
	if (!strlen($wpa_auth)) {
		$wpa_auth = "WPA-PSK";
	}

	cfg_set($cfg, "wpasupplicant.device.$idx.profile", $wpa_auth);
	cfg_set($cfg, "wpasupplicant.profile.$idx.name", $wpa_auth);
	cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.key_mgmt.1.name", $wpa_auth);
	cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.bssid", $apmac);
	if (strlen($wpa_key) != 0) {
		cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.psk", $wpa_key);
		cfg_set($cfg, "aaa.$idx.wpa.psk", $wpa_key);
	}
	if (strlen($wpa_eap) != 0) {
		cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.eap.1.status", "enabled");
		cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.eap.1.name", $wpa_eap);
	}
	else {
		cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.eap.1.status", "disabled");
	}
	if (strlen($wpa_ident) != 0) {
		cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.identity", $wpa_ident);
	}
	if ($wpa_auth != "WPA-PSK") {
		cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.anonymous_identity", $wpa_user);
	}
	if (strlen($wpa_passwd) != 0) {
		cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.password", $wpa_passwd);
	}
	if (strlen($wpa_inner) != 0) {
		cfg_set($cfg, "wpasupplicant.profile.$idx.network.1.phase2", $wpa_inner);
	}
	return 1;
);

Function set_authtype $cfg, $idx, $type (
	if (!strlen($type)) {
		$type = "1";
	}
	return cfg_set($cfg, "wireless.$idx.authmode", $type);
);

Function set_rate $cfg, $idx, $rate, $auto (
	if (!strlen($auto)) {
		$auto = "disabled";
	}
	cfg_set($cfg, "radio.$idx.rate.auto", $auto);
	if ($radio1_legacy == 1) {
		return cfg_set($cfg, "radio.$idx.rate.max", $rate);
	}
	else {
		return cfg_set($cfg, "radio.$idx.rate.mcs", $rate);
	}
);

Function set_clksel $cfg, $idx, $clksel (
	if ($clksel == "2") {
		$clksel = 4;
	}
	elseif ($clksel == "1") {
		$clksel = 2;
	}
	else {
		$clksel = 1;
	}
	return cfg_set($cfg, "radio.$idx.clksel", $clksel);
);

Function set_chanshift $cfg, $idx, $chanshift (
	return cfg_set($cfg, "radio.$idx.chanshift", $chanshift);
);

Function set_ieee_mode $cfg, $idx, $mode (
	return cfg_set($cfg, "radio.$idx.ieee_mode", $mode);
);

Function set_rts_treshold $cfg, $wlan_iface, $rts, $rtsoff (
	if (strlen($rtsoff)) {
		$rts = $rtsoff;
	}
	$idx = get_wlan_index($wlan_iface);
	return cfg_set($cfg, "radio.$idx.rts", $rts);
);

Function set_ack_distance $cfg, $wlan_iface, $ackdistance, $ieee_mode (
	$idx = get_wlan_index($wlan_iface);
	cfg_set($cfg, "radio.$idx.ackdistance", $ackdistance);
	/* backwards compatibility */
	if ($ieee_mode == "b" || $ieee_mode == "2") {
		$minack = 20 * 2 + 3;
	}
	else {
		$minack = 9 * 2 + 3;
	}
	$ack = cfg_set($cfg, "radio.$idx.acktimeout",
		$minack + (intVal($ackdistance) / 150)); /* 150 - speed of light (m/us)/2 */
);

Function set_autoack $cfg, $wlan_iface, $autoack (
	if ($autoack == "on") {
		$autoack = "enabled";
	} elseif ($autoack != "enabled") {
		$autoack = "disabled";
	}
	$idx = get_wlan_index($wlan_iface);
	return cfg_set($cfg, "radio.$idx.ack.auto", $autoack);
);

Function set_fast_frame $cfg, $wlan_iface, $fast_frame (
	if (!strlen($fast_frame)) {
		$fast_frame = "disabled";
	}
	$idx = get_wlan_index($wlan_iface);
	return cfg_set($cfg, "wireless.$idx.fastframes", $fast_frame);
);

Function set_bursting $cfg, $wlan_iface, $burst (
	if (!strlen($burst)) {
		$burst = "disabled";
	}
	$idx = get_wlan_index($wlan_iface);
	return cfg_set($cfg, "wireless.$idx.frameburst", $burst);
);

Function set_compression $cfg, $wlan_iface, $compression (
	if (!strlen($compression)) {
		$compression = "disabled";
	}
	$idx = get_wlan_index($wlan_iface);
	return cfg_set($cfg, "wireless.$idx.compression", $compression);
);

Function cfg_get_essid $cfg, $idx, $essid (
	if (!strlen($essid)) {
		$essid = "UBNT";
	}
	return cfg_get_def($cfg, "wireless.$idx.ssid", $essid);
);

Function cfg_get_apmac $cfg, $idx, $apmac (
	return cfg_get_def($cfg, "wireless.$idx.ap", $apmac);
);

Function cfg_get_country $cfg, $wlan_iface, $country (
	global $radio;
	if (count($radio) < 2) {
		init_board_inc($wlan_iface);
	}

	if ($radio["ccode_locked"] == 1 || $radio["ccode_fixed"] == 1) {
		$ret = $radio["ccode"];
	} else {
		if (!strlen($country)) {
			$country = $radio["ccode"];
			if (!strlen($country)) {
				$country = "840";
			}
		}
		$idx = get_wlan_index($wlan_iface);
		if ($idx == 1) {
			$key = "radio.countrycode";
		}
		else {
			$key = "radio.$idx.countrycode";
		}
		$ret = cfg_get_def($cfg, $key, $country);
	}

	return $ret;
);

Function cfg_get_txpower $cfg, $idx, $txpower (
	if (!strlen($txpower)) {
		$txpower = w_get_txpower($wlan_iface);
	}
	return cfg_get_def($cfg, "radio.$idx.txpower", $txpower);
);

Function cfg_get_obey $cfg, $wlan_iface, $obey (
	$idx = get_wlan_index($wlan_iface);
	$obey_value = cfg_get_def($cfg, "radio.$idx.reg_obey", "disabled");
	if ($obey_value == "enabled") {
		$obey_value = cfg_get_def($cfg, "radio.$idx.obey", $obey);
	}
	return $obey_value;
);

Function cfg_get_rate $cfg, $wlan_iface, $rate (
	$idx = get_wlan_index($wlan_iface);
	if ($radio1_legacy == 1) {
		if (!strlen($rate)) {
			$rate = w_get_rate($wlan_iface);
		}
		return cfg_get_def($cfg, "radio.$idx.rate.max", $rate);
	} else {
		if (!strlen($rate)) {
			$rate = 15;
		}
		return cfg_get_def($cfg, "radio.$idx.rate.mcs", $rate);
	}
);

Function cfg_get_channel_scan_list $cfg, $idx, $channel_scan_list
(
	if (!strlen($channel_scan_list))
	{
		$channel_scan_list = "disabled";
	}
	return cfg_get_def($cfg, "wireless.$idx.scan_list.status", $channel_scan_list);
);

Function set_channel_scan_list $cfg, $idx, $channel_scan_list
(
	if ($channel_scan_list != "enabled") {
		$channel_scan_list = "disabled";
	}
	return cfg_set($cfg, "wireless.$idx.scan_list.status", $channel_scan_list);
);

Function cfg_get_scan_channels $cfg, $idx, $scan_channels
(
	return cfg_get_def($cfg, "wireless.$idx.scan_list.channels", $scan_channels);
);

Function set_scan_channels $cfg, $idx, $scan_channels
(
	return cfg_set($cfg, "wireless.$idx.scan_list.channels", $scan_channels);
);

Function cfg_get_mac_acl $cfg, $idx, $mac_acl
(
	if (!strlen($mac_acl)) {
		$mac_acl = "disabled";
	}
	return cfg_get_def($cfg, "wireless.$idx.mac_acl.status", $mac_acl);
);

Function set_mac_acl $cfg, $idx, $mac_acl_status
(
	if ($mac_acl_status != "enabled") {
		$mac_acl_status = "disabled";
	}
	return cfg_set($cfg, "wireless.$idx.mac_acl.status", $mac_acl_status);
);

Function set_mac_acl_policy $cfg, $idx, $mac_acl_policy
(
	if ($mac_acl_policy != "deny") {
		$mac_acl_policy = "allow";
	}
	return cfg_set($cfg, "wireless.$idx.mac_acl.policy", $mac_acl_policy);
);

Function cfg_get_mac_acl_policy $cfg, $idx, $policy
(
	if (!strlen($policy)) {
		$policy = "allow";
	}
	return cfg_get_def($cfg, "wireless.$idx.mac_acl.policy", $policy);
);

Function cfg_get_clksel $cfg, $wlan_iface, $clksel (
	$idx = get_wlan_index($wlan_iface);
	$clksel = cfg_get_def($cfg, "radio.$idx.clksel", $clksel);
	if ($clksel == "4") {
		$clksel = 2;
	}
	elseif ($clksel == "2") {
		$clksel = 1;
	}
	else {
		$clksel = 0;
	}
	return $clksel;
);

Function cfg_get_chanshift $cfg, $wlan_iface, $chanshift (
	if (!strlen($chanshift)) { $chanshift = 0; }
	$idx = get_wlan_index($wlan_iface);
	$ret = cfg_get_def($cfg, "radio.$idx.chanshift", $chanshift);
	return $ret;
);

Function cfg_get_ieee_mode $cfg, $wlan_iface, $mode (
	global $radio;
	if (!IsSet($radio["name"])) {
		init_board_inc($wlan_iface);
	}

	if ($radio["ccode_fixed"] == 1  &&
            $radio["caps"] & $radio_cap_11n_no_ht40) {
		if ($radio["ieee_mode_a"] == 1) {
			$ret = "11naht";
		}
		else {
			$ret = "11nght";
		}
	}
	else {
		if (!strlen($mode)) {
			$mode = "11nght";
		}
		$idx = get_wlan_index($wlan_iface);
		$ret = cfg_get_def($cfg, "radio.$idx.ieee_mode", $mode);
	}
	return $ret;
);

Function cfg_get_security $cfg, $wlan_iface, $security, $wmode (
	$idx = get_wlan_index($wlan_iface);
	$sec = cfg_get_def($cfg, "wireless.$idx.security.type", "none");
	$newsec = substr($sec, 0, 3);
	if ($newsec == "wep") {
		$sec = "wep";
	}
	else {
		$newsec = "";
		switch ($wmode)
		{
		case "aprepeater";
		case "ap";
			$w_status = cfg_get_def($cfg, "aaa.status", "disabled");
			$w_dev_status = cfg_get_def($cfg, "aaa.$idx.status", "enabled");
			$newsec = cfg_get_def($cfg, "aaa.$idx.wpa.mode", 1 );
			$pkey = cfg_get_def($cfg, "aaa.$idx.wpa.1.pairwise", "TKIP CCMP" );
			break;
		case "sta";
		default;
			$w_status = cfg_get_def($cfg, "wpasupplicant.status", "disabled");
			$w_dev_status = cfg_get_def($cfg, "wpasupplicant.device.$idx.status", "enabled");
			$newsec = cfg_get_def($cfg, "wpasupplicant.profile.$idx.network.1.proto.1.name", "WPA" );
			$pkey = cfg_get_def($cfg, "wpasupplicant.profile.$idx.network.1.pairwise.1.name", "TKIP CCMP" );
			break;
		};
		if ($w_status == "enabled" && $w_dev_status == "enabled" && $newsec != 0) {
			$sec = "wpa";
			if ($newsec == "RSN" || $newsec == 2) {
				$sec = "wpa2";
			}
			switch ($pkey)
			{
			case "TKIP";
				$sec = $sec + "tkip";
				break;
			case "CCMP";
				$sec = $sec + "aes";
				break;
			}
		}
	}
	return $sec;
);

Function cfg_get_def_wep_id $cfg, $idx, $wep_key_id (
	if (!strlen($wep_key_id)) {
		$wep_key_id = 1;
	}
	return cfg_get_def($cfg, "wireless.$idx.security.default_key", $wep_key_id);
);

Function cfg_get_wep_key $cfg, $idx, $wep_key_id, $wep_key (
	if (!strlen($wep_key_id)) {
		$wep_key_id = 1;
	}
	return cfg_get_def($cfg, "wireless.$idx.security.$wep_key_id.key", $wep_key);
);

Function cfg_get_wep_key_length $cfg, $idx, $len (
	$key_len = cfg_get_def($cfg, "wireless.$idx.security.type", "wep64");
	if ($key_len == "none") {
		$key_len = "wep64";
	}
	return $key_len;
);

Function cfg_get_wpa_key $cfg, $idx, $wmode, $wpa_key (
	if ($wmode == "ap" || $wmode == "aprepeater") {
		$wpa_key = cfg_get_def($cfg, "wpasupplicant.profile.$idx.network.1.psk", $wpa_key);
		$wpa_key = cfg_get_def($cfg, "aaa.$idx.wpa.psk", $wpa_key);
	} else {
		$wpa_key = cfg_get_def($cfg, "aaa.$idx.wpa.psk", $wpa_key);
		$wpa_key = cfg_get_def($cfg, "wpasupplicant.profile.$idx.network.1.psk", $wpa_key);
	}
	return $wpa_key;
);

Function cfg_get_authtype $cfg, $idx, $type (
	if (!strlen($type)) {
		$type = 1;
	}
	return cfg_get_def($cfg, "wireless.$idx.authmode", $type);
);

Function cfg_get_ackdistance $cfg, $wlan_iface, $sltconst (
	$idx = get_wlan_index($wlan_iface);
	$distance = cfg_get_def($cfg, "radio.$idx.ackdistance", "");
	if (!strlen($distance)) {
		$ack = cfg_get_def($cfg, "radio.$idx.acktimeout", 0);
		$ack = intVal($ack);
		$ack -= $sltconst;
		if ($ack > 0) {
			$distance = $ack * 150; /* speed of light (m/us)/2 */
		}
		else {
			$distance = 0;
		}
	}
	return intVal($distance);
);

Function cfg_get_autoack $cfg, $wlan_iface, $autoack (
	if (!strlen($autoack)) {
		$autoack = "disabled";
	}
	$idx = get_wlan_index($wlan_iface);
	return cfg_get_def($cfg, "radio.$idx.ack.auto", $autoack);
);

Function cfg_get_fast_frame $cfg, $wlan_iface, $fast_frame (
	if (!strlen($fast_frame)) {
		$fast_frame = "disabled";
	}
	$idx = get_wlan_index($wlan_iface);
	return cfg_get_def($cfg, "wireless.$idx.fastframes", $fast_frame);
);

Function cfg_get_bursting $cfg, $wlan_iface, $burst (
	if (!strlen($burst)) {
		$burst = "disabled";
	}
	$idx = get_wlan_index($wlan_iface);
	return cfg_get_def($cfg, "wireless.$idx.frameburst", $burst);
);

Function cfg_get_compression $cfg, $wlan_iface, $compression (
	if (!strlen($compression)) {
		$compression = "disabled";
	}
	$idx = get_wlan_index($wlan_iface);
	return cfg_get_def($cfg, "wireless.$idx.compression", $compression);
);

Function cfg_get_ipv4addr $cfg, $iface, $ip (
	if (!strlen($ip)) {
		$ip = if_get_ipaddr($iface);
	}
	$idx = get_iface_index($cfg, $iface);
	$ip = cfg_get_def($cfg, "netconf.$idx.ip", $ip);
	return $ip;
);

Function cfg_get_ipv4mask $cfg, $iface, $mask (
	if (!strlen($mask)) {
		$mask = "255.255.255.0";
	}
	$idx = get_iface_index($cfg, $iface);
	$mask = cfg_get_def($cfg, "netconf.$idx.netmask", $mask);
	return $mask;
);

Function cfg_get_ipv4gw $cfg, $iface, $gw (
	if (!strlen($gw)) {
		$gw = "0.0.0.0";
	}
	return cfg_get_def($cfg, "route.1.gateway", $gw);
);

Function cfg_get_firewall $cfg, $firewall, $netmode
(
	$tbl = "ebtables";
	if ($netmode != "bridge") {
		$tbl = "iptables";
	}
	$f = cfg_get_def($cfg, $tbl+".status", $firewall);
	$firewall_chain = cfg_get_def($cfg, $tbl+".sys.fw.status", $firewall);

	if ($f == "enabled" && $firewall_chain == "enabled")
	{
		$firewall = "enabled";
	}
	else {
		$firewall = "disabled";
	}
	return $firewall;
);

Function get_mgmt_info $cfg (
	$res["http"] = cfg_get_def($cfg, "httpd.port", "80");
	$res["airview"] = cfg_get_def($cfg, "airview.tcp_port", "18888");
	$res["icmp"] = "8";
	$res["sptest"] = "9";
	if (cfg_get_def($cfg, "sshd.status", "enabled") == "enabled") {
		$res["ssh"] = cfg_get_def($cfg, "sshd.port", "22");
	}
	if (cfg_get_def($cfg, "httpd.https.status", "disabled") == "enabled") {
		$res["https"] = cfg_get_def($cfg, "httpd.https.port", "443");
	}
	if (cfg_get_def($cfg, "telnetd.status", "disabled") == "enabled") {
		$res["telnet"] = cfg_get_def($cfg, "telnetd.port", "23");
	}        
        if (cfg_get_def($cfg, "snmp.status", "disabled") == "enabled") {
        	$res["snmp"] = "161";
        }
        if (cfg_get_def($cfg, "discovery.status", "enabled") == "enabled") {
        	$res["discover"] = "1001";
        }
	return $res;
);

Function cfg_update_dmz_mgmt $cfg (
	$netmode = cfg_get_def($cfg, "netmode", "bridge");
        if ($netmode != "bridge" &&
	    cfg_get_def($cfg, "iptables.status", "disabled") == "enabled" &&
	    cfg_get_def($cfg, "iptables.sys.status", "disabled") == "enabled" &&
	    cfg_get_def($cfg, "iptables.sys.dmz.status", "disabled") == "enabled" &&
	    cfg_get_def($cfg, "iptables.sys.dmz.1.except.status", "disabled") == "enabled")
	{
		$ports = get_mgmt_info($cfg);
		Reset($ports);
		$i = 1;
		$c = count($ports);
		while($i <= $c);
			$key = key($ports);
			$proto = "TCP";
			if ($key == "snmp" || $key == "sptest") {
				$proto = "UDP";
			}
			$port = $ports[$key];
			if ($key == "icmp") {
				$proto = "ICMP";
			}
			cfg_set($cfg, "iptables.sys.dmz.1.except."+$i+".port", $port);
			cfg_set($cfg, "iptables.sys.dmz.1.except."+$i+".proto", $proto);
			cfg_set($cfg, "iptables.sys.dmz.1.except."+$i+".status", "enabled");
			next($ports);
			$i++;
		endwhile;
		while($i <= 8);
			if (cfg_get_def($cfg, "iptables.sys.dmz.1.except."+$i+".status", "disabled") == "enabled")
			{
				cfg_set($cfg, "iptables.sys.dmz.1.except."+$i+".status", "disabled");
			}
			$i++;
		endwhile;
        }
);

Function set_antenna $cfg, $idx, $div (
	return cfg_set($cfg, "radio.$idx.antenna.id", $div);
);

Function get_antenna_diversity_old $cfg, $idx, $div (
	$tx = cfg_get_def($cfg, "radio.$idx.tx_antenna", 0);
	$div = cfg_get_def($cfg, "radio.$idx.tx_antenna_diversity", $div);
	$ext = cfg_get_def($cfg, "radio.$idx.ext_antenna", 0);
	if ($ext != 0) {
		$div = 5;
	}
	else {
		if ($div == "enabled" || $tx == 0) {
			$div = 4;
		}
		else {
			$div = $tx;
		}
	}
	return $div;
);

Function get_antenna_diversity $cfg, $idx, $div (
	$div = cfg_get_def($cfg, "radio.$idx.antenna.id", -1);
	if ($div == -1) {
		$div = get_antenna_diversity_old($cfg, $idx, $div);
	}
	return $div;
);

Function get_manual_antenna_gain $cfg, $idx, $antenna_gain (
	if (!strlen($antenna_gain)) { $antenna_gain = 0; }
	return cfg_get_def($cfg, "radio.$idx.antenna.gain", $antenna_gain);
);

Function get_cable_loss $cfg, $idx, $cable_loss (
	if (!strlen($cable_loss)) { $cable_loss = 0; }
	return cfg_get_def($cfg, "radio.$idx.cable.loss", $cable_loss);
);

Function set_antenna_gain $cfg, $idx, $antenna_gain, $cable_loss (
	cfg_set($cfg, "radio.$idx.antenna.gain", $antenna_gain);
	cfg_set($cfg, "radio.$idx.cable.loss", $cable_loss);
);

Function has_builtin_antenna (
	global $radio;
	$i = 0; $has_builtin = 0;
	while ($i < $radio["ant_count"] && $has_builtin == 0) {
		$has_builtin = $radio["ant_builtin_$i"];
		$i++;
	}
	return $has_builtin;
);

Function get_antenna_index $ant_id (
	global $radio;
	$i = 0; $idx = 0;
	while ($i < $radio["ant_count"]) {
		if ($radio["ant_id_$i"] == $ant_id) {
			$idx = $i;
		}
		$i++;
	}
	return $idx;
);

Function generate_js_regdomain $cc, $name, $ieee_mode_a, $ieee_mode_bg, $chanbw
(
	global $cmd_regdomain;
	$modestring = "";
	$chanshift_flag = "";
	if ($ieee_mode_a) {
		$modestring += " -A -T ";
		$chanshift_flag = " -s 5";
	}
	if ($ieee_mode_bg) {
		$modestring += " -B -G -S ";
		$chanshift_flag = " -s 2";
	}
	$cc = intval($cc);
	echo "/* " + $cmd_regdomain + $cc + $modestring + " -r -a -j -w 3 -b " + $chanbw + $chanshift_flag + " */\n";
	echo ""+$name+" = [\n";
	PassThru($cmd_regdomain + $cc + $modestring + " -r -a -j -w 3 -b " + $chanbw + $chanshift_flag);
	/* workaround for explorer array termination (,) problem: one extra element at the end. */
	echo "\"\"];\n";
);

Function set_wds_info $cfg, $idx, $auto, $peer1, $peer2, $peer3, $peer4, $peer5, $peer6 (
	if ($auto == "on" || $auto == "enabled") {
		$auto = "enabled";
	} else {
		$auto = "disabled";
	}
	cfg_set($cfg, "wireless.$idx.autowds", $auto);
	cfg_set($cfg, "wireless.$idx.wds.1.peer", $peer1);
	cfg_set($cfg, "wireless.$idx.wds.2.peer", $peer2);
	cfg_set($cfg, "wireless.$idx.wds.3.peer", $peer3);
	cfg_set($cfg, "wireless.$idx.wds.4.peer", $peer4);
	cfg_set($cfg, "wireless.$idx.wds.5.peer", $peer5);
	return cfg_set($cfg, "wireless.$idx.wds.6.peer", $peer6);
);

Function get_wds_info $cfg, $idx (
	$auto = cfg_get($cfg, "wireless.$idx.autowds");
	$peer1 = cfg_get($cfg, "wireless.$idx.wds.1.peer");
	$peer2 = cfg_get($cfg, "wireless.$idx.wds.2.peer");
	$peer3 = cfg_get($cfg, "wireless.$idx.wds.3.peer");
	$peer4 = cfg_get($cfg, "wireless.$idx.wds.4.peer");
	$peer5 = cfg_get($cfg, "wireless.$idx.wds.5.peer");
	$peer6 = cfg_get($cfg, "wireless.$idx.wds.6.peer");
	$info[0] = $auto;
	$info[1] = $peer1;
	$info[2] = $peer2;
	$info[3] = $peer3;
	$info[4] = $peer4;
	$info[5] = $peer5;
	$info[6] = $peer6;
	return $info;
);

Function module_loaded $module
(
	global $proc_modules;
	$lines = @file($proc_modules);
	if ($lines == "-1" || count($lines) == 0 || strlen($module) == 0) {
		$result = -1;
	}
	else {
		$result = 0;
		$c = count($lines);
		$l = strlen($module);
		while ($c) {
			$c--;
			if (substr($lines[$c], 0, $l) == $module) {
				$result = 1;
				$c = 0;
			}
		}
	}
	return $result;
);

Function is_test
(
	global $test_lock_file;
	$result = 1;
	if (fileinode($test_lock_file) == -1) {
		$result = 0;
	} else {
		$fp = @fopen($test_lock_file, "r");
		if ($fp != -1) {
			$result = @fgets($fp, 10);
			$result = intval($result);
			@fclose($fp);
		}
	}
	return $result;
);

Function is_testmode_active
(
	global $test_lock_file;
	$result = 0;
	if (substr(sprintf("%o",filePerms($test_lock_file)), 3, 3) == "755") {
		$result = 1;
	}
	return $result;
);

Function cfg_get_vlan_status $cfg, $iface, $vlan_status (
	if ($vlan_status == "on" || $vlan_status == "enabled")
	{
		$vlan_status = "enabled";
	}
	else
	{
		$vlan_status = "disabled";
	}
	$enabled = cfg_get_def($cfg, "vlan.status", $vlan_status);
	$vlan_status = cfg_get_def($cfg, "vlan.1.status", $vlan_status);

	if ($enabled != "enabled" || $vlan_status != "enabled")
	{
		$vlan_status = "disabled";
	}
	return $vlan_status;
);

Function cfg_get_vlan_tag $cfg, $iface, $vlan_tag (
	return cfg_get_def($cfg, "vlan.1.id", $vlan_tag);
);

Function cfg_get_vlan_ifc $cfg, $iface, $vlan_ifc (
	global $wlan_iface;

	$netmode = cfg_get_def($cfg, "netmode", "bridge");
	$ifc = cfg_get_def($cfg, "vlan.1.devname", $iface);
	if ($netmode == "bridge") {
		if ($ifc == $wlan_iface) {
			return "wlan";
		} else {
			return "lan";
		}
	} else {
		if ($ifc == $iface) {
			return "wan";
		} else {
			return "lan";
		}
	}
);

Function init_board_inc $iface
(
	global $radio;
	$keys[] = "name";
	$keys[] = "shortname";
	$keys[] = "bus";
	$keys[] = "ccode";
	$keys[] = "ccode_locked";
	$keys[] = "ccode_fixed";
	$keys[] = "subsystemid";
	$keys[] = "subvendorid";
	$keys[] = "txpower_max";
	$keys[] = "txpower_offset";
	$keys[] = "low_txpower_atten";
	$keys[] = "low_txpower_limit";
	$keys[] = "ieee_modes";
	$keys[] = "ieee_mode";
	$keys[] = "ieee_mode_a";
	$keys[] = "ieee_mode_bg";
	$keys[] = "ant_count";
	$keys[] = "def_antenna";
	$keys[] = "caps";
	$keys[] = "distance_limit_km";
	$keys[] = "chains";
	$keys[] = "ieee_mode_bg";
	$keys[] = "ieee_mode";
	$keys[] = "low_txpower_mode";
	$keys[] = "legacy";
	$keys[] = "antennas";
	$keys[] = "chanbw";
	$keys[] = "ext_channel";

	$idx_keys[] = "ant_id";
	$idx_keys[] = "ant_name";
	$idx_keys[] = "ant_gain";
	$idx_keys[] = "ant_builtin";
	$idx_keys[] = "ant_chains";

	$idx = get_wlan_index($iface);
	$radio["index"] = $idx;

	$i = 0;
	while ($i < count($keys)) {
		$cfg_var = "radio$idx" + "_$keys[$i]";
		global $$cfg_var;
		$radio[$keys[$i]] = $$cfg_var;
		$i++;
	}

	$i = 0;
	while ($i < count($idx_keys)) {
		$cfg_var = "radio$idx" + "_$idx_keys[$i]";
		global $$cfg_var; $arr = $$cfg_var;
		$j = 0;
		while ($j < count($$cfg_var)) {
			$key_name = $idx_keys[$i]+ "_$j";
			$radio[$key_name] = $arr[$j];
			$j++;
		}
		$i++;
	}
);

Function get_wpasupplicant_status $cfg
(
	$max_radio = 2;
	$i = 1; $status = "disabled";
	while ($i <= $max_radio && $status == "disabled") {
		$device_status = cfg_get_def($cfg, "wpasupplicant.device.$i.status", "disabled");
		if ($device_status == "enabled") {
			$status = "enabled";
		}
		$i++;
	}
	return $status;
);

Function get_aaa_status $cfg
(
	$max_radio = 2;
	$i = 1; $status = "disabled";
	while ($i <= $max_radio && $status == "disabled") {
		$aaa_status = cfg_get_def($cfg, "aaa.$i.status", "disabled");
		if ($aaa_status == "enabled") {
			$status = "enabled";
		}
		$i++;
	}
	return $status;
);

Function get_status $status_value
(
	$status = "disabled";
	if (strlen($status_value)) {
		$status = "enabled";
	}
	return $status;
);

Function cfg_get_obey_default $cfg, $wlan_iface
(
	global $radio;
	if (($radio["ccode_fixed"] == 1) || (has_builtin_antenna() != 1)) {
		$obey_default = "disabled";
	} else {
		$obey_default = "enabled";
	}
	return $obey_default;
);

Function cfg_get_dfs $cfg, $wlan_iface
(
	$dummy = "dummy";
	$idx = get_wlan_index($wlan_iface);
	$dfs = cfg_get_def($cfg, "radio.$idx.dfs.status", $dummy);
	if ($dfs == $dummy || strlen($dfs) == 0) {
		$def_obey = cfg_get_obey_default($cfg, $wlan_iface);
		$dfs = cfg_get_obey($cfg, $wlan_iface, $def_obey);
	}
	return $dfs;
);

Function change_country $cfg, $idx, $country, $subsystemid
(
	global $cfg_file, $cmd_bgnd, $cmd_ccode;
	set_country($cfg, $idx, $country, $subsystemid);
	cfg_save($cfg, $cfg_file);
	exec("$cmd_bgnd -r ccode -- $cmd_ccode $country &", $lines, $res);
);

Function cfg_put $f_name, $str
(
	$res = 0;
	$f = @fopen($f_name, "w");
	if ($f != -1) {
		$str = ereg_replace(13, "", $str);
		fputs($f, $str);
		fclose($f);
		cfg_set_modified($f_name);
	} else {
		$res = -2;
	}
        return $res;
);

Function fill_eth_speed
(
	echo "<option value=\"0\">" + dict_translate("Auto") + "</option>";
	echo "<option value=\"1\">" + dict_translate("100Mbps-Full") + "</option>";
	echo "<option value=\"2\">" + dict_translate("100Mbps-Half") + "</option>";
	echo "<option value=\"3\">" + dict_translate("10Mbps-Full") + "</option>";
	echo "<option value=\"4\">" + dict_translate("10Mbps-Half") + "</option>";
);

Function get_eth_speed $cfg, $ifname
(
	$speed = 0;
	$idx = get_iface_index($cfg, $ifname);
	if ($idx > 0) {
		$eth_autoneg = cfg_get_def($cfg, "netconf.$idx.autoneg", "enabled");
		if ($eth_autoneg != "enabled") {
			$eth_duplex = cfg_get_def($cfg, "netconf.$idx.duplex", "enabled");
			$eth_speed = cfg_get_def($cfg, "netconf.$idx.speed", "100");
			if ($eth_speed == "100") {
				$speed = 1;
			}
			elseif ($eth_speed == "10") {
				$speed = 3;
			}
			if ($speed > 0 && $eth_duplex != "enabled") {
				$speed += 1;
			}
		}
	}

	return $speed;
);

Function set_eth_speed $cfg, $ifname, $ifspeed
(
	$idx = get_iface_index($cfg, $ifname);
	if ($idx > 0) {
		$speed_int = IntVal($ifspeed);
		if ($speed_int == 0) { /* easy way - just enable 'auto' */
			cfg_set($cfg, "netconf.$idx.autoneg", "enabled");
		}
		else {
			if ($speed_int % 2 == 1) {
				$duplex = "enabled";
			}
			else {
				$duplex = "disabled";
			}

			if ($speed_int <= 2) {
				$speed = 100;
			}
			else {
				$speed = 10;
			}

			cfg_set($cfg, "netconf.$idx.autoneg", "disabled");
			cfg_set($cfg, "netconf.$idx.duplex", $duplex);
			cfg_set($cfg, "netconf.$idx.speed", $speed);
		}
	}
);

Function get_role_index $cfg, $role
(
	$index = 0; /* not found */
	$idx = 1;
	$devname = "dummy";

	while ($index == 0 && strlen($devname) != 0 && $idx <= 32) {
		$devname = cfg_get_def($cfg, "netconf.$idx.devname", "");
		$devrole = cfg_get_def($cfg, "netconf.$idx.role", "");
		if ($devrole == $role) {
			$index = $idx;
		}
		$idx++;
	}
	return $index;
);

>
