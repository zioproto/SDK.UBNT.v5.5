#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file_bak);
include("lib/l10n.inc");
include("lib/link.inc");

$autoack = cfg_get_def($cfg, "radio.1.ack.auto", "disabled");
$wmode = cfg_get_wmode($cfg, $wlan_iface);
$polling = cfg_get_def($cfg, "radio.1.polling", "disabled");

if (($polling == "enabled") && ($wmode == "ap" || $wmode == "aprepeater")) {
	$airmax_on = "true";
} else {
	$airmax_on = "false";
}

if ($autoack == "enabled" && $airmax_on == "true") {
	$noack = cfg_get_def($cfg, "radio.1.pollingnoack", "0");
	if ($noack == "1") {
		$autoack = "disabled";
	}
}

if ($mode == "ap") {
	$page_title = dict_translate("AP Info:") + $sta_mac;
} else {
	$page_title = dict_translate("Station Info: ") + $sta_mac;
}
>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>

<head>
<title><? echo get_title($cfg, $page_title); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
</head>

<body class="popup">
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.l10n.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.utils.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/util.js"></script>
<script type="text/javascript">

var ab5BeamAngles = [
'<img src="FULL_VERSION_LINK/images/ab5-p39.png" title="+39 <? echo dict_translate("degrees");>">',
'<img src="FULL_VERSION_LINK/images/ab5-p26.png" title="+26 <? echo dict_translate("degrees");>">',
'<img src="FULL_VERSION_LINK/images/ab5-p13.png" title="+13 <? echo dict_translate("degrees");>">',
'<img src="FULL_VERSION_LINK/images/ab5-0.png" title="0 <? echo dict_translate("degrees");>">',
'<img src="FULL_VERSION_LINK/images/ab5-m13.png" title="-13 <? echo dict_translate("degrees");>">',
'<img src="FULL_VERSION_LINK/images/ab5-m26.png" title="-26 <? echo dict_translate("degrees");>">',
'<img src="FULL_VERSION_LINK/images/ab5-m39.png" title="-39 <? echo dict_translate("degrees");>">',
'<img src="FULL_VERSION_LINK/images/ab5-bcast.png" title="bcast">',
];

var si_global = {
	'mode' : '<? echo $mode; >',
	'ifname' : '<? echo $ifname; >',
	'sta_mac' : '<? echo $sta_mac; >',
	'autoack' : ('<? echo $autoack; >' == 'enabled'),
	'airmax_on' : <? echo $airmax_on; >,
    'phased_array' : ('<? echo $feature_phased_array; >' == '1'),
    'beam_angles' : ab5BeamAngles, /* Currently we only have Airbeam-5, need selectivty in future */
	'_' : '_'
};

var l10n_stainfo = {
	'day' : '<? echo dict_translate("day"); >',
	'days' : '<? echo dict_translate("days"); >',
	'unknown' : '<? echo dict_translate("unknown"); >',
	'Access Point' : '<? echo dict_translate("Access Point"); >',
	'Station' : '<? echo dict_translate("Station"); >',
	'AP-WDS' : '<? echo dict_translate("AP-WDS"); >',
	'N/A' : '<? echo dict_translate("N/A"); >',
	'High': '<? echo dict_translate("High"); >',
	'Medium': '<? echo dict_translate("Medium"); >',
	'Low': '<? echo dict_translate("Low"); >',
	'None': '<? echo dict_translate("None"); >',
	'Not Associated': '<? echo dict_translate("Not Associated"); >',
	'_' : '_'
};

var kick_url = "stakick.cgi?staif=<? echo $ifname;>&staid=<? echo $sta_mac;>";

</script>
<script type="text/javascript" src="FULL_VERSION_LINK/stainfo.js"></script>

<br>
<form action="<?echo $PHP_SELF;>" method="GET">

<table cellspacing="0" cellpadding="0" align="center">
<tr><td>
<table id="stainfo" class="listhead sortable" cellspacing="0" cellpadding="0">
	<tr>
		<th id="si_heading" colspan="2"><? echo dict_translate("Loading..."); ></th>
	</tr>
	<tr id="si_details" style="display: none;">
		<td valign="top">
			<table>
				<tr>
					<td class="f"><? echo dict_translate("Device Name:"); ></td>
					<td id="si_name"></td>
				</tr>

				<tr>
					<td class="f"><? echo dict_translate("Connection Time:"); ></td>
					<td id="si_uptime"></td>
				</tr>
				<tr>
					<td class="f"><? echo dict_translate("Signal Strength:"); ></td>
					<td id="si_signal"></td>
				</tr>
				<tr>
					<td class="f"><? echo dict_translate("Noise Floor:"); ></td>
					<td id="si_noise"></td>
				</tr>
				<tr id='si_ack_row'>
					<td class="f"><? echo dict_translate("Distance:"); ></td>
					<td id="si_ack"></td>
				</tr>
				<tr>
					<td class="f"><? echo dict_translate("CCQ:"); ></td>
					<td id="si_ccq"></td>
				</tr>
				<tr class="si_airmax">
					<td class="f"><? echo dict_translate("airMAX Priority:"); ></td>
					<td id="si_amp"></td>
				</tr>
				<tr class="si_airmax">
					<td class="f"><? echo dict_translate("airMAX Quality:"); ></td>
					<td id="si_amq"></td>
				</tr>
				<tr class="si_airmax">
					<td class="f"><? echo dict_translate("airMAX Capacity:"); ></td>
					<td id="si_amc"></td>
				</tr>
                <tr class="si_airbeam">
                    <td class="f"><? echo dict_translate("Antenna Beam:"); ></td>
                    <td id="si_beam"></td>
                </tr>
				<tr>
					<td class="f"><? echo dict_translate("Last IP:"); ></td>
					<td id="si_lastip"></td>
				</tr>
				<tr>
					<td class="f"><? echo dict_translate("TX/RX Rate:"); ></td>
					<td id="si_rate"></td>
				</tr>
				<tr>
					<td class="f"><? echo dict_translate("TX/RX Packets:"); ></td>
					<td id="si_packets"></td>
				</tr>
				<tr>
					<td class="f"><? echo dict_translate("TX/RX Packet Rate, pps:"); ></td>
					<td id="si_packet_rate"></td>
				</tr>
				<tr>
					<td class="f"><? echo dict_translate("Bytes Transmitted:"); ></td>
					<td id="si_txbytes"></td>
				</tr>
				<tr>
					<td class="f"><? echo dict_translate("Bytes Received:"); ></td>
					<td id="si_rxbytes"></td>
				</tr>
			</table>
		</td>
		<td valign="top">
			<table id="si_rate_tbl">
				<thead>
					<tr>
						<td class="f"><? echo dict_translate("Negotiated Rate"); ></td>
						<td class="f"><? echo dict_translate("Last Signal"); >, dBm</td>
					</tr>
				</thead>
				<tbody>
				</tbody>
			</table>
		</td>
	</tr>

	<tr>
		<td colspan="2">&nbsp;</td>
	</tr>
	<tr>
		<td colspan="2" class="change">
			<input type="button" id="si_kick"
				value="<? if ($mode == "ap") { $str = "Reconnect"; } else { $str = "Kick"; }; echo dict_translate($str); >"
				onClick="kickStation();" />
			<input type="button" id="si_refresh" value="<? echo dict_translate("Refresh"); >"
				onClick="refreshAll();" />
			<input type="button" id="si_close" value="<? echo dict_translate("Close"); >"
				onClick="window.close(); return false;"/>
		</td>
	</tr>
</table>
</td></tr>

</table>

</form>
</body>
</html>
