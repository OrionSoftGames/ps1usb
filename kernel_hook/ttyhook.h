/*
* PS1USB firmware by OrionSoft [2024]
* Hook to redirect printf/TTY to USB
*
* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
* If a copy of the MPL was not distributed with this file,
* You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "common/syscalls/syscalls.h"

// Not in syscall.h :/
static __attribute__((always_inline)) int syscall_removeDevice(const char *filename) {
    register int n asm("t1") = 0x48;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int (*)(const char *))0xb0)(filename);
}

int	USB_tty_open(struct File *file, const char *filename, int mode)
{
	return (file->deviceId);
}

int	USB_tty_action(struct File *file, enum FileAction action)
{
	int					i, j;
	volatile uint8_t	*buf8 = (volatile uint8_t *)file->buffer;
	bool				ret;

	for (i = j = 0; i < file->count; i++)
	{
		if (action == PSXWRITE)
			ret = USB_SendByte(*buf8);
		else
			ret = USB_GetByte(buf8);
		if (ret)
		{
			j++;
			buf8++;
		}
	}
	return (j);
}

int	USB_tty_dummy(void)
{
	return (0);
}

struct Device	USB_tty_dev =
{
	"tty",
	PSXDTTYPE_CHAR | PSXDTTYPE_CONS,
	0,
	"PS1USB tty by OrionSoft 2024",
	(device_init)USB_tty_dummy,
	USB_tty_open,
	USB_tty_action,
	(device_close)USB_tty_dummy,
	(device_ioctl)USB_tty_dummy,
	(device_read)USB_tty_dummy,
	(device_write)USB_tty_dummy,
	USB_tty_dummy,
	USB_tty_dummy,
	(device_firstFile)USB_tty_dummy,
	(device_nextFile)USB_tty_dummy,
	(device_format)USB_tty_dummy,
	USB_tty_dummy,
	USB_tty_dummy,
	(device_deinit)USB_tty_dummy,
	USB_tty_dummy
};

void	SetupTTYhook(void)
{
	syscall_close(0);						// Close stdin/stdout handles
	syscall_close(1);
	syscall_removeDevice(USB_tty_dev.name);	// Remove default TTY device
	syscall_addDevice(&USB_tty_dev);		// Add our own TTY device with USB communication callbacks
	syscall_open(USB_tty_dev.name, 2);		// Reopen stdin/stdout
	syscall_open(USB_tty_dev.name, 1);
}
