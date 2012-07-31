#!/sbin/cgi
<?
if (isset($av_warning) && $av_warning == "skip") {
	$start = 1;
}
/* check whether airview server is already running - if it does, go to stage 2 */
if (!isset($start) || $start != 2) {
	$retval = 1;
	exec("/sbin/airview status", $out, $retval);
	if ($retval == 0) {
		$start = 2;
	}
}
/* Stage 1 - start the server */
if (isset($start) && (strlen($start) > 0) && $start == 1) {
	$a = exec("/sbin/airview web_start 1>/dev/null 2>/dev/null", $retval);
	header("Location: $PHP_SELF?start=2");
	exit;
}
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
$l10n_no_cookies = 1;
include("lib/l10n.inc");
$port = strstr($HTTP_HOST, ":");
if (strlen($port) > 1) {
	$hostname_and_port = $HTTP_HOST;
} else {
	$hostname_and_port = $HTTP_HOST + ":" + $SERVER_PORT;
}
if ($HTTPS == "on") { 
	$protocol = "https";
} else {
	$protocol = "http";
}
>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title><? echo get_title($cfg, dict_translate("airView")); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<meta http-equiv="Expires" content="0">
<meta http-equiv="Cache-Control" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/images/airview.ico" >
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/deployJava.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/js/jquery.cookie.js"></script>
<script type="text/javascript" language="javascript">
//<!--
function deploy(url) {
	var minimumVersion = "1.6.0_15+";
	deployJava.returnPage = url;

	$('#loader').show();

	if (!deployJava.isWebStartInstalled(minimumVersion)) {
		if (deployJava.installLatestJRE()) {
			if (deployJava.launch(url)) {
				window.setTimeout("window.close()",1000);
			}
		}
	} else {
		if (deployJava.launch(url)) {
			// NB: If JWS is not installed then launch will invoke jnlp... leave the window up?
			// window.setTimeout("window.close()",1000);
		}
	}
}

var countdownTimer = null;
function update_countdown() {
	var s = new Number($('#counter').text());
	if (!isNaN(s) && s > 1) {
		$('#counter').text(s.valueOf() - 1);
	} else {
		clearInterval(countdownTimer);
		window.close();
	}
}

$(document).ready(function(){
	var jnlp_url      = "<? echo $protocol; ?>://<? echo $hostname_and_port; ?>/FULL_VERSION_LINK/airview.jnlp";
	var launch_stage1 = "<? echo $protocol; ?>://<? echo $hostname_and_port; ?>/FULL_VERSION_LINK/airview.cgi?start=1";

	$('a.a_stage0').attr('href', launch_stage1);
	$('a.jnlp').attr('href', jnlp_url);
<? if (isset($start) && (strlen($start) > 0) && $start == 2) { >
	$('.stage0').hide();
	deploy(jnlp_url);
	$('#loader').hide();
	$('.stage2').show();
	$('#counter').text(60);
	countdownTimer = setInterval("update_countdown()", 1000);
<? } else { >
	$('.stage0').show();
	$('.stage2').hide();
	$('#av_warn').click(function(){
		if ($(this).is(':checked')) {
			$.cookie('av_warning', 'skip', { path: '/', expires: 360 });
		} else {
			$.cookie('av_warning', null);
		}
	});
<? } >
});
//-->
</script>
</head>
<body class="popup">
<br />
<form action="#">
<table cellspacing="0" cellpadding="0" align="center" style="width: 490px;" class="popup">
<tr><th><? echo dict_translate("airView"); > <? echo dict_translate("Spectrum Analyzer"); ></th></tr>
<tr><td>&nbsp;</td></tr>
<tr class="initial_hide stage0"><td align="center">
<? echo dict_translate("msg_airview_warning|WARNING: Launching airView Spectrum Analyzer <br/> <span style=\"color: red\">WILL TERMINATE</span> <br/> all wireless connections on the device!"); >
<br /><br />
<input type="checkbox" id="av_warn" name="av_warn" value="" /><label for="av_warn"><? echo dict_translate("msg_airview_warning_skip|Do NOT warn me about this in the future."); ></label>
</td></tr>
<tr class="initial_hide stage0"><td align="center">
		<br />
		<a class="a_stage0" href="#">
			<img class="middle" height="32" width="32" src="FULL_VERSION_LINK/images/airview_32.png" border="0" />
		</a> 
		<span style="font-size: 200%; vertical-align: middle;">
			<a class="a_stage0" href="#"><? echo dict_translate("Launch airView"); ></a> <br/>
		</span>
</td></tr>
<tr class="initial_hide" id="loader"><td align="center">
        <img src="FULL_VERSION_LINK/images/ajax-loader.gif" alt="..." title="..."/>
</td></tr>
<tr class="initial_hide stage2"><td>
	<div valign="middle" align="center">
		<center><a class="jnlp" href="#"><img height="32" width="32" src="images/airview_32.png" border="0" /></a></center>
<? echo dict_translate("msg_airview_startup|airView should be starting now.<br/><br/>If you are not prompted to install Java and airView does not start,<br />it is possible that your browser is not yet supported by Java Web Start.<br /><br />If this is the case, you can download the <a class=\"jnlp\" href=\"#\">Java Network Launch Protocol (jnlp)</a> file <br />and configure your operating system and/or browser to open files of this type with <br />the javaws (Java Web Start) program."); >
		<br/><br/><br/>
	</div>
</td></tr>
<tr class="initial_hide stage2"><td align="center">
<? echo dict_translate("msg_airview_window_autoclose|This window will be automatically closed in <span id=\"counter\"></span>&nbsp;seconds"); >
</td></tr>
<tr><td align="center">
<br /><br />
<input type="button" value="<? echo dict_translate("Close this window"); >" onclick="window.close();">
</td></tr>
</table>
</form>
</body>
</html>
