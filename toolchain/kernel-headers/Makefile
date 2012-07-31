# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

KERNEL_BUILD_DIR := $(BUILD_DIR_TOOLCHAIN)
BUILD_DIR := $(KERNEL_BUILD_DIR)

STAGING_DIR_HOST:=$(TOOLCHAIN_DIR)
BUILD_DIR_HOST:=$(BUILD_DIR_TOOLCHAIN)
override QUILT:=
override CONFIG_AUTOREBUILD=

include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=linux
PKG_VERSION:=$(LINUX_VERSION)
PKG_SOURCE:=$(LINUX_SOURCE)
PKG_SOURCE_URL:=$(LINUX_SITE)
PKG_BUILD_DIR := $(KERNEL_BUILD_DIR)/linux-$(LINUX_VERSION)
PKG_MD5SUM:=$(LINUX_KERNEL_MD5SUM)
LINUX_DIR := $(PKG_BUILD_DIR)

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/kernel-defaults.mk

ifneq (,$(findstring uml,$(BOARD)))
  LINUX_KARCH:=$(ARCH)
endif

ifeq ($(strip $(call CompareKernelPatchVer,$(KERNEL_PATCHVER),ge,2.6.18)),1)

KMAKE := $(MAKE) -C $(PKG_BUILD_DIR) \
	ARCH=$(LINUX_KARCH) \
	CC="$(KERNEL_CC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	CROSS_COMPILE=$(TARGET_CROSS) \
	KBUILD_HAVE_NLS=no \
	CONFIG_SHELL=$(BASH)

define Build/Prepare/all
	mkdir -p $(LINUX_HEADERS_DIR)
	$(KMAKE) \
		INSTALL_HDR_PATH="$(LINUX_HEADERS_DIR)" \
		headers_install
endef

# XXX: the following is needed to build lzma-loader
ifneq ($(CONFIG_mips)$(CONFIG_mipsel),)
  define Build/Prepare/lzma
	$(CP) \
		$(PKG_BUILD_DIR)/arch/mips/include/asm/asm.h \
		$(PKG_BUILD_DIR)/arch/mips/include/asm/regdef.h \
		$(LINUX_HEADERS_DIR)/include/asm/
  endef
endif

define Build/Prepare/post/cris
	$(CP) \
		$(PKG_BUILD_DIR)/include/linux/user.h \
		$(LINUX_HEADERS_DIR)/include/linux/
	ln -snf $(BUILD_DIR_TOOLCHAIN)/linux-dev/include/arch-v10/arch \
		$(LINUX_HEADERS_DIR)/include/arch
	$(SED) '/#include <asm\/page\.h>/d' $(LINUX_HEADERS_DIR)/include/asm/user.h
endef

define Build/Prepare/post/ubicom32
	$(CP) \
		$(PKG_BUILD_DIR)/arch/ubicom32/include/asm/elf.h \
		$(PKG_BUILD_DIR)/arch/ubicom32/include/asm/user.h \
		$(PKG_BUILD_DIR)/arch/ubicom32/include/asm/page.h \
		$(PKG_BUILD_DIR)/arch/ubicom32/include/asm/page_offset.h \
		$(LINUX_HEADERS_DIR)/include/asm/
endef

define Build/Prepare/post/mips
	$(call Build/Prepare/lzma)
endef

define Build/Prepare/post/mipsel
	$(call Build/Prepare/lzma)
endef

define Build/Prepare
	$(call Kernel/Prepare/Default)
	ln -sf linux-$(LINUX_VERSION) $(LINUX_HEADERS_DIR)
	$(SED) 's/@expr length/@-expr length/' $(PKG_BUILD_DIR)/Makefile
	yes '' | $(KMAKE) oldconfig
	$(call Build/Prepare/all)
	$(call Build/Prepare/post/$(ARCH))
endef

else
define Build/Configure/cris
	ln -sf $(PKG_BUILD_DIR)/include/asm-cris/arch-v10 $(PKG_BUILD_DIR)/include/asm-cris/arch
	ln -sf $(PKG_BUILD_DIR)/include/asm-cris/arch-v10 $(PKG_BUILD_DIR)/arch/cris/arch
endef

define Build/Configure/powerpc
	$(CP) $(PKG_BUILD_DIR)/include/asm-ppc/* $(PKG_BUILD_DIR)/include/asm-powerpc/
	rm -rf $(PKG_BUILD_DIR)/include/asm-ppc
	ln -s $(PKG_BUILD_DIR)/include/asm-powerpc $(PKG_BUILD_DIR)/include/asm-ppc
endef

KMAKE := $(MAKE) -C $(PKG_BUILD_DIR) \
	ARCH=$(LINUX_KARCH) \
	KBUILD_HAVE_NLS=no \
	CONFIG_SHELL=$(BASH)

define Build/Prepare
	$(call Kernel/Prepare/Default)
	$(SED) 's/@expr length/@-expr length/' $(PKG_BUILD_DIR)/Makefile
	ln -sf linux-$(LINUX_VERSION) $(LINUX_HEADERS_DIR)
	yes '' | $(KMAKE) oldconfig
	$(KMAKE) include/linux/version.h include/asm
	$(call Build/Configure/$(ARCH))
endef
endif

define Build/Configure
endef

define Build/Compile
endef

define Build/Install
endef

$(eval $(call HostBuild))
