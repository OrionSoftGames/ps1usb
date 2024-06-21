/*
* PS1USB libsn_usb by OrionSoft [2024]
*
* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
* If a copy of the MPL was not distributed with this file,
* You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

static const uint8_t CRC8_Tab[256] =
{
	  0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
	 35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	 70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	 17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	 50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
	 87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};

uint8_t		CRC8_Compute(uint8_t *data, uint32_t size)
{
	uint8_t	crc = 0xFF;

	while (size--)
		crc = CRC8_Tab[crc ^ (*data++)];

	return (crc);
}

/****************************************/
// PS1 USB Data Transfer Protocol

#define	USB_STATUS	*((volatile uint8_t *)0x1F060000)	// USB Status A17+A18
#define	USB_DATA	*((volatile uint8_t *)0x1F020000)	// USB Data A17

// Status bits are inverted (0 = Yes, 1 = No)
#define	USB_STATUS_MASK_HAVE_DATA		(1 << 0)	// FT245R: RXF
#define	USB_STATUS_MASK_CAN_SEND_DATA	(1 << 1)	// FT245R: TXE
#define	USB_STATUS_MASK_USB_READY		(1 << 2)	// FT245R: PWREN

#define	TIMEOUT_CNT		5000000	// DO NOT MODIFY THIS !
#define	MAX_PACKET_SIZE	2048	// DO NOT MODIFY THIS !

bool	USB_GetByte(volatile uint8_t *data)
{
	uint32_t	timeout = TIMEOUT_CNT;

	while (USB_STATUS & USB_STATUS_MASK_HAVE_DATA)
		if (!(timeout--))
			return (false);

	*data = USB_DATA;
	return (true);
}

bool	USB_GetByteNoTimeout(volatile uint8_t *data)
{
	if (USB_STATUS & USB_STATUS_MASK_HAVE_DATA)
		return (false);
	*data = USB_DATA;
	return (true);
}

bool	USB_SendByte(uint8_t data)
{
	uint32_t	timeout = TIMEOUT_CNT;

	while (USB_STATUS & USB_STATUS_MASK_CAN_SEND_DATA)
		if (!(timeout--))
			return (false);

	USB_DATA = data;
	return (true);
}

