#!/bin/sh

pgset() {
    local result

    echo $1 > $PGDEV

    result=`cat $PGDEV | fgrep "Result: OK:"`
    if [ "$result" = "" ]; then
         cat $PGDEV | fgrep Result:
    fi
}

pg() {
    echo inject > $PGDEV
    cat $PGDEV
}

usage=\
"
Usage: $0 -i <if_name> -d <dst_ip> -m <dst_mac> -s <pkt_size> -c <pkt_count|0>
"

# Config Start Here -----------------------------------------------------------
DEVNAME=eth0
DST_IP=192.168.1.206
DST_MAC=00:1c:25:7e:34:26
SIZE=60
PKT_COUNT=10000000

if [ $# -eq 0 ]; then
	echo -e $usage
	exit 1
fi

while getopts "i:d:m:s:c:" options; do
    case $options in
	i ) DEVNAME=$OPTARG;;
	d ) DST_IP=$OPTARG;;
	m ) DST_MAC=$OPTARG;;
        s ) SIZE=$OPTARG;;
        c ) PKT_COUNT=$OPTARG;;
	* ) 	echo -e $usage
		exit 1;;
    esac
done


# thread config

PGDEV=/proc/net/pktgen/kpktgend_0

  echo "Removing all devices"
 pgset "rem_device_all" 
  echo "Adding ${DEVNAME}"
 pgset "add_device ${DEVNAME}" 
  echo "Setting max_before_softirq 10000"
 pgset "max_before_softirq 10000"


# device config
# delay 0 means maximum speed.

CLONE_SKB="clone_skb 1000000"
# NIC adds 4 bytes CRC
PKT_SIZE="pkt_size ${SIZE}"

# COUNT 0 means forever
#COUNT="count 0"
COUNT="count ${PKT_COUNT}"
DELAY="delay 0"

PGDEV=/proc/net/pktgen/${DEVNAME}
  echo "Configuring $PGDEV"
 pgset "$COUNT"
 pgset "$CLONE_SKB"
 pgset "$PKT_SIZE"
 pgset "$DELAY"
 pgset "dst ${DST_IP}" 
 pgset "dst_mac  ${DST_MAC}"


# Time to run
PGDEV=/proc/net/pktgen/pgctrl

 echo "Running... ctrl^C to stop"
 pgset "start" 
 echo "Done"

# Result can be vieved in /proc/net/pktgen/xxx
