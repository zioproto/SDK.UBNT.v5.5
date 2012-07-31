# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 4269 2006-07-24 18:58:47Z nico $

include $(TOPDIR)/rules.mk

PKG_NAME:=tinysnmp
PKG_VERSION:=0.8.4
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://archive.ubuntu.com/ubuntu/pool/universe/t/tinysnmp
PKG_MD5SUM:=892ddb02b74705e299ff0c69315b8943
PKG_CAT:=zcat

PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install
PKG_BUILD_DEPENDS:=libabz libber libdebug PACKAGE_tinysnmp-agent:libevent
$(eval $(call atheros_build_dep))

include $(INCLUDE_DIR)/package.mk

define Package/tinysnmp-tools
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+libabz +libber +libdebug
  TITLE:=TinySNMP Utilities
endef

define Package/tinysnmp-agent
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+libabz +libber +libdebug +libevent
  TITLE:=A lightweight SNMPv1 implementation
endef

define Package/tinysnmp-module-interfaces
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+tinysnmp-agent
  TITLE:=Interfaces MIB module for TinySNMP
endef

define Package/tinysnmp-module-resources
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+tinysnmp-agent
  TITLE:=Frogfoot Networks Resources MIB plugin for TinySNMP
endef

define Package/tinysnmp-module-ieee802dot11
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+tinysnmp-agent
  TITLE:=IEEE-802.11 MIB module for TinySNMP
endef

define Package/tinysnmp-module-mikrotik
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+tinysnmp-agent
  TITLE:=Mikrotik MIB module for TinySNMP
endef

#define Package/tinysnmp-agent/conffiles
#/etc/tinysnmp.conf
#endef

define Build/Configure
	$(CP) ./files/* $(PKG_BUILD_DIR)
endef

define Build/Compile
	CROSS="$(TARGET_CROSS)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	CPPFLAGS="-I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/usr/include/madwifi" \
	LDFLAGS="-L$(STAGING_DIR)/usr/lib" \
	$(MAKE) -C $(PKG_BUILD_DIR) SHELL=/bin/bash \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		prefix="$(PKG_INSTALL_DIR)/usr" \
		all install
endef

define Package/tinysnmp-tools/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/tinysnmpget $(1)/usr/bin
	ln -s tinysnmpget $(1)/usr/bin/tinysnmpgetnext
	ln -s tinysnmpget $(1)/usr/bin/tinysnmpwalk
endef

define Package/tinysnmp-agent/install
#	$(INSTALL_DIR) $(1)/etc
#	$(INSTALL_CONF) $(PKG_INSTALL_DIR)/etc/tinysnmp.conf $(1)/etc
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/tinysnmpd $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/share/tinysnmp/mibs
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/tinysnmp/mibs/SNMPv2-SMI.txt $(1)/usr/share/tinysnmp/mibs
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/tinysnmp/mibs/SNMPv2-TC.txt $(1)/usr/share/tinysnmp/mibs
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/tinysnmp/mibs/SNMPv2-CONF.txt $(1)/usr/share/tinysnmp/mibs
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/tinysnmp/mibs/SNMPv2-MIB.txt $(1)/usr/share/tinysnmp/mibs
endef

define Package/tinysnmp-module-interfaces/install
	$(INSTALL_DIR) $(1)/usr/lib/tinysnmp
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/tinysnmp/interfaces.so $(1)/usr/lib/tinysnmp
	$(INSTALL_DIR) $(1)/usr/share/tinysnmp/mibs
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/tinysnmp/mibs/IF-MIB.txt $(1)/usr/share/tinysnmp/mibs
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/tinysnmp/mibs/IANAifType-MIB.txt $(1)/usr/share/tinysnmp/mibs
endef

define Package/tinysnmp-module-resources/install
	$(INSTALL_DIR) $(1)/usr/lib/tinysnmp
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/tinysnmp/resources.so $(1)/usr/lib/tinysnmp
	$(INSTALL_DIR) $(1)/usr/share/tinysnmp/mibs
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/tinysnmp/mibs/FROGFOOT-RESOURCES-MIB.txt $(1)/usr/share/tinysnmp/mibs
endef

define Package/tinysnmp-module-ieee802dot11/install
	$(INSTALL_DIR) $(1)/usr/lib/tinysnmp
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/tinysnmp/ieee802dot11.so $(1)/usr/lib/tinysnmp
	$(INSTALL_DIR) $(1)/usr/share/tinysnmp/mibs
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/tinysnmp/mibs/IEEE802dot11-MIB.txt $(1)/usr/share/tinysnmp/mibs
endef

define Package/tinysnmp-module-mikrotik/install
	$(INSTALL_DIR) $(1)/usr/lib/tinysnmp
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/tinysnmp/mikrotik.so $(1)/usr/lib/tinysnmp
	$(INSTALL_DIR) $(1)/usr/share/tinysnmp/mibs
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/tinysnmp/mibs/Mikrotik.mib.txt $(1)/usr/share/tinysnmp/mibs
endef

$(eval $(call BuildPackage,tinysnmp-tools))
$(eval $(call BuildPackage,tinysnmp-agent))
$(eval $(call BuildPackage,tinysnmp-module-interfaces))
$(eval $(call BuildPackage,tinysnmp-module-resources))
$(eval $(call BuildPackage,tinysnmp-module-ieee802dot11))
$(eval $(call BuildPackage,tinysnmp-module-mikrotik))
