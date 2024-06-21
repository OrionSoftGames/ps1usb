TARGET = ps1usb
#PSXDIR = path to psyq sdk

SRCS = $(TARGET).c font8x8.bin
CFLAGS = -O2

# Release
#LDFLAGS += -Wl,--defsym=TLOAD_ADDR=0x801F6000

# Test
LDFLAGS += -Wl,--defsym=TLOAD_ADDR=0x801D0000

include $(PSXDIR)/psyq_sdk/common.mk

all:

%.o: %.bin
	$(call OBJCOPYME)

