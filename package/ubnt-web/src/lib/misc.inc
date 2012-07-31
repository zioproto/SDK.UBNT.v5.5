<?
$wmm_names[0] = dict_translate("No QoS");
$wmm_names[1] = dict_translate("Auto Priority");
$wmm_names[2] = dict_translate("Video Priority");
$wmm_names[3] = dict_translate("Voice Priority");
$wmm_values[0] = -1;
$wmm_values[1] = 0;
$wmm_values[2] = 4;
$wmm_values[3] = 8;

Function wmm_get_status $wmm_level (
   if ($wmm_level == "-1") {
       $status = "disabled";
   }
   else
   {
	   $level = intval($wmm_level);
	   $status = "disabled";
	   switch ($level) {
	   case 0;
	   case 4;
	   case 8;
	       $status = "enabled";
	       break;
	   }
   }

   return $status;
);

Function wmm_get_levelname $wmm_status, $wmm_level (
   global $wmm_names;
   global $wmm_values;
   $levelname = $wmm_names[0];
   if ($wmm_status == "enabled" || $wmm_status == 1) {
       $i = 0;
       while ($i < count($wmm_values)) {
           if ($wmm_values[$i] == $wmm_level) {
               $levelname = $wmm_names[$i];
               $i = count($wmm_values) - 1;
           }
           $i++;
       }
   }

   return $levelname;
);

Function wmm_generate_options $wmm_status, $wmm_level (
   global $wmm_names;
   global $wmm_values;

   $selectedName = wmm_get_levelname($wmm_status, $wmm_level);
    
   $i = 0;
   while ($i < count($wmm_values)) {
        $options += "<option value=\"" + $wmm_values[$i] + "\"";
        $name = $wmm_names[$i];
        if ($name == $selectedName) {
            $options += " selected";
        }
        $options += ">" + $name + "</option>";
        $i++;
   }

   return $options;
);

Function cfg_set_signal_led_source $cfg, $signal_led_source (
        global $feature_gps_leds;
        $led_status = "enabled";
        if ($feature_gps_leds > 0) {
                $gps_leds = 1;
                if ($signal_led_source == 1) {
                        $led_status = "disabled";
                        $gps_leds = $feature_gps_leds;
                }
                cfg_set($cfg, "gps.leds", $gps_leds);
        }
        cfg_set($cfg, "wireless.1.signal_led_status", $led_status);
);

Function cfg_get_signal_led_source $cfg, $signal_led_source (
        global $feature_gps_leds;
        $source = 0;
        if ($feature_gps_leds > 0) {
                $led_status = cfg_get_def($cfg, "wireless.1.signal_led_status", "enabled");
                if ($led_status != "enabled") {
                        $source = 1;
                }
        }
        return $source;
);

Function cfg_set_signal_leds $cfg,  $leds (
	global $feature_rssi_leds;
	$i = 0;
	while ($i < $feature_rssi_leds) {
		$i++;
		cfg_set($cfg, "wireless.1.signal_led"+$i, $leds[$i-1]);
	}
);

Function cfg_get_leds $cfg (
	global $feature_rssi_leds;
	$led_thresholds[0] = 94;
	$led_thresholds[1] = 65;
	$led_thresholds[2] = 65;
	$led_thresholds[3] = 65;
	$led_thresholds[4] = 65;
	$led_thresholds[5] = 65;
	$led_thresholds[6] = 65;
	$led_thresholds[7] = 65;
	switch ($feature_rssi_leds) {
	case 3;
		$led_thresholds[0] = 94;
		$led_thresholds[1] = 77;
		break;
	case 4;
		$led_thresholds[0] = 94;
		$led_thresholds[1] = 80;
		$led_thresholds[2] = 73;
		break;
	case 5;
		$led_thresholds[0] = 94;
		$led_thresholds[1] = 87;
		$led_thresholds[2] = 80;
		$led_thresholds[3] = 72;
		break;
	case 6;
		$led_thresholds[0] = 94;
		$led_thresholds[1] = 88;
		$led_thresholds[2] = 82;
		$led_thresholds[3] = 77;
		$led_thresholds[4] = 71;
		break;
	case 7;
		$led_thresholds[0] = 94;
		$led_thresholds[1] = 89;
		$led_thresholds[2] = 84;
		$led_thresholds[3] = 79;
		$led_thresholds[4] = 74;
		$led_thresholds[5] = 69;
		break;
	case 8;
		$led_thresholds[0] = 94;
		$led_thresholds[1] = 90;
		$led_thresholds[2] = 86;
		$led_thresholds[3] = 82;
		$led_thresholds[4] = 78;
		$led_thresholds[5] = 74;
		$led_thresholds[6] = 69;
		break;
	}
	$i = 0;
	while ($i < $feature_rssi_leds) {
		$i++;
		$v[$i-1] = cfg_get_def($cfg, "wireless.1.signal_led"+$i, $led_thresholds[$i-1]);
	}

	return $v;
);

