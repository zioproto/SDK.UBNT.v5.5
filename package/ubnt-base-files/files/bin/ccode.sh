#!/bin/sh

usage() {
	echo "Usage: $0 <countrycode>"
}

set_ccode() {
	CCODE_CURRENT=$(iwpriv wifi0 getCountryID |  sed 's/wifi0     getCountryID://')
	if [ $CCODE_CURRENT -eq $1 ]; then
		return 0
	fi

	RADIO_CONF=/etc/sysinit/radio.conf
	RADIO_CONF_NEW=/etc/sysinit/radio.conf.new

	/usr/etc/init.d/plugin stop wireless
	/usr/etc/init.d/plugin stop radio
	/usr/bin/garp

	/usr/bin/sed "s/setCountryID [0-9]*/setCountryID $1/g" $RADIO_CONF > $RADIO_CONF_NEW
	/usr/bin/mv $RADIO_CONF_NEW $RADIO_CONF

	/usr/etc/init.d/plugin start radio
	/usr/etc/init.d/plugin start wireless
	/usr/bin/ifconfig ath0 0.0.0.0 up
	/usr/bin/brctl addif br0 ath0
	/usr/bin/brctl setportprio br0 ath0 30
	/usr/bin/garp
}

save_cfg() {
	CFG_SYSTEM="/tmp/system.cfg"
	CFG_SYSTEM_SORTED="/tmp/.system.cfg.$$"
	CFG_RUNNING="/tmp/running.cfg"

	sort $CFG_SYSTEM > $CFG_SYSTEM_SORTED
	cp $CFG_SYSTEM_SORTED $CFG_RUNNING
	cp $CFG_SYSTEM_SORTED $CFG_SYSTEM
	bgnd -r cfgmtd -e $CFG_SYSTEM_SORTED \
		-- /sbin/cfgmtd -w -f $CFG_SYSTEM_SORTED \
		-p /etc/ 2>/dev/null &
}

if [ $# -lt 1 ]; then
	usage
	exit 254
fi

set_ccode $*
save_cfg

exit 0
