#
# Copyright (C) 2007 Ubiquiti Networks, Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ubnt-web
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install

FULL_VERSION:=$(shell date +%y%m%d.%H%M)

include $(INCLUDE_DIR)/package.mk

define Package/ubnt-web
  SECTION:=base
  CATEGORY:=Base system
  DEPENDS:=+ubnt-base-files +lighttpd +ubnt-cgi +php2 +wireless-tools +ubntbox @BUSYBOX_CONFIG_TRACEROUTE
  TITLE:=Web configuration system.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	tar -C src/ --exclude .svn -cf - . | tar -C $(PKG_BUILD_DIR) -xvf -
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/ubnt-web/install
	$(INSTALL_DIR) $(1)/usr/www
	$(call Build/Compile/Default, build_install_directory="$(1)" FULL_VERSION="$(FULL_VERSION)" install)
endef

$(eval $(call RequireCommand,yui-compressor, \
        $(PKG_NAME) requires yui-compressor - a JavaScript/CSS minifier \
))

$(eval $(call BuildPackage,ubnt-web))
