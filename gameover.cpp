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

// Expand a sprite into character data, used for the animated flag in
// the game over screen.  Each sprite pixel is blown up to four screen
// pixel, so each character on screen is based on a 2x2 pixel square
// of the sprite
void gover_spr_expand(char si)
{
	// Source address of the sprite to expand
	const char * sp = IntroSprites + 64 * si;

	// Screen destination address
	char * dp = Screen + 55;

	// Loop over the first 20 rows of the sprite in two row
	// increments

	for(char y=0; y<21; y+=2)
	{
		// Loop over the three bytes per row

		for(char x=0; x<3; x++)
		{
			// Get the bytes of the two consecutive rows
			char c0 = sp[3 * y + x + 0];
			char c1 = sp[3 * y + x + 3];

			// Loop over four pixel pairs
			for(char xi=0; xi<4; xi++)
			{
				// Calculate the character index
				char ci = (c0 & 3) | ((c1 & 3) << 2);

				// Advance to next pixel pair
				c0 >>= 2;
				c1 >>= 2;

				// Put char on screen
				dp[x * 4 + (3 - xi)] = ci | 0xe0;
			}			
		}

		// Next screen row
		dp += 40;
	}

	// Final row of the sprite
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

// Prepare four rows of one of the 16 chars that are used
// in the sprite blow up
inline void map_prep_char(char c, char m)
{
	static char imgs[4] = {0x00, 0x0f, 0xf0, 0xff};

	// Get row data
	char	*	p = DynCharset + c;

	m = imgs[m];

	// Fill four consecutive charset rows
	p[0] = m;
	p[1] = m;
	p[2] = m;
	p[3] = m;
}

// Write a 16x16 pixel character on screen composed of four base
// chars with different colors for upper and lower half
void gover_char(char x, char y, char ch, char color1, char color2)
{
	// Display address
	char * dp = Screen2 + 40 * y + x;
	char * cp = Color + 40 * y + x;

	// Write chars based on tile index map
	dp[ 0] = NumberTiles[4 * ch + 0];
	dp[ 1] = NumberTiles[4 * ch + 1];
	dp[40] = NumberTiles[4 * ch + 2];
	dp[41] = NumberTiles[4 * ch + 3];	

	// Write color
	cp[ 0] = color1;
	cp[ 1] = color1;
	cp[40] = color2;
	cp[41] = color2;
}

// Write a zero terminated string of 16x16 chars on screen
void gover_string(char x, char y, const char * str, char color1, char color2)
{
	// Loop over the chars of the string
	while (*str)	
	{
		// Write one character
		gover_char(x, y, *str, color1, color2);

		// Advance
		str++;
		x += 2;
	}
}

// Integer sin table for circular movement of tank sprites
const signed char sintab64[64] = {
	#for(i, 64) (signed char)floor(sin(i * PI / 32) * 45.0 + 0.5),
};


// Update position and image of the tanks in the two circles
void gover_spr_loop(char p, char c)
{
	// Loop over 8 sprites
	for(char i=0; i<8; i++)
	{
		vspr_set(i    ,  75 + sintab64[(p + i * 8    ) & 63], 120 + sintab64[(p + i * 8 + 16) & 63], 64 + (((p + i * 8 + 50) >> 2) & 15), c);
		vspr_set(i + 8, 268 + sintab64[(p + i * 8 + 4) & 63], 120 + sintab64[(p + i * 8 + 20) & 63], 64 + (((p + i * 8 + 54) >> 2) & 15), c);
	}
}

void level_complete(void)
{
	// Switch to game over tune, ensure no more sound effects and three channels
	music_active = false;
	sidfx_stop(2);
	music_patch_voice3(true);
	music_init(TUNE_GAME_OVER);
	music_active = true;

	// Increment score display for pending score counts
	while (players[0].dscore || players[1].dscore)
	{
		player_check_score(PLAYER_0, 255);
		player_check_score(PLAYER_1, 255);		
		vic_waitFrame();
	}

	char	tcolor = VCOL_LT_GREY;

	// Find winner
	char i = 0;
	while (i < 5 && score[0][i] == score[1][i])
		i++;

	// Load intro sprites into sprite image region
	oscar_expand_lzo(IntroSprites, SpriteImages);

	rirq_clear(11);

	// Hide all game sprites
	for(char i=0; i<16; i++)
		vspr_hide(i);
	vspr_sort();
	rirq_wait();

	vspr_update();
	rirq_sort();

	// Prepare char matrix for enlarged flag display
	for(char i=0; i<16; i++)
	{
		map_prep_char(8 * i + 0, i & 3);
		map_prep_char(8 * i + 4, i >> 2);
	}

	gover_spr_expand(16);

	// Clear playfield on screen
	memset(Screen, 0xe0, 20 * 40);
	memset(Screen2 + 40 * 23, 0, 80);

	// Display game over and winner message	
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

	// Animation counter
	char t = 0;

	// Initial joystick button state
	joy_poll(0);
	bool	bd = joyb[0];

	// Wait until button goes from not pressed to pressed
	while (bd || !joyb[0])	
	{
		// In odd frames expand flag, in even frames advance sprites
		if (t & 1)
			gover_spr_expand(16 + ((t >> 2) & 3));
		else
		{
			gover_spr_loop(t >> 1, tcolor);
			vspr_sort();
		}
		t++;

		// Update sprite multiplexer
		rirq_wait();

		vspr_update();
		rirq_sort();

		// Update joystick state
		bd = joyb[0];
		joy_poll(0);
	}


	// Hide all sprites
	for(char i=0; i<16; i++)
		vspr_hide(i);

	vspr_sort();
	rirq_wait();

	vspr_update();
	rirq_sort();

	// Wait for button up, to avoid running through intro screen
	while (joyb[0])	
	{
		vic_waitFrame();
		joy_poll(0);
	}
}

