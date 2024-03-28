#include "intro.h"
#include <oscar.h>
#include "display.h"
#include "levels.h"
#include "gamemusic.h"
#include <c64/sprites.h>
#include <c64/sid.h>

const char IntoSpriteImages[] = {
	#embed spd_sprites lzo "introtanks.spd"
};

void intro_tank(char i, int x, char y, char a)
{
	char si = i * 6;
	char sa = 128 + a * 6;

	char sc = i ? VCOL_YELLOW : VCOL_LT_BLUE;

	vspr_set(si + 0, x     , y     , sa + 0, sc);
	vspr_set(si + 1, x + 24, y     , sa + 1, sc);
	vspr_set(si + 2, x + 48, y     , sa + 2, sc);
	vspr_set(si + 3, x     , y + 21, sa + 3, sc);
	vspr_set(si + 4, x + 24, y + 21, sa + 4, sc);
	vspr_set(si + 5, x + 48, y + 21, sa + 5, sc);

}

void intro_char(char x, char y, char ch, char color1, char color2)
{
	char * dp = Screen + 40 * y + x;
	char * cp = Color + 40 * y + x;

	dp[ 0] = NumberTiles[4 * ch + 0];
	dp[ 1] = NumberTiles[4 * ch + 1];
	dp[40] = NumberTiles[4 * ch + 2];
	dp[41] = NumberTiles[4 * ch + 3];	

	cp[ 0] = color1;
	cp[ 1] = color1;
	cp[40] = color2;
	cp[41] = color2;
}

void intro_string(char x, char y, const char * str, char color1, char color2)
{
	while (*str)	
	{
		intro_char(x, y, *str, color1, color2);
		str++;
		x += 2;
	}
}

const char ScrollText[] = P" "
P" \x05***\x01 WELCOME TO OUR NEW 2024 \x04PLAYER\x01#\x02PLAYER\x01 OR \x04PLAYER\x01#\x02COMPUTER\x01 TANK BATTLE GAME."
P" \x05***\x01 MUSIC BY \x02CRISPS\x01, CODING AND GRAPHICS BY \x02DR.MORTAL WOMBAT."
P" \x05***\x01 CAPTURE THE ENEMY FLAG FOR \x051000$\x01, DESTROY THE ENEMY TANK FOR \x05200$\x01 OR COLLECT BOLTS FOR \x05100$\x01."
P" \x05***\x01 GREETINGS TO \x02JAZZCAT \x03ECTE \x04PHAZE101\x01 AND ALL OF THE \x05OSK\x01 GANG.";

char intro_scrx, intro_c0, intro_c1;
unsigned	intro_scrp;

void intro_scroll(void)
{
	intro_scrx++;
	rirq_data(&rirqbottom, 0, 7 - (intro_scrx & 7));
	if (!(intro_scrx & 7))
	{
		for(char i=0; i<39; i++)
		{
			Screen[23 * 40 + i] = Screen[23 * 40 + 1 + i];
			Screen[24 * 40 + i] = Screen[24 * 40 + 1 + i];
			Color[23 * 40 + i] = Color[23 * 40 + 1 + i];			
			Color[24 * 40 + i] = Color[24 * 40 + 1 + i];
		}

		char ch = ScrollText[intro_scrp];
		while (ch < 32)
		{
			switch (ch)
			{
				case 0:
					intro_scrp = 0;
					break;
				case 1:
					intro_c0 = VCOL_LT_GREY;
					intro_c1 = VCOL_MED_GREY;
					break;
				case 2:
					intro_c0 = VCOL_LT_BLUE;
					intro_c1 = VCOL_BLUE;
					break;
				case 3:
					intro_c0 = VCOL_LT_GREEN;
					intro_c1 = VCOL_GREEN;
					break;
				case 4:
					intro_c0 = VCOL_YELLOW;
					intro_c1 = VCOL_ORANGE;
					break;
				case 5:
					intro_c0 = VCOL_WHITE;
					intro_c1 = VCOL_LT_GREY;
					break;
			}

			ch = ScrollText[++intro_scrp];
		}

		Color[23 * 40 + 39] = intro_c0;
		Color[24 * 40 + 39] = intro_c1;

		if (intro_scrx & 8)
		{
			Screen[23 * 40 + 39] = NumberTiles[4 * ch + 1];
			Screen[24 * 40 + 39] = NumberTiles[4 * ch + 3];
			intro_scrp++;
		}
		else
		{
			Screen[23 * 40 + 39] = NumberTiles[4 * ch + 0];
			Screen[24 * 40 + 39] = NumberTiles[4 * ch + 2];
		}
	}
}

