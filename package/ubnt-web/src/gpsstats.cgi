#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");

if (IsSet($load) && $load == "y") {
	PassThru("cat /tmp/gps_sat_info");
	exit;
}

>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title><? echo get_title($cfg, dict_translate("GPS Details")); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
<script type="text/javascript" language="javascript" src="jsl10n.cgi"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.l10n.js"></script>
<script type="text/javascript" language="javascript">

var l10n_gpsstats = {
	'No data available' : '<? echo dict_translate("No data available"); >',
	'_' : '_'
};

</script>
<script type="text/javascript" src="FULL_VERSION_LINK/gpsstats.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/sorttable.js"></script>
</head>

<body class="popup">
<br>
<form action="gpsstats.cgi" method="GET">
<table cellspacing="0" cellpadding="0" align="center">
<tr><td>
<table id="sats" class="listhead sortable" cellspacing="0" cellpadding="0">
	<thead>
	<tr>
		<th><? echo dict_translate("Satelite"); ></th>
		<th><? echo dict_translate("Signal"); ></th>
		<th><? echo dict_translate("Satelite"); ></th>
		<th><? echo dict_translate("Signal"); ></th>
	</tr>
	</thead>
	<tbody>
	</tbody>
</table>
</td></tr>
<tr><td class="change">
	<input type="button" id="_refresh" onClick="refreshAll()" value=" <? echo dict_translate("Refresh"); > "></td>
</tr></table>
</form>
</body>
</html>
