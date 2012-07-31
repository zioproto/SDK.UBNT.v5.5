#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file_bak);
include("lib/l10n.inc");
include("lib/misc.inc");
include("lib/link.inc");

$global_ack = 0;
$global_distance = 0;

$wmode = cfg_get_wmode($cfg, $wlan_iface);
$polling = cfg_get_def($cfg, "radio.1.polling", "disabled");

if (($polling == "enabled") && ($wmode == "ap" || $wmode == "aprepeater")) {
    $airmax_on = "true";
} else {
    $airmax_on = "false";
}


$autoack = cfg_get_def($cfg, "radio.1.ack.auto", "disabled");
if ($autoack == "enabled") {
	$noack = cfg_get_def($cfg, "radio.1.pollingnoack", "0");
	$airsync = cfg_get_def($cfg, "radio.1.airsync.status", "disabled");
	if ($polling == "enabled" &&
		($wmode == "ap" || $wmode == "aprepeater") &&
		($noack == "1" || $airsync == "enabled")) {
		$autoack = "disabled";
	}
	else {
		$global_ack = get_current_ack();
		$global_distance = get_current_distance();
	}
}
>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title><? echo dict_translate("Associated Stations"); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
</head>

<body class="popup">
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.l10n.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.utils.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.dataTables.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/util.js"></script>
<script type="text/javascript" language="javascript">

var l10n_stalist = {
	'day' : '<? echo dict_translate("day"); >',
	'days' : '<? echo dict_translate("days"); >',
	'unknown' : '<? echo dict_translate("unknown"); >',
	'AP-WDS' : '<? echo dict_translate("AP-WDS"); >',
	'No Associated Stations' : '<? echo dict_translate("No Associated Stations"); >',
	'Loading, please wait...' : '<? echo dict_translate("Loading, please wait..."); >',
	'_' : '_'
};

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

var sl_global = {
	'wlan_iface' : '<? echo $wlan_iface; >',
	'autoack' : ('<? echo $autoack; >' == 'enabled'),
	'ack' : '<? echo $global_ack; >',
	'distance' : '<? echo $global_distance; >',
	'airmax_on' : <? echo $airmax_on; >,
	'phased_array' : ('<? echo $feature_phased_array; >' == '1'),
	'beam_angles' : ab5BeamAngles, /* Currently we only have Airbeam-5, need selectivty in future */
	'_': '_'
};

</script>
<script type="text/javascript" src="FULL_VERSION_LINK/stalist.js"></script>

<br>
<form action="<?echo $PHP_SELF;>" method="GET">
<table cellspacing="0" cellpadding="0" align="center">
	<tr>
		<td>
			<table id="sta_list" class="listhead dataTables_head" cellspacing="0" cellpadding="0">
				<thead>
					<tr>
						<th><? echo dict_translate("Station MAC"); >&nbsp;&nbsp;&nbsp;</th>
						<th><? echo dict_translate("Device Name"); >&nbsp;&nbsp;&nbsp;</th>
						<th><? echo dict_translate("Signal") + " / "+ dict_translate("Noise"); >, dBm&nbsp;&nbsp;&nbsp;</th>
						<th class="initial_hide"><? echo dict_translate("Beam"); >&nbsp;&nbsp;&nbsp;</th>
						<th class="initial_hide"><? echo dict_translate("Distance"); >&nbsp;&nbsp;&nbsp;</th>
						<th><? echo dict_translate("TX/RX"); >, Mbps&nbsp;&nbsp;&nbsp;</th>
						<th><? echo dict_translate("CCQ"); >, %&nbsp;&nbsp;&nbsp;</th>
						<th><? echo dict_translate("Connection Time"); >&nbsp;&nbsp;&nbsp;</th>
						<th><? echo dict_translate("Last IP"); >&nbsp;&nbsp;&nbsp;</th>
						<th><? echo dict_translate("Action"); >&nbsp;</th>
						<th>&nbsp;</th>
						<th>&nbsp;</th>
					</tr>
				</thead>
				<tbody>
				</tbody>
			</table>
		</td>
	</tr>
	<tr>
		<td class="change">
			<input type="button" id="_refresh" value="<? echo dict_translate("Refresh"); >">
		</td>
	</tr>
</table>
</form>
</body>
</html>
