#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
#

# Makefile for Atheros boards
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

#.S.s:
#	$(CPP) $(CFLAGS) $< -o $*.s
#.S.o:
#	$(CC) $(CFLAGS) -D__ASSEMBLY__ -c $< -o $*.o

obj-y	+=	prom.o	\
		gpio.o	\
		irq.o	\
		setup.o	\
		misc.o \
		platform.o \
		wdt.o

obj-$(CONFIG_MACH_AR724x) += 724x.o
obj-$(CONFIG_MACH_AR933x) += 933x.o
obj-$(CONFIG_MACH_AR934x) += 934x.o
obj-$(CONFIG_MACH_AR934x) += otp.o
obj-$(CONFIG_MACH_AR934x) += clksw.o

ifdef CONFIG_CUS100
CFLAGS_gpio.o += -DCONFIG_CUS100=1
endif

ifndef JUMPSTART_GPIO
JUMPSTART_GPIO=12
endif

ifndef WPS_LED_GPIO_PIN
WPS_LED_GPIO_PIN=5
endif

CFLAGS_gpio.o += -DWPS_LED_GPIO=$(WPS_LED_GPIO_PIN)
CFLAGS_gpio.o += -DJUMPSTART_GPIO=$(JUMPSTART_GPIO)

ifdef AP_USB_LED_GPIO
CFLAGS_gpio.o += -DAP_USB_LED_GPIO=$(AP_USB_LED_GPIO)
endif

ifdef CONFIG_I2S
CFLAGS_gpio.o += -DCONFIG_I2S=$(CONFIG_I2S)
endif

ifeq ($(BUILD_EMU), 24)
EXTRA_CFLAGS += -DCONFIG_HORNET_EMULATION_WLAN_HARDI
endif

ifeq ($(CONFIG_HORNET_XTAL), 40)
EXTRA_CFLAGS += -DCONFIG_40MHZ_XTAL_SUPPORT
endif

#EXTRA_CFLAGS				+=	-fno-schedule-insns -fno-schedule-insns2
