TARGET = ps1usb_example
#PSXDIR = here the path to psyq_sdk

SRCS = example.c libsn_usb.c
CFLAGS = -O2

include $(PSXDIR)/psyq_sdk/common.mk

all:
	mkpsxiso -y -q ps1usb_example.xml
	./ps1transfer -e -f ps1usb_example.ps-exe -c -p .

