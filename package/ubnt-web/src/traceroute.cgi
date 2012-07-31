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
<title><? echo get_title($cfg, dict_translate("Traceroute")); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
<link href="FULL_VERSION_LINK/traceroute.css" rel="stylesheet" type="text/css">
<script type="text/javascript" language="javascript" src="jsl10n.cgi"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/ajax.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/util.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/jsval.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/traceroute.js"></script>
<script type="text/javascript" language="javascript">
<!--
function init() {
}
-->
</script>
</head>
<body class="popup" onLoad="init()">
	<form name="traceroute" enctype="multipart/form-data" action="traceroute_action.cgi" method="POST" onSubmit="startTraceroute(this);return false;">
		<table cellspacing="0" cellpadding="0" align="center" class="popup">
			<tr><th colspan="3"><? echo dict_translate("Network Traceroute"); ></th></tr>
    		<tr>
		<td colspan="3">
		<div id="tr_content">
		  <table cellspacing="0" cellpadding="0" class="tr_opt">
		  <tr>
		  <td class="h"><? echo dict_translate("Destination Host:"); ></td>
		  <td style="width: 0px;"><input type="text" class="std_width" name="dst_host" id="dst_host"></td>
		  <td class="h"><input type="checkbox" name="resolve" id="resolve"><? echo dict_translate("Resolve IP Addresses"); ></td>
		  </tr>
		  <tr>
		  <td colspan="5" align="center" style="vertical-align: top;">
		    <table cellspacing="0" cellpadding="0" class="trh">
		    <tr>
		    <td class="h" style="width: 15px;">#</td>
		    <td class="h" style="width: 170px;"><? echo dict_translate("Host"); ></td>
		    <td class="h" style="width: 90px;"><? echo dict_translate("IP"); ></td>
		    <td class="h"><? echo dict_translate("Responses"); ></td>
		    </tr>
		    </table>
		    <div id="scroll_results" class="trc">
		    	<table cellspacing="0" cellpadding="0" id="tr_results">
			  <tbody><!-- output -->
			  </tbody>
			</table>
		    </div>
		  </td>
		  </tr>
		  <tr><th colspan="3">&nbsp;<input type="hidden" name="action" value="traceroute"></th></tr>
		  <tr>
		    <td id="tr_b" class="change" colspan="3">
		    <input type="button" id="tr_start" name="tr_start" value="Start" onClick="_traceroute(this.form);">
		    </td>
                  </tr>
		  </table>
		</div>
		</td>
		</table>
	</form>
</body>
</html>
