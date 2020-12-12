#include "eve_flash.h"
#include "sd_card.h"
#include "EVE_commands.h"
#include "EVE.h"
#include "EVE_config.h"
#include "debug.h"

#define BLOCK_SIZE              (64)
#define SECTOR_SIZE             (0x1000)
#define LINE_LIMIT              (35)
#define MEM_FLASH_READ          (0x000FE000)
#define FLASH_IMG               "output.bin"
#define BURN_FLAG               "buru.in"
#define RAM_G_WORKING           0x000FF000

map_info map_list[LIST_SIZE];

static uint32_t write_block_to_ram(uint32_t addr, uint8_t *data, uint8_t size)
{
	uint8_t index;
	uint32_t w_addr = addr;

	for (index = 0; index < size; index++)
	{
		//if (index && !(index % 16))
		//	LOG("\r\n");
		//LOG("%x ", data[index]);
		EVE_memWrite8(w_addr++, data[index]);
	}
	//LOG("\r\n");

	return (w_addr);
}

static void read_block_from_ram(uint32_t addr, uint8_t size)
{
	uint8_t data[BLOCK_SIZE];
	int i;
	uint32_t w_addr = addr;

	for (i = 0; i < size; i++)
	{
		if (i && !(i % 16))
			LOG("\r\n");
		LOG("%02x ", EVE_memRead8(addr++));
	}
	LOG("\r\n");
}

/*
 * fname: 文件名
 * flash_addr: flash的偏移地址，不是RAM_FLASH的地址
 */
static bool file_trans_to_flash(const char *fname, uint32_t flash_addr)
{
	uint8_t data[BLOCK_SIZE];
	uint32_t flash_size;
	uint32_t f_size;
	uint32_t f_block;
	uint32_t f_sectors;
	uint32_t available_sector;
	uint32_t blocks, bytes, s;
	File f;

	if (!SD_fexists(fname))
		return false;

	f = SD_fopen(fname, FILE_READ);
	if (!f)
		return false;

	EVE_cmd_flashattach();
	EVE_cmd_flashfast();

	/*将文件分成扇区*/
	f_size = f.size();
	f_sectors = f_size / SECTOR_SIZE;

	if (f_size % SECTOR_SIZE)
		f_sectors++;
	flash_size = EVE_get_flash_size();
	available_sector = (flash_size - (flash_addr - RAM_FLASH)) / BLOCK_SIZE;

	if (available_sector < f_sectors)
		return false;

	f_block = SECTOR_SIZE / BLOCK_SIZE;

	for (s = 0; s < f_sectors; s++)
	{
		for (blocks = 0; blocks < f_block; blocks++)
		{
			if (f_size > BLOCK_SIZE)
			{
				SD_fread(f, data, BLOCK_SIZE);
				f_size -= BLOCK_SIZE;
			}
			else
			{
				for (bytes = 0; bytes < BLOCK_SIZE; bytes++)
				{
					if (f_size)
					{
						data[bytes] = SD_fread_byte(f);
						f_size--;
					}
					else
						data[bytes] = 0xFF;
				}
			}

			/*将一个块的数据写入到内存中*/
			write_block_to_ram(RAM_G_WORKING + (blocks * BLOCK_SIZE), data, BLOCK_SIZE);
			//read_block_from_ram(RAM_G_WORKING + (blocks * BLOCK_SIZE), BLOCK_SIZE);
		}
		/*写完一个扇区的数据，将数据更新到flash*/
		LOG("flashupdate.....\r\n");
		EVE_cmd_flashupdate(flash_addr + (s * SECTOR_SIZE), RAM_G_WORKING, SECTOR_SIZE);
	}

	SD_fclose(f);
	return true;
}

bool EVE_load_flash(void)
{
	File f;
	/*当transfer.fin存在，说明已经将数据加载到eve的flash，不再需要加载数据过去*/
	if (SD_fexists(BURN_FLAG))
		return false;
	if (!file_trans_to_flash(FLASH_IMG, RAM_FLASH))
		return false;
	if (!(f = SD_fopen(BURN_FLAG, FILE_WRITE)))
		return false;
	SD_fclose(f);
	return true;
}

/*获取flash大小，以字节为单位*/
uint32_t EVE_get_flash_size(void)
{
	return (EVE_memRead32(REG_FLASH_SIZE) * 0x100000); // REG_FLASE_SIZE读到的是以M为单位数值
}

static void copy_to_map_list(map_info *ml, uint8_t *data)
{
	uint8_t cnt = 0;
	uint32_t temp = 0;
	uint8_t i = 0;

	while ((cnt < LINE_LIMIT) && (data[cnt++] != 0x20));
	memcpy(ml->name, data, cnt - 1);

	while ((cnt < LINE_LIMIT) && (data[cnt++] != 0x3A));
	while ((cnt < LINE_LIMIT) && (data[cnt++] == 0x20));

	cnt--;
	while (data[cnt] != 0x20)
	{
		temp *= 10;
		temp += (data[cnt] & 0x0F);
		cnt++;
	}

	ml->address = temp;
	temp = 0;

	while ((cnt < LINE_LIMIT) && (data[cnt++] == 0x20));
	while (data[cnt] != 0x00)
	{
		temp *= 10;
		temp += (data[cnt] & 0x0F);
		cnt++;
	}
	ml->size = temp;
}

bool EVE_flash_parse(map_info *ml, uint32_t ml_size)
{
	uint32_t i, byte_cnt = 0, cnt = 0;
	uint8_t tmp_buf[LINE_LIMIT];
	uint32_t data_addr = MEM_FLASH_READ;
	uint16_t index = 0;

	EVE_cmd_memzero(MEM_FLASH_READ, SECTOR_SIZE);
	EVE_cmd_flashread(data_addr, RAM_FLASH + 0x1000, 0x400);
	

	while (byte_cnt < LINE_LIMIT)
	{
		tmp_buf[byte_cnt] = EVE_memRead8(data_addr++);
		//LOG("%02x ", tmp_buf[byte_cnt]);
		//if (tmp_buf[byte_cnt] == 0x00)
		//	return false;

		if (tmp_buf[byte_cnt] == 0x0D)
		{
			if (EVE_memRead8(data_addr++) == 0x0A)
			{
				tmp_buf[byte_cnt] = 0x00;
				copy_to_map_list(&ml[index++], tmp_buf);
				if (index >= ml_size)
					break;
				byte_cnt = 0;
				memset(tmp_buf, 0, sizeof(tmp_buf));
				//LOG("\r\n");
				continue;
			}
			
		}
		
		byte_cnt++;
	}

	return true;
}

bool EVE_flash_attach(void)
{
	uint8_t ret;
	EVE_cmd_flashattach();

	if ((ret = EVE_memRead8(REG_FLASH_STATUS) != 0x02) || ret != 0x03)
		return false;
	return true;
}

bool load_file_exists(void)
{
	return SD_fexists(BURN_FLAG);
}
void load_file_del(void)
{
	SD_fremove(BURN_FLAG);
}

int8_t find_map_list(const char *str)
{
	uint8_t i;

	for (i = 0; i < LIST_SIZE; i++)
	{
		if (!strncmp(map_list[i].name, str, NAME_LEN))
			return i;
	}

	if (i == LIST_SIZE)
		return -1;
}