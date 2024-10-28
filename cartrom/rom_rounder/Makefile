ifeq ($(OS),Windows_NT)
	EXT = .exe
else
	EXT = 
endif

TARGET = rom_rounder
SRCS = main.c


all: $(TARGET)$(EXT)

$(TARGET)$(EXT): $(SRCS)
	gcc -o $(TARGET)$(EXT) $(SRCS)

clean:
	rm -f $(TARGET)$(EXT)


.PHONY: all clean

