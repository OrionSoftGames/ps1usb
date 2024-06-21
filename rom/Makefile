
all: uncompressed

uncompressed:

	mipsel-linux-gnu-as -o rom_uncompressed.o rom_uncompressed.s
	mipsel-linux-gnu-ld -T ./nugget/rom.ld -Ttext 0x1F000000 --oformat binary -o ps1usb.rom rom_uncompressed.o
	rm -f *.o
	../../../../ctools/rom_rounder/rom_rounder ps1usb.rom

