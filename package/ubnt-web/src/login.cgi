#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
include("lib/link.inc");
include("lib/misc.inc");

$idx = get_wlan_index($wlan_iface);
init_board_inc($wlan_iface);

$first_login = "false";
if (is_first_login($cfg, $idx, $uri) != 0) {
	$first_login = "true";
	if ($radio["ccode_fixed"] != 0) {
		$country = $radio["ccode"];
	}
}

$error_msg = "";

if ($REQUEST_METHOD=="POST") {
	if ($lang_changed != "yes") {
		$rc = PasswdAuth($username, $password);
		if ($rc == 1) {
			$cmd = "/bin/ma-auth $db_sessions $AIROS_SESSIONID $username";
			exec(EscapeShellCmd($cmd));

			if ($first_login == "true") {
				change_country($cfg, $idx, $country, $radio["subsystemid"]);
			}

			if (isset($uri) && strlen($uri) > 0) {
				Header("Location: " + urldecode($uri));
				exit;
			} else {
				Header("Location: /index.cgi");
				exit;
			}
		} else {
			$error_msg = dict_translate("Invalid credentials.");
		}
	}
	$postback = "true";
}
else {
	$postback = "false";
}
>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title><? echo dict_translate("Login"); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<meta http-equiv="Expires" content="0">
<meta http-equiv="Cache-Control" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<link href="FULL_VERSION_LINK/login.css" rel="stylesheet" type="text/css">
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
<script type="text/javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/util.js"></script>
<script type="text/javascript" src="FULL_VERSION_LINK/index.js"></script>
<script type="text/javascript" language="javascript">
//<!--
var globals = {
	first_login : <? echo $first_login; >,
	postback : <? echo $postback; >,
	fixed : <? if ($radio["ccode_fixed"] == 0) { echo "false"; } else { echo "true"; } >,
	country : "<? echo $country; >"
};

function onLangChange() {
	$("#lang_changed").val("yes");
	$("#loginform").submit();
}

function validateForm() {
	if ($("#lang_changed").val() == "yes")
		return true;

	if ($("#country").val() == "0") {
		$("#errmsg").text("<? echo dict_translate("Please select your country."); >");
		return false;
	}

	if (!$("#agreed").is(":checked")) {
		$("#errmsg").html("<? echo dict_translate("To use this product, you must agree to<br/>terms of use."); >");
		return false;
	}

	return true;
}

$(document).ready(function() {
	$("#username").focus();
	cache_images([
		'main_top.png', 'main.png', 'link.png',
		'net.png', '4dv.png', 'srv.png',
		'system.png', 'border.gif', 'spectr.gif']);

	if (globals.first_login) {
		$("#ui_language").change(onLangChange);
		$("#loginform").submit(validateForm);
		if (!globals.postback && !globals.fixed)
			$("#country").val(0);
		else
			$("#country").val(globals.country);
	}
});
//-->
</script>
</head>
<? flush(); >
<body>
<table border="0" cellpadding="0" cellspacing="0" align="center" class="loginsubtable">
<form enctype="multipart/form-data" id="loginform" method="post" action="<?echo $PHP_SELF;>">
	<tr>
		<td valign="top"><img src="FULL_VERSION_LINK/images/airos_logo.png"></td>
		<td class="loginsep">
				<input type="hidden" name="uri" id="uri" value="<? echo HtmlSpecialChars($uri); >" />
				<table border="0" cellpadding="0" cellspacing="0" class="logintable" align="center">
					<tr>
						<td colspan="2" align="center">
							<div id="errmsg" class="error">
								<? if (isset($error_msg) && (strlen($error_msg) > 0)) { echo $error_msg; } >
							</div>
						</td>
					</tr>
					<tr>
						<td colspan="2">&nbsp;</td>
					</tr>
					<tr>
						<td><label for="username"><? echo dict_translate("Username:"); ></label></td>
						<td><input type="text" name="username" id="username" /></td>
					</tr>
					<tr>
						<td><label for="password"><? echo dict_translate("Password:"); ></label></td>
						<td><input type="password" name="password" id="password" maxlength="8"/></td>
					</tr>
					<? if ($first_login == "true") { >
					<tr <? if ($radio["ccode_fixed"] != 0) { echo " style=\"display:none;\"";} >>
						<td><label for="country"><? echo dict_translate("Country:"); ></label></td>
						<td>
							<select name="country" id="country" />
								<? if ($radio["ccode_fixed"] == 0 && $radio["ccode_locked"] != 1) { >
									<option value="0"><? echo dict_translate("Select Your Country");></option>
									<? include("lib/ccode.inc"); >
								<? } else { >
									<option value="<? echo $country; >" selected>Locked</option>
								<? } >
							</select>
						</td>
					</tr>
					<tr>
						<td><label for="ui_language"><? echo dict_translate("Language:"); ></label></td>
						<td>
							<select name="ui_language" id="ui_language">
								<? echo get_language_options($languages, $active_language);>
							</select>
							<input type="hidden" id="lang_changed" name="lang_changed" value="no" />
						</td>
					</tr>
					<? } >
					<tr>
						<td colspan="2">&nbsp;</td>
					</tr>
				</table>
		</td>
	</tr>
	<? if ($first_login == "true") { >
	<tr>
		<td colspan="2"><strong><? echo dict_translate("TERMS OF USE"); ></strong></td>
	</tr>
	<tr>
		<td colspan="2" class="license">
			<? echo dict_translate("license_agreement|This Ubiquiti radio device must be professionally installed. Properly installed shielded Ethernet cable and earth grounding must be used as conditions of product warranty. It is the installer’s responsibility to follow local country regulations including operation within legal frequency channels, output power, and Dynamic Frequency Selection (DFS) requirements. The End User is responsible for keeping the unit working according to these rules."); >
			<? echo dict_translate("license_agreement2|For further information, please visit <a href=\"http://www.ubnt.com/\">www.ubnt.com</a>."); >
		</td>
	</tr>
	<? } >
	<tr>
		<td>
		<? if ($first_login == "true") { >
			<input type="checkbox" id="agreed"></input>
			<label for="agreed"><strong><? echo dict_translate("I agree to these terms of use"); ></strong></label>
		<? } >
		</td>
		<td class="submit" align="right">
			<input type="submit" value="<? echo dict_translate("Login"); >" />
		</td>
	</tr>
</form>
</table>
</body>
</html>
