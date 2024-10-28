/*
* PS1USB Transfer tool v1.0
* by OrionSoft [2024]
* https://orionsoft.games/
*
* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
* If a copy of the MPL was not distributed with this file,
* You can obtain one at https://mozilla.org/MPL/2.0/.
*
* Linux version require libftdi1-dev
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include "lodepng.h"
#include "crc.h"

/****************************************/

#ifdef _WIN32
#include "ftd2xx.h"
FT_HANDLE	ftdi;

#define	ftdi_tcioflush	FT_ResetDevice
#define	ftdi_free		FT_Close

uint32_t	ftdi_read_data(FT_HANDLE handle, uint8_t *data, uint32_t size)
{
	DWORD	rbyte;

	if (FT_Read(handle, data, size, &rbyte) == FT_OK)
		return (rbyte);
	return (0);
}

uint32_t	ftdi_write_data(FT_HANDLE handle, uint8_t *data, uint32_t size)
{
	DWORD	rbyte;

	if (FT_Write(handle, data, size, &rbyte) == FT_OK)
		return (rbyte);
	return (0);
}

#else
#include <ftdi.h>
struct ftdi_context *ftdi = NULL;

#define	sleep_ms(ms)	usleep(ms * 1000)

#endif

/****************************************/

#define	PCDRV_MAX_FD	16

FILE	*PCdrvFD[PCDRV_MAX_FD];

/****************************************/

enum
{
	CMD_UNKNOWN,
	CMD_UPLOAD,
	CMD_DOWNLOAD,
	CMD_EXECUTE,
	CMD_FLASH,
	CMD_VRAM
};

/****************************************/

void	*SystemGetFile(char *filename, int *sizeptr)
{
	FILE	*f;
	int		size;
	void	*buffer = NULL;

	if (f = fopen(filename, "rb"))
	{
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (sizeptr)
			*sizeptr = size;
		if (buffer = malloc(size))
			fread(buffer, 1, size, f);
		else
			printf("Memory Allocation Error for file '%s' of size %d\n", filename, size);
		fclose(f);
	}
	else
		printf("File '%s' not found !\n", filename);

	return (buffer);
}

uint32_t	atohex(char *str)
{
	uint32_t	value = 0;

	if ((str[0] == '0') && (str[1] == 'x'))
		str = &str[2];
	while (42)
	{
		if (((*str) >= '0') && ((*str) <= '9'))
		{
			value <<= 4;
			value |= (*str) - '0';
		}
		else if (((*str) >= 'A') && ((*str) <= 'F'))
		{
			value <<= 4;
			value |= ((*str) - 'A') + 0xA;
		}
		else if (((*str) >= 'a') && ((*str) <= 'f'))
		{
			value <<= 4;
			value |= ((*str) - 'a') + 0xA;
		}
		else
			break;
		str++;
	}

	return (value);
}

/****************************************/

#define	USB_PACKET_SIZE	128		// max TX buffer size of FTDI USB chip
#define	MAX_PACKET_SIZE	2048	// 2k max for reliability

uint8_t	USB_cmd[1+4+4+1];

