#!/bin/sh
[ $# -le 1 ] && echo "Usage: $0 <current_ver> <new_ver>" && exit 1
CUR_VER=$1
NEW_VER=$2
RET=0

if [ -e /tmp/customfix.sh ]; then
	echo "Found custom updatefix!"
	. /tmp/customfix.sh $*
fi

#v5.5 (5<<16|5<<8)=328960
if [ ${NEW_VER} -gt ${CUR_VER} -o ${NEW_VER} -ge 328960 ]; then
	echo "Current ver: ${CUR_VER}"
	echo "New version: ${NEW_VER}"
        echo "No need to fix."
        exit ${RET}
fi
#default ver < v5.5 fixer
RUNNING_CFG=/tmp/running.cfg
NEW_SYSTEM_CFG=/tmp/system.cfg.$$
SYSTEM_CFG=/tmp/system.cfg

INDEX_REGEXP=\.[[:digit:]]{1,}\.

get_index() {
	#$1 is prefix, $2 is postfix, $3 is value
        for match in `grep -E "$1$INDEX_REGEXP$2=$3" ${RUNNING_CFG}`
        do
                local i1=${match##$1.}
                local i2=${i1%.$2=$3*}
                if [ -n "$i2" ]; then
                        grep -q "$1\.$i2\.status=disabled" ${RUNNING_CFG} || \
                                { echo $i2; return 0; }
                fi
        done
        return 1
}

get_value() {
        #$1 is key
        local val=`grep "$1" ${RUNNING_CFG} | tr -d "\r" "\t" "\b" "\v"`
        echo `expr "$val" : '.*=\(.*\)'`
}

fix_netconf() {
        #$1 devname of netconf section
        #$2 new devname
        #$3 new index
        echo -n "Fixing netconf section for $2... "
        local index=$(get_index netconf devname $1)
        if [ -z "${index}" ]; then 
        	return 1
        fi;
        local ip=$(get_value "netconf\.${index}\.ip")
        if [ -z "$ip" ]; then
                return 1
        fi;
        grep "^netconf\.${index}\."  ${RUNNING_CFG} |\
                grep -v "role=\|up=" |\
                sed "s/netconf\.${index}\./netconf\.$3\./g;s/devname=$1/devname=$2/g;s/.mac=/=/g" >> ${NEW_SYSTEM_CFG}
        echo "Done"
        return 0
}

create_netconf() {
        #$1 is status [enabled|disabled]
	#$2 is devname
        #$3 is index
        cat >> ${NEW_SYSTEM_CFG} <<EOF
netconf.$3.status=$1
netconf.$3.devname=$2
netconf.$3.ip=0.0.0.0
netconf.$3.netmask=255.255.255.0
netconf.$3.up=enabled
EOF
}

is_ifc_down() {
        #$1 devname
        local index=$(get_index netconf devname $1)
        [ -z "$index" ] && return 1
        local ip=$(get_value "netconf\.${index}\.up")
        [ "$ip" == "disabled" ] && return 0
        return 1
}

fix_disable_network() {
        # Disabled lan
        is_ifc_down "eth0" && is_ifc_down "eth1" && \
                echo "netconf.1.up=disabled" >> ${NEW_SYSTEM_CFG} && \
                return 0
        # Disabled wlan
        is_ifc_down "ath0" && \
                echo "netconf.2.up=disabled" >> ${NEW_SYSTEM_CFG} && \
                return 0
        return 1
}

fix_disable_network_soho() {
        # Disabled wan
        is_ifc_down "eth0" && \
                echo "netconf.4.up=disabled" >> ${NEW_SYSTEM_CFG} && \
                return 0
        # Disabled lan
        is_ifc_down "eth0" && is_ifc_down "ath0" && \
                echo "netconf.1.up=disabled" >> ${NEW_SYSTEM_CFG} && \
                echo "netconf.2.up=disabled" >> ${NEW_SYSTEM_CFG} && \
                return 0
        # Disabled wlan
        is_ifc_down "ath0" && \
                echo "netconf.2.up=disabled" >> ${NEW_SYSTEM_CFG} && \
                return 0
        return 1
}

fix_ppp() {
        echo -n "Fixing ppp section... "
        local index=$(get_index ppp devname $1)
        grep "^ppp\.${index}"  ${RUNNING_CFG} |\
                sed "s/ppp\.${index}\./ppp\.1\./g;s/devname=$1/devname=$2/g" >> ${NEW_SYSTEM_CFG}
        echo "Done"
}

fix_bridge_vlan() {
        echo -n "Fixing vlan section... "
        local vlan_enabled=`grep -ci "^vlan\.status=enabled" ${RUNNING_CFG}`
        local vlan_count=`grep -ci "^vlan\..\.status=enabled" ${RUNNING_CFG}`
        [ $vlan_enabled -eq 0 -o $vlan_count -eq 0 -o $vlan_count -gt 2 ] && return 1
        local bridge_enabled=`grep -ci "^bridge\.status=enabled" ${RUNNING_CFG}`
        local bridge_count=`grep -ci "^bridge\..\.devname" ${RUNNING_CFG}`
        local br_port_count=`grep -ci "^bridge\.1\.port\..\.status=enabled" ${RUNNING_CFG}`
        [ $bridge_enabled -eq 0 -o $bridge_count -ne 1 -o $br_port_count -lt 2 ] && return 1
        local vlan1_dev=$(get_value "vlan\.1\.devname")
        local vlan1_id=$(get_value "vlan\.1\.id")
        [ -z "$vlan1_dev" -o -z "$vlan1_id" ] && return 1
        grep -q "^bridge\.1\.port\..\.devname=$vlan1_dev.$vlan1_id" ${RUNNING_CFG} || return 1
        if [ $vlan1_dev == "ath0" ]; then
                cat >> ${NEW_SYSTEM_CFG} <<EOF
bridge.1.port.2.status=disabled
bridge.1.port.4.status=enabled
bridge.1.port.4.devname=$vlan1_dev.$vlan1_id
EOF
        else
                local vlan2_dev=$(get_value "vlan\.2\.devname")
                local vlan2_id=$(get_value "vlan\.2\.id")
                if [ -z "$vlan2_dev" ]; then
                        vlan2_dev=eth1
                        vlan2_id=$vlan1_id
                cat >> ${NEW_SYSTEM_CFG} <<EOF
vlan.2.status=enabled
vlan.2.devname=$vlan2_dev
vlan.2.id=$vlan2_id
EOF
                fi;
                [ "$vlan1_dev" == "eth0" -a "$vlan2_dev" == "eth1" ] || return 1
                cat >> ${NEW_SYSTEM_CFG} <<EOF
bridge.1.port.1.status=disabled
bridge.1.port.3.status=disabled
bridge.1.port.4.status=enabled
bridge.1.port.4.devname=$vlan1_dev.$vlan1_id
bridge.1.port.5.status=enabled
bridge.1.port.5.devname=$vlan2_dev.$vlan2_id
EOF
        fi
        grep "^vlan\." ${RUNNING_CFG} >> ${NEW_SYSTEM_CFG}
        echo "Done"
}

fix_dmz() {
        local dmz_status=$(get_value "iptables\.sys\.dmz\.1\.status")
        local dmz_dev=$(get_value "iptables\.sys\.dmz\.1\.devname")
        local dmz_ip=$(get_value "iptables\.sys\.dmz\.1\.host")
        local dmz_except=$(get_value "iptables\.sys\.dmz\.1\.except.status")
        [ -z "$dmz_except" ] && dmz_except=disabled
        [ -z "$dmz_status" -o -z "$dmz_dev" -o -z "$dmz_ip" ] && return 1
        if [ $dmz_except == "disabled" ]; then
                cat >> ${NEW_SYSTEM_CFG} <<EOF
iptables.2.status=$dmz_status
iptables.2.cmd=-t nat -A PREROUTING -i $dmz_dev -j DNAT --to-destination $dmz_ip
iptables.200.status=disabled
EOF
        else
                cat >> ${NEW_SYSTEM_CFG} <<EOF
iptables.2.status=$dmz_status
iptables.2.cmd=-t nat -N DMZ_MGMT; iptables -t nat -A PREROUTING -i $dmz_dev -j DMZ_MGMT
EOF
                local excpref=iptables\.sys\.dmz\.1\.except
                local dstidx=200
                for match in `grep -E "$excpref${INDEX_REGEXP}status=enabled" ${RUNNING_CFG}`
                do
                        local i1=${match##$excpref.}
                        local idx=${i1%.status=enabled}
                        local port=$(get_value "$excpref\.$idx\.port")
                        local proto=$(get_value "$excpref\.$idx\.proto")
                        if [ -n "$port" -a -n "$proto" ]; then
                                echo "iptables.$dstidx.status=$dmz_status" >> ${NEW_SYSTEM_CFG}
                                if [ $proto == "ICMP" ]; then
                                        echo "iptables.$dstidx.cmd=-t nat -A DMZ_MGMT -p $proto --icmp-type $port -j RETURN" >> ${NEW_SYSTEM_CFG}
                                else
                                        echo "iptables.$dstidx.cmd=-t nat -A DMZ_MGMT -p $proto --dport $port -j RETURN" >> ${NEW_SYSTEM_CFG}
                                fi
                        fi
                        dstidx=`expr $dstidx + 1`
                done
                while [ $dstidx -lt 210 ]; do
                        echo "iptables.$dstidx.status=disabled" >> ${NEW_SYSTEM_CFG}
                        echo "iptables.$dstidx.cmd=" >> ${NEW_SYSTEM_CFG}
                        dstidx=`expr $dstidx + 1`
                done
                echo "iptables.210.status=$dmz_status" >> ${NEW_SYSTEM_CFG}
                echo "iptables.210.cmd=-t nat -A DMZ_MGMT -j DNAT --to-destination $dmz_ip" >> ${NEW_SYSTEM_CFG}
        fi;
}

fix_iptables() {
        #$1 is devname for NAT interface
	#$2 is for NAT status and has to be enabled|disabled
        #$3 is for wan iface
        echo -n "Fixing iptables section... "

        local pfw_status=$(get_value "iptables\.sys\.portfw\.status")
        local fw_status=$(get_value "iptables\.sys\.fw\.status")
        local ppp_status=$(get_value "ppp\.1\.status")
        local nat_ifc=$1
        [ ".$ppp_status" = ".enabled" ] && nat_ifc="ppp+"
        cat >> ${NEW_SYSTEM_CFG} <<EOF
iptables.1.status=$2
iptables.1.cmd=-t nat -I POSTROUTING -o $nat_ifc -j MASQUERADE
iptables.3.status=${ppp_status:=disabled}
iptables.3.cmd=-A FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu
iptables.4.status=${pfw_status:=disabled}
iptables.4.cmd=-t nat -N PORTFORWARD
iptables.5.status=$pfw_status
iptables.5.cmd=-t nat -I PREROUTING -i $nat_ifc -j PORTFORWARD
iptables.50.status=${fw_status:=disabled}
iptables.50.cmd=-N FIREWALL
iptables.51.status=$fw_status
iptables.51.cmd=-A INPUT -j FIREWALL
iptables.52.status=$fw_status
iptables.52.cmd=-A FORWARD -j FIREWALL
EOF

        tmpIFS=$IFS
        IFS=$'\012'
        local dstidx=6
        for devkey in `grep -E "iptables\.sys\.portfw${INDEX_REGEXP}devname" ${RUNNING_CFG} | tr -d "\r" "\t" "\b" "\v"`
        do
                local index=`expr "$devkey" : '.*portfw\.\([[:digit:]]*\)'\.`
                for key in `grep "iptables\.sys\.portfw\.$index\." ${RUNNING_CFG} | tr -d "\r" "\t" "\b" "\v"`
                do
                        local var=`expr "$key" : "iptables\.sys\.portfw\.$index\.\(.*\)="`
                        local val=`expr "$key" : ".*=\(.*\)"`
                        eval ${var}=`echo -ne \""${val}"\"`
                done 
                echo iptables.$dstidx.cmd=-t nat -A PORTFORWARD -p $proto -s $src \
                        -d $dst --dport $dport -j DNAT --to $host:$port >> ${NEW_SYSTEM_CFG}
                echo iptables.$dstidx.comment=$comment >> ${NEW_SYSTEM_CFG}
                echo iptables.$dstidx.status=$status >> ${NEW_SYSTEM_CFG}
                dstidx=`expr $dstidx + 1`
        done
        dstidx=53
        case "${netmode}" in
                router)
                        local fixifc=s/br0/eth+/g
                        ;;
                soho)
                        local fixifc=s/eth0/eth1/g\;s/br0/eth0/g
                        ;;
        esac
        for devkey in `grep -E "iptables${INDEX_REGEXP}cmd" ${RUNNING_CFG} | tr -d "\r" "\t" "\b" "\v"`
        do
                local index=`expr "$devkey" : 'iptables\.\([[:digit:]]*\)'\.`
                grep "^iptables\.${index}\." ${RUNNING_CFG} | sed "s/iptables\.${index}\./iptables\.$dstidx\./g;$fixifc" >> ${NEW_SYSTEM_CFG}
                dstidx=`expr $dstidx + 1`
        done
        IFS=$tmpIFS
        fix_dmz
        echo "Done"
}

fix_bridge() {
	#$1 is for BRIDGE status and has to be enabled|disabled
        echo -n "Fixing bridge section... "
        cat >> ${NEW_SYSTEM_CFG} <<EOF
bridge.status=$1
bridge.1.status=enabled
bridge.1.devname=br0
bridge.1.fd=1
bridge.1.port.1.devname=eth0
bridge.1.port.2.devname=ath0
bridge.1.port.3.devname=eth1
EOF
        echo "Done"
}

fix_dhcpd() {
	#$1 is for dhcpd devname
        echo -n "Fixing dhcpd section... "
        local index=$(get_index dhcpd devname $1)
        grep "^dhcpd\.${index}"  ${RUNNING_CFG} |\
                sed "s/dhcpd\.${index}\./dhcpd\.1\./g;s/devname=$1/devname=$2/g" >> ${NEW_SYSTEM_CFG}
        echo "Done"
}
        
fix_dhcpc() {
        echo -n "Fixing dhcpc section... "
        local index=$(get_index dhcpc devname $1)
        grep "^dhcpc\.${index}"  ${RUNNING_CFG} |\
                sed "s/dhcpc\.${index}\./dhcpc\.1\./g;s/devname=$1/devname=$2/g" >> ${NEW_SYSTEM_CFG}
        echo "Done"
}

fix_dhcprelay() {
        echo -n "Fixing dhcprelay section (not ready yet)... "
        echo "Done"
}

fix_igmpproxy() {
        echo -n "Fixing igmpproxy section... "
        grep "^igmpproxy\."  ${RUNNING_CFG} |\
                sed "s/devname=eth0/devname=eth1/g;s/devname=br0/devname=eth0/g" >> ${NEW_SYSTEM_CFG}
        echo "Done"
}

fix_tshaper() {
        echo -n "Fixing tshaper section... "
        local tshaper_status=$(get_value "tshaper\.status")
        local tshaper_outdev=$(get_value "tshaper\.1\.devname")
        local tshaper_indev=$(get_value "tshaper\.2\.devname")
        if [ -z "$tshaper_status" -o -z "$tshaper_indev" -o -z "$tshaper_outdev" ]; then
                echo "Done"
                return 1
        fi
        case "${netmode}" in
                router)
                        tshaper_indev="eth0"
                        tshaper_outdev="ath0"
                        ;;
                soho)
                        tshaper_indev="eth0"
                        tshaper_outdev="eth1"
                        ;;
        esac
        cat >> ${NEW_SYSTEM_CFG} <<EOF
