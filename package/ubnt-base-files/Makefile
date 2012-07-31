#
# Copyright (C) 2007 Ubiquiti Networks, Inc.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ubnt-base-files
PKG_RELEASE:=1

#PKG_FILE_DEPEND:=$(PLATFORM_DIR)/ $(GENERIC_PLATFORM_DIR)/ubnt-base-files/

include $(INCLUDE_DIR)/package.mk

SVN_REV:=$(shell svn info $(TOPDIR)/.. | grep "^Last Changed Rev:" | awk '{print $$4}')
ifeq ($(SVN_REV),)
	SVN_REV:=$(shell cd $(TOPDIR)/..; git svn info | grep "^Last Changed Rev:" | awk '{print $$4}'; cd - >/dev/null)
endif
ifeq ($(SVN_REV),)
	SVN_REV:=12536
endif
FULL_VERSION:=$(CONFIG_UBNT_FIRMWARE_VERSION).$(SVN_REV).$(shell date +%y%m%d.%H%M)
DSTAMP:=$(shell date +%m%d%H%M%Y)

ifneq ($(DUMP),1)
  TARGET:=-$(BOARD)
  ifneq ($(wildcard $(PLATFORM_DIR)/ubnt-base-files-$(PROFILE) $(PLATFORM_SUBDIR)/ubnt-base-files-$(PROFILE)),)
    TARGET:=$(TARGET)-$(PROFILE)
  endif
  LIBGCC_VERSION:=$(GCC_VERSION)
  ifeq ($(ARCH),x86_64)
    LIB_SUFFIX:=64
  endif
else
  LIBC_VERSION:=<LIBC_VERSION>
  LIBGCC_VERSION:=<LIBGCC_VERSION>
endif

CONFIG_PACKAGE_ubnt-base-files$(TARGET):=$(CONFIG_PACKAGE_ubnt-base-files)

define Package/ubnt-base-files$(TARGET)
  SECTION:=base
  CATEGORY:=Base system
  TITLE:=Base filesystem for OpenWrt (Ubiquiti flavour)
  URL:=http://www.ubnt.com
  VERSION:=$(PKG_RELEASE)-$(REV)
  DEPENDS:=+hotplug2 @!PACKAGE_base-files$(TARGET)
endef

define Package/ubnt-base-files$(TARGET)/conffiles
$(call $(TARGET)/conffiles)
endef

define Package/ubnt-base-files$(TARGET)/description
 This package contains a base filesystem and system scripts for OpenWrt (Ubiquiti flavour).
endef

define Package/ubnt-base-files/config
	source $(SOURCE)/config.in
endef

define Package/gcc/Default
  SECTION:=libs
  CATEGORY:=Base system
  DEPENDS:=@!NATIVE_TOOLCHAIN
  URL:=http://gcc.gnu.org/
  VERSION:=$(LIBGCC_VERSION)-$(PKG_RELEASE)
endef

define Package/libgcc
$(call Package/gcc/Default)
  TITLE:=GCC support library
  DEPENDS+=@!TARGET_avr32
endef

define Package/libssp
$(call Package/gcc/Default)
  DEPENDS+=@SSP_SUPPORT
  TITLE:=GCC support library
endef

define Package/libstdcpp
$(call Package/gcc/Default)
  NAME:=libstdc++
  TITLE:=GNU Standard C++ Library v3
  DEPENDS+=@INSTALL_LIBSTDCPP
endef

define Package/libc/Default
  SECTION:=libs
  CATEGORY:=Base system
  DEPENDS:=@!NATIVE_TOOLCHAIN
  VERSION:=$(LIBC_VERSION)-$(PKG_RELEASE)
  URL:=$(LIBC_URL)
endef

define Package/libc
$(call Package/libc/Default)
  TITLE:=C library
endef

define Package/libpthread
$(call Package/libc/Default)
  TITLE:=POSIX thread library
  DEPENDS:= +librt
endef

define Package/librt
$(call Package/libc/Default)
  TITLE:=POSIX.1b RealTime extension library
endef

define Package/ldd
$(call Package/uclibc/Default)
  TITLE:=LDD trace utility
endef

define Package/ldconfig
$(call Package/libc/Default)
  TITLE:=Shared library path configuration
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile/Default
endef

define Build/Compile
	export FULL_VERSION=$(FULL_VERSION)
	$(call Build/Compile/Default)
endef

define Package/ubnt-base-files$(TARGET)/install
	$(CP) files/* $(1)/
	if [ -d $(GENERIC_PLATFORM_DIR)/ubnt-base-files/. ]; then \
		$(CP) $(GENERIC_PLATFORM_DIR)/ubnt-base-files/* $(1)/; \
	fi
	if [ -d $(PLATFORM_DIR)/ubnt-base-files/default/. ]; then \
		$(CP) $(PLATFORM_DIR)/ubnt-base-files/default/* $(1)/; \
	fi
	if [ "$(PROFILE)" != "Default" ]; then \
		if [ -d files-$(PROFILE)/. ]; then \
			$(CP) files-$(PROFILE)/* $(1)/; \
		fi; \
	fi
	mkdir -p $(1)/dev
	mkdir -p $(1)/lib/firmware
	mkdir -p $(1)/mnt
	mkdir -p $(1)/proc
	mkdir -p $(1)/usr/lib
	mkdir -p $(1)/usr/bin
	mkdir -p $(1)/usr/www
	mkdir -p $(1)/sys
	mkdir -p $(1)/etc
	mkdir -p $(1)/var
	chmod +x $(1)/bin/support
	find $(1) -name ".svn" -exec rm -fr {} \; || echo "Ignoring errors!"
	ln -fs var/tmp $(1)/tmp
	echo $(FULL_VERSION) > $(1)/usr/lib/version
	$(SED) 's/^BDATE=/BDATE=$$(DSTAMP)/' $(1)/init
	# workaround -- force rebuild of this package every time
	rm -f $(PKG_BUILD_DIR)/.configured
endef

ifneq ($(DUMP),1)
  -include $(PLATFORM_DIR)/ubnt-base-files.mk
endif

$(eval $(call BuildPackage,ubnt-base-files$(TARGET)))
