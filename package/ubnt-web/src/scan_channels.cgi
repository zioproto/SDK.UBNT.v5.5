#!/sbin/cgi
<?
	include("lib/settings.inc");
	$cfg = @cfg_load($cfg_file);
	include("lib/l10n.inc");
	include("lib/link.inc");

	$wmode_type = get_wmode_type(cfg_get_wmode($cfg, $wlan_iface));
	$selected_chans = ereg_replace("[\" ]","", $scan_channels);
	$radio1_chanbw = cfg_get_def($cfg, "radio.1.chanbw", "0");
>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title><? echo get_title($cfg, dict_translate("Frequency List")); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
<script type="text/javascript" language="javascript1.2" src="FULL_VERSION_LINK/slink.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" language="javascript">
//<!--
<?
if ($radio1_ieee_mode_a == 1 && $radio1_ieee_mode_bg == 0) {
        if ($ieee_mode != "at" && $ieee_mode != "ast" && $ieee_mode != "a") {
                $ieee_mode = "a";
        }
} elseif ($radio1_ieee_mode_bg == 1 && $radio1_ieee_mode_a == 0) {
        if ($ieee_mode != "b" && $ieee_mode != "g" && $ieee_mode != "pureg") {
                $ieee_mode = "g";
        }
}

generate_js_regdomain($country, "full_regdomain", $radio1_ieee_mode_a, $radio1_ieee_mode_bg, $radio1_chanbw);
>

var ieee_mode = "<?echo $ieee_mode>".toLowerCase();
var clksel = "<?echo $clksel>";
var chanshift="<?echo $chanshift>";
var obey = '<? echo $obey; >' == 'true';
var is_ap = '<? echo $wmode_type; >' == '2';
var rg_data = parse_full_regdomain(full_regdomain);
var regdomain = rg_data.regdomain;
var channels = get_scan_channels(regdomain, ieee_mode, clksel, chanshift, obey, is_ap);

function selectChannels() {
	var chans = $('.frq:checked').map(function(i, n) { return $(n).val(); }).get();
	window.opener.setScanChannels("<?echo $elemId>", chans.join(","));
	window.close();
	return false;
}

function addRow(tbody, cols) {
	tbody.push('<tr>');
	tbody.push(cols.join(''));
	tbody.push('</tr>');
}

function fillTable(channels, selected_channels) {
	var col_count = 5;
	var tbody = [], cols = [];
	$('#channels > tbody').empty();
	
	cols.push('<td colspan="' + col_count + '"><input type="checkbox" id="allfreq" />Select All</td>');
	addRow(tbody, cols); cols = [];

	var i, c = 0, count = 0;
	for (i in channels) {
		var chan = channels[i];
		count++;

		cols.push('<td>');
		cols.push('<input type="checkbox" class="frq" value="' + chan + '"');
		if ($.inArray(chan, selected_channels) > -1)
			cols.push(' checked');
		cols.push('/>' + chan + ' MHz');
		cols.push('</td>');

		if ((++c % col_count) == 0) {
			addRow(tbody, cols); cols = [];
		}
	}

	if (cols.length > 0) {
		for (i = cols.length; i < col_count; ++i)
			cols.push('<td>&nbsp;</td>');
		addRow(tbody, cols);
	}

	$('#channels > tbody').append(tbody.join(''));

	$('#allfreq').change(function() {
		$('.frq').attr('checked', $(this).is(':checked'));
	});

	$('.frq').change(function() {
		var len = $('.frq:checked').length;
		$('#allfreq').attr('checked', len == count);
	});
	$('.frq').change();
}

$(document).ready(function() {
	var selected_channels = "<? echo $selected_chans; >".split(",");
	fillTable(channels, selected_channels);
});

//-->
</script>
</head>

<body class="popup">
<br>
<form enctype="multipart/form-data" action="#" method="POST" onSubmit="return selectChannels();">
	<table id="channels" class="popup" align="center" cellspacing="0" cellpadding="0">
		<thead>
			<tr><th colspan="5"><? echo dict_translate("Frequency List"); ><th><tr>
		</thead>
		<tbody>
		</tbody>
	</table>
	<br />
	<table align="center" cellspacing="0" cellpadding="0">
		<tr>
			<td><input class="fixwidth" type="submit" value="<? echo dict_translate("OK");>"></td>
			<td><input class="fixwidth" type="button" value="<? echo dict_translate("Close"); >"
				onClick="window.close()"></td>
		</tr>
	</table>
</form>
</body>
</html>
