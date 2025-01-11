#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include <libetc.h>
#include <libapi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

uint16_t	chip_id;

#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	240

void	InitDisplay(void)
{
	DISPENV	disp;
	RECT	screen = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

	ResetGraph(0);
	SetDefDispEnv(&disp, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	if (*((char *)0xbfc7ff52) == 'E')	// Check BIOS Video Mode (J/A -> NTSC, E -> PAL)
	{
		SetVideoMode(MODE_PAL);
		disp.screen.y += 16;
	}
	SetDispMask(1);
	PutDispEnv(&disp);
	ClearImage(&screen, 255, 255, 255);
}

void	LoadTexture(u_long *tim)
{
	TIM_IMAGE	tparam;

	OpenTIM(tim);
	ReadTIM(&tparam);
	LoadImage(tparam.prect, tparam.paddr);	// Load Image in VRAM
	DrawSync(0);
}

/****************************************/

#define	FONT_U					(1024-256)
#define	FONT_V					0
#define	FONT_CHAR_W				8
#define	FONT_CHAR_H				8
#define	FONT_N_CHAR_PER_LINE	32	// Must be multiple of power of 2
#define	FONT_N_FONT_LINE		3	// Must be multiple of power of 2
#define	FONT_FIRST_CHAR			'!'

void	DrawText(int x, int y, char *text)
{
	RECT	ch;

	ch.w = FONT_CHAR_W;
	ch.h = FONT_CHAR_H;
	while (*text)
	{
		char	c = (*text++) - FONT_FIRST_CHAR;

		if ((c >= 0) && (c < (FONT_N_CHAR_PER_LINE*FONT_N_FONT_LINE)))
		{
			ch.x = FONT_U + ((c & (FONT_N_CHAR_PER_LINE-1)) * FONT_CHAR_W);
			ch.y = FONT_V + ((c / FONT_N_CHAR_PER_LINE) * FONT_CHAR_H);
			MoveImage(&ch, x, y);
		}
		x += FONT_CHAR_W;
	}
	DrawSync(0);
}

void	htoa32(uint32_t n, char *strbuffer)
{
	uint8_t	i = 8;

	strbuffer = &strbuffer[8];
	*strbuffer-- = '\0';
	while (i--)
	{
		*strbuffer-- = ((n & 0xF) < 10) ? (n & 0xF) + '0' : ((n & 0xF) - 10) + 'A';
		n >>= 4;
	}
}

/****************************************/

extern u_long _binary_font_tim_start[];
extern uint8_t _binary_ps1usb_rom_start[];

char	strbuffer[16];

int main(void)
{
	int	i, size, y = 0;
	volatile uint8_t *arptr;
	uint8_t *ptr;

	ResetCallback();
	InitDisplay();
	LoadTexture(_binary_font_tim_start);

	DrawText(0, y += 8, "PS1USB Cartridge Flasher");
	DrawText(0, y += 8, "                  by OrionSoft [2024]");
	DrawText(0, y += 8, "Checking Flash Chip ID...");

	EnterCriticalSection();
	*((volatile uint8_t *)0x1F005555) = 0xAA;	// Enter ID report
	*((volatile uint8_t *)0x1F002AAA) = 0x55;
	*((volatile uint8_t *)0x1F005555) = 0x90;
	VSync(0);
	chip_id = *((volatile uint16_t *)0x1F000000);	// Read Chip ID
	*((volatile uint8_t *)0x1F005555) = 0xAA;	// Exit ID report
	*((volatile uint8_t *)0x1F002AAA) = 0x55;
	*((volatile uint8_t *)0x1F005555) = 0xF0;
	ExitCriticalSection();

	if (chip_id != 0xD5BF)	// Verify that we have a SST39VF010 chip
	{
		DrawText(0, y += 8, "Flash Chip ID is Wrong !");
		while (1);
	}

	DrawText(0, y += 8, "Erasing Chip...");

	EnterCriticalSection();
	*((volatile uint8_t *)0x1F005555) = 0xAA;	// Chip Erase
	*((volatile uint8_t *)0x1F002AAA) = 0x55;
	*((volatile uint8_t *)0x1F005555) = 0x80;
	*((volatile uint8_t *)0x1F005555) = 0xAA;
	*((volatile uint8_t *)0x1F002AAA) = 0x55;
	*((volatile uint8_t *)0x1F005555) = 0x10;
	ExitCriticalSection();
	for (i = 0; i < 10; i++)	// at least 100 ms
		VSync(0);

	DrawText(0, y += 8, "Flashing Chip...");

	// Program Chip by page of 128 bytes
	arptr = (volatile uint8_t *)0x1F000000;
	ptr = _binary_ps1usb_rom_start;
	size = 128*1024;
	EnterCriticalSection();
	SetRCnt(RCntCNT2, 10000, RCntMdNOINTR);
	while (size > 0)
	{
		*((volatile uint8_t *)0x1F005555) = 0xAA;
		*((volatile uint8_t *)0x1F002AAA) = 0x55;
		*((volatile uint8_t *)0x1F005555) = 0xA0;
		*arptr++ = *ptr++;

		// Byte program time = 20 us
		// RCntCNT2 = SysClock / 8 = 4233600Hz
		ResetRCnt(RCntCNT2);
		while (GetRCnt(RCntCNT2) < 100);

		size--;
	}
	ExitCriticalSection();

	DrawText(0, y += 8, "Flashing Done !");
	DrawText(0, y += 8, "Verifying...");

	arptr = (volatile uint8_t *)0x1F000000;
	ptr = _binary_ps1usb_rom_start;
	size = 128*1024;
	while (size--)
	{
		if (*arptr++ != *ptr++)
		{
			DrawText(0, y += 8, "Flash Error !");
			htoa32(((uint32_t)arptr)-1, strbuffer);
			DrawText(0, y += 8, strbuffer);
			while (1);
		}
	}
	DrawText(0, y += 8, "Flashing is Good !");

	while (1);

    return (0);
}
