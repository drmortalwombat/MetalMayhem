#pragma once

struct LevelData
{
	char	map[6 * 42];
	char	x0, y0, x1, y1;
};

extern const LevelData	level1;


#pragma compile("levels.cpp")
