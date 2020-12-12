#include <SD.h>
#include <SPI.h>
#include "sd_card.h"
#include "debug.h"

bool sd_card_init = false;

bool SD_init(int pin)
{
	if (!SD.begin(pin))
		return false;
	sd_card_init = 1;
	return true;
}

File SD_fopen(const char *fname, uint8_t mode)
{
	File f;
	switch (mode)
	{
	case FILE_READ:
		f = SD.open(fname, FILE_READ);
		break;
	case FILE_WRITE:
		f = SD.open(fname, FILE_WRITE);
		break;
	}

	return f;
}

void SD_fclose(File f)
{
	f.close();
}

void SD_fwrite(File f, const uint8_t *data, uint32_t size)
{
	f.write(data, size);
}

void SD_fread(File f, uint8_t *data, uint32_t size)
{
	f.read(data, size);
}

uint8_t SD_fread_byte(File f)
{
	return f.read();
}

bool SD_fexists(const char *fname)
{
	return SD.exists(fname);
}

void SD_fremove(const char *fname)
{
	SD.remove(fname);
}