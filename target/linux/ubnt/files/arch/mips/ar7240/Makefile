#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
#

# Makefile for Atheros ar531x boards
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
			ar7240.o \
			platform.o \
			wdt.o

ifdef CONFIG_CUS100
CFLAGS_gpio.o += -DCONFIG_CUS100=1
endif

ifndef JUMPSTART_GPIO
JUMPSTART_GPIO=12
endif

ifndef WPS_LED_GPIO_PIN
WPS_LED_GPIO_PIN=5
endif

CFLAGS_gpio.o += -DJUMPSTART_GPIO=$(JUMPSTART_GPIO)
CFLAGS_gpio.o += -DWPS_LED_GPIO=$(WPS_LED_GPIO_PIN)

ifdef ATH_SUPPORT_LED
CFLAGS_gpio.o += -DATH_SUPPORT_LED=$(ATH_SUPPORT_LED)
CFLAGS_gpio.o += -DGPIO_PIN_FUNC_0=$(GPIO_PIN_FUNC_0)
CFLAGS_gpio.o += -DGPIO_PIN_FUNC_1=$(GPIO_PIN_FUNC_1)
CFLAGS_gpio.o += -DGPIO_PIN_FUNC_2=$(GPIO_PIN_FUNC_2) 
endif

ifdef AP_USB_LED_GPIO
CFLAGS_gpio.o += -DAP_USB_LED_GPIO=$(AP_USB_LED_GPIO)
endif

ifeq ($(BUILD_EMU), 24)
EXTRA_CFLAGS += -DCONFIG_HORNET_EMULATION_WLAN_HARDI
endif

ifeq ($(CONFIG_HORNET_XTAL), 40)
EXTRA_CFLAGS += -DCONFIG_40MHZ_XTAL_SUPPORT
endif

ifndef AP_RESET_GPIO
AP_RESET_GPIO=21
endif

EXTRA_CFLAGS += -DAP_RESET_GPIO=$(AP_RESET_GPIO)

ifeq ($(AP_WATCHDOG_RESET_DISABLE), 1)
CFLAGS_wdt.o += -DAP_WATCHDOG_RESET_DISABLE
endif

#EXTRA_CFLAGS				+=	-fno-schedule-insns -fno-schedule-insns2
