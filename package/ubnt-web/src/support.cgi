#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/system.inc");
$hwaddr = if_get_hwaddr($wlan_iface);
if (strlen($hwaddr) == 0) {
        $hwaddr = if_get_hwaddr($eth0_iface);
}
$hwaddrstr = ereg_replace(":", "", $hwaddr);
$version = fw_get_version();
$p = strtok($version, ".");
$filename = $p + "-" + $hwaddrstr + ".sup";
$file = "/tmp/" + $filename;
$dirname = "support-" + $p + "-" + $hwaddrstr;
$ddir =  "/tmp/" + $dirname;

exec("support " + $ddir + " " + $file);

if (fileinode($file) != -1) {
header("Content-Type: application/force-download");
header("Content-Disposition: attachment; filename=" + $filename);
passthru("cat " + $file);
unlink($file);
exit;
}
>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head><title>Main</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<link rel="shortcut icon" href="FULL_VERSION_LINK/favicon.ico" >
<style type="text/css">
body, td, th, table {
    font-family: Verdana, Arial, Helvetica, sans-serif;
    font-size: 12px;
}
th {
color: #990000;
font-size: 14px;
}
</style>
</head>
<body bgcolor="white">
<th>OOPS - FAILED generating support information file!</th>
</body>
</html>
