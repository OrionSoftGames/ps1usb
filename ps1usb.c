/*
* PS1USB firmware v1.1
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
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "display.h"
#include "usb.h"
#include "tfs.h"
#include "ttyhook.h"
#include "exhandler.h"

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

int main(void)
{
	int		size;
	bool	usb_ok;
	RECT	vram = {0, 0, 1024, 512};
	u_long	pad;

	ResetCallback();
	PadInit(0);
	InitDisplay();

	InstallExceptionHandler();

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
	PrintConsole("PS1 USB dev cartridge : firmware v1.1");
	PrintConsole("by OrionSoft [2024]");
	PrintConsole("             https://orionsoft.games/");
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
		_96_init();
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
