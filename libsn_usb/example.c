/*
* PS1USB libsn_usb use example by OrionSoft [2024]
* This example shows how to use the PS1USB file server feature using the provided libsn_usb.c/.h
* The function "DataManager_Load" will load a file either from CDROM or from your computer over USB depending on the CDROM_RELEASE define.
* The command for the tool to act as a file server is: ps1transfer -e -f ps1usb_example.ps-exe -c -p .
* "-p ." is indicating the local path where the files are stored
*
* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
* If a copy of the MPL was not distributed with this file,
* You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include <sys/types.h>
#include <libetc.h>
#include <libapi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

// Uncomment this define for a CDROM release, otherwise the file will be loaded from your computer over USB
//#define	CDROM_RELEASE

#ifdef	CDROM_RELEASE
#include <libcd.h>
#define	CD_LOAD_RETRY	10
#else
#include "libsn_usb.h"
#define	PC_READ_SIZE	2048
#endif

int	DataManager_Load(char *filename, unsigned char *buffer)
{
#ifdef CDROM_RELEASE
	int		i, cnt;
	CdlFILE	file;
	char	flname[64];

	sprintf(flname, "\\%s;1", filename);
	filename = flname;

	printf("Loading '%s' from CD_ROM\n", filename);

	// ALWAYS UPPER CASE FILENAME FOR CD LOADING !!
	for (i = 0; i < strlen(flname); i++)
		if ((flname[i] >= 'a') && (flname[i] <= 'z'))
			flname[i] -= 'a' - 'A';

	for (i = 0; i < CD_LOAD_RETRY; i++)
	{
		if (CdSearchFile(&file, filename) != 0)
			break;
	}
	if (i == CD_LOAD_RETRY)
	{
		printf("File not found '%s'\n", filename);
		return (-1);
	}

	for (i = 0; i < CD_LOAD_RETRY; i++)
	{
		CdReadFile(filename, (u_long*)buffer, file.size);

		while ((cnt = CdReadSync(1, 0)) > 0)
			VSync(0);

		if (cnt == 0)
			break;
	}
	if (i == CD_LOAD_RETRY)
	{
		printf("Error while reading file '%s'\n", filename);
		return (-1);
	}

	return (file.size);
#else
	int		i, file, size;

	printf("Loading '%s' from PC\n", filename);

	file = PCopen(filename, 0, 0);

	if (file <= 0)
	{
		printf("File not found '%s'\n", filename);
		return (-1);
	}

	size = PClseek(file, 0, 2);
	PClseek(file, 0, 0);

	i = size;
	while (i > PC_READ_SIZE)
	{
		PCread(file, buffer, PC_READ_SIZE);
		buffer += PC_READ_SIZE;
		i -= PC_READ_SIZE;
	}
	PCread(file, buffer, i);
	PCclose(file);

	return (size);
#endif
}

/****************************************/

uint8_t	CD_sector[2048];	// Buffer must be multiple of 2048 whenever loading data from CD

int main(void)
{
	ResetCallback();

	printf("PS1USB libsn_usb example by OrionSoft [2024]\n\n");
#ifdef	CDROM_RELEASE
	CdInit();
#else
	PCinit();
#endif

	if (DataManager_Load("test.txt", CD_sector))
		printf(CD_sector);
	else
		printf("Error loading data !\n");

	while (1);

    return (0);
}
