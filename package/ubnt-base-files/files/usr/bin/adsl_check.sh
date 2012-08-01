#!/bin/sh

# Released under the GPLv3 Licence
#
# @AUTHORS: Clauz (clauz@ninux.org), Hispanico (marco.giuntini@gmail.com), Stefano (stefano@ninux.org)
# 

IP_PING=160.80.80.1 #Address to check for ADSL Connection
ADSL_GW='192.168.1.1' #Address of the ADSL Gateway
RT_TABLE=detection_table #Name of the new routing table for ADSL gw malfunction detection
RT_TABLE_NUM=201 #Number of the routing table for ADSL gw malfunction detection

grep $RT_TABLE /etc/iproute2/rt_tables || (echo $RT_TABLE_NUM $RT_TABLE >> /etc/iproute2/rt_tables) 
ip route add table $RT_TABLE default via $ADSL_GW 2>/dev/null

(ip rule show | grep -F ${IP_PING}) || ip rule add to $IP_PING table $RT_TABLE 

while [ 1 ]; do
		if ping -q -c 1 $IP_PING >/dev/null 2>/dev/null; then
				(ip r s |grep -F $ADSL_GW >/dev/null) || ip route add default via $ADSL_GW
		else
				(ip r s |grep -F $ADSL_GW >/dev/null) && ip route del default via $ADSL_GW
		fi
		sleep 50
done

