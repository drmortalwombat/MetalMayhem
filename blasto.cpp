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

#pragma region( zeropage, 0x80, 0xfc, , , {} )

__interrupt void irq_music(void)
{
	music_play();
	sidfx_loop_2();
}

char	clock_cy, clock_sec;

void clock_show(void)
{
	clock_put_char(0, 1);	
	for(char x=0; x<19; x++)
	{
		if (x < (clock_cy >> 3))
			clock_put_char(x + 1, 2);
		else if (x == (clock_cy >> 3))
			clock_put_char(x + 1, 5 + (clock_cy & 7));
		else
			clock_put_char(x + 1, 5);
	}
	clock_put_char(19, 4);
}

void level_init(const LevelData * level)
{
	level_active = level;

	back_init();

	player_init_hq(PLAYER_0, level->x0 * 8 + 24, level->y0 * 8 + 50);
	player_init_hq(PLAYER_1, level->x1 * 8 + 24, level->y1 * 8 + 50);
	player_init_flag(PLAYER_0);
	player_init_flag(PLAYER_1);
	player_init(PLAYER_0);
	player_init(PLAYER_1);
	bolt_init();

	display_game_frame();

	view_init(PLAYER_0);
	view_init(PLAYER_1);
}

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

void level_ready(void)
{
	status_text("READY", VCOL_YELLOW);
	for(char i=0; i<50; i++)
		vic_waitFrame();
	status_text("SET",   VCOL_YELLOW);
	for(char i=0; i<50; i++)
		vic_waitFrame();
	status_text("FIGHT", VCOL_WHITE);
	for(char i=0; i<50; i++)
		vic_waitFrame();
}

void level_complete(void)
{
	while (players[0].dscore || players[1].dscore)
	{
		player_check_score(PLAYER_0, 255);
		player_check_score(PLAYER_1, 255);		
		vic_waitFrame();
	}

	char i = 0;
	while (i < 5 && score[0][i] == score[1][i])
		i++;

	if (i == 5)
	{
		status_text("GAME OVER", VCOL_LT_GREY);
	}
	else if (score[0][i] > score[1][i])
	{
		status_text("YELLOW TANK WINS", VCOL_YELLOW);
	}
	else
	{
		status_text("BLUE TANK WINS", VCOL_LT_BLUE);
	}

	for(char i=0; i<200; i++)
		vic_waitFrame();
}

int main(void)
{
	cia_init();

	// Install memory trampoline to ensure system
	// keeps running when playing with the PLA
	mmap_trampoline();

	display_init();

	music_active = true;	
	music_init(TUNE_GAME_3);
	music_patch_voice3(false);

	for(;;)
	{
		intro_init();


		level_init(intro_select_level);

		score_init();

		level_ready();

		sid.fmodevol = 15;

		clock_cy = clock_sec = 0;
		clock_show();

		char	phase = 0;
		char	bphase = 0;
		while (clock_cy < 152)
		{
			view_move(PLAYER_0, phase);
			view_move(PLAYER_1, phase);

			vspr_sort();

			if (phase & 1)
			{
				clock_sec++;
				if (clock_sec == 5)
				{
					clock_cy++;
					clock_sec = 0;
					clock_show();
				}

				keyb_poll();

				if (keyb_key == (KSCAN_STOP | KSCAN_QUAL_DOWN))
				{
					clock_cy = clock_sec = 0;
					clock_show();
					player_init_flag(PLAYER_0);
					player_init_flag(PLAYER_1);
					player_init(PLAYER_0);
					player_init(PLAYER_1);
					bolt_init();

					score_init();
				}
			}

			phase++;

			rirq_wait();

			vspr_update();
			rirq_sort();

			view_scroll(PLAYER_0, phase & 7);
			view_scroll(PLAYER_1, (phase + 4) & 7);

			view_redraw(PLAYER_0, phase & 7);
			view_redraw(PLAYER_1, (phase + 4) & 7);

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

			player_check(PLAYER_0);
			player_check(PLAYER_1);
			collision_check();

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

			player_check_score(PLAYER_0, phase);
			player_check_score(PLAYER_1, phase);
		}

		level_complete();
	}

	return 0;
}
