#include "tft.h"
#include "EVE.h"
#include "EVE_commands.h"
#include "tft_data.h"
#include "eve_flash.h"
#include "debug.h"

#define MEM_LOGO        0x000F8000
#define MEM_PIC1        0x000FA000
#define MEM_DL_STATIC   (EVE_RAM_G_SIZE - 4096)

#define RED		0xff0000UL
#define ORANGE	0xffa500UL
#define GREEN	0x00ff00UL
#define BLUE	0x0000ffUL
#define BLUE_1	0x5dade2L
#define YELLOW	0xffff00UL
#define PINK	0xff00ffUL
#define PURPLE	0x800080UL
#define WHITE	0xffffffUL
#define BLACK	0x000000UL

#define LAYOUT_Y1       (66)

#define align4(n)     ((((n) + 4) - 1) & (~(3)))

static uint32_t num_dl_size = 0;
static uint32_t tft_active = 0;
static uint32_t b_color = WHITE;
static const uint32_t color_list[9] = {RED, ORANGE, GREEN, BLUE, BLUE_1, YELLOW, PINK, PURPLE, WHITE};
static uint8_t color_index = 0;

static void touch_calibrate(void)
{
	EVE_memWrite32(REG_TOUCH_TRANSFORM_A, 0x000107f9);
	EVE_memWrite32(REG_TOUCH_TRANSFORM_B, 0xffffff8c);
	EVE_memWrite32(REG_TOUCH_TRANSFORM_C, 0xfff451ae);
	EVE_memWrite32(REG_TOUCH_TRANSFORM_D, 0x000000d2);
	EVE_memWrite32(REG_TOUCH_TRANSFORM_E, 0x0000feac);
	EVE_memWrite32(REG_TOUCH_TRANSFORM_F, 0xfffcfaaf);
}

static void init_background(void)
{
	EVE_cmd_dl(CMD_DLSTART);       // 启动显示列表
	EVE_cmd_dl(TAG(0));            // 不适用一下对象进行触摸测试
	EVE_cmd_bgcolor(0x00C0C0C0);   // 背景颜色设置为浅灰色
	EVE_cmd_dl(VERTEX_FORMAT(0));  // 降低VERTEX2F的精度

	/*用矩形画顶部*/
	EVE_cmd_dl(DL_BEGIN | EVE_RECTS);
	EVE_cmd_dl(LINE_WIDTH(1 * 16));   // 大小是1/16的像素
	EVE_cmd_dl(DL_COLOR_RGB | BLUE_1);
	EVE_cmd_dl(VERTEX2F(0, 0));
	EVE_cmd_dl(VERTEX2F(EVE_HSIZE, LAYOUT_Y1 - 2));
	EVE_cmd_dl(DL_END);

	/*显示logo*/
	EVE_cmd_dl(DL_COLOR_RGB | BLUE_1);
	EVE_cmd_dl(DL_BEGIN | EVE_BITMAPS);
	//EVE_cmd_setbitmap(MEM_LOGO, EVE_ARGB1555, 56, 56);
	EVE_cmd_setbitmap(MEM_PIC1, EVE_RGB565, 100, 50);
	EVE_cmd_dl(VERTEX2F(EVE_HSIZE - 100, 5));
	EVE_cmd_dl(DL_END);

	/*画一条黑线将顶部分开*/
	EVE_cmd_dl(DL_COLOR_RGB | BLACK);
	EVE_cmd_dl(DL_BEGIN | EVE_LINES);
	EVE_cmd_dl(VERTEX2F(0, LAYOUT_Y1 - 2));
	EVE_cmd_dl(VERTEX2F(EVE_HSIZE, LAYOUT_Y1 - 2));
	EVE_cmd_dl(DL_END);

	EVE_cmd_text(EVE_HSIZE - 89, EVE_VSIZE - 37, 30, 0, ":");
	EVE_cmd_text(EVE_HSIZE - 47, EVE_VSIZE - 37, 30, 0, ":");

	while (EVE_busy());

	/*把以上的命令保存到指定的位置*/
	num_dl_size = EVE_memRead16(REG_CMD_DL);
	EVE_cmd_memcpy(MEM_DL_STATIC, EVE_RAM_DL, num_dl_size);
	while (EVE_busy());
}

