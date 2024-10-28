include ../config/Makefile.conf

TARGET = kernel_hook

SRCS = $(TARGET).c font8x8.bin
CFLAGS = -O2 -DHOOK

# See linker script in the current folder
LDFLAGS += -Wl,--defsym=TLOAD_ADDR=0x8000C160

LDSCRIPT ?= ps-exe_noheader_kernel_hook.ld

include $(PSXDIR)/common.mk


all: ../$(TARGET).bin

../$(TARGET).bin: $(TARGET).ps-exe
	cp $(TARGET).ps-exe ../$(TARGET).bin

%.o: %.bin
	$(call OBJCOPYME)

clean:
	rm -f $(OBJS) $(BINDIR)Overlay.* $(BINDIR)*.elf $(BINDIR)*.ps-exe $(BINDIR)*.map $(BINDIR)../$(TARGET).bin $(DEPS)


.PHONY: all clean

