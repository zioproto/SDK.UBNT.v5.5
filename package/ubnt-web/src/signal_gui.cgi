#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file_bak);
include("lib/l10n.inc");
include("lib/misc.inc");

$chain_names = get_chain_names($cfg);
$chain1_name = $chain_names[0];
$chain2_name = $chain_names[1];
>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
  <title><? echo get_title($cfg, dict_translate("Antenna Alignment Tool")); ></title>
  <meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
  <link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css" />
  <script type="text/javascript" language="javascript" src="jsl10n.cgi"></script>
  <script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
  <script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/signal.js"></script>
  <script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/slider-min.js"></script>
  <link type="text/css" rel="StyleSheet" href="FULL_VERSION_LINK/bluecurve.css" />
  <script type="text/javascript" language="javascript">
var timerID = null;
var lastSignal = null;

function reloadSignal() {
	timerID = null;
	jQuery.getJSON("signal.cgi?"+(new Date().getTime()), update);
}

function refreshDisplay(s)
{
	lastSignal = s;
	$('#signalinfo .switchable').toggle(s != null && s.signal != 0);
	if (typeof updateSignalLevel == 'function' && s != null)
		updateSignalLevel(s.signal, s.rssi, s.noisef, s.chwidth, s.rx_chainmask,
		s.chainrssi, s.chainrssiext);
}

function update(s) {
	refreshDisplay(s);
	if (timerID != null)
		clearTimeout(timerID);
	timerID = setTimeout('reloadSignal()', 500);
}

function createSlider() {
	var slider = new Slider(document.getElementById("slider-1"),
			document.getElementById("slider-input-1"));
	var rs = document.getElementById("rssifield");
	var n = document.getElementById("noisef");
	slider.setMaximum(80);
	slider.onchange = function() {
		var val = this.getValue();
		if (n)
	       	noise = parseFloat(n.innerHTML);
        else
           	noise = -95;
        if (isNaN(noise) || noise >= 0) noise = -95;
		if (rs) rs.value = noise + val;
		RSSI_Max = val;
		refreshDisplay(lastSignal);
	}
	rs.slider = slider;
	rs.onchange = function() {
		var intVal = parseFloat(this.value);
		if (isNaN(intVal)) intVal = -95;
			if (n)
				noise = parseFloat(n.innerHTML);
		else
			noise = -95;
		if (isNaN(noise) || noise >= 0) noise = -95;
        intVal = -1 * (noise - intVal);
		this.slider.setValue(intVal);
	}
	rs.onchange();
}
function init() {
	createSlider();
	reloadSignal();
}
jQuery(document).ready(init);
//-->
</script>

</head>

<body class="popup">
<br />
<form action="#">
<table cellspacing="0" cellpadding="0" align="center" style="width: 490px;" class="popup">
	<tr><th><? echo dict_translate("Antenna Alignment"); ></th></tr>
	<tr>
	  <td>
		<div id="signalinfo" class="row">
		  <span class="label"><? echo dict_translate("Signal Level:"); ></span>
		  <span class="value">
			<span class="percentborder switchable"><div id="signalbar" class="mainbar">&nbsp;</div></span>
			<span class="switchable">&nbsp;</span>
			<span id="signal"></span>
		  </span>
		</div>
		 <div id="signal_chain" class="row initial_hide">
                        <span class="label"><? echo $chain1_name; >&nbsp;/&nbsp;<? echo $chain2_name; >:</span>
                        <span class="value">
                                <span id="signal_0">&nbsp;</span>
                                <span>&nbsp;/&nbsp;</span>
                                <span id="signal_1">&nbsp;</span>
                                <span>&nbsp;dBm</span>
                        </span>
		</div>
		<div class="row">
		  <span class="label"><? echo dict_translate("Noise Level:"); ></span>
		  <span class="value">
			<span id="noisef"></span>
		  </span>
		</div>
		<div class="row">
		  <span class="label"><? echo dict_translate("Max Signal:"); ></span>
		  <span class="value">
			<div class="slider" id="slider-1">
	          <input class="slider-input" id="slider-input-1" name="slider-input-1"/>
			</div>
			<input type="text" class="std_width" id="rssifield" name="rssifield"
				size="4" value="-65" />
			<span>&nbsp;dBm</span>
		  </span>
		</div>
	  </td>
	</tr>
</table>
</form>
</body>
</html>
