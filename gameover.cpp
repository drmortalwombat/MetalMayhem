#include "gameover.h"
#include "gamemusic.h"
#include "display.h"
#include "playfield.h"
#include "players.h"
#include <c64/vic.h>
#include <c64/joystick.h>
#include <c64/sprites.h>
#include <c64/rasterirq.h>
#include <c64/memmap.h>
#include <math.h>
#include "intro.h"

void gover_spr_expand(char si)
{
	const char * sp = IntroSprites + 64 * si;
	char * dp = Screen + 55;

	for(char y=0; y<21; y+=2)
	{
		for(char x=0; x<3; x++)
		{
			char c0 = sp[3 * y + x + 0];
			char c1 = sp[3 * y + x + 3];

			for(char xi=0; xi<4; xi++)
			{
				char ci = (c0 & 3) | ((c1 & 3) << 2);
				c0 >>= 2;
				c1 >>= 2;

				dp[x * 4 + (3 - xi)] = ci | 0xe0;
			}			
		}

		dp += 40;
	}

	for(char x=0; x<3; x++)
	{
		char c0 = sp[3 * 20 + x + 0];

		for(char xi=0; xi<4; xi++)
		{
			char ci = (c0 & 3);
			c0 >>= 2;

			dp[x * 4 + (3 - xi)] = ci | 0xe0;
		}			
	}
}

inline void map_prep_char(char c, char m)
{
	static char imgs[4] = {0x00, 0x0f, 0xf0, 0xff};

	char	*	p = DynCharset + c;

	m = imgs[m];
	p[0] = m;
	p[1] = m;
	p[2] = m;
	p[3] = m;
}

void gover_char(char x, char y, char ch, char color1, char color2)
{
	char * dp = Screen2 + 40 * y + x;
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

void gover_string(char x, char y, const char * str, char color1, char color2)
{
	while (*str)	
	{
		gover_char(x, y, *str, color1, color2);
		str++;
		x += 2;
	}
}


const signed char sintab64[64] = {
	#for(i, 64) (signed char)floor(sin(i * PI / 32) * 45.0 + 0.5),
};

void gover_spr_loop(char p, char c)
{
	for(char i=0; i<8; i++)
	{
		vspr_set(i    ,  75 + sintab64[(p + i * 8    ) & 63], 120 + sintab64[(p + i * 8 + 16) & 63], 64 + (((p + i * 8 + 50) >> 2) & 15), c);
		vspr_set(i + 8, 268 + sintab64[(p + i * 8 + 4) & 63], 120 + sintab64[(p + i * 8 + 20) & 63], 64 + (((p + i * 8 + 54) >> 2) & 15), c);
	}
}

void level_complete(void)
{
	music_active = false;
	sidfx_stop(2);
	music_patch_voice3(true);
	music_init(TUNE_GAME_OVER);
	music_active = true;

	while (players[0].dscore || players[1].dscore)
	{
		player_check_score(PLAYER_0, 255);
		player_check_score(PLAYER_1, 255);		
		vic_waitFrame();
	}

	char	tcolor = VCOL_LT_GREY;

	char i = 0;
	while (i < 5 && score[0][i] == score[1][i])
		i++;

	oscar_expand_lzo(IntroSprites, SpriteImages);

	rirq_clear(11);

	for(char i=0; i<16; i++)
		vspr_hide(i);
	vspr_sort();
	rirq_wait();

	vspr_update();
	rirq_sort();

	for(char i=0; i<16; i++)
	{
		map_prep_char(8 * i + 0, i & 3);
		map_prep_char(8 * i + 4, i >> 2);
	}

	gover_spr_expand(16);

	memset(Screen, 0xe0, 20 * 40);
	memset(Screen2 + 40 * 23, 0, 80);

	gover_string(3, 23, "*** GAME OVER ***", VCOL_WHITE, VCOL_LT_GREY);
	if (i == 5)
	{
		memset(Color, VCOL_LT_GREY, 20 * 40);
	}
	else if (score[0][i] > score[1][i])
	{
		tcolor = VCOL_YELLOW;
		memset(Color, VCOL_YELLOW, 20 * 40);
		intro_string(4, 17, "YELLOW TANK WINS", VCOL_YELLOW, VCOL_ORANGE);
	}
	else
	{
		tcolor = VCOL_LT_BLUE;
		memset(Color, VCOL_LT_BLUE, 20 * 40);
		intro_string(6, 17, "BLUE TANK WINS", VCOL_LT_BLUE, VCOL_BLUE);
	}

	char t = 0;

	joy_poll(0);
	bool	bd = joyb[0];

	while (bd || !joyb[0])	
	{
		if (t & 1)
			gover_spr_expand(16 + ((t >> 2) & 3));
		else
		{
			gover_spr_loop(t >> 1, tcolor);
			vspr_sort();
		}
		t++;
		rirq_wait();

		vspr_update();
		rirq_sort();

		bd = joyb[0];
		joy_poll(0);
	}


	for(char i=0; i<16; i++)
		vspr_hide(i);

	vspr_sort();
	rirq_wait();

	vspr_update();
	rirq_sort();

	while (joyb[0])	
	{
		vic_waitFrame();
		joy_poll(0);
	}
}

