# 
# Copyright (C) 2007 Ubiquiti Networks, Inc
#
include $(TOPDIR)/rules.mk

PKG_NAME:=ubnt-mkfwimage

include $(INCLUDE_DIR)/host-build.mk

define Build/Compile
	$(CC) -O src/ubnt-mkfwimage.c -o $(PKG_BUILD_DIR)/ubnt-mkfwimage -lz
	$(CC) -O src/fwsplit.c -o $(PKG_BUILD_DIR)/fwsplit -lz
endef

define Build/Install
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ubnt-mkfwimage $(STAGING_DIR_HOST)/bin/
	# mkdir -p $(STAGING_DIR)/usr/include/$(PKG_NAME)
	# $(CP) src/*.h $(STAGING_DIR)/usr/include/$(PKG_NAME)/
endef

define Build/Clean
	rm -f $(STAGING_DIR)/bin/ubnt-mkfwimage
endef

$(eval $(call HostBuild))
