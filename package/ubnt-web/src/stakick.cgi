#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");

Function writeStatus $status, $message (
        header("Content-Type: application/json");
        echo "{ \"status\" : " + $status + ", \"message\" : \"" + $message + "\" }";
);

if (strlen($staid) == 0) {
	writeStatus(-1, "Station ID is not specified.");
	exit;
}

if (strlen($staif) == 0) {
	writeStatus(-1, "Station parent interface is not specified.");
	exit;
}

$cmd = EscapeShellCmd("$cmd_iwpriv $staif kickmac $staid");

Exec($cmd, $arr, $result);
if ($result == 0) {
	writeStatus(0, "Request fulfilled.");
} else {
	writeStatus($result, "Request failed.");
}
?>
