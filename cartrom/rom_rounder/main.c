#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint32_t		UtilsGetPowerOfTwo(uint32_t size)
{
	uint32_t	i = 0;

	while ((1 << i) < size)
		i++;

	return (1 << i);
}

int      main(int argc, char *argv[])
{
	FILE		*f;
	uint8_t		*data;
	uint32_t	size, gap;

	printf("ROM rounder - Orion_ [2022]\nFill a file with 0xFF up to the next multiple of power of 2.\n\n");

	if (argc != (1+1))
	{
		printf("Usage: rom_rounder rom.bin\n");
		return (-1);
	}

	if ((f = fopen(argv[1], "a+b")))
	{
		fseek(f, 0, SEEK_END);
		size = ftell(f);

		gap = UtilsGetPowerOfTwo(size) - size;
		data = malloc(gap);
		memset(data, 0xFF, gap);
		fwrite(data, 1, gap, f);
		free(data);

		fclose(f);
	}
	else
	{
		printf("Cannot open file '%s'.\n", argv[1]);
		return (-1);
	}

	return (0);
}
