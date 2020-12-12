
#include "EVE.h"
#include "tft.h"
#include "sd_card.h"
#include "debug.h"
#include "eve_flash.h"

#define SD_CS      (10)

static bool app_run = true;
static File f;

static void print_map_info(map_info *ml, uint8_t size)
{
	uint8_t i;

	for (i = 0; i < size; i++)
	{
		LOG("name: %s \taddr: %lu \tsize: %lu\r\n", ml[i].name, ml[i].address, ml[i].size);
	}

	while (1);
}

static void global_init(void)
{
	digitalWrite(EVE_CS, HIGH);
	pinMode(EVE_CS, OUTPUT);
	digitalWrite(EVE_PDN, HIGH);
	pinMode(EVE_PDN, OUTPUT);
	//pinMode(EVE_INT, INPUT);

	Serial.begin(115200);
	SPI.begin();
	SPI.setClockDivider(SPI_CLOCK_DIV2);
}

void setup()
{
	char buf[] = "Hello World";
	
	global_init();
	if (!SD_init(SD_CS))
	{
		Serial.println("SD_init Failed");
		app_run = false;
		return;
	}

	if (EVE_init() != 1)
	{
		LOG("EVE_init failed\r\n");
		app_run = false;
		return;
	}

	if (!EVE_flash_attach())
	{
		app_run = false;
		return;
	}

	EVE_load_flash();
	if (!EVE_flash_parse(map_list, LIST_SIZE))
	{
		app_run = false;
		LOG("EVE_flash_parse failed\r\n");
		return;
	}

	tft_init();
	//print_map_info(map_list, LIST_SIZE);
}

void loop()
{
	uint32_t start, current;

	start = current = 0;
	if (!app_run)
	{
		return;
	}

	while (1)
	{
		current = millis();
		if ((current - start) > 16)
		{
			tft_display();
			start = current;
		}
	}
}
