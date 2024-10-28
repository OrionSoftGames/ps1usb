/*
* PS1USB firmware v1.2
* by OrionSoft [2024]
* https://orionsoft.games/
*
* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
* If a copy of the MPL was not distributed with this file,
* You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include <libetc.h>
#include <libapi.h>
#include <libcd.h>
#include <libsnd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "display.h"
#include "usb.h"
#include "tfs.h"

#define KERNEL_UNUSED_START	0x8000C000
#define KERNEL_UNUSED_SIZE	0x2000
#define KERNEL_HOOK_START	(KERNEL_UNUSED_START + 0x160)

extern uint8_t	_binary_kernel_hook_bin_start[];
extern uint32_t	*_binary_kernel_hook_bin_size;

// Locations are hardcoded at the end of unused kernel space to make sure the original function addresses are valid
// That way, the hooks can be uninstalled when testing a new PS1USB exe with new hooks (for cart development purposes)
static uint32_t	*p_UnresolvedExceptionOld = (uint32_t *)0x8000DF78;
static uint32_t	*p_ReturnFromExceptionOld = (uint32_t *)0x8000DF7C;

/****************************************/

bool	PrintUSBStatus(void)
{
	if (USB_STATUS & USB_STATUS_MASK_USB_READY)
	{
		PrintConsole("USB socket unplugged !");
		return (false);
	}
	PrintConsole("USB connected !");
	PrintConsole("Waiting commands from your computer !");
	return (true);
}

/****************************************/
// I couldn't make this work

char	*cd_region_check[] = {"for Eur", "for U/C", "for NET"};
char	*cd_region_unlock[] = {"(Europe)", "of America", "World wide"};

bool	CD_Unlock(void)
{
	uint8_t	getid_cmd[] = {0x22,0,0,0,0};
	uint8_t	result[16];
	int		i;

	CdControl(0x19, getid_cmd, result);
	CdSync(0, NULL);

	for (i = 0; i < 3; i++)
		if (!strncmp(result, cd_region_check[i], 7))
			break;
	if (i == 3)
	{
		PrintConsole("Error: CD Drive not compatible...");
		return (false);
	}

	CdControl(0x50, NULL, NULL);
	CdSync(0, NULL);
	CdControl(0x51, "Licensed by", NULL);
	CdSync(0, NULL);
	CdControl(0x52, "Sony", NULL);
	CdSync(0, NULL);
	CdControl(0x53, "Computer", NULL);
	CdSync(0, NULL);
	CdControl(0x54, "Entertainment", NULL);
	CdSync(0, NULL);
	CdControl(0x55, cd_region_unlock[i], NULL);
	CdSync(0, NULL);
	CdControl(0x56, NULL, NULL);
	CdSync(0, NULL);

	return (true);
}

/****************************************/

static inline void	**GetB0Table(void)
{
	register volatile int n asm("t1") = 0x57;
	__asm__ volatile("" : "=r"(n) : "r"(n));
	return ((void **(*)(void))0xB0)();
}

void	KernelHooksUninstall(void)
{
	void	**a0table = (void **)0x200;
	void	**b0table = GetB0Table();

	// Addresses of original functions are less than 0x1000
	// Addresses of hooked functions are in range KERNEL_UNUSED_START...KERNEL_UNUSED_START+KERNEL_UNUSED_SIZE
	if ((uint32_t) a0table[0x40] >= KERNEL_UNUSED_START && (uint32_t) a0table[0x40] < KERNEL_UNUSED_START + KERNEL_UNUSED_SIZE)
	{
		a0table[0x40] = (void *) *p_UnresolvedExceptionOld;
		printf("Kernel hooks: Old UnresolvedException uninstalled\n");
	}

	if ((uint32_t) b0table[0x17] >= KERNEL_UNUSED_START && (uint32_t) b0table[0x17] < KERNEL_UNUSED_START + KERNEL_UNUSED_SIZE)
	{
		b0table[0x17] = (void *) *p_ReturnFromExceptionOld;
		printf("Kernel hooks: Old ReturnFromException uninstalled\n");
	}
}

void	KernelHooksInstall(void)
{
	// Copy kernel hook installer to unused/reserved kernel space and launch it
	memcpy((volatile uint8_t *)KERNEL_HOOK_START, _binary_kernel_hook_bin_start, &_binary_kernel_hook_bin_size);
	((void (*)())KERNEL_HOOK_START)();
	printf("Kernel hooks: Installed\n");
}

