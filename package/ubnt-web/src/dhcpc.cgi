#!/sbin/cgi
<?
include("lib/settings.inc");
if (fileinode($cfg_file_bak) != -1) {
$cfg = @cfg_load($cfg_file_bak);
} else {
$cfg = @cfg_load($cfg_file);
}
include("lib/l10n.inc");
include("lib/misc.inc");
$user=$REMOTE_USER; $userid=0; $groupid=0; $is_ro=0;
$user_regexp="([^:]+):([^:]+):([^:]+)";
if (ereg($user_regexp,$REMOTE_USER,$res)) {
	$user = $res[1];
        $userid = $res[2];
        $groupid = $res[3];
}
if ($groupid != 0) {
        $is_ro = 1;
}

>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title><? echo get_title($cfg, dict_translate("DHCP Client Information")); ></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
<script type="text/javascript" language="javascript" src="jsl10n.cgi"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/dhcpc.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/js/jquery.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/js/jquery.utils.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/js/jquery.tmpl.js"></script>
</head>

<body class="popup">
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/common.js"></script>
<script type="text/javascript" language="javascript" src="FULL_VERSION_LINK/dhcpc.js"></script>
<script type="text/javascript" language="javascript">
//<!--
dhcpc.warning = "<? echo dict_translate("warn_dhcpc_release|Warning: DHCP client information release may terminate connection to your device."); >";
function dhcpcRefresh() {
	dhcpc.fetch();
	if (typeof reloadStatus == 'function') {
		reloadStatus();
	}
	return false;
}

$(document).ready(function() {
	dhcpc.init();
	dhcpc.fetch(1);
});
//-->
</script>
<script type="text/x-jquery-tmpl" id="dhcpcTmpl">
	{{if error}}
	<tr><td style="vertical-align: top">
		<div class="row">
			<span class="label"><? echo dict_translate("Interface:");></span>
			<span class="value">${devname2uidevname(ifname)}</span>
		</div>
	</td><td style="vertical-align: top">
		<div class="row">
			<span class="label"><? echo dict_translate("Status:"); ></span>
			<span class="value">${error}</span>
		</div>
	</td></tr>
	{{else}}
	<tr><td style="vertical-align: top">
		<div class="row">
			<span class="label"><? echo dict_translate("Interface:");></span>
			<span class="value">${devname2uidevname(ifname)}</span>
		</div>
		<div class="row">
			<span class="label"><? echo dict_translate("IP Address:");></span>
			<span class="value">${ip}</span>
		</div>
		<div class="row">
			<span class="label"><? echo dict_translate("Netmask:");></span>
			<span class="value">${netmask}</span>
		</div>
		{{if gateway }}
		<div class="row">
			<span class="label"><? echo dict_translate("Gateway:");></span>
			<span class="value">${gateway}</span>
		</div>
		{{/if}}
		{{if dns[0] }}
		<div class="row">
			<span class="label"><? echo dict_translate("Primary DNS IP:");></span>
			<span class="value">${dns[0]}</span>
		</div>
		{{/if}}
		{{if dns[1] }}
		<div class="row">
			<span class="label"><? echo dict_translate("Secondary DNS IP:");></span>
			<span class="value">${dns[1]}</span>
		</div>
		{{/if}}
	</td><td style="vertical-align: top">
		<div class="row">
			<span class="label"><? echo dict_translate("DHCP Server:");></span>
			<span class="value">${serverid}</span>
		</div>
		{{if domain }}
		<div class="row">
			<span class="label"><? echo dict_translate("Domain:");></span>
			<span class="value">${domain}</span>
		</div>
		{{/if}}
		<div class="row">
			<span class="label"><? echo dict_translate("Total Lease Time:");></span>
			<span class="value">${leasetime_str}</span>
		</div>
		<div class="row">
			<span class="label"><? echo dict_translate("Remaining Lease Time:");></span>
			<span class="value">${leasetime_left}</span>
		</div>
<? if ($is_ro == 0) { >
		<div class="row" style="text-align: right">
			<input type="button" class="data ctrl dhcpcRenew" value=" <? echo dict_translate("Renew"); > ">
			<input type="button" class="data ctrl dhcpcRelease" value=" <? echo dict_translate("Release"); > ">
		</div>
<?}>
	</td></tr>
	{{/if}}
</script>
<br>
<form enctype="multipart/form-data" action="<? echo $PHP_SELF; ?>" method="POST">
<table cellspacing="0" cellpadding="0" class="listhead" id="dhcpcinfo">
	<thead>
		<tr><th colspan="2"><? echo dict_translate("DHCP Client Information"); ></th></tr>
	</thead>
	<tfoot>
		<tr><td colspan="2">
		<div class="row" style="text-align: right">
			<input type="button" id="ctrl_refresh" class="ctrl" onClick="return dhcpcRefresh();" value=" <? echo dict_translate("Refresh"); > ">
		</div>
		</tr></td>
	</tfoot>
	<tbody>
	</tbody>
</table>
</form>
  
</body>
</html>

