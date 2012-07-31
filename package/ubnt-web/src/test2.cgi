#!/sbin/cgi
<?
include("lib/settings.inc");

if ($action!="test") {
	if ($feature_poe_passthrough == 1) {
		if (!isset($poe_test)) {
			$poe_test="on";
		}
	}

	if (!isset($stress_test)) {
		$stress_test="on";
	}
}

if (!isset($poe_cnt)) {
	$poe_cnt="10";
}
if (!isset($stress_dur)) {
	$stress_dur="5";
}


>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>Testing utility</title>
<style type="text/css">
body, td, th, table {
    font-family: Verdana, Arial, Helvetica, sans-serif;
    font-size: 12px;
}
th { border: 1px solid #aaa;
background-color: #eee;
color: #900;
text-align: left;
text-indent: 10px;
white-space: nowrap;
margin-left: 10px;
font-size: 14px;
font-weight: bold;
letter-spacing: 0.3em;
width: 100%;
}
td { padding-left: 20px; padding-right: 20px; padding-top: 5px }
div.errmsg {
color: red;
font-weight: bold;
border-width: 1px 1px 0px 1px;
background-color: #FFFCE2; 
border-style: solid;
border-color: #666; 

padding: 5px 5px 5px 5px;
}
div.err {
color: red;
font-weight: bold;
font-size: 32px;
border-width: 0px 1px 1px 1px;
background-color: #FFFCE2; 
border-style: solid;
border-color: #666; 
padding: 5px 5px 5px 5px;
}
div.msg {
font-weight: bold;
border-width: 1px 1px 0px 1px;
background-color: #FFFCE2; 
border-style: solid;
border-color: #666; 
padding: 5px 5px 5px 5px;
}
div.success {
color: #009D53;
font-weight: bold;
font-size: 32px;
border-width: 0px 1px 1px 1px;
background-color: #FFFCE2; 
border-style: solid;
border-color: #666; 
padding: 5px 5px 5px 5px;
}
input[type=submit], input[type=reset] {
background: #eee;
color: #222;
border: 1px outset #ccc;
padding: .1em .5em;
}
input[type=text], input[type=file]{ padding: .25em .5em }
input[type=text]:focus, input[type=password]:focus, input[type=file]:focus { 
border: 1px solid #886 
}
input[type=text], input[type=file]{
background: #fff;
color: #000;
border: 1px solid #d7d7d7;
}
table { border: 0px; width: 700px }
table.data { border: 0px; width: 600px }
table.data th { border: 0px; background-color: #fff; color: #333; font-size: 12px; font-weight: bold; width: 30%; letter-spacing: normal}
table.data input[type=submit] { margin-right:100px; width: 6em; }
div#progress {
display: block;
border: 1px solid grey;
text-align: left;
width: 600px;
}
</style>
<script type="text/javascript" language="javascript" src="jsl10n.cgi"></script>
<script language="javascript1.2" type="text/javascript" src="FULL_VERSION_LINK/progress.js"></script> 
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" language="javascript">
//<!--
var duration = 0;
var check_timer = false;


function init() {
<?	
	if ($REQUEST_METHOD=="POST" && $action=="test") {
		echo "duration=0;";
		
		if (strlen($poe_test)!=0) {
			echo "duration="+intval($poe_cnt)+" * 14;";
		}
		
		if (strlen($stress_test) != 0) {
			echo "duration=duration+"+intval($stress_dur)+" * 60+60;";
		}
>
		var progress = new Progress("progress", "FULL_VERSION_LINK/images/p.gif", 600);

		var o = document.getElementById("prg_duration");
		o.innerHTML = "(duration "+duration+" sec.)";
		
		progress.run(duration * 1000, "shut(true)");
			checkDone();
<?
	}
>
	togglePOE();
	toggleStress();
}

function checkDone() {
	// check if we are still being displayed at all
	jQuery.getJSON("testdone.cgi?"+(new Date().getTime()), function(d) {
		if (d.done) {
			shut(true);
			return false;
		}
		if (check_timer)
			clearTimeout(check_timer);
			
		var o = document.getElementById("prg_duration");
		o.innerHTML = "(remaining "+duration+" sec.)";
		check_timer = setTimeout("checkDone()", 1000 * 5);
		if (duration != 5 ) {
			duration = duration - 5;
		}
	});
	return false;
}


function shut(reload) {
	if (reload) {
		var url = document.location.href;
		var idx = url.indexOf('?');
		if (idx > 0) {
			url = url.substring(0, idx);
		}
		
		if (check_timer)
			clearTimeout(check_timer);

		url += "?";
		url += "poe_test=" + "<?echo urlencode($poe_test)>" + "&";
		url += "poe_cnt=" + "<?echo urlencode($poe_cnt)>" + "&";
		url += "stress_test=" + "<?echo urlencode($stress_test)>" + "&";
		url += "stress_dur=" + "<?echo urlencode($stress_dur)>";
		document.location.href = url;
	}
}
function setEnabled(o, enabled) {
	if (!o) return false;
	o.disabled = !enabled;
	if (enabled) { o.style.backgroundColor = "#FFFFFF"; }
	else { o.style.backgroundColor = "#f0f0f0"; }
}

