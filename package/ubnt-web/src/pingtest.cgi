#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN"
    "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title><? echo get_title($cfg, dict_translate("Ping")); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
<link href="FULL_VERSION_LINK/pingtest.css" rel="stylesheet" type="text/css">
<script type="text/javascript" language="javascript" src="jsl10n.cgi"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.utils.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/ajax.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/util.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/jsval.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/system.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/pingtest.js"></script>
<script type="text/javascript" language="javascript">
<!--
var iplist;
function validateManualIP(id,name,value) {
	var o = document.getElementById('dst_addr_select');
	if (!o) return false;
	if (o.selectedIndex == 0) {
		return validateIP(value) && value != "0.0.0.0";
	}
	return true;
}
function setupValidation() {
        var o = document.getElementById('dst_addr_input');
        if (o) { o.req=1; o.callback='validateManualIP'; o.realname='<? echo dict_translate("Destination IP"); >'; }
        o = document.getElementById('ping_count');
        if (o) { o.req=1; o.minvalue=1; o.maxvalue=10000;o.realname='<? echo dict_translate("Packet Count"); >'; }
        o = document.getElementById('ping_size');
        if (o) { o.req=1; o.minvalue=1; o.maxvalue=32768;o.realname='<? echo dict_translate("Packet Size") + " [1-32768]"; >'; }
}
function _runPing(f) {
	if (validateStandard(f, 'error')) {
		runPing(f);
	}
}
function init() {
	iplist = new IPList('dst_addr_select','dst_addr_input','ip_refresh');
	setupValidation();
	$('#dst_addr_select').change(function() {
		$('#dst_addr_input').enable($('#dst_addr_select').val() == '0');
	});
}
-->
</script>
</head>
<body class="popup" onLoad="init();">
	<form name="pingtest" target="ping_results" enctype="multipart/form-data" action="pingtest_action.cgi" method="POST" onSubmit="_runPing(this);return false;">
		<table cellspacing="0" cellpadding="0" align="center" class="popup">
		<tr><th colspan="3"><? echo dict_translate("Network Ping"); ></th></tr>
		<tr><td colspan="3"><? include("lib/error.tmpl");></td></tr>
		<tr>
		<td colspan="3">
		<div id="pt_content">
		  <table cellspacing="0" cellpadding="0" class="pt_opt">
		  <tr><td class="h"><? echo dict_translate("Select Destination IP:"); ></td>
		  <td><select class="std_width" id="dst_addr_select" name="dst_addr_select"><option value="0"><? echo dict_translate("specify manually"); ></option></select><img id="ip_refresh" src="FULL_VERSION_LINK/images/refresh.png " width="16" height="16"></td>
		  <td style="width:3em;">&nbsp;</td>
		  <td class="h"><? echo dict_translate("Packet Count:"); ></td>
		  <td><input type="text" class="std_width" id="ping_count" name="ping_count" value="5" size="6"></td>
		  </tr>
		  <tr>
		  <td class="h">&nbsp;</td>
		  <td><input type="text" class="std_width" id="dst_addr_input" name="dst_addr_input" size="18"></td>
		  <td> </td>
		  <td class="h"><? echo dict_translate("Packet Size:"); ></td>
		  <td><input type="text" class="std_width" id="ping_size" name="ping_size" value="56" size="6"></td>
		  </tr>
		  <tr>
		    <td colspan="5" align="center" style="vertical-align: top;">
			<table cellspacing="0" cellpadding="0" class="pingh">
				<tr>
					<td class="h" style="width: 180px;"><? echo dict_translate("Host"); ></td>
					<td class="h" style="width: 140px;"><? echo dict_translate("Time"); ></td>
					<td class="h" ><? echo dict_translate("TTL"); ></td>
				</tr>
			</table>
			<div id="scroll_results" class="pingc">
			<table cellspacing="0" cellpadding="0" id="pingdata" class="pingdata">
				<tbody><!-- place for generated ping results -->
				</tbody>
			</table>
			</div>
			<table cellspacing="0" cellpadding="0" class="pingh">
			<tr>
				<td class="f" colspan="3">
					<? echo sprintf(
						dict_translate("msg_ping_stats|%s of %s packets received, %s loss"),
						"<span style=\"font-weight: normal;\" id=\"status_rcv\" class=\"status\">0</span>",
						"<span style=\"font-weight: normal;\" id=\"status_sent\" class=\"status\">0</span>",
						"<span style=\"font-weight: normal;\" id=\"status_loss\" class=\"status\">0</span><span style=\"font-weight: normal\">%</span>")>
				</td>
			</tr>
			<tr>
			<td class="f"><? echo dict_translate("Min:"); > 
				<span id="status_min" class="status">0</span> <strong>ms</strong></td>
			<td class="f"><? echo dict_translate("Avg:"); > 
				<span id="status_avg" class="status">0</span> <strong>ms</strong></td>
			<td class="f"><? echo dict_translate("Max:"); > 
				<span id="status_max" class="status">0</span> <strong>ms</strong></td>
			</tr>
			</table>

		    </td>
		  </tr>
		  <tr>
		  <th colspan="5">&nbsp;<input type="hidden" name="action" value="pingtest"></th>
                  </tr>
                  <tr>
		  <td colspan="5" class="change" id="pingb">
		  <input type="button" id="ping" name="ping" value="Start" onClick="_runPing(this.form);">
		  </td>
		  </tr>

		  </table>
		</div>
		</td>
		</tr>
		</table>
	</form>
</body>
</html>
