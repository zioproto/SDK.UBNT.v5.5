#
# Copyright (C) 2011 Ubiquiti Networks, Inc.
#

ifeq ($(CONFIG_PACKAGE_kmod-atheros-11n-9.2),y)
ATH_DRIVER_PACKAGE_NAME:=atheros-11n-9.2
else
ATH_DRIVER_PACKAGE_NAME:=ath-11n
endif

define atheros_build_dep
	PKG_BUILD_DEPENDS+=PACKAGE_kmod-atheros-11n-9.2:kmod-atheros-11n-9.2
	PKG_BUILD_DEPENDS+=PACKAGE_kmod-ath-11n:kmod-ath-11n
endef
