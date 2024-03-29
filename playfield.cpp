#include "playfield.h"
#include "display.h"
#include "levels.h"

// Space for the two expanded maps (one for each player with different
// char indices)
char * const BackgroundMap0 = (char *)0xe000;
char * const BackgroundMap1 = (char *)0xf000;

// Table with index into level bitmap for the 48 x coordinates, bitmaps
// are organized with 8 bits horizontal, then 42 bytes vertical, then
// next row of 8 bits horizontal
const char level_row[48] = {
	#for (i, 48)	42 * (i >> 3), 
};

// Get the map status for map coordinates
inline bool level_field(char cx, char cy)
{
	return (level_active->map[(char)(cy + level_row[cx])] & (0x80 >> (cx & 7))) != 0;
}

// Get the map status for pixel coordinates
inline bool level_pixel_field(int x, int y)
{
	signed char cx = x >> 3;
	signed char cy = y >> 3;

	if (cx < 0 || cy < 0 || cx >= 48 || cy >= 42)
		return true;
	else
	 	return level_field(cx, cy);
}

// Get the map status for sprite coordinates
inline bool level_sprite_field(int x, int y)
{
	char cx = (x - 24) >> 3;
	char cy = (y - 50) >> 3;

	if (cx >= 48 || cy >= 42)
		return true;
	else
		return level_field(cx, cy);
}

// Get the map status for player coordinates
inline bool level_player_field(int x, int y)
{
	char cx = (x - 208) >> 7;
	char cy = (y - 632) >> 7;

	if (cx >= 48 || cy >= 42)
		return true;
	else
		return level_field(cx, cy);
}

// Templated font expand routine for smooth scrolling.  The template
// parameter will be expanded for the range 0 to 7 to generate speed code

template<int y>
inline void font_expand_2(char t, const char * mp0, const char * mp1)
{
	// Help the compiler to avoid 16 bit pointer arithmetic
	__assume(t <= 128);

	// Loop over all 16 custom characters
	for(char i=0; i<16; i++)
	{
		// upper and lower pixels
		char m0 = mp0[i], m1 = mp1[i];

		// Full unroll over the character, results in eight stores
		#pragma unroll(full)
		for(char j=0; j<8; j++)
			DynCharset[t++] = j < (7 - y) ? m0 : m1;
	}
}

void font_expand(char t, char x, char y)
{
	// Left and right part of the scroll characters based
	// on x offset
	char m0 = 0xff << x;
	char m1 = ~m0;

	// Upper and lower bitpatter for all 16 custom characters
	char mt0[16], mt1[16];

	// Full unroll results in sequence of stores
	#pragma unroll(full)
	for(char i=0; i<16; i++)
	{
		// Check bitpattern and build character patterns
		char w0 = 0, w1 = 0;
		if (i & 1)
			w0 |= m0;
		if (i & 2)
			w0 |= m1;
		if (i & 4)
			w1 |= m0;
		if (i & 8)
			w1 |= m1;
		mt0[i] = w0;
		mt1[i] = w1;
	}

	// Template expansion for all eight possible y offsets
	switch(y)
	{
	case 0: font_expand_2<0>(t, mt0, mt1); break;
	case 1: font_expand_2<1>(t, mt0, mt1); break;
	case 2: font_expand_2<2>(t, mt0, mt1); break;
	case 3: font_expand_2<3>(t, mt0, mt1); break;
	case 4: font_expand_2<4>(t, mt0, mt1); break;
	case 5: font_expand_2<5>(t, mt0, mt1); break;
	case 6: font_expand_2<6>(t, mt0, mt1); break;
	case 7: font_expand_2<7>(t, mt0, mt1); break;		
	}
}

// Display char map in players split view
void map_expand(char pi, char x, char y)
{
	const char * sp = BackgroundMap0;

	char * dp = Screen;

	// Check for right side player
	if (pi)
	{
		dp += 22;
		sp = BackgroundMap1;
	}

	// Source position in map
	sp += 64 * y + x;

	// Copy 18x20 rect to screen
	for(char y=0; y<20; y++)
	{
		#pragma unroll(full)
		for(char x=0; x<18; x++)
			dp[x] = sp[x];

		dp += 40;
		sp += 64;
	}	
}

// Expand level bitfield to individual charmaps for both players
void back_init(void)
{
	// For all rows
	for(char y=0; y<42; y++)
	{
		// For all columns
		for(char x=0; x<48; x++)
		{
			// Get the four map elements that may contribute to
			// this character
			bool	c0 = level_field(x, y);
			bool	c1 = level_field(x + 1, y);
			bool	c2 = level_field(x, y + 1);
			bool	c3 = level_field(x + 1, y + 1);

			// Build character base index based on bits for each map element
			char m = 0;
			if (c0)
				m |= 1;
			if (c1)
				m |= 2;
			if (c2)
				m |= 4;
			if (c3)
				m |= 8;

			// Charset regions for left and right view
			BackgroundMap0[64 * y + x] = m | 0xe0;
			BackgroundMap1[64 * y + x] = m | 0xf0;
		}
	}
}