function togglePOE() {
	var o = document.getElementById("poe_cnt");
        var d = document.getElementById("poe_test");
       	setEnabled(o, d.checked);
}

function toggleStress() {
	var o = document.getElementById("stress_dur");
        var d = document.getElementById("stress_test");
       	setEnabled(o, d.checked);
}


function sync(cbox, text) {
        var o = document.getElementById(text);
      	var c = document.getElementById(cbox);
	setEnabled(o, c.checked);
        toggleDownload();
}

jQuery(document).ready(init);

// -->
</script>
</head>
<body bgcolor=white>
<a href="index.cgi">Main</a>
<table align="center">
<tr><th>Test Settings 
	</th>
</tr>
<tr><td>
<form enctype="multipart/form-data" action="<?echo $PHP_SELF;>" method="POST">
<table class="data">

<th>Stress test</th>
<td><input id="stress_test" name="stress_test" type="checkbox" value="on" <?if(strlen($stress_test)!=0) { echo "checked";}> onClick="toggleStress();"></td>
</tr>
<th>Duration(min):</th>
<td><input id="stress_dur" name="stress_dur" type="text" size="64" value="<?echo htmlspecialchars($stress_dur)>" <?if(strlen($stress_test)==0) { echo "disabled=\"disabled\"";}> > </td>
</tr>

<?if ($feature_poe_passthrough == 1) {>

<th>POE Pass-through test</th>
<td><input id="poe_test" name="poe_test" type="checkbox" value="on" <?if(strlen($poe_test)!=0) { echo "checked";}> onClick="togglePOE();"></td>
</tr>
<tr>
<th>Count:</th>
<td><input id="poe_cnt" name="poe_cnt" type="text" size="64" value="<?echo htmlspecialchars($poe_cnt)>"></td>

<?}>


<? if ($REQUEST_METHOD=="POST" && $action=="test") {
	@unlink("/tmp/test-started");
	@unlink("/tmp/test-done");
	@unlink("/tmp/test.sh");
	@unlink("/tmp/test-stress.dat");

	$fp = fopen("/tmp/test.sh", "w");
	fputs($fp, "#!/bin/sh\n");
	fputs($fp, "echo Start > /tmp/test-started\n");
	
	if (strlen($stress_test)!=0) {
		fputs($fp, "/sbin/factorytest stress -d "+intval($stress_dur)+" \n");
	}

	if (strlen($poe_test)!=0) {
		if ($feature_poe_passthrough == 1) {
			fputs($fp, "i=0; while [ \$i -lt $poe_cnt ]; do echo 8 1 1 >/proc/gpio/system_led; sleep 12; echo 8 1 0 >/proc/gpio/system_led; sleep 2; i=`expr \$i + 1`; done\n");
		}
	}

	
	fputs($fp, "echo Done > /tmp/test-done\n");
	fclose($fp);

	chmod("/tmp/test.sh", 755);
	$cmd="/tmp/test.sh";
	bgexec(1, $cmd);
>
<tr><td align="center" colspan="2">Test in progress <span id="prg_duration"></span>...</td></tr>
<tr><td colspan="2"><div id="progress"></div></td></tr>


<?} else {>
<tr>
<td colspan="2" align="right">
<input name="action" type="hidden" value="test">
<input type="submit" value="GO!">
</td>
</tr>
<tr><td colspan="2">
<?

if (fileinode("/tmp/test-started") != -1) {
	$cmd="kill -9 `pidof test.sh` ";
	exec($cmd);
	$cmd="kill -INT `pidof factorytest`";
	exec($cmd);
	@unlink("/tmp/test-started");
	@unlink("/tmp/test-done");
	@unlink("/tmp/test.sh");
	
	
	if (strlen($stress_test)!=0) {
		$stress_status = -1;

		if(fileinode("/tmp/test-stress.dat") != -1) {
			$stresstestdat = @cfg_load("/tmp/test-stress.dat");
			$stress_status = cfg_get_def($stresstestdat, "status", "-1");
		}

		if (intval($stress_status) != 0){
		
			$msg = "Failed";
			echo "<div class=\"errmsg\">\n";
			echo $msg;
			echo "</div><div class=\"err\">STRESS TEST FAILED</div>\n";
		} else {
			echo "<div class=\"msg\">\n";
			echo "Done";
			echo "</div><div class=\"success\">STRESS TEST SUCCESS</div>\n";
		}
		echo "<br>";
	}
	@unlink("/tmp/test-stress.dat");

	if (strlen($poe_test)!=0) {
			$msg = "Finished";
			echo "<div class=\"msg\">\n";
			echo $msg;
			echo "</div><div class=\"success\">POE pass-through TEST</div>\n";
	}


}>
</td></tr>
<?}>
</table>

</form>
</td></tr>
<tr><td>&nbsp;</td></tr>

</table>
</body>
</html>
