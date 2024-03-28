#include "playfield.h"
#include "display.h"
#include "levels.h"

char * const BackgroundMap0 = (char *)0xe000;
char * const BackgroundMap1 = (char *)0xf000;

const char level_row[48] = {
	#for (i, 48)	42 * (i >> 3), 
};

inline bool level_field(char cx, char cy)
{
	return (level_active->map[(char)(cy + level_row[cx])] & (0x80 >> (cx & 7))) != 0;
}

inline bool level_pixel_field(int x, int y)
{
	signed char cx = x >> 3;
	signed char cy = y >> 3;

	if (cx < 0 || cy < 0 || cx >= 48 || cy >= 42)
		return true;
	else
	 	return level_field(cx, cy);
}

inline bool level_sprite_field(int x, int y)
{
	char cx = (x - 24) >> 3;
	char cy = (y - 50) >> 3;

	if (cx >= 48 || cy >= 42)
		return true;
	else
		return level_field(cx, cy);
}


inline bool level_player_field(int x, int y)
{
	char cx = (x - 208) >> 7;
	char cy = (y - 632) >> 7;

	if (cx >= 48 || cy >= 42)
		return true;
	else
		return level_field(cx, cy);
}


template<int y>
inline void font_expand_2(char t, const char * mp0, const char * mp1)
{
	__assume(t <= 128);
	for(char i=0; i<16; i++)
	{
		char m0 = mp0[i], m1 = mp1[i];
		#pragma unroll(full)
		for(char j=0; j<8; j++)
			DynCharset[t++] = j < (7 - y) ? m0 : m1;
	}
}

void font_expand(char t, char x, char y)
{
	char m0 = 0xff << x;
	char m1 = ~m0;

	char mt0[16], mt1[16];
	#pragma unroll(full)
	for(char i=0; i<16; i++)
	{
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

void map_expand(char pi, char x, char y)
{
	const char * sp = BackgroundMap0;

	char * dp = Screen;
	if (pi)
	{
		dp += 22;
		sp = BackgroundMap1;
	}

	sp += 64 * y + x;

	for(char y=0; y<20; y++)
	{
		#pragma unroll(full)
		for(char x=0; x<18; x++)
			dp[x] = sp[x];

		dp += 40;
		sp += 64;
	}	
}

void back_init(void)
{
	for(char y=0; y<42; y++)
	{
		for(char x=0; x<48; x++)
		{
			bool	c0 = level_field(x, y);
			bool	c1 = level_field(x + 1, y);
			bool	c2 = level_field(x, y + 1);
			bool	c3 = level_field(x + 1, y + 1);

			char m = 0;
			if (c0)
				m |= 1;
			if (c1)
				m |= 2;
			if (c2)
				m |= 4;
			if (c3)
				m |= 8;

			BackgroundMap0[64 * y + x] = m | 0xe0;
			BackgroundMap1[64 * y + x] = m | 0xf0;
		}
	}
}

