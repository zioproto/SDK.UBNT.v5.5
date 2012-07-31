# 
# Copyright (C) 2006 OpenWrt.org
# Copyright 2006-2012, Ubiquiti Networks, Inc.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=matrixssl
PKG_VERSION:=1.8.3
PKG_RELEASE:=1


PKG_SOURCE:=$(PKG_NAME)-$(subst .,-,$(PKG_VERSION))-open.tar.gz
PKG_SOURCE_URL:=http://www.mirrorservice.org/sites/www.ibiblio.org/gentoo/distfiles/
PKG_MD5SUM:=1cfc2622b01b5457538c0d8ec58790bf

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE:.tar.gz=)
PKG_UNPACK:=zcat $(DL_DIR)/$(PKG_SOURCE) | \
	$(TAR) -C $(BUILD_DIR) $(TAR_OPTIONS); \
	find $(PKG_BUILD_DIR) -type f -exec $(SED) 's/\x0d$$$$//' {} \;
include $(INCLUDE_DIR)/package.mk

define Package/libmatrixssl
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Embedded SSL implementation
  URL:=http://www.matrixssl.org/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		$(TARGET_CONFIGURE_OPTS) \
		DFLAGS="$(TARGET_CFLAGS) -Wall"
endef

define Build/InstallDev
	mkdir -p $(STAGING_DIR)/usr/include/matrixssl
	$(CP) $(PKG_BUILD_DIR)/matrixSsl.h $(STAGING_DIR)/usr/include/matrixssl/
	$(CP) $(PKG_BUILD_DIR)/matrixCommon.h $(STAGING_DIR)/usr/include/matrixssl/
	$(CP) $(PKG_BUILD_DIR)/matrixssl_helper.h $(STAGING_DIR)/usr/include/matrixssl/
	mkdir -p $(STAGING_DIR)/usr/include/matrixssl/src
	$(CP) $(PKG_BUILD_DIR)/src/matrixConfig.h $(STAGING_DIR)/usr/include/matrixssl/src/matrixConfig.h
	
	mkdir -p $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/src/libmatrixssl.so* $(STAGING_DIR)/usr/lib/
endef

define Build/UninstallDev
	rm -rf \
		$(STAGING_DIR)/usr/include/matrixssl \
		$(STAGING_DIR)/usr/lib/libmatrixssl.so*
endef

define Package/libmatrixssl/install
	$(INSTALL_DIR) $(1)/lib
	$(CP) $(PKG_BUILD_DIR)/src/libmatrixssl.so* $(1)/lib/
endef

$(eval $(call BuildPackage,libmatrixssl))