bool	USB_Process(uint8_t cmd, uint32_t param, uint8_t *data, uint32_t size, uint32_t *ret)
{
	int		i;
	uint8_t	*ptr;
	uint8_t	USB_cmd[1+1+4+4+1];

	// Check if USB is connected
	if (USB_STATUS & USB_STATUS_MASK_USB_READY)
		return (false);

	// Send a special char to PC to tells it to switch from TTY Console to Binary Data Transfer
	USB_cmd[0] = 16;	// Data Link Escape
	USB_cmd[1] = cmd;
	USB_cmd[2] = (param >> 24) & 0xFF;
	USB_cmd[3] = (param >> 16) & 0xFF;
	USB_cmd[4] = (param >> 8) & 0xFF;
	USB_cmd[5] = param & 0xFF;
	USB_cmd[6] = (size >> 24) & 0xFF;
	USB_cmd[7] = (size >> 16) & 0xFF;
	USB_cmd[8] = (size >> 8) & 0xFF;
	USB_cmd[9] = size & 0xFF;
	USB_cmd[10] = CRC8_Compute(&USB_cmd[1], 1+4+4);

	// Send command packet
	for (i = 0; i < sizeof(USB_cmd); i++)
		if (!USB_SendByte(USB_cmd[i]))
			return (false);

	// Wait ACK
	if (!USB_GetByte(&USB_cmd[0]))
		return (false);

	// Check ACK
	if (USB_cmd[0] != 'G')
		return (false);

	// Read/Write data
	if (data)
	{
		uint8_t		crc;
		uint32_t	lsize;

		// Read data from PC
		ptr = data;
		if (cmd == 'R')
		{
			while (size)
			{
				crc = 0xFF;
				lsize = (size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : size;
				while (lsize--)
				{
					if (!USB_GetByte(ptr))
						return (false);
					crc = CRC8_Tab[crc ^ (*ptr++)];
				}
				if (!USB_SendByte(crc))
					return (false);
				if (!USB_GetByte(&cmd))
					return (false);
				lsize = (size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : size;
				if (cmd == 'N')
					size -= lsize;
				else
					ptr -= lsize;
			}
		}
		else // Write data to PC
		{
			while (size)
			{
				crc = 0xFF;
				lsize = (size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : size;
				while (lsize--)
				{
					if (!USB_SendByte(*ptr))
						return (false);
					crc = CRC8_Tab[crc ^ (*ptr++)];
				}
				if (!USB_SendByte(crc))
					return (false);
				if (!USB_GetByte(&cmd))
					return (false);
				lsize = (size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : size;
				if (cmd == 'N')
					size -= lsize;
				else
					ptr -= lsize;
			}
		}
	}

	// Get Return value
	i = 4;
	ptr = USB_cmd;
	while (i--)
	{
		if (!USB_GetByte(ptr))
			return (false);
		ptr++;
	}
	if (ret)
		*ret = (USB_cmd[0] << 24) | (USB_cmd[1] << 16) | (USB_cmd[2] << 8) | USB_cmd[3];

	return (true);
}

/*
** re-initialise PC filing system, close open files etc
**
** passed: void
**
** return: error code (0 if no error)
*/
int	PCinit(void)
{
	if (!USB_Process('I', 0, NULL, 0, NULL))
		return (-1);
	return (0);
}

/*
** open a file on PC host
**
** passed:	PC file pathname, open mode, permission flags
**
** return:	file-handle or -1 if error
**
** note: perms should be zero (it is ignored)
**
** open mode:	0 => read only
** 				1 => write only
**					2 => read/write
*/
int	PCopen(char *name, int flags, int perms)
{
	int	ret;

	if (!USB_Process('O', flags, name, strlen(name)+1, &ret))
		return (-1);
	return (ret);
}

/*
** create (and open) a file on PC host
**
** passed:	PC file pathname, open mode, permission flags
**
** return:	file-handle or -1 if error
**
** note: perms should be zero (it is ignored)
*/
int	PCcreat(char *name, int perms)
{
	int	ret;

	if (!USB_Process('C', 0, name, strlen(name)+1, &ret))
		return (-1);
	return (ret);
}

/*
** seek file pointer to new position in file
**
** passed: file-handle, seek offset, seek mode
**
** return: absolute value of new file pointer position
**
** (mode 0 = rel to start, mode 1 = rel to current fp, mode 2 = rel to end)
*/
int	PClseek(int fd, int offset, int mode)
{
	int	ret;

	if (fd < 0)
		return (-1);
	if (!USB_Process('S', (mode << 24) | (fd & 0xFFFFFF), NULL, offset, &ret))
		return (-1);
	return (ret);
}

/*
** read bytes from file on PC
**
** passed: file-handle, buffer address, count
**
** return: count of number of bytes actually read
**
** note: unlike assembler function this provides for full 32 bit count
*/
int	PCread(int fd, char *buff, int len)
{
	int	ret;

	if (fd < 0)
		return (-1);
	if (!USB_Process('R', fd, buff, len, &ret))
		return (-1);
	return (ret);
}

/*
** write bytes to file on PC
**
** passed: file-handle, buffer address, count
**
** return: count of number of bytes actually written
**
** note: unlike assembler function this provides for full 32 bit count
*/
int	PCwrite(int fd, char *buff, int len)
{
	int	ret;

	if (fd < 0)
		return (-1);
	if (!USB_Process('W', fd, buff, len, &ret))
		return (-1);
	return (ret);
}

/*
** close an open file on PC
**
** passed: file-handle
**
** return: negative if error
**
*/
int	PCclose(int fd)
{
	int	ret;

	if (fd < 0)
		return (-1);
	if (!USB_Process('c', fd, NULL, 0, &ret))
		return (-1);
	return (ret);
}
