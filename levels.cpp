#include "levels.h"

const LevelData	*	level_active;

constexpr LevelData level_expand(const char * d, char x0, char y0, char x1, char y1)
{
	LevelData	ld;

	ld.x0 = x0;
	ld.y0 = y0;
	ld.x1 = x1;
	ld.y1 = y1;

	for(int y=0; y<42; y++)
	{
		for(int x=0; x<6; x++)
		{
			char m = 0;
			for(int i=0; i<8; i++)
			{
				if (d[48 * y + 8 * x + i])
					m |= 0x80 >> i;
			}
			ld.map[42 * x + y] = m;
		}
	}

	return ld;
}

const char LevelMap1[] = {
	#embed ctm_map8 "playfield1.ctm"
};

const LevelData	level1 = level_expand(LevelMap1, 1, 1, 44, 38);

const char LevelMap2[] = {
	#embed ctm_map8 "playfield2.ctm"
};

const LevelData	level2 = level_expand(LevelMap2, 1, 1, 44, 38);

const char LevelMap3[] = {
	#embed ctm_map8 "playfield3.ctm"
};

const LevelData	level3 = level_expand(LevelMap3, 1, 1, 44, 38);

const char LevelMap4[] = {
	#embed ctm_map8 "playfield4.ctm"
};

const LevelData	level4 = level_expand(LevelMap4, 7, 15, 38, 24);