int main(void)
{
	int		size;
	bool	usb_ok;
	RECT	vram = {0, 0, 1024, 512};
	u_long	pad;

	ResetCallback();
	PadInit(0);
	InitDisplay();
	SsInit();

	// Uninstall kernel hooks to prevent crashes due to overwriting if
	// PS1USB firmware is ran from RAM after being booted from ROM
	// (normally useful for PS1USB cartridge development only)
	KernelHooksUninstall();

	KernelHooksInstall();

	// Save entire VRAM in RAM at boot (so the computer can download VRAM data)
	StoreImage(&vram, (u_long*)0x80020000);
	DrawSync(0);
	VSync(0);

	vram.w = SCREEN_WIDTH;
	vram.h = SCREEN_HEIGHT;
	ClearImage(&vram, 255, 255, 255);

	// Copy image data from ROM to RAM, because LoadImage doesn't work when loading directly from ROM ?!
	memcpy((volatile uint8_t *)0x80010000, ROM_GetFile("ps1usb.tim", &size), size);
	LoadTexture((u_long*)0x80010000);
	memcpy((volatile uint8_t *)0x80010000, ROM_GetFile("orion.tim", &size), size);
	LoadTexture((u_long*)0x80010000);
	memcpy((volatile uint8_t *)0x80010000, ROM_GetFile("soft.tim", &size), size);
	LoadTexture((u_long*)0x80010000);
	memcpy((volatile uint8_t *)0x80010000, ROM_GetFile("font.tim", &size), size);
	LoadTexture((u_long*)0x80010000);

	InitConsole();
	PrintConsole("PS1 USB dev cartridge : firmware v1.2");
	PrintConsole("by OrionSoft [2024]");
	PrintConsole("             https://orionsoft.games/");

	// Enable CD IRQs
	// Otherwise they are not enabled after CdInit() which will hang
	// and timeout the first CD command but they will be enabled afterwards
	// Intended behaviour? Or bug?
	//
	// For registers details and alternative implementation, see:
	// https://problemkaputt.de/psx-spx.htm#cdromcontrollerioports
	// https://github.com/Lameguy64/PSn00bSDK/blob/702bb601fb5712e2ae962a34b89204c646fe98f5/libpsn00b/psxcd/common.c#L217
	*((volatile uint8_t *)0x1F801800) = 1;	// Index Register
	*((volatile uint8_t *)0x1F801803) = 7;	// Interrupt Flag Register
	*((volatile uint8_t *)0x1F801802) = 7;	// Interrupt Enable Register

	pad = PadRead(1);
	if (pad & (PADRdown | PADRright))	// X = Init CD
	{
/*
		// I couldn't make this work
		if (pad & PADRright)			// O = Unlock CD
		{
			PrintConsole("Unlocking CD drive...");
			CdInit();
			CD_Unlock();
		}
*/
		PrintConsole("Initializing CD...");
		CdInit();
		PrintConsole("CD ready!");
	}

	usb_ok = PrintUSBStatus();

	Init_CRC8();	// Copy CRC8 lookup table in Data Cache
	while (1)
	{
		int		ret;

		// Handle USB cable plug/unplug status
		if (!usb_ok)
		{
			if (!(USB_STATUS & USB_STATUS_MASK_USB_READY))
				usb_ok = PrintUSBStatus();
		}
		else
		{
			if (USB_STATUS & USB_STATUS_MASK_USB_READY)
				usb_ok = PrintUSBStatus();
		}

/*		// This is only to test the ExceptionHandler crash screen
		if (pad & PADRdown)
		{
			asm("lui $v0,0x8001");
			asm("addiu $v0,0x0001");
			asm("lw $v0,0($v0)");
		}
*/

		if (usb_ok)
		{
			ret = USB_Process();

			switch (ret)
			{
				case USB_STATE_BAD_CRC:
					PrintConsole("BAD Command CRC !");
				break;

				case USB_STATE_TIMEOUT:
					PrintConsole("Timeout !");
				break;

				case USB_STATE_OK:
					PrintConsole("Transfer Successful !");
				break;
			}
		}
    }

    return (0);
}
