#
# Copyright (C) 2011 Ubiquiti Networks Inc.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mips
BOARD:=ubnt
BOARDNAME:=Ubiquiti AR724x
FEATURES:=squashfs tgz

LINUX_VERSION:=2.6.32.54
INITRAMFS_EXTRA_FILES:=${CURDIR}/image/initramfs-base-files.txt

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += hostapd-mini

define Target/Description
	Build firmware images for Atheros AR724x based boards, 2.6.32
endef

define Kernel/TransferDirectory
	mkdir -p $(1)/$(3)
	tar -C $(1)/$(2) -cf - . | tar -C $(1)/$(3) -xf -
	rm -rf $(1)/$(2)
	ln -sf $(patsubst %..,%../,$(subst $(space),/,$(patsubst %,..,$(subst /,$(space), $(subst .,,$(dir $(2)))))))$(3) $(1)/$(2)
endef

define Kernel/RemoveUnused
	mkdir -p $(TARGET_DIR)/usr/etc
	if [ -d $(TARGET_DIR)/etc/modules.d ]; then mv $(TARGET_DIR)/etc/modules.d $(TARGET_DIR)/usr/etc/; fi
	#remove everything that is not needed for us
	rm -rf $(TARGET_DIR)/usr/lib/ipkg \
		$(TARGET_DIR)/usr/lib/firewall.awk \
		$(TARGET_DIR)/usr/lib/opkg
	# $(TARGET_DIR)/etc
endef

define Kernel/FixAll
	$(call Kernel/RemoveUnused)
	$(call Kernel/TransferDirectory,$(TARGET_DIR),etc,var/etc)
	$(call Kernel/TransferDirectory,$(TARGET_DIR),sbin,bin)
	$(call Kernel/TransferDirectory,$(TARGET_DIR),usr/bin,bin)
	$(call Kernel/TransferDirectory,$(TARGET_DIR),usr/sbin,bin)
	$(call Kernel/TransferDirectory,$(TARGET_DIR),usr/lib,lib)
endef

define Kernel/CompileImage
	$(call Kernel/FixAll)
	$(call Kernel/CompileImage/Default)
endef

$(eval $(call BuildTarget))
