# SPDX-License-Identifier: GPL-2.0-only
obj-y	= printk.o
obj-$(CONFIG_PRINTK)	+= printk_safe.o
obj-$(CONFIG_A11Y_BRAILLE_CONSOLE)	+= braille.o
obj-$(CONFIG_PRINTK)	+= printk_ringbuffer.o

EXTRA_CFLAGS += -DDEVKMSG_LIMIT_CONTROL

ifeq ($(CONFIG_DFX_TIME), y)
EXTRA_CFLAGS += -I$(srctree)/platform_source/basicplatform/drivers/printk/
endif
