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

  $wmm_status = cfg_get_def($cfg, "wireless.1.wmm", "disabled");
  $level = cfg_get_def($cfg, "wireless.1.wmmlevel", "2");
  $wmode_type = get_wmode_type(cfg_get_wmode($cfg, $wlan_iface));
  $ieee_mode = cfg_get_ieee_mode($cfg, $wlan_iface, $ieee_mode);
  $ieee_mode = strtolower($ieee_mode);
  $netmode_cfg = cfg_get_def($cfg, "netmode", "bridge");
  $obey = cfg_get_obey($cfg, $wlan_iface, "enabled");
  if (strlen($netmode)==0) {
	$netmode = $netmode_cfg;
  }

  $ccode = cfg_get_country($cfg, $wlan_iface, "");
  if ($REQUEST_METHOD == "POST") {
	if (($ccode != 511) && ($radio1_ccode_fixed == 0) && ($obey != "enabled")) {
        	if ($eirp_status != "enabled") {
                        $error_msg = dict_translate("eirp_off_obey_off|'Installer EIRP Control' can not be disabled while 'Auto Adjust to EIRP Limit' under <a href='/link.cgi'>Wireless</a> settings is turned off.");
                }
        }
        if (strlen($error_msg) == 0) {
	$wmm_status = wmm_get_status($wmm_level);
	cfg_set($cfg, "wireless.1.wmm", $wmm_status);
	cfg_set($cfg, "wireless.1.wmmlevel", $wmm_level);
        $level = $wmm_level;
		if (!strlen($eirp_status)) {
			$eirp_status = "disabled";
		}
	set_rts_treshold($cfg, $wlan_iface, $rts, $rtsoff);
	set_ack_distance($cfg, $wlan_iface, $ackdistance, $ieee_mode);
	set_autoack($cfg, $wlan_iface, $autoack);
	if ($radio1_legacy == 0)
	{
		$fast_frame = 0;
		$burst = 0;
		$compression = 0;
	}
	set_fast_frame($cfg, $wlan_iface, $fast_frame);
	set_bursting($cfg, $wlan_iface, $burst);
	set_compression($cfg, $wlan_iface, $compression);
	if ($feature_ratemodule == 1) {
		cfg_set($cfg, "radio.ratemodule", $rate_module);
	}
	cfg_set_signal_led_source($cfg, $signal_led_source);
	$i = 0;
	while ($i < $feature_rssi_leds) {
		$i++;
		$led = "led"+$i;
		$leds[$i-1] = $$led;
	}
	cfg_set_signal_leds($cfg, $leds);

	if ($radio1_legacy != 1) {
		if (strlen($aggr_status)){
			$aggr_status = "enabled";
			cfg_set($cfg, "radio.1.ampdu.frames", $aggr_frames);
			cfg_set($cfg, "radio.1.ampdu.bytes", $aggr_bytes);
		} else {
			$aggr_status = "disabled";
		}
		cfg_set($cfg, "radio.1.ampdu.status", $aggr_status);
	}

	if (strlen($mcast_status)){
        	$mcast_status = "enabled";
        } else {
        	$mcast_status = "disabled";
        }
	if (strlen($client_isolation_status))
	{
		$client_isolation_status = "enabled";
	}
	else
	{
		$client_isolation_status = "disabled";
	}

	if (strlen($mtikie)) {
		$mtikie = "enabled";
	} else {
		$mtikie = "disabled";
	}

	cfg_set($cfg, "wireless.1.addmtikie", $mtikie);

	if ($feature_advanced_ethernet == 1) {
	if ($feature_poe_passthrough == 1) {
		cfg_set($cfg, "gpio.status", "enabled");
		cfg_set($cfg, "gpio.1.status", "enabled");
		cfg_set($cfg, "gpio.1.line", $poe_passthrough_gpio);
		cfg_set($cfg, "gpio.1.direction", 1);
			if (strlen($poe_pass)) {
				$poe_pass = "enabled";
			} else {
				$poe_pass = "disabled";
			}
		cfg_set($cfg, "gpio.1.value", $poe_pass);
		}
		if ($feature_advanced_ethernet_phy == 1) {
			if (isset($eth0_speed)) {
				set_eth_speed($cfg, "eth0", $eth0_speed);
			}
			if ($eth_mac_count > 1 && isset($eth1_speed)) {
				set_eth_speed($cfg, "eth1", $eth1_speed);
			}
		}
	}

	cfg_set($cfg, "netconf.2.allmulti", $mcast_status);
	cfg_set($cfg, "radio.1.mcastrate", $mcast_rate);
	cfg_set($cfg, "wireless.1.l2_isolation", $client_isolation_status);

	cfg_set($cfg, "radio.1.thresh62a", $noise_immunity);
	cfg_set($cfg, "radio.1.thresh62b", $noise_immunity);
	cfg_set($cfg, "radio.1.thresh62g", $noise_immunity);

	if (strlen($rssi_sens)) {
        	$rssi_sens = intval($rssi_sens);
        	if ($rssi_sens > 0) {
                	$rssi_sens = -1 * $rssi_sens;
                } elseif ($rssi_sens < -96) {
                	$rssi_sens = -96;
                }
        	cfg_set($cfg, "wireless.1.sens", 96 + $rssi_sens);
        } else {
        	cfg_set($cfg, "wireless.1.sens", 0);
        }
        
        	cfg_set($cfg, "system.eirp.status", $eirp_status);
	cfg_save($cfg, $cfg_file);
	cfg_set_modified($cfg_file);
	$message = dict_translate("Configuration saved");
	}
  }

  if (!strlen($rts)) {
	$rts = "off";
  }
  $rts = cfg_get_def($cfg, "radio.1.rts", $rts);
  $autoack = cfg_get_autoack($cfg, $wlan_iface, $autoack);
  $fast_frame = cfg_get_fast_frame($cfg, $wlan_iface, $fast_frame);
  $burst = cfg_get_bursting($cfg, $wlan_iface, $burst);
  $compression = cfg_get_compression($cfg, $wlan_iface, $compression);
  $rate_module = cfg_get_def($cfg, "radio.ratemodule", $def_rate_module);
  $wmm_options = wmm_generate_options($wmm_status, $level);
  $poe_pass=cfg_get_def($cfg, "gpio.1.value", $poe_pass);

  if ($feature_advanced_ethernet_phy == 1) {
	  $eth0_speed = get_eth_speed($cfg, "eth0");
	  if ($eth_mac_count > 1) {
		  $eth1_speed = get_eth_speed($cfg, "eth1");
	  }
  }

  $signal_led_source = cfg_get_signal_led_source($cfg, $signal_led_source);
  $leds = cfg_get_leds($cfg);

  if ($radio1_legacy != 1) {
	  $aggr_status = cfg_get_def($cfg, "radio.1.ampdu.status", "enabled");
	  $aggr_frames = cfg_get_def($cfg, "radio.1.ampdu.frames", "32");
	  $aggr_bytes = cfg_get_def($cfg, "radio.1.ampdu.bytes", "50000");
  }

  if (strlen($noise_immunity) == 0) {
  	$noise_immunity = 28;
  }
  $noise_immunity = cfg_get_def($cfg, "radio.1.thresh62a", $noise_immunity);
  $clksel = cfg_get_clksel($cfg, $wlan_iface, $clksel);
  $mcast_status = cfg_get_def($cfg, "netconf.2.allmulti", "enabled");
  $client_isolation_status = cfg_get_def($cfg, "wireless.1.l2_isolation", "disabled");
  $mtikie = cfg_get_def($cfg, "wireless.1.addmtikie", $mtikie);
  if (strlen($mtikie) == 0) {
  	$mtikie = "enabled";
  }
  $eirp_status = cfg_get_def($cfg, "system.eirp.status", $eirp_status);
  if ($obey != "enabled") {
	if ($eirp_status != "enabled") {
               	$eirp_status = "enabled";
	}
  }
  if (!strlen($eirp_status)) {
	if (has_builtin_antenna() != 1) {
       		$eirp_status = "enabled";
  	} else {
       		$eirp_status = "disabled";
        }
  }
  $mcast_rate = cfg_get_def($cfg, "radio.1.mcastrate", $mcast_rate);
  
  $timings = get_timings($ieee_mode, $clksel, $radio1_caps & $radio_cap_fast_clock);
  
  $sltconst = $timings[0];
  $maxacktimeout = $timings[1];
  $ackdistance = cfg_get_ackdistance($cfg, $wlan_iface, $sltconst);
  $minacktimeout = $sltconst;
  $acktimeout = $minacktimeout + ($ackdistance / 150);
  /* If maximum distance limit is placed on this board, we calculate new max ack timeout based
   * on speed of light. */
  if (strlen($radio1_distance_limit_km) && $radio1_distance_limit_km != "0") {
	$speed_of_light_meters_per_microsecond = 300.0;
	$uS = ((( 2 * $radio1_distance_limit_km * 1000.0 ) / $speed_of_light_meters_per_microsecond) ) + 0.5;
	$maxacktimeout = $minacktimeout + $uS;
  }

  $rssi_sens = cfg_get_def($cfg, "wireless.1.sens", 0);
  
  if (!strlen($rssi_sens)) {
  	$rssi_sens = -96;
  } else {
  	$rssi_sens = -96 + $rssi_sens;
  }

  include("lib/advanced.tmpl");
>
