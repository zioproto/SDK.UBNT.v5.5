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
			ar7100.o \
			platform.o 

obj-$(CONFIG_AR9100)	+= wdt.o

ifdef CONFIG_CUS100
CFLAGS_gpio.o += -DCONFIG_CUS100=1
endif
ifdef JUMPSTART_GPIO
CFLAGS_gpio.o += -DJUMPSTART_GPIO=$(JUMPSTART_GPIO)
endif
ifdef FACTORY_RESET_GPIO
CFLAGS_gpio.o += -DFACTORY_RESET_GPIO=$(FACTORY_RESET_GPIO)
endif
#EXTRA_CFLAGS				+=	-fno-schedule-insns -fno-schedule-insns2