void intro_map(char mi)
{
	const LevelData	*	lvl = level_all[mi];

	for(char i=0; i<21; i++)
	{
		DynSprites[0 * 64 + 3 * i + 0] = lvl->map[i + 0 * 42];
		DynSprites[0 * 64 + 3 * i + 1] = lvl->map[i + 1 * 42];
		DynSprites[0 * 64 + 3 * i + 2] = lvl->map[i + 2 * 42];

		DynSprites[1 * 64 + 3 * i + 0] = lvl->map[i + 3 * 42];
		DynSprites[1 * 64 + 3 * i + 1] = lvl->map[i + 4 * 42];
		DynSprites[1 * 64 + 3 * i + 2] = lvl->map[i + 5 * 42];

		DynSprites[2 * 64 + 3 * i + 0] = lvl->map[i + 0 * 42 + 21];
		DynSprites[2 * 64 + 3 * i + 1] = lvl->map[i + 1 * 42 + 21];
		DynSprites[2 * 64 + 3 * i + 2] = lvl->map[i + 2 * 42 + 21];

		DynSprites[3 * 64 + 3 * i + 0] = lvl->map[i + 3 * 42 + 21];
		DynSprites[3 * 64 + 3 * i + 1] = lvl->map[i + 4 * 42 + 21];
		DynSprites[3 * 64 + 3 * i + 2] = lvl->map[i + 5 * 42 + 21];
	}

	vspr_set(12, 280, 142, 16, VCOL_ORANGE);
	vspr_set(13, 304, 142, 17, VCOL_ORANGE);
	vspr_set(14, 280, 163, 18, VCOL_ORANGE);
	vspr_set(15, 304, 163, 19, VCOL_ORANGE);
}

const LevelData * intro_select_level;
bool intro_select_players;

void intro_anim(void)
{
	signed char pjx = 0, pjy = 0;

	char tx = 6;

	for(;;)
	{		
		for(int i=-48; i<344; i++)
		{
			intro_tank(0, 296 - i,  54, 0 + ((i >> 2) & 3));
			intro_tank(1, i,       190, 4 + ((i >> 2) & 3));

			Color[40 * 7 + tx + 0] = VCOL_MED_GREY;
			Color[40 * 8 + tx + 0] = VCOL_DARK_GREY;

			if (tx + 1 < 34)
			{
				Color[40 * 7 + tx + 1] = VCOL_LT_GREY;
				Color[40 * 8 + tx + 1] = VCOL_MED_GREY;
			}

			if (tx + 2 < 34)
			{
				Color[40 * 7 + tx + 2] = VCOL_WHITE;
				Color[40 * 8 + tx + 2] = VCOL_LT_GREY;
			}

			if (tx + 3 < 34)
			{
				Color[40 * 7 + tx + 3] = VCOL_LT_GREY;
				Color[40 * 8 + tx + 3] = VCOL_MED_GREY;
			}

			tx = tx + 1;
			if (tx == 34)
				tx = 6;

			joy_poll(0);
			if (joyx[0])			
			{			
				if (!pjx)	
				{
					pjx = joyx[0];
					intro_select_level_index = (intro_select_level_index + joyx[0]) & 3;
					intro_map(intro_select_level_index);
				}
			}
			else
				pjx = 0;

			if (joyy[0] < 0)
			{
				if (!intro_select_players)
				{
					intro_select_players = true;
					intro_string(2, 15, P" PLAYER ", VCOL_LT_BLUE, VCOL_BLUE);
				}
			}
			else if (joyy[0] > 0)
			{
				if (intro_select_players)
				{
					intro_select_players = false;
					intro_string(2, 15, P"COMPUTER", VCOL_LT_BLUE, VCOL_BLUE);					
				}				
			}

			if (joyb[0])
				return;

			vspr_sort();

			intro_scroll();

			rirq_wait();

			vspr_update();
			rirq_sort();
		}	
	}

}

