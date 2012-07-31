Source-Makefile: feeds/packages/net/asterisk-1.6.x/Makefile
Package: asterisk16
Submenu: asterisk16 (Complete Open Source PBX), v1.6.x
Version: 1.6.1-rc1-1.1
Depends: +libncurses +libpopt +libpthread +zlib @!TARGET_avr32 @!PACKAGE_asterisk14
Provides: 
Build-Depends: 
Section: net
Category: Network
Title: Complete open source PBX
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.

http://www.asterisk.org/
@@
Package: asterisk16-voicemail
Submenu: asterisk16 (Complete Open Source PBX), v1.6.x
Version: 1.6.1-rc1-1.1
Depends: +asterisk16
Provides: 
Build-Depends: 
Section: net
Category: Network
Title: Voicemail support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package contains voicemail related modules for Asterisk.

http://www.asterisk.org/
@@
Package: asterisk16-sounds
Submenu: asterisk16 (Complete Open Source PBX), v1.6.x
Version: 1.6.1-rc1-1.1
Depends: +asterisk16
Provides: 
Build-Depends: 
Section: net
Category: Network
Title: Sound files
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package contains sound files for Asterisk.

http://www.asterisk.org/
@@

