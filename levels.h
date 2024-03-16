#pragma once

struct LevelData
{
	char	map[6 * 42];
	char	x0, y0, x1, y1;
};

extern const LevelData	level1;
extern const LevelData	level2;
extern const LevelData	level3;
extern const LevelData	level4;

extern const LevelData	*	level_active;



#pragma compile("levels.cpp")