Function secsToCountdown $time
(
	$time = IntVal($time);
	$days = $time / 86400;
	$hours = ($time / 3600) % 24;
	$minutes = ($time / 60) % 60;
	$secs = $time % 60;

	if ($days > 0) {
		if ($days == 1) {
			$daystr = dict_translate("day");
		} else {
			$daystr = dict_translate("days");
		}
		$result = sprintf("%d %s %.2d:%.2d:%.2d",
			$days, $daystr, 
			$hours, $minutes, $secs);
	}
	else
	{
		$result = sprintf("%.2d:%.2d:%.2d", $hours, $minutes, $secs);
	}
	return $result;
);

Function fast_clock_mode $ieee_mode (
	switch ($ieee_mode) {
		case "11naht40minus";
		case "11naht40plus";
		case "11naht40";
			$fc_mode = "11nght40";
			break;
		case "1";
		case "a";
		case "11naht20";
			$fc_mode = "11nght20";
			break;
		default;
			$fc_mode = $ieee_mode;
			break;
	}
	return $fc_mode;
);

Function BW_SCALEUP $val, $org_bw, $target_bw (
	return ( (($val)*($org_bw)*10) / (($target_bw)*10) );
);

Function get_timings $ieee_mode, $clksel, $fastclock (
	$maxacktimeout = 204;
	$sltconst = 9; /*for 20Mhz width: slot=9*/

	if (!$fastclock) {
		$timings_ieee_mode = $ieee_mode;
	} else {
		$timings_ieee_mode = fast_clock_mode($ieee_mode);
	}

	switch ($timings_ieee_mode)
	{
		case "11naht40minus";
		case "11naht40plus";
		case "11naht40";
			$maxacktimeout = 204;
			break;
		case "1";
		case "a";
		case "11naht20";
			if ($clksel == 1)
			{
				$maxacktimeout = 819; /* 20 half */
			}
			elseif ($clksel == 2)
			{
				$maxacktimeout = 1638; /* 10 quarter */
			}
			else
			{
				$maxacktimeout = 409; /* 40 */
			}
			break;
		case "2";
		case "b";
			$maxacktimeout = 744; /* 22 */
                        $sltconst = 20; /*Long slot*/
			break;
		case "11nght40minus";
		case "11nght40plus";
		case "11nght40";
			$maxacktimeout = 186;
			break;  
		case "3";
		case "gt";
		case "g";
		case "pureg";
		case "11nght20";
			if ($clksel == 1)
			{
				$maxacktimeout = 744; /* 22 half */
			}
			elseif ($clksel == 2)
			{
				$maxacktimeout = 1489; /* 11 quarter */
			}
			else
			{
				$maxacktimeout = 372; /* 44 */
			}
			break;
		case "7";
		case "ast";
		case "5";
		case "at";
			$maxacktimeout = 204; /* 80 */
			break;
	}
        if ($clksel == 1) {
        	$timings[0] = -5 + BW_SCALEUP(20, 20, 10);
		/*
                if ($chan_bandwidth != 10) {
	                $timings[0] = BW_SCALEUP($sltconst + 4, 20, chan_bandwidth);
                }
                */
        } elseif ($clksel == 2) {
        	$timings[0] = -5 + BW_SCALEUP(20, 20, 5);
		/*
                if ($chan_bandwidth != 5) {
			$timings[0] = BW_SCALEUP($sltconst + 4, 20, chan_bandwidth);
                }
                */
        } else {
		$timings[0] = $sltconst + 4;
        }
        $timings[0] += 10;
	$timings[1] = $maxacktimeout;
	return $timings;
);

Function bytes_to_human $bytes (
	$b = DoubleVal($bytes);
	$str = "";
	if ($b > 1073741824) {
		$str = sprintf("%.2f GBytes", $b / 1073741824);
	} else {
		if ($b > 1048576) {
			$str = sprintf("%.2f MBytes", $b / 1048576);
		} else {
			if ($b > 1024) {
				$str = sprintf("%.2f kBytes", $b / 1024);
			}
		}
	}
	return $str;
);

Function get_current_ack (
	$retval = "";
	$ret = Exec("iwpriv wifi0 get_acktimeout", $arr, $result);
	if ($result == 0) {
		if (ereg("([[:print:]]*):([[:print:]]*)$", $arr[0], $res)) {
			$retval = $res[2];
		}
	}

	return $retval;
);

