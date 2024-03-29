#include <string.h>
#include <stdlib.h>
#include <c64/vic.h>
#include <c64/memmap.h>
#include <c64/joystick.h>
#include <c64/sprites.h>
#include <c64/rasterirq.h>
#include <c64/cia.h>
#include <c64/sid.h>
#include <c64/keyboard.h>
#include <audio/sidfx.h>
#include <oscar.h>
#include <conio.h>
#include <math.h>
#include "levels.h"
#include "gamemusic.h"
#include "display.h"
#include "playfield.h"
#include "players.h"
#include "intro.h"
#include "gameover.h"

// Main region shortened to make space for music

#pragma heapsize(0)

#pragma region( main, 0x0a00, 0x9000, , , {code, data, bss, heap, stack} )

#pragma region( zeropage, 0x80, 0xfc, , , {} )

// Interrupt sound routine invokes music and effects

__interrupt void irq_music(void)
{
	music_play();
	sidfx_loop_2();
}

char	clock_cy, clock_sec;

// Show the clock gauge at the bottom, could use some optimization
void clock_show(void)
{
	// Left char
	clock_put_char(0, 1);	

	// Center chars
	for(char x=0; x<19; x++)
	{
		if (x < (clock_cy >> 3))
			clock_put_char(x + 1, 2);
		else if (x == (clock_cy >> 3))
			clock_put_char(x + 1, 5 + (clock_cy & 7));
		else
			clock_put_char(x + 1, 5);
	}

	// Right char
	clock_put_char(19, 4);
}

void level_init(const LevelData * level)
{
	level_active = level;

	back_init();
	display_game_frame();

	player_init_hq(PLAYER_0, level->x0 * 8 + 24, level->y0 * 8 + 50);
	player_init_hq(PLAYER_1, level->x1 * 8 + 24, level->y1 * 8 + 50);
	player_init_flag(PLAYER_0);
	player_init_flag(PLAYER_1);
	player_init(PLAYER_0);
	player_init(PLAYER_1);
	bolt_init();

	view_init(PLAYER_0);
	view_init(PLAYER_1);

	display_unveil_frame();
}

// Show centered status text in bottom line
void status_text(const char * text, char color)
{
	char n = 0;
	while (text[n])
		n++;
	char x = (20 - n) / 2;
	memset(Color + 23 * 40, color, 80);
	for(char i=0; i<x; i++)
		clock_put_char(i, ' ');
	for(char i=0; i<n; i++)
		clock_put_char(x + i, text[i]);
	for(char i=x+n; i<20; i++)
		clock_put_char(i, ' ');
}

// Level ready count down
void level_ready(void)
{
	for(char i=0; i<150; i++)
	{
		if (i == 0)
			status_text("READY", VCOL_YELLOW);
		else if (i == 50)
			status_text("SET",   VCOL_YELLOW);
		else if (i == 100)
			status_text("FIGHT", VCOL_WHITE);

		vspr_sort();
		rirq_wait();

		vspr_update();
		rirq_sort();

		view_move(PLAYER_0, i);
		view_move(PLAYER_1, i);
	}
}

int main(void)
{
	cia_init();

	// Check for video norm PAL or NTSC

	system_ntsc = true;
	vic_waitTop();
	vic_waitBottom();
	while (vic.ctrl1 & VIC_CTRL1_RST8)
	{
		if (vic.raster > 20)
		{
			system_ntsc = false;
			break;
		}
	}

	// Install memory trampoline to ensure system
	// keeps running when playing with the PLA
	mmap_trampoline();

	display_init();

	music_active = true;	
	intro_select_level_index = 0;
	intro_select_level = level_all[0];

	for(;;)
	{
		intro_init();

		// Init music based on selected level
		music_active = false;
		music_patch_voice3(false);
		switch (intro_select_level_index)
		{
		case 0:
			music_init(TUNE_GAME_1);
			break;
		case 1:
			music_init(TUNE_GAME_2);
			break;
		case 2:
			music_init(TUNE_GAME_3);
			break;
		case 3:
			music_init(TUNE_GAME_4);
			break;
		}
		music_active = true;

		// Init level
		level_init(intro_select_level);

		score_init();

		level_ready();

		sid.fmodevol = 15;

		// Clock color
		clock_cy = clock_sec = 0;
		memset(Color + 23 * 40, 0x01, 40);
		memset(Color + 24 * 40, 0x0f, 40);
		clock_show();

		// Main game loop until clock runs out.  Loop iteration is synchronized
		// with raster beam

		char	phase = 0;
		char	bphase = 0;
		while (clock_cy < 144)
		{
			// Advance sprites and scroll register

			view_move(PLAYER_0, phase);
			view_move(PLAYER_1, phase);

			// Sort multiplexed sprite

			vspr_sort();

			// Odd phase is used for clock and keyboard

			if (phase & 1)
			{
				clock_sec++;
				if (clock_sec == 50)
				{
					clock_cy++;
					clock_sec = 0;
					clock_show();
				}

				keyb_poll();

				if (keyb_key == (KSCAN_STOP | KSCAN_QUAL_DOWN))
					break;
			}

			phase++;

			// Wait for end of display area

			rirq_wait();

			vspr_update();
			rirq_sort();

			// Update displays for both players.  Use a phase offset of four
			// for the second player to ensure not both are scrolling the
			// char view at the same frame

			view_scroll(PLAYER_0, phase & 7);
			view_scroll(PLAYER_1, (phase + 4) & 7);

			view_redraw(PLAYER_0, phase & 7);
			view_redraw(PLAYER_1, (phase + 4) & 7);

			// Animate flag and bolt

			char flgani = 80 + ((phase >> 2) & 3); 
			char boltani = 96 + (bphase >> 2);
			bphase++;
			if (bphase == 24)
				bphase = 0;

			vspr_image(4, flgani);
			vspr_image(5, flgani);
			vspr_image(12, flgani);
			vspr_image(13, flgani);

			vspr_image(3, boltani);
			vspr_image(11, boltani);

			// Check collisions
			player_check(PLAYER_0);
			player_check(PLAYER_1);
			collision_check();

			// Control players by joystick or AI
			player_control(PLAYER_0);
			if (intro_select_players)
				player_control(PLAYER_1);

	//		player_ai(PLAYER_0);
			if (!intro_select_players)
				player_ai(PLAYER_1);


	//		vic.color_border = VCOL_LT_BLUE;
			player_move(PLAYER_0);
			player_move(PLAYER_1);
	//		vic.color_border = VCOL_DARK_GREY;

			// Update score display
			player_check_score(PLAYER_0, phase);
			player_check_score(PLAYER_1, phase);
		}

		level_complete();
	}

	return 0;
}
