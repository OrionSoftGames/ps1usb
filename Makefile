include config/Makefile.conf

TARGET = ps1usb
TARGET_DEP = kernel_hook

SRCS = $(TARGET).c $(TARGET_DEP).bin
CFLAGS = -O2

# Release in ROM
LDFLAGS += -Wl,--defsym=TLOAD_ADDR=0x801EF000

# Test in RAM
#LDFLAGS += -Wl,--defsym=TLOAD_ADDR=0x801D0000

include $(PSXDIR)/common.mk


all:

# Run make inside $(TARGET_DEP) folder everytime, a bit hacky but works
# https://stackoverflow.com/questions/26226106/how-can-i-force-make-to-execute-a-recipe-all-the-time
$(TARGET_DEP).bin: FORCE
	$(MAKE) -C $(TARGET_DEP)

FORCE: ;

%.o: %.bin
	$(call OBJCOPYME)

clean:
	rm -f $(OBJS) $(BINDIR)Overlay.* $(BINDIR)*.elf $(BINDIR)*.ps-exe $(BINDIR)*.map $(DEPS)
	$(MAKE) -C $(TARGET_DEP) clean

.PHONY: all clean

