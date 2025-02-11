##### Taken from nolibgs_hello_worlds/thirdparty/nugget/common.mk #####
HAS_LINUX_MIPS_GCC = $(shell which mipsel-linux-gnu-gcc > /dev/null 2> /dev/null && echo true || echo false)

ifeq ($(HAS_LINUX_MIPS_GCC),true)
PREFIX ?= mipsel-linux-gnu
EXT = 
else
PREFIX ?= mipsel-none-elf
EXT = .exe
endif
#######################################################################

TARGET = ps1usb
TARGET_DEP = rom_rounder

AS = $(PREFIX)-as
LD = $(PREFIX)-ld


all: $(TARGET_DEP)$(EXT) $(TARGET).rom

# Run make inside $(TARGET_DEP) folder everytime, a bit hacky but works
# https://stackoverflow.com/questions/26226106/how-can-i-force-make-to-execute-a-recipe-all-the-time
$(TARGET_DEP)$(EXT): FORCE
	$(MAKE) -C $(TARGET_DEP)

FORCE: ;

$(TARGET).rom: rom.o
	$(LD) -T ./nugget/rom.ld -Ttext 0x1F000000 --oformat binary -o $(TARGET).rom rom.o
	./rom_rounder/rom_rounder $(TARGET).rom
	cp $(TARGET).rom ../ar_flash/

rom.o: ../ps1usb.ps-exe tim.tfs *.inc rom.s
	$(AS) -o rom.o rom.s

clean:
	rm -f *.o $(TARGET).rom ../ar_flash/$(TARGET).rom
	$(MAKE) -C $(TARGET_DEP) clean


.PHONY: all clean

