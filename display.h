/*
* PS1USB firmware by OrionSoft [2024]
* Minimal display functions using only Load/MoveImage to minimize memory footprint
*
* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
* If a copy of the MPL was not distributed with this file,
* You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	240

void	InitDisplay(void)
{
	DISPENV	disp;

	ResetGraph(0);
	SetDefDispEnv(&disp, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	if (*((char *)0xbfc7ff52) == 'E')	// Check BIOS Video Mode (J/A -> NTSC, E -> PAL)
	{
		SetVideoMode(MODE_PAL);
		disp.screen.y += 16;
	}
	SetDispMask(1);
	PutDispEnv(&disp);
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

/****************************************/

#define	CONSOLE_WIDTH		(SCREEN_WIDTH-(FONT_CHAR_W*2))
#define	CONSOLE_HEIGHT		96
#define	CONSOLE_N_CHAR		(CONSOLE_WIDTH/FONT_CHAR_W)
#define	CONSOLE_N_LINE		(CONSOLE_HEIGHT/FONT_CHAR_H)
#define	CONSOLE_OFFSET_X	((SCREEN_WIDTH-CONSOLE_WIDTH)/2)
#define	CONSOLE_OFFSET_Y	65

int		ConsoleCurLine;

void	InitConsole(void)
{
	ConsoleCurLine = CONSOLE_OFFSET_Y;
}

void	PrintConsole(char *text)
{
	if (ConsoleCurLine >= (CONSOLE_OFFSET_Y + CONSOLE_HEIGHT))
	{
		RECT	scroll = {CONSOLE_OFFSET_X, CONSOLE_OFFSET_Y + FONT_CHAR_H, CONSOLE_WIDTH, CONSOLE_HEIGHT - FONT_CHAR_H};
		RECT	line = {CONSOLE_OFFSET_X, CONSOLE_OFFSET_Y + ((CONSOLE_N_LINE - 1) * FONT_CHAR_H), CONSOLE_WIDTH, FONT_CHAR_H};

		// Scroll one line up
		MoveImage(&scroll, CONSOLE_OFFSET_X, CONSOLE_OFFSET_Y);
		// Clear last line
		ClearImage(&line, 255, 255, 255);
		ConsoleCurLine -= FONT_CHAR_H;
	}
	DrawText(CONSOLE_OFFSET_X, ConsoleCurLine, text);
	ConsoleCurLine += FONT_CHAR_H;
}
