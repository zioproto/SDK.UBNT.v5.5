#!/sbin/cgi
<?
header("Content-Type: application/json");
$done = "false";
if (fileinode("/tmp/test-done") != -1) {
	$done = "true";
}
echo "{\"done\": "+$done+"}";
>