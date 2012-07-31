#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
include("lib/link.inc");
include("lib/misc.inc");
>

<html>
<head>
<link href="FULL_VERSION_LINK/style.css" rel="stylesheet" type="text/css">
<script type="text/javascript" language="javascript">

$(document).ready(function() {
	<? if ($radio1_ccode_locked != 1) {>
	       	$("#country_select").val(0);
        <?}>
        $('#errmsg').hide();
});
</script>
</head>
<body>
<table border="0" cellpadding="0" cellspacing="0" align="center">
	<tr>
		<td colspan="2" align="center">
		    <div id="errmsg" class="error" style="display:none;">
		       &nbsp;
		    </div>
		</td>
	</tr>
	<tr>
		<td><label for="country_select"><? echo dict_translate("Country:"); ></label></td>
		<td>
		     <select name="country_select" id="country_select" <? if ($radio1_ccode_locked == 1) { echo "disabled";} >>
								<? if ($radio1_ccode_fixed == 0 && $radio1_ccode_locked != 1) { >
									<option value="0"><? echo dict_translate("Select new Country");></option>
								<? } 
								   include("lib/ccode.inc");
                                                                >
		     </select>
		</td>
	</tr>
	<tr>
	        <td colspan="2">&nbsp;</td>
	</tr>
	<tr>
		<td colspan="2"><strong><? echo dict_translate("TERMS OF USE"); ></strong></td>
	</tr>
	<tr>
		<td colspan="2" class="license">
			<p>
			<? echo dict_translate("license_agreement|This Ubiquiti Networks, Inc. radio device must be professionally installed. Properly installed shielded Ethernet cable and earth grounding must be used as conditions of product warranty. It is the installer’s responsibility to follow local country regulations including operation within legal frequency channels, output power, and Dynamic Frequency Selection (DFS) requirements. The End User is responsible for keeping the unit working according to these rules."); >
			<? echo dict_translate("license_agreement2| For further information, please visit <a href=\"http://www.ubnt.com/\">www.ubnt.com</a>."); >
			</p>
		</td>
	</tr>
	<tr>
		<td colspan="2">
			<input type="checkbox" id="agreed"></input>
			<label for="agreed"><strong><? echo dict_translate("I agree to these terms of use"); ></strong></label>
		</td>
	</tr>
</table>
</body>
</html>

