#ifndef __TFT_DATA_H__
#define __TFT_DATA_H__

#include <stdint.h>
#if	defined (__AVR__)
#include <avr/pgmspace.h>
#else
#define PROGMEM
#endif

#define BLOCK           (1024)
extern const uint8_t logo[2 * BLOCK];
extern const uint8_t logo1[239] PROGMEM;
extern const uint8_t pic1[4847] PROGMEM;
extern const uint8_t pic[3844] PROGMEM;
extern const uint8_t pana_logo[16565] PROGMEM;
#endif