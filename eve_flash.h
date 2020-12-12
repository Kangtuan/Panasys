#ifndef __EVE_FLASH_H__
#define __EVE_FLASH_H__
#include <stdint.h>

#define LIST_SIZE  (12)
#define NAME_LEN   (16)

typedef struct {
	char name[32];
	uint32_t address;
	uint16_t size_x;
	uint16_t size_y;
	uint32_t format;
}img_param;

typedef struct {
	char name[NAME_LEN];
	uint32_t address;
	uint32_t size;
}map_info;

extern map_info map_list[LIST_SIZE];
bool EVE_load_flash(void);
uint32_t EVE_get_flash_size(void);
bool EVE_flash_parse(map_info *ml, uint32_t ml_size);
bool EVE_flash_attach(void);
bool load_file_exists(void);
void load_file_del(void);
int8_t find_map_list(const char *str);

#endif