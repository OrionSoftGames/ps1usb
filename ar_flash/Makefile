TARGET = ar_flash
PSXDIR = /media/orion/Data/programmation/psx/psxdev

SRCS = $(TARGET).c font.tim ps1usb.rom
CFLAGS = -Os

include $(PSXDIR)/psyq_sdk/common.mk

all:
	mkpsxiso -y -q ar_flash.xml

%.o: %.rom
	$(call OBJCOPYME)