tshaper.status=$tshaper_status
tshaper.in.1.devname=$tshaper_indev
tshaper.out.1.devname=$tshaper_outdev
EOF
        grep "^tshaper\.1\.\(input\|output\)"  ${RUNNING_CFG} |\
                sed "s/1\.input/in/g;s/1\.output/out/g" >> ${NEW_SYSTEM_CFG}
        echo "Done"
}

fix_dnsmasq() {
        echo -n "Fixing dnsmasq section ... "
        local index=$(get_index dnsmasq devname $1)
        grep "^dnsmasq\.${index}"  ${RUNNING_CFG} | sed "s/\.${index}\./\.1\./g" >> ${NEW_SYSTEM_CFG}
        echo "Done"
}

fix_ebtables() {
	#$1 is for STA_BRIDGE and has to be enabled|disabled
	#$2 is for WPA and has to be enabled|disabled
        echo -n "Fixing ebtables section... "
        local fw_status=$(get_value "ebtables\.sys\.fw\.status")
        cat >> ${NEW_SYSTEM_CFG} <<EOF
ebtables.1.status=$1
ebtables.1.cmd=-t nat -A PREROUTING --in-interface ath0 -j arpnat --arpnat-target ACCEPT
ebtables.2.status=$1
ebtables.2.cmd=-t nat -A POSTROUTING --out-interface ath0 -j arpnat --arpnat-target ACCEPT
ebtables.3.status=$2
ebtables.3.cmd=-t broute -A BROUTING --protocol 0x888e --in-interface ath0 -j DROP
ebtables.50.status=$fw_status
ebtables.50.cmd=-N FIREWALL
ebtables.51.status=$fw_status
ebtables.51.cmd=-A INPUT -j FIREWALL
ebtables.52.status=$fw_status
ebtables.52.cmd=-A FORWARD -j FIREWALL
EOF
        tmpIFS=$IFS
        IFS=$'\012'
        local dstidx=53
        for devkey in `grep -E "ebtables${INDEX_REGEXP}cmd" ${RUNNING_CFG} | grep -v "\-i eth1" | tr -d "\r" "\t" "\b" "\v"`
        do
                local index=`expr "$devkey" : 'ebtables\.\([[:digit:]]*\)'\.`
                grep "^ebtables\.${index}\." ${RUNNING_CFG} | sed "s/ebtables\.${index}\./ebtables\.$dstidx\./g;s/eth0/eth+/g" >> ${NEW_SYSTEM_CFG}
                dstidx=`expr $dstidx + 1`
        done
        IFS=$tmpIFS
        echo "Done"
}

