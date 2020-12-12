#ifndef __TFT_H__
#define __TFT_H__
#include <stdint.h>

void tft_init(void);
void tft_display(void);
uint32_t tft_touch(void);

#endif