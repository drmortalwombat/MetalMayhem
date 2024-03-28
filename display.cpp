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

	rirq_init_io();

	vspr_init(Screen);

	rirq_build(&rirqtop, 1);
	rirq_write(&rirqtop, 0, &vic.color_back, VCOL_BLUE);
	rirq_set(9, 50, &rirqtop);

	rirq_build(&rirqbottom, 1);
	rirq_write(&rirqbottom, 0, &vic.color_back, VCOL_LT_BLUE);
	rirq_set(10, 250, &rirqbottom);

	rirq_sort();
	rirq_start();
	rirq_wait();

	char j = 16;
	int i = 0;
	while (i < 1600)
	{
		rirq_move(9, 50 + (i >> 4));
		rirq_move(10, 250 - (i >> 4));
		rirq_wait();
		rirq_sort();
		i += j;
		j++;
	}

	rirq_wait();

	rirq_clear(9);
	rirq_clear(10);
	vic.color_back = VCOL_LT_BLUE;
	rirq_sort();
	rirq_wait();

	memset(Screen, 0, 1000);
	vic_setmode(VICM_TEXT, Screen, Charset);

	vic.color_border = VCOL_BLUE;
	vic.color_back = VCOL_BLUE;
	rirq_sort();
	rirq_wait();
	vic.color_back = VCOL_BLACK;
	vic.color_border = VCOL_BLACK;
	rirq_sort();
	rirq_wait();

	// Background and border black

	vic.spr_multi = 0x00;
	vic.spr_priority = 0xff;

	memset(Screen2, 0, 1000);

	// Set VIC to show custom screen with custom charset
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
	memset(Screen2, 0, 40 * 20);

	rirq_sort();
	rirq_wait();

	rirq_build(&rirqbottom, 3);
	rirq_write(&rirqbottom, 0, &vic.memptr, 0x12);
	rirq_write(&rirqbottom, 1, &vic.color_back, VCOL_DARK_GREY);
	rirq_call(&rirqbottom, 2, irq_music);
	rirq_set(9, 54 + 20 * 8, &rirqbottom);

	rirq_build(&rirqtop, 3);
	rirq_write(&rirqtop, 0, &vic.memptr, 0x12);
	rirq_write(&rirqtop, 1, &vic.color_back, VCOL_DARK_GREY);
	rirq_write(&rirqtop, 2, &vic.ctrl2, VIC_CTRL2_CSEL);
	rirq_set(10, 40, &rirqtop);

	// move vsprite trigger up to allow for more early irq sort in NTSC
	rirq_move(8, 55 + 20 * 8);

	vspr_sort();
	vspr_update();
	rirq_sort();
	rirq_wait();

	memset(Color + 40 * 20, VCOL_DARK_GREY, 40);
	memset(Color + 40 * 21, VCOL_YELLOW, 20);
	memset(Color + 40 * 22, VCOL_ORANGE, 20);
	memset(Color + 40 * 21 + 20, VCOL_LT_BLUE, 20);
	memset(Color + 40 * 22 + 20, VCOL_BLUE, 20);

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

	vic.color_border = VCOL_DARK_GREY;

	memset(Screen, 255, 40 * 21);
	memset(Color, VCOL_RED, 40 * 20);

	for(char y=0; y<20; y++)
	{
		for(char x=18; x<22; x++)
			Color[40 * y + x] = VCOL_DARK_GREY;
	}

	vspr_sort();
	vspr_update();
	rirq_sort();
	rirq_wait();	
}

void display_unveil_frame(void)
{
	rirq_wait();
	rirq_data(&rirqtop, 0, 0x02);
	rirq_data(&rirqtop, 1, VCOL_BLACK);
	vspr_update();
	rirq_sort();
}