bool	USB_Process(char cmd, uint8_t *data, uint32_t adrs, uint32_t size)
{
	uint8_t		retry, ack, crc8, *odata;
	uint32_t	lsize, usize;

	USB_cmd[0] = cmd;
	USB_cmd[1] = (adrs >> 24) & 0xFF;
	USB_cmd[2] = (adrs >> 16) & 0xFF;
	USB_cmd[3] = (adrs >> 8) & 0xFF;
	USB_cmd[4] = adrs & 0xFF;
	USB_cmd[5] = (size >> 24) & 0xFF;
	USB_cmd[6] = (size >> 16) & 0xFF;
	USB_cmd[7] = (size >> 8) & 0xFF;
	USB_cmd[8] = size & 0xFF;
	USB_cmd[9] = CRC8_Compute(USB_cmd, 1+4+4);

	// Send Command Packet
	if (ftdi_write_data(ftdi, USB_cmd, 1+4+4+1) != 1+4+4+1)
		return (false);

	retry = 3;

	// Get Ack Packet
	if (ftdi_read_data(ftdi, &ack, 1) == 1)
	{
		if (ack == 'G')
		{
			if (cmd == 'D')
			{
				while (size)	// get all packets
				{
					printf("%d bytes remaining...\r", size);
					lsize = (size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : size;
					odata = data;
					// Get packet by small packet of usb tx buffer size
					while (lsize)
					{
						usize = (lsize > USB_PACKET_SIZE) ? USB_PACKET_SIZE : lsize;
						if (ftdi_read_data(ftdi, data, usize) != usize)
							return (false);
						data += usize;
						lsize -= usize;
					}
					// Check CRC
					lsize = (size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : size;
					if (ftdi_read_data(ftdi, &crc8, 1) == 1)
					{
						if (crc8 == CRC8_Compute(odata, lsize))
						{
							ack = 'N';	// next
							size -= lsize;
						}
						else
						{
							printf("\nretry...\n");
							ack = 'A';	// again
							//ftdi_tcioflush(ftdi);
							data = odata;
							retry--;
							if (!retry)
								return (false);
						}
					}
					else
						return (false);
					if (ftdi_write_data(ftdi, &ack, 1) != 1)
						return (false);
				}
			}
			else if (cmd != 'F')
			{
				while (size)	// get all packets
				{
					printf("%d bytes remaining...\r", size);
					lsize = (size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : size;
					odata = data;
					// Send packet by small packet of usb tx buffer size
					while (lsize)
					{
						usize = (lsize > USB_PACKET_SIZE) ? USB_PACKET_SIZE : lsize;
						if (ftdi_write_data(ftdi, data, usize) != usize)
							return (false);
						data += usize;
						lsize -= usize;
					}
					// Check CRC
					lsize = (size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : size;
					if (ftdi_read_data(ftdi, &crc8, 1) == 1)
					{
						if (crc8 == CRC8_Compute(odata, lsize))
						{
							ack = 'N';	// next
							size -= lsize;
						}
						else
						{
							printf("\nretry...\n");
							ack = 'A';	// again
							//ftdi_tcioflush(ftdi);
							data = odata;
							retry--;
							if (!retry)
								return (false);
						}
					}
					else
						return (false);
					if (ftdi_write_data(ftdi, &ack, 1) != 1)
						return (false);
				}
			}
			return (true);
		}
	}
	return (false);
}

/****************************************/
// PC File Server

void	PC_FileServer(void)
{
	uint8_t		*data, *ptr, answer, param[1+4+4+1];
	uint32_t	data_size, odata_size;
	uint32_t	fd;
	int			i;

	data = NULL;
	if (ftdi_read_data(ftdi, param, sizeof(param)) == sizeof(param))
	{
		if (param[1+4+4] == CRC8_Compute(param, 1+4+4))
			answer = 'G';	// Good ACK
		else
			answer = 'B';	// Bad command

		fd = (param[1] << 24) | (param[2] << 16) | (param[3] << 8) | param[4];
		odata_size = data_size = (param[5] << 24) | (param[6] << 16) | (param[7] << 8) | param[8];

		// Send ACK
		if (ftdi_write_data(ftdi, &answer, 1) != 1)
			goto pcdrvout;

		if (answer == 'G')
		{
			uint8_t		*odata, crc8;
			uint32_t	lsize, usize;
			int			ret_value = -1;

			if ((param[0] == 'O') || (param[0] == 'C') || (param[0] == 'R') || (param[0] == 'W'))
				ptr = data = malloc(data_size);

			// Read data
			if ((param[0] == 'O') || (param[0] == 'C') || (param[0] == 'W'))
			{
				while (data_size)
				{
					lsize = (data_size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : data_size;
					odata = ptr;
					while (lsize)
					{
						usize = (lsize > USB_PACKET_SIZE) ? USB_PACKET_SIZE : lsize;
						if (ftdi_read_data(ftdi, ptr, usize) != usize)
							goto pcdrvout;
						ptr += usize;
						lsize -= usize;
					}
					// Check CRC
					lsize = (data_size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : data_size;
					if (ftdi_read_data(ftdi, &crc8, 1) == 1)
					{
						if (crc8 == CRC8_Compute(odata, lsize))
						{
							answer = 'N';	// next
							data_size -= lsize;
						}
						else
						{
							answer = 'A';	// again
							ptr = odata;
						}
					}
					else
						goto pcdrvout;
					if (ftdi_write_data(ftdi, &answer, 1) != 1)
						goto pcdrvout;
				}
			}

			// Convert path
			if ((param[0] == 'O') || (param[0] == 'C'))
				for (i = 0; data[i]; i++)
					if (data[i] == '\\')
						data[i] = '/';

			switch (param[0])	// Process command
			{
				case 'I':	// Init
					ret_value = 0;
				break;

				case 'O':	// Open file
					for (i = 2; i < PCDRV_MAX_FD; i++)
					{
						if (!PCdrvFD[i])
						{
							PCdrvFD[i] = fopen(data, (fd == 0) ? "rb" : ((fd == 1) ? "wb" : "rb+"));
							if (PCdrvFD[i])
								ret_value = i;
							break;
						}
					}
				break;

				case 'C':	// Create file
					for (i = 2; i < PCDRV_MAX_FD; i++)
					{
						if (!PCdrvFD[i])
						{
							PCdrvFD[i] = fopen(data, "wb+");
							if (PCdrvFD[i])
								ret_value = i;
							break;
						}
					}
				break;

				case 'S':	// Seek file
					answer = (fd >> 24) & 0xFF;
					fd &= 0xFFFFFF;

					if (fd < PCDRV_MAX_FD)
					{
						if (PCdrvFD[fd])
						{
							fseek(PCdrvFD[fd], (int)data_size, (answer == 0) ? SEEK_SET : ((answer == 1) ? SEEK_CUR : SEEK_END));
							ret_value = ftell(PCdrvFD[fd]);
						}
					}
				break;

				case 'R':	// Read file
					if (fd < PCDRV_MAX_FD)
					{
						if (PCdrvFD[fd])
						{
							ret_value = fread(data, 1, data_size, PCdrvFD[fd]);

							// Send Data
							while (data_size)
							{
								lsize = (data_size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : data_size;
								odata = ptr;
								while (lsize)
								{
									usize = (lsize > USB_PACKET_SIZE) ? USB_PACKET_SIZE : lsize;
									if (ftdi_write_data(ftdi, ptr, usize) != usize)
										goto pcdrvout;
									ptr += usize;
									lsize -= usize;
								}
								// Check CRC
								lsize = (data_size > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : data_size;
								if (ftdi_read_data(ftdi, &crc8, 1) == 1)
								{
									if (crc8 == CRC8_Compute(odata, lsize))
									{
										answer = 'N';	// next
										data_size -= lsize;
									}
									else
									{
										answer = 'A';	// again
										ptr = odata;
									}
								}
								else
									goto pcdrvout;
								if (ftdi_write_data(ftdi, &answer, 1) != 1)
									goto pcdrvout;
							}
						}
					}
				break;

				case 'W':	// Write file
					if (fd < PCDRV_MAX_FD)
					{
						if (PCdrvFD[fd])
						{
							ret_value = fwrite(data, 1, odata_size, PCdrvFD[fd]);
						}
					}
				break;

				case 'c':	// Close file
					if (fd < PCDRV_MAX_FD)
					{
						if (PCdrvFD[fd])
						{
							ret_value = fclose(PCdrvFD[fd]);
							PCdrvFD[fd] = NULL;
						}
					}
				break;
			}

			// Send return value
			param[0] = (ret_value >> 24) & 0xFF;
			param[1] = (ret_value >> 16) & 0xFF;
			param[2] = (ret_value >> 8) & 0xFF;
			param[3] = ret_value & 0xFF;
			ftdi_write_data(ftdi, param, 4);
		}
	}
pcdrvout:
	if (data)
		free(data);
}

/****************************************/

int main(int argc, char *argv[])
{
	int			x, y, w, h, i, ret, cmd = CMD_UNKNOWN;
	uint32_t	size = 0, adrs = 0x80010000, *ptr32;
	uint16_t	*ptr16;
	uint8_t		*data = NULL, *ptr;
	char		*filename = NULL, *path = NULL;
	bool		ttymode = false, reset = false;

	printf("PS1USB Transfer tool v1.0 by OrionSoft [2024]\n\n");

#ifdef  _WIN32
	if (FT_Open(0, &ftdi) != FT_OK)
	{
		fprintf(stderr, "Unable to open USB device.\n");
		return EXIT_FAILURE;
	}
#else
	if ((ftdi = ftdi_new()) == 0)
	{
		fprintf(stderr, "ftdi_new failed\n");
		return EXIT_FAILURE;
	}

	if ((ret = ftdi_usb_open(ftdi, 0x0403, 0x6001)) < 0)
	{
		fprintf(stderr, "Unable to open USB device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		ftdi_free(ftdi);
		return EXIT_FAILURE;
	}
#endif

	ftdi_tcioflush(ftdi);

	// Handle arguments list
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
				case 'u':	// Upload file to specified address
					cmd = CMD_UPLOAD;
				break;

				case 'd':	// Download file from specified address and size
					cmd = CMD_DOWNLOAD;
				break;

				case 'e':	// Execute psexe file
					cmd = CMD_EXECUTE;
				break;

				case 'r':	// Reset the PS1
					reset = true;
				break;

				case 'w':	// Flash Action Replay
					cmd = CMD_FLASH;
				break;

				case 'c':	// TTY Console mode
					ttymode = true;
				break;

				case 'f':	// file
					filename = argv[i + 1];
					i++;
				break;

				case 'p':	// PCDRV path
					path = argv[i + 1];
					i++;
				break;

				case 'a':	// 0xAddress (hexadecimal address for download/upload)
					adrs = atohex(argv[i + 1]);
					i++;
				break;

				case 'v':	// VRAM x y w h
					if (argc >= i + 4)
					{
						x = atoi(argv[i + 1]);
						y = atoi(argv[i + 2]);
						w = atoi(argv[i + 3]);
						h = atoi(argv[i + 4]);
						i += 4;
						cmd = CMD_VRAM;

						if ((x < 0) || ((x+w) > 1024) || (y < 0) || ((y+h) > 512))
						{
							printf("VRAM coordinates must be within 1024x512 !\n");
							goto usage;
						}
					}
					else
					{
						printf("VRAM command needs x y w h parameters following -v\n");
						goto usage;
					}
				break;

				case 's':	// size (size of download in decimal)
					size = atoi(argv[i + 1]);
					if (size <= 0)
					{
						printf("Wrong size '%s'\n", argv[i + 1]);
						goto usage;
					}
					i++;
				break;
			}
		}
		else
			goto usage;
	}

	if (!filename && !reset)
	{
		printf("No file specified.\n");
		goto usage;
	}

	if ((cmd == CMD_UPLOAD) || (cmd == CMD_EXECUTE) || (cmd == CMD_FLASH))
	{
		data = SystemGetFile(filename, &size);
		if (!data)
			goto usage;
	}

	if (reset)
	{
		// Bug on Linux only (tested on Ubuntu 22.04 VM): sometimes an USB communication issue appears,
		// ps1transfer displays "USB error." then exits when trying to execute a ps-exe file just after
		// resetting (-r and -e options combined)
		// On Windows, it works fine, the data upload seems to wait that USB becomes ready after reset

		printf("Resetting the PS1...\n");
		if (!USB_Process('R', NULL, 0, 0))
#ifndef _WIN32
			if (!USB_Process('R', NULL, 0, 0))	// Try another time just in case but sometimes it still fails on Linux
#endif
				goto USB_error;

#ifndef _WIN32
		// Closing and opening again the USB device seems to help but not in every case
		if ((ret = ftdi_usb_close(ftdi)) < 0)
		{
			fprintf(stderr, "Unable to close USB device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
			ftdi_free(ftdi);
			return EXIT_FAILURE;
		}

		if ((ret = ftdi_usb_open(ftdi, 0x0403, 0x6001)) < 0)
		{
			fprintf(stderr, "Unable to open USB device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
			ftdi_free(ftdi);
			return EXIT_FAILURE;
		}

		// Wait some time for the USB communication to be ready again before executing a ps-exe file
		// Otherwise, next transfers fail
		if (cmd != CMD_UNKNOWN)
			sleep_ms(1000);
#endif
	}

	// Handle command
	ptr = data;
	switch (cmd)
	{
		case CMD_EXECUTE:
			ptr32 = (uint32_t*)&data[8+4+4+(2*4)];
			adrs = *ptr32;
			ptr = &data[2048];
			size -= 2048;
			printf("Uploading %u bytes to 0x%X...\n", size, adrs);
			if (USB_Process('U', ptr, adrs, size))
			{
				printf("\nExecuting...\n");
				if (!USB_Process('E', &data[8+4+4], 0, 15*4))
					goto USB_error;
			}
			else
				goto USB_error;
		break;

		case CMD_UPLOAD:
		case CMD_FLASH:
			printf("Uploading %u bytes to 0x%X...\n", size, adrs);
			if (USB_Process('U', ptr, adrs, size))
			{
				if (cmd == CMD_FLASH)
				{
					printf("Flashing Action Replay...\n");
					if (!USB_Process('F', NULL, adrs, (size+127)/128))
						goto USB_error;
				}
			}
			else
				goto USB_error;
		break;

		case CMD_DOWNLOAD:
			data = malloc(size);
			if (!data)
			{
				printf("Memory Allocation Error to download data of size %d\n", size);
				goto usage;
			}
			ptr = data;
			printf("Downloading %u bytes from 0x%X...\n", size, adrs);
			if (USB_Process('D', ptr, adrs, size))
			{
				FILE	*f;

				f = fopen(filename, "wb");
				fwrite(data, 1, size, f);
				fclose(f);
			}
			else
				goto USB_error;
		break;

		case CMD_VRAM:
			data = malloc(w*h*(sizeof(uint16_t)+3));
			if (!data)
			{
				printf("Memory Allocation Error to download VRAM image\n");
				goto usage;
			}
			ptr = data;
			printf("Downloading VRAM from coordinate %d,%d of size %d,%d...\n", x, y, w, h);
			// VRAM is saved at boot by PS1USB to RAM address 0x80020000
			for (i = 0; i < h; i++)
			{
				printf("Downloading line %d of %d...\n", i+1, h);
				adrs = 0x80020000 + ((((y + i) * 1024) + x) * sizeof(uint16_t));
				size = w * sizeof(uint16_t);
				if (USB_Process('D', ptr, adrs, size))
					ptr += size;
				else
					goto USB_error;
			}
			printf("Saving to PNG image: %s\n", filename);
			// Convert 16bits RAW data to PNG
			ptr16 = (uint16_t*)data;
			ptr = data + w*h*sizeof(uint16_t);
			for (i = 0; i < w * h; i++)
			{
				uint16_t	color = *ptr16++;
				uint8_t		r = color & 31;
				uint8_t		g = (color >> 5) & 31;
				uint8_t		b = (color >> 10) & 31;

				*ptr++ = ((r & 31) << (8-5)) | ((r & 31) >> (5-(8-5)));
				*ptr++ = ((g & 31) << (8-5)) | ((g & 31) >> (5-(8-5)));
				*ptr++ = ((b & 31) << (8-5)) | ((b & 31) >> (5-(8-5)));
			}
			lodepng_encode24_file(filename, data + w*h*sizeof(uint16_t), w, h);
		break;

		default:
			if (!reset)
				printf("No command specified.\n");
	}

	goto done;

USB_error:
	printf("\nUSB error.\n");
	goto done;

usage:
	if (ttymode)
		goto done;

	printf("\nUsage: ps1transfer parameters...\n");
	printf("\t-u Upload file to specified address\n");
	printf("\t-d Download file from specified address and size\n");
	printf("\t-v Download VRAM image from following parameters: x y w h\n");
	printf("\t-e Execute psexe file (use -r if a program is already running)\n");
	printf("\t-r Reset the PS1 (will be done first if combined with actions above)\n");
	printf("\t-f file.bin\n");
	printf("\t-a 0xAddress (hexadecimal address for download/upload)\n");
	printf("\t-s size (size of download in decimal)\n");
	printf("\t-c Switch to TTY console mode after executing command.\n");
	printf("\t-p path (indicate path to PCDRV file server, require -c)\n");
	printf("\nData are secured using CRC8.\n");

done:
	if (data)
		free(data);

	// Handle TTY Console mode
	memset(PCdrvFD, 0, sizeof(PCdrvFD));
	if (path)
		chdir(path);
	while (ttymode)
	{
		char	c;

		if (ftdi_read_data(ftdi, &c, 1) == 1)
		{
			// Data Link Escape (PCDRV mode)
			if (c == 16)
				PC_FileServer();
			else
				putchar(c);	// Just echo the received char
		}
	}

#ifndef _WIN32
	if ((ret = ftdi_usb_close(ftdi)) < 0)
	{
		fprintf(stderr, "Unable to close USB device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		ftdi_free(ftdi);
		return EXIT_FAILURE;
	}
#endif
    ftdi_free(ftdi);

	return (0);
}
