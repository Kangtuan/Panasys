#ifndef __SD_CARD_H__
#define __SD_CARD_H__
#include <stdint.h>
#include <SD.h>

extern bool sd_card_init;

bool SD_init(int pin);
File SD_fopen(const char *fname, uint8_t mode);
void SD_fclose(File f);
void SD_fwrite(File f, const uint8_t *data, uint32_t size);
void SD_fread(File f, uint8_t *data, uint32_t size);
bool SD_fexists(const char *fname);
uint8_t SD_fread_byte(File f);
void SD_fremove(const char *fname);

#endif