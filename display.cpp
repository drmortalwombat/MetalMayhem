#include "display.h"
#include <stdlib.h>
#include <c64/vic.h>
#include <c64/memmap.h>
#include <c64/sprites.h>
#include <oscar.h>

const char SpriteImages[] = {
	#embed spd_sprites lzo "tank.spd"
};

const char NumbersFont[] = {
	#embed ctm_chars lzo "numbers.ctm"
};

const char NumberTiles[] = {
	#embed ctm_tiles8 "numbers.ctm"
};

const char statx_score[2] = {1, 14};
const char statx_flag[2] = {7, 12};

RIRQCode		rirqtop, rirqbottom;

void display_init(void)
{
	// Swap in all RAM
	mmap_set(MMAP_RAM);

	oscar_expand_lzo(Charset, NumbersFont);
	oscar_expand_lzo(Sprites, SpriteImages);

	// Swap ROM back in
	mmap_set(MMAP_NO_ROM);


	// Background and border black
	vic.color_border = VCOL_DARK_GREY;
	vic.color_back = VCOL_BLACK;
	vic.spr_multi = 0x00;
	vic.spr_priority = 0xff;

	// Set VIC to show custom screen with custom charset
	vic_setmode(VICM_TEXT, Screen, Charset);

	rirq_init_io();

	vspr_init(Screen);

	rirq_sort();
	rirq_start();	
}

void stats_putchar(char x, char y, char ch)
{
	char * dp = Screen2 + 40 * 21 + 80 * y + 2 * x;
	dp[ 0] = NumberTiles[4 * ch + 0];
	dp[ 1] = NumberTiles[4 * ch + 1];
	dp[40] = NumberTiles[4 * ch + 2];
	dp[41] = NumberTiles[4 * ch + 3];
}


void clock_put_char(char x, char ch)
{
	char * dp = Screen2 + 40 * 23 + 2 * x;

	dp[ 0] = NumberTiles[4 * ch + 0];
	dp[ 1] = NumberTiles[4 * ch + 1];
	dp[40] = NumberTiles[4 * ch + 2];
	dp[41] = NumberTiles[4 * ch + 3];	
}


void display_game_frame(void)
{
	memset(Screen, 255, 40 * 21);
	memset(Color, VCOL_RED, 1000);

	for(char y=0; y<20; y++)
	{
		for(char x=18; x<22; x++)
			Color[40 * y + x] = VCOL_DARK_GREY;
	}

	memset(Color + 40 * 20, VCOL_DARK_GREY, 40 * 5);
	memset(Color + 40 * 21, VCOL_YELLOW, 20);
	memset(Color + 40 * 22, VCOL_YELLOW, 20);
	memset(Color + 40 * 21 + 20, VCOL_LT_BLUE, 20);
	memset(Color + 40 * 22 + 20, VCOL_LT_BLUE, 20);

	memset(Color + 40 * 23, VCOL_LT_GREY, 40 * 2);

	memset(Screen2 + 40 * 20, 255, 40);
	memset(Screen2 + 40 * 21, 0, 40 * 4);


	for(char i=0; i<8; i++)
		Screen2[0x3f8 + i] = 87;	

	vspr_set(0, 0, 0, 64, VCOL_YELLOW);
	vspr_set(1, 0, 0, 64, VCOL_LT_BLUE);
	vspr_set(2, 0, 0, 84, VCOL_YELLOW);
	vspr_set(3, 0, 0, 84, VCOL_LT_GREY);
	vspr_set(4, 0, 0, 80, VCOL_YELLOW);
	vspr_set(5, 0, 0, 80, VCOL_LT_BLUE);
	vspr_set(6, 0, 0, 85, VCOL_YELLOW);
	vspr_set(7, 0, 0, 85, VCOL_LT_BLUE);

	vspr_set(8, 0, 0, 64, VCOL_YELLOW);
	vspr_set(9, 0, 0, 64, VCOL_LT_BLUE);
	vspr_set(10, 0, 0, 84, VCOL_YELLOW);
	vspr_set(11, 0, 0, 84, VCOL_LT_GREY);
	vspr_set(12, 0, 0, 80, VCOL_YELLOW);
	vspr_set(13, 0, 0, 80, VCOL_LT_BLUE);
	vspr_set(14, 0, 0, 85, VCOL_YELLOW);
	vspr_set(15, 0, 0, 85, VCOL_LT_BLUE);	

	rirq_build(&rirqbottom, 2);
	rirq_write(&rirqbottom, 0, &vic.memptr, 0x12);
	rirq_write(&rirqbottom, 1, &vic.color_back, VCOL_DARK_GREY);
	rirq_set(9, 54 + 20 * 8, &rirqbottom);

	rirq_build(&rirqtop, 3);
	rirq_write(&rirqtop, 0, &vic.memptr, 0x02);
	rirq_write(&rirqtop, 1, &vic.color_back, VCOL_BLACK);
	rirq_call(&rirqtop, 2, irq_music);
	rirq_set(10, 40, &rirqtop);

	vspr_sort();
	vspr_update();
	rirq_sort();

	vic.color_border = VCOL_DARK_GREY;
	vic.color_back = VCOL_BLACK;
}
