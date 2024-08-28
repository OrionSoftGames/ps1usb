TARGET = ps1usb
PSXDIR = /media/orion/Data/programmation/psx/psxdev

SRCS = $(TARGET).c font8x8.bin
CFLAGS = -O2

# Release in ROM
LDFLAGS += -Wl,--defsym=TLOAD_ADDR=0x801F6000

# Test in RAM
#LDFLAGS += -Wl,--defsym=TLOAD_ADDR=0x801D0000

include $(PSXDIR)/psyq_sdk/common.mk

all:

%.o: %.bin
	$(call OBJCOPYME)