static uint32_t current, start = 0;
static void touch_handler(void)
{
	current = millis();
	if (current - start < 100)
		return;
	start = current;
	uint32_t point = EVE_memRead32(REG_TOUCH_SCREEN_XY);
	uint16_t x, y;
	uint16_t w0, w1, h0, h1;

	w0 = 5;
	w1 = 80;
	h0 = 5;
	h1 = 75;

	x = (point >> 16) & 0xFFFF;
	y = point & 0xFFFF;

	if ((x > w0 || x < w1) && (h0 > 5 || y < h1))
	{
		b_color = color_list[color_index++];
		if (color_index >= 9)
			color_index = 0;
	}
}

static void EVE_interrupts(void)
{
	uint8_t flags = EVE_memRead8(REG_INT_FLAGS);

	switch (flags & 0x02)
	{
	case 0x02:
		touch_handler();
	default:
		break;
	}
}

static void conversion_time(char *h, char *m, char *s, uint32_t ms)
{
	uint32_t hour, minute, second;
	uint32_t remainder_time;
	uint32_t base = 1000;
	uint32_t foctor = 60;
	
	second = (ms / base) % foctor;
	minute = (ms / (base * foctor)) % foctor;
	hour = (ms / (base * foctor * foctor));
	sprintf(h, "%02d", hour);
	sprintf(m, "%02d", minute);
	sprintf(s, "%02d", second);
}

void tft_init(void)
{
	int8_t index, i;

	index = find_map_list("pana_logo.jpg");
	if (index > 0)
	{
		tft_active = 1;
		EVE_memWrite8(REG_PWM_DUTY, 0x20);
		EVE_memWrite8(REG_INT_EN, 0x01);
		EVE_memWrite8(REG_INT_MASK, 0x02);

		//attachInterrupt(digitalPinToInterrupt(EVE_INT), EVE_interrupts, LOW);
		touch_calibrate();
		/*加载数据到图片都RAM*/
		//EVE_cmd_inflate(MEM_LOGO, logo1, sizeof(logo1));
		EVE_cmd_flashsource(RAM_FLASH + map_list[index].address);
		EVE_cmd_loadimage(MEM_PIC1, EVE_OPT_NODL, pana_logo, sizeof(pana_logo));
		init_background();
	}
}

void tft_display(void)
{
	uint16_t dl_size;
	
	if (tft_active)
	{
		dl_size = EVE_memRead16(REG_CMD_DL);
		EVE_start_cmd_burst();
		EVE_cmd_dl_burst(CMD_DLSTART);
		EVE_cmd_dl_burst(DL_CLEAR_RGB | b_color);
		EVE_cmd_dl_burst(DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG);
		EVE_cmd_dl_burst(TAG(0));

		/*get memory picture, and add button*/
		EVE_cmd_append_burst(MEM_DL_STATIC, num_dl_size);
		EVE_cmd_dl_burst(DL_COLOR_RGB | WHITE);
		EVE_cmd_fgcolor_burst(0x00C0C0C0);
		EVE_cmd_dl_burst(TAG(0));
		EVE_cmd_button_burst(20, 20, 50, 30, 28, 0, "Next");
		EVE_cmd_dl_burst(TAG(0));

		/*show image*/
		EVE_cmd_dl_burst(DL_COLOR_RGB | b_color);
		EVE_cmd_setbitmap_burst(MEM_PIC1, EVE_RGB565, 100, 50);
		EVE_cmd_dl_burst(CMD_LOADIDENTITY);
		EVE_cmd_translate_burst(65536 * 100, 65536 * 100);
		EVE_cmd_rotate_burst(0);
		EVE_cmd_translate_burst(65536 * -100, 65536 * -100);
		EVE_cmd_dl_burst(CMD_SETMATRIX);
		EVE_cmd_dl_burst(DL_BEGIN | EVE_BITMAPS);
		EVE_cmd_dl_burst(VERTEX2F(EVE_HSIZE - 400, (LAYOUT_Y1 + 50)));
		EVE_cmd_dl_burst(DL_END);

		/*获取当前时间*/
		uint32_t ms = millis();
		char h[3], m[3], s[3];
		conversion_time(h, m, s, ms);
		EVE_cmd_dl_burst(DL_COLOR_RGB | BLACK);
		EVE_cmd_text_burst(EVE_HSIZE - 125, EVE_VSIZE - 35, 30, 0, h);
		EVE_cmd_text_burst(EVE_HSIZE - 83, EVE_VSIZE - 35, 30, 0, m);
		EVE_cmd_text_burst(EVE_HSIZE - 40, EVE_VSIZE - 35, 30, 0, s);

		EVE_cmd_dl_burst(DL_DISPLAY);
		EVE_cmd_dl_burst(CMD_SWAP);
		EVE_end_cmd_burst();
		touch_handler();
	}
}
