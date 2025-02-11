include ../config/Makefile.conf

TARGET = ar_flash

SRCS = $(TARGET).c font.tim ps1usb.rom
CFLAGS = -Os

include $(PSXDIR)/common.mk


all: $(TARGET).bin $(TARGET).cue

$(TARGET).bin $(TARGET).cue: $(OBJS) $(TARGET).xml
	mkpsxiso -y -q $(TARGET).xml

%.o: %.rom
	$(call OBJCOPYME)

clean:
	rm -f $(OBJS) $(BINDIR)Overlay.* $(BINDIR)*.elf $(BINDIR)*.ps-exe $(BINDIR)*.map $(BINDIR)$(TARGET).bin $(BINDIR)$(TARGET).cue $(DEPS)


.PHONY: all clean

