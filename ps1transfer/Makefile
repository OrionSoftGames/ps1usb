# CodeBlocks project files were converted to makefiles with cbp2make (https://sourceforge.net/projects/cbp2make/files/binaries-beta/)
# ps1transfer_win.cbp -> Makefile_win
# ps1transfer_linux.cbp -> Makefile_linux

ifeq ($(OS),Windows_NT)
	MAKEFILE = Makefile_win
else
	MAKEFILE = Makefile_linux
endif

all:
	$(MAKE) -f $(MAKEFILE) release

clean:
	$(MAKE) -f $(MAKEFILE) clean_release


.PHONY: all clean

