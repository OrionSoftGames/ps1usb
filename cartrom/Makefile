
all:
	mipsel-linux-gnu-as -o rom.o rom.s
	mipsel-linux-gnu-ld -T ./nugget/rom.ld -Ttext 0x1F000000 --oformat binary -o ps1usb.rom rom.o
	rm -f *.o
	./rom_rounder/rom_rounder ps1usb.rom
	cp ps1usb.rom ../ar_flash/

