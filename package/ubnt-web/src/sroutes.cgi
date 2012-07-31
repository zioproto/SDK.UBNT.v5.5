#!/sbin/cgi
<?
include("lib/settings.inc");
include("lib/misc.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
$route_regexp="([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)$";
$page_title=dict_translate("Routing Table");
include("lib/ptable_head.tmpl");
>
<tr>
<th><? echo dict_translate("Destination"); ></th><th><? echo dict_translate("Gateway"); ></th><th><? echo dict_translate("Netmask"); ></th><th><? echo dict_translate("Interface"); ></th>
</tr>
<?
flush();

Exec("/sbin/route -n", $lines, $result);

if ($result == 0) {
	$i = 2;
	$size = count($lines);
	while ($i < $size) {
		if (ereg($route_regexp,$lines[$i],$res)) {
			$f = devname2ifname($res[8]);
			echo "<tr><td class=\"str\">" + $res[1] + "</td><td class=\"str\">" + $res[2] + "</td>";
			echo "<td class=\"str\">" + $res[3] + "</td>";
			echo "<td>" + $f + "</td></tr>\n";
		}
		$i++;
	}
}

include("lib/arp_tail.tmpl");
>
