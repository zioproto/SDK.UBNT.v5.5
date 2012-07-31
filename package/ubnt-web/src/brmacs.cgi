#!/sbin/cgi
<?
include("lib/settings.inc");
$cfg = @cfg_load($cfg_file);
include("lib/l10n.inc");
include("lib/misc.inc");
$brmacs_regexp="[[:space:]]*([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]*$";
include("lib/brmacs_head.tmpl");
flush();

$brifcs = get_bridges_list();
$bsize = count($brifcs);
while($bsize > 0) {
	$f = $brifcs[$bsize - 1];

        echo "<tr><th class=\"str\" colspan=\"3\">" + devname2ifname($f) + "</th></tr>";
        
	Exec($cmd_brctl + " showmacs " + $f, $arr, $result);
	if ($result == 0) {
	      $i = 1;
	      $size = count($arr);
	      while ($i < $size) {
		    if (ereg($brmacs_regexp, $arr[$i], $res)) {
			   if ($res[3] != "yes"){
				  $port = br_portno2ifc($f, $res[1]);
				  echo "<tr><td class=\"str\">" + strtoupper($res[2]) + "</td>";
                                  echo "<td class=\"str\">" + devname2ifname($port) + "</td>";
                                  echo "<td class=\"num\">" + $res[4] + "</td></tr>\n";
			   }
		     }

		     $i++;
	      }
      	}
        $bsize--;
}
include("lib/arp_tail.tmpl");
>
