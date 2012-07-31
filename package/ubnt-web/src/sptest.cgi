#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
	"http://www.w3.org/TR/html4/strict.dtd">
<html>

<head>
<title><? echo get_title($cfg, dict_translate("Speed Test")); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
<link href="FULL_VERSION_LINK/speedtest.css" rel="stylesheet" type="text/css">
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/util.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/sptest.js"></script>
<script type="text/javascript" language="javascript" src="jsl10n.cgi"></script>

<head>

<body class="popup">
<form name="speedtest">

<table cellspacing="0" cellpadding="0" align="center" class="popup">
	<tr>
		<th colspan="3"><? echo dict_translate("Network Speed Test");></th>
	</tr>
	<tr>
		<td colspan="3">
			<table cellspacing="0" cellpadding="0">
				<tr>
					<td>
						<table cellspacing="0" cellpadding="0" class="st_opt">
							<tr>
								<td class="f"><? echo dict_translate("Select Destination IP:");></td>
								<td>
									<select class="std_width" id="dst_addr_select" name="dst_addr_select">
										<option value="0"><? echo dict_translate("specify manually"); ></option>
									</select>
									<img id="ip_refresh" src="FULL_VERSION_LINK/images/refresh.png" width="16" height="16">
								</td>
							</tr>
							<tr>
								<td class="f">&nbsp;</td>
								<td><input type="text" class="std_width" id="dst_addr_input" name="dst_addr_input" size="18"></td>
							</tr>
							<tr>
								<td class="f"><? echo dict_translate("User:");></td>
								<td><input type="text" class="std_width" id="auth_user" name="auth_user" size="18"></td>
							</tr>
							<tr>
								<td class="f"><? echo dict_translate("Password:");></td>
								<td><input type="password" class="std_width" id="auth_password" name="auth_password" size="18" maxlength="8"></td>
							</tr>
							<tr>
								<td class="f"><? echo dict_translate("Remote WEB Port:");></td>
								<td><input type="text" class="std_width" id="launcher_port" name="launcher_port" size="5" maxlength="5" value="80"></td>
							</tr>
							<tr>
								<td class="f">&nbsp;</td>
								<td class="f-left">
									<input type="checkbox" id="show_adv" name="show_adv">
									<label for="show_adv"><? echo dict_translate("Show Advanced Options");></label>
								</td>
							</tr>
							<tr id="advanced1" style="display: none;">
								<td class="f"><? echo dict_translate("Direction:");></td>
								<td>
									<select class="std_width" id="direction_select" name="direction">
										<option value="dx"><? echo dict_translate("duplex");></option>
										<option value="rx"><? echo dict_translate("receive");></option>
										<option value="tx"><? echo dict_translate("transmit");></option>
									</select>
								</td>
							</tr>
							<tr id="advanced2" style="display: none;">
								<td class="f"><? echo dict_translate("Duration:");></td>
								<td><input class="std_width" type="text" id="time_limit" name="time_limit" value="10" size="10" /> &nbsp;<? echo dict_translate("seconds");></td>
							</tr>
						</table>
					</td>
					<td id="st_results">
						<fieldset>
							<legend><? echo dict_translate("Test Results");></legend>
							<table border="0" cellpadding="0" cellspacing="0">
								<tr>
									<td class="f-left"><? echo dict_translate("RX:"); ></td>
									<td class="f" id="rx_results"><? echo dict_translate("N/A"); ></td>
								</tr>
								<tr>
									<td class="f-left"><? echo dict_translate("TX:"); ></td>
									<td class="f" id="tx_results"><? echo dict_translate("N/A"); ></td>
								</tr>
								<tr>
									<td class="f-left"><? echo dict_translate("Total:");></td>
									<td class="f" id="total_results"><? echo dict_translate("N/A"); ></td>
								</tr>
							</table>
						</fieldset>
						<br />
						<span id="results" class="conn_warn"></span>
						<span id="loader" style="display: none; padding: 2px;">
							<img src="FULL_VERSION_LINK/images/ajax-loader.gif" />
							<br/>
							<? echo dict_translate("Running, please wait..."); >
						</span>
					</td>
				</tr>
			</table>
		</td>
	</tr>
	<tr><td colspan="3"><br>
	<div class="roundmsg_box">
	   <div class="roundmsg_top"><div></div></div>
	      <div class="roundmsg_content">
	         <? echo dict_translate("speedtst_warn|Warning! If traffic shaping is enabled on either device the speed test results will be limited accordingly."); >
	      </div>
	   <div class="roundmsg_bottom"><div></div></div>
	</div><td>
        </tr>
	<tr>
		<th colspan="3">&nbsp;</th>
	</tr>
	<tr>
		<td colspan="3" id="run">
			<input id="runtest" type="submit" value="<? echo dict_translate("Run Test"); >">
		</td>
	</tr>
</table>

</form>
</body>

</html>
