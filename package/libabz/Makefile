# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 5624 2006-11-23 00:29:07Z nbd $

include $(TOPDIR)/rules.mk

PKG_NAME:=libabz
PKG_VERSION:=0.6.2
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://archive.ubuntu.com/ubuntu/pool/universe/liba/libabz
PKG_MD5SUM:=a44c7a1c8c23e1059b98b1968c3d5d29
PKG_CAT:=zcat

PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install

include $(INCLUDE_DIR)/package.mk

define Package/libabz
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Miscellaneous useful routines
  URL:=http://oasis.frogfoot.net/code/libabz/
  DEPENDS:=+libdebug
endef

define Build/Configure
endef

define Build/Compile
	CROSS="$(TARGET_CROSS)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	CPPFLAGS="-I$(STAGING_DIR)/usr/include" \
	LDFLAGS="-L$(STAGING_DIR)/usr/lib" \
	$(MAKE) -C $(PKG_BUILD_DIR) SHELL=/bin/bash \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		prefix="$(PKG_INSTALL_DIR)/usr" \
		all install
endef

define Build/InstallDev
	mkdir -p $(STAGING_DIR)/usr/include/abz
	$(CP) -r $(PKG_INSTALL_DIR)/usr/include/abz/* $(STAGING_DIR)/usr/include/abz/
	mkdir -p $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libabz.{a,so} $(STAGING_DIR)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libabz.so.* $(STAGING_DIR)/usr/lib/
endef

define Build/UninstallDev
	rm -rf \
		$(STAGING_DIR)/usr/include/abz \
		$(STAGING_DIR)/usr/lib/libabz.{a,so} \
		$(STAGING_DIR)/usr/lib/libabz.so.*
endef

define Package/libabz/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libabz.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libabz))