void intro_init(void)
{
	music_active = false;
	music_patch_voice3(true);
	music_init(TUNE_INTRO);
	music_active = true;

	memset(Screen2, 0, 1000);

	oscar_expand_lzo(IntroSprites, IntoSpriteImages);

	memset(Screen, 0, 1000);

	rirq_wait();

	rirq_build(&rirqbottom, 1);
	rirq_write(&rirqbottom, 0, &vic.ctrl2, 0x00);
	rirq_set(9, 50 + 22 * 8 + 7, &rirqbottom);

	rirq_build(&rirqtop, 2);
	rirq_write(&rirqtop, 0, &vic.ctrl2, 0x08);
	rirq_call(&rirqtop, 1, irq_music);
	rirq_set(10, 40, &rirqtop);

	vic_setmode(VICM_TEXT, Screen, Charset);	
	vic.color_back = VCOL_BLACK;
	vic.color_border = VCOL_BLACK;

	for(char i=0; i<16; i++)
		vspr_hide(i);

	intro_scrx = 0;
	intro_scrp = 0;

	intro_map(intro_select_level_index);

	intro_string(0, 7, P"*** METAL MAYHEM ***", VCOL_MED_GREY, VCOL_DARK_GREY);
	for(char i=0; i<3; i++)
	{
		Color[40 * 7 + 2 * i +  0] = VCOL_WHITE;
		Color[40 * 7 + 2 * i +  1] = VCOL_LT_GREY;
		Color[40 * 7 + 2 * i + 40] = VCOL_LT_GREY;

		Color[40 * 7 + 2 * i + 34] = VCOL_WHITE;
		Color[40 * 7 + 2 * i + 35] = VCOL_LT_GREY;
		Color[40 * 7 + 2 * i + 74] = VCOL_LT_GREY;
	}


	intro_string(4, 11, P"PLAYER", VCOL_YELLOW, VCOL_ORANGE);
	intro_string(8, 13, P"VS", VCOL_LT_GREY, VCOL_MED_GREY);
	if (intro_select_players)
		intro_string(2, 15, P" PLAYER ", VCOL_LT_BLUE, VCOL_BLUE);	
	else
		intro_string(2, 15, P"COMPUTER", VCOL_LT_BLUE, VCOL_BLUE);

	intro_char(0, 12, P'^', VCOL_LT_GREY, VCOL_MED_GREY);
	intro_char(0, 14, P'_', VCOL_LT_GREY, VCOL_MED_GREY);

	intro_char(30, 13, P'<', VCOL_LT_GREY, VCOL_MED_GREY);
	intro_char(38, 13, P'>', VCOL_LT_GREY, VCOL_MED_GREY);

	for (char i=0; i<20; i++)
	{
		intro_char(2 * i, 4, 20 + (rand() & 3), VCOL_BROWN, VCOL_BROWN);
		intro_char(2 * i, 21, 20 + (rand() & 3), VCOL_BROWN, VCOL_BROWN);
	}

	intro_anim();

	intro_select_level = level_all[intro_select_level_index];

	static const char bcolors[] = {VCOL_DARK_GREY, VCOL_MED_GREY, VCOL_LT_GREY, VCOL_YELLOW, VCOL_WHITE};

	for(char i=0; i<5; i++)
	{
		vic.color_border = bcolors[i];
		vic.color_back = bcolors[i];

		rirq_wait();
		vspr_update();
		rirq_sort();
	}

	rirq_clear(9);
	rirq_clear(10);
	for(char i=0; i<16; i++)
		vspr_hide(i);
	vspr_sort();
	rirq_wait();
	vspr_update();
	rirq_sort();
	vic.ctrl2 = 0x08;

	music_active = false;

	memset(Screen, 0, 1000);

	for(char i=5; i>0; i--)
	{
		vic.color_border = bcolors[i - 1];
		vic.color_back = bcolors[i - 1];

		sid.fmodevol = 3 * (i - 1);

		rirq_wait();
		vspr_update();
		rirq_sort();
	}

}