fix_route() {
	#$1 is devname for default gw
        echo -n "Fixing route section... "
        local route_status=$(get_value "route\.1\.status")
        [ -z $route_status ] && route_status=enabled
        cat >> ${NEW_SYSTEM_CFG} <<EOF
route.1.status=$route_status
route.1.devname=$1
route.1.ip=0.0.0.0
route.1.netmask=0
EOF
	grep "^route.1.gateway=" ${RUNNING_CFG} >> ${NEW_SYSTEM_CFG}
        echo "Done"
}

fix_bridge_mode() {
        fix_netconf eth0 eth0 1
        fix_netconf ath0 ath0 2
        fix_netconf br0 br0 3 || fix_netconf br1 br0 3
        fix_netconf eth1 eth1 4
        fix_disable_network
        fix_bridge enabled
        fix_bridge_vlan
        if [ $is_ap -eq 1 -o $is_wds -eq 1 ]; then
        	fix_ebtables disabled enabled
        else
        	fix_ebtables enabled enabled
        fi
        fix_route br0
	fix_dhcpc br0 br0
        fix_igmpproxy
        fix_tshaper
}

fix_router_mode() {
        fix_netconf br0 eth0 1 || fix_netconf eth0 eth0 1 || fix_netconf eth1 eth0 1 || create_netconf enabled eth0 1
        fix_netconf ath0 ath0 2
        create_netconf disabled br0 3
        create_netconf enabled eth1 4
        fix_disable_network
        fix_bridge disabled
	curr_wan_ifc=br0
        curr_lan_ifc=ath0
	wan_ifc=eth0
        lan_ifc=ath0
        nat=enabled
        if [ $is_ap -eq 0 ]; then
                curr_wan_ifc=ath0
                curr_lan_ifc=br0
        	wan_ifc=ath0
                lan_ifc=eth0
        fi
        fix_route $wan_ifc
        if [ $is_nat -eq 0 ]; then
        	nat=disabled
        fi
        fix_iptables $wan_ifc $nat
	fix_dhcpd $curr_lan_ifc $lan_ifc
	fix_dhcpc $curr_wan_ifc $wan_ifc
        fix_ppp $curr_wan_ifc $wan_ifc
        fix_igmpproxy
        fix_tshaper
}