Function get_current_distance (
	$retval = "";
	$ret = Exec("iwpriv wifi0 get_distance", $arr, $result);
	if ($result == 0) {
		if (ereg("([[:print:]]*):([[:print:]]*)$", $arr[0], $res)) {
			$retval = $res[2];
		}
	}

	return $retval;
);

Function is_first_login $cfg, $idx, $uri (
	if (cfg_get_def($cfg, "radio.$idx.countrycode", 0) != 0) {
		$retval = 0;
	}
	else {
		if (isset($uri) && strstr($uri, "test.cgi") > 0) {
			$retval = 0;
		}
		else {
			$retval = 1;
		}
	}
	return $retval;
);

Function devname2ifname $ifname (
	$dev = ereg_replace("br", "BRIDGE", $ifname);
	$dev = ereg_replace("ath", "WLAN", $dev);
	$dev = ereg_replace("eth", "LAN", $dev);
	$dev = ereg_replace("ppp", "TUNNEL", $dev);
	return $dev;
);

function get_bridges_list (
	$netif = "/sys/class/net";
	if (fileinode($netif) != -1)
	{
	      @openDir($netif);
	      $f = readDir();
	      while (strlen($f) != 0 ) {
      		if ($f != "." && $f != "..") {
              		if (fileinode($netif + "/" + $f + "/bridge") != -1) {
                        	$brifc[count($brifc)] = $f;
	      		}
              	}
                $f = readDir();
              }
              @closeDir();
      	}
        return $brifc;
);

function br_portno2ifc $brname, $portno (
	$ifc = "";
	$brif = "/sys/class/net/" + $brname + "/brif";
	if (fileType($brif) == "dir")
	{
        	$done = 0;
		@openDir($brif);
		$f = readDir();
		while (strlen($f) != 0 && !$done) {
			if ($f != "." && $f != "..") {
                        	$port = $brif + "/" + $f + "/port_no";
                                $fp = @fopen($port, "r");
				if ($fp != -1)
				{
	                                $no = fgets($fp, 10);
                                        if (intVal($portno) == HexDec($no)) {
                                        	$done = 1;
                                                $ifc = $f;
                                        }
                                        @fclose($fp);
				}
			}
			$f = readDir();
		}
		@closeDir();
        }
        return $ifc;
);

Function get_chain_names $cfg
(
	global $radio1_subsystemid;

	$chain_names[0] = dict_translate("Chain 0");
	$chain_names[1] = dict_translate("Chain 1");

	if (
        $radio1_subsystemid == "0xe235" || /* NB5		NanoBridge5 */
        $radio1_subsystemid == "0xe2b5" || /* NB5 (New HW revision)		NanoBridge5 */
        $radio1_subsystemid == "0xe1a5" || /* PBM5		PowerBridge M5 */
        $radio1_subsystemid == "0xe012" || /* NanoStation M2 */
        $radio1_subsystemid == "0xe005" || /* NanoStation M5 */
        $radio1_subsystemid == "0xe805" || /* NanoStation M5 (Hitachi FEM)*/
		$radio1_subsystemid == "0xe0a2" || /* NanoStation Loco M2 */
		$radio1_subsystemid == "0xe009" || /* NanoStation Loco M900 */
		$radio1_subsystemid == "0xe2b9" || /* NanoStation M900 N9S	e2b9 */
		$radio1_subsystemid == "0xe035" || /* NanoStationM3 */
		$radio1_subsystemid == "0xe239" || /* NanoBridge M900	NB9 */
		$radio1_subsystemid == "0xe003") /* NanoStation M365 */
	{
		$chain_names[0] = dict_translate("Horizontal");
		$chain_names[1] = dict_translate("Vertical");
	} elseif (
        $radio1_subsystemid == "0xe232" || /* NB2		NanoBridge2 */
        $radio1_subsystemid == "0xe2a3" || /* PowerBridgeM3  */
        $radio1_subsystemid == "0xe1a3" || /* PowerBridgeM365 */
        $radio1_subsystemid == "0xe0a5" || /* NanoStation Loco M5 */
        $radio1_subsystemid == "0xe8a5") /* NanoStation Loco M5 (Hitachi FEM) */
	{
		$chain_names[0] = dict_translate("Vertical");
		$chain_names[1] = dict_translate("Horizontal");
	}

	/* NanoStation Loco M900 Internal + External (2x2) antena names  */
	if ($radio1_subsystemid == "0xe009" && cfg_get_def($cfg, "radio.1.antenna.id", -1) == 6) {
		$chain_names[0] = dict_translate("External");
		$chain_names[1] = dict_translate("Internal (Vertical)");
	}

	return $chain_names;
);

>
