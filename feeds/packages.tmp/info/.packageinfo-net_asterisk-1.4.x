Source-Makefile: feeds/packages/net/asterisk-1.4.x/Makefile
Package: asterisk14
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +libncurses +libpopt +libpthread @!TARGET_avr32
Provides: 
Build-Depends: libopenh323 pwlib
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
Package: asterisk14-mini
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +libncurses +libpthread @!TARGET_avr32
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: Minimal open source PBX
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package contains only the following modules:
 - app_dial
 - chan_iax2
 - chan_local
 - chan_sip
 - codec_gsm
 - codec_ulaw
 - format_gsm
 - format_pcm
 - format_wav
 - format_wav_gsm
 - pbx_config
 - res_features
 - res_musiconhold

http://www.asterisk.org/
@@
Package: asterisk14-app-meetme
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14 +zaptel14-libtonezone
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: conferencing support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package provides the MeetMe application driver Conferencing support to 
 Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-chan-oss
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: OSS soundcards support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package provides the channel driver for OSS sound cards support to 
 Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-chan-alsa
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14 +alsa-lib
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: ALSA soundcards support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package provides the channel driver for ALSA sound cards support to 
 Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-chan-gtalk
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14 +libiksemel
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: GTalk support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package provides the channel chan_gtalk and res_jabber for GTalk 
 support to Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-chan-h323
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14 +uclibcxx
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: H.323 support for Asterisk
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package provides H.323 support to Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-chan-mgcp
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: MGCP support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package provides MGCP (Media Gateway Control Protocol) support \ to Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-chan-skinny
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: Skinny Client Control Protocol support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package provided Skinny Client Control Protocol support to \ Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-codec-lpc10
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: LPC10 2.4kbps voice codec Translator
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description: 	LPC10 2.4kbps voice codec Translator

http://www.asterisk.org/
@@
Package: asterisk14-codec-speex
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14 +libspeex +libspeexdsp
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: Speex/PCM16 Codec Translator
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description: 	Speex/PCM16 Codec Translator

http://www.asterisk.org/
@@
Package: asterisk14-pbx-dundi
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: DUNDi support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package provides DUNDi (Distributed Universal Number Discovery) 
 support to Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-res-agi
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: AGI support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package provides AGI (Asterisk Gateway Interface) support to 
 Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-res-crypto
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14 +libopenssl
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: Cryptographic Digital Signatures support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package provides Cryptographic Digital Signatures support to 
 Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-pgsql
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14 +libpq
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: PostgreSQL support
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package contains PostgreSQL support modules for Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-sqlite
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14 +libsqlite2
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: SQLite modules
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
 This package contains SQLite support modules for Asterisk.

http://www.asterisk.org/
@@
Package: asterisk14-voicemail
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14
Provides: 
Build-Depends: libopenh323 pwlib
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
Package: asterisk14-sounds
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: +asterisk14
Provides: 
Build-Depends: libopenh323 pwlib
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
Package: asterisk14-rawplayer
Submenu: asterisk14 (Complete Open Source PBX), v1.4.x
Version: 1.4.23.1-1.2
Depends: 
Provides: 
Build-Depends: libopenh323 pwlib
Section: net
Category: Network
Title: Play raw files for asterisk
Maintainer: OpenWrt Developers Team <openwrt-devel@openwrt.org>
Type: ipkg
Description:  Contains the rawplayer utility for asterisk

http://www.asterisk.org/
@@

