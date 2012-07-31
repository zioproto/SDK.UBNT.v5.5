#!/sbin/cgi
<?
include("lib/settings.inc");
Function getReturnCode $cmd_code
(
	$res = (($cmd_code & 65280) / 256);
	if ($res > 127) {
		$res -= 256;
	}
	return $res;
);

if ($ping_size < 28 || $ping_size > 65507)
{
	$ping_size = 56;
}

$command = $cmd_webping + " -s " + EscapeShellCmd($ping_size) + " " + EscapeShellCmd($ip_addr);

exec($command, $lines, $res);
$res = getReturnCode($res);
$i = 0;
echo $res;
while ($i < count($lines))
{
	echo "|"+$lines[$i];    	
	$i++;
}
>
