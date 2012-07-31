#!/sbin/cgi
<?
include("lib/settings.inc");
  
$threeg_inc = "/etc/3g.inc";
if (fileSize($threeg_inc) > 0) {
	include($threeg_inc);
}

Function writeStatus $status, $message (
        header("Content-Type: application/json");
        echo "{ \"status\" : " + $status + ", \"message\" : \"" + $message + "\" }";
);

if ($threeg_simstatus == 2) {
	$l = strlen($gsmpin);
	if ($l < 4 || $l > 8) {
		writeStatus(1, "Invalid PIN");
		exit;
	}

	$cmd = "PINCODE=" + $gsmpin + " gcom -d " + $threeg_device + " -s /usr/etc/gcom/setpin.gcom && touch " + $simpinok_file;

	exec($cmd, $lines, $res);

	if ($res == 0) {
		writeStatus(0, "PIN OK");
	}
	else {
		writeStatus(2, dict_translate("PIN code incorrect. Three wrong entry of PIN code will result in SIM blocking."));
	}
}
else {
	writeStatus(0, "PIN not required");
}
?>
