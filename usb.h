/*
* PS1USB firmware by OrionSoft [2024]
* USB packet transfer protocol with CRC8 integrity check
*
* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
* If a copy of the MPL was not distributed with this file,
* You can obtain one at https://mozilla.org/MPL/2.0/.
*/

static struct EXEC	exe_header;

#ifndef HOOK
typedef struct
{
	bool	is_enabled;
} KernelHookParams;

// Kernel hook params are defined at the end of available reserved kernel space
// The address can be decreased to get more room if needed
// The linker script in the kernel_hook folder would need to be adjusted too
static KernelHookParams	*kernel_hook_params = (KernelHookParams *)0x8000DF70;
#endif

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

/****************************************/

#define	USB_STATUS	*((volatile uint8_t *)0x1F060000)	// USB Status A17+A18
#define	USB_DATA	*((volatile uint8_t *)0x1F020000)	// USB Data A17

// Status bits are inverted (0 = Yes, 1 = No)
#define	USB_STATUS_MASK_HAVE_DATA		(1 << 0)	// FT245R: RXF
#define	USB_STATUS_MASK_CAN_SEND_DATA	(1 << 1)	// FT245R: TXE
#define	USB_STATUS_MASK_USB_READY		(1 << 2)	// FT245R: PWREN

#ifndef HOOK
#define	DCACHE_ADRS	0x1F800000

#define	Init_CRC8()			memcpy((volatile uint8_t *)DCACHE_ADRS, CRC8_Tab, sizeof(CRC8_Tab))
#define	Fast_CRC8(_crc_)	*((volatile uint8_t *)(DCACHE_ADRS+(_crc_)))
#else
#define	Fast_CRC8(_crc_)	*((volatile uint8_t *)(CRC8_Tab+(_crc_)))
#endif

// Decreased timeout value in hook mode to avoid freezing programs for some time
// trying to send printf messages if ps1transfer is not in console mode
#ifndef HOOK
#define	TIMEOUT_CNT		5000000
#else
#define	TIMEOUT_CNT		5000
#endif
#define	MAX_PACKET_SIZE	2048	// Please do not modify this value !

enum
{
	USB_STATE_NONE,
	USB_STATE_BAD_CMD,
	USB_STATE_BAD_CRC,
	USB_STATE_TIMEOUT,
	USB_STATE_OK
};

void	ResetPS1(void)
{
	((void (*)())0xBFC00000)();
}

bool	USB_GetByte(volatile uint8_t *data)
{
	uint32_t	timeout = TIMEOUT_CNT;

	if (USB_STATUS & USB_STATUS_MASK_USB_READY)
		return (false);

	while (USB_STATUS & USB_STATUS_MASK_HAVE_DATA)
		if (!(timeout--))
			return (false);

	*data = USB_DATA;
	return (true);
}

bool	USB_GetByteNoTimeout(volatile uint8_t *data)
{
	if (USB_STATUS & USB_STATUS_MASK_USB_READY)
		return (false);
	if (USB_STATUS & USB_STATUS_MASK_HAVE_DATA)
		return (false);
	*data = USB_DATA;
	return (true);
}

bool	USB_SendByte(uint8_t data)
{
	uint32_t	timeout = TIMEOUT_CNT;

	if (USB_STATUS & USB_STATUS_MASK_USB_READY)
		return (false);

	while (USB_STATUS & USB_STATUS_MASK_CAN_SEND_DATA)
		if (!(timeout--))
			return (false);

	USB_DATA = data;
	return (true);
}

int		USB_Process(void)
{
	uint8_t						crc, USB_cmd[1+4+4+1];
	register uint8_t			USB_index;
	register volatile uint8_t	*ptr, *optr;
	register uint32_t			size, lsize, olsize, osize;

	if (!USB_GetByteNoTimeout(&USB_cmd[0]))
		return (USB_STATE_NONE);

	// Check received command
#ifndef HOOK
	if (!((USB_cmd[0] == 'D') || (USB_cmd[0] == 'U') || (USB_cmd[0] == 'E') || (USB_cmd[0] == 'F') || (USB_cmd[0] == 'R')))
#else
	if (!(USB_cmd[0] == 'R'))
#endif
		return (USB_STATE_BAD_CMD);

	crc = Fast_CRC8(0xFF ^ USB_cmd[0]);
	USB_index = 1;

	// Receive command packet
	while (USB_GetByte(&USB_cmd[USB_index]))
	{
		if (USB_index == 1+4+4)
			break;
		else
			crc = Fast_CRC8(crc ^ USB_cmd[USB_index]);
		USB_index++;
	}
	if (USB_index != 1+4+4)
		return (USB_STATE_TIMEOUT);

	// Verify command packet integrity
	if (USB_cmd[1+4+4] != crc)
	{
		USB_SendByte('B');	// Bad
		return (USB_STATE_BAD_CRC);
	}

	if (!USB_SendByte('G'))	// Good
		return (USB_STATE_TIMEOUT);

#ifndef HOOK
	// Get Address pointer from packet
	optr = ptr = (volatile uint8_t *)((USB_cmd[1] << 24) | (USB_cmd[2] << 16) | (USB_cmd[3] << 8) | USB_cmd[4]);
	// Get Data size from packet
	osize = size = (USB_cmd[5] << 24) | (USB_cmd[6] << 16) | (USB_cmd[7] << 8) | USB_cmd[8];

	// Download data (PS1 to Computer)
	if (USB_cmd[0] == 'D')
	{
		while (size)
		{
			olsize = lsize = (size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : size;
			crc = 0xFF;
			while (lsize--)
			{
				crc = Fast_CRC8(crc ^ (*ptr));
				if (!USB_SendByte(*ptr++))
					return (USB_STATE_TIMEOUT);
			}
			if (!USB_SendByte(crc))	// Send CRC of data
				return (USB_STATE_TIMEOUT);
			if (!USB_GetByte(&crc))	// Wait ACK (Next block or retry ?)
				return (USB_STATE_TIMEOUT);
			if (crc == 'N')	// Next
				size -= olsize;
			else			// Rewind
				ptr -= olsize;
		}
	}
	else	// Upload data (Computer to PS1)
	{
		if (USB_cmd[0] == 'E')	// Execute command
			optr = ptr = (volatile uint8_t *)&exe_header;

		while (size)
		{
			olsize = lsize = (size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : size;
			crc = 0xFF;
			while (lsize--)
			{
				if (!USB_GetByte(ptr))
					return (USB_STATE_TIMEOUT);
				crc = Fast_CRC8(crc ^ (*ptr++));
			}
			if (!USB_SendByte(crc))	// Send CRC of data
				return (USB_STATE_TIMEOUT);
			if (!USB_GetByte(&crc))	// Wait ACK (Next block or retry ?)
				return (USB_STATE_TIMEOUT);
			if (crc == 'N')	// Next
				size -= olsize;
			else			// Rewind
				ptr -= olsize;
		}
	}

	if (USB_cmd[0] == 'E')	// Execute
	{
		PadStop();
		ResetGraph(0);
		StopCallback();

		kernel_hook_params->is_enabled = true;

		EnterCriticalSection();
		Exec(&exe_header, 1, NULL);
	}
#endif

	if (USB_cmd[0] == 'R')	// Reset
		ResetPS1();

	return (USB_STATE_OK);
}