fix_soho_mode() {
        fix_netconf br0 eth0 1
        fix_netconf ath0 ath0 2
        create_netconf disabled br0 3
        fix_netconf eth0 eth1 4
        fix_disable_network_soho
        fix_bridge disabled
        fix_route eth1
        nat=enabled
        if [ $is_nat -eq 0 ]; then
        	nat=disabled
        fi
        fix_iptables eth1 $nat
	fix_dhcpd br0 eth0
	fix_dhcpc eth0 eth1
        fix_ppp eth0 eth1
        fix_igmpproxy
        fix_tshaper
}

fix_generic() {
	echo -n "Fixing generic stuff... "
	sed -i -e "s/^aaa\.\(.\)\.wpa.mode=/aaa\.\1\.wpa=/g;\
        s/^system.date.timestamp=/system.date=/g;\
        s/^radio\.\(.\)\.antenna.id=/radio\.\1\.antenna=/g;\
        s/^wireless\.\(.\)\.wds.status=/wireless\.\1\.wds=/g;\
        s/^wireless\.\(.\)\.security.type=/wireless\.\1\.security=/g" ${NEW_SYSTEM_CFG}
        echo "Done"
}

#strip out new stuff
KEYS2STRIP="\
^system.cfg.version=|\
^netconf$INDEX_REGEXP|\
^bridge\.|\
^vlan$INDEX_REGEXP|\
^ppp$INDEX_REGEXP|\
^route\.1\.|\
^ebtables$INDEX_REGEXP|\
^ebtables\.sys|\
^iptables$INDEX_REGEXP|\
^iptables\.sys|\
^dhcpd$INDEX_REGEXP|\
^dnsmasq$INDEX_REGEXP|\
^dhcpc$INDEX_REGEXP|\
^ppp$INDEX_REGEXP|\
^dhcprelay$INDEX_REGEXP|\
^igmpproxy\.|\
^tshaper\.|\
^aaa\.1\.wpa=|\
^system.date=|\
^wireless\.1\.wds=|\
^wireless\.1\.security=|\
^radio\.1\.chanbw=|\
^tshaper${INDEX_REGEXP}output.cburst=|\
^tshaper${INDEX_REGEXP}input.cburst=|\
^wpasupplicant.profile.1.network.1.anonymous_identity=[[:space:]]*\$|\
^system.modules.blacklist.status=\
"
grep -vE "${KEYS2STRIP}" ${RUNNING_CFG} > ${NEW_SYSTEM_CFG}

is_ap=`grep -ci "^radio.1.mode=master" ${RUNNING_CFG}`
is_wds=`grep -ci "^wireless.1.wds.status=enabled" ${RUNNING_CFG}`
is_nat=`grep -ci "^iptables.sys.masq.1.status=enabled" ${RUNNING_CFG}`

fix_generic

eval `grep "^netmode=" ${RUNNING_CFG} | tr -d "\r" "\t" "\b" "\v"`
echo "Going to fix ${netmode} mode:"
case "${netmode}" in
	bridge)
		fix_bridge_mode
		;;
        router)
        	fix_router_mode
                ;;
        soho)
        	fix_soho_mode
                ;;
        *)
        	fix_bridge_mode
                ;;
esac

tr -d "\r" "\t" "\b" "\v" < ${NEW_SYSTEM_CFG} | sort | uniq > ${SYSTEM_CFG}
cfgmtd -w -p /etc/
rm -f ${NEW_SYSTEM_CFG}
echo "Done"
exit ${RET}
