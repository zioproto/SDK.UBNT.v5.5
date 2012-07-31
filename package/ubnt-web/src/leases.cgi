#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");

$page_title=dict_translate("DHCP Leases");

include("lib/ptable_head.tmpl");
>

<tr>
	<th><? echo dict_translate("MAC Address"); > </th>
	<th><? echo dict_translate("IP Address"); > </th>
	<th><? echo dict_translate("Remaining Lease"); > </th>
	<th><? echo dict_translate("Hostname"); > </th>
</tr>

<?

$leasefile = "/tmp/dhcpd.leases";
$lease_regexp="^([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+).*$";

$fp = @fopen($leasefile, "r");
if ($fp != -1) {
    while (!feof($fp)) {
        $line=fgets($fp, 255);
        if (ereg($lease_regexp, $line, $res)) {
            $x=intval($res[1]);
            $hostname=ereg_replace("\*", " ", $res[4]);
            $now=time();
            if ($now > $x) {
                $left="expired";
            } else {
                $t = $x - $now;
                $h = $t / 3600;
                $m = $t / 60;
                $left = sprintf("%02u:%02u:%02u", $h,$m%60,$t%60);
            }
            echo "<tr><td class=\"str\">" + strtoupper($res[2]) + "</td><td class=\"str\">" + $res[3] + "</td>";
            echo "<td class=\"str\">" + $left + "</td>";
            echo "<td class=\"str\">" + $hostname + "&nbsp;</td></tr>";
        }
    }
}

include("lib/arp_tail.tmpl");
>
