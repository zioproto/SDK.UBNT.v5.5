#
# Copyright (C) 2006-2009 OpenWrt.org
# Copyright (C) 2009 Ubiquiti Networks, Inc.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 14266 2009-01-29 16:07:29Z florian $

include $(TOPDIR)/rules.mk

PKG_NAME:=lighttpd
PKG_VERSION:=1.4.30
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://download.lighttpd.net/lighttpd/releases-1.4.x/
PKG_MD5SUM:=63f9df52dcae0ab5689a95c99c54e48a

PKG_FIXUP = libtool

include $(INCLUDE_DIR)/package.mk

define Package/lighttpd/Default
  SUBMENU:=Web
  SECTION:=net
  CATEGORY:=Network
  URL:=http://www.lighttpd.net/
endef

define Package/lighttpd
  $(call Package/lighttpd/Default)
  MENU:=1
  DEPENDS:=+libopenssl
  TITLE:=A flexible and lightweight web server
endef


CONFIGURE_ARGS+= \
	--libdir=/usr/lib/lighttpd \
	--sysconfdir=/etc/lighttpd \
	--enable-shared \
	--enable-static \
	--disable-rpath \
	--without-attr \
	--without-bzip2 \
	--without-fam \
	--without-gdbm \
	--without-ldap \
	--without-lua \
	--without-memcache \
	--without-mysql \
	--with-openssl="$(STAGING_DIR)/usr" \
	--without-pcre \
	--with-tdb \
	--without-valgrind \
	--without-webdav-props \

CONFIGURE_VARS+= \
	CPPFLAGS="$$$$CPPFLAGS" \

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) DESTDIR="$(PKG_INSTALL_DIR)" all install
endef

# hack to cope with libtool rechecks during make
define Build/Configure
	(cd $(PKG_BUILD_DIR); \
	 	autoreconf -v || exit 1 \
	);
	$(call Build/Configure/Default)
endef

define Package/lighttpd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/lighttpd $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,lighttpd))
