# 
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=php
PKG_VERSION:=2.0.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://museum.php.net/php2/
PKG_MD5SUM:=c538adf59ddf8e99b5637292309a1e86
PKG_CAT:=zcat

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install

include $(INCLUDE_DIR)/package.mk

define Package/php2
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=PHP2 Web Scripting Language
  URL:=http://museum.php.net/php2/
endef

define Package/php2/description
	PHP is a widely-used general-purpose scripting language that is especially suited for Web development and can be embedded into HTML.
endef

define Build/Configure
        (cd $(PKG_BUILD_DIR); rm -rf config.cache; \
		$(TARGET_CONFIGURE_OPTS) \
		LDFLAGS=-lm \
		CFLAGS=-Os \
		CC="$(TARGET_CC)" \
                ./install \
                        --prefix=/usr \
			--sysconfdir=/etc \
			--datadir=/usr/share \
			--host=$(GNU_TARGET_NAME) \
			--build=$(GNU_HOST_NAME) \
        )
endef

define Build/Compile
	-$(MAKE) -C $(PKG_BUILD_DIR)/src clean
	$(MAKE) -C $(PKG_BUILD_DIR)/src
endef


define Package/php2/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/php.cgi $(1)/sbin/cgi
endef	

$(eval $(call BuildPackage,php2))
