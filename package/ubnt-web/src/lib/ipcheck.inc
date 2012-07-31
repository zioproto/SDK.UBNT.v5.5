<?
$ipc_ipaddress = 0;
$ipc_lanipaddress = 0;
$ipc_http_proto = "";
$ipc_http_port = "";
if ($cfg_bak != -1) {
	$cfg_bak = @cfg_load($cfg_file_bak);
	$ipc_wlanmode = cfg_get_wmode($cfg, $wlan_iface);
	$cfgnetmode = cfg_get_def($cfg, "netmode", "bridge");
	if ($cfgnetmode == "bridge") {
		$if_idx = get_role_index($cfg, "mlan");
		$devname = cfg_get_def($cfg, "netconf.$if_idx.devname", $br_iface);
		if ($if_idx > 0) {
			$ipc_ipaddress = cfg_get_ipv4addr($cfg, $devname, $defip);
		}
                if ($cfg_bak == -1) {
                	$running_ip = if_get_ipaddr($devname);
                } else {
                	$running_ip = cfg_get_ipv4addr($cfg_bak, $devname, $defip);
                }
                if ($ipc_ipaddress == $running_ip) {
       	        	$ipc_ipaddress = 0;
                }
                $ipc_lanipaddress = $ipc_ipaddress;
	} else {
              	if ($cfgnetmode == "router") {
	        	if ($ipc_wlanmode == "ap" || $ipc_wlanmode == "aprepeater") {
				$ipc_wan_iface = $eth0_iface;
				$ipc_lan_iface = $wlan_iface;
		        } else {
                		$ipc_wan_iface = $wlan_iface;
				$ipc_lan_iface = $eth0_iface;
                        }
	        } else {
               		$ipc_wan_iface = $eth1_iface;
       			$ipc_lan_iface = $eth0_iface;
                }
		$ipc_vlan_status = cfg_get_vlan_status($cfg, $ipc_wan_iface, "disabled");
       		$ipc_vlan_tag = cfg_get_vlan_tag($cfg, $ipc_wan_iface, 0);
       		if ($ipc_vlan_status=="enabled" && $ipc_vlan_tag != 0)
       		{
			$ipc_vlan_ifc = cfg_get_vlan_ifc($cfg, $ipc_wan_iface, "lan");
       			if ($ipc_vlan_ifc == "lan") {
       				$ipc_vlan_iface = $ipc_lan_iface;
       				$ipc_lan_iface = $ipc_vlan_iface + "." + $ipc_vlan_tag;
       			} else {
       				$ipc_vlan_iface = $ipc_wan_iface;
       				$ipc_wan_iface = $ipc_vlan_iface + "." + $ipc_vlan_tag;
       			}
       		}
		$ipc_ipaddress = cfg_get_ipv4addr($cfg, $ipc_wan_iface, $defip);
                $ipc_lanipaddress = cfg_get_ipv4addr($cfg, $ipc_lan_iface, $defip);
                if ($cfg_bak == -1) {
	                $running_ip = if_get_ipaddr($ipc_wan_iface);
        	        $running_lanip = if_get_ipaddr($ipc_lan_iface);
                } else {
			$running_ip = cfg_get_ipv4addr($cfg_bak, $ipc_wan_iface, $defip);
			$running_lanip = cfg_get_ipv4addr($cfg_bak, $ipc_lan_iface, $defip);
                
                }
              	if ($ipc_ipaddress == $running_ip) {
               		$ipc_ipaddress = 0;
                }
       	        if ($ipc_lanipaddress == $running_lanip) {
       			$ipc_lanipaddress = 0;
                }
        }
       	$ipc_https_state = cfg_get_def($cfg, "httpd.https.status", "disabled");
        if ($ipc_https_state == "enabled") {
        	$ipc_http_proto = "https";
	        $ipc_http_port = cfg_get_def($cfg, "httpd.https.port", "443");
        } else {
        	$ipc_http_proto = "http";
	        $ipc_http_port = cfg_get_def($cfg, "httpd.port", "80");
        }
        if ($cfg_bak != -1) {
       		$running_https_state = cfg_get_def($cfg_bak, "httpd.https.status", "disabled");
	        if ($running_https_state == "enabled") {
        		$running_http_proto = "https";
	        	$running_http_port = cfg_get_def($cfg_bak, "httpd.https.port", "443");
	        } else {
        		$running_http_proto = "http";
	        	$running_http_port = cfg_get_def($cfg_bak, "httpd.port", "80");
	        }
        }
        if ($ipc_https_state == $running_https_state &&
            $ipc_http_proto == $running_http_proto &&
            $ipc_http_port == $running_http_port) {
            $ipc_http_proto = "";
            $ipc_http_port = "";
        }
}
>
