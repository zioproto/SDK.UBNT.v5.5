#
# Copyright (C) 2007-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 12607 2008-09-14 00:44:37Z nbd $

include $(TOPDIR)/rules.mk

PKG_NAME:=ssmtp
PKG_VERSION:=2.61
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).orig.tar.gz
PKG_SOURCE_URL:=http://ftp.debian.org/debian/pool/main/s/ssmtp/
PKG_MD5SUM:=957e6fff08625fe34f4fc33d0925bbc9

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include

define Package/ssmtp
  SECTION:=mail
  CATEGORY:=Mail
  DEPENDS:=+libopenssl
  TITLE:=A minimal and secure mail sender
  URL:=http://packages.debian.org/stable/mail/ssmtp
endef

define Package/ssmtp/description
 A secure, effective and simple way of getting mail off a system to your 
 mail hub. It contains no suid-binaries or other dangerous things - no 
 mail spool to poke around in, and no daemons running in the background.
 mail is simply forwarded to the configured mailhost. Extremely easy 
 configuration.
endef

CONFIGURE_VARS += \
	LIBS="-L$(STAGING_DIR)/usr/lib -lcrypto -lssl"

CONFIGURE_ARGS += \
	--enable-ssl

define Package/ssmtp/install	
	$(INSTALL_DIR) $(1)/etc/ssmtp
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/ssmtp.conf $(1)/etc/ssmtp/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/revaliases $(1)/etc/ssmtp/
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ssmtp $(1)/usr/sbin/
	ln -sf ssmtp $(1)/usr/sbin/sendmail
endef

$(eval $(call BuildPackage,ssmtp))
